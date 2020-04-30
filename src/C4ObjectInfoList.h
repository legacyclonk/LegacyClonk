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

/* Dynamic list for crew member info */

#pragma once

#include <C4Id.h>

#include "cppc4group.hpp"

class C4ObjectInfoList
{
public:
	C4ObjectInfoList();
	~C4ObjectInfoList();

protected:
	class C4ObjectInfo *First;

public:
	int32_t iNumCreated; // number of new defs created during this round

public:
	void Default();
	void Clear();
	void Evaluate();
	void DetachFromObjects();
	int32_t Load(CppC4Group &group, bool fLoadPortraits);
	bool Add(C4ObjectInfo *pInfo);
	bool Save(CppC4Group &group, bool fSavegame, bool fStoreTiny, C4DefList *pDefs);
	C4ObjectInfo *New(C4ID n_id, class C4DefList *pDefs, const char *cpNames);
	C4ObjectInfo *GetIdle(C4ID c_id, C4DefList &rDefs);
	C4ObjectInfo *GetIdle(const char *szByName);
	C4ObjectInfo *GetFirst() { return First; }
	bool IsElement(C4ObjectInfo *pInfo);
	void Strip(C4DefList &rDefs);

public:
	void MakeValidName(char *sName);
	bool NameExists(const char *szName);

protected:
	C4ObjectInfo *GetLast();
	C4ObjectInfo *GetPrevious(C4ObjectInfo *pInfo);
	void Load(CppC4Group &group, const bool &fLoadPortraits, size_t &infoCount, const CppC4Group::EntryInfo &info);
};
