/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2020, The LegacyClonk Team and contributors
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

/* Small member of the landscape class to handle the sky background */

#include <C4Include.h>
#include <C4Sky.h>

#include <C4Game.h>
#include <C4Random.h>
#include <C4SurfaceFile.h>
#include <C4Components.h>
#include <C4Wrappers.h>

static bool SurfaceEnsureSize(C4Surface **ppSfc, int iMinWdt, int iMinHgt)
{
	// safety
	if (!ppSfc) return false; if (!*ppSfc) return false;
	// get size
	int iWdt = (*ppSfc)->Wdt, iHgt = (*ppSfc)->Hgt;
	int iDstWdt = iWdt, iDstHgt = iHgt;
	// check if it must be enlarged
	while (iDstWdt < iMinWdt) iDstWdt += iWdt;
	while (iDstHgt < iMinHgt) iDstHgt += iHgt;
	if (iDstWdt == iWdt && iDstHgt == iHgt) return true;
	// create new surface
	C4Surface *pNewSfc = new C4Surface();
	if (!pNewSfc->Create(iDstWdt, iDstHgt, false))
	{
		delete pNewSfc;
		return false;
	}
	// blit tiled into dest surface
	lpDDraw->BlitSurfaceTile2(*ppSfc, pNewSfc, 0, 0, iDstWdt, iDstHgt, 0, 0, false);
	// destroy old surface, assign new
	delete *ppSfc; *ppSfc = pNewSfc;
	// success
	return true;
}

void C4Sky::SetFadePalette(int32_t *ipColors)
{
	// If colors all zero, use game palette default blue
	if (ipColors[0] + ipColors[1] + ipColors[2] + ipColors[3] + ipColors[4] + ipColors[5] == 0)
	{
		uint8_t *pClr = Game.GraphicsResource.GamePalette + 3 * CSkyDef1;
		FadeClr1 = C4RGB(pClr[0], pClr[1], pClr[2]);
		FadeClr2 = C4RGB(pClr[3 * 19 + 0], pClr[3 * 19 + 1], pClr[3 * 19 + 2]);
	}
	else
	{
		// set colors
		FadeClr1 = C4RGB(ipColors[0], ipColors[1], ipColors[2]);
		FadeClr2 = C4RGB(ipColors[3], ipColors[4], ipColors[5]);
	}
}

bool C4Sky::Init(bool fSavegame)
{
	int32_t skylistn;

	// reset scrolling pos+speed
	// not in savegame, because it will have been loaded from game data there
	if (!fSavegame)
	{
		x = y = xdir = ydir = 0; ParX = ParY = 10; ParallaxMode = 0;
	}

	// Check for sky bitmap in scenario file
	Surface = new C4Surface();
	bool loaded = !!Surface->LoadAny(section.Group, C4CFN_Sky, true, true);

	// Else, evaluate scenario core landscape sky default list
	if (!loaded)
	{
		// Scan list sections
		SReplaceChar(section.C4S.Landscape.SkyDef, ',', ';'); // modifying the C4S here...!
		skylistn = SCharCount(';', section.C4S.Landscape.SkyDef) + 1;
		StdStrBuf sky;
		C4Random random{static_cast<std::uint32_t>(Game.Parameters.RandomSeed)};
		StdStrBuf::MakeRef(section.C4S.Landscape.SkyDef).GetSection(random.Random(skylistn), &sky, ';');
		sky.TrimSpaces();
		// Sky tile specified, try load
		if (sky.getLength() && sky != "Default")
		{
			// Check for sky tile in scenario file
			loaded = !!Surface->LoadAny(section.Group, sky.getData(), true, true);
			if (!loaded)
			{
				loaded = !!Surface->LoadAny(Game.GraphicsResource.Files, sky.getData(), true);
			}
		}
	}

	if (loaded)
	{
		FadeClr1 = FadeClr2 = 0xffffff;
		// enlarge surface to avoid slow 1*1-px-skies
		if (!SurfaceEnsureSize(&Surface, 128, 128)) return false;

		// set parallax scroll mode
		switch (section.C4S.Landscape.SkyScrollMode)
		{
		case 0: // default: no scrolling
			break;
		case 1: // go with the wind in xdir, and do some parallax scrolling in ydir
			ParallaxMode = C4SkyPM_Wind;
			ParY = 20;
			break;
		case 2: // parallax in both directions
			ParX = ParY = 20;
			break;
		}
	}

	// Else, try creating default Surface
	if (!loaded)
	{
		SetFadePalette(section.C4S.Landscape.SkyDefFade);
		delete Surface;
		Surface = nullptr;
	}

	// no sky - using fade in newgfx
	if (!Surface)
		return true;

	// Store size
	if (Surface)
	{
		int iWdt, iHgt;
		if (Surface->GetSurfaceSize(iWdt, iHgt))
		{
			Width = iWdt; Height = iHgt;
		}
	}

	// Success
	return true;
}

void C4Sky::Default()
{
	Width = Height = 0;
	Surface = nullptr;
	x = y = xdir = ydir = 0;
	Modulation = 0x00ffffff;
	ParX = ParY = 10;
	ParallaxMode = C4SkyPM_Fixed;
	BackClr = 0;
	BackClrEnabled = false;
}

C4Sky::~C4Sky()
{
	Clear();
}

void C4Sky::Clear()
{
	delete Surface; Surface = nullptr;
	Modulation = 0x00ffffff;
}

bool C4Sky::Save(C4Group &hGroup)
{
	// Sky-saving disabled by scenario core
	// (With this option enabled, script-defined changes to sky palette will not be saved!)
	if (section.C4S.Landscape.NoSky)
	{
		hGroup.Delete(C4CFN_Sky);
		return true;
	}
	// no sky?
	if (!Surface) return true;
	// FIXME?
	// Success
	return true;
}

void C4Sky::Execute()
{
	// surface exists?
	if (!Surface) return;
	// advance pos
	x += xdir; y += ydir;
	// clip by bounds
	if (x >= itofix(Width)) x -= itofix(Width);
	if (y >= itofix(Height)) y -= itofix(Height);
	// update speed
	if (ParallaxMode == C4SkyPM_Wind) xdir = FIXED100(section.Weather.Wind);
}

void C4Sky::Draw(C4FacetEx &cgo)
{
	// background color?
	if (BackClrEnabled) Application.DDraw->DrawBoxDw(cgo.Surface, cgo.X, cgo.Y, cgo.X + cgo.Wdt, cgo.Y + cgo.Hgt, BackClr);
	// sky surface?
	if (Modulation != 0xffffff) Application.DDraw->ActivateBlitModulation(Modulation);
	if (Surface)
	{
		// blit parallax sky
		int iParX = cgo.TargetX * 10 / ParX - fixtoi(x);
		int iParY = cgo.TargetY * 10 / ParY - fixtoi(y);
		Application.DDraw->BlitSurfaceTile2(Surface, cgo.Surface, cgo.X, cgo.Y, cgo.Wdt, cgo.Hgt, iParX, iParY, false);
	}
	else
	{
		// no sky surface: blit sky fade
		uint32_t dwClr1 = GetSkyFadeClr(cgo.TargetY);
		uint32_t dwClr2 = GetSkyFadeClr(cgo.TargetY + cgo.Hgt);
		Application.DDraw->DrawBoxFade(cgo.Surface, cgo.X, cgo.Y, cgo.Wdt, cgo.Hgt, dwClr1, dwClr1, dwClr2, dwClr2, cgo.TargetX, cgo.TargetY);
	}
	if (Modulation != 0xffffff) Application.DDraw->DeactivateBlitModulation();
	// done
}

uint32_t C4Sky::GetSkyFadeClr(int32_t iY)
{
	int32_t iPos2 = (iY * 256) / section.Landscape.Height; int32_t iPos1 = 256 - iPos2;
	return (((((FadeClr1 & 0xff00ff) * iPos1 + (FadeClr2 & 0xff00ff) * iPos2) & 0xff00ff00)
		| (((FadeClr1 & 0x00ff00) * iPos1 + (FadeClr2 & 0x00ff00) * iPos2) & 0x00ff0000)) >> 8)
		| (FadeClr1 & 0xff000000);
}

bool C4Sky::SetModulation(uint32_t dwWithClr, uint32_t dwBackClr)
{
	Modulation = dwWithClr;
	BackClr = dwBackClr;
	BackClrEnabled = (Modulation >> 24) ? true : false;
	return true;
}

void C4Sky::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkCastIntAdapt(x),    "X",              Fix0));
	pComp->Value(mkNamingAdapt(mkCastIntAdapt(y),    "Y",              Fix0));
	pComp->Value(mkNamingAdapt(mkCastIntAdapt(xdir), "XDir",           Fix0));
	pComp->Value(mkNamingAdapt(mkCastIntAdapt(ydir), "YDir",           Fix0));
	pComp->Value(mkNamingAdapt(Modulation,           "Modulation",     0x00ffffffU));
	pComp->Value(mkNamingAdapt(ParX,                 "ParX",           10));
	pComp->Value(mkNamingAdapt(ParY,                 "ParY",           10));
	pComp->Value(mkNamingAdapt(ParallaxMode,         "ParMode",        C4SkyPM_Fixed));
	pComp->Value(mkNamingAdapt(BackClr,              "BackClr",        0));
	pComp->Value(mkNamingAdapt(BackClrEnabled,       "BackClrEnabled", false));
}
