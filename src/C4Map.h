/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Create map from dynamic landscape data in scenario */

#ifndef INC_C4Map
#define INC_C4Map

#include <C4Scenario.h>

class C4MapCreator
{
public:
	C4MapCreator();

protected:
	int32_t MapIFT;
	CSurface8 *MapBuf;
	int32_t MapWdt, MapHgt;
	int32_t Exclusive;

public:
	void Create(CSurface8 *sfcMap,
		C4SLandscape &rLScape, C4TextureMap &rTexMap,
		BOOL fLayers = FALSE, int32_t iPlayerNum = 1);

protected:
	void Reset();
	void SetPix(int32_t x, int32_t y, BYTE col);
	void DrawLayer(int32_t x, int32_t y, int32_t size, BYTE col);
	BYTE GetPix(int32_t x, int32_t y);
};

#endif
