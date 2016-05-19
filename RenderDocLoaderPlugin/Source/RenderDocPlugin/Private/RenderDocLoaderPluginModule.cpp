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

static void* LoadAndCheckRenderDocLibrary(const FString& RenderdocPath)
{
	if (RenderdocPath.IsEmpty())
		return(nullptr);

	FString PathToRenderDocDLL = FPaths::Combine(*RenderdocPath, *FString("renderdoc.dll"));
	if (!FPaths::FileExists(PathToRenderDocDLL))
	{
		UE_LOG(RenderDocLoaderPlugin, Warning, TEXT("unable to locate RenderDoc library (renderdoc.dll) at: %s"), *PathToRenderDocDLL);
		return(nullptr);
	}

	UE_LOG(RenderDocLoaderPlugin, Log, TEXT("a RenderDoc library (renderdoc.dll) has been located at: %s"), *PathToRenderDocDLL);

	void* RenderDocDLL = FPlatformProcess::GetDllHandle(*PathToRenderDocDLL);
	if (!RenderDocDLL)
	{
		UE_LOG(RenderDocLoaderPlugin, Warning, TEXT("unable to dynamically load RenderDoc library 'renderdoc.dll'."));
		return(nullptr);
	}

	pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)FPlatformProcess::GetDllExport(RenderDocDLL, TEXT("RENDERDOC_GetAPI"));
	if (!RENDERDOC_GetAPI)
	{
		UE_LOG(RenderDocLoaderPlugin, Warning, TEXT("unable to obtain 'RENDERDOC_GetAPI' function from 'renderdoc.dll'. You are likely using an incompatible version of RenderDoc."), *PathToRenderDocDLL);
		FPlatformProcess::FreeDllHandle(RenderDocDLL);
		return(nullptr);
	}

	// Version checking and reporting
	RENDERDOC_API_1_0_0* RENDERDOC (nullptr);
	if (0 == RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_0_0, (void**)&RENDERDOC))
	{
		UE_LOG(RenderDocLoaderPlugin, Warning, TEXT("unable to initialize RenderDoc library (renderdoc.dll) due to API incompatibility (requires eRENDERDOC_API_Version_1_0_0)."), *PathToRenderDocDLL);
		FPlatformProcess::FreeDllHandle(RenderDocDLL);
		return(nullptr);
	}

	int major(0), minor(0), patch(0);
	RENDERDOC->GetAPIVersion(&major, &minor, &patch);
	UE_LOG(RenderDocLoaderPlugin, Log, TEXT("RenderDoc library (renderdoc.dll) has been loaded (RenderDoc API v%i.%i.%i)."), major, minor, patch);

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
		// THIS WILL NEVER TRIGGER because of a sort of chicken-and-egg problem: RenderDoc Loader is a PostConfigInit
		// plugin, and GUsingNullRHI is only initialized properly between PostConfigInit and PreLoadingScreen phases.
		// (nevertheless, keep this comment around for future iterations of UE4)
		UE_LOG(RenderDocLoaderPlugin, Warning, TEXT("RenderDoc Plugin will not be loaded because a Null RHI (Cook Server, perhaps) is being used."));
		return;
	}
	
	// Look for a renderdoc.dll somewhere in the system:
	RenderDocDLL = NULL;

	// 1) Check the Game configuration files:
	if (GConfig)
	{
		FString RenderdocPath;
		GConfig->GetString(TEXT("RenderDoc"), TEXT("BinaryPath"), RenderdocPath, GGameIni);
		RenderDocDLL = LoadAndCheckRenderDocLibrary(RenderdocPath);
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
		UE_LOG(RenderDocLoaderPlugin, Error, TEXT("unable to locate RenderDoc libray (renderdoc.dll); aborting module load..."));
		return;
	}

	UE_LOG(RenderDocLoaderPlugin, Log, TEXT("RenderDoc Loader Plugin loaded!"));
}

void FRenderDocLoaderPluginModule::ShutdownModule()
{
	if (GUsingNullRHI)
		return;

	if (RenderDocDLL)
    FPlatformProcess::FreeDllHandle(RenderDocDLL);

	UE_LOG(RenderDocLoaderPlugin, Log, TEXT("RenderDoc Loader Plugin unloaded!"));
}

IMPLEMENT_MODULE(FRenderDocLoaderPluginModule, RenderDocLoaderPlugin)

#undef LOCTEXT_NAMESPACE 