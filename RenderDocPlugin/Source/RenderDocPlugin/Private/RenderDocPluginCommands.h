#pragma once

#include "Slate.h"
#include "EditorStyle.h"
#include "RenderDocPluginStyle.h"

class FRenderDocPluginCommands : public TCommands<FRenderDocPluginCommands>
{
public:
	FRenderDocPluginCommands()
		: TCommands<FRenderDocPluginCommands>(TEXT("RenderDocPlugin"), NSLOCTEXT("Contexts", "RenderDocPlugin", "RenderDoc Plugin"), NAME_None, FRenderDocPluginStyle::Get()->GetStyleSetName())
	{
	}

	virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> CaptureFrameButton;
};