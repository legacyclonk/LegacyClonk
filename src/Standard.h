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

/* All kinds of valuable helpers */

#pragma once

#include "C4Breakpoint.h"
#include "C4Chrono.h"
#include "C4Math.h"
#include "C4Strings.h"
#include "StdColors.h"
#include "StdFile.h"
#include "StdHelpers.h"

// These functions have to be provided by the application.
bool Log(const char *szMessage);
bool LogSilent(const char *szMessage);

#include <algorithm>
