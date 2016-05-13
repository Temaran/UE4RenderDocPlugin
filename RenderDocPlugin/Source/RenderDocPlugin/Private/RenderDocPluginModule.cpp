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

static uint32 FrameNumber (0);
static FRenderDocPluginModule* ThePlugin (nullptr);

const FName FRenderDocPluginModule::SettingsUITabName(TEXT("RenderDocSettingsUI"));

#define LOCTEXT_NAMESPACE "RenderDocPlugin"

void* GetRenderDocLibrary(const FString& RenderdocPath)
{
	FString PathToRenderDocDLL = FPaths::Combine(*RenderdocPath, *FString("renderdoc.dll"));
	void* RenderDocDLL = FPlatformProcess::GetDllHandle(*PathToRenderDocDLL);
	return(RenderDocDLL);
}

void FRenderDocPluginModule::StartupModule()
{
	if (GUsingNullRHI)
	{
		UE_LOG(RenderDocPlugin, Warning, TEXT("RenderDoc Plugin will not be loaded because a Null RHI (Cook Server, perhaps) is being used."));
		return;
	}

	// Grab a handle to the RenderDoc DLL that has been loaded by the RenderDocLoaderPlugin:
	RenderDocDLL = NULL;
	if (GConfig)
	{
		FString RenderdocPath;
		GConfig->GetString(TEXT("RenderDoc"), TEXT("BinaryPath"), RenderdocPath, GGameIni);
		RenderDocDLL = GetRenderDocLibrary(RenderdocPath);
	}

	if (!RenderDocDLL)
	{
		UE_LOG(RenderDocPlugin, Error, TEXT("Could not find the renderdoc DLL: have you loaded the RenderDocLoaderPlugin?"));
		return;
	}

	// Initialize the RenderDoc API
	pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetRenderDocFunctionPointer(RenderDocDLL, TEXT("RENDERDOC_GetAPI"));
	if (0 == RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_0_0, (void**)&RENDERDOC))
	{
		UE_LOG(RenderDocPlugin, Error, TEXT("RenderDoc initialization failed."));
		return;
	}

  IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);
  ThePlugin = this;

	// Version checking
	int major(0), minor(0), patch(0);
	RENDERDOC->GetAPIVersion(&major, &minor, &patch);
	UE_LOG(RenderDocPlugin, Log, TEXT("RenderDoc v%i.%i.%i has been loaded."), major, minor, patch);

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
		RENDERDOC->SetLogFilePathTemplate((const char*)*CapturePath);
	else
		RENDERDOC->SetLogFilePathTemplate(TCHAR_TO_ANSI(*CapturePath));

	RENDERDOC->SetFocusToggleKeys(NULL, 0);
	RENDERDOC->SetCaptureKeys(NULL, 0);

	RENDERDOC->SetCaptureOptionU32(eRENDERDOC_Option_CaptureCallstacks, RenderDocSettings.bCaptureCallStacks  ? 1 : 0);
	RENDERDOC->SetCaptureOptionU32(eRENDERDOC_Option_RefAllResources,   RenderDocSettings.bRefAllResources    ? 1 : 0);
	RENDERDOC->SetCaptureOptionU32(eRENDERDOC_Option_SaveAllInitials,   RenderDocSettings.bSaveAllInitials    ? 1 : 0);

	RENDERDOC->MaskOverlayBits(eRENDERDOC_Overlay_None, eRENDERDOC_Overlay_None);

	//Init UI
	FRenderDocPluginStyle::Initialize();
	FRenderDocPluginCommands::Register();

	// The LoadModule request below will crash if running as an editor commandlet!
	// ( the GUsingNullRHI check above should prevent this code from executing, but I am
	//   re-emphasizing it here since many plugins appear to be ignoring this condition... )
	check(!IsRunningCommandlet());
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	TSharedRef<FUICommandList> CommandBindings = LevelEditorModule.GetGlobalLevelEditorActions();
	CommandBindings->MapAction(FRenderDocPluginCommands::Get().CaptureViewportFrame,
		FExecuteAction::CreateRaw(this, &FRenderDocPluginModule::CaptureCurrentViewport),
		FCanExecuteAction());
  CommandBindings->MapAction(FRenderDocPluginCommands::Get().CaptureEntireFrame,
    FExecuteAction::CreateRaw(this, &FRenderDocPluginModule::CaptureEntireFrame),
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
		FConsoleCommandDelegate::CreateRaw(this, &FRenderDocPluginModule::CaptureCurrentViewport));
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

	UE_LOG(RenderDocPlugin, Log, TEXT("RenderDoc plugin initialized!"));
}

void FRenderDocPluginModule::BeginCapture()
{
	HWND WindowHandle = GetActiveWindow();

	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
		StartRenderDocCapture,
		HWND, WindowHandle, WindowHandle,
		RENDERDOC_API_CONTEXT*, RENDERDOC, RENDERDOC,
		FRenderDocPluginModule*, Plugin, this,
		{
			Plugin->UE4_OverrideDrawEventsFlag();
			RENDERDOC_DevicePointer Device = GDynamicRHI->RHIGetNativeDevice();
			RENDERDOC->StartFrameCapture(Device, WindowHandle);
		});
}

void FRenderDocPluginModule::EndCapture()
{
  HWND WindowHandle = GetActiveWindow();

	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
		EndRenderDocCapture,
		HWND, WindowHandle, WindowHandle,
		RENDERDOC_API_CONTEXT*, RENDERDOC, RENDERDOC,
		FRenderDocPluginModule*, Plugin, this,
		{
			RENDERDOC_DevicePointer Device = GDynamicRHI->RHIGetNativeDevice();
			RENDERDOC->EndFrameCapture(Device, WindowHandle);
			Plugin->UE4_RestoreDrawEventsFlag();

      RunAsyncTask(ENamedThreads::GameThread, [this]()
      {
        Plugin->StartRenderDoc(FPaths::Combine(*FPaths::GameSavedDir(), *FString("RenderDocCaptures")));
      });
		});
}

void FRenderDocPluginModule::CaptureCurrentViewport()
{
	UE_LOG(RenderDocPlugin, Log, TEXT("Capture frame and launch renderdoc!"));

	FRenderDocPluginNotification::Get().ShowNotification( NSLOCTEXT("LaunchRenderDocGUI", "LaunchRenderDocGUIShow", "Capturing frame") );

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
  if (FrameNumber != 0)
    return;
  FrameNumber = GFrameNumber;
}

void FRenderDocPluginModule::OpenSettingsEditorWindow()
{
	if (!GEditor)
		return;

	UE_LOG(RenderDocPlugin, Log, TEXT("Opening settings window"));

	TSharedPtr<SRenderDocPluginSettingsEditorWindow> Window = SNew(SRenderDocPluginSettingsEditorWindow)
		.Settings(RenderDocSettings)
		.SetCaptureOptions(RENDERDOC->SetCaptureOptionU32);

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
		if (!RENDERDOC->IsRemoteAccessConnected())
		{
		  uint32 PID = (sizeof(TCHAR) == sizeof(char)) ?
			  RENDERDOC->LaunchReplayUI(true, (const char*)(*ArgumentString))
			: RENDERDOC->LaunchReplayUI(true, TCHAR_TO_ANSI(*ArgumentString));

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
	
	while (RENDERDOC->GetCapture(Index, LogFile, &LogPathLength, &Timestamp))
	{
		if (sizeof(TCHAR) == sizeof(char))
			OutString = FString(LogPathLength, (TCHAR*)LogFile);
		else
			OutString = FString(LogPathLength, ANSI_TO_TCHAR(LogFile));

		Index++;
	}
	
	return OutString;
}

void* FRenderDocPluginModule::GetRenderDocFunctionPointer(void* ModuleHandle, const TCHAR* FunctionName)
{
	void* OutTarget = FPlatformProcess::GetDllExport(ModuleHandle, FunctionName);

	if (!OutTarget)
	{
		UE_LOG(RenderDocPlugin, Error, TEXT("Could not load renderdoc function %s. You are most likely using an incompatible version of Renderdoc"), FunctionName);
	}

	check(OutTarget);
	return OutTarget;
}

void FRenderDocPluginModule::AddToolbarExtension(FToolBarBuilder& ToolbarBuilder)
{
#define LOCTEXT_NAMESPACE "LevelEditorToolBar"

	UE_LOG(RenderDocPlugin, Log, TEXT("Starting extension..."));
	ToolbarBuilder.AddSeparator();

	ToolbarBuilder.BeginSection("RenderdocPlugin");

	FSlateIcon IconBrush = FSlateIcon(FRenderDocPluginStyle::Get()->GetStyleSetName(), "RenderDocPlugin.CaptureFrameIcon.Small");
	ToolbarBuilder.AddToolBarButton(
		FRenderDocPluginCommands::Get().CaptureViewportFrame,
		NAME_None,
		LOCTEXT("RenderDocCapture_Override", "CaptureViewportFrame"),
		LOCTEXT("RenderDocCapture_ToolTipOverride", "Captures the next frame of this viewport and launches RenderDoc."),
		IconBrush,
		NAME_None);
	ToolbarBuilder.AddToolBarButton(
		FRenderDocPluginCommands::Get().CaptureEntireFrame,
		NAME_None,
		LOCTEXT("RenderDocCapture_Override", "CaptureEntireFrame"),
		LOCTEXT("RenderDocCapture_ToolTipOverride", "Captures the next frame (including Editor Slate UI and SceneCaptures) and launches RenderDoc."),
		IconBrush,
		NAME_None);

	FSlateIcon SettingsIconBrush = FSlateIcon(FRenderDocPluginStyle::Get()->GetStyleSetName(), "RenderDocPlugin.SettingsIcon.Small");
	ToolbarBuilder.AddToolBarButton(
		FRenderDocPluginCommands::Get().OpenSettings,
		NAME_None,
		LOCTEXT("RenderDocCaptureSettings_Override", "OpenSettings"),
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

  if (RenderDocDLL)
    FPlatformProcess::FreeDllHandle(RenderDocDLL);
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


void FRenderDocDummyInputDevice::Tick(float DeltaTime)
{
  if (FrameNumber == 0)
    return;

  if (GFrameNumber == FrameNumber + 1)
    ThePlugin->BeginCapture();

  if (GFrameNumber == FrameNumber + 2)
    ThePlugin->EndCapture(),
    FrameNumber = 0;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRenderDocPluginModule, RenderDocPlugin)
