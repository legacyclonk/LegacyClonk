/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
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

/* Main header to include all others */

#pragma once

#include "BuildConfig.h"

#ifdef _WIN64
#define C4_OS "win64"
#elif defined(_WIN32)
#define C4_OS "win32"
#elif defined(__linux__)
#ifdef __x86_64
#define C4_OS "linux64"
#else
#define C4_OS "linux"
#endif
#elif defined(__APPLE__)
#define C4_OS "mac"
#else
#define C4_OS "unknown";
#endif

#ifdef C4ENGINE

#ifndef HAVE_CONFIG_H
// different debugrec options
//#define DEBUGREC

// define directive STAT here to activate statistics
#undef STAT

#endif // HAVE_CONFIG_H

#ifdef DEBUGREC
#define DEBUGREC_SCRIPT
#define DEBUGREC_START_FRAME 0
#define DEBUGREC_PXS
#define DEBUGREC_OBJCOM
#define DEBUGREC_MATSCAN
//#define DEBUGREC_RECRUITMENT
#define DEBUGREC_MENU
#define DEBUGREC_OCF
#endif

// solidmask debugging
//#define SOLIDMASK_DEBUG

#ifdef _WIN32
// resources
#include "res/engine_resource.h"
#endif // _WIN32

#endif // C4ENGINE

#include <Standard.h>
#include <CStdFile.h>
#include <Fixed.h>
#include <StdAdaptors.h>
#include <StdBuf.h>
#include <StdConfig.h>
#include <StdCompiler.h>
#include <StdDDraw2.h>
#include <StdFacet.h>
#include <StdFile.h>
#include <StdFont.h>
#include <StdMarkup.h>
#include <StdPNG.h>
#include <StdResStr2.h>
#include <C4Surface.h>

#include "C4Id.h"
#include "C4Prototypes.h"
#include "C4Constants.h"

#ifdef _WIN32
#include <mmsystem.h>
#endif

#ifndef _WIN32
#include <unistd.h>
#endif

#include <time.h>
#include <map>
#include <vector>
#include <algorithm>
#include <assert.h>
#include <stdarg.h>
