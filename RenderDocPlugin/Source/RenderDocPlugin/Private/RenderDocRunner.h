#pragma once

#include "Slate.h"
#include "ThreadingBase.h"

class FRenderDocRunner : public FRunnable
{	
public:
	/** Singleton instance, can access the thread any time via static accessor, if it is active! */
	static FRenderDocRunner* Runnable;
 
	/** Thread to run the worker FRunnable on */
	FRunnableThread* Thread;
 
	/** Stop this thread? Uses Thread Safe Counter */
	FThreadSafeCounter StopTaskCounter;
 
	//~~~ Thread Core Functions ~~~
 
	//Constructor / Destructor
	FRenderDocRunner(FString PathToRenderDocExecutable, FString FrameCaptureBaseDirectory);
	virtual ~FRenderDocRunner();
 
	// Begin FRunnable interface.
	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();
	// End FRunnable interface
 
	/** Makes sure this thread has stopped properly */
	void EnsureCompletion();
  
	/* 
		Start the thread and the worker from static (easy access)! 
		This code ensures only 1 thread will be able to run at a time. 
		This function returns a handle to the newly started instance.
	*/
	static FRenderDocRunner* LaunchRenderDoc(FString PathToRenderDocExecutable, FString FrameCaptureBaseDirectory);
 
	/** Shuts down the thread. Static so it can easily be called from outside the thread context */
	static void Shutdown();
 
private:
	FString ExecutablePath;
	FString CaptureBaseDirectory;
};