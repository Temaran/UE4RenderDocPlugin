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
		FCanExecuteAction());

	ToolbarExtender = MakeShareable(new FExtender);
	ToolbarExtension = ToolbarExtender->AddToolBarExtension("CameraSpeed", EExtensionHook::After, RenderDocPluginCommands,
		FToolBarExtensionDelegate::CreateRaw(this, &FRenderDocPluginModule::AddToolbarExtension));

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);

	ExtensionManager = LevelEditorModule.GetToolBarExtensibilityManager();
	
	RenderDocMaskOverlayBits(eOverlay_None, eOverlay_None);

	RenderDocGUI = new FRenderDocGUI();

	_isInitialized = false;
	FSlateRenderer* SlateRenderer = FSlateApplication::Get().GetRenderer().Get();
	SlateRenderer->OnSlateWindowRendered().AddRaw(this, &FRenderDocPluginModule::Initialize);
}

void FRenderDocPluginModule::Initialize(SWindow& SlateWindow, void* ViewportRHIPtr)
{
	if (_isInitialized)
		return;

	_isInitialized = true;

	FSlateRenderer* SlateRenderer = FSlateApplication::Get().GetRenderer().Get();
	SlateRenderer->OnSlateWindowRendered().RemoveRaw(this, &FRenderDocPluginModule::Initialize);
	
	//Trigger a capture just to make sure we are set up correctly. This should prevent us from crashing on exit.
	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
		InitializeRenderDoc,
		FRenderDocGUI*, RenderDocGUI, RenderDocGUI,
		pRENDERDOC_StartFrameCapture, RenderDocStartFrameCapture, RenderDocStartFrameCapture,
		pRENDERDOC_EndFrameCapture, RenderDocEndFrameCapture, RenderDocEndFrameCapture,
		{
		HWND WindowHandle = GetActiveWindow();

		RenderDocStartFrameCapture(WindowHandle);
		RenderDocEndFrameCapture(WindowHandle);

		FString NewestCapture = RenderDocGUI->GetNewestCapture(FPaths::Combine(*FPaths::GameSavedDir(), *FString("RenderDocCaptures")));
		IFileManager::Get().Delete(*NewestCapture);
	});
	
	UE_LOG(RenderDocPlugin, Log, TEXT("RenderDoc plugin initialized!"));
}

void FRenderDocPluginModule::CaptureCurrentViewport()
{
	UE_LOG(RenderDocPlugin, Log, TEXT("Capture frame and launch renderdoc!"));

	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		StartRenderDocCapture,
		pRENDERDOC_StartFrameCapture, RenderDocStartFrameCapture, RenderDocStartFrameCapture,
		{
		RenderDocStartFrameCapture(GetActiveWindow());
	});

	GEditor->GetActiveViewport()->Draw(true);

	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
		EndRenderDocCapture,
		uint32, SocketPort, SocketPort,
		FRenderDocGUI*, RenderDocGUI, RenderDocGUI,
		pRENDERDOC_EndFrameCapture, RenderDocEndFrameCapture, RenderDocEndFrameCapture,
		{
		RenderDocEndFrameCapture(GetActiveWindow());

		FString BinaryPath;
		if (GConfig)
		{
			GConfig->GetString(TEXT("RenderDoc"), TEXT("BinaryPath"), BinaryPath, GGameIni);
		}

		RenderDocGUI->StartRenderDoc(FPaths::Combine(*BinaryPath, *FString("renderdocui.exe"))
			, FPaths::Combine(*FPaths::GameSavedDir(), *FString("RenderDocCaptures"))
			, SocketPort);
	});
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