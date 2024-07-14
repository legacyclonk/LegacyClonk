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

/* Move liquids in the landscape using individual transport spots */

#pragma once

#include "C4ForwardDeclarations.h"

#include <cstdint>

const int32_t C4MassMoverChunk = 10000;

class C4MassMoverSet;

class C4MassMover
{
	friend class C4MassMoverSet;

protected:
	int32_t Mat, x, y;

protected:
	void Cease(C4Section &section);
	bool Execute(C4Section &section);
	bool Init(C4Section &section, int32_t tx, int32_t ty);
	bool Corrosion(C4Section &section, int32_t dx, int32_t dy);
};

class C4MassMoverSet
{
public:
	C4MassMoverSet(C4Section &section);
	~C4MassMoverSet();

public:
	int32_t Count;
	int32_t CreatePtr;

protected:
	C4Section &section;
	C4MassMover Set[C4MassMoverChunk];

public:
	void Copy(C4MassMoverSet &rSet);
	void Synchronize();
	void Default();
	void Clear();
	void Execute();
	bool Create(int32_t x, int32_t y, bool fExecute = false);
	bool Load(C4Group &hGroup);
	bool Save(C4Group &hGroup);

protected:
	void Consolidate();
};
