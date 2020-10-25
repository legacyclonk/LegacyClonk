/*
 * LegacyClonk
 *
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

#include "Standard.h"
#include "StdSdlSubSystem.h"

#include <SDL.h>

#include <stdexcept>
#include <string>

using namespace std::string_literals;

StdSdlSubSystem::StdSdlSubSystem(const Uint32 flags) : flags{flags}
{
	if (SDL_InitSubSystem(flags) != 0)
	{
		throw std::runtime_error{"SDL_InitSubSystem failed: "s + SDL_GetError()};
	}
}
