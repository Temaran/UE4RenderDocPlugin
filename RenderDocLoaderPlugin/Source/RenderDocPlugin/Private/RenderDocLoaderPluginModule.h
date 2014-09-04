#pragma once

#include "ModuleManager.h"
#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "SharedPointer.h"
#include "Internationalization.h"
#include "Slate.h"
#include "MultiBoxExtender.h"

DECLARE_LOG_CATEGORY_EXTERN(RenderDocLoaderPlugin, Log, All);
DEFINE_LOG_CATEGORY(RenderDocLoaderPlugin);

class FRenderDocLoaderPluginModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private: 
	HINSTANCE RenderDocDLL;
};

