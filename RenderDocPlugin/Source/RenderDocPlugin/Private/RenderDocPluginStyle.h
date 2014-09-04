#pragma once

#include "Slate.h"

class FRenderDocPluginStyle
{
public:
	static void Initialize();

	static void Shutdown();

	static TSharedPtr<class ISlateStyle> Get();

private:
	static FString InContent(const FString& RelativePath, const ANSICHAR* Extension);

private:
	static TSharedPtr<class FSlateStyleSet> StyleSet;
};