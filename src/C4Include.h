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

/* Main header to include all others */

#pragma once

#ifdef C4ENGINE

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
