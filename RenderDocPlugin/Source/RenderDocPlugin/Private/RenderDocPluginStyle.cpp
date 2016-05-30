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

#include "RenderDocPluginStyle.h"
#include "SlateStyle.h"
#include "IPluginManager.h"

FString FRenderDocPluginStyle::InContent(const FString& RelativePath, const ANSICHAR* Extension)
{
	auto myself = IPluginManager::Get().FindPlugin(TEXT("RenderDocPlugin"));
	check(myself.IsValid());
	static FString ContentDir = myself->GetBaseDir() / TEXT("Resources");
	return (ContentDir / RelativePath) + Extension;
}

TSharedPtr<FSlateStyleSet> FRenderDocPluginStyle::StyleSet = NULL;
TSharedPtr<class ISlateStyle> FRenderDocPluginStyle::Get() { return StyleSet; }

void FRenderDocPluginStyle::Initialize()
{
	// Const icon sizes
	const FVector2D Icon20x20(20.0f, 20.0f);
	const FVector2D Icon40x40(40.0f, 40.0f);

	// Only register once
	if (StyleSet.IsValid())
	{
		return;
	}

	StyleSet = MakeShareable(new FSlateStyleSet("RenderDocPluginStyle"));

	FString ProjectResourceDir = FPaths::GamePluginsDir() / TEXT("RenderDocPlugin/Resources");
	FString EngineResourceDir = FPaths::EnginePluginsDir() / TEXT("RenderDocPlugin/Resources");

	if (IFileManager::Get().DirectoryExists(*ProjectResourceDir)) //Is the plugin in the project? In that case, use those resources
	{
		StyleSet->SetContentRoot(ProjectResourceDir);
		StyleSet->SetCoreContentRoot(ProjectResourceDir);
	}
	else //Otherwise, use the global ones
	{
		StyleSet->SetContentRoot(EngineResourceDir);
		StyleSet->SetCoreContentRoot(EngineResourceDir);
	}

	StyleSet->Set("RenderDocPlugin.CaptureFrameIcon", new FSlateImageBrush(FRenderDocPluginStyle::InContent("Icon40", ".png"), Icon40x40));
	StyleSet->Set("RenderDocPlugin.CaptureFrameIcon.Small", new FSlateImageBrush(FRenderDocPluginStyle::InContent("Icon20", ".png"), Icon20x20));
	StyleSet->Set("RenderDocPlugin.SettingsIcon.Small", new FSlateImageBrush(FRenderDocPluginStyle::InContent("SettingsIcon20", ".png"), Icon20x20));
	
	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
};

void FRenderDocPluginStyle::Shutdown()
{
	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
		ensure(StyleSet.IsUnique());
		StyleSet.Reset();
	}
}

#endif//WITH_EDITOR
