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
#include "RendererInterface.h"
#include "RenderDocPluginModule.h"
#include "RenderDocPluginNotification.h"

const FName FRenderDocPluginModule::SettingsUITabName(TEXT("RenderDocSettingsUI"));

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
  Loader.StartupModule(this);
}

void FRenderDocPluginModule::Initialize()
{
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

	//Init UI
	FRenderDocPluginStyle::Initialize();
	FRenderDocPluginCommands::Register();

	// The LoadModule request below will crash if running as an editor commandlet!
	// ( the GUsingNullRHI check above should prevent this code from executing, but I am
	//   re-emphasizing it here since many plugins appear to be ignoring this condition... )
	check(!IsRunningCommandlet());
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	TSharedRef<FUICommandList> CommandBindings = LevelEditorModule.GetGlobalLevelEditorActions();
	CommandBindings->MapAction(FRenderDocPluginCommands::Get().CaptureFrame,
		FExecuteAction::CreateRaw(this, &FRenderDocPluginModule::CaptureFrame),
		FCanExecuteAction());
	CommandBindings->MapAction(FRenderDocPluginCommands::Get().OpenSettings,
		FExecuteAction::CreateRaw(this, &FRenderDocPluginModule::OpenSettingsEditorWindow),
		FCanExecuteAction());

	ExtensionManager = LevelEditorModule.GetToolBarExtensibilityManager();
	ToolbarExtender = MakeShareable(new FExtender);
	ToolbarExtension = ToolbarExtender->AddToolBarExtension("CameraSpeed", EExtensionHook::After, CommandBindings,
		FToolBarExtensionDelegate::CreateRaw(this, &FRenderDocPluginModule::AddToolbarExtension));
	ExtensionManager->AddExtender(ToolbarExtender);

	IsInitialized = false;
	FSlateRenderer* SlateRenderer = FSlateApplication::Get().GetRenderer().Get();
	LoadedDelegateHandle = SlateRenderer->OnSlateWindowRendered().AddRaw(this, &FRenderDocPluginModule::OnEditorLoaded);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	static FAutoConsoleCommand CCmdRenderDocCaptureFrame = FAutoConsoleCommand(
		TEXT("RenderDoc.CaptureFrame"),
		TEXT("Captures the rendering commands of the next frame and launches RenderDoc"),
		FConsoleCommandDelegate::CreateRaw(this, &FRenderDocPluginModule::CaptureFrame));
#endif

	UE_LOG(RenderDocPlugin, Log, TEXT("RenderDoc plugin is ready!"));
}

void FRenderDocPluginModule::OnEditorLoaded(SWindow& SlateWindow, void* ViewportRHIPtr)
{
	// would be nice to use the preprocessor definition WITH_EDITOR instead,
	// but the user may launch a standalone the game through the editor...
	if (!GEditor)
		return;

	// --> YAGER by SKrysanov 6/11/2014 : fixed crash on removing this callback in render thread.
	if (IsInGameThread())
	{
		FSlateRenderer* SlateRenderer = FSlateApplication::Get().GetRenderer().Get();
		SlateRenderer->OnSlateWindowRendered().Remove(LoadedDelegateHandle);
	}
	// <-- YAGER by SKrysanov 6/11/2014

	if (IsInitialized)
	{
		return;
	}
	IsInitialized = true;

	if (GConfig)
	{
		bool bGreetingHasBeenShown (false);
		GConfig->GetBool(TEXT("RenderDoc"), TEXT("GreetingHasBeenShown"), bGreetingHasBeenShown, GGameIni);
		if (!bGreetingHasBeenShown && GEditor)
		{
			GEditor->EditorAddModalWindow(SNew(SRenderDocPluginAboutWindow));
			GConfig->SetBool(TEXT("RenderDoc"), TEXT("GreetingHasBeenShown"), true, GGameIni);
		}
	}
}

void FRenderDocPluginModule::BeginCapture()
{
	UE_LOG(RenderDocPlugin, Log, TEXT("Capture frame and launch renderdoc!"));
	FRenderDocPluginNotification::Get().ShowNotification(NSLOCTEXT("LaunchRenderDocGUI", "LaunchRenderDocGUIShow", "Capturing frame"));

	HWND WindowHandle = GetActiveWindow();

	typedef FRenderDocLoaderPluginModule::RENDERDOC_API_CONTEXT RENDERDOC_API_CONTEXT;
	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
		StartRenderDocCapture,
		HWND, WindowHandle, WindowHandle,
		RENDERDOC_API_CONTEXT*, RenderDocAPI, RenderDocAPI,
		FRenderDocPluginModule*, Plugin, this,
		{
			Plugin->UE4_OverrideDrawEventsFlag();
			RENDERDOC_DevicePointer Device = GDynamicRHI->RHIGetNativeDevice();
			RenderDocAPI->StartFrameCapture(Device, WindowHandle);
		});
}

void FRenderDocPluginModule::EndCapture()
{
  HWND WindowHandle = GetActiveWindow();

	typedef FRenderDocLoaderPluginModule::RENDERDOC_API_CONTEXT RENDERDOC_API_CONTEXT;
	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
		EndRenderDocCapture,
		HWND, WindowHandle, WindowHandle,
		RENDERDOC_API_CONTEXT*, RenderDocAPI, RenderDocAPI,
		FRenderDocPluginModule*, Plugin, this,
		{
			RENDERDOC_DevicePointer Device = GDynamicRHI->RHIGetNativeDevice();
			RenderDocAPI->EndFrameCapture(Device, WindowHandle);
			Plugin->UE4_RestoreDrawEventsFlag();

			RunAsyncTask(ENamedThreads::GameThread, [this]()
			{
				Plugin->StartRenderDoc(FPaths::Combine(*FPaths::GameSavedDir(), *FString("RenderDocCaptures")));
			});
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
	if (!Viewport && GEditor)
	{
		// WARNING: capturing from a "PIE-Eject" Editor viewport will not work as
		// expected; in such case, capture via the console command
		// (this has something to do with the 'active' editor viewport when the UI
		// button is clicked versus the one which the console is attached to)
		Viewport = GEditor->GetActiveViewport();
	}
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
	check(TickDiff <= 2);

	if (TickDiff == 1)
		BeginCapture();

	if (TickDiff == 2)
		EndCapture(),
		TickNumber = 0;
}

void FRenderDocPluginModule::OpenSettingsEditorWindow()
{
	if (!GEditor)
		return;

	UE_LOG(RenderDocPlugin, Log, TEXT("Opening settings window"));

	TSharedPtr<SRenderDocPluginSettingsEditorWindow> Window = SNew(SRenderDocPluginSettingsEditorWindow)
		.Settings(RenderDocSettings)
		.ThePlugin(this);

	Window->MoveWindowTo(FSlateApplication::Get().GetCursorPos());
	GEditor->EditorAddModalWindow(Window.ToSharedRef());

	RenderDocSettings = Window->GetSettings();
}

void FRenderDocPluginModule::StartRenderDoc(FString FrameCaptureBaseDirectory)
{
	FRenderDocPluginNotification::Get().ShowNotification( NSLOCTEXT("LaunchRenderDocGUI", "LaunchRenderDocGUIShow", "Launching RenderDoc GUI") );

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

	FRenderDocPluginNotification::Get().ShowNotification( NSLOCTEXT("LaunchRenderDocGUI", "LaunchRenderDocGUIHide", "RenderDoc GUI Launched!") );
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

void FRenderDocPluginModule::AddToolbarExtension(FToolBarBuilder& ToolbarBuilder)
{
#define LOCTEXT_NAMESPACE "LevelEditorToolBar"

	UE_LOG(RenderDocPlugin, Log, TEXT("Attaching toolbar extension..."));
	ToolbarBuilder.AddSeparator();

	ToolbarBuilder.BeginSection("RenderdocPlugin");

	FSlateIcon IconBrush = FSlateIcon(FRenderDocPluginStyle::Get()->GetStyleSetName(), "RenderDocPlugin.CaptureFrameIcon.Small");
	ToolbarBuilder.AddToolBarButton(
		FRenderDocPluginCommands::Get().CaptureFrame,
		NAME_None,
		LOCTEXT("RenderDocCapture_Override", "Capture Frame"),
		LOCTEXT("RenderDocCapture_ToolTipOverride", "Captures the next frame and launches RenderDoc."),
		IconBrush,
		NAME_None);

	FSlateIcon SettingsIconBrush = FSlateIcon(FRenderDocPluginStyle::Get()->GetStyleSetName(), "RenderDocPlugin.SettingsIcon.Small");
	ToolbarBuilder.AddToolBarButton(
		FRenderDocPluginCommands::Get().OpenSettings,
		NAME_None,
		LOCTEXT("RenderDocCaptureSettings_Override", "Open Settings"),
		LOCTEXT("RenderDocCaptureSettings_ToolTipOverride", "Edit RenderDoc Settings"),
		SettingsIconBrush,
		NAME_None);

	ToolbarBuilder.EndSection();

#undef LOCTEXT_NAMESPACE
}

void FRenderDocPluginModule::ShutdownModule()
{
	if (GUsingNullRHI)
		return;

	if (ExtensionManager.IsValid())
	{
		FRenderDocPluginStyle::Shutdown();
		FRenderDocPluginCommands::Unregister();

		ToolbarExtender->RemoveExtension(ToolbarExtension.ToSharedRef());

		ExtensionManager->RemoveExtender(ToolbarExtender);
	}
	else
	{
		ExtensionManager.Reset();
	}

	// Unregister the tab spawner
	FGlobalTabmanager::Get()->UnregisterTabSpawner(SettingsUITabName);

	Loader.ShutdownModule();
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
