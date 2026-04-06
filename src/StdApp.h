/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2005, Günther
 * Copyright (c) 2017-2024, The LegacyClonk Team and contributors
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

#pragma once

#include "C4Log.h"
#include "C4Windows.h"
#include "StdSync.h"
#include "StdWindow.h"

#include <thread>
#include <stdexcept>

#if defined(USE_SDL_MAINLOOP)
#include <StdSdlSubSystem.h>
#include <SDL3/SDL.h>
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
#define K_SUBTRACT SDL_SCANCODE_KP_MINUS
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
#define KEY_M SDL_SCANCODE_M // console mode control key
#define KEY_T SDL_SCANCODE_T // console mode control key
#define KEY_W SDL_SCANCODE_W // console mode control key
#define KEY_I SDL_SCANCODE_I // console mode control key
#define KEY_C SDL_SCANCODE_C // copy in GUI-editbox
#define KEY_V SDL_SCANCODE_V // paste in GUI-editbox
#define KEY_X SDL_SCANCODE_X // cut from GUI-editbox
#define KEY_A SDL_SCANCODE_A // select all in GUI-editbox
#define KEY_F SDL_SCANCODE_F // search in ScenSelDlg
#define MK_ALT SDL_KMOD_ALT
#define MK_CONTROL SDL_KMOD_CTRL
#define MK_SHIFT SDL_KMOD_SHIFT
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
#define MK_ALT 0
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

	bool Active{false};

	virtual void Clear();
	virtual void Execute();
	void Run();
	virtual void Quit();
	virtual int32_t &ScreenWidth() = 0;
	virtual int32_t &ScreenHeight() = 0;
	virtual float GetScale() = 0;
	C4AppHandleResult HandleMessage(unsigned int iTimeout = StdSync::Infinite, bool fCheckTimer = true);
	void SetDisplayMode(DisplayMode mode) { pWindow->SetDisplayMode(mode); }
	void ResetTimer(unsigned int uDelayMS);
	CStdWindow *pWindow;
	bool fQuitMsgReceived{false}; // if true, a quit message has been received and the application should terminate
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
#if !defined(USE_CONSOLE)
		if (pWindow)
		{
			pWindow->FlashWindow();
		}
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

#if defined(_WIN32)
	CStdEvent NetworkEvent{CStdEvent::AutoReset()}; // set if a network event occured
// TODO: Remove unused code
#endif

	const char *Location{""};
	void Init(int argc, char *argv[]);
	bool DoNotDelay{false};
	void NextTick(bool fYield);
	bool IsShiftDown() { return KeyMask & MK_SHIFT; }
	bool IsControlDown() { return KeyMask & MK_CONTROL; }
	bool IsAltDown() { return KeyMask & MK_ALT; }
	unsigned int GetModifiers() const { return KeyMask; }
	bool SignalNetworkEvent();

	void OnPipeInput();
	void OnStdInInput();

protected:
	std::uint32_t DelayNS{27777000}; // 36 FPS
	std::timespec LastExecute;
	int argc;
	char **argv;
	int Pipe[2];
	unsigned int KeyMask{0};

#if defined(USE_SDL_MAINLOOP)
	int nextWidth, nextHeight, nextBPP;
	std::optional<StdSdlSubSystem> sdlVideoSubSys;
	void HandleSDLEvent(SDL_Event &event);
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
