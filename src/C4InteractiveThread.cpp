/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2017-2021, The LegacyClonk Team and contributors
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
	// reset event handlers
	callbacks.fill(nullptr);
}

C4InteractiveThread::~C4InteractiveThread()
{
	const CStdLock PushLock(&EventPushCSec);
	const CStdLock PopLock(&EventPopCSec);

	// Remove all items. This may leak data, if pData was allocated on the heap.
	while (!events.empty())
	{
		events.pop();
	}

	destroyed = true;
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

#ifdef C4INTERACTIVETHREAD_DEBUG_BAD_ANY_CAST
bool C4InteractiveThread::PushEvent(C4InteractiveEventType eventType, const std::any &data, const char *const file, const char *const function, const std::uint_least32_t line)
#else
bool C4InteractiveThread::PushEvent(C4InteractiveEventType eventType, const std::any &data)
#endif
{
	CStdLock PushLock(&EventPushCSec);
	if (destroyed) return false;

	events.push(Event{
					eventType,
					data,
#ifdef _DEBUG
					timeGetTime(),
#endif
#ifdef C4INTERACTIVETHREAD_DEBUG_BAD_ANY_CAST
					file,
					function,
					line,
#endif
				});

	PushLock.Clear();
#ifdef _WIN32
	// post message to main thread
	const bool success = SetEvent(Application.hNetworkEvent);
	if (!success)
		// ThreadLog most likely won't work here, so Log will be used directly in hope
		// it doesn't screw too much
		LogFatal("Network: could not post message to main thread!");
	return success;
#else
	return Application.SignalNetworkEvent();
#endif
}

#ifdef _DEBUG
double AvgNetEvDelay = 0;
#endif

bool C4InteractiveThread::PopEvent(C4InteractiveEventType &eventType, std::any &data) // (by main thread)
{
	CStdLock PopLock(&EventPopCSec);
	if (events.empty() || destroyed) return false;
	// get event
	const Event &event{events.front()};

	// return
	eventType = event.Type;
	data = event.Data;

#ifdef _DEBUG
	if (Game.IsRunning)
		AvgNetEvDelay += ((timeGetTime() - event.Time) - AvgNetEvDelay) / 100;
#endif
	// remove
	events.pop();
	return true;
}

void C4InteractiveThread::ProcessEvents() // by main thread
{
	C4InteractiveEventType eventType;
	std::any eventData;

	while (PopEvent(eventType, eventData))
	{
		switch (eventType)
		{
		// Logging
		case Ev_Log: case Ev_LogSilent: case Ev_LogFatal:
		{
			// Reconstruct the StdStrBuf which allocated the data.
			auto log = std::any_cast<const StdStrBuf &>(eventData);
			switch (eventType)
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

		// Other events: check for a registered handler
		default:
			if (eventType >= Ev_None && eventType <= Ev_Last && callbacks[eventType])
			{
				callbacks[eventType]->OnThreadEvent(eventType, eventData);
			}

			// Note that memory might leak if the event wasn't processed....
		}
	}
}

bool C4InteractiveThread::ThreadLog(const char *szMessage)
{
	// send to main thread
	return PushEvent(Ev_Log, StdStrBuf{szMessage});
}

bool C4InteractiveThread::ThreadLogS(const char *szMessage)
{
	// send to main thread
	return PushEvent(Ev_LogSilent, StdStrBuf{szMessage});
}
