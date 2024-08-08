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

/* Core component of a scenario file */

#include <C4Include.h>
#include <C4Scenario.h>
#include <C4InputValidation.h>

#include <C4Random.h>
#include <C4Group.h>
#include <C4Game.h>
#include <C4Wrappers.h>

#include <iterator>

int32_t C4SHead::MainForcedAutoContextMenu{-1};
int32_t C4SHead::MainForcedControlStyle{-1};

// C4SVal

C4SVal::C4SVal(int32_t std, int32_t rnd, int32_t min, int32_t max)
	: Std(std), Rnd(rnd), Min(min), Max(max) {}

void C4SVal::Set(int32_t std, int32_t rnd, int32_t min, int32_t max)
{
	Std = std; Rnd = rnd; Min = min; Max = max;
}

int32_t C4SVal::Evaluate(C4Random &random)
{
	return BoundBy(Std + random.Random(2 * Rnd + 1) - Rnd, Min, Max);
}

void C4SVal::Default()
{
	Set();
}

void C4SVal::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkDefaultAdapt(Std, 0));
	if (!pComp->Separator()) return;
	pComp->Value(mkDefaultAdapt(Rnd, 0));
	if (!pComp->Separator()) return;
	pComp->Value(mkDefaultAdapt(Min, 0));
	if (!pComp->Separator()) return;
	pComp->Value(mkDefaultAdapt(Max, 100));
}

// C4Scenario

C4Scenario::C4Scenario()
{
	Default();
}

void C4Scenario::Default()
{
	int32_t cnt;
	Head.Default();
	Definitions.Default();
	Game.Default();
	for (cnt = 0; cnt < C4S_MaxPlayer; cnt++) PlrStart[cnt].Default();
	Landscape.Default();
	Animals.Default();
	Weather.Default();
	Disasters.Default();
	Game.Realism.Default();
	Environment.Default();
}

bool C4Scenario::Load(C4Group &hGroup, bool fLoadSection)
{
	char *pSource;
	// Load
	if (!hGroup.LoadEntry(C4CFN_ScenarioCore, &pSource, nullptr, 1)) return false;
	// Compile
	if (!Compile(pSource, fLoadSection)) { delete[] pSource; return false; }
	delete[] pSource;
	// Convert
	Game.ConvertGoals(Game.Realism);
	// Success
	return true;
}

bool C4Scenario::Save(C4Group &hGroup, bool fSaveSection)
{
	std::string buf;
	try
	{
		buf = DecompileToBuf<StdCompilerINIWrite>(mkParAdapt(*this, fSaveSection));
	}
	catch (const StdCompiler::Exception &)
	{
		return false;
	}

	StdStrBuf copy{buf.c_str(), buf.size()};
	if (!hGroup.Add(C4CFN_ScenarioCore, copy, false, true))
	{
		return false;
	}
	return true;
}

void C4Scenario::CompileFunc(StdCompiler *pComp, bool fSection)
{
	pComp->Value(mkNamingAdapt(mkParAdapt(Head, fSection), "Head"));
	if (!fSection) pComp->Value(mkNamingAdapt(Definitions, "Definitions"));
	pComp->Value(mkNamingAdapt(mkParAdapt(Game, fSection), "Game"));
	for (int32_t i = 0; i < C4S_MaxPlayer; i++)
		pComp->Value(mkNamingAdapt(PlrStart[i], std::format("Player{}", i + 1).c_str()));

	const bool newScenario{!Head.C4XVer[0] || CompareVersion(Head.C4XVer[0], Head.C4XVer[1], Head.C4XVer[2], Head.C4XVer[3], Head.C4XVer[4], 4, 6, 5, 0, 0) >= 0};
	pComp->Value(mkNamingAdapt(mkParAdapt(Landscape, newScenario),   "Landscape"));
	pComp->Value(mkNamingAdapt(Animals,     "Animals"));
	pComp->Value(mkNamingAdapt(Weather,     "Weather"));
	pComp->Value(mkNamingAdapt(Disasters,   "Disasters"));
	pComp->Value(mkNamingAdapt(Environment, "Environment"));
}

int32_t C4Scenario::GetMinPlayer()
{
	// MinPlayer is specified.
	if (Head.MinPlayer != 0)
		return Head.MinPlayer;
	// Melee? Need at least two.
	if (Game.IsMelee())
		return 2;
	// Otherwise/unknown: need at least one.
	return 1;
}

void C4SDefinitions::Default()
{
	LocalOnly = AllowUserChange = false;
	Definitions.clear();
	SkipDefs.Clear();
}

const int32_t C4S_MaxPlayerDefault = 12;

C4SHead::C4SHead() : MaxPlayer{C4S_MaxPlayerDefault}, MaxPlayerLeague{C4S_MaxPlayerDefault} {}

void C4SHead::Default()
{
	*this = {};
}

void C4SHead::CompileFunc(StdCompiler *pComp, bool fSection)
{
	if (!fSection)
	{
		pComp->Value(mkNamingAdapt(Icon,                      "Icon",            18));
		pComp->Value(mkNamingAdapt(mkStringAdaptMA(Title),    "Title",           "Default Title"));
		pComp->Value(mkNamingAdapt(mkStringAdaptMA(Loader),   "Loader",          ""));
		pComp->Value(mkNamingAdapt(mkStringAdaptMA(Font),     "Font",            ""));
		pComp->Value(mkNamingAdapt(mkArrayAdapt(C4XVer, 0),   "Version"));
		pComp->Value(mkNamingAdapt(Difficulty,                "Difficulty",      0));
		// Ignore EnableUnregisteredAccess
		int32_t EnableUnregisteredAccess = false;
		pComp->Value(mkNamingAdapt(EnableUnregisteredAccess,  "Access",          0));
		pComp->Value(mkNamingAdapt(MaxPlayer,                 "MaxPlayer",       C4S_MaxPlayerDefault));
		pComp->Value(mkNamingAdapt(MaxPlayerLeague,           "MaxPlayerLeague", MaxPlayer));
		pComp->Value(mkNamingAdapt(MinPlayer,                 "MinPlayer",       0));
		pComp->Value(mkNamingAdapt(SaveGame,                  "SaveGame",        0));
		pComp->Value(mkNamingAdapt(Replay,                    "Replay",          0));
		pComp->Value(mkNamingAdapt(Film,                      "Film",            0));
		pComp->Value(mkNamingAdapt(DisableMouse,              "DisableMouse",    0));
	}
	pComp->Value(mkNamingAdapt(NoInitialize, "NoInitialize", 0));
	pComp->Value(mkNamingAdapt(RandomSeed,   "RandomSeed",   0));
	pComp->Value(mkNamingAdapt(ForcedAutoContextMenu, "ForcedAutoContextMenu", fSection ? MainForcedAutoContextMenu : -1));
	pComp->Value(mkNamingAdapt(ForcedControlStyle,    "ForcedAutoStopControl", fSection ? MainForcedControlStyle : -1));
	if (!fSection)
	{
		MainForcedAutoContextMenu = ForcedAutoContextMenu;
		MainForcedControlStyle = ForcedControlStyle;
		pComp->Value(mkNamingAdapt(mkStringAdaptMA(Engine),        "Engine",                ""));
		pComp->Value(mkNamingAdapt(mkStringAdaptMA(MissionAccess), "MissionAccess",         ""));
		pComp->Value(mkNamingAdapt(NetworkGame,                    "NetworkGame",           false));
		pComp->Value(mkNamingAdapt(NetworkRuntimeJoin,             "NetworkRuntimeJoin",    false));
		pComp->Value(mkNamingAdapt(ForcedGfxMode,                  "ForcedGfxMode",         0));
		pComp->Value(mkNamingAdapt(ForcedFairCrew,                 "ForcedNoCrew",          0));
		pComp->Value(mkNamingAdapt(FairCrewStrength,               "DefCrewStrength",       0));
		pComp->Value(mkNamingAdapt(mkStrValAdapt(mkParAdapt(Origin, StdCompiler::RCT_All), C4InVal::VAL_SubPathFilename), "Origin", StdStrBuf()));
		// windows needs backslashes in Origin; other systems use forward slashes
		if (pComp->isCompiler()) Origin.ReplaceChar(AltDirectorySeparator, DirectorySeparator);
	}
}

void C4SGame::Default()
{
	Elimination = C4S_EliminateCrew;
	ValueGain = 0;
	CreateObjects.Clear();
	ClearObjects.Clear();
	ClearMaterial.Clear();
	Mode = C4S_Cooperative;
	CooperativeGoal = C4S_NoGoal;
	EnableRemoveFlag = false;
	Goals.Clear();
	Rules.Clear();
	FoWColor = 0;
}

void C4SGame::CompileFunc(StdCompiler *pComp, bool fSection)
{
	pComp->Value(mkNamingAdapt(Mode,                              "Mode",               C4S_Cooperative));
	pComp->Value(mkNamingAdapt(Elimination,                       "Elimination",        C4S_EliminateCrew));
	pComp->Value(mkNamingAdapt(CooperativeGoal,                   "CooperativeGoal",    C4S_NoGoal));
	pComp->Value(mkNamingAdapt(CreateObjects,                     "CreateObjects",      C4IDList()));
	pComp->Value(mkNamingAdapt(ClearObjects,                      "ClearObjects",       C4IDList()));
	pComp->Value(mkNamingAdapt(ClearMaterial,                     "ClearMaterials",     C4NameList()));
	pComp->Value(mkNamingAdapt(ValueGain,                         "ValueGain",          0));
	pComp->Value(mkNamingAdapt(EnableRemoveFlag,                  "EnableRemoveFlag",   false));
	pComp->Value(mkNamingAdapt(Realism.ConstructionNeedsMaterial, "StructNeedMaterial", false));
	pComp->Value(mkNamingAdapt(Realism.StructuresNeedEnergy,      "StructNeedEnergy",   true));
	if (!fSection)
	{
		pComp->Value(mkNamingAdapt(Realism.ValueOverloads, "ValueOverloads", C4IDList()));
	}
	pComp->Value(mkNamingAdapt(mkRuntimeValueAdapt(Realism.LandscapePushPull),     "LandscapePushPull",     0));
	pComp->Value(mkNamingAdapt(mkRuntimeValueAdapt(Realism.LandscapeInsertThrust), "LandscapeInsertThrust", 1));

	const StdBitfieldEntry<int32_t> BaseFunctionalities[] =
	{
		{ "BASEFUNC_AutoSellContents", BASEFUNC_AutoSellContents },
		{ "BASEFUNC_RegenerateEnergy", BASEFUNC_RegenerateEnergy },
		{ "BASEFUNC_Buy",              BASEFUNC_Buy },
		{ "BASEFUNC_Sell",             BASEFUNC_Sell },
		{ "BASEFUNC_RejectEntrance",   BASEFUNC_RejectEntrance },
		{ "BASEFUNC_Extinguish",       BASEFUNC_Extinguish },
		{ "BASEFUNC_Default",          BASEFUNC_Default },
		{ nullptr, 0 }
	};

	pComp->Value(mkNamingAdapt(mkRuntimeValueAdapt(mkBitfieldAdapt<int32_t>(Realism.BaseFunctionality, BaseFunctionalities)), "BaseFunctionality", BASEFUNC_Default));
	pComp->Value(mkNamingAdapt(mkRuntimeValueAdapt(Realism.BaseRegenerateEnergyPrice), "BaseRegenerateEnergyPrice", BASE_RegenerateEnergyPrice));
	pComp->Value(mkNamingAdapt(Goals,    "Goals",    C4IDList()));
	pComp->Value(mkNamingAdapt(Rules,    "Rules",    C4IDList()));
	pComp->Value(mkNamingAdapt(FoWColor, "FoWColor", 0u));
}

void C4SPlrStart::Default()
{
	NativeCrew = C4ID_None;
	Crew.Set(1, 0, 1, 10);
	Wealth.Set(0, 0, 0, 250);
	Position[0] = Position[1] = -1;
	EnforcePosition = 0;
	ReadyCrew.Clear();
	ReadyBase.Clear();
	ReadyVehic.Clear();
	ReadyMaterial.Clear();
	BuildKnowledge.Clear();
	HomeBaseMaterial.Clear();
	HomeBaseProduction.Clear();
	Magic.Clear();
}

void C4SPlrStart::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkC4IDAdapt(NativeCrew), "StandardCrew",       C4ID_None));
	pComp->Value(mkNamingAdapt(Crew,                    "Clonks",             C4SVal(1, 0, 1, 10),  true));
	pComp->Value(mkNamingAdapt(Wealth,                  "Wealth",             C4SVal(0, 0, 0, 250), true));
	pComp->Value(mkNamingAdapt(mkArrayAdapt(Position, -1), "Position"));
	pComp->Value(mkNamingAdapt(EnforcePosition,         "EnforcePosition",    0));
	pComp->Value(mkNamingAdapt(ReadyCrew,               "Crew",               C4IDList()));
	pComp->Value(mkNamingAdapt(ReadyBase,               "Buildings",          C4IDList()));
	pComp->Value(mkNamingAdapt(ReadyVehic,              "Vehicles",           C4IDList()));
	pComp->Value(mkNamingAdapt(ReadyMaterial,           "Material",           C4IDList()));
	pComp->Value(mkNamingAdapt(BuildKnowledge,          "Knowledge",          C4IDList()));
	pComp->Value(mkNamingAdapt(HomeBaseMaterial,        "HomeBaseMaterial",   C4IDList()));
	pComp->Value(mkNamingAdapt(HomeBaseProduction,      "HomeBaseProduction", C4IDList()));
	pComp->Value(mkNamingAdapt(Magic,                   "Magic",              C4IDList()));
}

void C4SLandscape::Default()
{
	BottomOpen = false; TopOpen = true;
	LeftOpen = 0; RightOpen = 0;
	AutoScanSideOpen = true;
	SkyDef[0] = 0;
	NoSky = false;
	for (int32_t cnt = 0; cnt < 6; cnt++) SkyDefFade[cnt] = 0;
	VegLevel.Set(50, 30, 0, 100);
	Vegetation.Clear();
	InEarthLevel.Set(50, 0, 0, 100);
	InEarth.Clear();
	MapWdt.Set(100, 0, 64, 250);
	MapHgt.Set(50, 0, 40, 250);
	MapZoom.Set(10, 0, 5, 15);
	Amplitude.Set(0, 0);
	Phase.Set(50);
	Period.Set(15);
	Random.Set(0);
	LiquidLevel.Default();
	MapPlayerExtend = false;
	Layers.Clear();
	SCopy("Earth", Material, C4M_MaxName);
	SCopy("Water", Liquid, C4M_MaxName);
	ExactLandscape = false;
	Gravity.Set(100, 0, 10, 200);
	NoScan = false;
	KeepMapCreator = false;
	SkyScrollMode = 0;
	NewStyleLandscape = 0;
	FoWRes = CClrModAddMap::iDefResolutionX;
	ShadeMaterials = true;
}

void C4SLandscape::GetMapSize(int32_t &rWdt, int32_t &rHgt, int32_t iPlayerNum, C4Random &random)
{
	rWdt = MapWdt.Evaluate(random);
	rHgt = MapHgt.Evaluate(random);
	iPlayerNum = std::max<int32_t>(iPlayerNum, 1);
	if (MapPlayerExtend)
		rWdt = (std::min)(rWdt * (std::min)(iPlayerNum, C4S_MaxMapPlayerExtend), MapWdt.Max);
}

void C4SLandscape::CompileFunc(StdCompiler *pComp, bool newScenario)
{
	pComp->Value(mkNamingAdapt(ExactLandscape,            "ExactLandscape",    false));
	pComp->Value(mkNamingAdapt(Vegetation,                "Vegetation",        C4IDList()));
	pComp->Value(mkNamingAdapt(VegLevel,                  "VegetationLevel",   C4SVal(50, 30, 0, 100), true));
	pComp->Value(mkNamingAdapt(InEarth,                   "InEarth",           C4IDList()));
	pComp->Value(mkNamingAdapt(InEarthLevel,              "InEarthLevel",      C4SVal(50, 0, 0, 100), true));
	pComp->Value(mkNamingAdapt(mkStringAdaptMA(SkyDef),   "Sky",               ""));
	pComp->Value(mkNamingAdapt(mkArrayAdapt(SkyDefFade, 0), "SkyFade"));
	pComp->Value(mkNamingAdapt(NoSky,                     "NoSky",             false));
	pComp->Value(mkNamingAdapt(BottomOpen,                "BottomOpen",        false));
	pComp->Value(mkNamingAdapt(TopOpen,                   "TopOpen",           true));
	pComp->Value(mkNamingAdapt(LeftOpen,                  "LeftOpen",          0));
	pComp->Value(mkNamingAdapt(RightOpen,                 "RightOpen",         0));
	pComp->Value(mkNamingAdapt(AutoScanSideOpen,          "AutoScanSideOpen",  true));
	pComp->Value(mkNamingAdapt(MapWdt,                    "MapWidth",          C4SVal(100, 0, 64, 250), true));
	pComp->Value(mkNamingAdapt(MapHgt,                    "MapHeight",         C4SVal(50, 0, 40, 250), true));
	pComp->Value(mkNamingAdapt(MapZoom,                   "MapZoom",           C4SVal(10, 0, 5, 15), true));
	pComp->Value(mkNamingAdapt(Amplitude,                 "Amplitude",         C4SVal(0)));
	pComp->Value(mkNamingAdapt(Phase,                     "Phase",             C4SVal(50)));
	pComp->Value(mkNamingAdapt(Period,                    "Period",            C4SVal(15)));
	pComp->Value(mkNamingAdapt(Random,                    "Random",            C4SVal(0)));
	pComp->Value(mkNamingAdapt(mkStringAdaptMA(Material), "Material",          "Earth"));
	pComp->Value(mkNamingAdapt(mkStringAdaptMA(Liquid),   "Liquid",            "Water"));
	pComp->Value(mkNamingAdapt(LiquidLevel,               "LiquidLevel",       C4SVal()));
	pComp->Value(mkNamingAdapt(MapPlayerExtend,           "MapPlayerExtend",   false));
	pComp->Value(mkNamingAdapt(Layers,                    "Layers",            C4NameList()));
	pComp->Value(mkNamingAdapt(Gravity,                   "Gravity",           C4SVal(100, 0, 10, 200), true));
	pComp->Value(mkNamingAdapt(NoScan,                    "NoScan",            false));
	pComp->Value(mkNamingAdapt(KeepMapCreator,            "KeepMapCreator",    false));
	pComp->Value(mkNamingAdapt(SkyScrollMode,             "SkyScrollMode",     0));
	pComp->Value(mkNamingAdapt(NewStyleLandscape,         "NewStyleLandscape", 0));
	pComp->Value(mkNamingAdapt(FoWRes,                    "FoWRes",            static_cast<int32_t>(CClrModAddMap::iDefResolutionX)));
	pComp->Value(mkNamingAdapt(ShadeMaterials,            "ShadeMaterials",    newScenario));
}

void C4SWeather::Default()
{
	Climate.Set(50, 10);
	StartSeason.Set(50, 50);
	YearSpeed.Set(50);
	Rain.Default(); Lightning.Default(); Wind.Set(0, 70, -100, +100);
	SCopy("Water", Precipitation, C4M_MaxName);
	NoGamma = true;
}

void C4SWeather::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(Climate,                        "Climate",       C4SVal(50, 10), true));
	pComp->Value(mkNamingAdapt(StartSeason,                    "StartSeason",   C4SVal(50, 50), true));
	pComp->Value(mkNamingAdapt(YearSpeed,                      "YearSpeed",     C4SVal(50)));
	pComp->Value(mkNamingAdapt(Rain,                           "Rain",          C4SVal()));
	pComp->Value(mkNamingAdapt(Wind,                           "Wind",          C4SVal(0, 70, -100, +100), true));
	pComp->Value(mkNamingAdapt(Lightning,                      "Lightning",     C4SVal()));
	pComp->Value(mkNamingAdapt(mkStringAdaptMA(Precipitation), "Precipitation", "Water"));
	pComp->Value(mkNamingAdapt(NoGamma,                        "NoGamma",       true));
}

void C4SAnimals::Default()
{
	FreeLife.Clear();
	EarthNest.Clear();
}

void C4SAnimals::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(FreeLife,  "Animal", C4IDList()));
	pComp->Value(mkNamingAdapt(EarthNest, "Nest",   C4IDList()));
}

void C4SEnvironment::Default()
{
	Objects.Clear();
}

void C4SEnvironment::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(Objects, "Objects", C4IDList()));
}

void C4SRealism::Default()
{
	ConstructionNeedsMaterial = false;
	StructuresNeedEnergy = true;
	LandscapePushPull = 0;
	LandscapeInsertThrust = 0;
	ValueOverloads.Clear();
	BaseFunctionality = BASEFUNC_Default;
	BaseRegenerateEnergyPrice = BASE_RegenerateEnergyPrice;
}

void C4SDisasters::Default()
{
	Volcano.Default();
	Earthquake.Default();
	Meteorite.Default();
}

void C4SDisasters::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(Meteorite,  "Meteorite",  C4SVal()));
	pComp->Value(mkNamingAdapt(Volcano,    "Volcano",    C4SVal()));
	pComp->Value(mkNamingAdapt(Earthquake, "Earthquake", C4SVal()));
}

bool C4Scenario::Compile(const char *szSource, bool fLoadSection)
{
	if (!fLoadSection) Default();
	return CompileFromBuf_LogWarn<StdCompilerINIRead>(mkParAdapt(*this, fLoadSection), StdStrBuf::MakeRef(szSource), C4CFN_ScenarioCore);
}

void C4Scenario::Clear() {}

void C4Scenario::SetExactLandscape()
{
	if (Landscape.ExactLandscape) return;
	// Set landscape
	Landscape.ExactLandscape = true;
}

std::vector<std::string> C4SDefinitions::GetModules() const
{
	return LocalOnly ? std::vector<std::string>() : Definitions;
}

void C4SDefinitions::SetModules(const std::vector<std::string> &modules, const std::string &relativeToPath, const std::string &relativeToPath2)
{
	Definitions.clear();
	std::transform(modules.begin(), modules.end(), std::inserter(Definitions, Definitions.begin()), [relativeToPath, relativeToPath2](std::string def)
	{
		if (relativeToPath.size() && SEqualNoCase(def.c_str(), relativeToPath.c_str(), static_cast<int32_t>(relativeToPath.length())))
		{
			def = def.substr(relativeToPath.length());
		}
		if (relativeToPath2.size() && SEqualNoCase(def.c_str(), relativeToPath2.c_str(), static_cast<int32_t>(relativeToPath2.length())))
		{
			def = def.substr(relativeToPath2.length());
		}
		return def;
	});

	LocalOnly = Definitions.empty();
}

void C4SDefinitions::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(LocalOnly,       "LocalOnly",       false));
	pComp->Value(mkNamingAdapt(AllowUserChange, "AllowUserChange", false));
	pComp->Value(mkNamingAdapt(mkSTLContainerAdapt(Definitions), "Definitions", decltype(Definitions)()));

	if (Definitions.empty() && pComp->isCompiler())
	{
		for (size_t i = 0; i < C4S_MaxDefinitions; ++i)
		{
			std::string def;
			pComp->Value(mkNamingAdapt(mkStringAdaptA(def), std::format("Definition{}", i + 1).c_str(), ""));

			if (!def.empty())
			{
				Definitions.push_back(def);
			}
		}
	}

	pComp->Value(mkNamingAdapt(SkipDefs, "SkipDefs", C4IDList()));
}

void C4SGame::ConvertGoals(C4SRealism &rRealism)
{
	// Convert mode to goals
	switch (Mode)
	{
	case C4S_Melee:         Goals.SetIDCount(C4Id("MELE"), 1, true); ClearOldGoals(); break;
	case C4S_MeleeTeamwork: Goals.SetIDCount(C4Id("MELE"), 1, true); ClearOldGoals(); break;
	}
	Mode = 0;

	// Convert goals (master selection)
	switch (CooperativeGoal)
	{
	case C4S_Goldmine:    Goals.SetIDCount(C4Id("GLDM"), 1, true); ClearOldGoals(); break;
	case C4S_Monsterkill: Goals.SetIDCount(C4Id("MNTK"), 1, true); ClearOldGoals(); break;
	case C4S_ValueGain:   Goals.SetIDCount(C4Id("VALG"), (std::max)(ValueGain / 100, 1), true); ClearOldGoals(); break;
	}
	CooperativeGoal = 0;
	// CreateObjects,ClearObjects,ClearMaterials are still valid but invisible

	// Convert realism to rules
	if (rRealism.ConstructionNeedsMaterial) Rules.SetIDCount(C4Id("CNMT"), 1, true); rRealism.ConstructionNeedsMaterial = false;
	if (rRealism.StructuresNeedEnergy) Rules.SetIDCount(C4Id("ENRG"), 1, true); rRealism.StructuresNeedEnergy = false;

	// Convert rules
	if (EnableRemoveFlag) Rules.SetIDCount(C4Id("FGRV"), 1, true); EnableRemoveFlag = false;

	// Convert eliminiation to rules
	switch (Elimination)
	{
	case C4S_KillTheCaptain: Rules.SetIDCount(C4Id("KILC"), 1, true); break;
	case C4S_CaptureTheFlag: Rules.SetIDCount(C4Id("CTFL"), 1, true); break;
	}
	Elimination = 1; // unconvertible default crew elimination

	// CaptureTheFlag requires FlagRemoveable
	if (Rules.GetIDCount(C4Id("CTFL"))) Rules.SetIDCount(C4Id("FGRV"), 1, true);
}

void C4SGame::ClearOldGoals()
{
	CreateObjects.Clear(); ClearObjects.Clear(); ClearMaterial.Clear();
	ValueGain = 0;
}

bool C4SGame::IsMelee()
{
	return (Goals.GetIDCount(C4Id("MELE")) || Goals.GetIDCount(C4Id("MEL2")));
}

// scenario sections

const char *C4ScenSect_Main = "main";

C4ScenarioSection::C4ScenarioSection(char *szName)
	: Name{(szName && !SEqualNoCase(szName, C4ScenSect_Main) && *szName)
		? szName : C4ScenSect_Main}
{
	// zero fields
	fModified = false;
	// link into main list
	pNext = Game.pScenarioSections;
	Game.pScenarioSections = this;
}

C4ScenarioSection::~C4ScenarioSection()
{
	// del following scenario sections
	while (pNext)
	{
		C4ScenarioSection *pDel = pNext;
		pNext = pNext->pNext;
		pDel->pNext = nullptr;
		delete pDel;
	}
	// del temp file
	if (!TempFilename.empty()) EraseItem(TempFilename.c_str());
}

bool C4ScenarioSection::ScenarioLoad(char *szFilename)
{
	// safety
	if (!Filename.empty()) return false;
	// store name
	Filename = szFilename;
	// extract if it's not an open folder
	if (Game.ScenarioFile.IsPacked()) if (!EnsureTempStore(true, true)) return false;
	// donce, success
	return true;
}

C4Group *C4ScenarioSection::GetGroupfile(C4Group &rGrp)
{
	// check temp filename
	if (!TempFilename.empty()) if (rGrp.Open(TempFilename.c_str())) return &rGrp; else return nullptr;
	// check filename within scenario
	if (!Filename.empty()) if (rGrp.OpenAsChild(&Game.ScenarioFile, Filename.c_str())) return &rGrp; else return nullptr;
	// unmodified main section: return main group
	if (SEqualNoCase(Name.c_str(), C4ScenSect_Main)) return &Game.ScenarioFile;
	// failure
	return nullptr;
}

bool C4ScenarioSection::EnsureTempStore(bool fExtractLandscape, bool fExtractObjects)
{
	// if it's temp store already, don't do anything
	if (!TempFilename.empty()) return true;
	// make temp filename
	StdStrBuf tmp{Config.AtTempPath(!Filename.empty() ? Filename.c_str() : Name.c_str())};
	MakeTempFilename(&tmp);
	// main section: extract section files from main scenario group (create group as open dir)
	if (Filename.empty())
	{
		if (!MakeDirectory(tmp.getData(), nullptr)) return false;
		C4Group hGroup;
		if (!hGroup.Open(tmp.getData(), true)) { EraseItem(tmp.getData()); return false; }
		// extract all desired section files
		Game.ScenarioFile.ResetSearch();
		char fn[_MAX_FNAME + 1]; *fn = 0;
		while (Game.ScenarioFile.FindNextEntry(C4FLS_Section, fn))
			if (fExtractLandscape || !WildcardMatch(C4FLS_SectionLandscape, fn))
				if (fExtractObjects || !WildcardMatch(C4FLS_SectionObjects, fn))
					Game.ScenarioFile.ExtractEntry(fn, tmp.getData());
		hGroup.Close();
	}
	else
	{
		// subsection: simply extract section from main group
		if (!Game.ScenarioFile.ExtractEntry(Filename.c_str(), tmp.getData())) return false;
		// delete undesired landscape/object files
		if (!fExtractLandscape || !fExtractObjects)
		{
			C4Group hGroup;
			if (hGroup.Open(Filename.c_str()))
			{
				if (!fExtractLandscape) hGroup.Delete(C4FLS_SectionLandscape);
				if (!fExtractObjects) hGroup.Delete(C4FLS_SectionObjects);
			}
		}
	}
	// copy temp filename
	TempFilename = tmp.getData();
	// done, success
	return true;
}

const char *C4ScenarioSection::GetName() const
{
	return Name.c_str();
}

const char *C4ScenarioSection::GetTempFilename() const
{
	return TempFilename.c_str();
}
