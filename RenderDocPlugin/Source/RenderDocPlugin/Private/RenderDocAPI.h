//////////////////////////////////////////////////////////////////////////
// This file is pieced together from several files from the renderdoc
// github repository
//////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>
#include <sstream>

#ifdef WIN32

#ifdef RENDERDOC_EXPORTS
#define RENDERDOC_API __declspec(dllexport)
#else
#define RENDERDOC_API __declspec(dllimport)
#endif
#define RENDERDOC_CC __cdecl

#elif defined(LINUX)

#ifdef RENDERDOC_EXPORTS
#define RENDERDOC_API __attribute__ ((visibility ("default")))
#else
#define RENDERDOC_API
#endif

#define RENDERDOC_CC

#else

#error "Unknown platform"

#endif

//////////////////////////////////////////////////////////////////////////
// Data types
//////////////////////////////////////////////////////////////////////////

typedef uint32_t bool32;

enum InAppOverlay
{
	eOverlay_Enabled = 0x1,
	eOverlay_FrameRate = 0x2,
	eOverlay_FrameNumber = 0x4,
	eOverlay_CaptureList = 0x8,

	eOverlay_Default = (eOverlay_Enabled|eOverlay_FrameRate|eOverlay_FrameNumber|eOverlay_CaptureList),
	eOverlay_All = ~0U,
	eOverlay_None = 0,
};

struct CaptureOptions
{
	CaptureOptions()
	: AllowVSync(true),
	AllowFullscreen(true),
	DebugDeviceMode(false),
	CaptureCallstacks(false),
	CaptureCallstacksOnlyDraws(false),
	DelayForDebugger(0),
	CacheStateObjects(true),
	HookIntoChildren(false),
	RefAllResources(false),
	SaveAllInitials(false),
	CaptureAllCmdLists(false)
	{}

	bool32 AllowVSync;
	bool32 AllowFullscreen;
	bool32 DebugDeviceMode;
	bool32 CaptureCallstacks;
	bool32 CaptureCallstacksOnlyDraws;
	uint32_t DelayForDebugger;
	bool32 CacheStateObjects;
	bool32 HookIntoChildren;
	bool32 RefAllResources;
	bool32 SaveAllInitials;
	bool32 CaptureAllCmdLists;

#ifdef __cplusplus
	void FromString(std::string str)
	{
		std::istringstream iss(str);

		iss >> AllowFullscreen
			>> AllowVSync
			>> DebugDeviceMode
			>> CaptureCallstacks
			>> CaptureCallstacksOnlyDraws
			>> DelayForDebugger
			>> CacheStateObjects
			>> HookIntoChildren
			>> RefAllResources
			>> SaveAllInitials
			>> CaptureAllCmdLists;
	}

	std::string ToString() const
	{
		std::ostringstream oss;

		oss << AllowFullscreen << " "
			<< AllowVSync << " "
			<< DebugDeviceMode << " "
			<< CaptureCallstacks << " "
			<< CaptureCallstacksOnlyDraws << " "
			<< DelayForDebugger << " "
			<< CacheStateObjects << " "
			<< HookIntoChildren << " "
			<< RefAllResources << " "
			<< SaveAllInitials << " "
			<< CaptureAllCmdLists << " ";

		return oss.str();
	}
#endif
};


//////////////////////////////////////////////////////////////////////////
// Injection/voluntary capture functions.
//////////////////////////////////////////////////////////////////////////

#define RENDERDOC_API_VERSION 1

extern "C" RENDERDOC_API int RENDERDOC_CC RENDERDOC_GetAPIVersion();
typedef int (RENDERDOC_CC *pRENDERDOC_GetAPIVersion)();

extern "C" RENDERDOC_API void RENDERDOC_CC RENDERDOC_SetLogFile(const wchar_t *logfile);
typedef void (RENDERDOC_CC *pRENDERDOC_SetLogFile)(const wchar_t *logfile);

extern "C" RENDERDOC_API void RENDERDOC_CC RENDERDOC_SetCaptureOptions(const CaptureOptions *opts);
typedef void (RENDERDOC_CC *pRENDERDOC_SetCaptureOptions)(const CaptureOptions *opts);

extern "C" RENDERDOC_API void RENDERDOC_CC RENDERDOC_SetActiveWindow(void *wndHandle);
typedef void (RENDERDOC_CC *pRENDERDOC_SetActiveWindow)(void *wndHandle);

extern "C" RENDERDOC_API void RENDERDOC_CC RENDERDOC_TriggerCapture();
typedef void (RENDERDOC_CC *pRENDERDOC_TriggerCapture)();

extern "C" RENDERDOC_API void RENDERDOC_CC RENDERDOC_StartFrameCapture(void *wndHandle);
typedef void (RENDERDOC_CC *pRENDERDOC_StartFrameCapture)(void *wndHandle);

extern "C" RENDERDOC_API bool RENDERDOC_CC RENDERDOC_EndFrameCapture(void *wndHandle);
typedef bool (RENDERDOC_CC *pRENDERDOC_EndFrameCapture)(void *wndHandle);

extern "C" RENDERDOC_API uint32_t RENDERDOC_CC RENDERDOC_GetOverlayBits();
typedef uint32_t (RENDERDOC_CC *pRENDERDOC_GetOverlayBits)();

extern "C" RENDERDOC_API void RENDERDOC_CC RENDERDOC_MaskOverlayBits(uint32_t and, uint32_t or);
typedef void (RENDERDOC_CC *pRENDERDOC_MaskOverlayBits)(uint32_t and, uint32_t or);

//////////////////////////////////////////////////////////////////////////
// Remote access and control
//////////////////////////////////////////////////////////////////////////

extern "C" RENDERDOC_API void RENDERDOC_CC RENDERDOC_InitRemoteAccess(uint32_t *ident);
typedef void (RENDERDOC_CC *pRENDERDOC_InitRemoteAccess)(uint32_t *ident);
