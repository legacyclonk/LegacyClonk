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

/* A facet that can hold its own surface and also target coordinates */

#pragma once

#include <C4Facet.h>
#include <C4Surface.h>

const int C4FCT_Full   = -1,
          C4FCT_Height = -2,
          C4FCT_Width  = -3;

class C4FacetEx : public C4Facet
{
public:
	C4FacetEx() { Default(); }
	~C4FacetEx() {}

public:
	int TargetX, TargetY;

public:
	void Default() { TargetX = TargetY = 0; C4Facet::Default(); }
	void Clear() { Surface = nullptr; }

	void Set(const C4Facet &cpy) { TargetX = TargetY = 0; C4Facet::Set(cpy); }
	void Set(const C4FacetEx &cpy) { *this = cpy; }
	void Set(C4Surface *nsfc, int nx, int ny, int nwdt, int nhgt, int ntx = 0, int nty = 0);

	void DrawBolt(int iX1, int iY1, int iX2, int iY2, uint8_t bCol, uint8_t bCol2);
	void DrawLine(int iX1, int iY1, int iX2, int iY2, uint8_t bCol1, uint8_t bCol2);
	C4FacetEx GetSection(int iSection);
	C4FacetEx GetPhase(int iPhaseX = 0, int iPhaseY = 0);

public:
	C4FacetEx &operator=(const C4Facet &rhs)
	{
		Set(rhs.Surface, rhs.X, rhs.Y, rhs.Wdt, rhs.Hgt);
		return *this;
	}
};

// facet that can hold its own surface
class C4FacetExSurface : public C4FacetEx
{
private:
	C4Surface Face;

private:
	C4FacetExSurface(const C4FacetExSurface &rCpy); // NO copy!
	C4FacetExSurface &operator=(const C4FacetExSurface &rCpy); // NO copy assignment!

public:
	C4FacetExSurface() { Default(); }
	~C4FacetExSurface() { Clear(); }

	void Default() { Face.Default(); C4FacetEx::Default(); }
	void Clear() { Face.Clear(); C4FacetEx::Clear(); }

	void Set(const C4Facet &cpy) { Clear(); C4Facet::Set(cpy); }
	void Set(const C4FacetEx &cpy) { Clear(); C4FacetEx::Set(cpy); }

	void Set(C4Surface *nsfc, int nx, int ny, int nwdt, int nhgt, int ntx = 0, int nty = 0)
	{
		C4FacetEx::Set(nsfc, nx, ny, nwdt, nhgt, ntx, nty);
	}

	void Grayscale(int32_t iOffset = 0);
	bool Create(int iWdt, int iHgt, int iWdt2 = C4FCT_Full, int iHgt2 = C4FCT_Full);
	C4Surface &GetFace() { return Face; } // get internal face
	bool CreateClrByOwner(C4Surface *pBySurface);
	bool EnsureSize(int iMinWdt, int iMinHgt);
	bool Load(C4Group &hGroup, const char *szName, int iWdt = C4FCT_Full, int iHgt = C4FCT_Full, bool fOwnPal = false, bool fNoErrIfNotFound = false);

	void GrabFrom(C4FacetExSurface &rSource)
	{
		Clear(); Default();
		Face.MoveFrom(&rSource.Face);
		Set(rSource.Surface == &rSource.Face ? &Face : rSource.Surface, rSource.X, rSource.Y, rSource.Wdt, rSource.Hgt, rSource.TargetX, rSource.TargetY);
		rSource.Default();
	}

	bool CopyFromSfcMaxSize(C4Surface &srcSfc, int32_t iMaxSize, uint32_t dwColor = 0u);
};

// facet with source group ID; used to avoid doubled loading from same group
class C4FacetExID : public C4FacetExSurface
{
public:
	int32_t idSourceGroup;

	C4FacetExID() : C4FacetExSurface(), idSourceGroup(0) {}

	void Default() { C4FacetExSurface::Default(); idSourceGroup = 0; } // default to std values
	void Clear() { C4FacetExSurface::Clear(); idSourceGroup = 0; } // clear all data in class
};
