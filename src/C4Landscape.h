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

/* Handles landscape and sky */

#pragma once

#include "C4Sky.h"
#include "C4Shape.h"

#include <StdSurface8.h>

const uint8_t GBM        = 128,
              GBM_ColNum = 64,
              IFT        = 0x80,
              IFTOld     = GBM_ColNum;

const uint8_t CSkyDef1 = 104;

const int32_t C4MaxMaterial = 125;

const int32_t C4LSC_Undefined = 0,
              C4LSC_Dynamic = 1,
              C4LSC_Static = 2,
              C4LSC_Exact = 3;

const int32_t C4LS_MaxRelights = 50;

class C4MapCreatorS2;
class C4Object;

class C4Landscape
{
public:
	C4Landscape();
	~C4Landscape();

public:
	int32_t Mode;
	int32_t Width, Height;
	int32_t MapWidth, MapHeight, MapZoom;
	CSurface8 *Map;
	uint32_t MatCount[C4MaxMaterial]; // NoSave //
	uint32_t EffectiveMatCount[C4MaxMaterial]; // NoSave //
	int32_t BlastMatCount[C4MaxMaterial]; // SyncClearance-NoSave //
	bool NoScan; // ExecuteScan() disabled
	int32_t ScanX, ScanSpeed; // SyncClearance-NoSave //
	int32_t LeftOpen, RightOpen;
	bool TopOpen, BottomOpen;
	FIXED Gravity;
	uint32_t Modulation; // landscape blit modulation; 0 means normal
	int32_t MapSeed; // random seed for MapToLandscape
	C4Sky Sky;
	C4MapCreatorS2 *pMapCreator; // map creator for script-generated maps
	bool fMapChanged;
	uint8_t *pInitial; // Initial landscape after creation - used for diff

protected:
	CSurface *Surface32;
	CSurface *AnimationSurface;
	CSurface8 *Surface8;
	int32_t Pix2Mat[256], Pix2Dens[256], Pix2Place[256];
	int32_t PixCntPitch;
	uint8_t *PixCnt;
	C4Rect Relights[C4LS_MaxRelights];

public:
	void Default();
	void Clear(bool fClearMapCreator = true, bool fClearSky = true);
	void Execute();
	void Synchronize();
	void Draw(C4FacetEx &cgo, int32_t iPlayer = -1);
	void ScenarioInit();
	void ClearRect(int32_t iTx, int32_t iTy, int32_t iWdt, int32_t iHgt);
	void ClearRectDensity(int32_t iTx, int32_t iTy, int32_t iWdt, int32_t iHgt, int32_t iOfDensity);
	void ClearMatCount();
	void ClearBlastMatCount();
	void ScanSideOpen();
	void CheckInstabilityRange(int32_t tx, int32_t ty);
	void ShakeFree(int32_t tx, int32_t ty, int32_t rad);
	void DigFree(int32_t tx, int32_t ty, int32_t rad, bool fRequest = false, C4Object *pByObj = nullptr);
	void DigFreeRect(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt, bool fRequest = false, C4Object *pByObj = nullptr);
	void DigFreeMat(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt, int32_t mat);
	void BlastFree(int32_t tx, int32_t ty, int32_t rad, int32_t grade, int32_t iByPlayer);
	void DrawMaterialRect(int32_t mat, int32_t tx, int32_t ty, int32_t wdt, int32_t hgt);
	void RaiseTerrain(int32_t tx, int32_t ty, int32_t wdt);
	void FindMatTop(int32_t mat, int32_t &x, int32_t &y);
	uint8_t GetMapIndex(int32_t iX, int32_t iY);
	bool Load(C4Group &hGroup, bool fLoadSky, bool fSavegame);
	bool Save(C4Group &hGroup);
	bool SaveDiff(C4Group &hGroup, bool fSyncSave);
	bool SaveMap(C4Group &hGroup);
	bool SaveInitial();
	bool SaveTextures(C4Group &hGroup);
	bool Init(C4Group &hGroup, bool fOverloadCurrent, bool fLoadSky, bool &rfLoaded, bool fSavegame);
	bool MapToLandscape();
	bool ApplyDiff(C4Group &hGroup);
	bool SetMode(int32_t iMode);
	bool SetPix(int32_t x, int32_t y, uint8_t npix); // set landscape pixel (bounds checked)
	bool SetPixDw(int32_t x, int32_t y, uint32_t dwPix); // set pixel how it is visible only
	bool _SetPix(int32_t x, int32_t y, uint8_t npix); // set landsape pixel (bounds not checked)
	bool _SetPixIfMask(int32_t x, int32_t y, uint8_t npix, uint8_t nMask); // set landscape pixel, if it matches nMask color (no bound-checks)
	bool CheckInstability(int32_t tx, int32_t ty);
	bool ClearPix(int32_t tx, int32_t ty);
	bool InsertMaterial(int32_t mat, int32_t tx, int32_t ty, int32_t vx = 0, int32_t vy = 0);
	bool FindMatPath(int32_t &fx, int32_t &fy, int32_t ydir, int32_t mdens, int32_t mslide);
	bool FindMatSlide(int32_t &fx, int32_t &fy, int32_t ydir, int32_t mdens, int32_t mslide);
	bool FindMatPathPush(int32_t &fx, int32_t &fy, int32_t mdens, int32_t mslide, bool liquid);
	bool Incinerate(int32_t x, int32_t y);
	bool DrawBrush(int32_t iX, int32_t iY, int32_t iGrade, const char *szMaterial, const char *szTexture, bool fIFT);
	bool DrawLine(int32_t iX1, int32_t iY1, int32_t iX2, int32_t iY2, int32_t iGrade, const char *szMaterial, const char *szTexture, bool fIFT);
	bool DrawBox(int32_t iX1, int32_t iY1, int32_t iX2, int32_t iY2, int32_t iGrade, const char *szMaterial, const char *szTexture, bool fIFT);
	bool DrawChunks(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt, int32_t icntx, int32_t icnty, const char *szMaterial, const char *szTexture, bool bIFT);
	bool DrawQuad(int32_t iX1, int32_t iY1, int32_t iX2, int32_t iY2, int32_t iX3, int32_t iY3, int32_t iX4, int32_t iY4, const char *szMaterial, bool bIFT);
	CStdPalette *GetPal() const { return Surface8 ? Surface8->pPal : nullptr; }

	inline uint8_t _GetPix(int32_t x, int32_t y) // get landscape pixel (bounds not checked)
	{
		return Surface8->_GetPix(x, y);
	}

	inline uint32_t _GetPixDw(int32_t x, int32_t y, bool fApplyModulation) // get landscape pixel (bounds not checked)
	{
		return Surface32->GetPixDw(x, y, fApplyModulation);
	}

	inline uint8_t GetPix(int32_t x, int32_t y) // get landscape pixel (bounds checked)
	{
		extern uint8_t MCVehic;
		// Border checks
		if (x < 0)
			if (y < LeftOpen) return 0;
			else return MCVehic;
			if (x >= Width)
				if (y < RightOpen) return 0;
				else return MCVehic;
				if (y < 0)
					if (TopOpen) return 0;
					else return MCVehic;
					if (y >= Height)
						if (BottomOpen) return 0;
						else return MCVehic;
						return Surface8->_GetPix(x, y);
	}

	inline int32_t _GetMat(int32_t x, int32_t y) // get landscape material (bounds not checked)
	{
		return Pix2Mat[_GetPix(x, y)];
	}

	inline int32_t _GetDensity(int32_t x, int32_t y) // get landscape density (bounds not checked)
	{
		return Pix2Dens[_GetPix(x, y)];
	}

	inline int32_t GetMat(int32_t x, int32_t y) // get landscape material (bounds checked)
	{
		return Pix2Mat[GetPix(x, y)];
	}

	inline int32_t GetDensity(int32_t x, int32_t y) // get landscape density (bounds checked)
	{
		return Pix2Dens[GetPix(x, y)];
	}

	inline int32_t GetPlacement(int32_t x, int32_t y) // get landscape material placement (bounds checked)
	{
		return Pix2Place[GetPix(x, y)];
	}

	inline int32_t GetPixMat(uint8_t byPix) { return Pix2Mat[byPix]; }
	bool _PathFree(int32_t x, int32_t y, int32_t x2, int32_t y2); // quickly checks wether there *might* be pixel in the path.
	int32_t GetMatHeight(int32_t x, int32_t y, int32_t iYDir, int32_t iMat, int32_t iMax);
	int32_t DigFreePix(int32_t tx, int32_t ty);
	int32_t ShakeFreePix(int32_t tx, int32_t ty);
	int32_t BlastFreePix(int32_t tx, int32_t ty, int32_t grade, int32_t iBlastSize);
	int32_t AreaSolidCount(int32_t x, int32_t y, int32_t wdt, int32_t hgt);
	int32_t ExtractMaterial(int32_t fx, int32_t fy);
	bool DrawMap(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, const char *szMapDef); // creates and draws a map section using MapCreatorS2
	bool ClipRect(int32_t &rX, int32_t &rY, int32_t &rWdt, int32_t &rHgt); // clip given rect by landscape size; return whether anything is left unclipped
	bool DrawDefMap(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, const char *szMapDef); // creates and draws a map section using MapCreatorS2 and a map from the loaded Landscape.txt

	bool SetModulation(uint32_t dwWithClr) // adjust the way the landscape is blitted
	{
		Modulation = dwWithClr; return true;
	}

	uint32_t GetModulation() { return Modulation; }
	bool PostInitMap(); // do script callbacks of MapCreatorS2 in finished landscape
	bool ReplaceMapColor(uint8_t iOldIndex, uint8_t iNewIndex); // find every occurance of iOldIndex in map; replace it by new index
	bool SetTextureIndex(const char *szMatTex, uint8_t iNewIndex, bool fInsert); // change color index of map texture, or insert a new one
	void SetMapChanged() { fMapChanged = true; }
	void HandleTexMapUpdate();
	void UpdatePixMaps();
	bool DoRelights();
	void RemoveUnusedTexMapEntries();

protected:
	void ExecuteScan();
	int32_t DoScan(int32_t x, int32_t y, int32_t mat, int32_t dir);
	int32_t ChunkyRandom(int32_t &iOffset, int32_t iRange); // return static random value, according to offset and MapSeed
	void DrawChunk(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt, int32_t mcol, int32_t iChunkType, int32_t cro);
	void DrawSmoothOChunk(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt, int32_t mcol, uint8_t flip, int32_t cro);
	void ChunkOZoom(CSurface8 *sfcMap, int32_t iMapX, int32_t iMapY, int32_t iMapWdt, int32_t iMapHgt, int32_t iTexture, int32_t iOffX = 0, int32_t iOffY = 0);
	bool GetTexUsage(CSurface8 *sfcMap, int32_t iMapX, int32_t iMapY, int32_t iMapWdt, int32_t iMapHgt, uint32_t *dwpTextureUsage);
	bool TexOZoom(CSurface8 *sfcMap, int32_t iMapX, int32_t iMapY, int32_t iMapWdt, int32_t iMapHgt, uint32_t *dwpTextureUsage, int32_t iToX = 0, int32_t iToY = 0);
	bool MapToSurface(CSurface8 *sfcMap, int32_t iMapX, int32_t iMapY, int32_t iMapWdt, int32_t iMapHgt, int32_t iToX, int32_t iToY, int32_t iToWdt, int32_t iToHgt, int32_t iOffX, int32_t iOffY);
	bool MapToLandscape(CSurface8 *sfcMap, int32_t iMapX, int32_t iMapY, int32_t iMapWdt, int32_t iMapHgt, int32_t iOffsX = 0, int32_t iOffsY = 0); // zoom map segment to surface (or sector surfaces)
	bool GetMapColorIndex(const char *szMaterial, const char *szTexture, bool fIFT, uint8_t &rbyCol);
	bool SkyToLandscape(int32_t iToX, int32_t iToY, int32_t iToWdt, int32_t iToHgt, int32_t iOffX, int32_t iOffY);
	CSurface8 *CreateMap(); // create map by landscape attributes
	CSurface8 *CreateMapS2(C4Group &ScenFile); // create map by def file
	bool Relight(C4Rect To);
	bool ApplyLighting(C4Rect To);
	uint32_t GetClrByTex(int32_t iX, int32_t iY);
	bool Mat2Pal(); // assign material colors to landscape palette

	void DigFreeSinglePix(int32_t x, int32_t y, int32_t dx, int32_t dy)
	{
		if (GetDensity(x, y) > GetDensity(x + dx, y + dy))
			DigFreePix(x, y);
	}

	void UpdatePixCnt(const class C4Rect &Rect, bool fCheck = false);
	void UpdateMatCnt(C4Rect Rect, bool fPlus);
	void PrepareChange(C4Rect BoundingBox, bool updateMatCnt = true);
	void FinishChange(C4Rect BoundingBox, bool updateMatAndPixCnt = true);
	static bool DrawLineLandscape(int32_t iX, int32_t iY, int32_t iGrade);

public:
	void CompileFunc(StdCompiler *pComp); // without landscape bitmaps and sky
};

/* Some global landscape functions */

bool AboveSolid(int32_t &rx, int32_t &ry);
bool AboveSemiSolid(int32_t &rx, int32_t &ry);
bool SemiAboveSolid(int32_t &rx, int32_t &ry);
bool FindSolidGround(int32_t &rx, int32_t &ry, int32_t width);
bool FindLiquid(int32_t &rx, int32_t &ry, int32_t width, int32_t height);
bool FindSurfaceLiquid(int32_t &rx, int32_t &ry, int32_t width, int32_t height);
bool FindLevelGround(int32_t &rx, int32_t &ry, int32_t width, int32_t hrange);
bool FindConSiteSpot(int32_t &rx, int32_t &ry, int32_t wdt, int32_t hgt, uint32_t category, int32_t hrange = -1);
bool FindThrowingPosition(int32_t iTx, int32_t iTy, FIXED fXDir, FIXED fYDir, int32_t iHeight, int32_t &rX, int32_t &rY);
bool PathFree(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t *ix = nullptr, int32_t *iy = nullptr);
bool PathFreeIgnoreVehicle(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t *ix = nullptr, int32_t *iy = nullptr);
bool FindClosestFree(int32_t &rX, int32_t &rY, int32_t iAngle1, int32_t iAngle2, int32_t iExcludeAngle1, int32_t iExcludeAngle2);
bool ConstructionCheck(C4ID id, int32_t iX, int32_t iY, C4Object *pByObj = nullptr);
