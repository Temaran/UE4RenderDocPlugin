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
#include "Engine.h"
#include "GlobalShader.h"
#include "Editor.h"
#include "RenderDocPluginStyle.h"
#include "RenderDocPluginSettingsEditorWindow.h"
#include "RenderDocPluginAboutWindow.h"

#include "RenderDocPluginModule.h"

#define LOCTEXT_NAMESPACE "RenderDocPluginSettingsEditor"

void SRenderDocPluginSettingsEditorWindow::Construct(const FArguments& InArgs)
{
	ThePlugin = InArgs._ThePlugin;
	RenderDocSettings = InArgs._Settings;

	SWindow::Construct(SWindow::FArguments()
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.IsPopupWindow(false)
		.CreateTitleBar(false)
		.SizingRule(ESizingRule::FixedSize)
		.SupportsTransparency(EWindowTransparency::None)
		.InitialOpacity(1.0f)
		.FocusWhenFirstShown(true)
		.bDragAnywhere(false)
		.ActivateWhenFirstShown(true)
		.ClientSize(FVector2D(325, 150))
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
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
					.IsChecked(RenderDocSettings.bCaptureAllActivity ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
					.OnCheckStateChanged(this, &SRenderDocPluginSettingsEditorWindow::OnCaptureAllActivityChanged)
				]
			]

			+ SVerticalBox::Slot()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
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
					.IsChecked(RenderDocSettings.bCaptureCallStacks ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
					.OnCheckStateChanged(this, &SRenderDocPluginSettingsEditorWindow::OnCaptureCallStacksChanged)
				]
			]

			+ SVerticalBox::Slot()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
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
					.IsChecked(RenderDocSettings.bRefAllResources ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
					.OnCheckStateChanged(this, &SRenderDocPluginSettingsEditorWindow::OnRefAllResourcesChanged)
				]
			]

			+ SVerticalBox::Slot()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
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
					.IsChecked(RenderDocSettings.bSaveAllInitials ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
					.OnCheckStateChanged(this, &SRenderDocPluginSettingsEditorWindow::OnSaveAllInitialsChanged)
				]
			]

			+ SVerticalBox::Slot()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.VAlign(EVerticalAlignment::VAlign_Center)
				.Padding(5)
				[
					SNew(SButton)
					.OnClicked(this, &SRenderDocPluginSettingsEditorWindow::SaveAndClose)
					.Text(LOCTEXT("SaveAndCloseButton", "Save and close"))
				]

				+ SHorizontalBox::Slot()
				.VAlign(EVerticalAlignment::VAlign_Center)
				.Padding(5)
				[
					SNew(SButton)
					.OnClicked(this, &SRenderDocPluginSettingsEditorWindow::ShowAboutWindow)
					.Text(LOCTEXT("AboutButton", "About"))
				]

				+ SHorizontalBox::Slot()
				.VAlign(EVerticalAlignment::VAlign_Center)
				.Padding(5)
				[
					SNew(SButton)
					.OnClicked(this, &SRenderDocPluginSettingsEditorWindow::Close)
					.Text(LOCTEXT("CloseButton", "Close"))
				]
			]
		]);

	bIsTopmostWindow = true;
}

void SRenderDocPluginSettingsEditorWindow::OnCaptureAllActivityChanged(ECheckBoxState NewState)
{
	RenderDocSettings.bCaptureAllActivity = NewState == ECheckBoxState::Checked ? true : false;
}

void SRenderDocPluginSettingsEditorWindow::OnCaptureCallStacksChanged(ECheckBoxState NewState)
{
	RenderDocSettings.bCaptureCallStacks = NewState == ECheckBoxState::Checked ? true : false;
  pRENDERDOC_SetCaptureOptionU32 SetOptions = ThePlugin->Loader.RenderDocAPI->SetCaptureOptionU32;
  int ok = SetOptions(eRENDERDOC_Option_CaptureCallstacks, RenderDocSettings.bCaptureCallStacks ? 1 : 0);
  check(ok);
}

void SRenderDocPluginSettingsEditorWindow::OnRefAllResourcesChanged(ECheckBoxState NewState)
{
	RenderDocSettings.bRefAllResources = NewState == ECheckBoxState::Checked ? true : false;
  pRENDERDOC_SetCaptureOptionU32 SetOptions = ThePlugin->Loader.RenderDocAPI->SetCaptureOptionU32;
  int ok = SetOptions(eRENDERDOC_Option_RefAllResources, RenderDocSettings.bRefAllResources ? 1 : 0);
  check(ok);
}

void SRenderDocPluginSettingsEditorWindow::OnSaveAllInitialsChanged(ECheckBoxState NewState)
{
	RenderDocSettings.bSaveAllInitials = NewState == ECheckBoxState::Checked ? true : false;
  pRENDERDOC_SetCaptureOptionU32 SetOptions = ThePlugin->Loader.RenderDocAPI->SetCaptureOptionU32;
  int ok = SetOptions(eRENDERDOC_Option_SaveAllInitials, RenderDocSettings.bSaveAllInitials ? 1 : 0);
  check(ok);
}

FReply SRenderDocPluginSettingsEditorWindow::SaveAndClose()
{
	RenderDocSettings.Save();
	return Close();
}

FReply SRenderDocPluginSettingsEditorWindow::ShowAboutWindow()
{
	GEditor->EditorAddModalWindow(SNew(SRenderDocPluginAboutWindow));
	return Close();
}

FReply SRenderDocPluginSettingsEditorWindow::Close()
{
	RequestDestroyWindow();
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE