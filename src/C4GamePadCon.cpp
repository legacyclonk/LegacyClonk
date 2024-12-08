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

/* Gamepad control */

#include <C4Include.h>
#include <C4GamePadCon.h>

#include <C4ObjectCom.h>
#include <C4Log.h>
#include <C4Game.h>

#ifdef _WIN32
#include "C4Windows.h"
#include <numbers>
#include <windowsx.h>

static uint32_t POV2Position(DWORD dwPOV, bool fVertical)
{
	// POV value is a 360° angle multiplied by 100
	double dAxis;
	// Centered
	if (dwPOV == JOY_POVCENTERED)
		dAxis = 0.0;
	// Angle: convert to linear value -100 to +100
	else
	{
		const auto angleRad = dwPOV * std::numbers::pi / (180.0 * 100);
		dAxis = (fVertical ? -cos(angleRad) : sin(angleRad)) * 100.0;
	}
	// Gamepad configuration wants unsigned and gets 0 to 200
	return static_cast<uint32_t>(dAxis + 100.0);
}

C4GamePad::C4GamePad(int id) : iRefCount{1}, Buttons{0}, id{id}
{
	std::fill(AxisPosis.begin(), AxisPosis.end(), Mid);
	SetCalibration(&(Config.Gamepads[id].AxisMin[0]), &(Config.Gamepads[id].AxisMax[0]), &(Config.Gamepads[id].AxisCalibrated[0]));
}

C4GamePad::~C4GamePad()
{
	GetCalibration(&(Config.Gamepads[id].AxisMin[0]), &(Config.Gamepads[id].AxisMax[0]), &(Config.Gamepads[id].AxisCalibrated[0]));
}

void C4GamePad::SetCalibration(uint32_t *pdwAxisMin, uint32_t *pdwAxisMax, bool *pfAxisCalibrated)
{
	// params to calibration
	for (int i = 0; i < MaxCalAxis; ++i)
	{
		dwAxisMin[i] = pdwAxisMin[i];
		dwAxisMax[i] = pdwAxisMax[i];
		fAxisCalibrated[i] = pfAxisCalibrated[i];
	}
}

void C4GamePad::GetCalibration(uint32_t *pdwAxisMin, uint32_t *pdwAxisMax, bool *pfAxisCalibrated)
{
	// calibration to params
	for (int i = 0; i < MaxCalAxis; ++i)
	{
		pdwAxisMin[i] = dwAxisMin[i];
		pdwAxisMax[i] = dwAxisMax[i];
		pfAxisCalibrated[i] = fAxisCalibrated[i];
	}
}

bool C4GamePad::Update()
{
	joynfo.dwSize = sizeof(joynfo);
	joynfo.dwFlags = JOY_RETURNBUTTONS | JOY_RETURNRAWDATA | JOY_RETURNX | JOY_RETURNY | JOY_RETURNZ | JOY_RETURNR | JOY_RETURNU | JOY_RETURNV | JOY_RETURNPOV;
	return joyGetPosEx(JOYSTICKID1 + id, &joynfo) == JOYERR_NOERROR;
}

uint32_t C4GamePad::GetCurrentButtons()
{
	return joynfo.dwButtons;
}

constexpr DWORD GetAxisValue(const JOYINFOEX &info, const int axis) noexcept
{
	assert(Inside(axis, 0, C4GamePad::MaxCalAxis - 1));
	switch (axis)
	{
		case 0: return info.dwXpos;
		case 1: return info.dwYpos;
		case 2: return info.dwZpos;
		case 3: return info.dwRpos;
		case 4: return info.dwUpos;
		case 5: return info.dwVpos;
	}
	return -1;
}

C4GamePad::AxisPos C4GamePad::GetAxisPos(int idAxis)
{
	if (idAxis < 0 || idAxis >= MaxAxis) return Mid; // wrong axis
	// get raw axis data
	if (idAxis < MaxCalAxis)
	{
		uint32_t dwPos = GetAxisValue(joynfo, idAxis);
		// evaluate axis calibration
		if (fAxisCalibrated[idAxis])
		{
			// update it
			dwAxisMin[idAxis] = std::min<uint32_t>(dwAxisMin[idAxis], dwPos);
			dwAxisMax[idAxis] = std::max<uint32_t>(dwAxisMax[idAxis], dwPos);
			// Calculate center
			uint32_t dwCenter = (dwAxisMin[idAxis] + dwAxisMax[idAxis]) / 2;
			// Trigger range is 30% off center
			uint32_t dwRange = (dwAxisMax[idAxis] - dwCenter) / 3;
			if (dwPos < dwCenter - dwRange) return Low;
			if (dwPos > dwCenter + dwRange) return High;
		}
		else
		{
			// init it
			dwAxisMin[idAxis] = dwAxisMax[idAxis] = dwPos;
			fAxisCalibrated[idAxis] = true;
		}
	}
	else
	{
		// It's a POV head
		uint32_t dwPos = POV2Position(joynfo.dwPOV, idAxis == AxisPOV::Y);
		if (dwPos > 130) return High; else if (dwPos < 70) return Low;
	}
	return Mid;
}

void C4GamePad::IncRef()
{
	++iRefCount;
}

bool C4GamePad::DecRef()
{
	if (!--iRefCount)
	{
		delete this;
		return false;
	}

	return true;
}

C4GamePadControl *C4GamePadControl::pInstance = nullptr;

C4GamePadControl::C4GamePadControl()
{
	iNumGamepads = 0;
	// singleton
	if (!pInstance) pInstance = this;
}

C4GamePadControl::~C4GamePadControl()
{
	if (pInstance == this) pInstance = nullptr;
	Clear();
}

void C4GamePadControl::Clear()
{
	for (int i = 0; i < C4GamePad::MaxGamePad; ++i)
		while (Gamepads[i]) Gamepads[i]->DecRef();

	iNumGamepads = 0;
}

void C4GamePadControl::OpenGamepad(int id)
{
	if (!Inside(id, 0, C4GamePad::MaxGamePad - 1)) return;

	if (!(Gamepads[id]))
	{
		Gamepads[id] = new C4GamePad{id};
		++iNumGamepads;
	}
	else
	{
		Gamepads[id]->IncRef();
	}
}

void C4GamePadControl::CloseGamepad(int id)
{
	if (!Inside(id, 0, C4GamePad::MaxGamePad - 1)) return;

	if (!Gamepads[id]->DecRef())
	{
		Gamepads[id] = nullptr;
		--iNumGamepads;
	}
}

int C4GamePadControl::GetGamePadCount()
{
	JOYINFOEX joy{};
	joy.dwSize = sizeof(JOYINFOEX); joy.dwFlags = JOY_RETURNALL;
	int iCnt = 0;
	while (iCnt < C4GamePad::MaxGamePad && ::joyGetPosEx(iCnt, &joy) == JOYERR_NOERROR) ++iCnt;
	return iCnt;
}

const int MaxGamePadButton = 22;

void C4GamePadControl::Execute()
{
	// Get gamepad inputs
	for (auto &pad : Gamepads)
	{
		if (!pad || !pad->Update()) continue;
		for (int iAxis = 0; iAxis < C4GamePad::MaxAxis; ++iAxis)
		{
			C4GamePad::AxisPos eAxisPos = pad->GetAxisPos(iAxis), ePrevAxisPos = pad->AxisPosis[iAxis];
			// Evaluate changes and pass single controls
			// this is a generic Gamepad-control: Create events
			if (eAxisPos != ePrevAxisPos)
			{
				pad->AxisPosis[iAxis] = eAxisPos;
				if (ePrevAxisPos != C4GamePad::Mid)
					Game.DoKeyboardInput(KEY_Gamepad(pad->GetID(), KEY_JOY_Axis(iAxis, (ePrevAxisPos == C4GamePad::High))), KEYEV_Up, false, false, false, false);
				if (eAxisPos != C4GamePad::Mid)
					Game.DoKeyboardInput(KEY_Gamepad(pad->GetID(), KEY_JOY_Axis(iAxis, (eAxisPos == C4GamePad::High))), KEYEV_Down, false, false, false, false);
			}
		}

		const uint32_t Buttons = pad->GetCurrentButtons();
		const uint32_t PrevButtons = pad->Buttons;
		if (Buttons != PrevButtons)
		{
			pad->Buttons = Buttons;
			for (int iButton = 0; iButton < MaxGamePadButton; ++iButton)
				if ((Buttons & (1 << iButton)) != (PrevButtons & (1 << iButton)))
				{
					bool fRelease = ((Buttons & (1 << iButton)) == 0);
					Game.DoKeyboardInput(KEY_Gamepad(pad->GetID(), KEY_JOY_Button(iButton)), fRelease ? KEYEV_Up : KEYEV_Down, false, false, false, false);
				}
		}
	}
}

C4GamePadOpener::C4GamePadOpener(int iGamepad)
{
	assert(C4GamePadControl::pInstance);
	this->iGamePad = iGamepad;
	C4GamePadControl::pInstance->OpenGamepad(iGamePad);
}

C4GamePadOpener::~C4GamePadOpener()
{
	if (C4GamePadControl::pInstance)
		C4GamePadControl::pInstance->CloseGamepad(iGamePad);
}

void C4GamePadOpener::SetGamePad(int iNewGamePad)
{
	if (iNewGamePad == iGamePad) return;
	assert(C4GamePadControl::pInstance);
	C4GamePadControl::pInstance->CloseGamepad(iGamePad);
	C4GamePadControl::pInstance->OpenGamepad(iGamePad = iNewGamePad);
}

#elif defined(USE_SDL_FOR_GAMEPAD)

#include <SDL.h>
#include <stdexcept>

C4GamePadControl::C4GamePadControl()
{
	// Initialize SDL, if necessary.
	try
	{
		sdlJoystickSubSys.emplace(SDL_INIT_JOYSTICK);
	}
	catch (const std::runtime_error &e)
	{
		LogNTr(spdlog::level::err, "SDL: {}", e.what());
		// TODO: Handle
		throw;
	}
	SDL_JoystickEventState(SDL_ENABLE);
	if (!SDL_NumJoysticks()) LogNTr("No Gamepad found");
}

C4GamePadControl::~C4GamePadControl() {}

void C4GamePadControl::Execute()
{
#ifndef USE_SDL_MAINLOOP
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_JOYAXISMOTION:
		case SDL_JOYBALLMOTION:
		case SDL_JOYHATMOTION:
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			FeedEvent(event);
			break;
		}
	}
#endif
}

namespace
{
	const int deadZone = 13337;

	int amplify(int i)
	{
		if (i < 0)
			return -(deadZone + 1);
		if (i > 0)
			return deadZone + 1;
		return 0;
	}
}

void C4GamePadControl::FeedEvent(SDL_Event &event)
{
	switch (event.type)
	{
	case SDL_JOYHATMOTION:
	{
		SDL_Event fakeX;
		fakeX.jaxis.type = SDL_JOYAXISMOTION;
		fakeX.jaxis.which = event.jhat.which;
		fakeX.jaxis.axis = event.jhat.hat * 2 + 6; /* *magic*number* */
		fakeX.jaxis.value = 0;
		SDL_Event fakeY = fakeX;
		fakeY.jaxis.axis += 1;
		switch (event.jhat.value)
		{
		case SDL_HAT_LEFTUP:    fakeX.jaxis.value = amplify(-1); fakeY.jaxis.value = amplify(-1); break;
		case SDL_HAT_LEFT:      fakeX.jaxis.value = amplify(-1); break;
		case SDL_HAT_LEFTDOWN:  fakeX.jaxis.value = amplify(-1); fakeY.jaxis.value = amplify(+1); break;
		case SDL_HAT_UP:        fakeY.jaxis.value = amplify(-1); break;
		case SDL_HAT_DOWN:      fakeY.jaxis.value = amplify(+1); break;
		case SDL_HAT_RIGHTUP:   fakeX.jaxis.value = amplify(+1); fakeY.jaxis.value = amplify(-1); break;
		case SDL_HAT_RIGHT:     fakeX.jaxis.value = amplify(+1); break;
		case SDL_HAT_RIGHTDOWN: fakeX.jaxis.value = amplify(+1); fakeY.jaxis.value = amplify(+1); break;
		}
		FeedEvent(fakeX);
		FeedEvent(fakeY);
		return;
	}
	case SDL_JOYBALLMOTION:
	{
		SDL_Event fake;
		fake.jaxis.type = SDL_JOYAXISMOTION;
		fake.jaxis.which = event.jball.which;
		fake.jaxis.axis = event.jball.ball * 2 + 12; /* *magic*number* */
		fake.jaxis.value = amplify(event.jball.xrel);
		FeedEvent(event);
		fake.jaxis.axis += 1;
		fake.jaxis.value = amplify(event.jball.yrel);
		FeedEvent(event);
		return;
	}
	case SDL_JOYAXISMOTION:
	{
		C4KeyCode minCode = KEY_Gamepad(event.jaxis.which, KEY_JOY_Axis(event.jaxis.axis, false));
		C4KeyCode maxCode = KEY_Gamepad(event.jaxis.which, KEY_JOY_Axis(event.jaxis.axis, true));

		// FIXME: This assumes that the axis really rests around (0, 0) if it is not used, which is not always true.
		if (event.jaxis.value < -deadZone)
		{
			if (PressedAxis.count(minCode) == 0)
			{
				Game.DoKeyboardInput(
					KEY_Gamepad(event.jaxis.which, minCode),
					KEYEV_Down, false, false, false, false);
				PressedAxis.insert(minCode);
			}
		}
		else
		{
			if (PressedAxis.count(minCode) != 0)
			{
				Game.DoKeyboardInput(
					KEY_Gamepad(event.jaxis.which, minCode),
					KEYEV_Up, false, false, false, false);
				PressedAxis.erase(minCode);
			}
		}
		if (event.jaxis.value > +deadZone)
		{
			if (PressedAxis.count(maxCode) == 0)
			{
				Game.DoKeyboardInput(
					KEY_Gamepad(event.jaxis.which, maxCode),
					KEYEV_Down, false, false, false, false);
				PressedAxis.insert(maxCode);
			}
		}
		else
		{
			if (PressedAxis.count(maxCode) != 0)
			{
				Game.DoKeyboardInput(
					KEY_Gamepad(event.jaxis.which, maxCode),
					KEYEV_Up, false, false, false, false);
				PressedAxis.erase(maxCode);
			}
		}
		break;
	}
	case SDL_JOYBUTTONDOWN:
		Game.DoKeyboardInput(
			KEY_Gamepad(event.jbutton.which, KEY_JOY_Button(event.jbutton.button)),
			KEYEV_Down, false, false, false, false);
		break;
	case SDL_JOYBUTTONUP:
		Game.DoKeyboardInput(
			KEY_Gamepad(event.jbutton.which, KEY_JOY_Button(event.jbutton.button)),
			KEYEV_Up, false, false, false, false);
		break;
	}
}

int C4GamePadControl::GetGamePadCount()
{
	return (SDL_NumJoysticks());
}

C4GamePadOpener::C4GamePadOpener(int iGamepad)
{
	Joy = SDL_JoystickOpen(iGamepad);
	if (!Joy) LogNTr(spdlog::level::err, "SDL: {}", SDL_GetError());
}

C4GamePadOpener::~C4GamePadOpener()
{
	if (Joy) SDL_JoystickClose(Joy);
}

void C4GamePadOpener::SetGamePad(int iGamepad)
{
	if (Joy)
		SDL_JoystickClose(Joy);
	Joy = SDL_JoystickOpen(iGamepad);
	if (!Joy)
		LogNTr(spdlog::level::err, "SDL: {}", SDL_GetError());
}

#else

// Dedicated server and everything else with neither Win32 nor SDL.

C4GamePadControl::C4GamePadControl() { LogNTr(spdlog::level::warn, "Engine without Gamepad support"); }
C4GamePadControl::~C4GamePadControl() {}
void C4GamePadControl::Execute() {}
int C4GamePadControl::GetGamePadCount() { return 0; }

C4GamePadOpener::C4GamePadOpener(int iGamepad) {}
C4GamePadOpener::~C4GamePadOpener() {}
void C4GamePadOpener::SetGamePad(int iGamepad) {}

#endif // _WIN32
