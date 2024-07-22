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

#include <C4SurfaceFile.h>

#include <C4Surface.h>
#include <C4Group.h>
#include "StdSurface8.h"

C4Surface *GroupReadSurface(CStdStream &hGroup, uint8_t *bpPalette)
{
	// create surface
	C4Surface *pSfc = new C4Surface();
	if (!pSfc->Read(hGroup, !!bpPalette))
	{
		delete pSfc; return nullptr;
	}
	return pSfc;
}

std::unique_ptr<CSurface8> GroupReadSurface8(CStdStream &hGroup)
{
	// create surface
	auto sfc = std::make_unique<CSurface8>();
	if (!sfc->Read(hGroup, false))
	{
		return nullptr;
	}

	return std::move(sfc);
}

std::unique_ptr<CSurface8> GroupReadSurfaceOwnPal8(CStdStream &hGroup)
{
	// create surface
	auto sfc = std::make_unique<CSurface8>();
	if (!sfc->Read(hGroup, true))
	{
		return nullptr;
	}

	return std::move(sfc);
}

std::unique_ptr<C4Surface> GroupReadSurfacePNG(CStdStream &hGroup)
{
	// create surface
	auto sfc = std::make_unique<C4Surface>();
	if (!sfc->ReadPNG(hGroup))
	{
		return nullptr;
	}

	return std::move(sfc);
}
