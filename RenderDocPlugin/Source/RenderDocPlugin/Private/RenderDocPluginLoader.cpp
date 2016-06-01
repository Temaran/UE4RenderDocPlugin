/******************************************************************************
* The MIT License (MIT)
*
* Copyright (c) 2014-2016 Fredrik Lindh
*                         Marcos Slomp
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
#include "RenderDocPluginLoader.h"
#include "RenderDocPluginModule.h"

#include "Internationalization.h"

#include "Developer/DesktopPlatform/public/DesktopPlatformModule.h"

#define LOCTEXT_NAMESPACE "RenderDocLoaderPluginNamespace" 

static void* LoadAndCheckRenderDocLibrary(FRenderDocPluginLoader::RENDERDOC_API_CONTEXT*& RenderDocAPI, const FString& RenderdocPath)
{
	check(nullptr == RenderDocAPI);

	if (RenderdocPath.IsEmpty())
		return(nullptr);

	FString PathToRenderDocDLL = FPaths::Combine(*RenderdocPath, *FString("renderdoc.dll"));
	if (!FPaths::FileExists(PathToRenderDocDLL))
	{
		UE_LOG(RenderDocPlugin, Warning, TEXT("unable to locate RenderDoc library at: %s"), *PathToRenderDocDLL);
		return(nullptr);
	}

	UE_LOG(RenderDocPlugin, Log, TEXT("a RenderDoc library has been located at: %s"), *PathToRenderDocDLL);

	void* RenderDocDLL = FPlatformProcess::GetDllHandle(*PathToRenderDocDLL);
	if (!RenderDocDLL)
	{
		UE_LOG(RenderDocPlugin, Warning, TEXT("unable to dynamically load RenderDoc library"));
		return(nullptr);
	}

	pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)FPlatformProcess::GetDllExport(RenderDocDLL, TEXT("RENDERDOC_GetAPI"));
	if (!RENDERDOC_GetAPI)
	{
		UE_LOG(RenderDocPlugin, Warning, TEXT("unable to obtain 'RENDERDOC_GetAPI' function from 'renderdoc.dll'. You are likely using an incompatible version of RenderDoc."), *PathToRenderDocDLL);
		FPlatformProcess::FreeDllHandle(RenderDocDLL);
		return(nullptr);
	}

	// Version checking and reporting
	if (0 == RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_0_0, (void**)&RenderDocAPI))
	{
		UE_LOG(RenderDocPlugin, Warning, TEXT("unable to initialize RenderDoc library due to API incompatibility (plugin requires eRENDERDOC_API_Version_1_0_0)."), *PathToRenderDocDLL);
		FPlatformProcess::FreeDllHandle(RenderDocDLL);
		return(nullptr);
	}

	int major(0), minor(0), patch(0);
	RenderDocAPI->GetAPIVersion(&major, &minor, &patch);
	UE_LOG(RenderDocPlugin, Log, TEXT("RenderDoc library has been loaded (RenderDoc API v%i.%i.%i)."), major, minor, patch);

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

void FRenderDocPluginLoader::Initialize()
{
	if (GUsingNullRHI)
	{
		// THIS WILL NEVER TRIGGER because of a sort of chicken-and-egg problem: RenderDoc Loader is a PostConfigInit
		// plugin, and GUsingNullRHI is only initialized properly between PostConfigInit and PreLoadingScreen phases.
		// (nevertheless, keep this comment around for future iterations of UE4)
		UE_LOG(RenderDocPlugin, Warning, TEXT("this plugin will not be loaded because a null RHI (Cook Server, perhaps) is being used."));
		return;
	}
	
	// Look for a renderdoc.dll somewhere in the system:
	UE_LOG(RenderDocPlugin, Log, TEXT("locating RenderDoc library (renderdoc.dll)..."));
	RenderDocDLL = RenderDocAPI = NULL;

	// 1) Check the Game configuration files:
	if (GConfig)
	{
		FString RenderdocPath;
		GConfig->GetString(TEXT("RenderDoc"), TEXT("BinaryPath"), RenderdocPath, GGameIni);
		RenderDocDLL = LoadAndCheckRenderDocLibrary(RenderDocAPI, RenderdocPath);
	}

	// 2) Check for a RenderDoc system installation in the registry:
	if (!RenderDocDLL)
	{
		FString RenderdocPath;
		FWindowsPlatformMisc::QueryRegKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Classes\\RenderDoc.RDCCapture.1\\DefaultIcon\\"), TEXT(""), RenderdocPath);
		RenderDocDLL = LoadAndCheckRenderDocLibrary(RenderDocAPI, RenderdocPath);
		if (RenderDocDLL)
			UpdateConfigFiles(RenderdocPath);
	}

	// 3) Check for a RenderDoc custom installation by prompting the user:
	if (!RenderDocDLL)
	{
		//Renderdoc does not seem to be installed, but it might be built from source or downloaded by archive, 
		//so prompt the user to navigate to the main exe file
		UE_LOG(RenderDocPlugin, Log, TEXT("RenderDoc library not found; provide a custom installation location..."));
		FString RenderdocPath;
		// TODO: rework the logic here by improving error checking and reporting
		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
		if (DesktopPlatform)
		{
			FString Filter = TEXT("Renderdoc executable|renderdocui.exe");
			TArray<FString> OutFiles;
			if (DesktopPlatform->OpenFileDialog(NULL, TEXT("Locate main Renderdoc executable..."), TEXT(""), TEXT(""), Filter, EFileDialogFlags::None, OutFiles))
				RenderdocPath = OutFiles[0];
		}
		RenderdocPath = FPaths::GetPath(RenderdocPath);
		RenderDocDLL = LoadAndCheckRenderDocLibrary(RenderDocAPI, RenderdocPath);
		if (RenderDocDLL)
			UpdateConfigFiles(RenderdocPath);
	}

	// 4) All bets are off; aborting...
	if (!RenderDocDLL)
	{
		UE_LOG(RenderDocPlugin, Error, TEXT("unable to initialize the plugin because no RenderDoc libray has been located."));
		return;
	}

	UE_LOG(RenderDocPlugin, Log, TEXT("plugin has been loaded successfully."));
}

void FRenderDocPluginLoader::Release()
{
	if (GUsingNullRHI)
		return;

	if (RenderDocDLL)
		FPlatformProcess::FreeDllHandle(RenderDocDLL);

	UE_LOG(RenderDocPlugin, Log, TEXT("plugin has been unloaded."));
}

#undef LOCTEXT_NAMESPACE 
