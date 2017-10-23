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

/* Some useful wrappers to globals */

#pragma once

#ifdef C4ENGINE

#include <C4Id.h>
#include <C4Game.h>
#include <C4Landscape.h>
#include <C4Log.h>

// ID2Def

inline C4Def *C4Id2Def(C4ID id)
{
	return Game.Defs.ID2Def(id);
}

// Graphics Resource

#define GfxR (&(Game.GraphicsResource))

// Ticks

#define Tick2 Game.iTick2
#define Tick3 Game.iTick3
#define Tick5 Game.iTick5
#define Tick10 Game.iTick10
#define Tick35 Game.iTick35
#define Tick255 Game.iTick255
#define Tick1000 Game.iTick1000

// Players

int32_t ValidPlr(int32_t plr);
int32_t Hostile(int32_t plr1, int32_t plr2);

// IFT

inline uint8_t PixColIFT(uint8_t pixc)
{
	return pixc & IFT;
}

// always use OldGfx-version (used for convert)
inline uint8_t PixColIFTOld(uint8_t pixc)
{
	if (pixc >= GBM + IFTOld) return IFTOld;
	return 0;
}

// Density

inline bool DensitySolid(int32_t dens)
{
	return (dens >= C4M_Solid);
}

inline bool DensitySemiSolid(int32_t dens)
{
	return (dens >= C4M_SemiSolid);
}

inline bool DensityLiquid(int32_t dens)
{
	return ((dens >= C4M_Liquid) && (dens < C4M_Solid));
}

// Materials

extern int32_t MVehic, MTunnel, MWater, MSnow, MEarth, MGranite; // presearched materials
extern uint8_t MCVehic; // precalculated material color

#define GBackWdt Game.Landscape.Width
#define GBackHgt Game.Landscape.Height
#define GBackPix Game.Landscape.GetPix
#define SBackPix Game.Landscape.SetPix
#define ClearBackPix Game.Landscape.ClearPix
#define _GBackPix Game.Landscape._GetPix
#define _SBackPix Game.Landscape._SetPix
#define _SBackPixIfMask Game.Landscape._SetPixIfMask

int32_t PixCol2MatOld(uint8_t pixc);
int32_t PixCol2MatOld2(uint8_t pixc);

inline bool MatValid(int32_t mat)
{
	return Inside<int32_t>(mat, 0, Game.Material.Num - 1);
}

inline bool MatVehicle(int32_t iMat)
{
	return iMat == MVehic;
}

inline int32_t PixCol2Tex(uint8_t pixc)
{
	// Remove IFT
	int32_t iTex = int32_t(pixc & (IFT - 1));
	// Validate
	if (iTex >= C4M_MaxTexIndex) return 0;
	// Done
	return iTex;
}

inline int32_t PixCol2Mat(uint8_t pixc)
{
	// Get texture
	int32_t iTex = PixCol2Tex(pixc);
	if (!iTex) return MNone;
	// Get material-texture mapping
	const C4TexMapEntry *pTex = Game.TextureMap.GetEntry(iTex);
	// Return material
	return pTex ? pTex->GetMaterialIndex() : MNone;
}

inline uint8_t MatTex2PixCol(int32_t tex)
{
	return uint8_t(tex);
}

inline uint8_t Mat2PixColDefault(int32_t mat)
{
	return Game.Material.Map[mat].DefaultMatTex;
}

inline int32_t MatDensity(int32_t mat)
{
	if (!MatValid(mat)) return 0;
	return Game.Material.Map[mat].Density;
}

inline int32_t MatPlacement(int32_t mat)
{
	if (!MatValid(mat)) return 0;
	return Game.Material.Map[mat].Placement;
}

inline int32_t MatDigFree(int32_t mat)
{
	if (!MatValid(mat)) return 1;
	return Game.Material.Map[mat].DigFree;
}

inline uint8_t GBackIFT(int32_t x, int32_t y)
{
	return PixColIFT(GBackPix(x, y));
}

inline int32_t GBackMat(int32_t x, int32_t y)
{
	return Game.Landscape.GetMat(x, y);
}

inline int32_t GBackDensity(int32_t x, int32_t y)
{
	return Game.Landscape.GetDensity(x, y);
}

inline bool GBackSolid(int32_t x, int32_t y)
{
	return DensitySolid(GBackDensity(x, y));
}

inline bool GBackSemiSolid(int32_t x, int32_t y)
{
	return DensitySemiSolid(GBackDensity(x, y));
}

inline bool GBackLiquid(int32_t x, int32_t y)
{
	return DensityLiquid(GBackDensity(x, y));
}

inline int32_t GBackWind(int32_t x, int32_t y)
{
	return GBackIFT(x, y) ? 0 : Game.Weather.Wind;
}

// StdCompiler

void StdCompilerWarnCallback(void *pData, const char *szPosition, const char *szError);

template <class CompT, class StructT>
bool CompileFromBuf_LogWarn(StructT &&TargetStruct, const typename CompT::InT &SrcBuf, const char *szName)
{
	try
	{
		CompT Compiler;
		Compiler.setInput(SrcBuf);
		Compiler.setWarnCallback(StdCompilerWarnCallback, reinterpret_cast<void *>(const_cast<char *>(szName)));
		Compiler.Compile(TargetStruct);
		return true;
	}
	catch (StdCompiler::Exception *pExc)
	{
		if (!pExc->Pos.getLength())
			LogF("ERROR: %s (in %s)", pExc->Msg.getData(), szName);
		else
			LogF("ERROR: %s (in %s, %s)", pExc->Msg.getData(), pExc->Pos.getData(), szName);
		delete pExc;
		return false;
	}
}

template <class CompT, class StructT>
bool DecompileToBuf_Log(StructT &&TargetStruct, typename CompT::OutT *pOut, const char *szName)
{
	if (!pOut) return false;
	try
	{
		pOut->Take(DecompileToBuf<CompT>(TargetStruct));
		return true;
	}
	catch (StdCompiler::Exception *pExc)
	{
		LogF("ERROR: %s (in %s)", pExc->Msg.getData(), szName);
		delete pExc;
		return false;
	}
}

#endif // C4ENGINE
