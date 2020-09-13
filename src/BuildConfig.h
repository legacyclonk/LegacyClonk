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

#pragma once

#ifdef HAVE_CONFIG_H
#include <config.h>
#elif defined(_WIN32)
#define HAVE_IO_H 1
#define HAVE_DIRECT_H 1
#define HAVE_SHARE_H 1
#define HAVE_FREETYPE
#endif
