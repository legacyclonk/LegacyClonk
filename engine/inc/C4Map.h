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
    int32_t MapWdt,MapHgt;
    int32_t Exclusive;
  public:
    void Create(CSurface8 *sfcMap,
                C4SLandscape &rLScape, C4TextureMap &rTexMap,
                BOOL fLayers=FALSE, int32_t iPlayerNum=1);
		BOOL Load(BYTE **pbypBuffer, 
							int32_t &rBufWdt, int32_t &rMapWdt, int32_t &rMapHgt,
							C4Group &hGroup, const char *szEntryName,
							C4TextureMap &rTexMap);
  protected:
    void Reset();
    void SetPix(int32_t x, int32_t y, BYTE col);
    void SetSpot(int32_t x, int32_t y, int32_t rad, BYTE col);
    void DrawLayer(int32_t x, int32_t y, int32_t size, BYTE col);
		void ValidateTextureIndices(C4TextureMap &rTexMap);
    BYTE GetPix(int32_t x, int32_t y);
  };

#endif
