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

/* A wrapper class to OS dependent event and window interfaces */

#pragma once

#include <Standard.h>
#include <StdBuf.h>

#ifdef _WIN32
const int SEC1_TIMER = 1, SEC1_MSEC = 1000;

#include <shobjidl.h>
#include <wrl/client.h>

#endif

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

#ifdef _WIN32
#define K_ALT VK_MENU
#define K_ESCAPE VK_ESCAPE
#define K_PAUSE VK_PAUSE
#define K_TAB VK_TAB
#define K_RETURN VK_RETURN
#define K_DELETE VK_DELETE
#define K_INSERT VK_INSERT
#define K_BACK VK_BACK
#define K_SPACE VK_SPACE
#define K_F1 VK_F1
#define K_F2 VK_F2
#define K_F3 VK_F3
#define K_F4 VK_F4
#define K_F5 VK_F5
#define K_F6 VK_F6
#define K_F7 VK_F7
#define K_F8 VK_F8
#define K_F9 VK_F9
#define K_F10 VK_F10
#define K_F11 VK_F11
#define K_F12 VK_F12
#define K_ADD VK_ADD
#define K_SUBTRACT VK_SUBTRACT
#define K_MULTIPLY VK_MULTIPLY
#define K_UP VK_UP
#define K_DOWN VK_DOWN
#define K_LEFT VK_LEFT
#define K_RIGHT VK_RIGHT
#define K_HOME VK_HOME
#define K_END VK_END
#define K_SCROLL VK_SCROLL
#define K_MENU VK_APPS
#define K_PAGEUP VK_PRIOR
#define K_PAGEDOWN VK_NEXT
#define KEY_A ((uint16_t) 'A') // select all in GUI-editbox
#define KEY_C ((uint16_t) 'C') // copy in GUI-editbox
#define KEY_I ((uint16_t) 'I') // console mode control key
#define KEY_M ((uint16_t) 'M') // console mode control key
#define KEY_T ((uint16_t) 'T') // console mode control key
#define KEY_V ((uint16_t) 'V') // paste in GUI-editbox
#define KEY_W ((uint16_t) 'W') // console mode control key
#define KEY_X ((uint16_t) 'X') // cut from GUI-editbox
#elif defined(USE_X11)
#include <X11/keysym.h>
#include <sys/time.h>
#define K_F1 XK_F1
#define K_F2 XK_F2
#define K_F3 XK_F3
#define K_F4 XK_F4
#define K_F5 XK_F5
#define K_F6 XK_F6
#define K_F7 XK_F7
#define K_F8 XK_F8
#define K_F9 XK_F9
#define K_F10 XK_F10
#define K_F11 XK_F11
#define K_F12 XK_F12
#define K_ADD XK_KP_Add
#define K_SUBTRACT XK_KP_Subtract
#define K_MULTIPLY XK_KP_Multiply
#define K_ESCAPE XK_Escape
#define K_PAUSE XK_Pause
#define K_TAB XK_Tab
#define K_RETURN XK_Return
#define K_DELETE XK_Delete
#define K_INSERT XK_Insert
#define K_BACK XK_BackSpace
#define K_SPACE XK_space
#define K_UP XK_Up
#define K_DOWN XK_Down
#define K_LEFT XK_Left
#define K_RIGHT XK_Right
#define K_HOME XK_Home
#define K_END XK_End
#define K_SCROLL XK_Scroll_Lock
#define K_MENU XK_Menu
#define K_PAGEUP XK_Page_Up
#define K_PAGEDOWN XK_Page_Down
#define KEY_A XK_a // select all in GUI-editbox
#define KEY_C XK_c // copy in GUI-editbox
#define KEY_I XK_i // console mode control key
#define KEY_M XK_m // console mode control key
#define KEY_T XK_t // console mode control key
#define KEY_V XK_v // paste in GUI-editbox
#define KEY_W XK_w // console mode control key
#define KEY_X XK_x // cut from GUI-editbox
// from X.h:
//#define ShiftMask (1 << 0)
//#define ControlMask (1 << 2)
#define MK_CONTROL (1 << 2)
#define MK_SHIFT (1 << 0)
#elif defined(USE_SDL_MAINLOOP)
#include <SDL.h>
#define K_F1 SDLK_F1
#define K_F2 SDLK_F2
#define K_F3 SDLK_F3
#define K_F4 SDLK_F4
#define K_F5 SDLK_F5
#define K_F6 SDLK_F6
#define K_F7 SDLK_F7
#define K_F8 SDLK_F8
#define K_F9 SDLK_F9
#define K_F10 SDLK_F10
#define K_F11 SDLK_F11
#define K_F12 SDLK_F12
#define K_ADD SDLK_KP_PLUS
#define K_SUBTRACT SDLK_KP_MINUS
#define K_MULTIPLY SDLK_KP_MULTIPLY
#define K_ESCAPE SDLK_ESCAPE
#define K_PAUSE SDLK_PAUSE
#define K_TAB SDLK_TAB
#define K_RETURN SDLK_RETURN
#define K_DELETE SDLK_DELETE
#define K_INSERT SDLK_INSERT
#define K_BACK SDLK_BACKSPACE
#define K_SPACE SDLK_SPACE
#define K_UP SDLK_UP
#define K_DOWN SDLK_DOWN
#define K_LEFT SDLK_LEFT
#define K_RIGHT SDLK_RIGHT
#define K_HOME SDLK_HOME
#define K_END SDLK_END
#define K_SCROLL SDLK_SCROLLOCK
#define K_MENU SDLK_MENU
#define K_PAGEUP SDLK_PAGEUP
#define K_PAGEDOWN SDLK_PAGEDOWN
#define KEY_M SDLK_m
#define KEY_T SDLK_t
#define KEY_W SDLK_w
#define KEY_I SDLK_i
#define KEY_C SDLK_c
#define KEY_V SDLK_v
#define KEY_X SDLK_x
#define KEY_A SDLK_a
#define MK_SHIFT (KMOD_LSHIFT | KMOD_RSHIFT)
#define MK_CONTROL (KMOD_LCTRL | KMOD_RCTRL)
#elif defined(USE_CONSOLE)
#define K_F1 0
#define K_F2 0
#define K_F3 0
#define K_F4 0
#define K_F5 0
#define K_F6 0
#define K_F7 0
#define K_F8 0
#define K_F9 0
#define K_F10 0
#define K_F11 0
#define K_F12 0
#define K_ADD 0
#define K_SUBTRACT 0
#define K_MULTIPLY 0
#define K_ESCAPE 0
#define K_PAUSE 0
#define K_TAB 0
#define K_RETURN 0
#define K_DELETE 0
#define K_INSERT 0
#define K_BACK 0
#define K_SPACE 0
#define K_UP 0
#define K_DOWN 0
#define K_LEFT 0
#define K_RIGHT 0
#define K_HOME 0
#define K_END 0
#define K_SCROLL 0
#define K_MENU 0
#define K_PAGEUP 0
#define K_PAGEDOWN 0
#define KEY_M 0
#define KEY_T 0
#define KEY_W 0
#define KEY_I 0
#define KEY_C 0
#define KEY_V 0
#define KEY_X 0
#define KEY_A 0
#define MK_SHIFT 0
#define MK_CONTROL 0
#else
#error need window system
#endif

enum C4AppHandleResult
{
	HR_Timeout,
	HR_Message, // handled a message
	HR_Timer,   // got timer event
	HR_Failure, // error, or quit message received
};

class CStdApp;
#ifdef USE_X11
// Forward declarations because xlib.h is evil
typedef union _XEvent XEvent;
typedef struct _XDisplay Display;
#endif

enum class DisplayMode {
	Fullscreen,
	Window
};

class CStdWindow
{
public:
	CStdWindow();
	virtual ~CStdWindow();
	bool Active;
	virtual void Clear();
	// Only when the wm requests a close
	// For example, when the user clicks the little x in the corner or uses Alt-F4
	virtual void Close() = 0;
	// Keypress(es) translated to a char
	virtual void CharIn(const char *c) {}
	virtual CStdWindow *Init(CStdApp *pApp);
#ifndef _WIN32
	virtual CStdWindow *Init(CStdApp *pApp, const char *Title, CStdWindow *pParent = nullptr, bool HideCursor = true);
#endif
	bool RestorePosition(const char *szWindowName, const char *szSubKey, bool fHidden = false);
	bool GetSize(RECT *pRect);
	void SetSize(unsigned int cx, unsigned int cy); // resize
	void SetTitle(const char *Title);
	void FlashWindow();
	void SetDisplayMode(DisplayMode mode);

protected:
	virtual void Sec1Timer() {};

#ifdef _WIN32

public:
	HWND hWindow;
	void Maximize();
	void SetPosition(int x, int y);

protected:
	bool RegisterWindowClass(HINSTANCE hInst);
	virtual bool Win32DialogMessageHandling(MSG *msg) { return false; };

private:
	DWORD style = WS_OVERLAPPEDWINDOW;
	DWORD styleEx = 0;
	Microsoft::WRL::ComPtr<ITaskbarList2> taskBarList = nullptr;

#elif defined(USE_X11)

protected:
	bool FindInfo();

	unsigned long wnd;
	unsigned long renderwnd;
	Display *dpy;
	virtual void HandleMessage(XEvent &);
	// The currently set window hints
	void *Hints;
	bool HasFocus; // To clear urgency hint
	// The XVisualInfo the window was created with
	void *Info;

#elif defined(USE_SDL_MAINLOOP)

private:
	int width, height;

protected:
	virtual void HandleMessage(SDL_Event &) {}

#endif

	friend class CStdDDraw;
	friend class CStdGL;
	friend class CStdGLCtx;
	friend class CStdApp;
	friend class CStdGtkWindow;
};

class CStdApp
{
public:
	CStdApp();
	virtual ~CStdApp();

	bool Active;
	bool MMTimer;

	virtual void Clear();
	virtual void Execute();
	void Run();
	virtual void Quit();
	virtual int32_t &ScreenWidth() = 0;
	virtual int32_t &ScreenHeight() = 0;
	virtual float GetScale() = 0;
	C4AppHandleResult HandleMessage(unsigned int iTimeout = INFINITE, bool fCheckTimer = true);
	void SetDisplayMode(DisplayMode mode) { pWindow->SetDisplayMode(mode); }
	void ResetTimer(unsigned int uDelay);
	CStdWindow *pWindow;
	bool fQuitMsgReceived; // if true, a quit message has been received and the application should terminate
	const char *GetCommandLine() { return szCmdLine; }

	// Copy the text to the clipboard or the primary selection
	void Copy(const StdStrBuf &text, bool fClipboard = true);
	// Paste the text from the clipboard or the primary selection
	StdStrBuf Paste(bool fClipboard = true);
	// Is there something in the clipboard?
	bool IsClipboardFull(bool fClipboard = true);
	// a command from stdin
	virtual void OnCommand(const char *szCmd) = 0; // callback

	// notify user to get back to the program
	void NotifyUserIfInactive()
	{
#ifdef _WIN32
		if (!Active && pWindow) pWindow->FlashWindow();
#elif defined(__APPLE__)
		if (pWindow) pWindow->FlashWindow();
#elif defined(USE_X11)
		if (pWindow) pWindow->FlashWindow();
#endif
	}

#ifdef _WIN32
	HINSTANCE hInstance;
	HANDLE hMainThread; // handle to main thread that initialized the app
	int iLastExecute, iTimerOffset;
	HANDLE hTimerEvent, // set periodically by critical timer (C4Engine)
		hNetworkEvent; // set if a network event occured
	bool Init(HINSTANCE hInst, int nCmdShow, char *szCmdLine);
	void NextTick(bool fYield) { SetEvent(hTimerEvent); if (fYield) Sleep(0); }
	bool IsShiftDown() { return GetKeyState(VK_SHIFT) < 0; }
	bool IsControlDown() { return GetKeyState(VK_CONTROL) < 0; }
	bool IsAltDown() { return GetKeyState(VK_MENU) < 0; }
	HWND GetWindowHandle() { return pWindow ? pWindow->hWindow : nullptr; }

	bool AssertMainThread()
	{
#ifdef _DEBUG
		if (hMainThread && hMainThread != ::GetCurrentThread())
		{
			assert(false);
			return false;
		}
#endif
		return true;
	}

protected:
	bool SetCriticalTimer();
	void CloseCriticalTimer();
	bool fTimePeriod;
	UINT uCriticalTimerDelay, uCriticalTimerResolution;
	UINT idCriticalTimer;
	UINT GetDelay() { return uCriticalTimerDelay; }
#else
#if defined(USE_X11)
	Display *dpy = nullptr;
	int xf86vmode_major_version, xf86vmode_minor_version;
#endif
#if defined(USE_SDL_MAINLOOP)
	void HandleSDLEvent(SDL_Event &event);
#endif
	const char *Location;
	pthread_t MainThread;
	bool Init(int argc, char *argv[]);
	bool DoNotDelay;
	void NextTick(bool fYield);
	bool IsShiftDown() { return KeyMask & MK_SHIFT; }
	bool IsControlDown() { return KeyMask & MK_CONTROL; }
	bool IsAltDown() { return KeyMask & (1 << 3); }
	bool SignalNetworkEvent();

	bool AssertMainThread()
	{
		assert(MainThread == pthread_self());
		return MainThread == pthread_self();
	}

	// These must be public to be callable from callback functions from
	// the glib main loop that are in an anonymous namespace in
	// StdXApp.cpp.
	void OnXInput();
	void OnPipeInput();
	void OnStdInInput();

protected:
#if defined(USE_SDL_MAINLOOP)
	int argc; char **argv;
	int Pipe[2];
	int nextWidth, nextHeight, nextBPP;
#endif
	class CStdAppPrivate *Priv;
	void HandleXMessage();

	unsigned int Delay;
	timeval LastExecute;
	unsigned int KeyMask;
#endif
	const char *szCmdLine;
	bool InitTimer();
	virtual bool DoInit() = 0;
	virtual void OnNetworkEvents() = 0;

	// commands from stdin (console only)
	StdCopyStrBuf CmdBuf;
	bool ReadStdInCommand();

	friend class CStdGL;
	friend class CStdWindow;
	friend class CStdGtkWindow;
};
