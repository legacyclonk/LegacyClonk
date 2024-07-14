/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2021, The LegacyClonk Team and contributors
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

#include <C4Include.h>
#include <C4Landscape.h>
#include <C4SolidMask.h>

#include <C4Map.h>
#include <C4MapCreatorS2.h>
#include <C4SolidMask.h>
#include <C4Object.h>
#include <C4Physics.h>
#include <C4Random.h>
#include <C4SurfaceFile.h>
#include <C4ToolsDlg.h>
#ifdef DEBUGREC
#include <C4Record.h>
#endif
#include <C4Material.h>
#include <C4Game.h>
#include <C4Application.h>
#include <C4Wrappers.h>

#include <StdBitmap.h>
#include <StdPNG.h>

#include <cmath>
#include <memory>
#include <stdexcept>
#include <utility>

const int C4LS_MaxLightDistY = 8;
const int C4LS_MaxLightDistX = 1;

C4Landscape::C4Landscape(C4Section &section)
	: Section{section}, Sky{section}
{
	Default();
}

C4Landscape::~C4Landscape()
{
	Clear();
}

void C4Landscape::ScenarioInit()
{
	// Gravity
	Gravity = FIXED100(Section.C4S.Landscape.Gravity.Evaluate()) / 5;
	// Opens
	LeftOpen = Section.C4S.Landscape.LeftOpen;
	RightOpen = Section.C4S.Landscape.RightOpen;
	TopOpen = Section.C4S.Landscape.TopOpen;
	BottomOpen = Section.C4S.Landscape.BottomOpen;
	// Side open scan
	if (Section.C4S.Landscape.AutoScanSideOpen) ScanSideOpen();
}

bool C4Landscape::EnumerateMaterials()
{
	// Get hardcoded system material indices
	MVehic = Section.Material.Get("Vehicle"); MCVehic = Section.Mat2PixColDefault(MVehic);
	MTunnel = Section.Material.Get("Tunnel");
	MWater = Section.Material.Get("Water");
	MSnow = Section.Material.Get("Snow");
	MGranite = Section.Material.Get("Granite");

	const std::string_view material{Section.C4S.Landscape.Material};

	if (const std::size_t pos = material.find('-'); pos != std::string_view::npos)
	{
		if (CompareVersion(Section.C4S.Head.C4XVer[0], Section.C4S.Head.C4XVer[1], Section.C4S.Head.C4XVer[2], Section.C4S.Head.C4XVer[3], Section.C4S.Head.C4XVer[4], 4, 9, 10, 15, 359) == -1)
		{
			if (!Section.C4S.Landscape.InEarth.IsClear() || !Section.C4S.Animals.EarthNest.IsClear())
			{
				DebugLog(spdlog::level::warn, "Scenario.txt: Material={} specifies a texture, which breaks InEarth and Nest before [359]. Version=4,9,10,15,359 or higher enables the fixed behavior.", Section.C4S.Landscape.Material);
			}

			MEarth = MNone;
		}
		else
		{
			MEarth = Section.Material.Get(std::string{material.substr(0, pos)}.c_str());
		}
	}
	else
	{
		MEarth = Section.Material.Get(Section.C4S.Landscape.Material);
	}

	if ((MVehic == MNone) || (MTunnel == MNone))
	{
		LogFatal(C4ResStrTableKey::IDS_PRC_NOSYSMATS); return false;
	}

	return true;
}

void C4Landscape::Execute()
{
	// Landscape scan
	if (!NoScan)
		ExecuteScan();
	// move sky
	Sky.Execute();
	// Relights
	if (!Tick35)
		DoRelights();
}

void C4Landscape::ExecuteScan()
{
	int32_t cy, mat;

	// Check: Scan needed?
	const int32_t iTemperature = Section.Weather.GetTemperature();
	for (mat = 0; mat < Section.Material.Num; mat++)
		if (MatCount[mat])
			if (Section.Material.Map[mat].BelowTempConvertTo &&
				iTemperature < Section.Material.Map[mat].BelowTempConvert)
				break;
			else if (Section.Material.Map[mat].AboveTempConvertTo &&
				iTemperature > Section.Material.Map[mat].AboveTempConvert)
				break;
	if (mat >= Section.Material.Num)
		return;

#ifdef DEBUGREC_MATSCAN
	AddDbgRec(RCT_MatScan, &ScanX, sizeof(ScanX));
#endif

	for (int32_t cnt = 0; cnt < ScanSpeed; cnt++)
	{
		// Scan landscape column: sectors down
		int32_t last_mat = -1;
		for (cy = 0; cy < Height; cy++)
		{
			mat = _GetMat(ScanX, cy);
			// material change?
			if (last_mat != mat)
			{
				// upwards
				if (last_mat != -1)
					DoScan(ScanX, cy - 1, last_mat, 1);
				// downwards
				if (mat != -1)
					cy += DoScan(ScanX, cy, mat, 0);
			}
			last_mat = mat;
		}

		// Scan advance & rewind
		ScanX++;
		if (ScanX >= Width)
			ScanX = 0;
	}
}

#define PRETTY_TEMP_CONV

int32_t C4Landscape::DoScan(int32_t cx, int32_t cy, int32_t mat, int32_t dir)
{
	int32_t conv_to_tex = 0;
	int32_t iTemperature = Section.Weather.GetTemperature();
	// Check below conv
	if (Section.Material.Map[mat].BelowTempConvertDir == dir)
		if (Section.Material.Map[mat].BelowTempConvertTo)
			if (iTemperature < Section.Material.Map[mat].BelowTempConvert)
				conv_to_tex = Section.Material.Map[mat].BelowTempConvertTo;
	// Check above conv
	if (Section.Material.Map[mat].AboveTempConvertDir == dir)
		if (Section.Material.Map[mat].AboveTempConvertTo)
			if (iTemperature > Section.Material.Map[mat].AboveTempConvert)
				conv_to_tex = Section.Material.Map[mat].AboveTempConvertTo;
	// nothing to do?
	if (!conv_to_tex) return 0;
	// find material
	int32_t conv_to = Section.TextureMap.GetEntry(conv_to_tex)->GetMaterialIndex();
	// find mat top
	int32_t mconv = Section.Material.Map[mat].TempConvStrength,
		mconvs = mconv;
#ifdef DEBUGREC_MATSCAN
	C4RCMatScan rc = { cx, cy, mat, conv_to, dir, mconvs };
	AddDbgRec(RCT_MatScanDo, &rc, sizeof(C4RCMatScan));
#endif
	int32_t ydir = (dir == 0 ? +1 : -1), cy2;
#ifdef PRETTY_TEMP_CONV
	// get left pixel
	int32_t lmat = (cx > 0 ? _GetMat(cx - 1, cy) : -1);
	// left pixel not converted? do nothing
	if (lmat == mat) return 0;
	// left pixel converted? suppose it was done by a prior scan and skip check
	if (lmat != conv_to)
	{
		int32_t iSearchRange = std::max<int32_t>(5, mconvs);
		// search upper/lower bound
		int32_t cys = cy, cxs = cx;
		while (cxs < Width - 1)
		{
			// one step right
			cxs++;
			if (_GetMat(cxs, cys) == mat)
			{
				// search surface
				cys -= ydir;
				while (Inside<int32_t>(cys, 0, Height - 1) && _GetMat(cxs, cys) == mat)
				{
					cys -= ydir;
					if ((mconvs = (std::min)(mconv - Abs(cys - cy), mconvs)) < 0)
						return 0;
				}
				// out of bounds?
				if (!Inside<int32_t>(cys, 0, Height - 1)) break;
				// back one step
				cys += ydir;
			}
			else
			{
				// search surface
				cys += ydir;
				while (Inside<int32_t>(cys, 0, Height - 1) && _GetMat(cxs, cys) != mat)
				{
					cys += ydir;
					if (Abs(cys - cy) > iSearchRange)
						break;
				}
				// out of bounds?
				if (!Inside<int32_t>(cys, 0, Height - 1)) break;
				if (Abs(cys - cy) > iSearchRange) break;
			}
		}
	}
#endif
	// Conversion
	for (cy2 = cy; mconvs >= 0 && Inside<int32_t>(cy2, 0, Height - 1); cy2 += ydir, mconvs--)
	{
		// material changed?
		int32_t pix = _GetPix(cx, cy2);
		if (Section.PixCol2Mat(pix) != mat) break;
#ifdef PRETTY_TEMP_CONV
		// get left pixel
		int32_t lmat = (cx > 0 ? _GetMat(cx - 1, cy2) : -1);
		// left pixel not converted? break
		if (lmat == mat) break;
#endif
		// set mat
		SetPix(cx, cy2, Section.MatTex2PixCol(conv_to_tex) + PixColIFT(pix));
		CheckInstabilityRange(cx, cy2);
	}
	// return pixel converted
	return Abs(cy2 - cy);
}

void C4Landscape::ScanSideOpen()
{
	int32_t cy;
	for (cy = 0; (cy < Height) && !GetPix(0, cy); cy++);
	LeftOpen = cy;
	for (cy = 0; (cy < Height) && !GetPix(Width - 1, cy); cy++);
	RightOpen = cy;
}

void C4Landscape::Clear(bool fClearMapCreator, bool fClearSky)
{
	if (fClearMapCreator) { delete pMapCreator; pMapCreator = nullptr; }
	// clear sky
	if (fClearSky) Sky.Clear();
	// clear surfaces, if assigned
	delete Surface32;        Surface32        = nullptr;
	delete AnimationSurface; AnimationSurface = nullptr;
	delete Surface8;         Surface8         = nullptr;
	delete Map;              Map              = nullptr;
	// clear initial landscape
	delete[] pInitial;       pInitial         = nullptr;
	// clear scan
	ScanX = 0;
	Mode = C4LSC_Undefined;
	// clear pixel count
	delete[] PixCnt;         PixCnt           = nullptr;
	PixCntPitch = 0;
}

void C4Landscape::Draw(C4FacetEx &cgo, int32_t iPlayer)
{
	if (Modulation) Application.DDraw->ActivateBlitModulation(Modulation);
	// do relights
	DoRelights();
	// blit landscape
	if (Game.GraphicsSystem.ShowSolidMask)
		Application.DDraw->Blit8Fast(Surface8, cgo.TargetX, cgo.TargetY, cgo.Surface, cgo.X, cgo.Y, cgo.Wdt, cgo.Hgt);
	else
		Application.DDraw->BlitLandscape(Surface32, AnimationSurface, &Game.GraphicsResource.sfcLiquidAnimation, cgo.TargetX, cgo.TargetY, cgo.Surface, cgo.X, cgo.Y, cgo.Wdt, cgo.Hgt);
	if (Modulation) Application.DDraw->DeactivateBlitModulation();
}

int32_t C4Landscape::ChunkyRandom(int32_t &iOffset, int32_t iRange)
{
	if (!iRange) return 0;
	iOffset += 3;
	return (iOffset ^ MapSeed) % iRange;
}

void C4Landscape::DrawChunk(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt, int32_t mcol, int32_t iChunkType, int32_t cro)
{
	uint8_t top_rough; uint8_t side_rough;
	// what to do?
	switch (iChunkType)
	{
	case C4M_Flat:
		Surface8->Box(tx, ty, tx + wdt, ty + hgt, mcol);
		return;
	case C4M_TopFlat:
		top_rough = 0; side_rough = 1;
		break;
	case C4M_Smooth:
		top_rough = 1; side_rough = 1;
		break;
	case C4M_Rough:
		top_rough = 1; side_rough = 2;
		break;
	}
	int vtcs[16];
	int32_t rx = (std::max)(wdt / 2, 1);

	vtcs[ 0] = tx - ChunkyRandom(cro, rx / 2);                vtcs[ 1] = ty - ChunkyRandom(cro, rx / 2 * top_rough);
	vtcs[ 2] = tx - ChunkyRandom(cro, rx * side_rough);       vtcs[ 3] = ty + hgt / 2;
	vtcs[ 4] = tx - ChunkyRandom(cro, rx);                    vtcs[ 5] = ty + hgt + ChunkyRandom(cro, rx);
	vtcs[ 6] = tx + wdt / 2;                                  vtcs[ 7] = ty + hgt + ChunkyRandom(cro, 2 * rx);
	vtcs[ 8] = tx + wdt + ChunkyRandom(cro, rx);              vtcs[ 9] = ty + hgt + ChunkyRandom(cro, rx);
	vtcs[10] = tx + wdt + ChunkyRandom(cro, rx * side_rough); vtcs[11] = ty + hgt / 2;
	vtcs[12] = tx + wdt + ChunkyRandom(cro, rx / 2);          vtcs[13] = ty - ChunkyRandom(cro, rx / 2 * top_rough);
	vtcs[14] = tx + wdt / 2;                                  vtcs[15] = ty - ChunkyRandom(cro, rx * top_rough);

	Surface8->Polygon(8, vtcs, mcol);
}

void C4Landscape::DrawSmoothOChunk(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt, int32_t mcol, uint8_t flip, int32_t cro)
{
	int vtcs[8];
	int32_t rx = (std::max)(wdt / 2, 1);

	vtcs[0] = tx;       vtcs[1] = ty - ChunkyRandom(cro, rx / 2);
	vtcs[2] = tx;       vtcs[3] = ty + hgt;
	vtcs[4] = tx + wdt; vtcs[5] = ty + hgt;
	vtcs[6] = tx + wdt; vtcs[7] = ty - ChunkyRandom(cro, rx / 2);

	if (flip)
	{
		vtcs[0] = tx + wdt / 2; vtcs[1] = ty + hgt / 3;
	}
	else
	{
		vtcs[6] = tx + wdt / 2; vtcs[7] = ty + hgt / 3;
	}

	Surface8->Polygon(4, vtcs, mcol);
}

void C4Landscape::ChunkOZoom(CSurface8 *sfcMap, int32_t iMapX, int32_t iMapY, int32_t iMapWdt, int32_t iMapHgt, int32_t iTexture, int32_t iOffX, int32_t iOffY)
{
	int32_t iX, iY, iChunkWidth, iChunkHeight, iToX, iToY;
	int32_t iIFT;
	uint8_t byMapPixel, byMapPixelBelow;
	int iMapWidth, iMapHeight;
	C4Material *pMaterial = Section.TextureMap.GetEntry(iTexture)->GetMaterial();
	if (!pMaterial) return;
	int32_t iChunkType = pMaterial->MapChunkType;
	uint8_t byColor = Section.MatTex2PixCol(iTexture);
	// Get map & landscape size
	sfcMap->GetSurfaceSize(iMapWidth, iMapHeight);
	// Clip desired map segment to map size
	iMapX = BoundBy<int32_t>(iMapX, 0, iMapWidth - 1); iMapY = BoundBy<int32_t>(iMapY, 0, iMapHeight - 1);
	iMapWdt = BoundBy<int32_t>(iMapWdt, 0, iMapWidth - iMapX); iMapHgt = BoundBy<int32_t>(iMapHgt, 0, iMapHeight - iMapY);
	// get chunk size
	iChunkWidth = MapZoom; iChunkHeight = MapZoom;
	Surface32->Lock();
	if (AnimationSurface) AnimationSurface->Lock();
	// Scan map lines
	for (iY = iMapY; iY < iMapY + iMapHgt; iY++)
	{
		// Landscape target coordinate vertical
		iToY = iY * iChunkHeight + iOffY;
		// Scan map line
		for (iX = iMapX; iX < iMapX + iMapWdt; iX++)
		{
			// Map scan line start
			byMapPixel = sfcMap->GetPix(iX, iY);
			// Map scan line pixel below
			byMapPixelBelow = sfcMap->GetPix(iX, iY + 1);
			// Landscape target coordinate horizontal
			iToX = iX * iChunkWidth + iOffX;
			// Here's a chunk of the texture-material to zoom
			if ((byMapPixel & 127) == iTexture)
			{
				// Determine IFT
				iIFT = 0; if (byMapPixel >= 128) iIFT = IFT;
				// Draw chunk
				DrawChunk(iToX, iToY, iChunkWidth, iChunkHeight, byColor + iIFT, pMaterial->MapChunkType, (iX << 2) + iY);
			}
			// Other chunk, check for slope smoothers
			else
				// Smooth chunk & same texture-material below
				if ((iChunkType == C4M_Smooth) && (iY < iMapHeight - 1) && ((byMapPixelBelow & 127) == iTexture))
				{
					// Same texture-material on left
					if ((iX > 0) && ((sfcMap->GetPix(iX - 1, iY) & 127) == iTexture))
					{
						// Determine IFT
						iIFT = 0; if (sfcMap->GetPix(iX - 1, iY) >= 128) iIFT = IFT;
						// Draw smoother
						DrawSmoothOChunk(iToX, iToY, iChunkWidth, iChunkHeight, byColor + iIFT, 0, (iX << 2) + iY);
					}
					// Same texture-material on right
					if ((iX < iMapWidth - 1) && ((sfcMap->GetPix(iX + 1, iY) & 127) == iTexture))
					{
						// Determine IFT
						iIFT = 0; if (sfcMap->GetPix(iX + 1, iY) >= 128) iIFT = IFT;
						// Draw smoother
						DrawSmoothOChunk(iToX, iToY, iChunkWidth, iChunkHeight, byColor + iIFT, 1, (iX << 2) + iY);
					}
				}
		}
	}
	Surface32->Unlock();
	if (AnimationSurface) AnimationSurface->Unlock();
}

bool C4Landscape::GetTexUsage(CSurface8 *sfcMap, int32_t iMapX, int32_t iMapY, int32_t iMapWdt, int32_t iMapHgt, uint32_t *dwpTextureUsage)
{
	int iX, iY;
	// No good parameters
	if (!sfcMap || !dwpTextureUsage) return false;
	// Clip desired map segment to map size
	iMapX = BoundBy<int32_t>(iMapX, 0, sfcMap->Wdt - 1); iMapY = BoundBy<int32_t>(iMapY, 0, sfcMap->Hgt - 1);
	iMapWdt = BoundBy<int32_t>(iMapWdt, 0, sfcMap->Wdt - iMapX); iMapHgt = BoundBy<int32_t>(iMapHgt, 0, sfcMap->Hgt - iMapY);
	// Zero texture usage list
	for (int32_t cnt = 0; cnt < C4M_MaxTexIndex; cnt++) dwpTextureUsage[cnt] = 0;
	// Scan map pixels
	for (iY = iMapY; iY < iMapY + iMapHgt; iY++)
		for (iX = iMapX; iX < iMapX + iMapWdt; iX++)
			// Count texture map index only (no IFT)
			dwpTextureUsage[sfcMap->GetPix(iX, iY) & (IFT - 1)]++;
	// Done
	return true;
}

bool C4Landscape::TexOZoom(CSurface8 *sfcMap, int32_t iMapX, int32_t iMapY, int32_t iMapWdt, int32_t iMapHgt, uint32_t *dwpTextureUsage, int32_t iToX, int32_t iToY)
{
	int32_t iIndex;

	// ChunkOZoom all used textures
	for (iIndex = 1; iIndex < C4M_MaxTexIndex; iIndex++)
		if (dwpTextureUsage[iIndex] > 0)
		{
			// ChunkOZoom map to landscape
			ChunkOZoom(sfcMap, iMapX, iMapY, iMapWdt, iMapHgt, iIndex, iToX, iToY);
		}

	// Done
	return true;
}

bool C4Landscape::SkyToLandscape(int32_t iToX, int32_t iToY, int32_t iToWdt, int32_t iToHgt, int32_t iOffX, int32_t iOffY)
{
	if (!Surface32->Lock()) return false;
	// newgfx: simply blit the sky in realtime...
	Surface32->ClearBoxDw(iToX, iToY, iToWdt, iToHgt);
	Surface8->ClearBox8Only(iToX, iToY, iToWdt, iToHgt);
	// unlock
	Surface32->Unlock();
	// Done
	return true;
}

bool C4Landscape::MapToSurface(CSurface8 *sfcMap, int32_t iMapX, int32_t iMapY, int32_t iMapWdt, int32_t iMapHgt, int32_t iToX, int32_t iToY, int32_t iToWdt, int32_t iToHgt, int32_t iOffX, int32_t iOffY)
{
	// Sky background segment
	SkyToLandscape(iToX, iToY, iToWdt, iToHgt, iOffX, iOffY);

	// assign clipper
	Surface8->Clip(iToX, iToY, iToX + iToWdt - 1, iToY + iToHgt - 1);
	Surface32->Clip(iToX, iToY, iToX + iToWdt - 1, iToY + iToHgt - 1);
	if (AnimationSurface) AnimationSurface->Clip(iToX, iToY, iToX + iToWdt - 1, iToY + iToHgt - 1);
	Application.DDraw->NoPrimaryClipper();

	// Enlarge map segment for chunky rim
	iMapX -= 2; iMapY -= 2; iMapWdt += 4; iMapHgt += 4;

	// Determine texture usage in map segment
	uint32_t dwTexUsage[C4M_MaxTexIndex + 1];
	if (!GetTexUsage(sfcMap, iMapX, iMapY, iMapWdt, iMapHgt, dwTexUsage)) return false;
	// Texture zoom map to landscape
	if (!TexOZoom(sfcMap, iMapX, iMapY, iMapWdt, iMapHgt, dwTexUsage, iOffX, iOffY)) return false;

	// remove clipper
	Surface8->NoClip();
	Surface32->NoClip();
	if (AnimationSurface) AnimationSurface->NoClip();

	// success
	return true;
}

bool C4Landscape::MapToLandscape(CSurface8 *sfcMap, int32_t iMapX, int32_t iMapY, int32_t iMapWdt, int32_t iMapHgt, int32_t iOffsX, int32_t iOffsY)
{
	assert(Surface8); assert(Surface32);
	// Clip to map/landscape segment
	int iMapWidth, iMapHeight, iLandscapeWidth, iLandscapeHeight;
	// Get map & landscape size
	sfcMap->GetSurfaceSize(iMapWidth, iMapHeight);
	Surface8->GetSurfaceSize(iLandscapeWidth, iLandscapeHeight);
	// Clip map segment to map size
	iMapX = BoundBy<int32_t>(iMapX, 0, iMapWidth - 1); iMapY = BoundBy<int32_t>(iMapY, 0, iMapHeight - 1);
	iMapWdt = BoundBy<int32_t>(iMapWdt, 0, iMapWidth - iMapX); iMapHgt = BoundBy<int32_t>(iMapHgt, 0, iMapHeight - iMapY);
	// No segment
	if (!iMapWdt || !iMapHgt) return true;

	// Get affected landscape rect
	C4Rect To;
	To.x = iMapX * MapZoom + iOffsX;
	To.y = iMapY * MapZoom + iOffsY;
	To.Wdt = iMapWdt * MapZoom;
	To.Hgt = iMapHgt * MapZoom;

	Surface32->Lock();
	if (AnimationSurface) AnimationSurface->Lock();
	PrepareChange(To);
	MapToSurface(sfcMap, iMapX, iMapY, iMapWdt, iMapHgt, To.x, To.y, To.Wdt, To.Hgt, iOffsX, iOffsY);
	FinishChange(To);
	Surface32->Unlock();
	if (AnimationSurface) AnimationSurface->Unlock();
	return true;
}

CSurface8 *C4Landscape::CreateMap()
{
	std::int32_t width{0};
	std::int32_t height{0};

	// Create map surface
	Section.C4S.Landscape.GetMapSize(width, height, Game.Parameters.StartupPlayerCount);
	auto surfaceMap = std::make_unique<CSurface8>(width, height);

	// Fill sfcMap
	C4MapCreator MapCreator;
	MapCreator.Create(surfaceMap.get(),
		Section.C4S.Landscape, Section.TextureMap,
		true, Game.Parameters.StartupPlayerCount);

	return surfaceMap.release();
}

CSurface8 *C4Landscape::CreateMapS2(C4Group &ScenFile)
{
	// file present?
	if (!ScenFile.AccessEntry(C4CFN_DynLandscape)) return nullptr;

	// create map creator
	if (!pMapCreator)
		pMapCreator = new C4MapCreatorS2(&Section.C4S.Landscape, &Section.TextureMap, &Section.Material, Game.Parameters.StartupPlayerCount);

	// read file
	pMapCreator->ReadFile(C4CFN_DynLandscape, &ScenFile);
	// render landscape
	CSurface8 *sfc = pMapCreator->Render(nullptr);

	// keep map creator until script callbacks have been done
	return sfc;
}

bool C4Landscape::PostInitMap()
{
	// map creator present?
	if (!pMapCreator) return true;
	// call scripts
	pMapCreator->ExecuteCallbacks(MapZoom);
	// destroy map creator, if not needed later
	if (!Section.C4S.Landscape.KeepMapCreator) { delete pMapCreator; pMapCreator = nullptr; }
	// done, success
	return true;
}

bool C4Landscape::Init(C4Group &hGroup, bool fOverloadCurrent, bool fLoadSky, bool &rfLoaded, bool fSavegame)
{
	// set map seed, if not pre-assigned
	if (!MapSeed) MapSeed = Random(3133700);

	ShadeMaterials = Section.C4S.Landscape.ShadeMaterials;

	// increase max map size, since developers might set a greater one here
	Section.C4S.Landscape.MapWdt.Max = 10000;
	Section.C4S.Landscape.MapHgt.Max = 10000;

	// map and landscape must be initialized with fixed random, so runtime joining clients may recreate it
	// with same seed
	// after map/landscape creation, the seed must be fixed again, so there's no difference between clients creating
	// and not creating the map
	// this, however, would cause syncloss to DebugRecs
	C4DebugRecOff DBGRECOFF(!!Section.C4S.Landscape.ExactLandscape);

	Game.FixRandom(Game.Parameters.RandomSeed);

	// map is like it's loaded for regular gamestart
	// but it's changed and would have to be saved if a new section is loaded
	fMapChanged = fOverloadCurrent;

	// don't change landscape mode in runtime joins
	bool fLandscapeModeSet = (Mode != C4LSC_Undefined);

	Game.SetInitProgress(60);
	// create map if necessary
	if (!Section.C4S.Landscape.ExactLandscape)
	{
		CSurface8 *sfcMap = nullptr;
		// Static map from scenario
		if (hGroup.AccessEntry(C4CFN_Map))
			if (sfcMap = GroupReadSurface8(hGroup))
				if (!fLandscapeModeSet) Mode = C4LSC_Static;

		// allow C4CFN_Landscape as map for downwards compatibility
		if (!sfcMap)
			if (hGroup.AccessEntry(C4CFN_Landscape))
				if (sfcMap = GroupReadSurface8(hGroup))
				{
					if (!fLandscapeModeSet) Mode = C4LSC_Static;
					fMapChanged = true;
				}

		// dynamic map from file
		if (!sfcMap)
			if (sfcMap = CreateMapS2(hGroup))
				if (!fLandscapeModeSet) Mode = C4LSC_Dynamic;

		// Dynamic map by scenario
		if (!sfcMap && !fOverloadCurrent)
			if (sfcMap = CreateMap())
				if (!fLandscapeModeSet) Mode = C4LSC_Dynamic;

		// No map failure
		if (!sfcMap)
		{
			// no problem if only overloading
			if (!fOverloadCurrent) return false;
			if (fLoadSky) if (!Sky.Init(fSavegame)) return false;
			return true;
		}

#ifdef DEBUGREC
		AddDbgRec(RCT_Block, "|---MAP---|", 12);
		AddDbgRec(RCT_Map, sfcMap->Bits, sfcMap->Pitch * sfcMap->Hgt);
#endif

		// Store map size and calculate map zoom
		int iWdt, iHgt;
		sfcMap->GetSurfaceSize(iWdt, iHgt);
		MapWidth = iWdt; MapHeight = iHgt;
		MapZoom = Section.C4S.Landscape.MapZoom.Evaluate();

		// Calculate landscape size
		Width = MapZoom * MapWidth;
		Height = MapZoom * MapHeight;
		Width = std::max<int32_t>(Width, 100);
		Height = std::max<int32_t>(Height, 100);

		// if overloading, clear current landscape (and sections, etc.)
		// must clear, of course, before new sky is eventually read
		if (fOverloadCurrent) Clear(!Section.C4S.Landscape.KeepMapCreator, fLoadSky);

		// assign new map
		Map = sfcMap;

		// Sky (might need to know landscape height)
		if (fLoadSky)
		{
			Game.SetInitProgress(70);
			if (!Sky.Init(fSavegame)) return false;
		}
	}

	// Exact landscape from scenario (no map or exact recreation)
	else
	{
		C4DebugRecOff DBGRECOFF;
		// if overloading, clear current
		if (fOverloadCurrent) Clear(!Section.C4S.Landscape.KeepMapCreator, fLoadSky);
		// load it
		if (!fLandscapeModeSet) Mode = C4LSC_Exact;
		rfLoaded = true;
		if (!Load(hGroup, fLoadSky, fSavegame)) return false;
	}

	// Make pixel maps
	UpdatePixMaps();

	// progress
	Game.SetInitProgress(80);

	// mark as new-style
	Section.C4S.Landscape.NewStyleLandscape = 2;

	// copy noscan-var
	NoScan = Section.C4S.Landscape.NoScan;

	// Scan settings
	ScanSpeed = BoundBy(Width / 500, 2, 15);

	// create it
	if (!Section.C4S.Landscape.ExactLandscape)
	{
		// map to big surface and sectionize it
		// Create landscape surface
		Surface32 = new C4Surface();
		Surface8 = new CSurface8();
		if (Config.Graphics.ColorAnimation && Config.Graphics.Shader)
			AnimationSurface = new C4Surface(Width, Height);
		if (!Surface32->Create(Width, Height, true)
			|| !Surface8->Create(Width, Height, true)
			|| (AnimationSurface && !AnimationSurface->Create(Width, Height))
			|| !Mat2Pal())
		{
			delete Surface8;    delete Surface32;    delete AnimationSurface;
			Surface8 = nullptr; Surface32 = nullptr; AnimationSurface = nullptr;
			return false;
		}

		// Map to landscape
		if (!MapToLandscape()) return false;
	}
	Game.SetInitProgress(87);

#ifdef DEBUGREC
	AddDbgRec(RCT_Block, "|---LS---|", 11);
	AddDbgRec(RCT_Ls, Surface8->Bits, Surface8->Pitch * Surface8->Hgt);
#endif

	// Create pixel count array
	// We will use 15x17 blocks so the pixel count can't get over 255.
	int32_t PixCntWidth = (Width + 16) / 17;
	PixCntPitch = (Height + 14) / 15;
	PixCnt = new uint8_t[PixCntWidth * PixCntPitch];
	UpdatePixCnt(C4Rect(0, 0, Width, Height));
	ClearMatCount();
	UpdateMatCnt(C4Rect(0, 0, Width, Height), true);

	// Save initial landscape
	if (!SaveInitial())
		return false;

	// Load diff, if existent
	ApplyDiff(hGroup);

	// enforce first color to be transparent
	Surface8->EnforceC0Transparency();

	// after map/landscape creation, the seed must be fixed again, so there's no difference between clients creating
	// and not creating the map
	Game.FixRandom(Game.Parameters.RandomSeed);

	// Success
	rfLoaded = true;
	return true;
}

bool C4Landscape::SetPix(int32_t x, int32_t y, uint8_t npix)
{
#ifdef DEBUGREC
	C4RCSetPix rc;
	rc.x = x; rc.y = y; rc.clr = npix;
	AddDbgRec(RCT_SetPix, &rc, sizeof(rc));
#endif
	// check bounds
	if (x < 0 || y < 0 || x >= Width || y >= Height)
		return false;
	// no change?
	if (npix == _GetPix(x, y))
		return true;
	// note for relight
	C4Rect CheckRect(x - 2 * C4LS_MaxLightDistX, y - 2 * C4LS_MaxLightDistY, 4 * C4LS_MaxLightDistX + 1, 4 * C4LS_MaxLightDistY + 1);
	for (int32_t i = 0; i < C4LS_MaxRelights; i++)
		if (!Relights[i].Wdt || Relights[i].Overlap(CheckRect) || i + 1 >= C4LS_MaxRelights)
		{
			Relights[i].Add(C4Rect(x, y, 1, 1));
			break;
		}
	// set pixel
	return _SetPix(x, y, npix);
}

bool C4Landscape::SetPixDw(int32_t x, int32_t y, uint32_t dwPix)
{
	if (!Surface32->LockForUpdate({x, y, 1, 1}))
	{
		return false;
	}
	// set in surface
	const auto ret = Surface32->SetPixDw(x, y, dwPix);
	Surface32->Unlock();
	return ret;
}

bool C4Landscape::_SetPix(int32_t x, int32_t y, uint8_t npix)
{
#ifdef DEBUGREC
	C4RCSetPix rc;
	rc.x = x; rc.y = y; rc.clr = npix;
	AddDbgRec(RCT_SetPix, &rc, sizeof(rc));
#endif
	assert(x >= 0 && y >= 0 && x < Width && y < Height);
	// get and check pixel
	uint8_t opix = _GetPix(x, y);
	if (npix == opix) return true;
	// count pixels
	if (Pix2Dens[npix])
	{
		if (!Pix2Dens[opix]) PixCnt[(y / 15) + (x / 17) * PixCntPitch]++;
	}
	else
	{
		if (Pix2Dens[opix]) PixCnt[(y / 15) + (x / 17) * PixCntPitch]--;
	}

	// count material
	if (!npix || Section.MatValid(Pix2Mat[npix]))
	{
		int32_t omat = Pix2Mat[opix], nmat = Pix2Mat[npix];
		if (opix) MatCount[omat]--;
		if (npix) MatCount[nmat]++;
		// count effective material
		if (omat != nmat)
		{
			if (npix && Section.Material.Map[nmat].MinHeightCount)
			{
				// Check for material above & below
				int iMinHeight = Section.Material.Map[nmat].MinHeightCount,
					iBelow = GetMatHeight(x, y + 1, +1, nmat, iMinHeight),
					iAbove = GetMatHeight(x, y - 1, -1, nmat, iMinHeight);
				// Will be above treshold?
				if (iBelow + iAbove + 1 >= iMinHeight)
				{
					int iChange = 1;
					// Check for heights below threshold
					if (iBelow < iMinHeight) iChange += iBelow;
					if (iAbove < iMinHeight) iChange += iAbove;
					// Change
					EffectiveMatCount[nmat] += iChange;
				}
			}
			if (opix && Section.Material.Map[omat].MinHeightCount)
			{
				// Check for material above & below
				int iMinHeight = Section.Material.Map[omat].MinHeightCount,
					iBelow = GetMatHeight(x, y + 1, +1, omat, iMinHeight),
					iAbove = GetMatHeight(x, y - 1, -1, omat, iMinHeight);
				// Not already below threshold?
				if (iBelow + iAbove + 1 >= iMinHeight)
				{
					int iChange = 1;
					// Check for heights that will get below threshold
					if (iBelow < iMinHeight) iChange += iBelow;
					if (iAbove < iMinHeight) iChange += iAbove;
					// Change
					EffectiveMatCount[omat] -= iChange;
				}
			}
		}
	}

	// set 8bpp-surface only!
	Surface8->SetPix(x, y, npix);
	// success
	return true;
}

bool C4Landscape::_SetPixIfMask(int32_t x, int32_t y, uint8_t npix, uint8_t nMask)
{
	// set 8bpp-surface only!
	if (_GetPix(x, y) == nMask)
		_SetPix(x, y, npix);
	// success
	return true;
}

bool C4Landscape::CheckInstability(int32_t tx, int32_t ty)
{
	int32_t mat = GetMat(tx, ty);
	if (Section.MatValid(mat))
		if (Section.Material.Map[mat].Instable)
			return Section.MassMover.Create(tx, ty);
	return false;
}

void C4Landscape::CheckInstabilityRange(int32_t tx, int32_t ty)
{
	if (!CheckInstability(tx, ty))
	{
		CheckInstability(tx, ty - 1);
		CheckInstability(tx, ty - 2);
		CheckInstability(tx - 1, ty);
		CheckInstability(tx + 1, ty);
	}
}

bool C4Landscape::ClearPix(int32_t tx, int32_t ty)
{
	uint8_t bcol;
	if (GBackIFT(tx, ty))
		bcol = Section.Mat2PixColDefault(MTunnel) + IFT;
	else
		bcol = 0;
	return SetPix(tx, ty, bcol);
}

bool C4Landscape::_PathFree(int32_t x, int32_t y, int32_t x2, int32_t y2)
{
	x /= 17; y /= 15; x2 /= 17; y2 /= 15;
	while (x != x2 && y != y2)
	{
		if (PixCnt[x * PixCntPitch + y])
			return false;
		if (x > x2) x--; else x++;
		if (y > y2) y--; else y++;
	}
	if (x != x2)
		do
		{
			if (PixCnt[x * PixCntPitch + y])
				return false;
			if (x > x2) x--; else x++;
		} while (x != x2);
	else
		while (y != y2)
		{
			if (PixCnt[x * PixCntPitch + y])
				return false;
			if (y > y2) y--; else y++;
		}
	return !PixCnt[x * PixCntPitch + y];
}

int32_t C4Landscape::GetMatHeight(int32_t x, int32_t y, int32_t iYDir, int32_t iMat, int32_t iMax)
{
	if (iYDir > 0)
	{
		iMax = std::min<int32_t>(iMax, Height - y);
		for (int32_t i = 0; i < iMax; i++)
			if (_GetMat(x, y + i) != iMat)
				return i;
	}
	else
	{
		iMax = std::min<int32_t>(iMax, y + 1);
		for (int32_t i = 0; i < iMax; i++)
			if (_GetMat(x, y - i) != iMat)
				return i;
	}
	return iMax;
}

int32_t C4Landscape::DigFreePix(int32_t tx, int32_t ty)
{
	int32_t mat = GetMat(tx, ty);
	if (mat != MNone)
		if (Section.Material.Map[mat].DigFree)
			ClearPix(tx, ty);
	CheckInstabilityRange(tx, ty);
	return mat;
}

int32_t C4Landscape::ShakeFreePix(int32_t tx, int32_t ty)
{
	int32_t mat = GetMat(tx, ty);
	if (mat != MNone)
		if (Section.Material.Map[mat].DigFree)
		{
			ClearPix(tx, ty);
			Section.PXS.Create(mat, itofix(tx), itofix(ty));
		}
	CheckInstabilityRange(tx, ty);
	return mat;
}

int32_t C4Landscape::BlastFreePix(int32_t tx, int32_t ty, int32_t grade, int32_t iBlastSize)
{
	int32_t mat = GetMat(tx, ty);
	if (Section.MatValid(mat))
	{
		// Blast Shift
		if (Section.Material.Map[mat].BlastShiftTo)
		{
			// blast free amount; always blast if 100% is to be blasted away
			if (Random(BlastMatCount[mat]) < iBlastSize * grade / 6)
				SetPix(tx, ty, Section.MatTex2PixCol(Section.Material.Map[mat].BlastShiftTo) + GBackIFT(tx, ty));
		}
		// Blast Free
		if (Section.Material.Map[mat].BlastFree) ClearPix(tx, ty);
	}

	CheckInstabilityRange(tx, ty);

	return mat;
}

void C4Landscape::DigFree(int32_t tx, int32_t ty, int32_t rad, bool fRequest, C4Object *pByObj)
{
	int32_t ycnt, xcnt, iLineWidth, iLineY, iMaterial;
	// Dig free
	for (ycnt = -rad; ycnt < rad; ycnt++)
	{
		iLineWidth = static_cast<int32_t>(sqrt(double(rad * rad - ycnt * ycnt)));
		iLineY = ty + ycnt;
		for (xcnt = -iLineWidth; xcnt < iLineWidth + (iLineWidth == 0); xcnt++)
			if (Section.MatValid(iMaterial = DigFreePix(tx + xcnt, iLineY)))
				if (pByObj) pByObj->AddMaterialContents(iMaterial, 1);
		// Clear single pixels - left and right
		DigFreeSinglePix(tx - iLineWidth - 1, iLineY, -1, 0);
		DigFreeSinglePix(tx + iLineWidth + (iLineWidth == 0), iLineY, +1, 0);
	}
	// Clear single pixels - up and down
	DigFreeSinglePix(tx, ty - rad - 1, 0, -1);
	for (xcnt = -iLineWidth; xcnt < iLineWidth + (iLineWidth == 0); xcnt++)
		DigFreeSinglePix(tx + xcnt, ty + rad, 0, +1);
	// Dig out material cast
	if (!Tick5) if (pByObj) pByObj->DigOutMaterialCast(fRequest);
}

void C4Landscape::DigFreeRect(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt, bool fRequest, C4Object *pByObj)
{
	// Dig free pixels
	int32_t cx, cy, iMaterial;
	for (cx = tx; cx < tx + wdt; cx++)
		for (cy = ty; cy < ty + hgt; cy++)
			if (Section.MatValid(iMaterial = DigFreePix(cx, cy)))
				if (pByObj) pByObj->AddMaterialContents(iMaterial, 1);
	// Clear single pixels

	// Dig out material cast
	if (!Tick5) if (pByObj) pByObj->DigOutMaterialCast(fRequest);
}

void C4Landscape::ShakeFree(int32_t tx, int32_t ty, int32_t rad)
{
	int32_t ycnt, xcnt, lwdt, dpy;
	// Shake free pixels
	for (ycnt = rad - 1; ycnt >= -rad; ycnt--)
	{
		lwdt = static_cast<int32_t>(sqrt(double(rad * rad - ycnt * ycnt)));
		dpy = ty + ycnt;
		for (xcnt = -lwdt; xcnt < lwdt + (lwdt == 0); xcnt++)
			ShakeFreePix(tx + xcnt, dpy);
	}
}

void C4Landscape::DigFreeMat(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt, int32_t mat)
{
	int32_t cx, cy;
	if (Section.MatValid(mat))
		for (cx = tx; cx < tx + wdt; cx++)
			for (cy = ty; cy < ty + hgt; cy++)
				if (GetMat(cx, cy) == mat)
					DigFreePix(cx, cy);
}

void C4Landscape::BlastFree(int32_t tx, int32_t ty, int32_t rad, int32_t grade, int32_t iByPlayer)
{
	int32_t ycnt, xcnt, lwdt, dpy, mat, cnt;

	// Reset material count
	ClearBlastMatCount();

	// Blast free pixels
	// count pixel before, so BlastShiftTo can be evaluated
	for (ycnt = -rad; ycnt <= rad; ycnt++)
	{
		lwdt = static_cast<int32_t>(sqrt(double(rad * rad - ycnt * ycnt))); dpy = ty + ycnt;
		for (xcnt = -lwdt; xcnt < lwdt + (lwdt == 0); xcnt++)
			if (Section.MatValid(mat = GetMat(tx + xcnt, dpy)))
				BlastMatCount[mat]++;
	}
	// blast pixels
	int32_t iBlastSize = rad * rad * 6283 / 2000; // rad^2 * pi
	for (ycnt = -rad; ycnt <= rad; ycnt++)
	{
		lwdt = static_cast<int32_t>(sqrt(double(rad * rad - ycnt * ycnt))); dpy = ty + ycnt;
		for (xcnt = -lwdt; xcnt < lwdt + (lwdt == 0); xcnt++)
			BlastFreePix(tx + xcnt, dpy, grade, iBlastSize);
	}

	// Evaluate material count
	for (cnt = 0; cnt < Section.Material.Num; cnt++)
		if (BlastMatCount[cnt])
		{
			if (Section.Material.Map[cnt].Blast2Object != C4ID_None)
				if (Section.Material.Map[cnt].Blast2ObjectRatio != 0)
					Section.BlastCastObjects(Section.Material.Map[cnt].Blast2Object, nullptr,
						BlastMatCount[cnt] / Section.Material.Map[cnt].Blast2ObjectRatio,
						tx, ty, iByPlayer);

			if (Section.Material.Map[cnt].Blast2PXSRatio != 0)
				Section.PXS.Cast(cnt,
					BlastMatCount[cnt] / Section.Material.Map[cnt].Blast2PXSRatio,
					tx, ty, 60);
		}
}

void C4Landscape::DrawMaterialRect(int32_t mat, int32_t tx, int32_t ty, int32_t wdt, int32_t hgt)
{
	int32_t cx, cy;
	for (cy = ty; cy < ty + hgt; cy++)
		for (cx = tx; cx < tx + wdt; cx++)
			if ((Section.MatDensity(mat) > GetDensity(cx, cy))
				|| ((Section.MatDensity(mat) == GetDensity(cx, cy)) && (Section.MatDigFree(mat) <= Section.MatDigFree(GetMat(cx, cy)))))
				SetPix(cx, cy, Section.Mat2PixColDefault(mat) + GBackIFT(cx, cy));
}

void C4Landscape::RaiseTerrain(int32_t tx, int32_t ty, int32_t wdt)
{
	int32_t cx, cy;
	uint8_t cpix;
	for (cx = tx; cx < tx + wdt; cx++)
	{
		for (cy = ty; (cy + 1 < Height) && !GBackSolid(cx, cy + 1); cy++);
		if (cy + 1 < Height) if (cy - ty < 20)
		{
			cpix = GetPix(cx, cy + 1);
			if (!MatVehicle(Section.PixCol2Mat(cpix)))
				while (cy >= ty) { SetPix(cx, cy, cpix); cy--; }
		}
	}
}

int32_t C4Landscape::AreaSolidCount(int32_t x, int32_t y, int32_t wdt, int32_t hgt)
{
	int32_t cx, cy, ascnt = 0;
	for (cy = y; cy < y + hgt; cy++)
		for (cx = x; cx < x + wdt; cx++)
			if (GBackSolid(cx, cy))
				ascnt++;
	return ascnt;
}

void C4Landscape::FindMatTop(int32_t mat, int32_t &x, int32_t &y)
{
	int32_t mslide, cslide, tslide; // tslide 0 none 1 left 2 right
	bool fLeft, fRight;

	if (!Section.MatValid(mat)) return;
	mslide = Section.Material.Map[mat].MaxSlide;

	do
	{
		// Find upwards slide
		fLeft = true; fRight = true; tslide = 0;
		for (cslide = 0; (cslide <= mslide) && (fLeft || fRight); cslide++)
		{
			// Left
			if (fLeft)
				if (GetMat(x - cslide, y) != mat) fLeft = false;
				else if (GetMat(x - cslide, y - 1) == mat) { tslide = 1; break; }
				// Right
				if (fRight)
					if (GetMat(x + cslide, y) != mat) fRight = false;
					else if (GetMat(x + cslide, y - 1) == mat) { tslide = 2; break; }
		}

		// Slide
		if (tslide == 1) { x -= cslide; y--; }
		if (tslide == 2) { x += cslide; y--; }
	} while (tslide);
}

int32_t C4Landscape::ExtractMaterial(int32_t fx, int32_t fy)
{
	int32_t mat = GetMat(fx, fy);
	if (mat == MNone) return MNone;
	FindMatTop(mat, fx, fy);
	ClearPix(fx, fy);
	CheckInstabilityRange(fx, fy);
	return mat;
}

bool C4Landscape::InsertMaterial(int32_t mat, int32_t tx, int32_t ty, int32_t vx, int32_t vy)
{
	int32_t mdens;
	if (!Section.MatValid(mat)) return false;
	mdens = Section.MatDensity(mat);
	if (!mdens) return true;

	// Bounds
	if (!Inside<int32_t>(tx, 0, Width - 1) || !Inside<int32_t>(ty, 0, Height)) return false;

	if (Section.C4S.Game.Realism.LandscapePushPull)
	{
		// Same or higher density?
		if (GetDensity(tx, ty) >= mdens)
			// Push
			if (!FindMatPathPush(tx, ty, mdens, Section.Material.Map[mat].MaxSlide, !!Section.Material.Map[mat].Instable))
				// Or die
				return false;
	}
	else
	{
		// Move up above same density
		while (mdens == GetDensity(tx, ty))
		{
			ty--; if (ty < 0) return false;
			// Primitive slide (1)
			if (GetDensity(tx - 1, ty) < mdens) tx--;
			if (GetDensity(tx + 1, ty) < mdens) tx++;
		}
		// Stuck in higher density
		if (GetDensity(tx, ty) > mdens) return false;
	}

	// Try slide
	while (FindMatSlide(tx, ty, +1, mdens, Section.Material.Map[mat].MaxSlide))
		if (GetDensity(tx, ty + 1) < mdens)
		{
			Section.PXS.Create(mat, itofix(tx), itofix(ty), FIXED10(vx), FIXED10(vy)); return true;
		}

	// Try reaction with material below
	C4MaterialReaction *pReact; int32_t tmat;
	if (pReact = Section.Material.GetReactionUnsafe(mat, tmat = GetMat(tx, ty + Sign(Gravity))))
	{
		C4Fixed fvx = FIXED10(vx), fvy = FIXED10(vy);
		if ((*pReact->pFunc)(pReact, Section, tx, ty, tx, ty + Sign(Gravity), fvx, fvy, mat, tmat, meePXSPos, nullptr))
		{
			// the material to be inserted killed itself in some material reaction below
			return true;
		}
	}

	int omat;
	if (Section.C4S.Game.Realism.LandscapeInsertThrust)
		omat = GetMat(tx, ty);

	// Insert dead material
	SetPix(tx, ty, Section.Mat2PixColDefault(mat) + GBackIFT(tx, ty));

	// Search a position for the old material pixel
	if (Section.C4S.Game.Realism.LandscapeInsertThrust && Section.MatValid(omat))
		InsertMaterial(omat, tx, ty - 1);

	return true;
}

// Finds the next pixel position moving to desired slide.

bool C4Landscape::FindMatPath(int32_t &fx, int32_t &fy, int32_t ydir, int32_t mdens, int32_t mslide)
{
	int32_t cslide;
	bool fLeft = true, fRight = true;

	// One downwards
	if (GetDensity(fx, fy + ydir) < mdens) { fy += ydir; return true; }

	// Find downwards slide path
	for (cslide = 1; (cslide <= mslide) && (fLeft || fRight); cslide++)
	{
		// Check left
		if (fLeft)
			if (GetDensity(fx - cslide, fy) >= mdens) // Left clogged
				fLeft = false;
			else if (GetDensity(fx - cslide, fy + ydir) < mdens) // Left slide okay
			{
				fx--; return true;
			}
		// Check right
		if (fRight)
			if (GetDensity(fx + cslide, fy) >= mdens) // Right clogged
				fRight = false;
			else if (GetDensity(fx + cslide, fy + ydir) < mdens) // Right slide okay
			{
				fx++; return true;
			}
	}

	return false;
}

// Finds the closest immediate slide position.

bool C4Landscape::FindMatSlide(int32_t &fx, int32_t &fy, int32_t ydir, int32_t mdens, int32_t mslide)
{
	int32_t cslide;
	bool fLeft = true, fRight = true;

	// One downwards
	if (GetDensity(fx, fy + ydir) < mdens) { fy += ydir; return true; }

	// Find downwards slide path
	for (cslide = 1; (cslide <= mslide) && (fLeft || fRight); cslide++)
	{
		// Check left
		if (fLeft)
			if (GetDensity(fx - cslide, fy) >= mdens && GetDensity(fx - cslide, fy + ydir) >= mdens) // Left clogged
				fLeft = false;
			else if (GetDensity(fx - cslide, fy + ydir) < mdens) // Left slide okay
			{
				fx -= cslide; fy += ydir; return true;
			}
		// Check right
		if (fRight)
			if (GetDensity(fx + cslide, fy) >= mdens && GetDensity(fx + cslide, fy + ydir) >= mdens) // Right clogged
				fRight = false;
			else if (GetDensity(fx + cslide, fy + ydir) < mdens) // Right slide okay
			{
				fx += cslide; fy += ydir; return true;
			}
	}

	return false;
}

// Find closest point with density below mdens. Note this may return a point outside of the landscape,
// Assumption: There are no holes with smaller density inside of material with greater
//             density.
bool C4Landscape::FindMatPathPush(int32_t &fx, int32_t &fy, int32_t mdens, int32_t mslide, bool liquid)
{
	// Startpoint must be inside landscape
	fx = BoundBy<int32_t>(fx, 0, Width - 1);
	fy = BoundBy<int32_t>(fy, 0, Height - 1);
	// Range to search, calculate bounds
	const int32_t iPushRange = 500;
	int32_t left = std::max<int32_t>(0, fx - iPushRange), right = std::min<int32_t>(Width - 1, fx + iPushRange),
		top = std::max<int32_t>(0, fy - iPushRange), bottom = std::min<int32_t>(Height - 1, fy + iPushRange);
	// Direction constants
	const int8_t R = 0, D = 1, L = 2, U = 3;
	int8_t dir = 0;
	int32_t x = fx, y = fy;
	// Get startpoint density
	int32_t dens = GetDensity(fx, fy);
	// Smaller density? We're done.
	if (dens < mdens)
		return true;
	// Right density?
	else if (dens == mdens)
	{
		// Find start point for border search
		for (int32_t i = 0; ; i++)
			if (x - i - 1 < left || GetDensity(x - i - 1, y) != mdens)
			{
				x -= i; dir = L; break;
			}
			else if (y - i - 1 < top || GetDensity(x, y - i - 1) != mdens)
			{
				y -= i; dir = U; break;
			}
			else if (x + i + 1 > right || GetDensity(x + i + 1, y) != mdens)
			{
				x += i; dir = R; break;
			}
			else if (y + i + 1 > bottom || GetDensity(x, y + i + 1) != mdens)
			{
				y += i; dir = D; break;
			}
	}
	// Greater density
	else
	{
		// Try to find a way out
		int i = 1;
		for (; i < iPushRange; i++)
			if (GetDensity(x - i, y) <= mdens)
			{
				x -= i; dir = R; break;
			}
			else if (GetDensity(x, y - i) <= mdens)
			{
				y -= i; dir = D; break;
			}
			else if (GetDensity(x + i, y) <= mdens)
			{
				x += i; dir = L; break;
			}
			else if (GetDensity(x, y + i) <= mdens)
			{
				y += i; dir = U; break;
			}
		// Not found?
		if (i >= iPushRange) return false;
		// Done?
		if (GetDensity(x, y) < mdens)
		{
			fx = x; fy = y;
			return true;
		}
	}
	// Save startpoint of search
	int32_t sx = x, sy = y, sdir = dir;
	// Best point so far
	bool fGotBest = false; int32_t bx, by, bdist;
	// Start searching
	do
	{
		// We should always be in a material of same density
		assert(x >= left && y >= top && x <= right && y <= bottom && GetDensity(x, y) == mdens);
		// Calc new position
		int nx = x, ny = y;
		switch (dir)
		{
		case R: nx++; break;
		case D: ny++; break;
		case L: nx--; break;
		case U: ny--; break;
		default: assert(false);
		}
		// In bounds?
		bool fInBounds = (nx >= left && ny >= top && nx <= right && ny <= bottom);
		// Get density. Not this performs an SideOpen-check if outside landscape bounds.
		int32_t dens = GetDensity(nx, ny);
		// Flow possible?
		if (dens < mdens)
		{
			// Calculate "distance".
			int32_t dist = Abs(nx - fx) + mslide * (liquid ? fy - ny : Abs(fy - ny));
			// New best point?
			if (!fGotBest || dist < bdist)
			{
				// Save it
				bx = nx; by = ny; bdist = dist; fGotBest = true;
				// Adjust borders: We can obviously safely ignore anything at greater distance
				top = std::max<int32_t>(top, fy - dist / mslide - 1);
				if (!liquid)
				{
					bottom = std::min<int32_t>(bottom, fy + dist / mslide + 1);
					left = std::max<int32_t>(left, fx - dist - 1);
					right = std::min<int32_t>(right, fx + dist + 1);
				}
				// Set new startpoint
				sx = x; sy = y; sdir = dir;
			}
		}
		// Step?
		if (fInBounds && dens == mdens)
		{
			// New point
			x = nx; y = ny;
			// Turn left
			(dir += 3) %= 4;
		}
		// Otherwise: Turn right
		else
			++dir %= 4;
	} while (x != sx || y != sy || dir != sdir);
	// Nothing found?
	if (!fGotBest) return false;
	// Return it
	fx = bx; fy = by;
	return true;
}

bool C4Landscape::Incinerate(int32_t x, int32_t y)
{
	int32_t mat = GetMat(x, y);
	if (Section.MatValid(mat))
		if (Section.Material.Map[mat].Inflammable)
			// Not too much FLAMs
			if (!Section.FindObject(C4Id("FLAM"), x - 4, y - 1, 8, 20))
				if (Section.CreateObject(C4Id("FLAM"), nullptr, NO_OWNER, x, y))
					return true;
	return false;
}

bool C4Landscape::Save(C4Group &hGroup)
{
	// Save members
	if (!Sky.Save(hGroup))
		return false;

	// Save landscape surface
	char szTempLandscape[_MAX_PATH + 1];
	SCopy(Config.AtTempPath(C4CFN_TempLandscape), szTempLandscape);
	MakeTempFilename(szTempLandscape);
	if (!Surface8->Save(szTempLandscape))
		return false;

	// Move temp file to group
	if (!hGroup.Move(szTempLandscape, C4CFN_Landscape))
		return false;

	SCopy(Config.AtTempPath(C4CFN_TempLandscapePNG), szTempLandscape);
	MakeTempFilename(szTempLandscape);
	if (!Surface32->SavePNG(szTempLandscape, true, false, false))
		return false;
	if (!hGroup.Move(szTempLandscape, C4CFN_LandscapePNG)) return false;

	if (fMapChanged && Map)
		if (!SaveMap(hGroup)) return false;

	// save textures (if changed)
	if (!SaveTextures(hGroup)) return false;

	return true;
}

bool C4Landscape::SaveDiff(C4Group &hGroup, bool fSyncSave)
{
	assert(pInitial);
	if (!pInitial) return false;

	// If it shouldn't be sync-save: Clear all bytes that have not changed
	bool fChanged = false;
	if (!fSyncSave)
		for (int y = 0; y < Height; y++)
			for (int x = 0; x < Width; x++)
				if (pInitial[y * Width + x] == _GetPix(x, y))
					Surface8->SetPix(x, y, 0xff);
				else
					fChanged = true;

	if (fSyncSave || fChanged)
	{
		// Save landscape surface
		if (!Surface8->Save(Config.AtTempPath(C4CFN_TempLandscape)))
			return false;

		// Move temp file to group
		if (!hGroup.Move(Config.AtTempPath(C4CFN_TempLandscape),
			C4CFN_DiffLandscape))
			return false;
	}

	// Restore landscape pixels
	if (!fSyncSave)
		if (pInitial)
			for (int y = 0; y < Height; y++)
				for (int x = 0; x < Width; x++)
					if (_GetPix(x, y) == 0xff)
						Surface8->SetPix(x, y, pInitial[y * Width + x]);

	// Save changed map, too
	if (fMapChanged && Map)
		if (!SaveMap(hGroup)) return false;

	// and textures (if changed)
	if (!SaveTextures(hGroup)) return false;

	return true;
}

bool C4Landscape::SaveInitial()
{
	// Create array
	delete[] pInitial;
	pInitial = new uint8_t[Width * Height];

	// Save material data
	for (int y = 0; y < Height; y++)
		for (int x = 0; x < Width; x++)
			pInitial[y * Width + x] = _GetPix(x, y);

	return true;
}

bool C4Landscape::Load(C4Group &hGroup, bool fLoadSky, bool fSavegame)
{
	// Load exact landscape from group
	if (!hGroup.AccessEntry(C4CFN_Landscape)) return false;
	if (!(Surface8 = GroupReadSurfaceOwnPal8(hGroup))) return false;
	int iWidth, iHeight;
	Surface8->GetSurfaceSize(iWidth, iHeight);
	Width = iWidth; Height = iHeight;
	Surface32 = new C4Surface(Width, Height);
	if (Config.Graphics.ColorAnimation && Config.Graphics.Shader)
		AnimationSurface = new C4Surface(Width, Height);
	// adjust pal
	if (!Mat2Pal()) return false;
	// load the 32bit-surface, too
	size_t iSize;
	if (hGroup.AccessEntry(C4CFN_LandscapePNG, &iSize))
	{
		const std::unique_ptr<uint8_t []> pPNG(new uint8_t[iSize]);
		hGroup.Read(pPNG.get(), iSize);
		bool locked = false;
		try
		{
			CPNGFile png(pPNG.get(), iSize);
			StdBitmap bmp(png.Width(), png.Height(), png.UsesAlpha());
			png.Decode(bmp.GetBytes());
			if (!Surface32->Lock()) throw std::runtime_error("Could not lock surface");
			locked = true;
			for (int32_t y = 0; y < Height; ++y) for (int32_t x = 0; x < Width; ++x)
				Surface32->SetPixDw(x, y, bmp.GetPixel(x, y));
		}
		catch (const std::runtime_error &e)
		{
			LogNTr(spdlog::level::err, "Could not load 32 bit landscape surface from PNG file: {}", e.what());
		}
		if (locked) Surface32->Unlock();
		UpdateAnimationSurface({0, 0, Width, Height});
	}
	// no PNG: convert old-style landscapes
	else if (!Section.C4S.Landscape.NewStyleLandscape)
	{
		// convert all pixels
		for (int32_t y = 0; y < Height; ++y) for (int32_t x = 0; x < Width; ++x)
		{
			uint8_t byPix = Surface8->GetPix(x, y);
			int32_t iMat = Section.PixCol2MatOld(byPix); uint8_t byIFT = PixColIFTOld(byPix);
			if (byIFT) byIFT = IFT;
			// set pixel in 8bpp-surface only, so old-style landscapes won't be screwed up!
			Surface8->SetPix(x, y, Section.Mat2PixColDefault(iMat) + byIFT);
		}
		// NewStyleLandscape-flag will be set in C4Landscape::Init later
	}
	// New style landscape first generation: just correct
	if (Section.C4S.Landscape.NewStyleLandscape == 1)
	{
		// convert all pixels
		for (int32_t y = 0; y < Height; ++y) for (int32_t x = 0; x < Width; ++x)
		{
			// get material
			uint8_t byPix = Surface8->GetPix(x, y);
			int32_t iMat = Section.PixCol2MatOld2(byPix);
			if (Section.MatValid(iMat))
				// insert pixel
				Surface8->SetPix(x, y, Section.Mat2PixColDefault(iMat) + (byPix & IFT));
			else
				Surface8->SetPix(x, y, 0);
		}
	}
	else
	{
		// Landscape should be in correct format: Make sure it is!
		for (int32_t y = 0; y < Height; ++y) for (int32_t x = 0; x < Width; ++x)
		{
			uint8_t byPix = Surface8->GetPix(x, y);
			int32_t iMat = Section.PixCol2Mat(byPix);
			if (byPix && !Section.MatValid(iMat))
			{
				LogFatalNTr("Landscape loading error at ({}/{}): Pixel value {} not a valid material!", x, y, byPix);
				return false;
			}
		}
	}
	// Init sky
	if (fLoadSky)
	{
		Game.SetInitProgress(70);
		if (!Sky.Init(fSavegame)) return false;
	}
	// Success
	return true;
}

bool C4Landscape::ApplyDiff(C4Group &hGroup)
{
	CSurface8 *pDiff;
	// Load diff landscape from group
	if (!hGroup.AccessEntry(C4CFN_DiffLandscape)) return false;
	if (!(pDiff = GroupReadSurfaceOwnPal8(hGroup))) return false;
	// convert all pixels: keep if same material; re-set if different material
	uint8_t byPix;
	for (int32_t y = 0; y < Height; ++y) for (int32_t x = 0; x < Width; ++x)
		if (pDiff->GetPix(x, y) != 0xff)
			if (Surface8->GetPix(x, y) != (byPix = pDiff->GetPix(x, y)))
				// material has changed here: readjust with new texture
				SetPix(x, y, byPix);
	// done; clear diff
	delete pDiff;
	return true;
}

void C4Landscape::Default()
{
	Mode = C4LSC_Undefined;
	Surface8 = nullptr;
	Surface32 = nullptr;
	AnimationSurface = nullptr;
	Map = nullptr;
	Width = Height = 0;
	MapWidth = MapHeight = MapZoom = 0;
	ClearMatCount();
	ClearBlastMatCount();
	ScanX = 0;
	ScanSpeed = 2;
	LeftOpen = RightOpen = 0;
	TopOpen = BottomOpen = false;
	Gravity = FIXED100(20); // == 0.2
	MapSeed = 0; NoScan = false;
	pMapCreator = nullptr;
	Modulation = 0;
	fMapChanged = false;
	pInitial = nullptr;
	ShadeMaterials = true;
	PixCntPitch = 0;
	PixCnt = nullptr;
}

void C4Landscape::ClearBlastMatCount()
{
	for (int32_t cnt = 0; cnt < C4MaxMaterial; cnt++) BlastMatCount[cnt] = 0;
}

void C4Landscape::ClearMatCount()
{
	for (int32_t cnt = 0; cnt < C4MaxMaterial; cnt++) { MatCount[cnt] = 0; EffectiveMatCount[cnt] = 0; }
}

void C4Landscape::Synchronize()
{
	ScanX = 0;
	ClearBlastMatCount();
}

namespace
{
bool ForLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
	bool(C4Landscape::*fnCallback)(int32_t, int32_t, int32_t), C4Landscape &landscape, int32_t iPar = 0,
	int32_t *lastx = nullptr, int32_t *lasty = nullptr)
{
	int d, dx, dy, aincr, bincr, xincr, yincr, x, y;
	if (std::abs(x2 - x1) < std::abs(y2 - y1))
	{
		if (y1 > y2) { std::swap(x1, x2); std::swap(y1, y2); }
		xincr = (x2 > x1) ? 1 : -1;
		dy = y2 - y1; dx = std::abs(x2 - x1);
		d = 2 * dx - dy; aincr = 2 * (dx - dy); bincr = 2 * dx; x = x1; y = y1;
		if (!(landscape.*fnCallback)(x, y, iPar))
		{
			if (lastx) *lastx = x; if (lasty) *lasty = y;
			return false;
		}
		for (y = y1 + 1; y <= y2; ++y)
		{
			if (d >= 0) { x += xincr; d += aincr; }
			else d += bincr;
			if (!(landscape.*fnCallback)(x, y, iPar))
			{
				if (lastx) *lastx = x; if (lasty) *lasty = y;
				return false;
			}
		}
	}
	else
	{
		if (x1 > x2) { std::swap(x1, x2); std::swap(y1, y2); }
		yincr = (y2 > y1) ? 1 : -1;
		dx = x2 - x1; dy = std::abs(y2 - y1);
		d = 2 * dy - dx; aincr = 2 * (dy - dx); bincr = 2 * dy; x = x1; y = y1;
		if (!(landscape.*fnCallback)(x, y, iPar))
		{
			if (lastx) *lastx = x; if (lasty) *lasty = y;
			return false;
		}
		for (x = x1 + 1; x <= x2; ++x)
		{
			if (d >= 0) { y += yincr; d += aincr; }
			else d += bincr;
			if (!(landscape.*fnCallback)(x, y, iPar))
			{
				if (lastx) *lastx = x; if (lasty) *lasty = y;
				return false;
			}
		}
	}
	return true;
}
}

bool C4Landscape::AboveSemiSolid(int32_t &rx, int32_t &ry) // Nearest free above semi solid
{
	int32_t cy1 = ry, cy2 = ry;
	bool UseUpwardsNextFree = false, UseDownwardsNextSolid = false;

	while ((cy1 >= 0) || (cy2 < Height))
	{
		// Check upwards
		if (cy1 >= 0)
			if (GBackSemiSolid(rx, cy1)) UseUpwardsNextFree = true;
			else if (UseUpwardsNextFree) { ry = cy1; return true; }
			// Check downwards
			if (cy2 < Height)
				if (!GBackSemiSolid(rx, cy2)) UseDownwardsNextSolid = true;
				else if (UseDownwardsNextSolid) { ry = cy2; return true; }
				// Advance
				cy1--; cy2++;
	}

	return false;
}

bool C4Landscape::AboveSolid(int32_t &rx, int32_t &ry) // Nearest free directly above solid
{
	int32_t cy1 = ry, cy2 = ry;

	while ((cy1 >= 0) || (cy2 < Height))
	{
		// Check upwards
		if (cy1 >= 0)
			if (!GBackSemiSolid(rx, cy1))
				if (GBackSolid(rx, cy1 + 1))
				{
					ry = cy1; return true;
				}
		// Check downwards
		if (cy2 + 1 < Height)
			if (!GBackSemiSolid(rx, cy2))
				if (GBackSolid(rx, cy2 + 1))
				{
					ry = cy2; return true;
				}
		// Advance
		cy1--; cy2++;
	}

	return false;
}

bool C4Landscape::SemiAboveSolid(int32_t &rx, int32_t &ry) // Nearest free/semi above solid
{
	int32_t cy1 = ry, cy2 = ry;

	while ((cy1 >= 0) || (cy2 < Height))
	{
		// Check upwards
		if (cy1 >= 0)
			if (!GBackSolid(rx, cy1))
				if (GBackSolid(rx, cy1 + 1))
				{
					ry = cy1; return true;
				}
		// Check downwards
		if (cy2 + 1 < Height)
			if (!GBackSolid(rx, cy2))
				if (GBackSolid(rx, cy2 + 1))
				{
					ry = cy2; return true;
				}
		// Advance
		cy1--; cy2++;
	}

	return false;
}

bool C4Landscape::FindLiquidHeight(int32_t cx, int32_t &ry, int32_t hgt)
{
	int32_t cy1 = ry, cy2 = ry, rl1 = 0, rl2 = 0;

	while ((cy1 >= 0) || (cy2 < Height))
	{
		// Check upwards
		if (cy1 >= 0)
			if (GBackLiquid(cx, cy1))
			{
				rl1++; if (rl1 >= hgt) { ry = cy1 + hgt / 2; return true; }
			}
			else rl1 = 0;
			// Check downwards
			if (cy2 + 1 < Height)
				if (GBackLiquid(cx, cy2))
				{
					rl2++; if (rl2 >= hgt) { ry = cy2 - hgt / 2; return true; }
				}
				else rl2 = 0;
				// Advance
				cy1--; cy2++;
	}

	return false;
}

// Starting from rx/ry, searches for a width
// of solid ground. Returns bottom center
// of surface space found.

bool C4Landscape::FindSolidGround(int32_t &rx, int32_t &ry, int32_t width)
{
	bool fFound = false;

	int32_t cx1, cx2, cy1, cy2, rl1 = 0, rl2 = 0;

	for (cx1 = cx2 = rx, cy1 = cy2 = ry; (cx1 > 0) || (cx2 < Width); cx1--, cx2++)
	{
		// Left search
		if (cx1 >= 0) // Still going
		{
			if (AboveSolid(cx1, cy1)) rl1++; // Run okay
			else rl1 = 0; // No run
		}
		// Right search
		if (cx2 < Width) // Still going
		{
			if (AboveSolid(cx2, cy2)) rl2++; // Run okay
			else rl2 = 0; // No run
		}
		// Check runs
		if (rl1 >= width) { rx = cx1 + rl1 / 2; ry = cy1; fFound = true; break; }
		if (rl2 >= width) { rx = cx2 - rl2 / 2; ry = cy2; fFound = true; break; }
	}

	if (fFound) AboveSemiSolid(rx, ry);

	return fFound;
}

bool C4Landscape::FindSurfaceLiquid(int32_t &rx, int32_t &ry, int32_t width, int32_t height)
{
	bool fFound = false;

	int32_t cx1, cx2, cy1, cy2, rl1 = 0, rl2 = 0, cnt;
	bool lokay;
	for (cx1 = cx2 = rx, cy1 = cy2 = ry; (cx1 > 0) || (cx2 < Width); cx1--, cx2++)
	{
		// Left search
		if (cx1 > 0) // Still going
			if (!AboveSemiSolid(cx1, cy1)) cx1 = -1; // Abort left
			else
			{
				for (lokay = true, cnt = 0; cnt < height; cnt++) if (!GBackLiquid(cx1, cy1 + 1 + cnt)) lokay = false;
				if (lokay) rl1++; // Run okay
				else rl1 = 0; // No run
			}
		// Right search
		if (cx2 < Width) // Still going
			if (!AboveSemiSolid(cx2, cy2)) cx2 = Width; // Abort right
			else
			{
				for (lokay = true, cnt = 0; cnt < height; cnt++) if (!GBackLiquid(cx2, cy2 + 1 + cnt)) lokay = false;
				if (lokay) rl2++; // Run okay
				else rl2 = 0; // No run
			}
		// Check runs
		if (rl1 >= width) { rx = cx1 + rl1 / 2; ry = cy1; fFound = true; break; }
		if (rl2 >= width) { rx = cx2 - rl2 / 2; ry = cy2; fFound = true; break; }
	}

	if (fFound) AboveSemiSolid(rx, ry);

	return fFound;
}

bool C4Landscape::FindLiquid(int32_t &rx, int32_t &ry, int32_t width, int32_t height)
{
	int32_t cx1, cx2, cy1, cy2, rl1 = 0, rl2 = 0;

	for (cx1 = cx2 = rx, cy1 = cy2 = ry; (cx1 > 0) || (cx2 < Width); cx1--, cx2++)
	{
		// Left search
		if (cx1 > 0)
			if (FindLiquidHeight(cx1, cy1, height)) rl1++;
			else rl1 = 0;
			// Right search
			if (cx2 < Width)
				if (FindLiquidHeight(cx2, cy2, height)) rl2++;
				else rl2 = 0;
				// Check runs
				if (rl1 >= width) { rx = cx1 + rl1 / 2; ry = cy1; return true; }
				if (rl2 >= width) { rx = cx2 - rl2 / 2; ry = cy2; return true; }
	}

	return false;
}

// FindLevelGround: Starting from rx/ry, searches for a width
//                  of solid ground. Extreme distances may not
//                  exceed hrange.
//                  Returns bottom center of surface found.

bool C4Landscape::FindLevelGround(int32_t &rx, int32_t &ry, int32_t width, int32_t hrange)
{
	bool fFound = false;

	int32_t cx1, cx2, cy1, cy2, rh1, rh2, rl1, rl2;

	cx1 = cx2 = rx; cy1 = cy2 = ry;
	rh1 = cy1; rh2 = cy2;
	rl1 = rl2 = 0;

	for (cx1--, cx2++; (cx1 > 0) || (cx2 < Width); cx1--, cx2++)
	{
		// Left search
		if (cx1 > 0) // Still going
			if (!AboveSemiSolid(cx1, cy1)) cx1 = -1; // Abort left
			else if (GBackSolid(cx1, cy1 + 1) && (Abs(cy1 - rh1) < hrange))
				rl1++; // Run okay
			else
			{
				rl1 = 0; rh1 = cy1;
			} // No run

		// Right search
		if (cx2 < Width) // Still going
			if (!AboveSemiSolid(cx2, cy2)) cx2 = Width; // Abort right
			else if (GBackSolid(cx2, cy2 + 1) && (Abs(cy2 - rh2) < hrange))
				rl2++; // Run okay
			else
			{
				rl2 = 0; rh2 = cy2;
			} // No run

		// Check runs
		if (rl1 >= width) { rx = cx1 + rl1 / 2; ry = cy1; fFound = true; break; }
		if (rl2 >= width) { rx = cx2 - rl2 / 2; ry = cy2; fFound = true; break; }
	}

	if (fFound) AboveSemiSolid(rx, ry);

	return fFound;
}

// Starting from rx/ry, searches for a width of solid level
// ground with structure clearance (category).
// Returns bottom center of surface found.

bool C4Landscape::FindConSiteSpot(int32_t &rx, int32_t &ry, int32_t wdt, int32_t hgt,
	uint32_t category, int32_t hrange)
{
	bool fFound = false;

	// No hrange limit, use standard smooth surface limit
	if (hrange == -1) hrange = (std::max)(wdt / 4, 5);

	int32_t cx1, cx2, cy1, cy2, rh1, rh2, rl1, rl2;

	// Left offset starting position
	cx1 = (std::min)(rx + wdt / 2, Width - 1); cy1 = ry;
	// No good: use centered starting position
	if (!AboveSemiSolid(cx1, cy1)) { cx1 = std::min<int32_t>(rx, Width - 1); cy1 = ry; }
	// Right offset starting position
	cx2 = (std::max)(rx - wdt / 2, 0); cy2 = ry;
	// No good: use centered starting position
	if (!AboveSemiSolid(cx2, cy2)) { cx2 = std::min<int32_t>(rx, Width - 1); cy2 = ry; }

	rh1 = cy1; rh2 = cy2; rl1 = rl2 = 0;

	for (cx1--, cx2++; (cx1 > 0) || (cx2 < Width); cx1--, cx2++)
	{
		// Left search
		if (cx1 > 0) // Still going
			if (!AboveSemiSolid(cx1, cy1))
				cx1 = -1; // Abort left
			else if (GBackSolid(cx1, cy1 + 1) && (Abs(cy1 - rh1) < hrange))
				rl1++; // Run okay
			else
			{
				rl1 = 0; rh1 = cy1;
			} // No run

		// Right search
		if (cx2 < Width) // Still going
			if (!AboveSemiSolid(cx2, cy2))
				cx2 = Width; // Abort right
			else if (GBackSolid(cx2, cy2 + 1) && (Abs(cy2 - rh2) < hrange))
				rl2++; // Run okay
			else
			{
				rl2 = 0; rh2 = cy2;
			} // No run

		// Check runs & object overlap
		if (rl1 >= wdt) if (cx1 > 0)
			if (!Section.OverlapObject(cx1, cy1 - hgt - 10, wdt, hgt + 40, category))
			{
				rx = cx1 + wdt / 2; ry = cy1; fFound = true; break;
			}
		if (rl2 >= wdt) if (cx2 < Width)
			if (!Section.OverlapObject(cx2 - wdt, cy2 - hgt - 10, wdt, hgt + 40, category))
			{
				rx = cx2 - wdt / 2; ry = cy2; fFound = true; break;
			}
	}

	if (fFound) AboveSemiSolid(rx, ry);

	return fFound;
}

// Returns false on any solid pix in path.

bool C4Landscape::PathFreePix(int32_t x, int32_t y, int32_t par)
{
	return !GBackSolid(x, y);
}

bool C4Landscape::PathFree(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t *ix, int32_t *iy)
{
	return ForLine(x1, y1, x2, y2, &C4Landscape::PathFreePix, *this, 0, ix, iy);
}

bool C4Landscape::PathFreeIgnoreVehiclePix(int32_t x, int32_t y, int32_t par)
{
	uint8_t byPix = GetPix(x, y);
	return !byPix || !DensitySolid(GetPixMat(byPix)) || GetPixMat(byPix) == MVehic;
}

bool C4Landscape::PathFreeIgnoreVehicle(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t *ix, int32_t *iy)
{
	return ForLine(x1, y1, x2, y2, &C4Landscape::PathFreeIgnoreVehiclePix, *this, 0, ix, iy);
}

int32_t C4Landscape::TrajectoryDistance(int32_t iFx, int32_t iFy, C4Fixed iXDir, C4Fixed iYDir, int32_t iTx, int32_t iTy)
{
	int32_t iClosest = Distance(iFx, iFy, iTx, iTy);
	// Follow free trajectory, take closest point distance
	C4Fixed cx = itofix(iFx), cy = itofix(iFy);
	int32_t cdis;
	while (Inside(fixtoi(cx), 0, Width - 1) && Inside(fixtoi(cy), 0, Height - 1) && !GBackSolid(fixtoi(cx), fixtoi(cy)))
	{
		cdis = Distance(fixtoi(cx), fixtoi(cy), iTx, iTy);
		if (cdis < iClosest) iClosest = cdis;
		cx += iXDir; cy += iYDir; iYDir += Gravity;
	}
	return iClosest;
}

const int32_t C4LSC_Throwing_MaxVertical   = 50,
              C4LSC_Throwing_MaxHorizontal = 60;

bool C4Landscape::FindThrowingPosition(int32_t iTx, int32_t iTy, C4Fixed fXDir, C4Fixed fYDir, int32_t iHeight, int32_t &rX, int32_t &rY)
{
	// Start underneath throwing target
	rX = iTx; rY = iTy; // improve: check from overhanging cliff
	if (!SemiAboveSolid(rX, rY)) return false;

	// Target too far above surface
	if (!Inside(rY - iTy, -C4LSC_Throwing_MaxVertical, +C4LSC_Throwing_MaxVertical)) return false;

	// Search in direction according to launch fXDir
	int32_t iDir = +1; if (fXDir > 0) iDir = -1;

	// Move along surface
	for (int32_t cnt = 0; Inside<int32_t>(rX, 0, Width - 1) && (cnt <= C4LSC_Throwing_MaxHorizontal); rX += iDir, cnt++)
	{
		// Adjust to surface
		if (!SemiAboveSolid(rX, rY)) return false;

		// Check trajectory distance
		int32_t itjd = TrajectoryDistance(rX, rY - iHeight, fXDir, fYDir, iTx, iTy);

		// Hitting range: success
		if (itjd <= 2) return true;
	}

	// Failure
	return false;
}

const int32_t C4LSC_Closest_MaxRange = 200,
              C4LSC_Closest_Step     = 10;

bool C4Landscape::FindClosestFree(int32_t &rX, int32_t &rY, int32_t iAngle1, int32_t iAngle2,
	int32_t iExcludeAngle1, int32_t iExcludeAngle2)
{
	int32_t iX, iY;
	for (int32_t iR = C4LSC_Closest_Step; iR < C4LSC_Closest_MaxRange; iR += C4LSC_Closest_Step)
		for (int32_t iAngle = iAngle1; iAngle < iAngle2; iAngle += C4LSC_Closest_Step)
			if (!Inside(iAngle, iExcludeAngle1, iExcludeAngle2))
			{
				iX = rX + fixtoi(Sin(itofix(iAngle)) * iR);
				iY = rY - fixtoi(Cos(itofix(iAngle)) * iR);
				if (Inside<int32_t>(iX, 0, Width - 1))
					if (Inside<int32_t>(iY, 0, Height - 1))
						if (!GBackSemiSolid(iX, iY))
						{
							rX = iX; rY = iY; return true;
						}
			}
	return false;
}

bool C4Landscape::ConstructionCheck(C4ID id, int32_t iX, int32_t iY, C4Object *pByObj)
{
	C4Def *ndef;
	char idostr[5];

	// Check def
	if (!(ndef = Game.Defs.ID2Def(id)))
	{
		GetC4IdText(id, idostr);
		if (pByObj) GameMsgObject(LoadResStr(C4ResStrTableKey::IDS_OBJ_UNDEF, idostr).c_str(), pByObj, FRed);
		return false;
	}

	// Constructable?
	if (!ndef->Constructable)
	{
		if (pByObj) GameMsgObject(LoadResStr(C4ResStrTableKey::IDS_OBJ_NOCON, ndef->GetName()).c_str(), pByObj, FRed);
		return false;
	}

	// Check area
	int32_t rtx, rty, wdt, hgt;
	wdt = ndef->Shape.Wdt; hgt = ndef->Shape.Hgt - ndef->ConSizeOff;
	rtx = iX - wdt / 2; rty = iY - hgt;
	if (AreaSolidCount(rtx, rty, wdt, hgt) > (wdt * hgt / 20))
	{
		if (pByObj) GameMsgObject(LoadResStr(C4ResStrTableKey::IDS_OBJ_NOROOM), pByObj, FRed);
		return false;
	}
	if (AreaSolidCount(rtx, rty + hgt, wdt, 5) < (wdt * 2))
	{
		if (pByObj) GameMsgObject(LoadResStr(C4ResStrTableKey::IDS_OBJ_NOLEVEL), pByObj, FRed);
		return false;
	}

	// Check other structures
	C4Object *other;
	if (other = Section.OverlapObject(rtx, rty, wdt, hgt, ndef->Category))
	{
		if (pByObj) GameMsgObject(LoadResStr(C4ResStrTableKey::IDS_OBJ_NOOTHER, other->GetName()).c_str(), pByObj, FRed);
		return false;
	}

	return true;
}

void C4Landscape::ClearRect(int32_t iTx, int32_t iTy, int32_t iWdt, int32_t iHgt)
{
	C4Rect rt(iTx, iTy, iWdt, iHgt);
	PrepareChange(rt, false);
	for (int32_t y = iTy; y < iTy + iHgt; y++)
	{
		for (int32_t x = iTx; x < iTx + iWdt; x++) ClearPix(x, y);
		if (Rnd3()) Rnd3();
	}
	FinishChange(rt, false);
}

void C4Landscape::ClearRectDensity(int32_t iTx, int32_t iTy, int32_t iWdt, int32_t iHgt, int32_t iOfDensity)
{
	int32_t iMinDensity = iOfDensity, iMaxDensity = iOfDensity;
	switch (iOfDensity)
	{
	case C4M_Vehicle: iMaxDensity = 1000; break;
	case C4M_Solid: iMaxDensity = C4M_Vehicle - 1; break;
	case C4M_Liquid: iMaxDensity = C4M_Solid - 1; break;
	case C4M_Background: iMaxDensity = C4M_Liquid - 1; break;
	default: break; // min=max as given
	}

	for (int32_t y = iTy; y < iTy + iHgt; y++)
	{
		for (int32_t x = iTx; x < iTx + iWdt; x++)
		{
			if (Inside(GetDensity(x, y), iMinDensity, iMaxDensity))
				ClearPix(x, y);
		}
		if (Rnd3()) Rnd3();
	}
}

bool C4Landscape::SaveMap(C4Group &hGroup)
{
	// No map
	if (!Map) return false;

	// Create map palette
	uint8_t bypPalette[3 * 256];
	Section.TextureMap.StoreMapPalette(bypPalette, Section.Material);

	// Save map surface
	if (!Map->Save(Config.AtTempPath(C4CFN_TempMap), bypPalette))
		return false;

	// Move temp file to group
	if (!hGroup.Move(Config.AtTempPath(C4CFN_TempMap),
		C4CFN_Map))
		return false;

	// Success
	return true;
}

bool C4Landscape::SaveTextures(C4Group &hGroup)
{
	// if material-texture-combinations have been added, write the texture map
	if (Section.TextureMap.fEntriesAdded)
	{
		C4Group *pMatGroup = new C4Group();
		bool fSuccess = false;
		// create local material group
		if (!hGroup.FindEntry(C4CFN_Material))
		{
			// delete previous item at temp path
			EraseItem(Config.AtTempPath(C4CFN_Material));
			// create at temp path
			if (pMatGroup->Open(Config.AtTempPath(C4CFN_Material), true))
				// write to it
				if (Section.TextureMap.SaveMap(*pMatGroup, C4CFN_TexMap))
					// close (flush)
					if (pMatGroup->Close())
						// add it
						if (hGroup.Move(Config.AtTempPath(C4CFN_Material), C4CFN_Material))
							fSuccess = true;
			// temp group must remain for scenario file closure
			// it will be deleted when the group is closed
		}
		else
			// simply write it to the local material file
			if (pMatGroup->OpenAsChild(&hGroup, C4CFN_Material))
				fSuccess = Section.TextureMap.SaveMap(*pMatGroup, C4CFN_TexMap);
		// close material group again
		if (pMatGroup->IsOpen()) pMatGroup->Close();
		delete pMatGroup;
		// fail if unsuccessful
		if (!fSuccess) return false;
	}
	// done, success
	return true;
}

bool C4Landscape::SetMode(int32_t iMode)
{
	// Invalid mode
	if (!Inside<int32_t>(iMode, C4LSC_Dynamic, C4LSC_Exact)) return false;
	// Set mode
	Mode = iMode;
	// Done
	return true;
}

bool C4Landscape::MapToLandscape()
{
	// zoom map to landscape
	return MapToLandscape(Map, 0, 0, MapWidth, MapHeight);
}

bool C4Landscape::GetMapColorIndex(const char *szMaterial, const char *szTexture, bool fIFT, uint8_t &rbyCol)
{
	// Sky
	if (SEqual(szMaterial, C4TLS_MatSky))
		rbyCol = 0;
	// Material-Texture
	else
	{
		if (!(rbyCol = Section.TextureMap.GetIndex(szMaterial, szTexture))) return false;
		if (fIFT) rbyCol += IFT;
	}
	// Found
	return true;
}

bool C4Landscape::DrawBrush(int32_t iX, int32_t iY, int32_t iGrade, const char *szMaterial, const char *szTexture, bool fIFT)
{
	uint8_t byCol;
	switch (Mode)
	{
	// Dynamic: ignore
	case C4LSC_Dynamic:
		break;
	// Static: draw to map by material-texture-index, chunk-o-zoom to landscape
	case C4LSC_Static:
		// Get map color index by material-texture
		if (!GetMapColorIndex(szMaterial, szTexture, fIFT, byCol)) return false;
		// Draw to map
		int32_t iRadius; iRadius = std::max<int32_t>(2 * iGrade / MapZoom, 1);
		if (iRadius == 1) { if (Map) Map->SetPix(iX / MapZoom, iY / MapZoom, byCol); }
		else Map->Circle(iX / MapZoom, iY / MapZoom, iRadius, byCol);
		// Update landscape
		MapToLandscape(Map, iX / MapZoom - iRadius - 1, iY / MapZoom - iRadius - 1, 2 * iRadius + 2, 2 * iRadius + 2);
		SetMapChanged();
		break;
	// Exact: draw directly to landscape by color & pattern
	case C4LSC_Exact:
		// Set texture pattern & get material color
		if (!GetMapColorIndex(szMaterial, szTexture, fIFT, byCol)) return false;
		C4Rect BoundingBox(iX - iGrade - 1, iY - iGrade - 1, iGrade * 2 + 2, iGrade * 2 + 2);
		// Draw to landscape
		PrepareChange(BoundingBox);
		Surface8->Circle(iX, iY, iGrade, byCol);
		FinishChange(BoundingBox);
		break;
	}
	return true;
}

uint8_t DrawLineCol;

bool C4Landscape::DrawLineLandscape(int32_t iX, int32_t iY, int32_t iGrade)
{
	Surface8->Circle(iX, iY, iGrade, DrawLineCol);
	return true;
}

bool C4Landscape::DrawLineMap(int32_t iX, int32_t iY, int32_t iRadius)
{
	if (iRadius == 1) { if (Map) Map->SetPix(iX, iY, DrawLineCol); }
	else Map->Circle(iX, iY, iRadius, DrawLineCol);
	return true;
}

bool C4Landscape::DrawLine(int32_t iX1, int32_t iY1, int32_t iX2, int32_t iY2, int32_t iGrade, const char *szMaterial, const char *szTexture, bool fIFT)
{
	switch (Mode)
	{
	// Dynamic: ignore
	case C4LSC_Dynamic:
		break;
	// Static: draw to map by material-texture-index, chunk-o-zoom to landscape
	case C4LSC_Static:
		// Get map color index by material-texture
		if (!GetMapColorIndex(szMaterial, szTexture, fIFT, DrawLineCol)) return false;
		// Draw to map
		int32_t iRadius; iRadius = std::max<int32_t>(2 * iGrade / MapZoom, 1);
		iX1 /= MapZoom; iY1 /= MapZoom; iX2 /= MapZoom; iY2 /= MapZoom;
		ForLine(iX1, iY1, iX2, iY2, &C4Landscape::DrawLineMap, *this, iRadius);
		// Update landscape
		int32_t iUpX, iUpY, iUpWdt, iUpHgt;
		iUpX = (std::min)(iX1, iX2) - iRadius - 1; iUpY = (std::min)(iY1, iY2) - iRadius - 1;
		iUpWdt = Abs(iX2 - iX1) + 2 * iRadius + 2; iUpHgt = Abs(iY2 - iY1) + 2 * iRadius + 2;
		MapToLandscape(Map, iUpX, iUpY, iUpWdt, iUpHgt);
		SetMapChanged();
		break;
	// Exact: draw directly to landscape by color & pattern
	case C4LSC_Exact:
		// Set texture pattern & get material color
		if (!GetMapColorIndex(szMaterial, szTexture, fIFT, DrawLineCol)) return false;
		C4Rect BoundingBox(iX1 - iGrade, iY1 - iGrade, iGrade * 2 + 1, iGrade * 2 + 1);
		BoundingBox.Add(C4Rect(iX2 - iGrade, iY2 - iGrade, iGrade * 2 + 1, iGrade * 2 + 1));
		// Draw to landscape
		PrepareChange(BoundingBox);
		ForLine(iX1, iY1, iX2, iY2, &C4Landscape::DrawLineLandscape, *this, iGrade);
		FinishChange(BoundingBox);
		break;
	}
	return true;
}

bool C4Landscape::DrawBox(int32_t iX1, int32_t iY1, int32_t iX2, int32_t iY2, int32_t iGrade, const char *szMaterial, const char *szTexture, bool fIFT)
{
	// get upper-left/lower-right - corners
	int32_t iX0 = (std::min)(iX1, iX2); int32_t iY0 = (std::min)(iY1, iY2);
	iX2 = (std::max)(iX1, iX2); iY2 = (std::max)(iY1, iY2); iX1 = iX0; iY1 = iY0;
	uint8_t byCol;
	switch (Mode)
	{
	// Dynamic: ignore
	case C4LSC_Dynamic:
		break;
	// Static: draw to map by material-texture-index, chunk-o-zoom to landscape
	case C4LSC_Static:
		// Get map color index by material-texture
		if (!GetMapColorIndex(szMaterial, szTexture, fIFT, byCol)) return false;
		// Draw to map
		iX1 /= MapZoom; iY1 /= MapZoom; iX2 /= MapZoom; iY2 /= MapZoom;
		Map->Box(iX1, iY1, iX2, iY2, byCol);
		// Update landscape
		MapToLandscape(Map, iX1 - 1, iY1 - 1, iX2 - iX1 + 3, iY2 - iY1 + 3);
		SetMapChanged();
		break;
	// Exact: draw directly to landscape by color & pattern
	case C4LSC_Exact:
		// Set texture pattern & get material color
		if (!GetMapColorIndex(szMaterial, szTexture, fIFT, byCol)) return false;
		C4Rect BoundingBox(iX1, iY1, iX2 - iX1 + 1, iY2 - iY1 + 1);
		// Draw to landscape
		PrepareChange(BoundingBox);
		Surface8->Box(iX1, iY1, iX2, iY2, byCol);
		FinishChange(BoundingBox);
		break;
	}
	return true;
}

bool C4Landscape::DrawChunks(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt, int32_t icntx, int32_t icnty, const char *szMaterial, const char *szTexture, bool bIFT)
{
	uint8_t byColor;
	if (!GetMapColorIndex(szMaterial, szTexture, bIFT, byColor)) return false;

	int32_t iMaterial = Section.Material.Get(szMaterial); if (!Section.MatValid(iMaterial)) return false;

	C4Rect BoundingBox(tx - 5, ty - 5, wdt + 10, hgt + 10);
	PrepareChange(BoundingBox);

	// assign clipper
	Surface8->Clip(BoundingBox.x, BoundingBox.y, BoundingBox.x + BoundingBox.Wdt, BoundingBox.y + BoundingBox.Hgt);
	Application.DDraw->NoPrimaryClipper();

	// draw all chunks
	int32_t x, y;
	for (x = 0; x < icntx; x++)
		for (y = 0; y < icnty; y++)
			DrawChunk(tx + wdt * x / icntx, ty + hgt * y / icnty, wdt / icntx, hgt / icnty, byColor, Section.Material.Map[iMaterial].MapChunkType, Random(1000));

	// remove clipper
	Surface8->NoClip();

	FinishChange(BoundingBox);

	// success
	return true;
}

bool C4Landscape::DrawQuad(int32_t iX1, int32_t iY1, int32_t iX2, int32_t iY2, int32_t iX3, int32_t iY3, int32_t iX4, int32_t iY4, const char *szMaterial, bool fIFT)
{
	// get texture
	int32_t iMatTex = Section.TextureMap.GetIndexMatTex(szMaterial);
	if (!iMatTex) return false;
	// prepate pixel count update
	C4Rect BoundingBox(iX1, iY1, 1, 1);
	BoundingBox.Add(C4Rect(iX2, iY2, 1, 1));
	BoundingBox.Add(C4Rect(iX3, iY3, 1, 1));
	BoundingBox.Add(C4Rect(iX4, iY4, 1, 1));
	// set vertices
	int vtcs[8];
	vtcs[0] = iX1; vtcs[1] = iY1;
	vtcs[2] = iX2; vtcs[3] = iY2;
	vtcs[4] = iX3; vtcs[5] = iY3;
	vtcs[6] = iX4; vtcs[7] = iY4;
	// draw quad
	PrepareChange(BoundingBox);
	Surface8->Polygon(4, vtcs, Section.MatTex2PixCol(iMatTex) + (fIFT ? IFT : 0));
	FinishChange(BoundingBox);
	return true;
}

uint8_t C4Landscape::GetMapIndex(int32_t iX, int32_t iY)
{
	if (!Map) return 0;
	return Map->GetPix(iX, iY);
}

bool C4Landscape::DoRelights()
{
	if (!Relights[0].Wdt) return true;

	if (!Surface32->Lock()) return false;
	if (AnimationSurface)
	{
		AnimationSurface->Lock();
	}

	for (int32_t i = 0; i < C4LS_MaxRelights; i++)
	{
		if (!Relights[i].Wdt)
			break;
		C4Rect SolidMaskRect = Relights[i];
		SolidMaskRect.x -= 2 * C4LS_MaxLightDistX; SolidMaskRect.y -= 2 * C4LS_MaxLightDistY;
		SolidMaskRect.Wdt += 4 * C4LS_MaxLightDistX; SolidMaskRect.Hgt += 4 * C4LS_MaxLightDistY;
		C4SolidMask *pSolid;
		for (pSolid = C4SolidMask::Last; pSolid; pSolid = pSolid->Prev)
		{
			pSolid->RemoveTemporary(SolidMaskRect);
		}
		Relight(Relights[i]);
		// Restore Solidmasks
		for (pSolid = C4SolidMask::First; pSolid; pSolid = pSolid->Next)
		{
			pSolid->PutTemporary(SolidMaskRect);
		}
		Relights[i].Default();
		C4SolidMask::CheckConsistency();
	}

	Surface32->Unlock();
	if (AnimationSurface) AnimationSurface->Unlock();

	return true;
}

bool C4Landscape::Relight(C4Rect To)
{
	// Enlarge to relight pixels surrounding a changed one
	To.x -= C4LS_MaxLightDistX; To.y -= C4LS_MaxLightDistY;
	To.Wdt += 2 * C4LS_MaxLightDistX; To.Hgt += 2 * C4LS_MaxLightDistY;
	// Apply lighting
	return ApplyLighting(To);
}

bool C4Landscape::ApplyLighting(C4Rect To)
{
	// clip to landscape size
	To.Intersect(C4Rect(0, 0, Width, Height));
	// everything clipped?
	if (To.Wdt <= 0 || To.Hgt <= 0) return true;

	if (!Surface32->LockForUpdate(To)) return false;
	Surface32->ClearBoxDw(To.x, To.y, To.Wdt, To.Hgt);
	// do lightning
	for (int32_t iX = To.x; iX < To.x + To.Wdt; ++iX)
	{
		int AboveDensity = 0, BelowDensity = 0;
		if (ShadeMaterials)
		{
			for (int i = 1; i <= 8; ++i)
			{
				AboveDensity += GetPlacement(iX, To.y - i - 1);
				BelowDensity += GetPlacement(iX, To.y + i - 1);
			}
		}

		for (int32_t iY = To.y; iY < To.y + To.Hgt; ++iY)
		{
			// do not move that code into the if (ShadeMaterials) block, as it needs to be run for sky pixels as well
			AboveDensity -= GetPlacement(iX, iY - 9);
			AboveDensity += GetPlacement(iX, iY - 1);
			BelowDensity -= GetPlacement(iX, iY);
			BelowDensity += GetPlacement(iX, iY + 8);

			// Normal color
			uint32_t dwBackClr = GetClrByTex(iX, iY);

			uint8_t pix = _GetPix(iX, iY);
			// Sky
			if (!pix)
			{
				Surface32->SetPixDw(iX, iY, dwBackClr);
				continue;
			}

			if (ShadeMaterials)
			{
				// get density
				int iOwnDens = Pix2Place[pix];
				if (!iOwnDens) continue;
				iOwnDens *= 2;
				iOwnDens += GetPlacement(iX + 1, iY) + GetPlacement(iX - 1, iY);
				iOwnDens /= 4;
				// get density of surrounding materials
				int iCompareDens = AboveDensity / 8;
				if (iOwnDens > iCompareDens)
				{
					// apply light
					LightenClrBy(dwBackClr, (std::min)(30, 2 * (iOwnDens - iCompareDens)));
				}
				else if (iOwnDens < iCompareDens && iOwnDens < 30)
				{
					DarkenClrBy(dwBackClr, (std::min)(30, 2 * (iCompareDens - iOwnDens)));
				}
				iCompareDens = BelowDensity / 8;
				if (iOwnDens > iCompareDens)
				{
					DarkenClrBy(dwBackClr, (std::min)(30, 2 * (iOwnDens - iCompareDens)));
				}
			}

			Surface32->SetPixDw(iX, iY, dwBackClr);
		}
	}
	Surface32->Unlock();

	return UpdateAnimationSurface(To);
}

bool C4Landscape::UpdateAnimationSurface(C4Rect To)
{
	if (!AnimationSurface) return true;

	if (!AnimationSurface->LockForUpdate(To)) return false;

	AnimationSurface->ClearBoxDw(To.x, To.y, To.Wdt, To.Hgt);

	for (int32_t iX = To.x; iX < To.x + To.Wdt; ++iX)
	{
		for (int32_t iY = To.y; iY < To.y + To.Hgt; ++iY)
		{
			AnimationSurface->SetPixDw(iX, iY, DensityLiquid(Pix2Dens[_GetPix(iX, iY)]) ? 255 << 24 : 0);
		}
	}

	AnimationSurface->Unlock();
	return true;
}

uint32_t C4Landscape::GetClrByTex(int32_t iX, int32_t iY)
{
	// Get pixel and default color
	uint8_t pix = _GetPix(iX, iY);
	uint32_t dwPix = Surface8->pPal->GetClr(pix);
	// get texture map entry for pixel
	const C4TexMapEntry *pTex;
	if (pix && (pTex = Section.TextureMap.GetEntry(Section.PixCol2Tex(pix))))
	{
		// pattern color
		pTex->getPattern().PatternClr(iX, iY, pix, dwPix, Application.DDraw->Pal);
		if (pTex->GetMaterial())
			pTex->GetMaterial()->MatPattern.PatternClr(iX, iY, pix, dwPix, Application.DDraw->Pal);
	}
	return dwPix;
}

bool C4Landscape::DrawMap(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, const char *szMapDef)
{
	// safety
	if (!szMapDef) return false;
	// clip to landscape size
	if (!ClipRect(iX, iY, iWdt, iHgt)) return false;
	// get needed map size
	int32_t iMapWdt = (iWdt - 1) / MapZoom + 1;
	int32_t iMapHgt = (iHgt - 1) / MapZoom + 1;
	C4SLandscape FakeLS = Section.C4S.Landscape;
	FakeLS.MapWdt.Set(iMapWdt, 0, iMapWdt, iMapWdt);
	FakeLS.MapHgt.Set(iMapHgt, 0, iMapHgt, iMapHgt);
	// create map creator
	C4MapCreatorS2 MapCreator(&FakeLS, &Section.TextureMap, &Section.Material, Game.Parameters.StartupPlayerCount);
	// read file
	MapCreator.ReadScript(szMapDef);
	// render map
	CSurface8 *sfcMap = MapCreator.Render(nullptr);
	if (!sfcMap) return false;
	// map it to the landscape
	bool fSuccess = MapToLandscape(sfcMap, 0, 0, iMapWdt, iMapHgt, iX, iY);
	// cleanup
	delete sfcMap;
	// return whether successful
	return fSuccess;
}

bool C4Landscape::DrawDefMap(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, const char *szMapDef)
{
	// safety
	if (!szMapDef || !pMapCreator) return false;
	// clip to landscape size
	if (!ClipRect(iX, iY, iWdt, iHgt)) return false;
	// get needed map size
	int32_t iMapWdt = (iWdt - 1) / MapZoom + 1;
	int32_t iMapHgt = (iHgt - 1) / MapZoom + 1;
	bool fSuccess = false;
	// render map
	C4MCMap *pMap = pMapCreator->GetMap(szMapDef);
	if (!pMap) return false;
	pMap->SetSize(iMapWdt, iMapHgt);
	CSurface8 *sfcMap = pMapCreator->Render(szMapDef);
	if (sfcMap)
	{
		// map to landscape
		fSuccess = MapToLandscape(sfcMap, 0, 0, iMapWdt, iMapHgt, iX, iY);
	}
	// cleanup
	delete sfcMap;
	// done
	return fSuccess;
}

bool C4Landscape::ClipRect(int32_t &rX, int32_t &rY, int32_t &rWdt, int32_t &rHgt)
{
	// clip by bounds
	if (rX < 0) { rWdt += rX; rX = 0; }
	if (rY < 0) { rHgt += rY; rY = 0; }
	int32_t iOver;
	iOver = rX + rWdt - Width; if (iOver > 0) { rWdt -= iOver; }
	iOver = rY + rHgt - Height; if (iOver > 0) { rHgt -= iOver; }
	// anything left inside the bounds?
	return rWdt > 0 && rHgt > 0;
}

bool C4Landscape::ReplaceMapColor(uint8_t iOldIndex, uint8_t iNewIndex)
{
	// find every occurance of iOldIndex in map; replace it by new index
	if (!Map) return false;
	int iPitch, iMapWdt, iMapHgt;
	uint8_t *pMap = Map->Bits;
	iMapWdt = Map->Wdt;
	iMapHgt = Map->Hgt;
	iPitch = Map->Pitch;
	if (!pMap) return false;
	for (int32_t y = 0; y < iMapHgt; ++y)
	{
		for (int32_t x = 0; x < iMapWdt; ++x)
		{
			if ((*pMap & 0x7f) == iOldIndex)
				*pMap = (*pMap & 0x80) + iNewIndex;
			++pMap;
		}
		pMap += iPitch - iMapWdt;
	}
	return true;
}

bool C4Landscape::SetTextureIndex(const char *szMatTex, uint8_t iNewIndex, bool fInsert)
{
	if (((!szMatTex || !*szMatTex) && !fInsert) || !Inside<int>(iNewIndex, 0x01, 0x7f))
	{
		DebugLog(spdlog::level::err, "Cannot insert new texture {} to index {}: Invalid parameters.", szMatTex, static_cast<int>(iNewIndex));
		return false;
	}
	// get last mat index - returns zero for not found (valid for insertion mode)
	StdStrBuf Material, Texture;
	Material.CopyUntil(szMatTex, '-'); Texture.Copy(SSearch(szMatTex, "-"));
	uint8_t iOldIndex = (szMatTex && *szMatTex) ? Section.TextureMap.GetIndex(Material.getData(), Texture.getData(), false) : 0;
	// insertion mode?
	if (fInsert)
	{
		// there must be room to move up to
		uint8_t byLastMoveIndex = C4M_MaxTexIndex - 1;
		while (Section.TextureMap.GetEntry(byLastMoveIndex))
			if (--byLastMoveIndex == iNewIndex)
			{
				DebugLog(spdlog::level::err, "Cannot insert new texture {} to index {}: No room for insertion.", szMatTex, static_cast<int>(iNewIndex));
				return false;
			}
		// then move up all other textures first
		// could do this in one loop, but it's just a developement call anyway, so move one index at a time
		while (--byLastMoveIndex >= iNewIndex)
			if (Section.TextureMap.GetEntry(byLastMoveIndex))
			{
				ReplaceMapColor(byLastMoveIndex, byLastMoveIndex + 1);
				Section.TextureMap.MoveIndex(byLastMoveIndex, byLastMoveIndex + 1);
			}
		// new insertion desired?
		if (szMatTex && *szMatTex)
		{
			// move from old or create new
			if (iOldIndex)
			{
				ReplaceMapColor(iOldIndex, iNewIndex);
				Section.TextureMap.MoveIndex(iOldIndex, iNewIndex);
			}
			else
			{
				StdStrBuf Material, Texture;
				Material.CopyUntil(szMatTex, '-'); Texture.Copy(SSearch(szMatTex, "-"));
				// new insertion
				if (!Section.TextureMap.AddEntry(iNewIndex, Material.getData(), Texture.getData()))
				{
					LogNTr(spdlog::level::err, "Cannot insert new texture {} to index {}: Texture map entry error", szMatTex, iNewIndex);
					return false;
				}
			}
		}
		// done, success
		return true;
	}
	else
	{
		// new index must not be occupied
		const C4TexMapEntry *pOld;
		if ((pOld = Section.TextureMap.GetEntry(iNewIndex)) && !pOld->isNull())
		{
			DebugLog(spdlog::level::err, "Cannot move texture {} to index {}: Index occupied by {}-{}.", szMatTex, static_cast<int>(iNewIndex), pOld->GetMaterialName(), pOld->GetTextureName());
			return false;
		}
		// must only move existing textures
		if (!iOldIndex)
		{
			DebugLog(spdlog::level::err, "Cannot move texture {} to index {}: Texture not found.", szMatTex, iNewIndex);
			return false;
		}
		// update map
		ReplaceMapColor(iOldIndex, iNewIndex);
		// change to new index in texmap
		Section.TextureMap.MoveIndex(iOldIndex, iNewIndex);
		// done, success
		return true;
	}
}

void C4Landscape::HandleTexMapUpdate()
{
	// Pixel maps must be update
	UpdatePixMaps();
	// Update landscape palette
	Mat2Pal();
}

void C4Landscape::UpdatePixMaps()
{
	int32_t i;
	for (i = 0; i < 256; i++) Pix2Mat[i] = Section.PixCol2Mat(i);
	for (i = 0; i < 256; i++) Pix2Dens[i] = Section.MatDensity(Pix2Mat[i]);
	for (i = 0; i < 256; i++) Pix2Place[i] = Section.MatValid(Pix2Mat[i]) ? Section.Material.Map[Pix2Mat[i]].Placement : 0;
	Pix2Place[0] = 0;
}

bool C4Landscape::Mat2Pal()
{
	if (!Surface8) return false;
	// set landscape pal
	int32_t tex, rgb;
	for (tex = 0; tex < C4M_MaxTexIndex; tex++)
	{
		const C4TexMapEntry *pTex = Section.TextureMap.GetEntry(tex);
		if (!pTex || pTex->isNull())
			continue;
		// colors
		for (rgb = 0; rgb < 3; rgb++)
			Surface8->pPal->Colors[Section.MatTex2PixCol(tex) * 3 + rgb]
			= Surface8->pPal->Colors[(Section.MatTex2PixCol(tex) + IFT) * 3 + rgb]
			= pTex->GetMaterial()->Color[rgb];
		// alpha
		Surface8->pPal->Alpha[Section.MatTex2PixCol(tex)] = pTex->GetMaterial()->Alpha[0];
		Surface8->pPal->Alpha[Section.MatTex2PixCol(tex) + IFT] = pTex->GetMaterial()->Alpha[C4M_ColsPerMat];
	}
	// success
	return true;
}

void C4Landscape::PrepareChange(C4Rect BoundingBox, const bool updateMatCnt)
{
	// move solidmasks out of the way
	C4Rect SolidMaskRect = BoundingBox;
	SolidMaskRect.x -= 2 * C4LS_MaxLightDistX; SolidMaskRect.y -= 2 * C4LS_MaxLightDistY;
	SolidMaskRect.Wdt += 4 * C4LS_MaxLightDistX; SolidMaskRect.Hgt += 4 * C4LS_MaxLightDistY;
	for (C4SolidMask *pSolid = C4SolidMask::Last; pSolid; pSolid = pSolid->Prev)
	{
		pSolid->RemoveTemporary(SolidMaskRect);
	}
	if (updateMatCnt) UpdateMatCnt(BoundingBox, false);
}

void C4Landscape::FinishChange(C4Rect BoundingBox, const bool updateMatAndPixCnt)
{
	// relight
	Relight(BoundingBox);
	if (updateMatAndPixCnt) UpdateMatCnt(BoundingBox, true);
	// Restore Solidmasks
	C4Rect SolidMaskRect = BoundingBox;
	SolidMaskRect.x -= 2 * C4LS_MaxLightDistX; SolidMaskRect.y -= 2 * C4LS_MaxLightDistY;
	SolidMaskRect.Wdt += 4 * C4LS_MaxLightDistX; SolidMaskRect.Hgt += 4 * C4LS_MaxLightDistY;
	for (C4SolidMask *pSolid = C4SolidMask::First; pSolid; pSolid = pSolid->Next)
	{
		pSolid->Repair(SolidMaskRect);
	}
	if (updateMatAndPixCnt) UpdatePixCnt(BoundingBox);
	C4SolidMask::CheckConsistency();
}

void C4Landscape::UpdatePixCnt(const C4Rect &Rect, bool fCheck)
{
	int32_t PixCntWidth = (Width + 16) / 17;
	for (int32_t y = std::max<int32_t>(0, Rect.y / 15); y < std::min<int32_t>(PixCntPitch, (Rect.y + Rect.Hgt + 14) / 15); y++)
		for (int32_t x = std::max<int32_t>(0, Rect.x / 17); x < std::min<int32_t>(PixCntWidth, (Rect.x + Rect.Wdt + 16) / 17); x++)
		{
			int iCnt = 0;
			for (int32_t x2 = x * 17; x2 < std::min<int32_t>(x * 17 + 17, Width); x2++)
				for (int32_t y2 = y * 15; y2 < std::min<int32_t>(y * 15 + 15, Height); y2++)
					if (_GetDensity(x2, y2))
						iCnt++;
			if (fCheck)
				assert(iCnt == PixCnt[x * PixCntPitch + y]);
			PixCnt[x * PixCntPitch + y] = iCnt;
		}
}

void C4Landscape::UpdateMatCnt(C4Rect Rect, bool fPlus)
{
	Rect.Intersect(C4Rect(0, 0, Width, Height));
	if (!Rect.Hgt || !Rect.Wdt) return;
	// Multiplicator for changes
	const int32_t iMul = fPlus ? +1 : -1;
	// Count pixels
	for (int32_t x = 0; x < Rect.Wdt; x++)
	{
		int iHgt = 0;
		int32_t y;
		for (y = 1; y < Rect.Hgt; y++)
		{
			int32_t iMat = _GetMat(Rect.x + x, Rect.y + y - 1);
			// Same material? Count it.
			if (iMat == _GetMat(Rect.x + x, Rect.y + y))
				iHgt++;
			else
			{
				if (iMat >= 0)
				{
					// Normal material counting
					MatCount[iMat] += iMul * (iHgt + 1);
					// Effective material counting enabled?
					if (int32_t iMinHgt = Section.Material.Map[iMat].MinHeightCount)
					{
						// First chunk? Add any material above when checking chunk height
						int iAddedHeight = 0;
						if (Rect.y && iHgt + 1 == y)
							iAddedHeight = GetMatHeight(Rect.x + x, Rect.y - 1, -1, iMat, iMinHgt);
						// Check the chunk height
						if (iHgt + 1 + iAddedHeight >= iMinHgt)
						{
							EffectiveMatCount[iMat] += iMul * (iHgt + 1);
							if (iAddedHeight < iMinHgt)
								EffectiveMatCount[iMat] += iMul * iAddedHeight;
						}
					}
				}
				// Next chunk of material
				iHgt = 0;
			}
		}
		// Check last pixel
		int32_t iMat = _GetMat(Rect.x + x, Rect.y + Rect.Hgt - 1);
		if (iMat >= 0)
		{
			// Normal material counting
			MatCount[iMat] += iMul * (iHgt + 1);
			// Minimum height counting?
			if (int32_t iMinHgt = Section.Material.Map[iMat].MinHeightCount)
			{
				int iAddedHeight1 = 0, iAddedHeight2 = 0;
				// Add any material above for chunk size check
				if (Rect.y && iHgt + 1 == Rect.Hgt)
					iAddedHeight1 = GetMatHeight(Rect.x + x, Rect.y - 1, -1, iMat, iMinHgt);
				// Add any material below for chunk size check
				if (Rect.y + y < Height)
					iAddedHeight2 = GetMatHeight(Rect.x + x, Rect.y + Rect.Hgt, 1, iMat, iMinHgt);
				// Chunk tall enough?
				if (iHgt + 1 + iAddedHeight1 + iAddedHeight2 >= Section.Material.Map[iMat].MinHeightCount)
				{
					EffectiveMatCount[iMat] += iMul * (iHgt + 1);
					if (iAddedHeight1 < iMinHgt)
						EffectiveMatCount[iMat] += iMul * iAddedHeight1;
					if (iAddedHeight2 < iMinHgt)
						EffectiveMatCount[iMat] += iMul * iAddedHeight2;
				}
			}
		}
	}
}

void C4Landscape::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(MapSeed,                 "MapSeed",       0));
	pComp->Value(mkNamingAdapt(LeftOpen,                "LeftOpen",      0));
	pComp->Value(mkNamingAdapt(RightOpen,               "RightOpen",     0));
	pComp->Value(mkNamingAdapt(TopOpen,                 "TopOpen",       false));
	pComp->Value(mkNamingAdapt(BottomOpen,              "BottomOpen",    false));
	pComp->Value(mkNamingAdapt(mkCastIntAdapt(Gravity), "Gravity",       FIXED100(20)));
	pComp->Value(mkNamingAdapt(Modulation,              "MatModulation", 0U));
	pComp->Value(mkNamingAdapt(Mode,                    "Mode",          C4LSC_Undefined));
}

void C4Landscape::RemoveUnusedTexMapEntries()
{
	// check usage in landscape
	bool fTexUsage[128];
	int32_t iMatTex;
	for (iMatTex = 0; iMatTex < 128; ++iMatTex) fTexUsage[iMatTex] = false;
	for (int32_t y = 0; y < Height; ++y)
		for (int32_t x = 0; x < Width; ++x)
			fTexUsage[Surface8->GetPix(x, y) & 0x7f] = true;
	// check usage by materials
	for (int32_t iMat = 0; iMat < Section.Material.Num; ++iMat)
	{
		C4Material *pMat = Section.Material.Map + iMat;
		if (pMat->BlastShiftTo >= 0) fTexUsage[pMat->BlastShiftTo & 0x7f] = true;
		if (pMat->BelowTempConvertTo >= 0) fTexUsage[pMat->BelowTempConvertTo & 0x7f] = true;
		if (pMat->AboveTempConvertTo >= 0) fTexUsage[pMat->AboveTempConvertTo & 0x7f] = true;
		if (pMat->DefaultMatTex >= 0) fTexUsage[pMat->DefaultMatTex & 0x7f] = true;
	}
	// remove unused
	for (iMatTex = 1; iMatTex < C4M_MaxTexIndex; ++iMatTex)
		if (!fTexUsage[iMatTex])
			Section.TextureMap.RemoveEntry(iMatTex);
	// flag rewrite
	Section.TextureMap.fEntriesAdded = true;
}
