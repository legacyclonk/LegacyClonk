/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2017-2022, The LegacyClonk Team and contributors
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

#define C4ENGINENAME "LegacyClonk"

#define FANPROJECTTEXT "LegacyClonk is a fan project based on Clonk Rage."
#define TRADEMARKTEXT "'Clonk' is a registered trademark of Matthes Bender."

#define C4ENGINECAPTION "LegacyClonk"
#define C4EDITORCAPTION "Clonk Editor"

/* These values are now controlled by the file source/version - DO NOT MODIFY DIRECTLY */
#define C4XVER1 4
#define C4XVER2 9
#define C4XVER3 10
#define C4XVER4 15
#define C4XVERBUILD 357
#define C4VERSIONEXTRA ""
/* These values are now controlled by the file source/version - DO NOT MODIFY DIRECTLY */

// Build Options
#ifndef NDEBUG
#define C4BUILDDEBUG " DEBUG"
#else
#define C4BUILDDEBUG
#endif

#define C4BUILDOPT C4BUILDDEBUG

#define C4ENGINEINFO C4ENGINENAME " " C4VERSIONEXTRA
#ifdef C4VERSIONBUILDNAME
#define C4ENGINEINFOLONG C4ENGINENAME " " C4VERSIONEXTRA " (" C4VERSIONBUILDNAME ")"
#else
#define C4ENGINEINFOLONG C4ENGINEINFO
#endif

#define C4XVERTOC4XVERS(s) C4XVERTOC4XVERS2(s)
#define C4XVERTOC4XVERS2(s) #s
#if C4XVERBUILD <= 99
#define C4VERSION C4XVERTOC4XVERS(C4XVER1) "." C4XVERTOC4XVERS(C4XVER2) "." C4XVERTOC4XVERS(C4XVER3) "." C4XVERTOC4XVERS(C4XVER4) " [0" C4XVERTOC4XVERS(C4XVERBUILD) "]" C4VERSIONEXTRA " " C4BUILDOPT
#else
#define C4VERSION C4XVERTOC4XVERS(C4XVER1) "." C4XVERTOC4XVERS(C4XVER2) "." C4XVERTOC4XVERS(C4XVER3) "." C4XVERTOC4XVERS(C4XVER4) " [" C4XVERTOC4XVERS(C4XVERBUILD) "]" C4VERSIONEXTRA " " C4BUILDOPT
#endif

/* entries for engine.rc (VC++ will overwrite them)

#include "..\inc\C4Version.h"
[...]
 FILEVERSION C4XVER1,C4XVER2,C4XVER3,C4XVER4
 PRODUCTVERSION C4XVER1,C4XVER2,C4XVER3,C4XVER4
[...]
			VALUE "FileDescription", C4ENGINECAPTION "\0"
			VALUE "FileVersion", C4VERSION "\0"
			VALUE "SpecialBuild", C4BUILDOPT "\0"
			VALUE "ProductVersion", C4VERSION "\0"
*/
