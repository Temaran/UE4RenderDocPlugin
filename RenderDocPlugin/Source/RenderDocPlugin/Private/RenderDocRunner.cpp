#include "RenderDocPluginPrivatePCH.h"
#include "RenderDocRunner.h"

FRenderDocRunner* FRenderDocRunner::Runnable = NULL;

FRenderDocRunner::FRenderDocRunner(FString PathToRenderDocExecutable, FString FrameCaptureBaseDirectory)
: ExecutablePath(PathToRenderDocExecutable)
, CaptureBaseDirectory(FrameCaptureBaseDirectory)
{
	Thread = FRunnableThread::Create(this, TEXT("FRenderDocRunner"), 0, TPri_BelowNormal);
}
 
FRenderDocRunner::~FRenderDocRunner()
{
	delete Thread;
	Thread = NULL;
}
 
bool FRenderDocRunner::Init()
{
	return true;
}
 
uint32 FRenderDocRunner::Run()
{
	//Initial wait before starting to ensure RenderDoc has the time to capture a new frame
	FPlatformProcess::Sleep(4);

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
	FString NewestCaptureInQuotes = FString::Printf(TEXT("\"%s\""), *FPaths::ConvertRelativePathToFull(NewestCapture));

	if (!NewestCapture.IsEmpty())
	{
		int32 ReturnCode;

		if (!FPlatformProcess::ExecProcess(
			*ExecutablePathInQuotes,
			*NewestCaptureInQuotes,
			&ReturnCode,
			/*OutStdOut =*/nullptr,
			/*OutStdErr =*/nullptr))
		{
			UE_LOG(LogTemp, Error, TEXT("Could not launch RenderDoc!!"));
		}
	}

	return 0;
}
 
void FRenderDocRunner::Stop()
{
	StopTaskCounter.Increment();
}
 
FRenderDocRunner* FRenderDocRunner::LaunchRenderDoc(FString PathToRenderDocExecutable, FString FrameCaptureBaseDirectory)
{
	if (!Runnable && FPlatformProcess::SupportsMultithreading())
	{
		Runnable = new FRenderDocRunner(PathToRenderDocExecutable, FrameCaptureBaseDirectory);
	}
	return Runnable;
}
 
void FRenderDocRunner::EnsureCompletion()
{
	Stop();
	Thread->WaitForCompletion();
}
 
void FRenderDocRunner::Shutdown()
{
	if (Runnable)
	{
		Runnable->EnsureCompletion();
		delete Runnable;
		Runnable = NULL;
	}
}
