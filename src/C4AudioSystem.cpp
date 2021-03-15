/*
 * LegacyClonk
 *
 * Copyright (c) 2020, The LegacyClonk Team and contributors
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

#include "C4AudioSystem.h"

#include "BuildConfig.h"
#include "C4AudioSystemNone.h"
#include "C4Log.h"

#ifdef USE_FMOD
#include "C4AudioSystemFmodRuntime.h"
#endif

#ifdef USE_SDL_MIXER
#include "C4AudioSystemSdl.h"
#endif

C4AudioSystem *C4AudioSystem::NewInstance(int maxChannels)
{
#ifdef USE_FMOD
	try
	{
		const auto system = CreateC4AudioSystemFmodRuntime(maxChannels);
		Log("Using dynamically loaded FMod for sound.");
		return system;
	}
	catch (const std::runtime_error &e)
	{
		LogSilentF("Loading FMod sound system failed: %s", e.what());
	}
#endif
#ifdef USE_SDL_MIXER
	try
	{
		return CreateC4AudioSystemSdl(maxChannels);
	}
	catch (const std::runtime_error &e)
	{
		Log(e.what());
	}
#endif
	return new C4AudioSystemNone{};
}
