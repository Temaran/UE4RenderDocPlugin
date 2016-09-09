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

#if WITH_EDITOR

#include "Engine.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "Editor/UnrealEd/Public/SEditorViewportToolBarMenu.h"
#include "Editor/UnrealEd/Public/SViewportToolBarComboMenu.h"
#include "RenderDocPluginStyle.h"
#include "RenderDocPluginCommands.h"
#include "RenderDocPluginToolbar.h"
#include "RenderDocPluginModule.h"
#include "RenderDocPluginAboutWindow.h"

FRenderDocPluginEditorExtension::FRenderDocPluginEditorExtension(FRenderDocPluginModule* ThePlugin, FRenderDocPluginSettings* Settings)
{
	// Defer Level Editor UI extensions until Level Editor has been loaded:
	if (FModuleManager::Get().IsModuleLoaded("LevelEditor"))
    Initialize(ThePlugin, Settings);
	else
		FModuleManager::Get().OnModulesChanged().AddLambda([this, ThePlugin, Settings](FName name, EModuleChangeReason reason)
		{
			if ((name == "LevelEditor") && (reason == EModuleChangeReason::ModuleLoaded))
        Initialize(ThePlugin, Settings);
		});
}

FRenderDocPluginEditorExtension::~FRenderDocPluginEditorExtension()
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

void FRenderDocPluginEditorExtension::Initialize(FRenderDocPluginModule* ThePlugin, FRenderDocPluginSettings* Settings)
{
	if (GUsingNullRHI)
	{
		UE_LOG(RenderDocPlugin, Warning, TEXT("RenderDoc Plugin will not be loaded because a Null RHI (Cook Server, perhaps) is being used."));
		return;
	}

	// The LoadModule request below will crash if running as an editor commandlet!
	// ( the GUsingNullRHI check above should prevent this code from executing, but I am
	//   re-emphasizing it here since many plugins appear to be ignoring this condition... )
	check(!IsRunningCommandlet());

	FRenderDocPluginStyle::Initialize();
	FRenderDocPluginCommands::Register();

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	TSharedRef<FUICommandList> CommandBindings = LevelEditorModule.GetGlobalLevelEditorActions();

	ExtensionManager = LevelEditorModule.GetToolBarExtensibilityManager();
	ToolbarExtender = MakeShareable(new FExtender);
	ToolbarExtension = ToolbarExtender->AddToolBarExtension("CameraSpeed", EExtensionHook::After, CommandBindings,
		FToolBarExtensionDelegate::CreateLambda([this, ThePlugin, Settings](FToolBarBuilder& ToolbarBuilder)
		{ AddToolbarExtension(ToolbarBuilder, ThePlugin, Settings); })
	);
	ExtensionManager->AddExtender(ToolbarExtender);

	IsEditorInitialized = false;
	FSlateRenderer* SlateRenderer = FSlateApplication::Get().GetRenderer().Get();
	LoadedDelegateHandle = SlateRenderer->OnSlateWindowRendered().AddRaw(this, &FRenderDocPluginEditorExtension::OnEditorLoaded);
}

void FRenderDocPluginEditorExtension::OnEditorLoaded(SWindow& SlateWindow, void* ViewportRHIPtr)
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

	if (IsEditorInitialized)
	{
		return;
	}
	IsEditorInitialized = true;

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

void FRenderDocPluginEditorExtension::AddToolbarExtension(FToolBarBuilder& ToolbarBuilder, FRenderDocPluginModule* ThePlugin, FRenderDocPluginSettings* Settings)
{
#define LOCTEXT_NAMESPACE "LevelEditorToolBar"

	UE_LOG(RenderDocPlugin, Log, TEXT("Attaching toolbar extension..."));
	ToolbarBuilder.AddSeparator();

	ToolbarBuilder.BeginSection("RenderdocPlugin");

	ToolbarBuilder.AddWidget(
		SNew(SRenderDocPluginToolbar)
		.ThePlugin(ThePlugin)
		.Settings(Settings)
	);

	ToolbarBuilder.EndSection();

#undef LOCTEXT_NAMESPACE//"LevelEditorToolBar"
}

#define LOCTEXT_NAMESPACE "RenderDocPluginSettingsEditor"

namespace TransformViewportToolbarDefs
{
	/** Size of the arrow shown on SGridSnapSettings Menu button */
	const float DownArrowSize = 4.0f;

	/** Size of the icon displayed on the toggle button of SGridSnapSettings */
	const float ToggleImageScale = 16.0f;
}

void SRenderDocPluginToolbar::Construct(const FArguments& InArgs)
{
	auto ThePlugin = InArgs._ThePlugin;
	auto RenderDocSettings = InArgs._Settings;

	BindCommands(ThePlugin, RenderDocSettings);

	FSlateIcon IconBrush = FSlateIcon(FRenderDocPluginStyle::Get()->GetStyleSetName(), "RenderDocPlugin.CaptureFrameIcon.Small");
	FSlateIcon SettingsIconBrush = FSlateIcon(FRenderDocPluginStyle::Get()->GetStyleSetName(), "RenderDocPlugin.SettingsIcon.Small");

	// Widget inspired by STransformViewportToolbar::MakeSurfaceSnappingButton()
	FName ToolBarStyle = TEXT("ViewportMenu");
	ChildSlot
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SButton)
			.ToolTipText(FRenderDocPluginCommands::Get().CaptureFrame->GetDescription())  // TODO: [Alt+F12] does not show up in the tooltip...
			.OnClicked_Lambda([this]() { CommandList->GetActionForCommand(FRenderDocPluginCommands::Get().CaptureFrame)->Execute(); return(FReply::Handled()); })
		[
			SNew(SImage)
			.Image(IconBrush.GetIcon())
		]
	]

	+ SHorizontalBox::Slot()
	.AutoWidth()
	[
		SNew(SCheckBox)
		.Cursor(EMouseCursor::Default)
		.Style(FEditorStyle::Get(), EMultiBlockLocation::ToName(FEditorStyle::Join(ToolBarStyle, ".ToggleButton"), EMultiBlockLocation::End))
		.Padding(0)
		.ToolTipText(LOCTEXT("RenderDocSettings_ToolTipOverride", "RenderDoc Settings"))
		.IsChecked_Static([] { return(ECheckBoxState::Unchecked); })
		.Content()
		[
			SNew( SComboButton )
			.ButtonStyle( FEditorStyle::Get(), "HoverHintOnly" )
			.HasDownArrow( false )
			.ContentPadding( 0 )
			.ButtonContent()
			[
				SNew( SVerticalBox )

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(3.f, 2.f, 5.f, 0.f))
				[
					SNew( SBox )
					.WidthOverride( TransformViewportToolbarDefs::ToggleImageScale )
					.HeightOverride( TransformViewportToolbarDefs::ToggleImageScale )
					.HAlign( HAlign_Center )
					.VAlign( VAlign_Center )
					[
						SNew( SImage )
						.Image( SettingsIconBrush.GetIcon() )
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign( HAlign_Center )
				.Padding(FMargin(0.f, 0.f, 0.f, 3.f))
				[
					SNew( SBox )
					.WidthOverride( TransformViewportToolbarDefs::DownArrowSize )
					.HeightOverride( TransformViewportToolbarDefs::DownArrowSize )
					[
						SNew(SImage)
						.Image(FEditorStyle::GetBrush("ComboButton.Arrow"))
						.ColorAndOpacity(FLinearColor::Black)
					]
				]
			]
			.MenuContent()
			[        
				([this,RenderDocSettings]() -> TSharedRef<SWidget>
				{
					auto& Commands = FRenderDocPluginCommands::Get();
					FMenuBuilder ShowMenuBuilder (true, CommandList);

					ShowMenuBuilder.AddMenuEntry(Commands.Settings_CaptureAllActivity);
					ShowMenuBuilder.AddMenuEntry(Commands.Settings_CaptureCallstack);
					ShowMenuBuilder.AddMenuEntry(Commands.Settings_CaptureAllResources);
					ShowMenuBuilder.AddMenuEntry(Commands.Settings_SaveAllInitialState);

					ShowMenuBuilder.AddWidget(
						SNew(SVerticalBox)
						+SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(EVerticalAlignment::VAlign_Center)
							.Padding(5)
							[
								SNew(SButton)
								.Text(LOCTEXT("SaveButton", "Save"))
								.ToolTipText(LOCTEXT("SaveButton_ToolTipOverride", "Save current RenderDoc settings for this game project."))
								.OnClicked_Lambda([RenderDocSettings]()
								{
									RenderDocSettings->Save();
									return( FReply::Handled() );
								})
							]

							+SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(EVerticalAlignment::VAlign_Center)
							.Padding(5)
							[
								SNew(SButton)
								.Text(LOCTEXT("AboutButton", "About"))
								.OnClicked_Lambda([]()
								{
									GEditor->EditorAddModalWindow( SNew(SRenderDocPluginAboutWindow) );
									return( FReply::Handled() );
								})
							]
						],
						FText()
					);

					return(ShowMenuBuilder.MakeWidget());
				}())
			] // end of SComboButton::MenuContent()
		] // end of SComboButton
	] // end of SCheckBox
];  // end of Toolbar extension

}

void SRenderDocPluginToolbar::BindCommands(FRenderDocPluginModule* ThePlugin, FRenderDocPluginSettings* Settings)
{
	check(!CommandList.IsValid());
	CommandList = MakeShareable(new FUICommandList);

	auto& Commands = FRenderDocPluginCommands::Get();

	CommandList->MapAction(
		Commands.CaptureFrame,
		FExecuteAction::CreateLambda([ThePlugin]() { ThePlugin->CaptureFrame(); }),
		FCanExecuteAction()
	);

	CommandList->MapAction(
		Commands.Settings_CaptureAllActivity,
		FExecuteAction::CreateLambda([](bool* flag) { *flag = !*flag; },
			&Settings->bCaptureAllActivity),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([](const bool* flag) { return(*flag); },
			&Settings->bCaptureAllActivity)
	);

	CommandList->MapAction(
		Commands.Settings_CaptureCallstack,
		FExecuteAction::CreateLambda([](bool* flag) { *flag = !*flag; },
			&Settings->bCaptureCallStacks),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([](const bool* flag) { return(*flag); },
			&Settings->bCaptureCallStacks)
	);

	CommandList->MapAction(
		Commands.Settings_CaptureAllResources,
		FExecuteAction::CreateLambda([](bool* flag) { *flag = !*flag; },
			&Settings->bRefAllResources),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([](const bool* flag) { return(*flag); },
			&Settings->bRefAllResources)
	);

	CommandList->MapAction(
		Commands.Settings_SaveAllInitialState,
		FExecuteAction::CreateLambda([](bool* flag) { *flag = !*flag; },
			&Settings->bSaveAllInitials),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([](const bool* flag) { return(*flag); },
			&Settings->bSaveAllInitials)
	);
}

#undef LOCTEXT_NAMESPACE

#endif//WITH_EDITOR
