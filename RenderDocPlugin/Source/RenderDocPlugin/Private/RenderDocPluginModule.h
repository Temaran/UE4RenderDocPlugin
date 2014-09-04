#pragma once
 
#include "ModuleManager.h"
#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "SharedPointer.h"
#include "RenderDocPluginStyle.h"
#include "RenderDocPluginCommands.h"
#include "Internationalization.h"
#include "Slate.h"
#include "MultiBoxExtender.h"

#include "../ThirdParty/RenderDoc/Include/renderdoc.h"

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
	void AddToolbarExtension(FToolBarBuilder& ToolbarBuilder);

	TSharedPtr<FUICommandList> RenderDocPluginCommands;
	TSharedPtr<FExtensibilityManager> ExtensionManager;
	TSharedPtr<FExtender> ToolbarExtender;
	TSharedPtr<const FExtensionBase> ToolbarExtension;

	HINSTANCE RenderDocDLL;

	pRENDERDOC_TriggerCapture RenderDocTriggerCapture;
	pRENDERDOC_SetLogFile RenderDocSetLogFile;
};

