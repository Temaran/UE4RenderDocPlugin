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

#include "Editor.h"
#include "RenderDocPluginStyle.h"
#include "RenderDocPluginAboutWindow.h"

#define LOCTEXT_NAMESPACE "RenderDocPluginAboutWindow"

void SRenderDocPluginAboutWindow::Construct(const FArguments& InArgs)
{
	FString Message = FString(
    "Hello and thank you for trying out the RenderDoc plugin!\n\n" \
		"To capture a frame, press the green capture button on the top\n" \
		"right of any viewport. If you cannot see the button, you might\n" \
		"have to adjust your viewport width until all buttons fit the screen.\n" \
		"You can also use the assigned hotkey (default is Alt+F12).\n\n" \
		"For best performance I recommend using the experimental\n" \
		"AutoCompileShaders branch in the repository and downloading\n" \
		"my fork of the engine so you are able to include shader meta\n" \
		"data in your compiled shaders.This will make it easier to step\n" \
		"through your shader code in the UI as you will be able to see \n" \
		"your variable names etc.\n\n" \
		"If you have any questions or suggestions I'll try to answer them \n" \
		"as best I can at:\ntemaran(at) gmail(dot) com.\n\n" \
		"You can find my fork and updates to the plugins at these github repos:\n" \
		"https://github.com/Temaran/UE4RenderDocPlugin\n" \
		"https://github.com/Temaran/UnrealEngine (RenderDocPluginChanges)");
	
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
		.ClientSize(FVector2D(400, 400))
		.ScreenPosition(FVector2D((float)(GEditor->GetActiveViewport()->GetSizeXY().X) / 2.0,
		(float)(GEditor->GetActiveViewport()->GetSizeXY().Y) / 2.0))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(0.9f)
			.Padding(5)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Message))
			]

			+ SVerticalBox::Slot()
				.FillHeight(0.1f)
				[
					SNew(SButton)
					.VAlign(VAlign_Center)
					.OnClicked(this, &SRenderDocPluginAboutWindow::Close)
					.Text(LOCTEXT("OkButton", "Ok"))
				]
		]);

	bIsTopmostWindow = true;
	FlashWindow();
}

FReply SRenderDocPluginAboutWindow::Close()
{
	RequestDestroyWindow();
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

#endif//WITH_EDITOR
