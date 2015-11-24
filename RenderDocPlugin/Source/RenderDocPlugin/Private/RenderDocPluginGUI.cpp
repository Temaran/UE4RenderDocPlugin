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
	IsRunning = false;
}

FRenderDocPluginGUI::~FRenderDocPluginGUI()
{
	Stop();
}

bool FRenderDocPluginGUI::Init()
{
	return true;
}

uint32 FRenderDocPluginGUI::Run()
{
	IsRunning = true;

	FPlatformProcess::Sleep(1);

	FString NewestCapture = GetNewestCapture(CaptureBaseDirectory);
	FString ExecutablePathInQuotes = FString::Printf(TEXT("\"%s\""), *ExecutablePath);
	FString ArgumentString = FString::Printf(TEXT("--remoteaccess localhost:%u \"%s\""), SocketPort, *FPaths::ConvertRelativePathToFull(NewestCapture));

	if (!NewestCapture.IsEmpty())
	{
    // This is the new, recommended way of launching the RenderDoc GUI:
    if (!RENDERDOC->IsRemoteAccessConnected())
    {
      uint32 PID = (sizeof(TCHAR) == sizeof(char)) ?
          RENDERDOC->LaunchReplayUI(false, (const char*)(*ArgumentString))
        : RENDERDOC->LaunchReplayUI(false, TCHAR_TO_ANSI(*ArgumentString));

      if (0 == PID)
        UE_LOG(LogTemp, Error, TEXT("Could not launch RenderDoc!!"));
    }
	}

	IsRunning = false;
	return 0;
}

void FRenderDocPluginGUI::Stop()
{
	if (Thread)
	{
		delete Thread;
	}

	Thread = NULL;
	IsRunning = false;
}

void FRenderDocPluginGUI::StartRenderDoc(FString PathToRenderDocExecutable, FString FrameCaptureBaseDirectory, uint32 Port)
{
	if (IsRunning)
		return;
	
	ExecutablePath = PathToRenderDocExecutable;
	CaptureBaseDirectory = FrameCaptureBaseDirectory;
	SocketPort = Port;
	Thread = FRunnableThread::Create(this, TEXT("FRenderDocRunner"), 0, TPri_BelowNormal);
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
		{
			OutString = FString(LogPathLength, (TCHAR*)LogFile);
		}
		else
		{
			TCHAR LogFileWide[512];
			ZeroMemory(LogFileWide, 512);
			size_t NumCharsConverted;
			mbstowcs_s(&NumCharsConverted, LogFileWide, LogFile, LogPathLength);
			OutString = FString(LogPathLength, LogFileWide);
		}

		Index++;
	}
	
	return OutString;
}
