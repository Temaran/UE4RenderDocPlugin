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

#include "ModuleManager.h"
#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "SharedPointer.h"
#include "Internationalization.h"
#include "SlateBasics.h"
#include "MultiBoxExtender.h"

#include "IRenderDocPlugin.h"
#include "RenderDocPluginStyle.h"
#include "RenderDocPluginCommands.h"
#include "RenderDocPluginSettings.h"
#include "RenderDocPluginSettingsEditorWindow.h"
#include "RenderDocPluginAboutWindow.h"

#include "../../../../RenderDocAPI/renderdoc_app.h"

DECLARE_LOG_CATEGORY_EXTERN(RenderDocPlugin, Log, All);
DEFINE_LOG_CATEGORY(RenderDocPlugin);

/**
* A dummy input device in order to be able to listen and respond to engine tick
* events. The whole rendering activity between two engine ticks can be captured
* including SceneCapture updates, Material Editor previews, Material Thumbnail
* previews, Editor UI (Slate) widget rendering, etc.
*/
class FRenderDocDummyInputDevice : public IInputDevice
{
public:
	FRenderDocDummyInputDevice(const TSharedRef< FGenericApplicationMessageHandler >& MessageHandler) : ThePlugin(NULL) { }
	virtual ~FRenderDocDummyInputDevice() { }

	/** Tick the interface (used for controlling full engine frame captures). */
	virtual void Tick(float DeltaTime) override;

	/** The remaining interfaces are irrelevant for this dummy input device. */
	virtual void SendControllerEvents() override { }
	virtual void SetMessageHandler(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler) override { }
	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override { return(false); }
	virtual void SetChannelValue(int32 ControllerId, FForceFeedbackChannelType ChannelType, float Value) override { }
	virtual void SetChannelValues(int32 ControllerId, const FForceFeedbackValues &values) override { }

private:
	friend class FRenderDocPluginModule;
	class FRenderDocPluginModule* ThePlugin;

};

class FRenderDocPluginModule : public IRenderDocPlugin
{
public:	
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	virtual TSharedPtr< class IInputDevice > CreateInputDevice(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler) override
	{
		UE_LOG(RenderDocPlugin, Log, TEXT("Create Input Device"));
		TSharedPtr<FRenderDocDummyInputDevice> InputDev = MakeShareable( new FRenderDocDummyInputDevice(InMessageHandler) );
		InputDev->ThePlugin = this;
		return(InputDev);
	}

private:
	friend class FRenderDocDummyInputDevice;
	void Tick(float DeltaTime);

private:
	static const FName SettingsUITabName;

	FDelegateHandle LoadedDelegateHandle;

	TSharedPtr<FExtensibilityManager> ExtensionManager;
	TSharedPtr<FExtender> ToolbarExtender;
	TSharedPtr<const FExtensionBase> ToolbarExtension;

	FRenderDocPluginSettings RenderDocSettings;
	void* RenderDocDLL;
	bool IsInitialized;

	void OnEditorLoaded(SWindow& SlateWindow, void* ViewportRHIPtr);

	void BeginCapture();
	void EndCapture();

	void CaptureCurrentViewport();	
	void CaptureEntireFrame();
	void OpenSettingsEditorWindow();

	void StartRenderDoc(FString FrameCaptureBaseDirectory);
	FString GetNewestCapture(FString BaseDirectory);

	void AddToolbarExtension(FToolBarBuilder& ToolbarBuilder); 

	void* GetRenderDocFunctionPointer(void* ModuleHandle, const TCHAR* FunctionName);

 	static void RunAsyncTask(ENamedThreads::Type Where, TFunction<void()> What);
	
	// RenderDoc API context
	typedef RENDERDOC_API_1_0_0 RENDERDOC_API_CONTEXT;
	RENDERDOC_API_CONTEXT* RENDERDOC;

	// UE4-related: enable DrawEvents during captures, if necessary:
	bool UE4_GEmitDrawEvents_BeforeCapture;
	void UE4_OverrideDrawEventsFlag(const bool flag=true);
	void UE4_RestoreDrawEventsFlag();

	// Tracks the frame count (tick number) for a full frame capture:
	uint32 TickNumber;
};
