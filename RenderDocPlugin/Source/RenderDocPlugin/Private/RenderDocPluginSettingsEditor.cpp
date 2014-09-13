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
#include "RenderDocPluginStyle.h"
#include "RenderDocPluginSettingsEditor.h"

#define LOCTEXT_NAMESPACE "RenderDocPluginSettingsEditor"

void SRenderDocPluginSettingsEditor::Construct(const FArguments& Args)
{
	ChildSlot.Widget =
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(EVerticalAlignment::VAlign_Top)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("CaptureCallstacks", "Capture callstacks"))
				.ToolTipText(FString("Save the call stack for every draw event in addition to the event itself. This is useful when you need additional information to solve your particular problem."))
			]

			+ SHorizontalBox::Slot()
			.VAlign(EVerticalAlignment::VAlign_Top)
			.HAlign(HAlign_Right)
			[
				SNew(SCheckBox)
				.IsChecked(bCaptureCallStacks ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked)
			]
		]

		+ SVerticalBox::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(EVerticalAlignment::VAlign_Top)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("RefAllResources", "Capture all resources"))
				.ToolTipText(FString("Capture all resources, including those that are not referenced by the current frame."))
			]

			+ SHorizontalBox::Slot()
			.VAlign(EVerticalAlignment::VAlign_Top)
			.HAlign(HAlign_Right)
			[
				SNew(SCheckBox)
				.IsChecked(bRefAllResources ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked)
			]
		]

		+ SVerticalBox::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(EVerticalAlignment::VAlign_Top)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SaveAllInitials", "Save all initial states"))
				.ToolTipText(FString("Save the initial status of all resources, even if we think that they will be overwritten in this frame."))
			]

			+ SHorizontalBox::Slot()
				.VAlign(EVerticalAlignment::VAlign_Top)
				.HAlign(HAlign_Right)
			[
				SNew(SCheckBox)
				.IsChecked(bSaveAllInitials ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked)
			]
		]

		+ SVerticalBox::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(EVerticalAlignment::VAlign_Top)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("DoNotStripShaderDebugData", "Do not strip shader debug data"))
				.ToolTipText(FString("Unreal Engine 4 strips debug data from its shaders by default. While this takes less space, it also makes it very hard to debug the shaders in question. We recommend that you turn this on as it will make debugging a lot easier. Do not forget to turn this back off before building for release though."))
			]

			+ SHorizontalBox::Slot()
			.VAlign(EVerticalAlignment::VAlign_Top)
			.HAlign(HAlign_Right)
			[
				SNew(SCheckBox)
				.IsChecked(bDoNotStripShaderDebugData ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked)
			]
		];
}

FReply SRenderDocPluginSettingsEditor::OnKeyDown(const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent)
{
	int32 A = 0;
	return FReply::Unhandled();
}

void SRenderDocPluginSettingsEditor::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	// Call parent implementation
	SCompoundWidget::Tick( AllottedGeometry, InCurrentTime, InDeltaTime );
}

#undef LOCTEXT_NAMESPACE