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

#pragma once

#include "Slate.h"
#include "renderdoc_app.h"

struct FRenderDocPluginSettings
{
public:
	bool bCaptureCallStacks;
	bool bRefAllResources;
	bool bSaveAllInitials;
	bool bDoNotStripShaderDebugData;

	FRenderDocPluginSettings()
	{
		if (!GConfig->GetBool(TEXT("RenderDoc"), TEXT("CaptureCallStacks"), bCaptureCallStacks, GGameIni))
			bCaptureCallStacks = false;

		if (!GConfig->GetBool(TEXT("RenderDoc"), TEXT("RefAllResources"), bRefAllResources, GGameIni))
			bRefAllResources = false;

		if (!GConfig->GetBool(TEXT("RenderDoc"), TEXT("SaveAllInitials"), bSaveAllInitials, GGameIni))
			bSaveAllInitials = false;

		if (!GConfig->GetBool(TEXT("RenderDoc"), TEXT("DoNotStripShaderDebugData"), bDoNotStripShaderDebugData, GGameIni))
			bDoNotStripShaderDebugData = false;
	}

	void Save()
	{
		GConfig->SetBool(TEXT("RenderDoc"), TEXT("CaptureCallStacks"), bCaptureCallStacks, GGameIni);
		GConfig->SetBool(TEXT("RenderDoc"), TEXT("RefAllResources"), bRefAllResources, GGameIni);
		GConfig->SetBool(TEXT("RenderDoc"), TEXT("SaveAllInitials"), bSaveAllInitials, GGameIni);
		GConfig->SetBool(TEXT("RenderDoc"), TEXT("DoNotStripShaderDebugData"), bDoNotStripShaderDebugData, GGameIni);
		GConfig->Flush(false, GGameIni);
	}

	CaptureOptions CreateOptions()
	{
		CaptureOptions Options;
		Options.CaptureCallstacks = bCaptureCallStacks;
		Options.RefAllResources = bRefAllResources;
		Options.SaveAllInitials = bSaveAllInitials;

		return Options;
	}
};
