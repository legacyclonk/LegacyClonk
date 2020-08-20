/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
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

#include <Standard.h>

#ifdef C4ENGINE

#ifdef WITH_GLIB
#include <glib.h>
#endif

#ifdef WIN32
#include "C4Windows.h"
#include <shellapi.h>
#endif

#ifdef __APPLE__
#include <cstdlib>
#include <string>
#endif

// open a weblink in an external browser
bool OpenURL(const char *szURL)
{
#ifdef _WIN32
	if ((int)ShellExecute(nullptr, "open", szURL, nullptr, nullptr, SW_SHOW) > 32)
		return true;
#endif
#ifdef WITH_GLIB
	const char *argv[][3] =
	{
		{ "xdg-open",         szURL, nullptr },
		{ "sensible-browser", szURL, nullptr },
		{ "firefox",          szURL, nullptr },
		{ "mozilla",          szURL, nullptr },
		{ "konqueror",        szURL, nullptr },
		{ "epiphany",         szURL, nullptr },
		{ 0, 0, 0 }
	};
	for (int i = 0; argv[i][0]; ++i)
	{
		GError *error = 0;
		if (g_spawn_async(g_get_home_dir(), const_cast<gchar**>(argv[i]), 0, G_SPAWN_SEARCH_PATH, 0, 0, 0, &error))
			return true;
		else fprintf(stderr, "%s\n", error->message);
	}
#endif
#ifdef __APPLE__
	std::string command = std::string("open ") + '"' + szURL + '"';
	std::system(command.c_str());
	return true;
#endif
	// operating system not supported, or all opening method(s) failed
	return false;
}

#endif
