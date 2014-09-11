#include "RenderDocPluginPrivatePCH.h"
#include "RenderDocRunner.h"

FRenderDocRunner::FRenderDocRunner()
{
	IsRunning = false;
}

FRenderDocRunner::~FRenderDocRunner()
{
	Stop();
}

bool FRenderDocRunner::Init()
{
	return true;
}

uint32 FRenderDocRunner::Run()
{
	IsRunning = true;
	//Initial wait before starting to ensure RenderDoc has the time to capture a new frame
	FPlatformProcess::Sleep(2);

	TArray<FString> AllCaptures;
	IFileManager::Get().FindFilesRecursive(AllCaptures, *CaptureBaseDirectory, *FString("*.*"), true, false);
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

void FRenderDocRunner::Stop()
{
	if (Thread)
	{
		delete Thread;
	}

	Thread = NULL;
	IsRunning = false;
}

void FRenderDocRunner::StartRenderDoc(FString PathToRenderDocExecutable, FString FrameCaptureBaseDirectory, uint32 Port)
{
	if (IsRunning)
		return;
	
	ExecutablePath = PathToRenderDocExecutable;
	CaptureBaseDirectory = FrameCaptureBaseDirectory;
	SocketPort = Port;
	Thread = FRunnableThread::Create(this, TEXT("FRenderDocRunner"), 0, TPri_BelowNormal);
}
