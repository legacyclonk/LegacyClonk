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
#include <StdNoGfx.h>

BOOL CStdNoGfx::CreateDirectDraw()
	{
	Log("  Graphics disabled");
	return TRUE;
	}

CStdNoGfx::CStdNoGfx()
	{
	Default();
	}

CStdNoGfx::~CStdNoGfx()
	{
	delete lpPrimary; lpPrimary = NULL;
	Clear();
	}

bool CStdNoGfx::CreatePrimarySurfaces(BOOL Fullscreen, int iColorDepth, unsigned int iMonitor)
	{
	// Save back color depth
	byByteCnt = iColorDepth / 8;
	// Create dummy surface
	lpPrimary = lpBack = new CSurface();
	return true;
	}
