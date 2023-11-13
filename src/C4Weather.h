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

/* Controls temperature, wind, and natural disasters */

#pragma once

#include "C4ForwardDeclarations.h"

#include <cstdint>

class C4Weather
{
public:
	C4Weather(C4Section &section);
	~C4Weather();

private:
	C4Section &section;

public:
	int32_t Season, YearSpeed, SeasonDelay;
	int32_t Wind, TargetWind;
	int32_t Temperature, TemperatureRange, Climate;
	int32_t MeteoriteLevel, VolcanoLevel, EarthquakeLevel, LightningLevel;
	bool NoGamma;

public:
	void Default();
	void Clear();
	void Execute();
	void SetClimate(int32_t iClimate);
	void SetSeason(int32_t iSeason);
	void SetTemperature(int32_t iTemperature);
	void Init(bool fScenario);
	void SetWind(int32_t iWind);
	int32_t GetWind(int32_t x, int32_t y);
	int32_t GetTemperature();
	int32_t GetSeason();
	int32_t GetClimate();
	bool LaunchLightning(int32_t x, int32_t y, int32_t xdir, int32_t xrange, int32_t ydir, int32_t yrange, bool fDoGamma);
	bool LaunchVolcano(int32_t mat, int32_t x, int32_t y, int32_t size);
	bool LaunchEarthquake(int32_t iX, int32_t iY);
	bool LaunchCloud(int32_t iX, int32_t iY, int32_t iWidth, int32_t iStrength, const char *szPrecipitation);
	void SetSeasonGamma(); // set gamma adjustment for season
	void CompileFunc(StdCompiler *pComp);
};
