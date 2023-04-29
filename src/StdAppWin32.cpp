/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2005, Günther
 * Copyright (c) 2017-2022, The LegacyClonk Team and contributors
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

#include "Standard.h"
#include "StdApp.h"
#include "res/engine_resource.h"

#include <array>
#include <mutex>
#include <stdexcept>

#include <mmsystem.h>
#include <stdio.h>
#include <io.h>
#include <ctype.h>
#include <conio.h>

CStdApp::CStdApp() : Active(false), hInstance(nullptr), fQuitMsgReceived(false),
	hTimerEvent(CreateEvent(nullptr, TRUE, FALSE, nullptr)),
	hNetworkEvent(CreateEvent(nullptr, TRUE, FALSE, nullptr)),
	idCriticalTimer(0),
	uCriticalTimerDelay(28),
	uCriticalTimerResolution(5),
	fTimePeriod(false),
	iLastExecute(0),
	iTimerOffset(0) {}

CStdApp::~CStdApp()
{
	// Close events
	CloseHandle(hTimerEvent); CloseHandle(hNetworkEvent);
}

void CStdApp::Init(HINSTANCE hInst, int nCmdShow, char *szCmdLine)
{
	// Set instance vars
	hInstance = hInst;
	this->szCmdLine = szCmdLine;
	mainThread = std::this_thread::get_id();
	// Custom initialization
	DoInit();
}

void CStdApp::Clear()
{
	// Close timers
	CloseCriticalTimer();
}

void CStdApp::Run()
{
	// Main message loop
	while (true)
		if (HandleMessage(INFINITE, true) == HR_Failure) return;
}

void CStdApp::Quit()
{
	PostQuitMessage(0);
}

C4AppHandleResult CStdApp::HandleMessage(unsigned int iTimeout, bool fCheckTimer)
{
	MSG msg;

	// quit check for nested HandleMessage-calls
	if (fQuitMsgReceived) return HR_Failure;

	// Calculate timing (emulate it sleepy - gosu-style [pssst]).
	unsigned int iMSecs;
	if (fCheckTimer && !MMTimer)
	{
		iMSecs = std::max<int>(0, iLastExecute + GetDelay() + iTimerOffset - timeGetTime());
		if (iTimeout != INFINITE && iTimeout < iMSecs) iMSecs = iTimeout;
	}
	else
	{
		iMSecs = iTimeout;
	}

#ifdef USE_CONSOLE
	// Console input
	if (!ReadStdInCommand())
		return HR_Failure;
#endif

	const std::array<HANDLE, 2> events{hNetworkEvent, hTimerEvent};

	// Wait for something to happen
	switch (MsgWaitForMultipleObjects(fCheckTimer ? 2 : 1, events.data(), false, iMSecs, QS_ALLEVENTS))
	{
	case WAIT_OBJECT_0: // network event
		// reset event
		ResetEvent(hNetworkEvent);
		// call network class to handle it
		OnNetworkEvents();
		return HR_Message;
	case WAIT_TIMEOUT: // timeout
		// Timeout not changed? Real timeout
		if (MMTimer || iMSecs == iTimeout)
		{
			return HR_Timeout;
		}
		// Try to make some adjustments. Still only as exact as timeGetTime().
		if (iLastExecute + GetDelay() > timeGetTime()) iTimerOffset++;
		if (iLastExecute + GetDelay() < timeGetTime()) iTimerOffset--;
		// fallthru
	case WAIT_OBJECT_0 + 1: // timer event / message
		if (fCheckTimer)
		{
			// reset event
			ResetEvent(hTimerEvent);
			// execute
			Execute();
			// return it
			return HR_Timer;
		}
		// fallthru
	case WAIT_OBJECT_0 + 2: // message
		// Peek messages
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			// quit?
			if (msg.message == WM_QUIT)
			{
				fQuitMsgReceived = true;
				return HR_Failure;
			}
			// Dialog message transfer
			if (!pWindow->Win32DialogMessageHandling(&msg))
			{
				TranslateMessage(&msg); DispatchMessage(&msg);
			}
		}
		return HR_Message;
	default: // error
		return HR_Failure;
	}
}

void CStdApp::Execute()
{
	// Timer emulation
	if (!MMTimer)
		iLastExecute = timeGetTime();
}

bool CStdApp::InitTimer()
{
	// Init game timers
	if (!SetCriticalTimer() || !SetTimer(pWindow->hWindow, SEC1_TIMER, SEC1_MSEC, nullptr)) return false;
	return true;
}

bool CStdApp::SetCriticalTimer()
{
	// Get resolution caps
	TIMECAPS tc;
	if (timeGetDevCaps(&tc, sizeof(tc)) != TIMERR_NOERROR)
		return false;
	// Establish minimum resolution
	uCriticalTimerResolution = BoundBy(uCriticalTimerResolution, tc.wPeriodMin, tc.wPeriodMax);
	if (timeBeginPeriod(uCriticalTimerResolution) != TIMERR_NOERROR)
		return false;
	fTimePeriod = true;
	if (MMTimer)
	{
		// Set critical timer
		if (!(idCriticalTimer = timeSetEvent(
			uCriticalTimerDelay, uCriticalTimerResolution,
			reinterpret_cast<LPTIMECALLBACK>(hTimerEvent),
			0, TIME_PERIODIC | TIME_CALLBACK_EVENT_SET)))
		{
			return false;
		}
	}
	return true;
}

void CStdApp::CloseCriticalTimer()
{
	if (idCriticalTimer)
	{
		timeKillEvent(idCriticalTimer);
		idCriticalTimer = 0;
	}
	if (fTimePeriod)
	{
		timeEndPeriod(uCriticalTimerResolution);
		fTimePeriod = false;
	}
}

void CStdApp::ResetTimer(unsigned int uDelay)
{
	uCriticalTimerDelay = uDelay;
	CloseCriticalTimer();
	SetCriticalTimer();
}

namespace
{
	struct ClipboardCleanup
	{
		~ClipboardCleanup() { CloseClipboard(); }
	};

	struct GlobalLockHandle
	{
		explicit GlobalLockHandle(HGLOBAL handle) : handle{handle} {}

		void lock()
		{
			data = reinterpret_cast<decltype(data)>(GlobalLock(handle));
		}

		void unlock()
		{
			GlobalUnlock(handle);
			data = nullptr;
		}

		bool try_lock()
		{
			if (!data)
			{
				lock();
				return true;
			}

			return false;
		}

		HGLOBAL handle;
		char *data{nullptr};
	};
}

bool CStdApp::Copy(std::string_view text, bool fClipboard)
{
	if (!fClipboard)
	{
		throw std::runtime_error{"No primary selection on Windows"};
	}

	// gain clipboard ownership
	if (!OpenClipboard(GetWindowHandle())) return false;
	const ClipboardCleanup cleanup;

	// must empty the global clipboard, so the application clipboard equals the Windows clipboard
	EmptyClipboard();

	// allocate a global memory object for the text.
	GlobalLockHandle handle{GlobalAlloc(GMEM_MOVEABLE, text.size() + 1)};
	if (!handle.handle)
	{
		return false;
	}

	// lock the handle and copy the text to the buffer.
	std::lock_guard guard{handle};
	if (!text.empty()) memcpy(handle.data, text.data(), text.size());
	handle.data[text.size()] = '\0';

	// place the handle on the clipboard.
	return SetClipboardData(CF_TEXT, handle.data);
}

std::string CStdApp::Paste(bool fClipboard)
{
	if (!fClipboard)
	{
		throw std::runtime_error{"No primary selection on Windows"};
	}

	if (!IsClipboardFormatAvailable(CF_TEXT) || !OpenClipboard(GetWindowHandle())) return "";

	const ClipboardCleanup cleanup;

	// get text from clipboard
	if (GlobalLockHandle handle{GetClipboardData(CF_TEXT)}; handle.handle)
	{
		std::lock_guard guard{handle};
		if (handle.data)
		{
			return handle.data;
		}
	}

	return "";
}

bool CStdApp::ReadStdInCommand()
{
	while (_kbhit())
	{
		// Surely not the most efficient way to do it, but we won't have to read much data anyway.
		char c = getch();
		if (c == '\r')
		{
			if (!CmdBuf.isNull())
			{
				OnCommand(CmdBuf.getData()); CmdBuf.Clear();
			}
		}
		else if (isprint((unsigned char)c))
			CmdBuf.AppendChar(c);
	}
	return true;
}
