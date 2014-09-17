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

#define LOCTEXT_NAMESPACE "RenderDocPluginSettingsEditor"

void SRenderDocPluginSettingsEditorWindow::Construct(const FArguments& InArgs)
{
	SetOptions = InArgs._SetCaptureOptions;
	RenderDocSettings = InArgs._Settings;
	bOriginalShaderDebugData = RenderDocSettings.bShaderDebugData;

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
		.ClientSize(FVector2D(250, 150))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.VAlign(EVerticalAlignment::VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("CaptureCallstacks", "Capture callstacks"))
					.ToolTipText(FString("Save the call stack for every draw event in addition to the event itself. This is useful when you need additional information to solve your particular problem."))
				]

				+ SHorizontalBox::Slot()
					.VAlign(EVerticalAlignment::VAlign_Center)
					.HAlign(HAlign_Right)
					[
						SNew(SCheckBox)
						.IsChecked(RenderDocSettings.bCaptureCallStacks ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked)
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
						.ToolTipText(FString("Capture all resources, including those that are not referenced by the current frame."))
					]

					+ SHorizontalBox::Slot()
						.VAlign(EVerticalAlignment::VAlign_Center)
						.HAlign(HAlign_Right)
						[
							SNew(SCheckBox)
							.IsChecked(RenderDocSettings.bRefAllResources ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked)
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
						.ToolTipText(FString("Save the initial status of all resources, even if we think that they will be overwritten in this frame."))
					]

					+ SHorizontalBox::Slot()
						.VAlign(EVerticalAlignment::VAlign_Center)
						.HAlign(HAlign_Right)
						[
							SNew(SCheckBox)
							.IsChecked(RenderDocSettings.bSaveAllInitials ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked)
							.OnCheckStateChanged(this, &SRenderDocPluginSettingsEditorWindow::OnSaveAllInitialsChanged)
						]
				]

			+ SVerticalBox::Slot()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.VAlign(EVerticalAlignment::VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ShaderDebugData", "Shader debug data"))
						.ToolTipText(FString("Unreal Engine 4 strips debug data from its shaders by default. While this takes less space, it also makes it very hard to debug the shaders in question. We recommend that you turn this on as it will make debugging a lot easier. Do not forget to turn this back off before building for release though."))
					]

					+ SHorizontalBox::Slot()
						.VAlign(EVerticalAlignment::VAlign_Center)
						.HAlign(HAlign_Right)
						[
							SNew(SCheckBox)
							.IsChecked(RenderDocSettings.bShaderDebugData ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked)
							.OnCheckStateChanged(this, &SRenderDocPluginSettingsEditorWindow::OnShaderDebugDataChanged)
						]
				]

			+ SVerticalBox::Slot()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.VAlign(EVerticalAlignment::VAlign_Center)
					.HAlign(EHorizontalAlignment::HAlign_Left)
					[
						SNew(SButton)
						.OnClicked(this, &SRenderDocPluginSettingsEditorWindow::SaveAndClose)
						.Text(LOCTEXT("SaveAndCloseButton", "Save and close"))
					]

					+ SHorizontalBox::Slot()
						.VAlign(EVerticalAlignment::VAlign_Center)
						.HAlign(EHorizontalAlignment::HAlign_Right)
						[
							SNew(SButton)
							.OnClicked(this, &SRenderDocPluginSettingsEditorWindow::Close)
							.Text(LOCTEXT("CloseButton", "Close"))
						]
				]
		]);

	bIsTopmostWindow = true;
}

void SRenderDocPluginSettingsEditorWindow::OnCaptureCallStacksChanged(ESlateCheckBoxState::Type NewState)
{
	RenderDocSettings.bCaptureCallStacks = NewState == ESlateCheckBoxState::Checked ? true : false;
}

void SRenderDocPluginSettingsEditorWindow::OnRefAllResourcesChanged(ESlateCheckBoxState::Type NewState)
{
	RenderDocSettings.bRefAllResources = NewState == ESlateCheckBoxState::Checked ? true : false;
}

void SRenderDocPluginSettingsEditorWindow::OnSaveAllInitialsChanged(ESlateCheckBoxState::Type NewState)
{
	RenderDocSettings.bSaveAllInitials = NewState == ESlateCheckBoxState::Checked ? true : false;
}

void SRenderDocPluginSettingsEditorWindow::OnShaderDebugDataChanged(ESlateCheckBoxState::Type NewState)
{
	RenderDocSettings.bShaderDebugData = NewState == ESlateCheckBoxState::Checked ? true : false;
}

FReply SRenderDocPluginSettingsEditorWindow::SaveAndClose()
{
	RenderDocSettings.Save();
	CaptureOptions Options = RenderDocSettings.CreateOptions();
	SetOptions(&Options);
	
	if (RenderDocSettings.bShaderDebugData != bOriginalShaderDebugData)
	{
		FShaderCompilerEnvironment Environment;

		if (RenderDocSettings.bShaderDebugData)
		{
			FGlobalShader::ModifyCompilationEnvironment(SP_OPENGL_SM4, Environment);
			Environment.CompilerFlags.Add(CFLAG_Debug);
			FGlobalShader::ModifyCompilationEnvironment(SP_OPENGL_SM5, Environment);
			Environment.CompilerFlags.Add(CFLAG_Debug);
			FGlobalShader::ModifyCompilationEnvironment(SP_OPENGL_ES2, Environment);
			Environment.CompilerFlags.Add(CFLAG_Debug);
			FGlobalShader::ModifyCompilationEnvironment(SP_PCD3D_ES2, Environment);
			Environment.CompilerFlags.Add(CFLAG_Debug);
			FGlobalShader::ModifyCompilationEnvironment(SP_PCD3D_SM4, Environment);
			Environment.CompilerFlags.Add(CFLAG_Debug);
			FGlobalShader::ModifyCompilationEnvironment(SP_PCD3D_SM5, Environment);
			Environment.CompilerFlags.Add(CFLAG_Debug);
		}
		else
		{
			FGlobalShader::ModifyCompilationEnvironment(SP_OPENGL_SM4, Environment);
			Environment.CompilerFlags.Remove(CFLAG_Debug);
			FGlobalShader::ModifyCompilationEnvironment(SP_OPENGL_SM5, Environment);
			Environment.CompilerFlags.Remove(CFLAG_Debug);
			FGlobalShader::ModifyCompilationEnvironment(SP_OPENGL_ES2, Environment);
			Environment.CompilerFlags.Remove(CFLAG_Debug);
			FGlobalShader::ModifyCompilationEnvironment(SP_PCD3D_ES2, Environment);
			Environment.CompilerFlags.Remove(CFLAG_Debug);
			FGlobalShader::ModifyCompilationEnvironment(SP_PCD3D_SM4, Environment);
			Environment.CompilerFlags.Remove(CFLAG_Debug);
			FGlobalShader::ModifyCompilationEnvironment(SP_PCD3D_SM5, Environment);
			Environment.CompilerFlags.Remove(CFLAG_Debug);
		}

		RenderDocSettings.bRequestRecompile = true;
	}

	return Close();
}

FReply SRenderDocPluginSettingsEditorWindow::Close()
{
	RequestDestroyWindow();
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE