#pragma once

#include "ModuleManager.h"
#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "SharedPointer.h"
#include "RenderDocPluginStyle.h"
#include "RenderDocPluginCommands.h"
#include "Internationalization.h"
#include "Slate.h"
#include "MultiBoxExtender.h"
#include "RenderDocAPI.h"
#include "RenderDocRunner.h"

DECLARE_LOG_CATEGORY_EXTERN(RenderDocPlugin, Log, All);
DEFINE_LOG_CATEGORY(RenderDocPlugin);

class FRenderDocPluginModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void CaptureNextFrameAndLaunchUI();
	bool CanCaptureNextFrameAndLaunchUI();

private:
	FRenderDocRunner* RenderDocRunner;

	TSharedPtr<FUICommandList> RenderDocPluginCommands;
	TSharedPtr<FExtensibilityManager> ExtensionManager;
	TSharedPtr<FExtender> ToolbarExtender;
	TSharedPtr<const FExtensionBase> ToolbarExtension;

	HINSTANCE RenderDocDLL;
	uint32 SocketPort;

	void* GetRenderDocFunctionPointer(HINSTANCE ModuleHandle, LPCSTR FunctionName);
	void AddToolbarExtension(FToolBarBuilder& ToolbarBuilder);

	//General
	pRENDERDOC_GetAPIVersion RenderDocGetAPIVersion;
	pRENDERDOC_SetLogFile RenderDocSetLogFile;
	pRENDERDOC_SetCaptureOptions RenderDocSetCaptureOptions;

	//Capture
	pRENDERDOC_SetActiveWindow RenderDocSetActiveWindow;
	pRENDERDOC_TriggerCapture RenderDocTriggerCapture;
	pRENDERDOC_StartFrameCapture RenderDocStartFrameCapture;
	pRENDERDOC_EndFrameCapture RenderDocEndFrameCapture;

	//Overlay
	pRENDERDOC_GetOverlayBits RenderDocGetOverlayBits;
	pRENDERDOC_MaskOverlayBits RenderDocMaskOverlayBits;

	//Remote access
	pRENDERDOC_InitRemoteAccess RenderDocInitRemoteAccess;
};
