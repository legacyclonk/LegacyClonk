/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
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

#pragma once

#include <C4AudioSystem.h>
#include <C4Config.h>
#include <C4Group.h>
#include <C4MusicSystem.h>
#include <C4SoundSystem.h>
#include <C4Components.h>
#include <C4InteractiveThread.h>
#include <C4Network2IRC.h>
#include "StdApp.h"
#include <StdWindow.h>

#include <optional>

class C4ToastSystem;
class CStdDDraw;

class C4Sec1Callback
{
public:
	virtual void OnSec1Timer() = 0;
	virtual ~C4Sec1Callback() {}
};

// callback interface for sec1timers
class C4Sec1TimerCallbackBase
{
protected:
	C4Sec1TimerCallbackBase *pNext;
	int iRefs;

public:
	C4Sec1TimerCallbackBase(); // ctor - ref set to 2
	virtual ~C4Sec1TimerCallbackBase() {}
	void Release() { if (!--iRefs) delete this; } // release: destruct in next cycle

protected:
	virtual void OnSec1Timer() = 0;
	bool IsReleased() { return iRefs <= 1; }

	friend class C4Application;
};

template <class T> class C4Sec1TimerCallback : public C4Sec1TimerCallbackBase
{
private:
	T *pCallback;

protected:
	virtual void OnSec1Timer() override { pCallback->OnSec1Timer(); }

public:
	C4Sec1TimerCallback(T *pCB) : pCallback(pCB) {}
};

/* Main class to initialize configuration and execute the game */

class C4Application : public CStdApp
{
private:
	// if set, this mission will be launched next
	StdStrBuf NextMission;

public:
	C4Application();
	~C4Application();
	// set by ParseCommandLine
	bool isFullScreen;
	// set by ParseCommandLine, if neither scenario nor direct join adress has been specified
	bool UseStartupDialog;
	// set by ParseCommandLine, for manually applying downloaded update packs
	StdStrBuf IncomingUpdate;
	// set by ParseCommandLine, for manually invoking an update check by command line or url
	bool CheckForUpdates;
	// Flag for launching editor on quit
	bool launchEditor;
	// Flag for restarting the engine at the end
	bool restartAtEnd;
	// main System.c4g in working folder
	C4Group SystemGroup;
	std::unique_ptr<C4AudioSystem> AudioSystem;
	std::optional<C4MusicSystem> MusicSystem;
	std::optional<C4SoundSystem> SoundSystem;
	std::unique_ptr<C4ToastSystem> ToastSystem;
	C4GamePadControl *pGamePadControl;
	// Thread for interactive processes (automatically starts as needed)
	C4InteractiveThread InteractiveThread;
	// IRC client for global chat
	C4Network2IRCClient IRCClient;
	// Tick timing
	unsigned int iLastGameTick, iGameTickDelay, iExtraGameTickDelay;
	class CStdDDraw *DDraw;
	virtual int32_t &ScreenWidth() override { return Config.Graphics.ResX; }
	virtual int32_t &ScreenHeight() override { return Config.Graphics.ResY; }
	virtual float GetScale() override { return Config.Graphics.Scale / 100.0f; }
	void Clear() override;
	void Execute() override;
	// System.c4g helper funcs
	bool OpenSystemGroup() { return SystemGroup.IsOpen() || SystemGroup.Open(C4CFN_System); }
	void DoSec1Timers();
	void SetGameTickDelay(int iDelay);
	bool SetResolution(int32_t iNewResX, int32_t iNewResY);
	bool SetGameFont(const char *szFontFace, int32_t iFontSize);

protected:
	enum State { C4AS_None, C4AS_PreInit, C4AS_Startup, C4AS_StartGame, C4AS_Game, C4AS_Quit, } AppState;

protected:
	virtual void DoInit() override;
	bool OpenGame();
	bool PreInit();
	virtual void OnNetworkEvents() override;
	static bool ProcessCallback(const char *szMessage, int iProcess);

	std::vector<std::unique_ptr<C4Sec1TimerCallbackBase>> sec1TimerCallbacks;
	void AddSec1Timer(C4Sec1TimerCallbackBase *callback);

	friend class C4Sec1TimerCallbackBase;

	virtual void OnCommand(const char *szCmd) override;

public:
	virtual void Quit() override;
	void QuitGame(); // quit game only, but restart application if in fullscreen startup menu mode
	void Activate(); // activate app to gain full focus in OS
	void SetNextMission(const char *szMissionFilename);
};

extern C4Application Application;
