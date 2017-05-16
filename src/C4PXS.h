/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Pixel Sprite system for tiny bits of moving material */

#ifndef INC_C4PXS
#define INC_C4PXS

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
	BOOL Create(int32_t mat, FIXED ix, FIXED iy, FIXED ixdir = Fix0, FIXED iydir = Fix0);
	BOOL Load(C4Group &hGroup);
	BOOL Save(C4Group &hGroup);

protected:
	C4PXS *New();
};

#endif
