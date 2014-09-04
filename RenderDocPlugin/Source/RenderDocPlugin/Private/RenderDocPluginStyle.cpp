// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "RenderDocPluginPrivatePCH.h"
#include "RenderDocPluginStyle.h"
#include "SlateStyle.h"

FString FRenderDocPluginStyle::InContent(const FString& RelativePath, const ANSICHAR* Extension)
{
	static FString ContentDir = FPaths::GamePluginsDir() / TEXT("RenderDocPlugin/Resources");
	return (ContentDir / RelativePath) + Extension;
}

TSharedPtr<FSlateStyleSet> FRenderDocPluginStyle::StyleSet = NULL;
TSharedPtr<class ISlateStyle> FRenderDocPluginStyle::Get() { return StyleSet; }

void FRenderDocPluginStyle::Initialize()
{
	// Const icon sizes
	const FVector2D Icon40x40(40.0f, 40.0f);

	// Only register once
	if (StyleSet.IsValid())
	{
		return;
	}

	StyleSet = MakeShareable(new FSlateStyleSet("RenderDocPluginStyle"));
	StyleSet->SetContentRoot(FPaths::GamePluginsDir() / TEXT("RenderDocPlugin/Resources"));
	StyleSet->SetCoreContentRoot(FPaths::GamePluginsDir() / TEXT("RenderDocPlugin/Resources"));

	StyleSet->Set("RenderDocPlugin.CaptureFrameIcon", new FSlateImageBrush(FRenderDocPluginStyle::InContent("Icon40", ".png"), Icon40x40));
	
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