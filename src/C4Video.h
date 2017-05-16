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

/* Captures uncompressed AVI from game view */

#pragma once

#include <C4Surface.h>

// avoid pulling in vfw.h in every file
struct IAVIFile;
struct IAVIStream;

class C4Video
{
public:
	C4Video();
	~C4Video();

protected:
	bool Active;
	IAVIFile   *pAviFile;
	IAVIStream *pAviStream;
	int AviFrame;
	double AspectRatio;
	int X, Y, Width, Height;
	uint8_t *Buffer;
	int BufferSize;
	int InfoSize;
	bool Recording;
	SURFACE Surface;
	int ShowFlash;

public:
	void Draw();
	void Draw(C4FacetEx &cgo);
	void Resize(int iChange);
	bool Start(const char *szFilename);
	void Default();
	void Init(SURFACE sfcSource, int iWidth = 768, int iHeight = 576);
	void Clear();
	bool Start();
	bool Stop();
	bool Toggle();
	void Execute();
	bool Reduce();
	bool Enlarge();

protected:
	bool RecordFrame();
	bool AdjustPosition();
};
