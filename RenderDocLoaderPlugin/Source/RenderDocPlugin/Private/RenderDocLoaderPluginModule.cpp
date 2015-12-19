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

#include "../../../../RenderDocAPI/renderdoc_app.h"

#define LOCTEXT_NAMESPACE "RenderDocLoaderPluginNamespace" 

static HINSTANCE LoadAndCheckRenderDocLibrary(const FString& RenderdocPath)
{
	if (RenderdocPath.IsEmpty())
		return(nullptr);

	FString PathToRenderDocDLL = FPaths::Combine(*RenderdocPath, *FString("renderdoc.dll"));
	HINSTANCE RenderDocDLL = LoadLibrary(*PathToRenderDocDLL);
	if (!RenderDocDLL)
		return(nullptr);

	pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)(void*)GetProcAddress(RenderDocDLL, "RENDERDOC_GetAPI");
	if (!RENDERDOC_GetAPI)
	{
		UE_LOG(RenderDocLoaderPlugin, Warning, TEXT("'%s' : Could not load renderdoc function 'RENDERDOC_GetAPI'. You are most likely using an incompatible version of Renderdoc."), *PathToRenderDocDLL);
		FreeLibrary(RenderDocDLL);
		return(nullptr);
	}

	RENDERDOC_API_1_0_0* RENDERDOC (nullptr);
	if (0 == RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_0_0, (void**)&RENDERDOC))
	{
		UE_LOG(RenderDocLoaderPlugin, Warning, TEXT("'%s' : RenderDoc initialization failed due to API incompatibility with eRENDERDOC_API_Version_1_0_0."), *PathToRenderDocDLL);
		FreeLibrary(RenderDocDLL);
		return(nullptr);
	}

	return(RenderDocDLL);
}

static void UpdateConfigFiles(const FString& RenderdocPath)
{
	if (GConfig)
	{
		GConfig->SetString(TEXT("RenderDoc"), TEXT("BinaryPath"), *RenderdocPath, GGameIni);
		GConfig->Flush(false, GGameIni);
	}
}

void FRenderDocLoaderPluginModule::StartupModule()
{
	if (GUsingNullRHI)
	{
		UE_LOG(RenderDocLoaderPlugin, Warning, TEXT("RenderDoc Plugin will not be loaded because a Null RHI (Cook Server, perhaps) is being used."));
		return;
	}
	
	// Look for a renderdoc.dll somewhere in the system:
	RenderDocDLL = NULL;

	// 1) Check the configuration files first:
	if (GConfig)
	{
		// 1.1) The Game configuration:
		FString RenderdocPath;
		GConfig->GetString(TEXT("RenderDoc"), TEXT("BinaryPath"), RenderdocPath, GGameIni);
		RenderDocDLL = LoadAndCheckRenderDocLibrary(RenderdocPath);
		if (!RenderDocDLL)
		{
			// 1.2) The Engine configuration:
			FString RenderdocPath;
			GConfig->GetString(TEXT("RenderDoc"), TEXT("BinaryPath"), RenderdocPath, GEngineIni);
			RenderDocDLL = LoadAndCheckRenderDocLibrary(RenderdocPath);
		}
	}

	// 2) Check for a RenderDoc system installation in the registry:
	if (!RenderDocDLL)
	{
		FString RenderdocPath;
		FWindowsPlatformMisc::QueryRegKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Classes\\RenderDoc.RDCCapture.1\\DefaultIcon\\"), TEXT(""), RenderdocPath);
		RenderDocDLL = LoadAndCheckRenderDocLibrary(RenderdocPath);
		if (RenderDocDLL)
			UpdateConfigFiles(RenderdocPath);
	}

	// 3) Check for a RenderDoc custom installation by prompting the user:
	if (!RenderDocDLL)
	{
		//Renderdoc does not seem to be installed, but it might be built from source or downloaded by archive, 
		//so prompt the user to navigate to the main exe file
		UE_LOG(RenderDocLoaderPlugin, Log, TEXT("RenderDoc is not installed! Please provide custom exe path..."));
		FString RenderdocPath;
		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
		if (DesktopPlatform)
		{
			FString Filter = TEXT("Renderdoc executable|renderdocui.exe");
			TArray<FString> OutFiles;
			if (DesktopPlatform->OpenFileDialog(NULL, TEXT("Locate main Renderdoc executable..."), TEXT(""), TEXT(""), Filter, EFileDialogFlags::None, OutFiles))
				RenderdocPath = OutFiles[0];
		}
		RenderdocPath = FPaths::GetPath(RenderdocPath);
		RenderDocDLL = LoadAndCheckRenderDocLibrary(RenderdocPath);
		if (RenderDocDLL)
			UpdateConfigFiles(RenderdocPath);
	}

	// 4) All bets are off; aborting...
	if (!RenderDocDLL)
	{
		UE_LOG(RenderDocLoaderPlugin, Error, TEXT("Could not locate RenderDoc! Aborting module load..."));
		return;
	}

	UE_LOG(RenderDocLoaderPlugin, Log, TEXT("RenderDoc Loader Plugin loaded!"));
}

void FRenderDocLoaderPluginModule::ShutdownModule()
{
	if (GUsingNullRHI)
		return;

	if (RenderDocDLL)
		FreeLibrary(RenderDocDLL);

	UE_LOG(RenderDocLoaderPlugin, Log, TEXT("RenderDoc Loader Plugin unloaded!"));
}

IMPLEMENT_MODULE(FRenderDocLoaderPluginModule, RenderDocLoaderPlugin)

#undef LOCTEXT_NAMESPACE 