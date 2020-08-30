/*
 * LegacyClonk
 *
 * Copyright (c) 2019, The LegacyClonk Team and contributors
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

#if defined(_WIN32) && !defined(_INC_WINDOWS)

#include <sdkddkver.h>

#undef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WIN7

#undef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WIN7

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#endif
