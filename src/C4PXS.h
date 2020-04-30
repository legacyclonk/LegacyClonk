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

/* Pixel Sprite system for tiny bits of moving material */

#pragma once

#include <C4Material.h>

class C4PXS
{
	C4PXS() : Mat(MNone), x(Fix0), y(Fix0), xdir(Fix0), ydir(Fix0) {}

	friend class C4PXSSystem;

protected:
	int32_t Mat;
	FIXED x, y, xdir, ydir;

protected:
	void Execute();
	void Deactivate();
};

const size_t PXSChunkSize = 500, PXSMaxChunk = 20;

class C4PXSSystem
{
public:
	C4PXSSystem();
	~C4PXSSystem();

public:
	int32_t Count;

protected:
	C4PXS *Chunk[PXSMaxChunk];
	size_t iChunkPXS[PXSMaxChunk];

public:
	void Delete(C4PXS *pPXS);
	void Default();
	void Clear();
	void Execute();
	void Draw(C4FacetEx &cgo);
	void Synchronize();
	void SyncClearance();
	void Cast(int32_t mat, int32_t num, int32_t tx, int32_t ty, int32_t level);
	bool Create(int32_t mat, FIXED ix, FIXED iy, FIXED ixdir = Fix0, FIXED iydir = Fix0);
	bool Load(CppC4Group &group);
	bool Save(CppC4Group &group);

protected:
	C4PXS *New();
};
