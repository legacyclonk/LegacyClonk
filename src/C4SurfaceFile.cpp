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

/* Another C4Group bitmap-to-surface loader and saver */

#include <C4Include.h>
#include <C4SurfaceFile.h>

#include <C4Surface.h>
#include <C4Group.h>

C4Surface *GroupReadSurface(CppC4Group &group, const std::string &path, uint8_t *bpPalette)
{
	// create surface
	C4Surface *pSfc = new C4Surface();
	if (!pSfc->Read(group, path, !!bpPalette))
	{
		delete pSfc; return nullptr;
	}
	return pSfc;
}

CSurface8 *GroupReadSurface8(CppC4Group &group, const std::string &path)
{
	// create surface
	CSurface8 *pSfc = new CSurface8();
	if (!pSfc->Read(group, path, false))
	{
		delete pSfc; return nullptr;
	}
	return pSfc;
}

CSurface8 *GroupReadSurfaceOwnPal8(CppC4Group &group, const std::string &path)
{
	// create surface
	CSurface8 *pSfc = new CSurface8();
	if (!pSfc->Read(group, path, true))
	{
		delete pSfc; return nullptr;
	}
	return pSfc;
}

C4Surface *GroupReadSurfacePNG(CppC4Group &group, const std::string &path)
{
	// create surface
	C4Surface *pSfc = new C4Surface();
	pSfc->ReadPNG(group, path);
	return pSfc;
}
