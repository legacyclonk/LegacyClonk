/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2022, The LegacyClonk Team and contributors
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

#include "C4Game.h"
#include <C4Include.h>
#include <C4Weather.h>

#include <C4Object.h>
#include <C4Random.h>
#include "C4SoundSystem.h"
#include <C4Wrappers.h>

C4Weather::C4Weather(C4Section &section)
	: section{section}
{
	Default();
}

C4Weather::~C4Weather()
{
	Clear();
}

void C4Weather::Init(bool fScenario)
{
	if (fScenario)
	{
		// Season
		Season = section.C4S.Weather.StartSeason.Evaluate();
		YearSpeed = section.C4S.Weather.YearSpeed.Evaluate();
		// Temperature
		Climate = 100 - section.C4S.Weather.Climate.Evaluate() - 50;
		Temperature = Climate;
		// Wind
		Wind = TargetWind = section.C4S.Weather.Wind.Evaluate();
		// Precipitation
		if (!section.C4S.Head.NoInitialize)
			if (section.C4S.Weather.Rain.Evaluate())
				for (int32_t iClouds = (std::min)(section.Landscape.Width / 500, 5); iClouds > 0; iClouds--)
				{
					volatile int iWidth = section.Landscape.Width / 15 + Random(320);
					volatile int iX = Random(section.Landscape.Width);
					LaunchCloud(iX, -1, iWidth,
						section.C4S.Weather.Rain.Evaluate(),
						section.C4S.Weather.Precipitation);
				}
		// Lightning
		LightningLevel = section.C4S.Weather.Lightning.Evaluate();
		// Disasters
		MeteoriteLevel = section.C4S.Disasters.Meteorite.Evaluate();
		VolcanoLevel = section.C4S.Disasters.Volcano.Evaluate();
		EarthquakeLevel = section.C4S.Disasters.Earthquake.Evaluate();
		// gamma?
		NoGamma = section.C4S.Weather.NoGamma;
	}
	// set gamma
	SetSeasonGamma();
}

void C4Weather::Execute()
{
	// Season
	if (!Tick35)
	{
		SeasonDelay += YearSpeed;
		if (SeasonDelay >= 200)
		{
			SeasonDelay = 0;
			Season++;
			if (Season > section.C4S.Weather.StartSeason.Max)
				Season = section.C4S.Weather.StartSeason.Min;
			SetSeasonGamma();
		}
	}
	// Temperature
	if (!Tick35)
	{
		int32_t iTemperature = Climate - static_cast<int32_t>(TemperatureRange * cos(6.28 * static_cast<float>(Season) / 100.0));
		if (Temperature < iTemperature) Temperature++;
		else if (Temperature > iTemperature) Temperature--;
	}
	// Wind
	if (!Tick1000)
		TargetWind = section.C4S.Weather.Wind.Evaluate();
	if (!Tick10)
		Wind = BoundBy<int32_t>(Wind + Sign(TargetWind - Wind),
			section.C4S.Weather.Wind.Min,
			section.C4S.Weather.Wind.Max);
	if (!Tick10)
		SoundLevel("Wind", nullptr, (std::max)(Abs(Wind) - 30, 0) * 2);
	// Disaster launch
	if (!Tick10)
	{
		// Meteorite
		if (!Random(60))
			if (Random(100) < MeteoriteLevel)
			{
				C4Object *meto;
				// In cave landscapes, meteors must be created a bit lower so they don't hit the ceiling
				// (who activates meteors in cave landscapes anyway?)
				// force argument evaluation order
				const auto r2 = Random(100 + 1);
				const auto r1 = Random(section.Landscape.Width);
				meto = section.CreateObject(C4ID_Meteor, nullptr, NO_OWNER,
					r1, section.Landscape.TopOpen ? -20 : 5, 0,
					itofix(r2 - 50) / 10,
					section.Landscape.TopOpen ? Fix0 : itofix(2), itofix(1) / 5);
			}
		// Lightning
		if (!Random(35))
			if (Random(100) < LightningLevel)
			{
				LaunchLightning(Random(section.Landscape.Width), 0,
					-20, 41, +5, 15, true);
			}
		// Earthquake
		if (!Random(50))
			if (Random(100) < EarthquakeLevel)
			{
				// force argument evaluation order
				const auto r2 = Random(section.Landscape.Height);
				const auto r1 = Random(section.Landscape.Width);
				LaunchEarthquake(r1, r2);
			}
		// Volcano
		if (!Random(60))
			if (Random(100) < VolcanoLevel)
			{
				// force argument evaluation order
				const auto r2 = Random(10);
				const auto r1 = Random(section.Landscape.Width);
				LaunchVolcano(section.Material.Get("Lava"),
					r1, section.Landscape.Height - 1,
					BoundBy(15 * section.Landscape.Height / 500 + r2, 10, 60));
			}
	}
}

void C4Weather::Clear() {}

bool C4Weather::LaunchLightning(int32_t x, int32_t y, int32_t xdir, int32_t xrange, int32_t ydir, int32_t yrange, bool fDoGamma)
{
	C4Object *pObj;
	if (pObj = section.CreateObject(C4Id("FXL1"), nullptr))
		pObj->Call(PSF_Activate, {C4VInt(x),
			C4VInt(y),
			C4VInt(xdir),
			C4VInt(xrange),
			C4VInt(ydir),
			C4VInt(yrange),
			C4VBool(!!fDoGamma)});
	return true;
}

int32_t C4Weather::GetWind(int32_t x, int32_t y)
{
	if (section.Landscape.GBackIFT(x, y)) return 0;
	return Wind;
}

int32_t C4Weather::GetTemperature()
{
	return Temperature;
}

bool C4Weather::LaunchVolcano(int32_t mat, int32_t x, int32_t y, int32_t size)
{
	C4Object *pObj;
	if (pObj = section.CreateObject(C4Id("FXV1"), nullptr))
		pObj->Call(PSF_Activate, {C4VInt(x), C4VInt(y), C4VInt(size), C4VInt(mat)});
	return true;
}

void C4Weather::Default()
{
	Season = 0; YearSpeed = 0; SeasonDelay = 0;
	Wind = TargetWind = 0;
	Temperature = Climate = 0;
	TemperatureRange = 30;
	MeteoriteLevel = VolcanoLevel = EarthquakeLevel = LightningLevel = 0;
	NoGamma = true;
}

bool C4Weather::LaunchEarthquake(int32_t iX, int32_t iY)
{
	C4Object *pObj;
	if (pObj = section.CreateObject(C4Id("FXQ1"), nullptr, NO_OWNER, iX, iY))
		if (pObj->Call(PSF_Activate))
			return true;
	return false;
}

bool C4Weather::LaunchCloud(int32_t iX, int32_t iY, int32_t iWidth, int32_t iStrength, const char *szPrecipitation)
{
	if (section.Material.Get(szPrecipitation) == MNone) return false;
	C4Object *pObj;
	if (pObj = section.CreateObject(C4Id("FXP1"), nullptr, NO_OWNER, iX, iY))
		if (pObj->Call(PSF_Activate, {C4VInt(section.Material.Get(szPrecipitation)),
			C4VInt(iWidth),
			C4VInt(iStrength)}))
			return true;
	return false;
}

void C4Weather::SetWind(int32_t iWind)
{
	Wind = BoundBy<int32_t>(iWind, -100, +100);
	TargetWind = BoundBy<int32_t>(iWind, -100, +100);
}

void C4Weather::SetTemperature(int32_t iTemperature)
{
	Temperature = BoundBy<int32_t>(iTemperature, -100, 100);
	SetSeasonGamma();
}

void C4Weather::SetSeason(int32_t iSeason)
{
	Season = BoundBy<int32_t>(iSeason, 0, 100);
	SetSeasonGamma();
}

int32_t C4Weather::GetSeason()
{
	return Season;
}

void C4Weather::SetClimate(int32_t iClimate)
{
	Climate = BoundBy<int32_t>(iClimate, -50, +50);
	SetSeasonGamma();
}

int32_t C4Weather::GetClimate()
{
	return Climate;
}

static uint32_t SeasonColors[4][3] =
{
	{ 0x000000, 0x7f7f90, 0xefefff }, // winter: slightly blue; blued out by temperature
	{ 0x070f00, 0x90a07f, 0xffffdf }, // spring: green to yellow
	{ 0x000000, 0x808080, 0xffffff }, // summer: regular ramp
	{ 0x0f0700, 0xa08067, 0xffffdf }  // fall:   dark, brown ramp
};

void C4Weather::SetSeasonGamma()
{
	if (NoGamma) return;
	// get season num and offset
	int32_t iSeason1 = (Season / 25) % 4; int32_t iSeason2 = (iSeason1 + 1) % 4;
	int32_t iSeasonOff1 = BoundBy(Season % 25, 5, 19) - 5; int32_t iSeasonOff2 = 15 - iSeasonOff1;
	uint32_t dwClr[3]{};
	// interpolate between season colors
	for (int32_t i = 0; i < 3; ++i)
		for (int32_t iChan = 0; iChan < 24; iChan += 8)
		{
			uint8_t byC1 = uint8_t(SeasonColors[iSeason1][i] >> iChan);
			uint8_t byC2 = uint8_t(SeasonColors[iSeason2][i] >> iChan);
			int32_t iChanVal = (byC1 * iSeasonOff2 + byC2 * iSeasonOff1) / 15;
			// red+green: reduce in winter
			if (Temperature < 0)
				if (iChan)
					iChanVal += Temperature / 2;
				else
					// blue channel: emphasize in winter
					iChanVal -= Temperature / 2;
			// set channel
			dwClr[i] |= BoundBy<int32_t>(iChanVal, 0, 255) << iChan;
		}
	// apply gamma ramp
	Game.GraphicsSystem.SetGamma(dwClr[0], dwClr[1], dwClr[2], C4GRI_SEASON);
}

void C4Weather::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(Season,           "Season",           0));
	pComp->Value(mkNamingAdapt(YearSpeed,        "YearSpeed",        0));
	pComp->Value(mkNamingAdapt(SeasonDelay,      "SeasonDelay",      0));
	pComp->Value(mkNamingAdapt(Wind,             "Wind",             0));
	pComp->Value(mkNamingAdapt(TargetWind,       "TargetWind",       0));
	pComp->Value(mkNamingAdapt(Temperature,      "Temperature",      0));
	pComp->Value(mkNamingAdapt(TemperatureRange, "TemperatureRange", 30));
	pComp->Value(mkNamingAdapt(Climate,          "Climate",          0));
	pComp->Value(mkNamingAdapt(MeteoriteLevel,   "MeteoriteLevel",   0));
	pComp->Value(mkNamingAdapt(VolcanoLevel,     "VolcanoLevel",     0));
	pComp->Value(mkNamingAdapt(EarthquakeLevel,  "EarthquakeLevel",  0));
	pComp->Value(mkNamingAdapt(LightningLevel,   "LightningLevel",   0));
	pComp->Value(mkNamingAdapt(NoGamma,          "NoGamma",          false));
	uint32_t dwGammaDefaults[C4MaxGammaRamps * 3];
	for (int32_t i = 0; i < C4MaxGammaRamps; ++i)
	{
		dwGammaDefaults[i * 3 + 0] = 0x000000;
		dwGammaDefaults[i * 3 + 1] = 0x808080;
		dwGammaDefaults[i * 3 + 2] = 0xffffff;
	}
	pComp->Value(mkNamingAdapt(mkArrayAdapt(Game.GraphicsSystem.dwGamma), "Gamma", dwGammaDefaults));
}
