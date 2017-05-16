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

/* Screen area marked for mouse interaction */

#pragma once

#include <C4Id.h>

const int C4RGN_MaxCaption = 256;

class C4Region
{
	friend class C4RegionList;

public:
	C4Region();
	~C4Region();

public:
	int X, Y, Wdt, Hgt;
	char Caption[C4RGN_MaxCaption + 1];
	int Com, RightCom, MoveOverCom, HoldCom;
	int Data;
	C4ID id;
	C4Object *Target;

protected:
	C4Region *Next;

public:
	void Set(C4Facet &fctArea, const char *szCaption = nullptr, C4Object *pTarget = nullptr);
	void Clear();
	void Default();
	void Set(int iX, int iY, int iWdt, int iHgt, const char *szCaption, int iCom, int iMoveOverCom, int iHoldCom, int iData, C4Object *pTarget);

protected:
	void ClearPointers(C4Object *pObj);
};

class C4RegionList
{
public:
	C4RegionList();
	~C4RegionList();

protected:
	int AdjustX, AdjustY;
	C4Region *First;

public:
	void ClearPointers(C4Object *pObj);
	void SetAdjust(int iX, int iY);
	void Clear();
	void Default();
	C4Region *Find(int iX, int iY);
	bool Add(int iX, int iY, int iWdt, int iHgt, const char *szCaption = nullptr, int iCom = COM_None, C4Object *pTarget = nullptr, int iMoveOverCom = COM_None, int iHoldCom = COM_None, int iData = 0);
	bool Add(C4Region &rRegion);
};
