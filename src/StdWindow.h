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

#include <Standard.h>
#include <StdBuf.h>

#ifdef _WIN32
const int SEC1_TIMER = 1, SEC1_MSEC = 1000;

#include <shobjidl.h>
#include <wrl/client.h>

#endif

#include <thread>
#include <stdexcept>

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
#define KEY_F ((uint16_t) 'F') // search in ScenSelDlg
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
#define KEY_F XK_f // search in ScenSelDlg
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
#include <StdSdlSubSystem.h>
#include <SDL.h>
#include <optional>
#define K_F1 SDL_SCANCODE_F1
#define K_F2 SDL_SCANCODE_F2
#define K_F3 SDL_SCANCODE_F3
#define K_F4 SDL_SCANCODE_F4
#define K_F5 SDL_SCANCODE_F5
#define K_F6 SDL_SCANCODE_F6
#define K_F7 SDL_SCANCODE_F7
#define K_F8 SDL_SCANCODE_F8
#define K_F9 SDL_SCANCODE_F9
#define K_F10 SDL_SCANCODE_F10
#define K_F11 SDL_SCANCODE_F11
#define K_F12 SDL_SCANCODE_F12
#define K_ADD SDL_SCANCODE_KP_PLUS
#define K_SUBTRACT SDL_SCANCODE_MINUS
#define K_MULTIPLY SDL_SCANCODE_KP_MULTIPLY
#define K_ESCAPE SDL_SCANCODE_ESCAPE
#define K_PAUSE SDL_SCANCODE_PAUSE
#define K_TAB SDL_SCANCODE_TAB
#define K_RETURN SDL_SCANCODE_RETURN
#define K_DELETE SDL_SCANCODE_DELETE
#define K_INSERT SDL_SCANCODE_INSERT
#define K_BACK SDL_SCANCODE_BACKSPACE
#define K_SPACE SDL_SCANCODE_SPACE
#define K_UP SDL_SCANCODE_UP
#define K_DOWN SDL_SCANCODE_DOWN
#define K_LEFT SDL_SCANCODE_LEFT
#define K_RIGHT SDL_SCANCODE_RIGHT
#define K_HOME SDL_SCANCODE_HOME
#define K_END SDL_SCANCODE_END
#define K_SCROLL SDL_SCANCODE_SCROLLOCK
#define K_MENU SDL_SCANCODE_MENU
#define K_PAGEUP SDL_SCANCODE_PAGEUP
#define K_PAGEDOWN SDL_SCANCODE_PAGEDOWN
#define KEY_M SDL_SCANCODE_M
#define KEY_T SDL_SCANCODE_T
#define KEY_W SDL_SCANCODE_W
#define KEY_I SDL_SCANCODE_I
#define KEY_C SDL_SCANCODE_C
#define KEY_V SDL_SCANCODE_V
#define KEY_X SDL_SCANCODE_X
#define KEY_A SDL_SCANCODE_A
#define KEY_F SDL_SCANCODE_F
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
#define KEY_F 0
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

enum class DisplayMode
{
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
	bool RestorePosition(const char *szWindowName, const char *szSubKey);
	bool GetSize(RECT *pRect);
	void SetSize(unsigned int cx, unsigned int cy); // resize
	void SetTitle(const char *Title);
	void FlashWindow();
	void SetDisplayMode(DisplayMode mode);
	void SetProgress(uint32_t progress); // progress 100 disables the progress bar

protected:
	virtual void Sec1Timer() {}

#ifdef _WIN32

public:
	HWND hWindow;
	HWND hRenderWindow;
	void Maximize();
	void SetPosition(int x, int y);
	HWND GetRenderWindow() { return hRenderWindow ? hRenderWindow : hWindow; }

protected:
	bool RegisterWindowClass(HINSTANCE hInst);
	virtual bool Win32DialogMessageHandling(MSG *msg) { return false; };

private:
	DWORD style = WS_OVERLAPPEDWINDOW;
	DWORD styleEx = 0;
	Microsoft::WRL::ComPtr<ITaskbarList3> taskBarList = nullptr;

	struct ComUnInit
	{
		ComUnInit() { CoInitializeEx(nullptr, COINIT_MULTITHREADED); }
		~ComUnInit() { CoUninitialize(); }
	} const comUninit;

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

class CStdApp
{
public:
	class StartupException : public std::runtime_error
	{
	public:
		using runtime_error::runtime_error;
	};

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
	bool Copy(std::string_view text, bool fClipboard = true);
	// Paste the text from the clipboard or the primary selection
	std::string Paste(bool fClipboard = true);
	// Is there something in the clipboard?
	bool IsClipboardFull(bool fClipboard = true);
	// a command from stdin
	virtual void OnCommand(const char *szCmd) = 0; // callback

	// notify user to get back to the program
	void NotifyUserIfInactive()
	{
#ifndef USE_CONSOLE
		if (pWindow) pWindow->FlashWindow();
#endif
	}

	bool IsMainThread()
	{
		return mainThread == std::this_thread::get_id();
	}

	bool AssertMainThread()
	{
		if (!IsMainThread())
		{
			throw std::runtime_error{"Not in main thread"};
		}

		return true;
	}

#ifdef _WIN32
	HINSTANCE hInstance;
	int iLastExecute, iTimerOffset;
	HANDLE hTimerEvent, // set periodically by critical timer (C4Engine)
		hNetworkEvent; // set if a network event occured
	void Init(HINSTANCE hInst, int nCmdShow, char *szCmdLine);
	void NextTick(bool fYield) { SetEvent(hTimerEvent); if (fYield) Sleep(0); }
	bool IsShiftDown() { return GetKeyState(VK_SHIFT) < 0; }
	bool IsControlDown() { return GetKeyState(VK_CONTROL) < 0; }
	bool IsAltDown() { return GetKeyState(VK_MENU) < 0; }
	HWND GetWindowHandle() { return pWindow ? pWindow->hWindow : nullptr; }

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
	void Init(int argc, char *argv[]);
	bool DoNotDelay;
	void NextTick(bool fYield);
	bool IsShiftDown() { return KeyMask & MK_SHIFT; }
	bool IsControlDown() { return KeyMask & MK_CONTROL; }
	bool IsAltDown() { return KeyMask & (1 << 3); }
	bool SignalNetworkEvent();

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
	std::optional<StdSdlSubSystem> sdlVideoSubSys;
#endif
	class CStdAppPrivate *Priv;
	void HandleXMessage();

	unsigned int Delay;
	timeval LastExecute;
	unsigned int KeyMask;
#endif
#ifdef __APPLE__
	std::string GetGameDataPath();
#endif
	const char *szCmdLine;
	std::thread::id mainThread{};
	bool InitTimer();
	virtual void DoInit() = 0;
	virtual void OnNetworkEvents() = 0;

	// commands from stdin (console only)
	StdStrBuf CmdBuf;
	bool ReadStdInCommand();

	friend class CStdGL;
	friend class CStdWindow;
	friend class CStdGtkWindow;
};
