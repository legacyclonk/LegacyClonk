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

/* A wrapper class to OS dependent event and window interfaces */

#pragma once

#include "C4Rect.h"
#include <Standard.h>

#include "C4Rect.h"
#include <StdBuf.h>

#include <tuple>

#ifdef _WIN32
#include "C4Com.h"
#include "C4WinRT.h"
#include <shobjidl.h>
#endif

class CStdApp;
#ifdef USE_X11
// Forward declarations because xlib.h is evil
typedef union _XEvent XEvent;
typedef struct _XDisplay Display;
#elif defined(USE_SDL_MAINLOOP)
struct SDL_Window;
union SDL_Event;
#endif

enum class DisplayMode
{
	Fullscreen,
	Window
};

class CStdWindow
{
public:
	CStdWindow() = default;
	virtual ~CStdWindow();
	bool Active{false};
	virtual void Clear();
	// Only when the wm requests a close
	// For example, when the user clicks the little x in the corner or uses Alt-F4
	virtual void Close() = 0;
	// Keypress(es) translated to a char
	virtual void CharIn(const char *c) {}
	virtual bool Init(CStdApp *app, const char *title, const C4Rect &bounds = DefaultBounds, CStdWindow *parent = nullptr);
	void StorePosition();
	void RestorePosition();
	bool GetSize(C4Rect &rect);

#ifdef _WIN32
	virtual
#endif
	void SetSize(unsigned int cx, unsigned int cy); // resiz
	void SetTitle(const char *Title);
	void FlashWindow();
	void SetDisplayMode(DisplayMode mode);
	void SetProgress(uint32_t progress); // progress 100 disables the progress bar

protected:
	virtual void Sec1Timer() {}

#ifdef _WIN32

public:
	static constexpr C4Rect DefaultBounds{CW_USEDEFAULT, CW_USEDEFAULT, 0, 0};

	HWND hWindow{nullptr};
	void Maximize();
	void SetPosition(int x, int y);
	virtual HWND GetRenderWindow() const { return hWindow; }

	static LRESULT DefaultWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	virtual WNDCLASSEX GetWindowClass(HINSTANCE instance) const = 0;
	virtual bool Win32DialogMessageHandling(MSG *msg) { return false; };
	virtual bool GetPositionData(std::string &id, std::string &subKey, bool &storeSize) const { return {}; }
	virtual std::pair<DWORD, DWORD> GetWindowStyle() const { return {WS_OVERLAPPEDWINDOW, 0}; }

private:
	C4Com com{winrt::apartment_type::multi_threaded};
	DWORD style = WS_OVERLAPPEDWINDOW;
	DWORD styleEx = 0;
	winrt::com_ptr<ITaskbarList3> taskBarList{nullptr};

#elif defined(USE_X11)

public:
	static constexpr C4Rect DefaultBounds{0, 0, 640, 480};

protected:
	bool FindInfo();
	virtual bool HideCursor() const { return false; }

	unsigned long wnd{0};
	unsigned long renderwnd{0};
	Display *dpy{nullptr};
	virtual void HandleMessage(XEvent &);
	// The currently set window hints
	void *Hints{nullptr};
	bool HasFocus{false}; // To clear urgency hint
	// The XVisualInfo the window was created with
	void *Info{nullptr};

#elif defined(USE_SDL_MAINLOOP)
public:
	static constexpr C4Rect DefaultBounds{0, 0, 100, 100};

public:
	float GetInputScale();

private:
	int width, height;
	CStdApp *app;
	DisplayMode displayMode;

protected:
	SDL_Window *sdlWindow;
	virtual void HandleMessage(SDL_Event &) {}
#endif

	friend class CStdDDraw;
	friend class CStdGL;
	friend class CStdGLCtx;
	friend class CStdApp;
	friend class CStdGtkWindow;
};
