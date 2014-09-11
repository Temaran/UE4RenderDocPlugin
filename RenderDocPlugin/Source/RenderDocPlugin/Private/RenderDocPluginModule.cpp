#include "RenderDocPluginPrivatePCH.h" 
#include "RendererInterface.h"
#include "RenderDocPluginModule.h"

void FRenderDocPluginModule::StartupModule()
{
	//Load DLL
	FString BinaryPath;
	if (GConfig)
	{
		GConfig->GetString(TEXT("RenderDoc"), TEXT("BinaryPath"), BinaryPath, GGameIni);
	}
	FString PathToRenderDocDLL = FPaths::Combine(*BinaryPath, *FString("renderdoc.dll"));

	RenderDocDLL = NULL;
	RenderDocDLL = GetModuleHandle(*PathToRenderDocDLL);
	if (BinaryPath.IsEmpty() || !RenderDocDLL)
	{
		UE_LOG(RenderDocPlugin, Error, TEXT("Could not find the renderdoc DLL, have you loaded the RenderDocLoaderPlugin?"));
		return;
	}

	//Init function pointers
	RenderDocGetAPIVersion = (pRENDERDOC_GetAPIVersion)GetRenderDocFunctionPointer(RenderDocDLL, "RENDERDOC_GetAPIVersion");
	RenderDocSetLogFile = (pRENDERDOC_SetLogFile)GetRenderDocFunctionPointer(RenderDocDLL, "RENDERDOC_SetLogFile");
	RenderDocSetCaptureOptions = (pRENDERDOC_SetCaptureOptions)GetRenderDocFunctionPointer(RenderDocDLL, "RENDERDOC_SetCaptureOptions");
	RenderDocSetActiveWindow = (pRENDERDOC_SetActiveWindow)GetRenderDocFunctionPointer(RenderDocDLL, "RENDERDOC_SetActiveWindow");
	RenderDocTriggerCapture = (pRENDERDOC_TriggerCapture)GetRenderDocFunctionPointer(RenderDocDLL, "RENDERDOC_TriggerCapture");
	RenderDocStartFrameCapture = (pRENDERDOC_StartFrameCapture)GetRenderDocFunctionPointer(RenderDocDLL, "RENDERDOC_StartFrameCapture");
	RenderDocEndFrameCapture = (pRENDERDOC_EndFrameCapture)GetRenderDocFunctionPointer(RenderDocDLL, "RENDERDOC_EndFrameCapture");
	RenderDocGetOverlayBits = (pRENDERDOC_GetOverlayBits)GetRenderDocFunctionPointer(RenderDocDLL, "RENDERDOC_GetOverlayBits");
	RenderDocMaskOverlayBits = (pRENDERDOC_MaskOverlayBits)GetRenderDocFunctionPointer(RenderDocDLL, "RENDERDOC_MaskOverlayBits");
	RenderDocInitRemoteAccess = (pRENDERDOC_InitRemoteAccess)GetRenderDocFunctionPointer(RenderDocDLL, "RENDERDOC_InitRemoteAccess");

	//Set capture settings
	FString RenderDocCapturePath = FPaths::Combine(*FPaths::GameSavedDir(), *FString("RenderDocCaptures"));
	if (!IFileManager::Get().DirectoryExists(*RenderDocCapturePath))
	{
		IFileManager::Get().MakeDirectory(*RenderDocCapturePath, true);
	}

	FString CapturePath = FPaths::Combine(*RenderDocCapturePath, *FDateTime::Now().ToString());
	CapturePath = FPaths::ConvertRelativePathToFull(CapturePath);
	FPaths::NormalizeDirectoryName(CapturePath);
	RenderDocSetLogFile(*CapturePath);

	//Init remote access
	SocketPort = 0;
	RenderDocInitRemoteAccess(&SocketPort);

	//Init UI
	int32 RenderDocVersion = RenderDocGetAPIVersion();
	UE_LOG(RenderDocPlugin, Log, TEXT("RenderDoc plugin started! Your renderdoc installation is v%i"), RenderDocVersion);

	FRenderDocPluginStyle::Initialize();
	FRenderDocPluginCommands::Register();

	RenderDocPluginCommands = MakeShareable(new FUICommandList);

	RenderDocPluginCommands->MapAction(FRenderDocPluginCommands::Get().CaptureFrameButton,
		FExecuteAction::CreateRaw(this, &FRenderDocPluginModule::CaptureCurrentViewport),
		FCanExecuteAction::CreateRaw(this, &FRenderDocPluginModule::CanCaptureCurrentViewport));

	ToolbarExtender = MakeShareable(new FExtender);
	ToolbarExtension = ToolbarExtender->AddToolBarExtension("CameraSpeed", EExtensionHook::After, RenderDocPluginCommands,
		FToolBarExtensionDelegate::CreateRaw(this, &FRenderDocPluginModule::AddToolbarExtension));

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);

	ExtensionManager = LevelEditorModule.GetToolBarExtensibilityManager();

	RenderDocMaskOverlayBits(eOverlay_None, eOverlay_None);

	RenderDocRunner = new FRenderDocRunner();
}

void FRenderDocPluginModule::CaptureCurrentViewport()
{
	UE_LOG(RenderDocPlugin, Log, TEXT("Capture frame and launch renderdoc!"));

	CaptureFrame();
	LaunchRenderDoc();
}

bool FRenderDocPluginModule::CanCaptureCurrentViewport()
{
	return true;
}

void FRenderDocPluginModule::CaptureFrame()
{
	HWND ActiveWindowHandle = GetActiveWindow();
	FViewport* ActiveViewport = GEditor->GetActiveViewport();

	RenderDocSetActiveWindow(ActiveWindowHandle);
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		StartRenderDocCapture,
		HWND, WindowHandle, ActiveWindowHandle,
		pRENDERDOC_StartFrameCapture, StartFrameCapture, RenderDocStartFrameCapture,
		{
		StartFrameCapture(WindowHandle);
	});

	ActiveViewport->Draw(true);

	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		EndRenderDocCapture,
		HWND, WindowHandle, ActiveWindowHandle,
		pRENDERDOC_EndFrameCapture, EndFrameCapture, RenderDocEndFrameCapture,
		{
		EndFrameCapture(WindowHandle);
	});
}

void FRenderDocPluginModule::LaunchRenderDoc()
{
	FString BinaryPath;
	if (GConfig)
	{
		GConfig->GetString(TEXT("RenderDoc"), TEXT("BinaryPath"), BinaryPath, GGameIni);
	}

	RenderDocRunner->StartRenderDoc(FPaths::Combine(*BinaryPath, *FString("renderdocui.exe"))
		, FPaths::Combine(*FPaths::GameSavedDir(), *FString("RenderDocCaptures"))
		, SocketPort);
}

void* FRenderDocPluginModule::GetRenderDocFunctionPointer(HINSTANCE ModuleHandle, LPCSTR FunctionName)
{
	void* OutTarget = NULL;
	OutTarget = (void*)GetProcAddress(ModuleHandle, FunctionName);

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
	ToolbarBuilder.AddToolBarButton(FRenderDocPluginCommands::Get().CaptureFrameButton, NAME_None, LOCTEXT("MyButton_Override", "Capture Frame"), LOCTEXT("MyButton_ToolTipOverride", "Captures the next frame and launches the renderdoc UI"), IconBrush, NAME_None);
	ToolbarBuilder.EndSection();

#undef LOCTEXT_NAMESPACE
}

void FRenderDocPluginModule::ShutdownModule()
{
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
}

IMPLEMENT_MODULE(FRenderDocPluginModule, RenderDocPlugin)