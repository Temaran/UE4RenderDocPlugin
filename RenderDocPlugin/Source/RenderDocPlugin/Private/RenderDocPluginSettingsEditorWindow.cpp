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
#include "GlobalShader.h"
#include "Editor.h"
#include "Editor/UnrealEd/Public/SEditorViewportToolBarMenu.h"
#include "RenderDocPluginStyle.h"
#include "RenderDocPluginSettingsEditorWindow.h"
#include "RenderDocPluginAboutWindow.h"

#include "RenderDocPluginModule.h"

#define LOCTEXT_NAMESPACE "RenderDocPluginSettingsEditor"

class FRenderDocSettingsCommands : public TCommands<FRenderDocSettingsCommands>
{
public:
  FRenderDocSettingsCommands() : TCommands<FRenderDocSettingsCommands>
  (
    TEXT("RenderDocSettings"), // Context name for fast lookup
    NSLOCTEXT("Contexts", "RenderDocSettings", "RenderDoc Settings"), // Localized context name for displaying
    TEXT("EditorViewport"), // Parent context name.  
    FEditorStyle::GetStyleSetName() // Icon Style Set
  ) { }

  virtual ~FRenderDocSettingsCommands() { }

  virtual void RegisterCommands()
  {
    //UI_COMMAND(ToggleMaximize, "Maximize Viewport", "Toggles the Maximize state of the current viewport", EUserInterfaceActionType::ToggleButton, FInputChord());

    Commands.Add(
      FUICommandInfoDecl(this->AsShared(), FName(TEXT("CaptureAllActivity")), LOCTEXT("CaptureAllActivity", "Capture all activity"), 
        LOCTEXT("CaptureAllActivityToolTip", "If enabled, capture all rendering activity during the next engine update tick; if disabled, only the rendering activity of the active viewport will be captured."))
      .UserInterfaceType(EUserInterfaceActionType::ToggleButton)
    );
    Flags.Add(false);

    Commands.Add(
      FUICommandInfoDecl(this->AsShared(), FName(TEXT("CaptureCallstacks")), LOCTEXT("CaptureCallstacks", "Capture callstacks"),
        LOCTEXT("CaptureCallstacksToolTip", "Save the call stack for every draw event in addition to the event itself. This is useful when you need additional information to solve your particular problem."))
      .UserInterfaceType(EUserInterfaceActionType::ToggleButton)
    );
    Flags.Add(false);

    Commands.Add(
      FUICommandInfoDecl(this->AsShared(), FName(TEXT("RefAllResources")), LOCTEXT("RefAllResources", "Capture all resources"),
        LOCTEXT("RefAllResourcesToolTip", "Capture all resources, including those that are not referenced by the current frame."))
      .UserInterfaceType(EUserInterfaceActionType::ToggleButton)
      );
    Flags.Add(false);

    Commands.Add(
      FUICommandInfoDecl(this->AsShared(), FName(TEXT("SaveAllInitials")), LOCTEXT("SaveAllInitials", "Save all initial states"),
        LOCTEXT("SaveAllInitialsToolTip", "Save the initial status of all resources, even if we think that they will be overwritten in this frame."))
      .UserInterfaceType(EUserInterfaceActionType::ToggleButton)
      );
    Flags.Add(false);

    CommandList = MakeShareable( new FUICommandList );
  }

  TArray< bool > Flags;
  TArray< TSharedPtr<FUICommandInfo> > Commands;
  TSharedPtr< FUICommandList > CommandList;

  void ToggleShowFlag(uint32 FlagIndex)
  {
    bool bOldState = Flags[FlagIndex];
    Flags[FlagIndex] = !bOldState;
  }

  bool IsShowFlagEnabled(uint32 FlagIndex) const
  {
    return(Flags[FlagIndex]);
  }
};

static TSharedPtr<FRenderDocSettingsCommands> RenderDocSettingsCommands;

TSharedRef<SWidget> SRenderDocPluginSettingsEditorWindow::GenerateSettingsMenu() const
{
  FMenuBuilder ShowMenuBuilder (true, RenderDocSettingsCommands->CommandList);

  int index (0);
  for (auto& command : RenderDocSettingsCommands->Commands)
  {
    RenderDocSettingsCommands->CommandList->MapAction(
      command,
      FExecuteAction::CreateLambda( [](bool* flag) { *flag = !*flag; },
        &RenderDocSettingsCommands->Flags[index] ),
      FCanExecuteAction(),
      FIsActionChecked::CreateLambda( [](const bool* flag) { return(*flag); },
        &RenderDocSettingsCommands->Flags[index] )
    );
    ++index;
    ShowMenuBuilder.AddMenuEntry(command);
  }

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
        .OnClicked(this, &SRenderDocPluginSettingsEditorWindow::SaveAndClose)
      .Text(LOCTEXT("SaveButton", "Save"))
      ]

      +SHorizontalBox::Slot()
      .AutoWidth()
      .VAlign(EVerticalAlignment::VAlign_Center)
      .Padding(5)
      [
        SNew(SButton)
        .OnClicked(this, &SRenderDocPluginSettingsEditorWindow::ShowAboutWindow)
      .Text(LOCTEXT("AboutButton", "About"))
      ]
    ],
    FText()
  );

/*
  TSharedPtr<FUICommandList> CommandList = MakeShareable(new FUICommandList);
  FMenuBuilder ShowMenuBuilder (true, CommandList);
  ShowMenuBuilder.AddWidget(

    SNew(SVerticalBox)

    + SVerticalBox::Slot()
    .AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
      .AutoWidth()
      .HAlign(EHorizontalAlignment::HAlign_Left)
			.VAlign(EVerticalAlignment::VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("CaptureAllActivity", "Capture all activity"))
				.ToolTipText(LOCTEXT("CaptureAllActivityToolTip", "If enabled, capture all rendering activity during the next engine update tick; if disabled, only the rendering activity of the active viewport will be captured."))
			]
			+ SHorizontalBox::Slot()
			.VAlign(EVerticalAlignment::VAlign_Center)
			.HAlign(HAlign_Right)
			[
				SNew(SCheckBox)
				.IsChecked(RenderDocSettings->bCaptureAllActivity ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.OnCheckStateChanged(this, &SRenderDocPluginSettingsEditorWindow::OnCaptureAllActivityChanged)
			]
		]

		+ SVerticalBox::Slot()
    .AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
      .AutoWidth()
			.VAlign(EVerticalAlignment::VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("CaptureCallstacks", "Capture callstacks"))
				.ToolTipText(LOCTEXT("CaptureCallstacksToolTip", "Save the call stack for every draw event in addition to the event itself. This is useful when you need additional information to solve your particular problem."))
			]

			+ SHorizontalBox::Slot()
			.VAlign(EVerticalAlignment::VAlign_Center)
			.HAlign(HAlign_Right)
			[
				SNew(SCheckBox)
				.IsChecked(RenderDocSettings->bCaptureCallStacks ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.OnCheckStateChanged(this, &SRenderDocPluginSettingsEditorWindow::OnCaptureCallStacksChanged)
			]
		]

		+ SVerticalBox::Slot()
    .AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
      .AutoWidth()
			.VAlign(EVerticalAlignment::VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("RefAllResources", "Capture all resources"))
				.ToolTipText(LOCTEXT("RefAllResourcesToolTip", "Capture all resources, including those that are not referenced by the current frame."))
			]

			+ SHorizontalBox::Slot()
			.VAlign(EVerticalAlignment::VAlign_Center)
			.HAlign(HAlign_Right)
			[
				SNew(SCheckBox)
				.IsChecked(RenderDocSettings->bRefAllResources ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.OnCheckStateChanged(this, &SRenderDocPluginSettingsEditorWindow::OnRefAllResourcesChanged)
			]
		]

		+ SVerticalBox::Slot()
    .AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
      .AutoWidth()
			.VAlign(EVerticalAlignment::VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SaveAllInitials", "Save all initial states"))
				.ToolTipText(LOCTEXT("SaveAllInitialsToolTip", "Save the initial status of all resources, even if we think that they will be overwritten in this frame."))
			]

			+ SHorizontalBox::Slot()
			.VAlign(EVerticalAlignment::VAlign_Center)
			.HAlign(HAlign_Right)
			[
				SNew(SCheckBox)
				.IsChecked(RenderDocSettings->bSaveAllInitials ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.OnCheckStateChanged(this, &SRenderDocPluginSettingsEditorWindow::OnSaveAllInitialsChanged)
			]
		]

		+ SVerticalBox::Slot()
    .AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
      .AutoWidth()
			.VAlign(EVerticalAlignment::VAlign_Center)
			.Padding(5)
			[
				SNew(SButton)
				.OnClicked(this, &SRenderDocPluginSettingsEditorWindow::SaveAndClose)
				.Text(LOCTEXT("SaveButton", "Save"))
			]

			+ SHorizontalBox::Slot()
      .AutoWidth()
			.VAlign(EVerticalAlignment::VAlign_Center)
			.Padding(5)
			[
				SNew(SButton)
				.OnClicked(this, &SRenderDocPluginSettingsEditorWindow::ShowAboutWindow)
				.Text(LOCTEXT("AboutButton", "About"))
			]
    ],
    FText()
  );
  */
  return(ShowMenuBuilder.MakeWidget());
}

void SRenderDocPluginSettingsEditorWindow::Construct(const FArguments& InArgs)
{
	ThePlugin = InArgs._ThePlugin;
	RenderDocSettings = InArgs._Settings;

  if (!RenderDocSettingsCommands.IsValid())
  {
    RenderDocSettingsCommands = MakeShareable(new FRenderDocSettingsCommands);
    RenderDocSettingsCommands->RegisterCommands();
  }

  FSlateIcon SettingsIconBrush = FSlateIcon(FRenderDocPluginStyle::Get()->GetStyleSetName(), "RenderDocPlugin.SettingsIcon.Small");

  ChildSlot
  [
    SNew(SEditorViewportToolbarMenu)
    .ToolTipText(LOCTEXT("RenderDocSettingsMenu", "RenderDoc Settings"))
    .Cursor(EMouseCursor::Default)
    .ParentToolBar(SharedThis(this))
    .AddMetaData<FTagMetaData>(FTagMetaData(TEXT("EditorViewportToolBar.RenderDocSettingsMenu")))
    .OnGetMenuContent(this, &SRenderDocPluginSettingsEditorWindow::GenerateSettingsMenu)
    .LabelIcon(SettingsIconBrush.GetIcon())
  ];

	//bIsTopmostWindow = true;
}

void SRenderDocPluginSettingsEditorWindow::OnCaptureAllActivityChanged(ECheckBoxState NewState)
{
	RenderDocSettings->bCaptureAllActivity = NewState == ECheckBoxState::Checked ? true : false;
}

void SRenderDocPluginSettingsEditorWindow::OnCaptureCallStacksChanged(ECheckBoxState NewState)
{
	RenderDocSettings->bCaptureCallStacks = NewState == ECheckBoxState::Checked ? true : false;
	pRENDERDOC_SetCaptureOptionU32 SetOptions = ThePlugin->Loader.RenderDocAPI->SetCaptureOptionU32;
	int ok = SetOptions(eRENDERDOC_Option_CaptureCallstacks, RenderDocSettings->bCaptureCallStacks ? 1 : 0);
	check(ok);
}

void SRenderDocPluginSettingsEditorWindow::OnRefAllResourcesChanged(ECheckBoxState NewState)
{
	RenderDocSettings->bRefAllResources = NewState == ECheckBoxState::Checked ? true : false;
	pRENDERDOC_SetCaptureOptionU32 SetOptions = ThePlugin->Loader.RenderDocAPI->SetCaptureOptionU32;
	int ok = SetOptions(eRENDERDOC_Option_RefAllResources, RenderDocSettings->bRefAllResources ? 1 : 0);
	check(ok);
}

void SRenderDocPluginSettingsEditorWindow::OnSaveAllInitialsChanged(ECheckBoxState NewState)
{
	RenderDocSettings->bSaveAllInitials = NewState == ECheckBoxState::Checked ? true : false;
	pRENDERDOC_SetCaptureOptionU32 SetOptions = ThePlugin->Loader.RenderDocAPI->SetCaptureOptionU32;
	int ok = SetOptions(eRENDERDOC_Option_SaveAllInitials, RenderDocSettings->bSaveAllInitials ? 1 : 0);
	check(ok);
}

FReply SRenderDocPluginSettingsEditorWindow::SaveAndClose()
{
	RenderDocSettings->Save();
	return Close();
}

FReply SRenderDocPluginSettingsEditorWindow::ShowAboutWindow()
{
	GEditor->EditorAddModalWindow(SNew(SRenderDocPluginAboutWindow));
	return Close();
}

FReply SRenderDocPluginSettingsEditorWindow::Close()
{
	//RequestDestroyWindow();
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

#endif//WITH_EDITOR
