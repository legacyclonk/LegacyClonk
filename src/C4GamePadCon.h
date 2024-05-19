/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2020, The LegacyClonk Team and contributors
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

/* Gamepad control - forwards gamepad events of opened gamepads to Game.KeyboardInput */

#pragma once

#ifdef _WIN32
#include "C4Windows.h"

#include <array>
#include <cinttypes>
#include <Xinput.h>

#endif

#ifdef USE_SDL_FOR_GAMEPAD
#include <C4KeyboardInput.h>
#include <StdSdlSubSystem.h>
#include <optional>
#include <set>
#endif

struct _SDL_Joystick;
typedef struct _SDL_Joystick SDL_Joystick;

union SDL_Event;
typedef union SDL_Event SDL_Event;

#ifdef _WIN32

class C4GamePad
{
public:
	enum AxisPos { Low, Mid, High, }; // quantized axis positions

	static constexpr int32_t
		MaxGamePad{ 4 }, // maximum number of supported gamepads
		NumStickAxes{ 4 },  // thumbstick axes
		NumAxes{ 6 };     // total axes including triggers

public:
	C4GamePad(int id);
	~C4GamePad();

public:
	bool Update(); // read current gamepad data
	uint32_t GetCurrentButtons(); // returns bitmask of pressed buttons for last retrieved info
	AxisPos GetAxisPos(int idAxis); // return axis extension - mid for error or center position

	const char* getButtonName(int wcode);
	const char* getAxisName(int wcode);

	void IncRef();
	bool DecRef();
	int32_t GetID() const { return id; }

public:
	std::array<AxisPos, NumAxes> AxisPosis;

	int iRefCount;
	uint32_t Buttons;

private:
	int id; // gamepad number
	XINPUT_STATE state; // WIN32 gamepad info
};
#endif

class C4GamePadControl
{
#ifdef _WIN32

private:
	std::array<C4GamePad *, C4GamePad::MaxGamePad> Gamepads{};
	int iNumGamepads;

public:
	void OpenGamepad(int id);  // add gamepad ref
	void CloseGamepad(int id); // del gamepad ref
	static C4GamePadControl *pInstance; // singleton

#elif defined(USE_SDL_FOR_GAMEPAD)

public:
	void FeedEvent(SDL_Event &e);

private:
	std::optional<StdSdlSubSystem> sdlJoystickSubSys;
	std::set<C4KeyCode> PressedAxis;

#endif

public:
	C4GamePadControl();
	~C4GamePadControl();
	void Clear();
	int GetGamePadCount();
	void Execute();
};

class C4GamePadOpener
{
#ifdef _WIN32
	int iGamePad;
#endif

public:
	C4GamePadOpener(int iGamePad);
	~C4GamePadOpener();
	void SetGamePad(int iNewGamePad);
#ifdef USE_SDL_FOR_GAMEPAD
	SDL_Joystick *Joy;
#endif
};
