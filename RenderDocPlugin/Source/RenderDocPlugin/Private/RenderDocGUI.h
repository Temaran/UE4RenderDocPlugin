#pragma once

#include "Slate.h"
#include "ThreadingBase.h"

class FRenderDocGUI : public FRunnable
{	
public: 
	//Constructor / Destructor
	FRenderDocGUI();
	virtual ~FRenderDocGUI();
 
	// Begin FRunnable interface.
	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();
	// End FRunnable interface
 
	void StartRenderDoc(FString PathToRenderDocExecutable, FString FrameCaptureBaseDirectory, uint32 Port);
	FString GetNewestCapture(FString BaseDirectory);

private:
	FRunnableThread* Thread;

	bool IsRunning;
	uint32 SocketPort;
	FString ExecutablePath;
	FString CaptureBaseDirectory;
};