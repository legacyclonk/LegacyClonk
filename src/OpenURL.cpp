/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
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

#include <Standard.h>

#ifdef _WIN32
#include "C4Windows.h"
#include <shellapi.h>
#else
#include <system_error>

#include "C4Log.h"
#include "C4PosixSpawn.h"
#endif

// open a weblink in an external browser
bool OpenURL(const char *szURL)
{
#ifdef _WIN32
	if (reinterpret_cast<intptr_t>(ShellExecute(nullptr, "open", szURL, nullptr, nullptr, SW_SHOW)) > 32)
		return true;
#else
	try
	{
#ifdef __APPLE__
		C4PosixSpawn::SpawnP({"open", szURL});
#else
		C4PosixSpawn::SpawnP({"xdg-open", szURL});
#endif
		return true;
	}
	catch (const std::system_error &e)
	{
		LogF("OpenURL failed: %s\n", e.what());
		return false;
	}
#endif
	// operating system not supported, or all opening method(s) failed
	return false;
}
