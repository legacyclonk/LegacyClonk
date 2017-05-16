/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Another C4Group bitmap-to-surface loader and saver */

#include <C4Include.h>
#include <C4SurfaceFile.h>

#ifndef BIG_C4INCLUDE
#include <C4Surface.h>
#include <C4Group.h>
#endif

C4Surface *GroupReadSurface(CStdStream &hGroup, BYTE *bpPalette)
	{
	// create surface
	C4Surface *pSfc=new C4Surface();
	if (!pSfc->Read(hGroup, !!bpPalette))
		{ delete pSfc; return NULL; }
	return pSfc;
	}

CSurface8 *GroupReadSurface8(CStdStream &hGroup)
	{
	// create surface
	CSurface8 *pSfc=new CSurface8();
	if (!pSfc->Read(hGroup, false))
		{ delete pSfc; return NULL; }
	return pSfc;
	}

CSurface8 *GroupReadSurfaceOwnPal8(CStdStream &hGroup)
	{
	// create surface
	CSurface8 *pSfc=new CSurface8();
	if (!pSfc->Read(hGroup, true))
		{ delete pSfc; return NULL; }
	return pSfc;
	}

C4Surface *GroupReadSurfacePNG(CStdStream &hGroup)
	{
	// create surface
	C4Surface *pSfc=new C4Surface();
	pSfc->ReadPNG(hGroup);
	return pSfc;
	}
