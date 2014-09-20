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
#include "Editor.h"
#include "RenderDocPluginStyle.h"
#include "RenderDocPluginAreYouSureWindow.h"

#define LOCTEXT_NAMESPACE "RenderDocPluginAreYouSureWindow"

void SRenderDocPluginAreYouSureWindow::Construct(const FArguments& InArgs)
{
	bUserStillWantsToGoAhead = false;

	SWindow::Construct(SWindow::FArguments()
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.IsPopupWindow(false)
		.CreateTitleBar(false)
		.SizingRule(ESizingRule::FixedSize)
		.SupportsTransparency(false)
		.InitialOpacity(1.0f)
		.FocusWhenFirstShown(true)
		.bDragAnywhere(false)
		.ActivateWhenFirstShown(true)
		.ClientSize(FVector2D(400, 300))
		.ScreenPosition(FVector2D((float)(GEditor->GetActiveViewport()->GetSizeXY().X) / 2.0,
		(float)(GEditor->GetActiveViewport()->GetSizeXY().Y) / 2.0))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(0.9f)
			.Padding(5)
			[
				SNew(STextBlock)
				.Text(FText::FromString(InArgs._ContentText))
			]

			+ SVerticalBox::Slot()
				.FillHeight(0.1f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.Padding(5)
					[
						SNew(SButton)
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Left)
						.OnClicked(this, &SRenderDocPluginAreYouSureWindow::Ok)
						.Text(LOCTEXT("OkButton", "Ok"))
					]

					+ SHorizontalBox::Slot()
						.Padding(5)
						[
							SNew(SButton)
							.VAlign(VAlign_Center)
							.HAlign(HAlign_Right)
							.OnClicked(this, &SRenderDocPluginAreYouSureWindow::Cancel)
							.Text(LOCTEXT("OkButton", "Cancel"))
						]
				]
		]);

	bIsTopmostWindow = true;
	FlashWindow();
}

FReply SRenderDocPluginAreYouSureWindow::Ok()
{
	bUserStillWantsToGoAhead = true;
	RequestDestroyWindow();
	return FReply::Handled();
}

FReply SRenderDocPluginAreYouSureWindow::Cancel()
{
	bUserStillWantsToGoAhead = false;
	RequestDestroyWindow();
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE