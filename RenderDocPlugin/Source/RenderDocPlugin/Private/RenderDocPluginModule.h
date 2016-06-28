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

#pragma once

#include "IRenderDocPlugin.h"

#include "RenderDocPluginLoader.h"
#include "RenderDocPluginSettings.h"

#if WITH_EDITOR
#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "SlateBasics.h"
#include "MultiBoxExtender.h"
#include "RenderDocPluginStyle.h"
#include "RenderDocPluginCommands.h"
#include "RenderDocPluginToolbar.h"
#include "RenderDocPluginAboutWindow.h"
#endif//WITH_EDITOR

#include "SharedPointer.h"
#include "Engine.h"

DECLARE_LOG_CATEGORY_EXTERN(RenderDocPlugin, Log, All);

class FRenderDocPluginModule : public IRenderDocPlugin
{
public:	
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	// Tick made possible via the dummy input device declared below:
	void Tick(float DeltaTime);
	class FRenderDocDummyInputDevice;
	// Mandatory IInputDeviceModule override that spawns the dummy input device:
	virtual TSharedPtr< class IInputDevice > CreateInputDevice(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler) override;

	void BeginCapture();
	void EndCapture();

  friend class SRenderDocPluginToolbar;
	void CaptureFrame();
	void CaptureCurrentViewport();	
	void CaptureEntireFrame();

	void StartRenderDoc(FString FrameCaptureBaseDirectory);
	FString GetNewestCapture(FString BaseDirectory);

 	static void RunAsyncTask(ENamedThreads::Type Where, TFunction<void()> What);
	
	// UE4-related: enable DrawEvents during captures, if necessary:
	bool UE4_GEmitDrawEvents_BeforeCapture;
	void UE4_OverrideDrawEventsFlag(const bool flag=true);
	void UE4_RestoreDrawEventsFlag();

	FRenderDocPluginLoader Loader;
	FRenderDocPluginSettings RenderDocSettings;
	FRenderDocPluginLoader::RENDERDOC_API_CONTEXT* RenderDocAPI;

	// Tracks the frame count (tick number) for a full frame capture:
	uint32 TickNumber;

#if WITH_EDITOR
  FRenderDocPluginEditorExtension* EditorExtensions;
#endif//WITH_EDITOR

private:
	// TODO: refactor the plugin into subclasses:
	class InputDevice;
	class RenderDocLoader;
	class Settings;
	class FrameCapturer;
	class UserInterface;

};
