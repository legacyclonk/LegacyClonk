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
#ifdef USE_GL
#include <StdGL.h>
#endif
#include <StdWindow.h>
#include <mmsystem.h>
#include <stdio.h>
#include <io.h>
#include <ctype.h>
#include <conio.h>

#include "res/engine_resource.h"

#define C4FullScreenClassName "C4FullScreen"
LRESULT APIENTRY FullScreenWinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

CStdWindow::CStdWindow() : Active(false), hWindow(0) {}
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
		WS_POPUP,
		CW_USEDEFAULT, CW_USEDEFAULT, 0, 0,
		nullptr, nullptr, pApp->hInstance, nullptr);

#ifndef USE_CONSOLE
	// Show & focus
	ShowWindow(hWindow, SW_SHOWNORMAL);
	SetFocus(hWindow);
	ShowCursor(FALSE);
#endif

	return this;
}

void CStdWindow::Clear()
{
	// Destroy window
	if (hWindow) DestroyWindow(hWindow);
	hWindow = nullptr;
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
	// resize
	if (hWindow)
	{
		::SetWindowPos(hWindow, nullptr, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_NOZORDER);
	}
}

void CStdWindow::FlashWindow()
{
	// please activate me!
	if (hWindow)
		::FlashWindow(hWindow, FLASHW_ALL | FLASHW_TIMERNOFG);
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
	iTimerOffset(0),
	fDspModeSet(false),
	pfd{}, dspMode{}, OldDspMode{}
{
	pfd.nSize = sizeof(pfd);
	dspMode.dmSize = sizeof(dspMode);
	OldDspMode.dmSize = sizeof(OldDspMode);
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
		iMSecs = Max<int>(0, iLastExecute + GetDelay() + iTimerOffset - timeGetTime());
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

int GLMonitorInfoEnumCount;

BOOL CALLBACK GLMonitorInfoEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	// get to indexed monitor
	if (GLMonitorInfoEnumCount--) return TRUE;
	// store it
	CStdApp *pApp = (CStdApp *)dwData;
	pApp->hMon = hMonitor;
	pApp->MonitorRect = *lprcMonitor;
	return TRUE;
}

bool CStdApp::SetOutputAdapter(unsigned int iMonitor)
{
	Monitor = iMonitor;
	hMon = nullptr;
	// get monitor infos
	GLMonitorInfoEnumCount = iMonitor;
	EnumDisplayMonitors(nullptr, nullptr, GLMonitorInfoEnumProc, (LPARAM)this);
	// no monitor assigned?
	if (!hMon)
	{
		// Okay for primary; then just use a default
		if (!iMonitor)
		{
			MonitorRect.left = MonitorRect.top = 0;
			MonitorRect.right = ScreenWidth(); MonitorRect.bottom = ScreenHeight();
			return true;
		}
		else return false;
	}
	return true;
}

bool CStdApp::GetIndexedDisplayMode(int32_t iIndex, int32_t *piXRes, int32_t *piYRes, int32_t *piBitDepth, uint32_t iMonitor)
{
	// prepare search struct
	DEVMODE dmode{}; dmode.dmSize = sizeof(dmode);
	StdStrBuf Mon;
	if (iMonitor)
		Mon.Format("\\\\.\\Display%d", iMonitor + 1);
	// check if indexed mode exists
	if (!EnumDisplaySettings(Mon.getData(), iIndex, &dmode)) return false;
	// mode exists; return it
	if (piXRes) *piXRes = dmode.dmPelsWidth;
	if (piYRes) *piYRes = dmode.dmPelsHeight;
	if (piBitDepth) *piBitDepth = dmode.dmBitsPerPel;
	return true;
}

bool CStdApp::FindDisplayMode(unsigned int iXRes, unsigned int iYRes, unsigned int iMonitor)
{
	bool fFound = false;
	DEVMODE dmode;
	// if a monitor is given, search on that instead
	SetOutputAdapter(iMonitor);
	StdStrBuf Mon;
	if (iMonitor)
		Mon.Format("\\\\.\\Display%d", iMonitor + 1);
	// enumerate modes
	int i = 0;
	dmode = {}; dmode.dmSize = sizeof(dmode);
	while (EnumDisplaySettings(Mon.getData(), i++, &dmode))
		// size and bit depth is OK?
		if (dmode.dmPelsWidth == iXRes && dmode.dmPelsHeight == iYRes && dmode.dmBitsPerPel == 32)
		{
			// compare with found one
			if (fFound)
				// try getting a mode that is close to 85Hz, rather than taking the one with highest refresh rate
				// (which may set absurd modes on some devices)
				if (Abs<int>(85 - dmode.dmDisplayFrequency) > Abs<int>(85 - dspMode.dmDisplayFrequency))
					// the previous one was better
					continue;
			// choose this one
			fFound = true;
			dspMode = dmode;
		}
	return fFound;
}

bool CStdApp::SetFullScreen(bool fFullScreen, bool fMinimize)
{
	if (fFullScreen == fDspModeSet) return true;
#ifdef _DEBUG
	SetWindowPos(pWindow->hWindow, HWND_TOP, MonitorRect.left, MonitorRect.top, dspMode.dmPelsWidth, dspMode.dmPelsHeight, SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
	SetWindowLong(pWindow->hWindow, GWL_STYLE, (WS_VISIBLE | WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX));
	return true;
#else
	if (!fFullScreen)
	{
		ChangeDisplaySettings(nullptr, CDS_RESET);
		fDspModeSet = false;
		return true;
	}
	// save original display mode
	// if a monitor is given, use that instead
	char Mon[256];
	// change to that mode
	fDspModeSet = true;
	if (Monitor)
	{
		sprintf(Mon, "\\\\.\\Display%d", Monitor + 1);
		dspMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
		if (ChangeDisplaySettingsEx(Mon, &dspMode, nullptr, CDS_FULLSCREEN, nullptr) != DISP_CHANGE_SUCCESSFUL)
		{
			fDspModeSet = false;
		}
	}
	else
	{
		if (ChangeDisplaySettings(&dspMode, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
		{
			fDspModeSet = false;
		}
	}
	SetWindowPos(pWindow->hWindow, 0, MonitorRect.left, MonitorRect.top, dspMode.dmPelsWidth, dspMode.dmPelsHeight, 0);
	return fDspModeSet;
#endif
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
