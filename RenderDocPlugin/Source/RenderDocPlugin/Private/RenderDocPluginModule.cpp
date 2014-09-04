#include "RenderDocPluginPrivatePCH.h" 
#include "RenderDocPluginModule.h"
#include "RenderDocRunner.h"

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
	RenderDocTriggerCapture = NULL;
	RenderDocSetLogFile = NULL;
	RenderDocTriggerCapture = (pRENDERDOC_TriggerCapture)(void*)GetProcAddress(RenderDocDLL, "RENDERDOC_TriggerCapture");
	RenderDocSetLogFile = (pRENDERDOC_SetLogFile)(void*)GetProcAddress(RenderDocDLL, "RENDERDOC_SetLogFile");

	if (!RenderDocTriggerCapture || !RenderDocSetLogFile)
	{
		UE_LOG(RenderDocPlugin, Error, TEXT("Could not load Renderdoc function pointers, the function names might have changed, or you are using an incompatible version of Renderdoc"));
		return;
	}
	
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

	//Init UI
	UE_LOG(RenderDocPlugin, Log, TEXT("RenderDoc plugin started!"));
	
	FRenderDocPluginStyle::Initialize();
	FRenderDocPluginCommands::Register();

	RenderDocPluginCommands = MakeShareable(new FUICommandList);

	RenderDocPluginCommands->MapAction(FRenderDocPluginCommands::Get().CaptureFrameButton,
		FExecuteAction::CreateRaw(this, &FRenderDocPluginModule::CaptureNextFrameAndLaunchUI),
		FCanExecuteAction::CreateRaw(this, &FRenderDocPluginModule::CanCaptureNextFrameAndLaunchUI));

	ToolbarExtender = MakeShareable(new FExtender);       //"Debugging" ?
	ToolbarExtension = ToolbarExtender->AddToolBarExtension("Game", EExtensionHook::After, RenderDocPluginCommands,
		FToolBarExtensionDelegate::CreateRaw(this, &FRenderDocPluginModule::AddToolbarExtension));

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);

	ExtensionManager = LevelEditorModule.GetToolBarExtensibilityManager();
}

void FRenderDocPluginModule::CaptureNextFrameAndLaunchUI()
{
	UE_LOG(RenderDocPlugin, Log, TEXT("Capture frame and launch renderdoc!"));

	RenderDocTriggerCapture();
	
	FString BinaryPath;
	if (GConfig)
	{
		GConfig->GetString(TEXT("RenderDoc"), TEXT("BinaryPath"), BinaryPath, GGameIni);
	}
	FRenderDocRunner::LaunchRenderDoc(FPaths::Combine(*BinaryPath, *FString("renderdocui.exe"))
									, FPaths::Combine(*FPaths::GameSavedDir(), *FString("RenderDocCaptures")));
}

bool FRenderDocPluginModule::CanCaptureNextFrameAndLaunchUI()
{
	return true;
}

void FRenderDocPluginModule::AddToolbarExtension(FToolBarBuilder& ToolbarBuilder)
{
#define LOCTEXT_NAMESPACE "LevelEditorToolBar"

	UE_LOG(RenderDocPlugin, Log, TEXT("Starting extension..."));
	FSlateIcon IconBrush = FSlateIcon(FRenderDocPluginStyle::Get()->GetStyleSetName(), "RenderDocPlugin.CaptureFrameIcon");
	ToolbarBuilder.AddToolBarButton(FRenderDocPluginCommands::Get().CaptureFrameButton, NAME_None, LOCTEXT("MyButton_Override", "Capture Frame"), LOCTEXT("MyButton_ToolTipOverride", "Captures the next frame and launches the renderdoc UI"), IconBrush, NAME_None);

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