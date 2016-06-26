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
#include "Editor/UnrealEd/Public/SEditorViewportToolBarMenu.h"
#include "RenderDocPluginStyle.h"
#include "RenderDocPluginSettingsEditorWindow.h"
#include "RenderDocPluginAboutWindow.h"

#define LOCTEXT_NAMESPACE "RenderDocPluginSettingsEditor"

class FRenderDocSettingsCommands : public TCommands<FRenderDocSettingsCommands>
{
public:
  FRenderDocSettingsCommands()
  : TCommands<FRenderDocSettingsCommands>(
    TEXT("RenderDocSettings"), // Context name for fast lookup
    NSLOCTEXT("Contexts", "RenderDocSettings", "RenderDoc Settings"), // Localized context name for displaying
    TEXT("EditorViewport"), // Parent context name.  
    FEditorStyle::GetStyleSetName() // Icon Style Set
    )
  { }

  virtual ~FRenderDocSettingsCommands() { }

  virtual void RegisterCommands()
  {
    //UI_COMMAND(ToggleMaximize, "Maximize Viewport", "Toggles the Maximize state of the current viewport", EUserInterfaceActionType::ToggleButton, FInputChord());

    Commands.Add(
      FUICommandInfoDecl(this->AsShared(), FName(TEXT("CaptureAllActivity")), LOCTEXT("CaptureAllActivity", "Capture all activity"), 
        LOCTEXT("CaptureAllActivityToolTip", "If enabled, capture all rendering activity during the next engine update tick; if disabled, only the rendering activity of the active viewport will be captured."))
      .UserInterfaceType(EUserInterfaceActionType::ToggleButton)
    );

    Commands.Add(
      FUICommandInfoDecl(this->AsShared(), FName(TEXT("CaptureCallstacks")), LOCTEXT("CaptureCallstacks", "Capture callstacks"),
        LOCTEXT("CaptureCallstacksToolTip", "Save the call stack for every draw event in addition to the event itself. This is useful when you need additional information to solve your particular problem."))
      .UserInterfaceType(EUserInterfaceActionType::ToggleButton)
    );

    Commands.Add(
      FUICommandInfoDecl(this->AsShared(), FName(TEXT("RefAllResources")), LOCTEXT("RefAllResources", "Capture all resources"),
        LOCTEXT("RefAllResourcesToolTip", "Capture all resources, including those that are not referenced by the current frame."))
      .UserInterfaceType(EUserInterfaceActionType::ToggleButton)
      );

    Commands.Add(
      FUICommandInfoDecl(this->AsShared(), FName(TEXT("SaveAllInitials")), LOCTEXT("SaveAllInitials", "Save all initial states"),
        LOCTEXT("SaveAllInitialsToolTip", "Save the initial status of all resources, even if we think that they will be overwritten in this frame."))
      .UserInterfaceType(EUserInterfaceActionType::ToggleButton)
      );
  }

  TArray< TSharedPtr<FUICommandInfo> > Commands;

};

void SRenderDocPluginSettingsEditorWindow::Construct(const FArguments& InArgs)
{
	auto RenderDocSettings = InArgs._Settings;

  FRenderDocSettingsCommands::Register();
  BindCommands(RenderDocSettings);

  FSlateIcon SettingsIconBrush = FSlateIcon(FRenderDocPluginStyle::Get()->GetStyleSetName(), "RenderDocPlugin.SettingsIcon.Small");

  ChildSlot
  [
    SNew(SEditorViewportToolbarMenu)
    .ToolTipText(LOCTEXT("RenderDocSettingsMenu", "RenderDoc Settings"))
    .Cursor(EMouseCursor::Default)
    .ParentToolBar(SharedThis(this))
    .AddMetaData<FTagMetaData>(FTagMetaData(TEXT("EditorViewportToolBar.RenderDocSettingsMenu")))
    .LabelIcon(SettingsIconBrush.GetIcon())
    .OnGetMenuContent_Lambda([this,RenderDocSettings]() -> TSharedRef<SWidget>
    {
      auto& Commands = FRenderDocSettingsCommands::Get();
      FMenuBuilder ShowMenuBuilder (true, CommandList);

      for (auto& command : Commands.Commands)
        ShowMenuBuilder.AddMenuEntry(command);

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
    })
  ];

}

void SRenderDocPluginSettingsEditorWindow::BindCommands(FRenderDocPluginSettings* Settings)
{
  check(!CommandList.IsValid());
  CommandList = MakeShareable(new FUICommandList);

  auto& Commands = FRenderDocSettingsCommands::Get().Commands;

  CommandList->MapAction(
    Commands[0],
    FExecuteAction::CreateLambda([](bool* flag) { *flag = !*flag; },
      &Settings->bCaptureAllActivity),
    FCanExecuteAction(),
    FIsActionChecked::CreateLambda([](const bool* flag) { return(*flag); },
      &Settings->bCaptureAllActivity)
  );

  CommandList->MapAction(
    Commands[1],
    FExecuteAction::CreateLambda([](bool* flag) { *flag = !*flag; },
      &Settings->bCaptureCallStacks),
    FCanExecuteAction(),
    FIsActionChecked::CreateLambda([](const bool* flag) { return(*flag); },
      &Settings->bCaptureCallStacks)
  );

  CommandList->MapAction(
    Commands[2],
    FExecuteAction::CreateLambda([](bool* flag) { *flag = !*flag; },
      &Settings->bRefAllResources),
    FCanExecuteAction(),
    FIsActionChecked::CreateLambda([](const bool* flag) { return(*flag); },
      &Settings->bRefAllResources)
  );

  CommandList->MapAction(
    Commands[3],
    FExecuteAction::CreateLambda([](bool* flag) { *flag = !*flag; },
      &Settings->bSaveAllInitials),
    FCanExecuteAction(),
    FIsActionChecked::CreateLambda([](const bool* flag) { return(*flag); },
      &Settings->bSaveAllInitials)
  );
}

#undef LOCTEXT_NAMESPACE

#endif//WITH_EDITOR
