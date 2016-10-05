/******************************************************************************
* The MIT License (MIT)
*
* Copyright (c) 2014 Fredrik Lindh
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
******************************************************************************/

#include "RenderDocPluginPrivatePCH.h"
#include "RenderDocPluginModule.h"

#include "Internationalization.h"
#include "RendererInterface.h"

#include "RenderDocPluginNotification.h"

DEFINE_LOG_CATEGORY(RenderDocPlugin);

#define LOCTEXT_NAMESPACE "RenderDocPlugin"



/**
* A dummy input device in order to be able to listen and respond to engine tick
* events. The whole rendering activity between two engine ticks can be captured
* including SceneCapture updates, Material Editor previews, Material Thumbnail
* previews, Editor UI (Slate) widget rendering, etc.
*/
class FRenderDocPluginModule::FRenderDocDummyInputDevice : public IInputDevice
{
public:
	//FRenderDocDummyInputDevice(const TSharedRef< FGenericApplicationMessageHandler >& MessageHandler) : ThePlugin(NULL) { }
	FRenderDocDummyInputDevice() : ThePlugin(NULL) { }
	virtual ~FRenderDocDummyInputDevice() { }

	/** Tick the interface (used for controlling full engine frame captures). */
	virtual void Tick(float DeltaTime) override
	{
		check(ThePlugin);
		ThePlugin->Tick(DeltaTime);
	}

	/** The remaining interfaces are irrelevant for this dummy input device. */
	virtual void SendControllerEvents() override { }
	virtual void SetMessageHandler(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler) override { }
	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override { return(false); }
	virtual void SetChannelValue(int32 ControllerId, FForceFeedbackChannelType ChannelType, float Value) override { }
	virtual void SetChannelValues(int32 ControllerId, const FForceFeedbackValues &values) override { }

private:
	friend class FRenderDocPluginModule;
	FRenderDocPluginModule* ThePlugin;

};

TSharedPtr< class IInputDevice > FRenderDocPluginModule::CreateInputDevice(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler)
{
	UE_LOG(RenderDocPlugin, Log, TEXT("Creating dummy input device (for intercepting engine ticks)"));
	FRenderDocDummyInputDevice* InputDev = new FRenderDocDummyInputDevice();
	InputDev->ThePlugin = this;
	return( MakeShareable(InputDev) );
}



void* GetRenderDocLibrary()
{
	void* RenderDocDLL (NULL);
	if (GConfig)
	{
		FString RenderdocPath;
		GConfig->GetString(TEXT("RenderDoc"), TEXT("BinaryPath"), RenderdocPath, GGameIni);
		FString PathToRenderDocDLL = FPaths::Combine(*RenderdocPath, *FString("renderdoc.dll"));
		RenderDocDLL = FPlatformProcess::GetDllHandle(*PathToRenderDocDLL);
	}
	return(RenderDocDLL);
}



void FRenderDocPluginModule::StartupModule()
{
	Loader.Initialize();

	if (!Loader.RenderDocAPI)
		return;

	// Regrettably, GUsingNullRHI is set to true AFTER the PostInitConfig modules
	// have been loaded (RenderDoc plugin being one of them). When this code runs
	// the following condition will never be true, so it must be tested again in
	// the Toolbar initialization code.
	if (GUsingNullRHI)
	{
		UE_LOG(RenderDocPlugin, Warning, TEXT("RenderDoc Plugin will not be loaded because a Null RHI (Cook Server, perhaps) is being used."));
		return;
	}

	// Obtain a handle to the RenderDoc DLL that has been loaded by the RenderDoc
	// Loader Plugin; no need for error handling here since the Loader would have
	// already handled and logged these errors (but check() them just in case...)
	RenderDocAPI = Loader.RenderDocAPI;
	check(RenderDocAPI);

	IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);
	TickNumber = 0;

	// Setup RenderDoc settings
	FString RenderDocCapturePath = FPaths::Combine(*FPaths::GameSavedDir(), *FString("RenderDocCaptures"));
	if (!IFileManager::Get().DirectoryExists(*RenderDocCapturePath))
	{
		IFileManager::Get().MakeDirectory(*RenderDocCapturePath, true);
	}

	FString CapturePath = FPaths::Combine(*RenderDocCapturePath, *FDateTime::Now().ToString());
	CapturePath = FPaths::ConvertRelativePathToFull(CapturePath);
	FPaths::NormalizeDirectoryName(CapturePath);
	
	if (sizeof(TCHAR) == sizeof(char))
		RenderDocAPI->SetLogFilePathTemplate((const char*)*CapturePath);
	else
		RenderDocAPI->SetLogFilePathTemplate(TCHAR_TO_ANSI(*CapturePath));

	RenderDocAPI->SetFocusToggleKeys(NULL, 0);
	RenderDocAPI->SetCaptureKeys(NULL, 0);

	RenderDocAPI->SetCaptureOptionU32(eRENDERDOC_Option_CaptureCallstacks, RenderDocSettings.bCaptureCallStacks  ? 1 : 0);
	RenderDocAPI->SetCaptureOptionU32(eRENDERDOC_Option_RefAllResources,   RenderDocSettings.bRefAllResources    ? 1 : 0);
	RenderDocAPI->SetCaptureOptionU32(eRENDERDOC_Option_SaveAllInitials,   RenderDocSettings.bSaveAllInitials    ? 1 : 0);

	RenderDocAPI->MaskOverlayBits(eRENDERDOC_Overlay_None, eRENDERDOC_Overlay_None);

#if WITH_EDITOR
	EditorExtensions = new FRenderDocPluginEditorExtension (this, &RenderDocSettings);
#endif//WITH_EDITOR

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	static FAutoConsoleCommand CCmdRenderDocCaptureFrame = FAutoConsoleCommand(
		TEXT("RenderDoc.CaptureFrame"),
		TEXT("Captures the rendering commands of the next frame and launches RenderDoc"),
		FConsoleCommandDelegate::CreateRaw(this, &FRenderDocPluginModule::CaptureFrame));
#endif

	UE_LOG(RenderDocPlugin, Log, TEXT("RenderDoc plugin is ready!"));
}

class FRenderDocPluginModule::FrameCapturer
{
public:
	static void BeginCapture(HWND WindowHandle, FRenderDocPluginLoader::RENDERDOC_API_CONTEXT* RenderDocAPI, FRenderDocPluginModule* Plugin)
	{
		Plugin->UE4_OverrideDrawEventsFlag();
		RENDERDOC_DevicePointer Device = GDynamicRHI->RHIGetNativeDevice();
		RenderDocAPI->StartFrameCapture(Device, WindowHandle);
	}
	static void EndCapture(HWND WindowHandle, FRenderDocPluginLoader::RENDERDOC_API_CONTEXT* RenderDocAPI, FRenderDocPluginModule* Plugin)
	{
		RENDERDOC_DevicePointer Device = GDynamicRHI->RHIGetNativeDevice();
		RenderDocAPI->EndFrameCapture(Device, WindowHandle);
		Plugin->UE4_RestoreDrawEventsFlag();

		Plugin->RunAsyncTask(ENamedThreads::GameThread, [Plugin]()
		{
			Plugin->StartRenderDoc(FPaths::Combine(*FPaths::GameSavedDir(), *FString("RenderDocCaptures")));
		});
	}
};

void FRenderDocPluginModule::BeginCapture()
{
	UE_LOG(RenderDocPlugin, Log, TEXT("Capture frame and launch renderdoc!"));
#if WITH_EDITOR
	FRenderDocPluginNotification::Get().ShowNotification(NSLOCTEXT("LaunchRenderDocGUI", "LaunchRenderDocGUIShow", "Capturing frame"));
#else
	// TODO: if there is no editor, notify via game viewport text
#endif//WITH_EDITOR

	// TODO: maybe move these SetOptions() to FRenderDocPluginSettings...
	pRENDERDOC_SetCaptureOptionU32 SetOptions = Loader.RenderDocAPI->SetCaptureOptionU32;
	int ok = SetOptions(eRENDERDOC_Option_CaptureCallstacks, RenderDocSettings.bCaptureCallStacks ? 1 : 0); check(ok);
	    ok = SetOptions(eRENDERDOC_Option_RefAllResources,   RenderDocSettings.bRefAllResources   ? 1 : 0); check(ok);
	    ok = SetOptions(eRENDERDOC_Option_SaveAllInitials,   RenderDocSettings.bSaveAllInitials   ? 1 : 0); check(ok);

	HWND WindowHandle = GetActiveWindow();

	typedef FRenderDocPluginLoader::RENDERDOC_API_CONTEXT RENDERDOC_API_CONTEXT;
	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
		StartRenderDocCapture,
		HWND, WindowHandle, WindowHandle,
		RENDERDOC_API_CONTEXT*, RenderDocAPI, RenderDocAPI,
		FRenderDocPluginModule*, Plugin, this,
		{
			FrameCapturer::BeginCapture(WindowHandle, RenderDocAPI, Plugin);
		});
}

void FRenderDocPluginModule::EndCapture()
{
	HWND WindowHandle = GetActiveWindow();

	typedef FRenderDocPluginLoader::RENDERDOC_API_CONTEXT RENDERDOC_API_CONTEXT;
	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
		EndRenderDocCapture,
		HWND, WindowHandle, WindowHandle,
		RENDERDOC_API_CONTEXT*, RenderDocAPI, RenderDocAPI,
		FRenderDocPluginModule*, Plugin, this,
		{
			FrameCapturer::EndCapture(WindowHandle, RenderDocAPI, Plugin); return;
		});
}

void FRenderDocPluginModule::CaptureFrame()
{
	if (RenderDocSettings.bCaptureAllActivity)
		CaptureEntireFrame();
	else
		CaptureCurrentViewport();
}

void FRenderDocPluginModule::CaptureCurrentViewport()
{
	BeginCapture();

	// infer the intended viewport to intercept/capture:
	FViewport* Viewport (NULL);
	check(GEngine);
	if (!Viewport && GEngine->GameViewport)
	{
		check(GEngine->GameViewport->Viewport);
		if (GEngine->GameViewport->Viewport->HasFocus())
			Viewport = GEngine->GameViewport->Viewport;
	}
#if WITH_EDITOR
	if (!Viewport && GEditor)
	{
		// WARNING: capturing from a "PIE-Eject" Editor viewport will not work as
		// expected; in such case, capture via the console command
		// (this has something to do with the 'active' editor viewport when the UI
		// button is clicked versus the one which the console is attached to)
		Viewport = GEditor->GetActiveViewport();
	}
#endif//WITH_EDITOR
	check(Viewport);
	Viewport->Draw(true);

	EndCapture();
}

void FRenderDocPluginModule::CaptureEntireFrame()
{
	// Are we already in thw workings of capturing an entire engine frame?
	if (TickNumber != 0)
		return;

	// Begin tracking the global tick counter so that the Tick() method below can
	// identify the beginning and end of a complete engine update cycle:
	TickNumber = GFrameCounter;
	// NOTE: GFrameCounter counts engine ticks, while GFrameNumber counts render
	// frames. Multiple frames might get rendered in a single engine update tick.
	// All active windows are updated, in a round-robin fashion, within a single
	// engine tick. This includes thumbnail images for material preview, material
	// editor previews, cascade/persona previes, etc.
}

void FRenderDocPluginModule::Tick(float DeltaTime)
{
	if (TickNumber == 0)
		return;

	const uint32 TickDiff = GFrameCounter - TickNumber;
	const uint32 MaxCount = 2;

	check(TickDiff <= MaxCount);

	if (TickDiff == 1)
		BeginCapture();

	if (TickDiff == MaxCount)
		EndCapture(),
		TickNumber = 0;
}

void FRenderDocPluginModule::StartRenderDoc(FString FrameCaptureBaseDirectory)
{
#if WITH_EDITOR
	FRenderDocPluginNotification::Get().ShowNotification( NSLOCTEXT("LaunchRenderDocGUI", "LaunchRenderDocGUIShow", "Launching RenderDoc GUI") );
#else
	// TODO: if there is no editor, notify via game viewport text
#endif//WITH_EDITOR

	FString NewestCapture = GetNewestCapture(FrameCaptureBaseDirectory);
	FString ArgumentString = FString::Printf(TEXT("\"%s\""), *FPaths::ConvertRelativePathToFull(NewestCapture).Append(TEXT(".log")));

	if (!NewestCapture.IsEmpty())
	{
		// This is the new, recommended way of launching the RenderDoc GUI:
		if (!RenderDocAPI->IsRemoteAccessConnected())
		{
			uint32 PID = (sizeof(TCHAR) == sizeof(char)) ?
			  RenderDocAPI->LaunchReplayUI(true, (const char*)(*ArgumentString))
			: RenderDocAPI->LaunchReplayUI(true, TCHAR_TO_ANSI(*ArgumentString));

		if (0 == PID)
			UE_LOG(LogTemp, Error, TEXT("Could not launch RenderDoc!!"));
		}
	}

#if WITH_EDITOR
	FRenderDocPluginNotification::Get().ShowNotification( NSLOCTEXT("LaunchRenderDocGUI", "LaunchRenderDocGUIHide", "RenderDoc GUI Launched!") );
#else
	// TODO: if there is no editor, notify via game viewport text
#endif//WITH_EDITOR
}

FString FRenderDocPluginModule::GetNewestCapture(FString BaseDirectory)
{
	char LogFile[512];
	uint64_t Timestamp;
	uint32_t LogPathLength = 512;
	uint32_t Index = 0;
	FString OutString;
	
	while (RenderDocAPI->GetCapture(Index, LogFile, &LogPathLength, &Timestamp))
	{
		if (sizeof(TCHAR) == sizeof(char))
			OutString = FString(LogPathLength, (TCHAR*)LogFile);
		else
			OutString = FString(LogPathLength, ANSI_TO_TCHAR(LogFile));

		Index++;
	}
	
	return OutString;
}

void FRenderDocPluginModule::ShutdownModule()
{
	if (GUsingNullRHI)
		return;

#if WITH_EDITOR
	delete(EditorExtensions);
#endif//WITH_EDITOR

	Loader.Release();

	RenderDocAPI = NULL;
}

void FRenderDocPluginModule::UE4_OverrideDrawEventsFlag(const bool flag)
{
	//UE_LOG(RenderDocPlugin, Log, TEXT("Overriding GEmitDrawEvents flag"));
	//UE_LOG(RenderDocPlugin, Log, TEXT("  GEmitDrawEvents=%d"), GEmitDrawEvents);
	UE4_GEmitDrawEvents_BeforeCapture = GEmitDrawEvents;
	GEmitDrawEvents = flag;
	//UE_LOG(RenderDocPlugin, Log, TEXT("  GEmitDrawEvents=%d"), GEmitDrawEvents);
}

void FRenderDocPluginModule::UE4_RestoreDrawEventsFlag()
{
	//UE_LOG(RenderDocPlugin, Log, TEXT("Restoring GEmitDrawEvents flag"));
	//UE_LOG(RenderDocPlugin, Log, TEXT("  GEmitDrawEvents=%d"), GEmitDrawEvents);
	GEmitDrawEvents = UE4_GEmitDrawEvents_BeforeCapture;
	//UE_LOG(RenderDocPlugin, Log, TEXT("  GEmitDrawEvents=%d"), GEmitDrawEvents);
}

void FRenderDocPluginModule::RunAsyncTask(ENamedThreads::Type Where, TFunction<void()> What)
{
	struct FAsyncGraphTask : public FAsyncGraphTaskBase
	{
		ENamedThreads::Type TargetThread;
		TFunction<void()> TheTask;

		FAsyncGraphTask(ENamedThreads::Type Thread, TFunction<void()>&& Task) : TargetThread(Thread), TheTask(MoveTemp(Task)) { }
		void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) { TheTask(); }
		ENamedThreads::Type GetDesiredThread() { return(TargetThread); }
	};

	TGraphTask<FAsyncGraphTask>::CreateTask().ConstructAndDispatchWhenReady(Where, MoveTemp(What));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRenderDocPluginModule, RenderDocPlugin)
