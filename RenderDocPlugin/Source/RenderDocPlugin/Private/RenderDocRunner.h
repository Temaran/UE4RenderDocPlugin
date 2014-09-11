#pragma once

#include "Slate.h"
#include "ThreadingBase.h"

class FRenderDocRunner : public FRunnable
{	
public: 
	//Constructor / Destructor
	FRenderDocRunner();
	virtual ~FRenderDocRunner();
 
	// Begin FRunnable interface.
	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();
	// End FRunnable interface
 
	void StartRenderDoc(FString PathToRenderDocExecutable, FString FrameCaptureBaseDirectory, uint32 Port);

private:
	FRunnableThread* Thread;

	bool IsRunning;
	uint32 SocketPort;
	FString ExecutablePath;
	FString CaptureBaseDirectory;
};