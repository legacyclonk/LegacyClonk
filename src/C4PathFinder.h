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

/* Finds the way through the Clonk landscape */

#pragma once

#include "C4ForwardDeclarations.h"
#include <C4TransferZone.h>

class C4PathFinderRay
{
	friend class C4PathFinder;

public:
	C4PathFinderRay();
	~C4PathFinderRay();

public:
	void Clear();
	void Default();

protected:
	int32_t Status;
	int32_t X, Y, X2, Y2, TargetX, TargetY;
	int32_t CrawlStartX, CrawlStartY, CrawlAttach, CrawlLength, CrawlStartAttach;
	int32_t Direction, Depth;
	C4TransferZone *UseZone;
	C4PathFinderRay *From;
	C4PathFinderRay *Next;
	C4PathFinder *pPathFinder;

protected:
	void SetCompletePath();
	void TurnAttach(int32_t &rAttach, int32_t iDirection);
	void CrawlToAttach(int32_t &rX, int32_t &rY, int32_t iAttach);
	void CrawlByAttach(int32_t &rX, int32_t &rY, int32_t iAttach, int32_t iDirection);
	void Draw(C4FacetEx &cgo);
	int32_t FindCrawlAttachDiagonal(int32_t iX, int32_t iY, int32_t iDirection);
	int32_t FindCrawlAttach(int32_t iX, int32_t iY);
	bool IsCrawlAttach(int32_t iX, int32_t iY, int32_t iAttach);
	bool CheckBackRayShorten();
	bool Execute();
	bool CrawlTargetFree(int32_t iX, int32_t iY, int32_t iAttach, int32_t iDirection);
	bool PointFree(int32_t iX, int32_t iY);
	bool Crawl();
	bool PathFree(int32_t &rX, int32_t &rY, int32_t iToX, int32_t iToY, C4TransferZone **ppZone = nullptr);
};

class C4PathFinder
{
	friend class C4PathFinderRay;

public:
	C4PathFinder();
	~C4PathFinder();

protected:
	C4Landscape *Landscape;
	bool(*PointFree)(C4Landscape &, int32_t, int32_t);
	// iToX and iToY are intptr_t because there are stored object
	// pointers sometimes
	bool(*SetWaypoint)(int32_t, int32_t, intptr_t, intptr_t);
	C4PathFinderRay *FirstRay;
	intptr_t WaypointParameter;
	bool Success;
	C4TransferZones *TransferZones;
	bool TransferZonesEnabled;
	int Level;

public:
	void Draw(C4FacetEx &cgo);
	void Clear();
	void Default();
	void Init(C4Landscape &landscape, bool(*fnPointFree)(C4Landscape &, int32_t, int32_t), C4TransferZones *pTransferZones = nullptr);
	bool Find(int32_t iFromX, int32_t iFromY, int32_t iToX, int32_t iToY, bool(*fnSetWaypoint)(int32_t, int32_t, intptr_t, intptr_t), intptr_t iWaypointParameter);
	void EnableTransferZones(bool fEnabled);
	void SetLevel(int iLevel);

protected:
	void Run();
	bool AddRay(int32_t iFromX, int32_t iFromY, int32_t iToX, int32_t iToY, int32_t iDepth, int32_t iDirection, C4PathFinderRay *pFrom, C4TransferZone *pUseZone = nullptr);
	bool SplitRay(C4PathFinderRay *pRay, int32_t iAtX, int32_t iAtY);
	bool Execute();
};
