/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2005, Günther
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

/* A wrapper class to OS dependent event and window interfaces, WIN32 version */

#include <Standard.h>
#include <StdRegistry.h>
#ifndef USE_CONSOLE
#include <StdGL.h>
#endif
#include <StdWindow.h>
#include <mmsystem.h>
#include <stdio.h>
#include <io.h>
#include <ctype.h>
#include <conio.h>
#include <stdexcept>

#include "res/engine_resource.h"

#define C4FullScreenClassName "C4FullScreen"
LRESULT APIENTRY FullScreenWinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

CStdWindow::CStdWindow() : Active(false), hWindow(nullptr), hRenderWindow(nullptr) {}
CStdWindow::~CStdWindow() {}

bool CStdWindow::RegisterWindowClass(HINSTANCE hInst)
{
	WNDCLASSEX WndClass;
	WndClass.cbSize = sizeof(WNDCLASSEX);
	WndClass.style = CS_DBLCLKS;
	WndClass.lpfnWndProc = FullScreenWinProc;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hInstance = hInst;
	WndClass.hCursor = nullptr;
	WndClass.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
	WndClass.lpszMenuName = nullptr;
	WndClass.lpszClassName = C4FullScreenClassName;
	WndClass.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_00_C4X));
	WndClass.hIconSm = LoadIcon(hInst, MAKEINTRESOURCE(IDI_00_C4X));
	return RegisterClassEx(&WndClass);
}

CStdWindow *CStdWindow::Init(CStdApp *pApp)
{
	Active = true;

	// Register window class
	if (!RegisterWindowClass(pApp->hInstance)) return nullptr;

	// Create window
	hWindow = CreateWindowEx(
		0,
		C4FullScreenClassName,
		STD_PRODUCT,
		style,
		CW_USEDEFAULT, CW_USEDEFAULT, 0, 0,
		nullptr, nullptr, pApp->hInstance, nullptr);

	RECT rect;
	GetClientRect(hWindow, &rect);

	hRenderWindow = CreateWindowEx(
		0,
		"STATIC",
		nullptr,
		WS_CHILD,
		0, 0, rect.right - rect.left, rect.bottom - rect.top,
		hWindow, nullptr, pApp->hInstance, nullptr);

	ShowWindow(hRenderWindow, SW_SHOW);

#ifndef USE_CONSOLE
	// Show & focus
	ShowWindow(hWindow, SW_SHOWNORMAL);
	SetFocus(hWindow);
#endif

	return this;
}

void CStdWindow::Clear()
{
	// Destroy window
	if (hRenderWindow) DestroyWindow(hRenderWindow);
	if (hWindow) DestroyWindow(hWindow);
	hWindow = nullptr;
	hRenderWindow = nullptr;
}

bool CStdWindow::RestorePosition(const char *szWindowName, const char *szSubKey, bool fHidden)
{
	if (!RestoreWindowPosition(hWindow, szWindowName, szSubKey, fHidden))
		ShowWindow(hWindow, SW_SHOWNORMAL);
	return true;
}

void CStdWindow::SetTitle(const char *szToTitle)
{
	if (hWindow) SetWindowText(hWindow, szToTitle ? szToTitle : "");
}

bool CStdWindow::GetSize(RECT *pRect)
{
	if (!(hWindow && GetClientRect(hWindow, pRect))) return false;
	return true;
}

void CStdWindow::SetSize(unsigned int cx, unsigned int cy)
{
	if (!hWindow) return;

	RECT rect = { 0, 0, static_cast<LONG>(cx), static_cast<LONG>(cy) };
	AdjustWindowRectEx(&rect, GetWindowLong(hWindow, GWL_STYLE), FALSE, GetWindowLong(hWindow, GWL_EXSTYLE));
	cx = rect.right - rect.left;
	cy = rect.bottom - rect.top;
	SetWindowPos(hWindow, nullptr, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_NOZORDER);

	if (hRenderWindow)
	{
		// Also resize child window
		SetWindowPos(hRenderWindow, nullptr, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_NOZORDER);
	}
}

void CStdWindow::FlashWindow()
{
	// please activate me!
	if (hWindow)
		::FlashWindow(hWindow, FLASHW_ALL | FLASHW_TIMERNOFG);
}

void CStdWindow::SetDisplayMode(DisplayMode mode)
{
	const auto fullscreen = mode == DisplayMode::Fullscreen;

	auto newStyle = style;
	auto newStyleEx = styleEx;
	if (fullscreen)
	{
		newStyle &= ~(WS_CAPTION | WS_THICKFRAME);
		newStyleEx &= ~(WS_EX_DLGMODALFRAME |
		WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
	}

	SetWindowLong(hWindow, GWL_STYLE, newStyle);
	SetWindowLong(hWindow, GWL_EXSTYLE, newStyleEx);

	if (fullscreen)
	{
		MONITORINFO monitorInfo;
		monitorInfo.cbSize = sizeof(monitorInfo);

		GetMonitorInfo(MonitorFromWindow(hWindow, MONITOR_DEFAULTTONEAREST), &monitorInfo);
		SetWindowPos(hWindow, nullptr, monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top, monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top, SWP_NOZORDER | SWP_FRAMECHANGED | SWP_DRAWFRAME);
	}
	else
	{
		SetWindowPos(hWindow, 0, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_DRAWFRAME);
	}

	ShowWindow(hWindow, SW_SHOW);

	if (!taskBarList)
	{
		HRESULT hr = ::CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&taskBarList));

		if (SUCCEEDED(hr) && FAILED(taskBarList->HrInit())) taskBarList = nullptr;
	}

	if (taskBarList)
	{
		taskBarList->MarkFullscreenWindow(hWindow, fullscreen);
	}
}

void CStdWindow::Maximize()
{
	ShowWindow(hWindow, SW_SHOWMAXIMIZED);
}

void CStdWindow::SetPosition(int x, int y)
{
	SetWindowPos(hWindow, nullptr, x, y, 0, 0, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);
}

/* CStdApp */

CStdApp::CStdApp() : Active(false), hInstance(nullptr), fQuitMsgReceived(false),
	hTimerEvent(CreateEvent(nullptr, TRUE, FALSE, nullptr)),
	hNetworkEvent(CreateEvent(nullptr, TRUE, FALSE, nullptr)),
	idCriticalTimer(0),
	uCriticalTimerDelay(28),
	uCriticalTimerResolution(5),
	fTimePeriod(false),
	iLastExecute(0),
	iTimerOffset(0)
{
	hMainThread = nullptr;
}

CStdApp::~CStdApp()
{
	// Close events
	CloseHandle(hTimerEvent); CloseHandle(hNetworkEvent);
}

char *LoadResStr(const char *id);

bool CStdApp::Init(HINSTANCE hInst, int nCmdShow, char *szCmdLine)
{
	// Set instance vars
	hInstance = hInst;
	this->szCmdLine = szCmdLine;
	hMainThread = ::GetCurrentThread();
	// Custom initialization
	return DoInit();
}

void CStdApp::Clear()
{
	// Close timers
	CloseCriticalTimer();
	hMainThread = nullptr;
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
	int iEvents = 0;
	HANDLE Events[3] = { hNetworkEvent, hTimerEvent };

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

	// Check network event
	Events[iEvents++] = hNetworkEvent;
	// Check timer
	if (fCheckTimer)
		Events[iEvents++] = hTimerEvent;

	// Wait for something to happen
	switch (MsgWaitForMultipleObjects(iEvents, Events, false, iMSecs, QS_ALLEVENTS))
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
			(LPTIMECALLBACK)hTimerEvent, 0, TIME_PERIODIC | TIME_CALLBACK_EVENT_SET)))
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
