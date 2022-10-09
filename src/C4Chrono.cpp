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

#include "C4Chrono.h"

#include <ctime>

#ifndef _WIN32

#include <sys/time.h>

unsigned long timeGetTime(void)
{
	static time_t sec_offset;
	timeval tv;
	gettimeofday(&tv, nullptr);
	if (!sec_offset) sec_offset = tv.tv_sec;
	return (tv.tv_sec - sec_offset) * 1000 + tv.tv_usec / 1000;
}

#endif

const char *GetCurrentTimeStamp(bool enableMarkupColor)
{
	static char buf[25];

	time_t timenow;
	time(&timenow);

	strftime(buf, sizeof(buf), enableMarkupColor ? "<c 909090>[%H:%M:%S]</c>" : "[%H:%M:%S]", localtime(&timenow));

	return buf;
}
