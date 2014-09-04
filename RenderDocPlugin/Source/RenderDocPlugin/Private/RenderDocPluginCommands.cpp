#include "RenderDocPluginPrivatePCH.h"
#include "RenderDocPluginCommands.h"
 
PRAGMA_DISABLE_OPTIMIZATION
void FRenderDocPluginCommands::RegisterCommands()
{
	UI_COMMAND(CaptureFrameButton, "Capture Frame", "Captures the next frame and launches the renderdoc UI", EUserInterfaceActionType::Button, FInputGesture(EKeys::R, EModifierKey::Alt));
}
PRAGMA_ENABLE_OPTIMIZATION
