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

#include "RenderDocPluginPrivatePCH.h"

#if WITH_EDITOR

#include "RenderDocPluginNotification.h"
#include "SNotificationList.h"
#include "NotificationManager.h"

void FRenderDocPluginNotification::ShowNotification(const FText& Message)
{
	LastEnableTime = FPlatformTime::Seconds();

	// Starting a new request! Notify the UI.
	if (RenderDocNotificationPtr.IsValid())
	{
		RenderDocNotificationPtr.Pin()->ExpireAndFadeout();
	}

	FNotificationInfo Info (Message);
	Info.bFireAndForget = false;

	// Setting fade out and expire time to 0 as the expire message is currently very obnoxious
	Info.FadeOutDuration = 1.0f;
	Info.ExpireDuration = 0.0f;

	RenderDocNotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);

	if (RenderDocNotificationPtr.IsValid())
	{
		RenderDocNotificationPtr.Pin()->SetCompletionState(SNotificationItem::CS_Pending);
	}
}

void FRenderDocPluginNotification::HideNotification()
{
	// Finished all requests! Notify the UI.
	TSharedPtr<SNotificationItem> NotificationItem = RenderDocNotificationPtr.Pin();

	if (NotificationItem.IsValid())
	{
		NotificationItem->SetCompletionState(SNotificationItem::CS_Success);
		NotificationItem->ExpireAndFadeout();

		RenderDocNotificationPtr.Reset();
	}
}

void FRenderDocPluginNotification::Tick(float DeltaTime)
{
	double OpenTime = 5.0;
	//if (bIsGUIOpen)
	//	OpenTime = 0.5;

	if (RenderDocNotificationPtr.IsValid() && (FPlatformTime::Seconds() - LastEnableTime) > OpenTime)
	{
		HideNotification();
	}
}

TStatId FRenderDocPluginNotification::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FRenderDocPluginNotification, STATGROUP_Tickables);
}

#endif//WITH_EDITOR
