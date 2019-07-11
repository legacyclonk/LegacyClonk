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

#include <C4Include.h>
#include <C4Region.h>

#include <C4Facet.h>

C4Region::C4Region()
{
	Default();
}

C4Region::~C4Region()
{
	Clear();
}

void C4Region::Default()
{
	X = Y = Wdt = Hgt = 0;
	Caption[0] = 0;
	Com = RightCom = MoveOverCom = HoldCom = COM_None;
	Data = 0;
	id = C4ID_None;
	Target = nullptr;
}

void C4Region::Clear() {}

C4RegionList::C4RegionList()
{
	Default();
}

C4RegionList::~C4RegionList()
{
	Clear();
}

void C4RegionList::Default()
{
	First = nullptr;
	AdjustX = AdjustY = 0;
}

void C4RegionList::Clear()
{
	C4Region *pRgn, *pNext;
	for (pRgn = First; pRgn; pRgn = pNext) { pNext = pRgn->Next; delete pRgn; }
	First = nullptr;
}

bool C4RegionList::Add(int iX, int iY, int iWdt, int iHgt, const char *szCaption,
	int iCom, C4Object *pTarget, int iMoveOverCom, int iHoldCom, int iData)
{
	C4Region *pRgn = new C4Region;
	pRgn->Set(iX + AdjustX, iY + AdjustY, iWdt, iHgt, szCaption, iCom, iMoveOverCom, iHoldCom, iData, pTarget);
	pRgn->Next = First;
	First = pRgn;
	return true;
}

bool C4RegionList::Add(C4Region &rRegion)
{
	C4Region *pRgn = new C4Region;
	*pRgn = rRegion;
	pRgn->X += AdjustX;
	pRgn->Y += AdjustY;
	pRgn->Next = First;
	First = pRgn;
	return true;
}

void C4Region::Set(int iX, int iY, int iWdt, int iHgt, const char *szCaption,
	int iCom, int iMoveOverCom, int iHoldCom, int iData,
	C4Object *pTarget)
{
	X = iX; Y = iY; Wdt = iWdt; Hgt = iHgt;
	SCopy(szCaption, Caption, C4RGN_MaxCaption);
	Com = iCom; MoveOverCom = iMoveOverCom; HoldCom = iHoldCom;
	Data = iData;
	Target = pTarget;
}

void C4RegionList::SetAdjust(int iX, int iY)
{
	AdjustX = iX; AdjustY = iY;
}

C4Region *C4RegionList::Find(int iX, int iY)
{
	for (C4Region *pRgn = First; pRgn; pRgn = pRgn->Next)
		if (Inside(iX - pRgn->X, 0, pRgn->Wdt - 1))
			if (Inside(iY - pRgn->Y, 0, pRgn->Hgt - 1))
				return pRgn;
	return nullptr;
}

void C4Region::ClearPointers(C4Object *pObj)
{
	if (Target == pObj) Target = nullptr;
}

void C4RegionList::ClearPointers(C4Object *pObj)
{
	for (C4Region *pRgn = First; pRgn; pRgn = pRgn->Next)
		pRgn->ClearPointers(pObj);
}

void C4Region::Set(C4Facet &fctArea, const char *szCaption, C4Object *pTarget)
{
	X = fctArea.X;
	Y = fctArea.Y;
	Wdt = fctArea.Wdt;
	Hgt = fctArea.Hgt;
	if (szCaption) SCopy(szCaption, Caption, C4RGN_MaxCaption);
	if (pTarget) Target = pTarget;
}
