/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2020, The LegacyClonk Team and contributors
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

/* Basic classes for rectangles */

#pragma once

#include "C4ForwardDeclarations.h"

#include <cstddef>
#include <cstdint>
#include <vector>

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
	bool operator==(const C4Rect &r2) const { return !((x - r2.x) | (y - r2.y) | (Wdt - r2.Wdt) | (Hgt - r2.Hgt)); }

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

	constexpr C4Rect(int32_t tx, int32_t ty, int32_t twdt, int32_t thgt)
	{
		x = tx; y = ty; Wdt = twdt; Hgt = thgt;
	}

	constexpr C4Rect() = default; // default ctor; doesn't initialize

	C4Rect Scaled(float scale) const noexcept;

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

// a bunch of rectangles
// rects NOT including pos+size-point
class C4RectList : public std::vector<C4Rect>
{
public:
	void AddRect(const C4Rect &rNewRect)
	{
		push_back(rNewRect);
	}

	void RemoveIndexedRect(size_t idx)
	{
		if (idx < GetCount() - 1) Get(idx) = Get(GetCount() - 1); pop_back();
	}

	void Clear() { clear(); }
	size_t GetCount() const { return size(); }
	C4Rect &Get(size_t idx) { return (*this)[idx]; } // access w/o range check

	void ClipByRect(const C4Rect &rClip); // split up rectangles
};
