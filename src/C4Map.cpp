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

/* Create map from dynamic landscape data in scenario */

#include <C4Include.h>
#include <C4Map.h>

#include <C4Random.h>
#include <C4Texture.h>
#include <C4Group.h>

#include <Bitmap256.h>

#include <numbers>

C4MapCreator::C4MapCreator()
{
	Reset();
}

void C4MapCreator::Reset()
{
	MapIFT = 128;
	MapBuf = nullptr;
	Exclusive = -1;
}

void C4MapCreator::SetPix(int32_t x, int32_t y, uint8_t col)
{
	// Safety
	if (!Inside<int32_t>(x, 0, MapWdt - 1) || !Inside<int32_t>(y, 0, MapHgt - 1)) return;
	// Exclusive
	if (Exclusive > -1) if (GetPix(x, y) != Exclusive) return;
	// Set pix
	MapBuf->SetPix(x, y, col);
}

void C4MapCreator::DrawLayer(C4Random &random, int32_t x, int32_t y, int32_t size, uint8_t col)
{
	int32_t cnt, cnt2;
	for (cnt = 0; cnt < size; cnt++)
	{
		x += random.Random(9) - 4; y += random.Random(3) - 1;
		for (cnt2 = random.Random(3); cnt2 < 5; cnt2++)
		{
			SetPix(x + cnt2, y, col); SetPix(x + cnt2 + 1, y + 1, col);
		}
	}
}

uint8_t C4MapCreator::GetPix(int32_t x, int32_t y)
{
	// Safety
	if (!Inside<int32_t>(x, 0, MapWdt - 1) || !Inside<int32_t>(y, 0, MapHgt - 1)) return 0;
	// Get pix
	return MapBuf->GetPix(x, y);
}

void C4MapCreator::Create(CSurface8 *sfcMap,
	C4SLandscape &rLScape, C4Random &random, C4TextureMap &rTexMap,
	bool fLayers, int32_t iPlayerNum)
{
	double fullperiod = 20.0 * std::numbers::pi;
	uint8_t ccol;
	int32_t cx, cy;

	// Safeties
	if (!sfcMap) return;
	iPlayerNum = BoundBy<int32_t>(iPlayerNum, 1, C4S_MaxPlayer);

	// Set creator variables
	MapBuf = sfcMap;
	MapWdt = MapBuf->Wdt; MapHgt = MapBuf->Hgt;

	// Reset map (0 is sky)
	MapBuf->ClearBox8Only(0, 0, MapBuf->Wdt, MapBuf->Hgt);

	// Surface
	ccol = rTexMap.GetIndexMatTex(rLScape.Material, "Smooth") + MapIFT;
	float amplitude = static_cast<float>(rLScape.Amplitude.Evaluate(random));
	float phase = static_cast<float>(rLScape.Phase.Evaluate(random));
	float period = static_cast<float>(rLScape.Period.Evaluate(random));
	if (rLScape.MapPlayerExtend) period *= (std::min)(iPlayerNum, C4S_MaxMapPlayerExtend);
	float natural = static_cast<float>(rLScape.Random.Evaluate(random));
	int32_t level0 = (std::min)(MapWdt, MapHgt) / 2;
	int32_t maxrange = level0 * 3 / 4;
	double cy_curve, cy_natural; // -1.0 - +1.0 !

	double rnd_cy, rnd_tend; // -1.0 - +1.0 !
	rnd_cy = (random.Random(2000 + 1) - 1000) / 1000.0;
	rnd_tend = (random.Random(200 + 1) - 100) / 20000.0;

	for (cx = 0; cx < MapWdt; cx++)
	{
		rnd_cy += rnd_tend;
		rnd_tend += (random.Random(100 + 1) - 50) / 10000.0;
		if (rnd_tend > +0.05) rnd_tend = +0.05;
		if (rnd_tend < -0.05) rnd_tend = -0.05;
		if (rnd_cy < -0.5) rnd_tend += 0.01;
		if (rnd_cy > +0.5) rnd_tend -= 0.01;

		cy_natural = rnd_cy * natural / 100.0;
		cy_curve = sin(fullperiod * period / 100.0 * cx / static_cast<double>(MapWdt) +
			2.0 * std::numbers::pi * phase / 100.0) * amplitude / 100.0;

		cy = level0 + BoundBy(static_cast<int32_t>(maxrange * (cy_curve + cy_natural)),
			-maxrange, +maxrange);

		SetPix(cx, cy, ccol);
	}

	// Raise bottom to surface
	for (cx = 0; cx < MapWdt; cx++)
		for (cy = MapHgt - 1; (cy >= 0) && !GetPix(cx, cy); cy--)
			SetPix(cx, cy, ccol);
	// Raise liquid level
	Exclusive = 0;
	ccol = rTexMap.GetIndexMatTex(rLScape.Liquid, "Smooth");
	int32_t wtr_level = rLScape.LiquidLevel.Evaluate(random);
	for (cx = 0; cx < MapWdt; cx++)
		for (cy = MapHgt * (100 - wtr_level) / 100; cy < MapHgt; cy++)
			SetPix(cx, cy, ccol);
	Exclusive = -1;

	// Layers
	if (fLayers)
	{
		// Base material
		Exclusive = rTexMap.GetIndexMatTex(rLScape.Material, "Smooth") + MapIFT;

		int32_t cnt, clayer, layer_num, sptx, spty;

		// Process layer name list
		for (clayer = 0; clayer < C4MaxNameList; clayer++)
			if (rLScape.Layers.Name[clayer][0])
			{
				// Draw layers
				ccol = rTexMap.GetIndexMatTex(rLScape.Layers.Name[clayer], "Rough") + MapIFT;
				layer_num = rLScape.Layers.Count[clayer];
				layer_num = layer_num * MapWdt * MapHgt / 15000;
				for (cnt = 0; cnt < layer_num; cnt++)
				{
					// Place layer
					sptx = random.Random(MapWdt);
					for (spty = 0; (spty < MapHgt) && (GetPix(sptx, spty) != Exclusive); spty++);
					spty += 5 + random.Random((MapHgt - spty) - 10);
					DrawLayer(random, sptx, spty, random.Random(15), ccol);
				}
			}

		Exclusive = -1;
	}
}
