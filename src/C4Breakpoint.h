/*
 * LegacyClonk
 *
 * Copyright (c) 2022, The LegacyClonk Team and contributors
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

#if defined(NDEBUG)

#define BREAKPOINT_HERE

#elif defined(_WIN32)

#include <intrin.h>

#define BREAKPOINT_HERE __debugbreak()

#elif __has_builtin(__builtin_debugtrap)

#define BREAKPOINT_HERE __builtin_debugtrap()

#else

#include <csignal>

#define BREAKPOINT_HERE std::raise(SIGTRAP)

#endif
