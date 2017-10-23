/*
 * LegacyClonk
 *
 * Copyright (c) 2017, The LegacyClonk Team and contributors
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

// RAII wrapper class for SDL_InitSubSystem/SDL_QuitSubSystem.

#pragma once

#include <SDL.h>

class StdSdlSubSystem
{
public:
	StdSdlSubSystem(Uint32 flags);
	StdSdlSubSystem(const StdSdlSubSystem &) = delete;
	StdSdlSubSystem(StdSdlSubSystem &&o) noexcept { o.flags = 0; }
	~StdSdlSubSystem() { if (flags != 0) SDL_QuitSubSystem(flags); }
	StdSdlSubSystem &operator=(const StdSdlSubSystem &) = delete;

private:
	Uint32 flags;
};
