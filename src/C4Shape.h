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

/* Basic classes for rectangles and vertex outlines */

#pragma once

#include "C4FacetEx.h"
#include "C4Constants.h"

#include <vector>

#define C4D_VertexCpyPos (C4D_MaxVertex/2)

class C4Rect
{
public:
	int32_t x, y, Wdt, Hgt;

public:
	void Set(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt);
	void Default();
	bool Overlap(C4Rect &rTarget);
	void Intersect(const C4Rect &r2);
	void Add(const C4Rect &r2);
	bool operator==(const C4Rect &r2) { return !((x - r2.x) | (y - r2.y) | (Wdt - r2.Wdt) | (Hgt - r2.Hgt)); }
	bool operator!=(const C4Rect &r2) { return 0 != ((x - r2.x) | (y - r2.y) | (Wdt - r2.Wdt) | (Hgt - r2.Hgt)); }

	bool Contains(int32_t iX, int32_t iY)
	{
		return iX >= x && iX < x + Wdt && iY >= y && iY < y + Hgt;
	}

	bool Contains(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt)
	{
		return iX >= x && iX + iWdt < x + Wdt && iY >= y && iY + iHgt < y + Hgt;
	}

	bool Contains(const C4Rect &rect)
	{
		return Contains(rect.x, rect.y, rect.Wdt, rect.Hgt);
	}

	bool IntersectsLine(int32_t iX, int32_t iY, int32_t iX2, int32_t iY2);

	void Normalize()
	{
		if (Wdt < 0) { x += Wdt + 1; Wdt = -Wdt; } if (Hgt < 0) { y += Hgt + 1; Hgt = -Hgt; }
	}

	void Enlarge(int32_t iBy)
	{
		x -= iBy; y -= iBy; Wdt += 2 * iBy; Hgt += 2 * iBy;
	}

	int32_t GetMiddleX() { return x + Wdt / 2; }
	int32_t GetMiddleY() { return y + Hgt / 2; }
	int32_t GetBottom() { return y + Hgt; }

	C4Rect(int32_t tx, int32_t ty, int32_t twdt, int32_t thgt)
	{
		x = tx; y = ty; Wdt = twdt; Hgt = thgt;
	}

	C4Rect() {} // default ctor; doesn't initialize

	C4Rect(const FLOAT_RECT &rcfOuter) // set to surround floating point rectangle
	{
		x = static_cast<int32_t>(rcfOuter.left); y = static_cast<int32_t>(rcfOuter.top);
		Wdt = static_cast<int32_t>(ceilf(rcfOuter.right) - floorf(rcfOuter.left));
		Hgt = static_cast<int32_t>(ceilf(rcfOuter.bottom) - floorf(rcfOuter.top));
	}

	void CompileFunc(StdCompiler *pComp);
};

class C4TargetRect : public C4Rect
{
public:
	int32_t tx, ty;

public:
	C4TargetRect(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, int32_t iTX, int32_t iTY)
		: C4Rect(iX, iY, iWdt, iHgt), tx(iTX), ty(iTY) {}
	C4TargetRect() {} // default ctor; doesn't initialize
	void Set(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, int32_t iTX, int32_t iTY);
	void Default();
	bool ClipBy(C4TargetRect &rClip); // clip this rectangle by the given one (adding target positions) - return false if they don't overlap

	void CompileFunc(StdCompiler *pComp);
};

const C4Rect Rect0(0, 0, 0, 0);
const C4TargetRect TargetRect0(0, 0, 0, 0, 0, 0);

// a functional class to provide density for coordinates
class C4DensityProvider
{
public:
	virtual int32_t GetDensity(int32_t x, int32_t y) const;
	virtual ~C4DensityProvider() {}
};

extern C4DensityProvider DefaultDensityProvider;

class C4Shape : public C4Rect
{
public:
	// remember to adjust C4Shape::CopyFrom and CreateOwnOriginalCopy when adding members here!
	int32_t FireTop{};
	int32_t VtxNum{};
	int32_t VtxX[C4D_MaxVertex]{};
	int32_t VtxY[C4D_MaxVertex]{};
	int32_t VtxCNAT[C4D_MaxVertex]{};
	int32_t VtxFriction[C4D_MaxVertex]{};
	int32_t ContactDensity;
	int32_t ContactCNAT{};
	int32_t ContactCount{};
	int32_t AttachMat;
	int32_t VtxContactCNAT[C4D_MaxVertex]{};
	int32_t VtxContactMat[C4D_MaxVertex]{};
	int32_t iAttachX{}, iAttachY{}, iAttachVtx{};

public:
	C4Shape();
	void Default();
	void Rotate(int32_t iAngle, bool bUpdateVertices);
	void Stretch(int32_t iPercent, bool bUpdateVertices);
	void Jolt(int32_t iPercent, bool bUpdateVertices);
	void GetVertexOutline(C4Rect &rRect);
	int32_t GetVertexY(int32_t iVertex);
	int32_t GetVertexX(int32_t iVertex);
	bool AddVertex(int32_t iX, int32_t iY);
	bool CheckContact(int32_t cx, int32_t cy);
	bool ContactCheck(int32_t cx, int32_t cy);
	bool Attach(int32_t &cx, int32_t &cy, uint8_t cnat_pos);
	bool LineConnect(int32_t tx, int32_t ty, int32_t cvtx, int32_t ld, int32_t oldx, int32_t oldy);
	bool InsertVertex(int32_t iPos, int32_t tx, int32_t ty);
	bool RemoveVertex(int32_t iPos);
	void CopyFrom(C4Shape rFrom, bool bCpyVertices, bool fCopyVerticesFromSelf);
	int32_t GetBottomVertex();
	int32_t GetVertexContact(int32_t iVtx, uint32_t dwCheckMask, int32_t tx, int32_t ty, const C4DensityProvider &rDensityProvider = DefaultDensityProvider); // get CNAT-mask for given vertex - does not check range for iVtx!
	void CreateOwnOriginalCopy(C4Shape &rFrom); // create copy of all vertex members in back area of own buffers
	void CompileFunc(StdCompiler *pComp, bool fRuntime);
};

#ifdef C4ENGINE

// a bunch of rectangles
// rects NOT including pos+size-point
class C4RectList : public std::vector<C4Rect>
{
public:
	void AddRect(const C4Rect &rNewRect)
	{
		push_back(rNewRect);
	}

	void RemoveIndexedRect(int32_t idx)
	{
		if (idx < GetCount() - 1) Get(idx) = Get(GetCount() - 1); pop_back();
	}

	void Clear() { clear(); }
	int32_t GetCount() const { return size(); }
	C4Rect &Get(int32_t idx) { return (*this)[idx]; } // access w/o range check

	void ClipByRect(const C4Rect &rClip); // split up rectangles
};

#endif
