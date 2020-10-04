/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2017-2019, The LegacyClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

#include "C4Include.h"
#include "C4InteractiveThread.h"
#include "C4Application.h"
#include "C4Log.h"

#include <C4Game.h>

#include <cassert>

// *** C4InteractiveThread

C4InteractiveThread::C4InteractiveThread()
{
	// Add head-item
	pFirstEvent = pLastEvent = new Event();
	pFirstEvent->Type = Ev_None;
	pFirstEvent->Next = nullptr;
	// reset event handlers
	std::fill(pCallbacks, std::end(pCallbacks), nullptr);
}

C4InteractiveThread::~C4InteractiveThread()
{
	CStdLock PushLock(&EventPushCSec), PopLock(&EventPopCSec);
	// Remove all items. This may leak data, if pData was allocated on the heap.
	while (PopEvent(nullptr, nullptr));
	// Delete head-item
	delete pFirstEvent;
	pFirstEvent = pLastEvent = nullptr;
}

bool C4InteractiveThread::AddProc(StdSchedulerProc *pProc)
{
	bool fFirst = !Scheduler.getProcCnt();
	// Add the proc
	Scheduler.Add(pProc);
	// Not started yet?
	if (fFirst)
		if (!Scheduler.Start())
			return false;
	return true;
}

void C4InteractiveThread::RemoveProc(StdSchedulerProc *pProc)
{
	// Process not in list?
	if (!Scheduler.hasProc(pProc))
		return;
	// Last proc to be removed?
	if (Scheduler.getProcCnt() == 1)
		Scheduler.Stop();
	// Remove
	Scheduler.Remove(pProc);
}

bool C4InteractiveThread::PushEvent(C4InteractiveEventType eEvent, const std::any &data)
{
	CStdLock PushLock(&EventPushCSec);
	if (!pLastEvent) return false;
	// create event
	Event *pEvent = new Event;
	pEvent->Type = eEvent;
	pEvent->Data = data;
#ifdef _DEBUG
	pEvent->Time = timeGetTime();
#endif
	pEvent->Next = nullptr;
	// add item (at end)
	pLastEvent->Next = pEvent;
	pLastEvent = pEvent;
	PushLock.Clear();
#ifdef _WIN32
	// post message to main thread
	bool fSuccess = !!SetEvent(Application.hNetworkEvent);
	if (!fSuccess)
		// ThreadLog most likely won't work here, so Log will be used directly in hope
		// it doesn't screw too much
		LogFatal("Network: could not post message to main thread!");
	return fSuccess;
#else
	return Application.SignalNetworkEvent();
#endif
}

#ifdef _DEBUG
double AvgNetEvDelay = 0;
#endif

bool C4InteractiveThread::PopEvent(C4InteractiveEventType *pEventType, std::any *data) // (by main thread)
{
	CStdLock PopLock(&EventPopCSec);
	if (!pFirstEvent) return false;
	// get event
	Event *pEvent = pFirstEvent->Next;
	if (!pEvent) return false;
	// return
	if (pEventType)
		*pEventType = pEvent->Type;
	if (data)
		*data = pEvent->Data;
#ifdef _DEBUG
	if (Game.IsRunning)
		AvgNetEvDelay += ((timeGetTime() - pEvent->Time) - AvgNetEvDelay) / 100;
#endif
	// remove
	delete pFirstEvent;
	pFirstEvent = pEvent;
	pFirstEvent->Type = Ev_None;
	return true;
}

void C4InteractiveThread::ProcessEvents() // by main thread
{
	C4InteractiveEventType eEventType; std::any eventData;
	while (PopEvent(&eEventType, &eventData))
		switch (eEventType)
		{
		// Logging
		case Ev_Log: case Ev_LogSilent: case Ev_LogFatal:
		{
			// Reconstruct the StdStrBuf which allocated the data.
			auto log = std::any_cast<const StdStrBuf &>(eventData);
			switch (eEventType)
			{
			case Ev_Log:
				Log(log.getData()); break;
			case Ev_LogSilent:
				LogSilent(log.getData()); break;
			case Ev_LogFatal:
				LogFatal(log.getData()); break;
			default:
				// can't really get here in any sane way
				assert(!"Unhandled switch case");
				break;
			}
		}
		break;

		// Execute in main thread
		case Ev_MainThread:
			std::any_cast<const std::function<void()> &>(eventData)();
			break;

		// Other events: check for a registered handler
		default:
			if (eEventType >= Ev_None && eEventType <= Ev_Last)
				if (pCallbacks[eEventType])
					pCallbacks[eEventType]->OnThreadEvent(eEventType, eventData);
			// Note that memory might leak if the event wasn't processed....
		}
}

bool C4InteractiveThread::ThreadLog(const char *szMessage)
{
	// send to main thread
	return PushEvent(Ev_Log, StdStrBuf{szMessage});
}

bool C4InteractiveThread::ThreadLogF(const char *szMessage, ...)
{
	// format message
	va_list lst; va_start(lst, szMessage);
	StdStrBuf Msg = FormatStringV(szMessage, lst);
	// send to main thread
	return PushEvent(Ev_Log, Msg);
}

bool C4InteractiveThread::ThreadLogS(const char *szMessage)
{
	// send to main thread
	return PushEvent(Ev_LogSilent, StdStrBuf{szMessage});
}

bool C4InteractiveThread::ThreadLogSF(const char *szMessage, ...)
{
	// format message
	va_list lst; va_start(lst, szMessage);
	StdStrBuf Msg = FormatStringV(szMessage, lst);
	// send to main thread
	return PushEvent(Ev_LogSilent, Msg);
}

bool C4InteractiveThread::ExecuteInMainThread(const std::function<void()> &function)
{
	return PushEvent(Ev_MainThread, function);
}
