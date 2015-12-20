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
#include "RenderDocPluginGUI.h"

FRenderDocPluginGUI::FRenderDocPluginGUI(RENDERDOC_API_CONTEXT* pRENDERDOC)
{
	RENDERDOC = pRENDERDOC;
}

void FRenderDocPluginGUI::StartRenderDoc(FString FrameCaptureBaseDirectory)
{
	FString NewestCapture = GetNewestCapture(FrameCaptureBaseDirectory);
	FString ArgumentString = FString::Printf(TEXT("\"%s\""), *FPaths::ConvertRelativePathToFull(NewestCapture).Append(TEXT(".log")));

	if (!NewestCapture.IsEmpty())
	{
		// This is the new, recommended way of launching the RenderDoc GUI:
		if (!RENDERDOC->IsRemoteAccessConnected())
		{
		  uint32 PID = (sizeof(TCHAR) == sizeof(char)) ?
			  RENDERDOC->LaunchReplayUI(true, (const char*)(*ArgumentString))
			: RENDERDOC->LaunchReplayUI(true, TCHAR_TO_ANSI(*ArgumentString));

		  if (0 == PID)
			UE_LOG(LogTemp, Error, TEXT("Could not launch RenderDoc!!"));
		}
	}
}

FString FRenderDocPluginGUI::GetNewestCapture(FString BaseDirectory)
{
	char LogFile[512];
	uint64_t Timestamp;
	uint32_t LogPathLength = 512;
	uint32_t Index = 0;
	FString OutString;
	
	while (RENDERDOC->GetCapture(Index, LogFile, &LogPathLength, &Timestamp))
	{
		if (sizeof(TCHAR) == sizeof(char))
			OutString = FString(LogPathLength, (TCHAR*)LogFile);
		else
			OutString = FString(LogPathLength, ANSI_TO_TCHAR(LogFile));

		Index++;
	}
	
	return OutString;
}
