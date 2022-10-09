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

#ifdef _WIN32
#include "C4Windows.h"
#include <timeapi.h>
#endif

#ifndef _WIN32

#define INFINITE 0xFFFFFFFF

unsigned long timeGetTime(void);

#endif

const char *GetCurrentTimeStamp(bool enableMarkupColor = true);
