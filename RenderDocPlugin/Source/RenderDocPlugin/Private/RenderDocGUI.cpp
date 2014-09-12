#include "RenderDocPluginPrivatePCH.h"
#include "RenderDocGUI.h"

FRenderDocGUI::FRenderDocGUI()
{
	IsRunning = false;
}

FRenderDocGUI::~FRenderDocGUI()
{
	Stop();
}

bool FRenderDocGUI::Init()
{
	return true;
}

uint32 FRenderDocGUI::Run()
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

void FRenderDocGUI::Stop()
{
	if (Thread)
	{
		delete Thread;
	}

	Thread = NULL;
	IsRunning = false;
}

void FRenderDocGUI::StartRenderDoc(FString PathToRenderDocExecutable, FString FrameCaptureBaseDirectory, uint32 Port)
{
	if (IsRunning)
		return;
	
	ExecutablePath = PathToRenderDocExecutable;
	CaptureBaseDirectory = FrameCaptureBaseDirectory;
	SocketPort = Port;
	Thread = FRunnableThread::Create(this, TEXT("FRenderDocRunner"), 0, TPri_BelowNormal);
}

FString FRenderDocGUI::GetNewestCapture(FString BaseDirectory)
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
