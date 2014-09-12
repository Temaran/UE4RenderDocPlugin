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