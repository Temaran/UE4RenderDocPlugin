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

FRenderDocPluginGUI::FRenderDocPluginGUI()
{
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
		int32 ReturnCode;

		if (!FPlatformProcess::ExecProcess(
			*ExecutablePathInQuotes,
			*ArgumentString,
			&ReturnCode,
			/*OutStdOut =*/nullptr,
			/*OutStdErr =*/nullptr))
		{
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
	TArray<FString> AllCaptures;
	IFileManager::Get().FindFilesRecursive(AllCaptures, *BaseDirectory, *FString("*.*"), true, false);
	FString NewestCapture;
	double ShortestLifeTime = MAXDWORD32;
	for (int32 i = 0; i < AllCaptures.Num(); i++)
	{
		double FileLifeTime = IFileManager::Get().GetFileAgeSeconds(*AllCaptures[i]);

		if (FileLifeTime < ShortestLifeTime)
		{
			ShortestLifeTime = FileLifeTime;
			NewestCapture = AllCaptures[i];
		}
	}

	return NewestCapture;
}
