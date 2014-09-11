#include "RenderDocLoaderPluginPrivatePCH.h" 
#include "RenderDocLoaderPluginModule.h"
#include "Developer/DesktopPlatform/public/DesktopPlatformModule.h"

#define LOCTEXT_NAMESPACE "RenderDocLoaderPluginNamespace" 

void FRenderDocLoaderPluginModule::StartupModule()
{
	FString BinaryPath;
	if (GConfig)
	{
		GConfig->GetString(TEXT("RenderDoc"), TEXT("BinaryPath"), BinaryPath, GGameIni);
	}

	if (BinaryPath.IsEmpty())
	{
		//Try to localize the installation path and update the BinaryPath
		if (!(FWindowsPlatformMisc::QueryRegKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Classes\\RenderDoc.RDCCapture.1\\DefaultIcon\\"), TEXT(""), BinaryPath) && (BinaryPath.Len() > 0)))
		{
			//Renderdoc does not seem to be installed, but it might be built from source or downloaded by archive, 
			//so prompt the user to navigate to the main exe file
			UE_LOG(RenderDocLoaderPlugin, Log, TEXT("RenderDoc is not installed! Please provide custom exe path..."));

			IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
			if (DesktopPlatform)
			{
				FString Filter = TEXT("Renderdoc executable|renderdocui.exe");

				TArray<FString> OutFiles;

				if (DesktopPlatform->OpenFileDialog(NULL, TEXT("Locate main Renderdoc executable..."), TEXT(""), TEXT(""), Filter, EFileDialogFlags::None, OutFiles))
				{
					BinaryPath = OutFiles[0];
				}
			}
		}

		if (!BinaryPath.IsEmpty())
		{			
			BinaryPath = FPaths::GetPath(BinaryPath);

			if (GConfig)
			{
				GConfig->SetString(TEXT("RenderDoc"), TEXT("BinaryPath"), *BinaryPath, GGameIni);
				GConfig->Flush(false, GGameIni);
			}
		}
	}

	if (BinaryPath.IsEmpty())
	{
		UE_LOG(RenderDocLoaderPlugin, Error, TEXT("Could not locate Renderdoc! Aborting module load..."));
		return;
	}

	RenderDocDLL = NULL;

	FString PathToRenderDocDLL = FPaths::Combine(*BinaryPath, *FString("renderdoc.dll"));
	RenderDocDLL = LoadLibrary(*PathToRenderDocDLL);

	if (!RenderDocDLL)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("CouldNotLoadDLLDialog", "Could not load renderdoc DLL, if you have moved your renderdoc installation, please amend the path declaration in your Game.ini file. The path that was searched for the module was {0}"), FText::FromString(PathToRenderDocDLL)));
		UE_LOG(RenderDocLoaderPlugin, Error, TEXT("Could not load renderdoc DLL! Aborting module load..."));
		return;
	}

	UE_LOG(RenderDocLoaderPlugin, Log, TEXT("RenderDoc Loader Plugin loaded!"));
}

void FRenderDocLoaderPluginModule::ShutdownModule()
{
	/*if (RenderDocDLL)
		FreeLibrary(RenderDocDLL);*/

	UE_LOG(RenderDocLoaderPlugin, Log, TEXT("RenderDoc Loader Plugin unloaded!"));
}

IMPLEMENT_MODULE(FRenderDocLoaderPluginModule, RenderDocLoaderPlugin)

#undef LOCTEXT_NAMESPACE 