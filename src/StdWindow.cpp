/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2005, Günther
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

/* A wrapper class to OS dependent event and window interfaces, WIN32 version */

#include <Standard.h>
#include "StdApp.h"
#include <StdRegistry.h>
#ifndef USE_CONSOLE
#include <StdGL.h>
#endif
#include <StdWindow.h>

#include <mutex>
#include <stdexcept>

#include <dwmapi.h>

#ifdef __MINGW32__
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

CStdWindow::~CStdWindow()
{
	CStdWindow::Clear();
}

bool CStdWindow::Init(CStdApp *const app, const char *const title, const C4Rect &bounds, CStdWindow *const parent)
{
	com = C4Com{winrt::apartment_type::multi_threaded};

	const WNDCLASSEX windowClass{GetWindowClass(app->hInstance)};
	RegisterClassEx(&windowClass);

	// Create window
	const auto [style, exStyle] = GetWindowStyle();
	hWindow = CreateWindowEx(
		exStyle,
		windowClass.lpszClassName,
		title,
		style,
		bounds.x, bounds.y, bounds.Wdt, bounds.Hgt,
		nullptr, nullptr, app->hInstance, this);

#ifndef USE_CONSOLE
	RestorePosition();
	SetFocus(hWindow);
#endif

	if (!taskBarList)
	{
		auto list = winrt::try_create_instance<ITaskbarList3>(CLSID_TaskbarList);
		if (list && SUCCEEDED(list->HrInit()))
		{
			taskBarList = std::move(list);
		}
	}

	Active = true;
	return true;
}

void CStdWindow::StorePosition()
{
	std::string id;
	std::string subKey;
	bool storeSize{false};
	if (GetPositionData(id, subKey, storeSize))
	{
		StoreWindowPosition(hWindow, id.c_str(), subKey.c_str(), storeSize);
	}
}

void CStdWindow::RestorePosition()
{
	std::string id;
	std::string subKey;
	bool storeSize{false};
	if (GetPositionData(id, subKey, storeSize))
	{
		RestoreWindowPosition(hWindow, id.c_str(), subKey.c_str());
	}
	else
	{
		ShowWindow(hWindow, SW_SHOWNORMAL);
	}
}

void CStdWindow::Clear()
{
	// Destroy window
	if (hWindow) DestroyWindow(hWindow);
	hWindow = nullptr;
	taskBarList = nullptr;
}

void CStdWindow::SetTitle(const char *const szToTitle)
{
	if (hWindow) SetWindowText(hWindow, szToTitle ? szToTitle : "");
}

bool CStdWindow::GetSize(C4Rect &rect)
{
	RECT clientRect;
	if (!hWindow || !GetClientRect(hWindow, &clientRect)) return false;
	rect.x = clientRect.left;
	rect.y = clientRect.top;
	rect.Wdt = clientRect.right - clientRect.left;
	rect.Hgt = clientRect.bottom - clientRect.top;
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
}

void CStdWindow::FlashWindow()
{
	// please activate me!
	if (hWindow)
		::FlashWindow(hWindow, FLASHW_ALL | FLASHW_TIMERNOFG);
}

void CStdWindow::SetDisplayMode(const DisplayMode mode)
{
	const auto fullscreen = mode == DisplayMode::Fullscreen;

	auto newStyle = GetWindowLong(hWindow, GWL_STYLE);
	auto newStyleEx = GetWindowLong(hWindow, GWL_EXSTYLE);
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

	if (taskBarList)
	{
		taskBarList->MarkFullscreenWindow(hWindow, fullscreen);
	}
}

void CStdWindow::SetProgress(const uint32_t progress)
{
	if (taskBarList)
	{
		if (progress == 100)
		{
			taskBarList->SetProgressState(hWindow, TBPF_NOPROGRESS);
		}
		else
		{
			taskBarList->SetProgressState(hWindow, TBPF_INDETERMINATE);
			taskBarList->SetProgressValue(hWindow, progress, 100);
		}
	}
}

void CStdWindow::Maximize()
{
	ShowWindow(hWindow, SW_SHOWMAXIMIZED);
}

void CStdWindow::SetPosition(const int x, const int y)
{
	SetWindowPos(hWindow, nullptr, x, y, 0, 0, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);
}

LRESULT CStdWindow::DefaultWindowProc(const HWND hwnd, const UINT uMsg, const WPARAM wParam, const LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NCCREATE:
	{
		auto *const window = reinterpret_cast<CStdWindow *>(reinterpret_cast<CREATESTRUCT *>(lParam)->lpCreateParams);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));

		const BOOL supportsDarkMode{window->SupportsDarkMode()};
		DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &supportsDarkMode, sizeof(supportsDarkMode));
	}
		break;

	case WM_DESTROY:
	{
		reinterpret_cast<CStdWindow *>(GetWindowLongPtr(hwnd, GWLP_USERDATA))->StorePosition();
		return 0;
	}
	default:
		break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
