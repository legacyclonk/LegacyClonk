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

/* Object definition */

#include <C4Include.h>
#include <C4Def.h>
#include <C4Version.h>
#include <C4GameVersion.h>
#include <C4FileMonitor.h>

#ifndef BIG_C4INCLUDE
#include <C4SurfaceFile.h>
#include <C4Log.h>
#include <C4Components.h>
#include <C4Config.h>
#include <C4ValueList.h>

#ifdef C4ENGINE
#include <C4Wrappers.h>
#include <C4Object.h>
#include "C4Network2Res.h"
#endif
#endif

#ifdef C4GROUP
#include "C4Group.h"
#include "C4Scenario.h"
#include "C4CompilerWrapper.h"
#endif

// Default Action Procedures

const char *ProcedureName[C4D_MaxDFA] =
{
	"WALK",
	"FLIGHT",
	"KNEEL",
	"SCALE",
	"HANGLE",
	"DIG",
	"SWIM",
	"THROW",
	"BRIDGE",
	"BUILD",
	"PUSH",
	"CHOP",
	"LIFT",
	"FLOAT",
	"ATTACH",
	"FIGHT",
	"CONNECT",
	"PULL"
};

// C4ActionDef

C4ActionDef::C4ActionDef()
{
	Default();
}

void C4ActionDef::Default()
{
	ZeroMem(this, sizeof(C4ActionDef));
	Procedure = DFA_NONE;
	NextAction = ActIdle;
	Directions = 1;
	FlipDir = 0;
	Length = 1;
	Delay = 0;
	FacetBase = 0;
	Step = 1;
	StartCall = PhaseCall = EndCall = AbortCall = nullptr;
}

void C4ActionDef::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(toC4CStr(Name),          "Name",       ""));
	pComp->Value(mkNamingAdapt(toC4CStr(ProcedureName), "Procedure",  ""));
	pComp->Value(mkNamingAdapt(Directions,              "Directions", 1));
	pComp->Value(mkNamingAdapt(FlipDir,                 "FlipDir",    0));
	pComp->Value(mkNamingAdapt(Length,                  "Length",     1));

	StdBitfieldEntry<int32_t> CNATs[] =
	{
		{ "CNAT_None",        CNAT_None },
		{ "CNAT_Left",        CNAT_Left },
		{ "CNAT_Right",       CNAT_Right },
		{ "CNAT_Top",         CNAT_Top },
		{ "CNAT_Bottom",      CNAT_Bottom },
		{ "CNAT_Center",      CNAT_Center },
		{ "CNAT_MultiAttach", CNAT_MultiAttach },
		{ "CNAT_NoCollision", CNAT_NoCollision },

		{ nullptr, 0 }
	};

	pComp->Value(mkNamingAdapt(mkBitfieldAdapt(Attach, CNATs),
		"Attach", 0));

	pComp->Value(mkNamingAdapt(Delay,                    "Delay",              0));
	pComp->Value(mkNamingAdapt(Facet,                    "Facet",              TargetRect0));
	pComp->Value(mkNamingAdapt(FacetBase,                "FacetBase",          0));
	pComp->Value(mkNamingAdapt(FacetTopFace,             "FacetTopFace",       0));
	pComp->Value(mkNamingAdapt(FacetTargetStretch,       "FacetTargetStretch", 0));
	pComp->Value(mkNamingAdapt(toC4CStr(NextActionName), "NextAction",         ""));
	pComp->Value(mkNamingAdapt(NoOtherAction,            "NoOtherAction",      0));
	pComp->Value(mkNamingAdapt(toC4CStr(SStartCall),     "StartCall",          ""));
	pComp->Value(mkNamingAdapt(toC4CStr(SEndCall),       "EndCall",            ""));
	pComp->Value(mkNamingAdapt(toC4CStr(SAbortCall),     "AbortCall",          ""));
	pComp->Value(mkNamingAdapt(toC4CStr(SPhaseCall),     "PhaseCall",          ""));
	pComp->Value(mkNamingAdapt(toC4CStr(Sound),          "Sound",              ""));
	pComp->Value(mkNamingAdapt(Disabled,                 "ObjectDisabled",     0));
	pComp->Value(mkNamingAdapt(DigFree,                  "DigFree",            0));
	pComp->Value(mkNamingAdapt(EnergyUsage,              "EnergyUsage",        0));
	pComp->Value(mkNamingAdapt(toC4CStr(InLiquidAction), "InLiquidAction",     ""));
	pComp->Value(mkNamingAdapt(toC4CStr(TurnAction),     "TurnAction",         ""));
	pComp->Value(mkNamingAdapt(Reverse,                  "Reverse",            0));
	pComp->Value(mkNamingAdapt(Step,                     "Step",               1));
}

// C4DefCore

C4DefCore::C4DefCore()
{
	Default();
}

void C4DefCore::Default()
{
	rC4XVer[0] = rC4XVer[1] = rC4XVer[2] = rC4XVer[3] = 0;
	RequireDef.Clear();
	Name.Ref("Undefined");
	Physical.Default();
	Shape.Default();
	Entrance.Default();
	Collection.Default();
	PictureRect.Default();
	SolidMask.Default();
	TopFace.Default();
	Component.Default();
	BurnTurnTo = C4ID_None;
	BuildTurnTo = C4ID_None;
	STimerCall[0] = 0;
	Timer = 35;
	ColorByMaterial[0] = 0;
	GrowthType = 0;
	Basement = 0;
	CanBeBase = 0;
	CrewMember = 0;
	NativeCrew = 0;
	Mass = 0;
	Value = 0;
	Exclusive = 0;
	Category = 0;
	Growth = 0;
	Rebuyable = 0;
	ContactIncinerate = 0;
	BlastIncinerate = 0;
	Constructable = 0;
	Grab = 0;
	Carryable = 0;
	Rotateable = 0;
	RotatedEntrance = 0;
	Chopable = 0;
	Float = 0;
	ColorByOwner = 0;
	NoHorizontalMove = 0;
	BorderBound = 0;
	LiftTop = 0;
	CollectionLimit = 0;
	GrabPutGet = 0;
	ContainBlast = 0;
	UprightAttach = 0;
	ContactFunctionCalls = 0;
	MaxUserSelect = 0;
	Line = 0;
	LineConnect = 0;
	LineIntersect = 0;
	NoBurnDecay = 0;
	IncompleteActivity = 0;
	Placement = 0;
	Prey = 0;
	Edible = 0;
	AttractLightning = 0;
	Oversize = 0;
	Fragile = 0;
	NoPushEnter = 0;
	Explosive = 0;
	Projectile = 0;
	DragImagePicture = 0;
	VehicleControl = 0;
	Pathfinder = 0;
	NoComponentMass = 0;
	MoveToRange = 0;
	NoStabilize = 0;
	ClosedContainer = 0;
	SilentCommands = 0;
	NoBurnDamage = 0;
	TemporaryCrew = 0;
	SmokeRate = 100;
	BlitMode = C4D_Blit_Normal;
	NoBreath = 0;
	ConSizeOff = 0;
	NoSell = NoGet = 0;
	NoFight = 0;
	RotatedSolidmasks = 0;
	NeededGfxMode = 0;
	NoTransferZones = 0;
}

bool C4DefCore::Load(C4Group &hGroup)
{
	StdStrBuf Source;
	if (hGroup.LoadEntryString(C4CFN_DefCore, Source))
	{
		StdStrBuf Name = hGroup.GetFullName() + (const StdStrBuf &)FormatString("%cDefCore.txt", DirectorySeparator);
		if (!Compile(Source.getData(), Name.getData()))
			return false;
		Source.Clear();

		// Adjust category: C4D_CrewMember by CrewMember flag
		if (CrewMember) Category |= C4D_CrewMember;

		// Adjust picture rect
		if ((PictureRect.Wdt == 0) || (PictureRect.Hgt == 0))
			PictureRect.Set(0, 0, Shape.Wdt, Shape.Hgt);

		// Check category
#ifdef C4ENGINE
		if (!(Category & C4D_SortLimit))
		{
			// special: Allow this for spells
			if (~Category & C4D_Magic)
				DebugLogF("WARNING: Def %s (%s) at %s has invalid category!", GetName(), C4IdText(id), hGroup.GetFullName().getData());
			// assign a default category here
			Category = (Category & ~C4D_SortLimit) | 1;
		}
		// Check mass
		if (Mass < 0)
		{
			DebugLogF("WARNING: Def %s (%s) at %s has invalid mass!", GetName(), C4IdText(id), hGroup.GetFullName().getData());
			Mass = 0;
		}
#endif

		return true;
	}
	return false;
}

bool C4DefCore::Compile(const char *szSource, const char *szName)
{
	return CompileFromBuf_LogWarn<StdCompilerINIRead>(mkNamingAdapt(*this, "DefCore"), StdStrBuf(szSource), szName);
}

void C4DefCore::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkC4IDAdapt(id),               "id",         C4ID_None));
	pComp->Value(mkNamingAdapt(toC4CArr(rC4XVer),             "Version"));
	pComp->Value(mkNamingAdapt(toC4CStrBuf(Name),             "Name",       "Undefined"));
	pComp->Value(mkNamingAdapt(mkParAdapt(RequireDef, false), "RequireDef", C4IDList()));

	const StdBitfieldEntry<int32_t> Categories[] =
	{
		{ "C4D_StaticBack", C4D_StaticBack },
		{ "C4D_Structure",  C4D_Structure },
		{ "C4D_Vehicle",    C4D_Vehicle },
		{ "C4D_Living",     C4D_Living },
		{ "C4D_Object",     C4D_Object },

		{ "C4D_Goal",             C4D_Goal },
		{ "C4D_Environment",      C4D_Environment },
		{ "C4D_SelectBuilding",   C4D_SelectBuilding },
		{ "C4D_SelectVehicle",    C4D_SelectVehicle },
		{ "C4D_SelectMaterial",   C4D_SelectMaterial },
		{ "C4D_SelectKnowledge",  C4D_SelectKnowledge },
		{ "C4D_SelectHomebase",   C4D_SelectHomebase },
		{ "C4D_SelectAnimal",     C4D_SelectAnimal },
		{ "C4D_SelectNest",       C4D_SelectNest },
		{ "C4D_SelectInEarth",    C4D_SelectInEarth },
		{ "C4D_SelectVegetation", C4D_SelectVegetation },

		{ "C4D_TradeLiving", C4D_TradeLiving },
		{ "C4D_Magic",       C4D_Magic },
		{ "C4D_CrewMember",  C4D_CrewMember },

		{ "C4D_Rule", C4D_Rule },

		{ "C4D_Background",  C4D_Background },
		{ "C4D_Parallax",    C4D_Parallax },
		{ "C4D_MouseSelect", C4D_MouseSelect },
		{ "C4D_Foreground",  C4D_Foreground },
		{ "C4D_MouseIgnore", C4D_MouseIgnore },
		{ "C4D_IgnoreFoW",   C4D_IgnoreFoW },

		{ nullptr, 0 }
	};

	pComp->Value(mkNamingAdapt(mkBitfieldAdapt<int32_t>(Category, Categories),
		"Category", 0));

	pComp->Value(mkNamingAdapt(MaxUserSelect,           "MaxUserSelect",     0));
	pComp->Value(mkNamingAdapt(Timer,                   "Timer",             35));
	pComp->Value(mkNamingAdapt(toC4CStr(STimerCall),    "TimerCall",         ""));
	pComp->Value(mkNamingAdapt(ContactFunctionCalls,    "ContactCalls",      0));
	pComp->Value(mkParAdapt(Shape,                      false));
	pComp->Value(mkNamingAdapt(Value,                   "Value",             0));
	pComp->Value(mkNamingAdapt(Mass,                    "Mass",              0));
	pComp->Value(mkNamingAdapt(Component,               "Components",        C4IDList()));
	pComp->Value(mkNamingAdapt(SolidMask,               "SolidMask",         TargetRect0));
	pComp->Value(mkNamingAdapt(TopFace,                 "TopFace",           TargetRect0));
#ifdef C4ENGINE
	pComp->Value(mkNamingAdapt(PictureRect,             "Picture",           Rect0));
	pComp->Value(mkNamingAdapt(StdNullAdapt(),          "PictureFE"));
#else
	pComp->Value(mkNamingAdapt(PictureRect,             "Picture",           Rect0));
	pComp->Value(mkNamingAdapt(PictureRectFE,           "PictureFE",         Rect0));
#endif
	pComp->Value(mkNamingAdapt(Entrance,                "Entrance",          Rect0));
	pComp->Value(mkNamingAdapt(Collection,              "Collection",        Rect0));
	pComp->Value(mkNamingAdapt(CollectionLimit,         "CollectionLimit",   0));
	pComp->Value(mkNamingAdapt(Placement,               "Placement",         0));
	pComp->Value(mkNamingAdapt(Exclusive,               "Exclusive",         0));
	pComp->Value(mkNamingAdapt(ContactIncinerate,       "ContactIncinerate", 0));
	pComp->Value(mkNamingAdapt(BlastIncinerate,         "BlastIncinerate",   0));
	pComp->Value(mkNamingAdapt(mkC4IDAdapt(BurnTurnTo), "BurnTo",            C4ID_None));
	pComp->Value(mkNamingAdapt(CanBeBase,               "Base",              0));

	const StdBitfieldEntry<int32_t> LineTypes[] =
	{
		{ "C4D_LinePower",     C4D_Line_Power },
		{ "C4D_LineSource",    C4D_Line_Source },
		{ "C4D_LineDrain",     C4D_Line_Drain },
		{ "C4D_LineLightning", C4D_Line_Lightning },
		{ "C4D_LineVolcano",   C4D_Line_Volcano },
		{ "C4D_LineRope",      C4D_Line_Rope },
		{ "C4D_LineColored",   C4D_Line_Colored },
		{ "C4D_LineVertex",    C4D_Line_Vertex },

		{ nullptr, 0 }
	};

	pComp->Value(mkNamingAdapt(mkBitfieldAdapt(Line, LineTypes), "Line", 0));

	const StdBitfieldEntry<int32_t> LineConnectTypes[] =
	{
		{ "C4D_PowerInput",     C4D_Power_Input },
		{ "C4D_PowerOutput",    C4D_Power_Output },
		{ "C4D_LiquidInput",    C4D_Liquid_Input },
		{ "C4D_LiquidOutput",   C4D_Liquid_Output },
		{ "C4D_PowerGenerator", C4D_Power_Generator },
		{ "C4D_PowerConsumer",  C4D_Power_Consumer },
		{ "C4D_LiquidPump",     C4D_Liquid_Pump },
		{ "C4D_ConnectRope",    C4D_Connect_Rope },
		{ "C4D_EnergyHolder",   C4D_EnergyHolder },

		{ nullptr, 0 }
	};

	pComp->Value(mkNamingAdapt(mkBitfieldAdapt(LineConnect, LineConnectTypes),
		"LineConnect", 0));

	pComp->Value(mkNamingAdapt(LineIntersect,            "LineIntersect",  0));
	pComp->Value(mkNamingAdapt(Prey,                     "Prey",           0));
	pComp->Value(mkNamingAdapt(Edible,                   "Edible",         0));
	pComp->Value(mkNamingAdapt(CrewMember,               "CrewMember",     0));
	pComp->Value(mkNamingAdapt(NativeCrew,               "NoStandardCrew", 0));
	pComp->Value(mkNamingAdapt(Growth,                   "Growth",         0));
	pComp->Value(mkNamingAdapt(Rebuyable,                "Rebuy",          0));
	pComp->Value(mkNamingAdapt(Constructable,            "Construction",   0));
	pComp->Value(mkNamingAdapt(mkC4IDAdapt(BuildTurnTo), "ConstructTo",    0));
	pComp->Value(mkNamingAdapt(Grab,                     "Grab",           0));

	const StdBitfieldEntry<int32_t> GrabPutGetTypes[] =
	{
		{ "C4D_GrabGet", C4D_Grab_Get },
		{ "C4D_GrabPut", C4D_Grab_Put },

		{ nullptr, 0 }
	};

	pComp->Value(mkNamingAdapt(mkBitfieldAdapt(GrabPutGet, GrabPutGetTypes),
		"GrabPutGet", 0));

	pComp->Value(mkNamingAdapt(Carryable,                 "Collectible",        0));
	pComp->Value(mkNamingAdapt(Rotateable,                "Rotate",             0));
	pComp->Value(mkNamingAdapt(RotatedEntrance,           "RotatedEntrance",    0));
	pComp->Value(mkNamingAdapt(Chopable,                  "Chop",               0));
	pComp->Value(mkNamingAdapt(Float,                     "Float",              0));
	pComp->Value(mkNamingAdapt(ContainBlast,              "ContainBlast",       0));
	pComp->Value(mkNamingAdapt(ColorByOwner,              "ColorByOwner",       0));
	pComp->Value(mkNamingAdapt(toC4CStr(ColorByMaterial), "ColorByMaterial",    ""));
	pComp->Value(mkNamingAdapt(NoHorizontalMove,          "HorizontalFix",      0));
	pComp->Value(mkNamingAdapt(BorderBound,               "BorderBound",        0));
	pComp->Value(mkNamingAdapt(LiftTop,                   "LiftTop",            0));
	pComp->Value(mkNamingAdapt(UprightAttach,             "UprightAttach",      0));
	pComp->Value(mkNamingAdapt(GrowthType,                "StretchGrowth",      0));
	pComp->Value(mkNamingAdapt(Basement,                  "Basement",           0));
	pComp->Value(mkNamingAdapt(NoBurnDecay,               "NoBurnDecay",        0));
	pComp->Value(mkNamingAdapt(IncompleteActivity,        "IncompleteActivity", 0));
	pComp->Value(mkNamingAdapt(AttractLightning,          "AttractLightning",   0));
	pComp->Value(mkNamingAdapt(Oversize,                  "Oversize",           0));
	pComp->Value(mkNamingAdapt(Fragile,                   "Fragile",            0));
	pComp->Value(mkNamingAdapt(Explosive,                 "Explosive",          0));
	pComp->Value(mkNamingAdapt(Projectile,                "Projectile",         0));
	pComp->Value(mkNamingAdapt(NoPushEnter,               "NoPushEnter",        0));
	pComp->Value(mkNamingAdapt(DragImagePicture,          "DragImagePicture",   0));
	pComp->Value(mkNamingAdapt(VehicleControl,            "VehicleControl",     0));
	pComp->Value(mkNamingAdapt(Pathfinder,                "Pathfinder",         0));
	pComp->Value(mkNamingAdapt(MoveToRange,               "MoveToRange",        0));
	pComp->Value(mkNamingAdapt(NoComponentMass,           "NoComponentMass",    0));
	pComp->Value(mkNamingAdapt(NoStabilize,               "NoStabilize",        0));
	pComp->Value(mkNamingAdapt(ClosedContainer,           "ClosedContainer",    0));
	pComp->Value(mkNamingAdapt(SilentCommands,            "SilentCommands",     0));
	pComp->Value(mkNamingAdapt(NoBurnDamage,              "NoBurnDamage",       0));
	pComp->Value(mkNamingAdapt(TemporaryCrew,             "TemporaryCrew",      0));
	pComp->Value(mkNamingAdapt(SmokeRate,                 "SmokeRate",          100));
	pComp->Value(mkNamingAdapt(BlitMode,                  "BlitMode",           C4D_Blit_Normal));
	pComp->Value(mkNamingAdapt(NoBreath,                  "NoBreath",           0));
	pComp->Value(mkNamingAdapt(ConSizeOff,                "ConSizeOff",         0));
	pComp->Value(mkNamingAdapt(NoSell,                    "NoSell",             0));
	pComp->Value(mkNamingAdapt(NoGet,                     "NoGet",              0));
	pComp->Value(mkNamingAdapt(NoFight,                   "NoFight",            0));
	pComp->Value(mkNamingAdapt(RotatedSolidmasks,         "RotatedSolidmasks",  0));
	pComp->Value(mkNamingAdapt(NoTransferZones,           "NoTransferZones",    0));
	pComp->Value(mkNamingAdapt(AutoContextMenu,           "AutoContextMenu",    0));
	pComp->Value(mkNamingAdapt(NeededGfxMode,             "NeededGfxMode",      0));

	const StdBitfieldEntry<int32_t> AllowPictureStackModes[] =
	{
		{ "APS_Color",    APS_Color },
		{ "APS_Graphics", APS_Graphics },
		{ "APS_Name",     APS_Name },
		{ "APS_Overlay",  APS_Overlay },
		{ nullptr,        0 }
	};

	pComp->Value(mkNamingAdapt(mkBitfieldAdapt<int32_t>(AllowPictureStack, AllowPictureStackModes),
		"AllowPictureStack", 0));

	pComp->FollowName("Physical");
	pComp->Value(Physical);
}

// C4Def

C4Def::C4Def()
{
#ifdef C4ENGINE
	Graphics.pDef = this;
#endif
	Default();
}

void C4Def::Default()
{
	C4DefCore::Default();

#if !defined(C4ENGINE) && !defined(C4GROUP)
	Picture = nullptr;
	Image = nullptr;
#endif
	ActNum = 0;
	ActMap = nullptr;
	Next = nullptr;
	Temporary = false;
	Maker[0] = 0;
	Filename[0] = 0;
	Creation = 0;
	Count = 0;
	TimerCall = nullptr;
#ifdef C4ENGINE
	MainFace.Set(nullptr, 0, 0, 0, 0);
	Script.Default();
	StringTable.Default();
	pClonkNames = nullptr;
	pRankNames = nullptr;
	pRankSymbols = nullptr;
	fClonkNamesOwned = fRankNamesOwned = fRankSymbolsOwned = false;
	iNumRankSymbols = 1;
	PortraitCount = 0;
	Portraits = nullptr;
	pFairCrewPhysical = nullptr;
#endif
}

C4Def::~C4Def()
{
	Clear();
}

void C4Def::Clear()
{
#ifdef C4ENGINE
	Graphics.Clear();

	Script.Clear();
	StringTable.Clear();
	if (pClonkNames  && fClonkNamesOwned)  delete pClonkNames;  pClonkNames  = nullptr;
	if (pRankNames   && fRankNamesOwned)   delete pRankNames;   pRankNames   = nullptr;
	if (pRankSymbols && fRankSymbolsOwned) delete pRankSymbols; pRankSymbols = nullptr;
	if (pFairCrewPhysical) { delete pFairCrewPhysical; pFairCrewPhysical = nullptr; }
	fClonkNamesOwned = fRankNamesOwned = fRankSymbolsOwned = false;

	PortraitCount = 0;
	Portraits = nullptr;

#endif

	if (ActMap) delete[] ActMap; ActMap = nullptr;
	Desc.Clear();
}

bool C4Def::Load(C4Group &hGroup,
	uint32_t dwLoadWhat,
	const char *szLanguage,
	C4SoundSystem *pSoundSystem)
{
	bool fSuccess = true;

#ifdef C4ENGINE
	bool AddFileMonitoring = false;
	if (Game.pFileMonitor && !SEqual(hGroup.GetFullName().getData(), Filename) && !hGroup.IsPacked())
		AddFileMonitoring = true;
#endif

	// Store filename, maker, creation
	SCopy(hGroup.GetFullName().getData(), Filename);
	SCopy(hGroup.GetMaker(), Maker, C4MaxName);
	Creation = hGroup.GetCreation();

#ifdef C4ENGINE
	// Verbose log filename
	if (Config.Graphics.VerboseObjectLoading >= 3)
		Log(hGroup.GetFullName().getData());

	if (AddFileMonitoring) Game.pFileMonitor->AddDirectory(Filename);

	// particle def?
	if (hGroup.AccessEntry(C4CFN_ParticleCore))
	{
		// def loading not successful; abort after reading sounds
		fSuccess = false;
		// create new particle def
		C4ParticleDef *pParticleDef = new C4ParticleDef();
		// load it
		if (!pParticleDef->Load(hGroup))
		{
			// not successful :( - destroy it again
			delete pParticleDef;
		}
		// done
	}

#endif

	// Read DefCore
	if (fSuccess) fSuccess = C4DefCore::Load(hGroup);
	// check id
	if (fSuccess) if (!LooksLikeID(id))
	{
#ifdef C4ENGINE
		// wie geth ID?????ßßßß
		if (!Name[0]) Name = GetFilename(hGroup.GetName());
		sprintf(OSTR, LoadResStr("IDS_ERR_INVALIDID"), Name.getData());
		Log(OSTR);
#endif
		fSuccess = false;
	}

#ifdef C4ENGINE
	// skip def: don't even read sounds!
	if (fSuccess && Game.C4S.Definitions.SkipDefs.GetIDCount(id, 1)) return false;

	// OldGfx is no longer supported
	if (NeededGfxMode == C4DGFXMODE_OLDGFX) return false;
#endif

	if (!fSuccess)
	{
#ifdef C4ENGINE
		// Read sounds even if not a valid def (for pure c4d sound folders)
		if (dwLoadWhat & C4D_Load_Sounds)
			if (pSoundSystem)
				pSoundSystem->LoadEffects(hGroup);
#endif

		return false;
	}

#ifdef C4ENGINE
	// Read surface bitmap
	if (dwLoadWhat & C4D_Load_Bitmap)
		if (!Graphics.LoadBitmaps(hGroup, !!ColorByOwner))
		{
			DebugLogF("  Error loading graphics of %s (%s)", hGroup.GetFullName().getData(), C4IdText(id));
			return false;
		}

	// Read portraits
	if (dwLoadWhat & C4D_Load_Bitmap)
		if (!LoadPortraits(hGroup))
		{
			DebugLogF("  Error loading portrait graphics of %s (%s)", hGroup.GetFullName().getData(), C4IdText(id));
			return false;
		}

#endif

#if !defined(C4ENGINE) && !defined(C4GROUP)

	// Override PictureRect if PictureRectFE is given
	if (PictureRectFE.Wdt > 0)
		PictureRect = PictureRectFE;

	// Read picture section (this option is currently unused...)
	if (dwLoadWhat & C4D_Load_Picture)
		// Load from PNG graphics
		if (!hGroup.AccessEntry(C4CFN_DefGraphicsPNG)
			|| !hGroup.ReadPNGSection(&Picture, nullptr, PictureRect.x, PictureRect.y, PictureRect.Wdt, PictureRect.Hgt))
			// Load from BMP graphics
			if (!hGroup.AccessEntry(C4CFN_DefGraphics)
				|| !hGroup.ReadDDBSection(&Picture, nullptr, PictureRect.x, PictureRect.y, PictureRect.Wdt, PictureRect.Hgt))
				// None loaded
				return false;

	// Read picture section for use in image list
	if (dwLoadWhat & C4D_Load_Image)
		// Load from PNG title
		if (!hGroup.AccessEntry(C4CFN_ScenarioTitlePNG)
			|| !hGroup.ReadPNGSection(&Image, nullptr, -1, -1, -1, -1, 32, 32))
			// Load from BMP title
			if (!hGroup.AccessEntry(C4CFN_ScenarioTitle)
				|| !hGroup.ReadDDBSection(&Image, nullptr, -1, -1, -1, -1, 32, 32, true))
				// Load from PNG graphics
				if (!hGroup.AccessEntry(C4CFN_DefGraphicsPNG)
					|| !hGroup.ReadPNGSection(&Image, nullptr, PictureRect.x, PictureRect.y, PictureRect.Wdt, PictureRect.Hgt, 32, 32))
					// Load from BMP graphics
					if (!hGroup.AccessEntry(C4CFN_DefGraphics)
						|| !hGroup.ReadDDBSection(&Image, nullptr, PictureRect.x, PictureRect.y, PictureRect.Wdt, PictureRect.Hgt, 32, 32, true))
						// None loaded
						return false;
#endif

#ifdef C4ENGINE
	// Read ActMap
	if (dwLoadWhat & C4D_Load_ActMap)
		if (!LoadActMap(hGroup))
		{
			DebugLogF("  Error loading ActMap of %s (%s)", hGroup.GetFullName().getData(), C4IdText(id));
			return false;
		}
#endif

#ifdef C4ENGINE
	// Read script
	if (dwLoadWhat & C4D_Load_Script)
	{
		// reg script to engine
		Script.Reg2List(&Game.ScriptEngine, &Game.ScriptEngine);
		// Load script - loads string table as well, because that must be done after script load
		// for downwards compatibility with packing order
		Script.Load("Script", hGroup, C4CFN_Script, szLanguage, this, &StringTable, true);
	}
#endif

	// Read name
	C4ComponentHost DefNames;
	if (DefNames.LoadEx("Names", hGroup, C4CFN_DefNames, szLanguage))
		DefNames.GetLanguageString(szLanguage, Name);
	DefNames.Close();

#ifdef C4ENGINE
	// read clonknames
	if (dwLoadWhat & C4D_Load_ClonkNames)
	{
		// clear any previous
		if (pClonkNames) delete pClonkNames; pClonkNames = nullptr;
		if (hGroup.FindEntry(C4CFN_ClonkNameFiles))
		{
			// create new
			pClonkNames = new C4ComponentHost();
			if (!pClonkNames->LoadEx(LoadResStr("IDS_CNS_NAMES"), hGroup, C4CFN_ClonkNames, szLanguage))
			{
				delete pClonkNames; pClonkNames = nullptr;
			}
			else
				fClonkNamesOwned = true;
		}
	}

	// read clonkranks
	if (dwLoadWhat & C4D_Load_RankNames)
	{
		// clear any previous
		if (pRankNames) delete pRankNames; pRankNames = nullptr;
		if (hGroup.FindEntry(C4CFN_RankNameFiles))
		{
			// create new
			pRankNames = new C4RankSystem();
			// load from group
			if (!pRankNames->Load(hGroup, C4CFN_RankNames, 1000, szLanguage))
			{
				delete pRankNames; pRankNames = nullptr;
			}
			else
				fRankNamesOwned = true;
		}
	}

	// read rankfaces
	if (dwLoadWhat & C4D_Load_RankFaces)
	{
		// clear any previous
		if (pRankSymbols) delete pRankSymbols; pRankSymbols = nullptr;
		// load new: try png first
		if (hGroup.AccessEntry(C4CFN_RankFacesPNG))
		{
			pRankSymbols = new C4FacetExSurface();
			if (!pRankSymbols->GetFace().ReadPNG(hGroup)) { delete pRankSymbols; pRankSymbols = nullptr; }
		}
		else if (hGroup.AccessEntry(C4CFN_RankFaces))
		{
			pRankSymbols = new C4FacetExSurface();
			if (!pRankSymbols->GetFace().Read(hGroup)) { delete pRankSymbols; pRankSymbols = nullptr; }
		}
		// set size
		if (pRankSymbols)
		{
			pRankSymbols->Set(&pRankSymbols->GetFace(), 0, 0, pRankSymbols->GetFace().Hgt, pRankSymbols->GetFace().Hgt);
			int32_t Q; pRankSymbols->GetPhaseNum(iNumRankSymbols, Q);
			if (!iNumRankSymbols) { delete pRankSymbols; pRankSymbols = nullptr; }
			else
			{
				if (pRankNames)
				{
					// if extended rank names are defined, subtract those from the symbol count. The last symbols are used as overlay
					iNumRankSymbols = Max<int32_t>(1, iNumRankSymbols - pRankNames->GetExtendedRankNum());
				}
				fRankSymbolsOwned = true;
			}
		}
	}

#endif

	// Read desc
	if (dwLoadWhat & C4D_Load_Desc)
	{
		Desc.LoadEx("Desc", hGroup, C4CFN_DefDesc, szLanguage);
		Desc.TrimSpaces();
	}

#ifdef C4ENGINE

	// Read sounds
	if (dwLoadWhat & C4D_Load_Sounds)
		if (pSoundSystem)
			pSoundSystem->LoadEffects(hGroup);

	// Bitmap post-load settings
	if (Graphics.GetBitmap())
	{
		// check SolidMask
		if (SolidMask.x < 0 || SolidMask.y < 0 || SolidMask.x + SolidMask.Wdt > Graphics.Bitmap->Wdt || SolidMask.y + SolidMask.Hgt > Graphics.Bitmap->Hgt) SolidMask.Default();
		// Set MainFace (unassigned bitmap: will be set by GetMainFace())
		MainFace.Set(nullptr, 0, 0, Shape.Wdt, Shape.Hgt);
	}

	// validate TopFace
	if (TopFace.x < 0 || TopFace.y < 0 || TopFace.x + TopFace.Wdt > Graphics.Bitmap->Wdt || TopFace.y + TopFace.Hgt > Graphics.Bitmap->Hgt)
	{
		TopFace.Default();
		// warn in debug mode
		DebugLogF("invalid TopFace in %s(%s)", Name.getData(), C4IdText(id));
	}

#endif

	// Temporary flag
	if (dwLoadWhat & C4D_Load_Temporary) Temporary = true;

	return true;
}

bool C4Def::LoadActMap(C4Group &hGroup)
{
	// New format
	StdStrBuf Data;
	if (hGroup.LoadEntryString(C4CFN_DefActMap, Data))
	{
		// Get action count (hacky), create buffer
		int actnum;
		if (!(actnum = SCharCount('[', Data.getData()))
			|| !(ActMap = new C4ActionDef[actnum]))
			return false;
		// Compile
		if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(
			mkNamingAdapt(mkArrayAdapt(ActMap, actnum), "Action"),
			Data,
			(hGroup.GetFullName() + DirSep C4CFN_DefActMap).getData()))
			return false;
		ActNum = actnum;
		// Process map
		CrossMapActMap();
		return true;
	}

	// No act map in group: okay
	return true;
}

void C4Def::CrossMapActMap()
{
	int32_t cnt, cnt2;
	for (cnt = 0; cnt < ActNum; cnt++)
	{
		// Map standard procedures
		ActMap[cnt].Procedure = DFA_NONE;
		for (cnt2 = 0; cnt2 < C4D_MaxDFA; cnt2++)
			if (SEqual(ActMap[cnt].ProcedureName, ProcedureName[cnt2]))
				ActMap[cnt].Procedure = cnt2;
		// Map next action
		if (ActMap[cnt].NextActionName[0])
		{
			if (SEqualNoCase(ActMap[cnt].NextActionName, "Hold"))
				ActMap[cnt].NextAction = ActHold;
			else
				for (cnt2 = 0; cnt2 < ActNum; cnt2++)
					if (SEqual(ActMap[cnt].NextActionName, ActMap[cnt2].Name))
						ActMap[cnt].NextAction = cnt2;
		}
		// Check act calls
		if (SEqualNoCase(ActMap[cnt].SStartCall, "None")) ActMap[cnt].SStartCall[0] = 0;
		if (SEqualNoCase(ActMap[cnt].SPhaseCall, "None")) ActMap[cnt].SPhaseCall[0] = 0;
		if (SEqualNoCase(ActMap[cnt].SEndCall,   "None")) ActMap[cnt].SEndCall  [0] = 0;
		if (SEqualNoCase(ActMap[cnt].SAbortCall, "None")) ActMap[cnt].SAbortCall[0] = 0;
	}
}

bool C4Def::ColorizeByMaterial(C4MaterialMap &rMats, uint8_t bGBM)
{
#ifdef C4ENGINE
	if (ColorByMaterial[0])
	{
		int32_t mat = rMats.Get(ColorByMaterial);
		if (mat == MNone) { sprintf(OSTR, "C4Def::ColorizeByMaterial: mat %s not defined", ColorByMaterial); Log(OSTR); return false; }
		if (!Graphics.ColorizeByMaterial(mat, rMats, bGBM)) return false;
	}
#endif
	// success
	return true;
}

void C4Def::Draw(C4Facet &cgo, bool fSelected, uint32_t iColor, C4Object *pObj, int32_t iPhaseX, int32_t iPhaseY)
{
#ifdef C4ENGINE

	// default: def picture rect
	C4Rect fctPicRect = PictureRect;
	C4Facet fctPicture;

	// if assigned: use object specific rect and graphics
	if (pObj) if (pObj->PictureRect.Wdt) fctPicRect = pObj->PictureRect;

	fctPicture.Set((pObj ? *pObj->GetGraphics() : Graphics).GetBitmap(iColor), fctPicRect.x, fctPicRect.y, fctPicRect.Wdt, fctPicRect.Hgt);

	if (fSelected)
		Application.DDraw->DrawBox(cgo.Surface, cgo.X, cgo.Y, cgo.X + cgo.Wdt - 1, cgo.Y + cgo.Hgt - 1, CRed);

	// specific object color?
	if (pObj) pObj->PrepareDrawing();
	fctPicture.Draw(cgo, true, iPhaseX, iPhaseY, true);
	if (pObj) pObj->FinishedDrawing();

	// draw overlays
	if (pObj && pObj->pGfxOverlay)
		for (C4GraphicsOverlay *pGfxOvrl = pObj->pGfxOverlay; pGfxOvrl; pGfxOvrl = pGfxOvrl->GetNext())
			if (pGfxOvrl->IsPicture())
				pGfxOvrl->DrawPicture(cgo, pObj);
#endif
}

#ifdef C4ENGINE
int32_t C4Def::GetValue(C4Object *pInBase, int32_t iBuyPlayer)
{
	// CalcDefValue defined?
	C4AulFunc *pCalcValueFn = Script.GetSFunc(PSF_CalcDefValue, AA_PROTECTED);
	int32_t iValue;
	if (pCalcValueFn)
		// then call it!
		iValue = pCalcValueFn->Exec(nullptr, &C4AulParSet(C4VObj(pInBase), C4VInt(iBuyPlayer))).getInt();
	else
		// otherwise, use default value
		iValue = Value;
	// do any adjustments based on where the item is bought
	if (pInBase)
	{
		C4AulFunc *pFn;
		if (pFn = pInBase->Def->Script.GetSFunc(PSF_CalcBuyValue, AA_PROTECTED))
			iValue = pFn->Exec(pInBase, &C4AulParSet(C4VID(id), C4VInt(iValue))).getInt();
	}
	return iValue;
}

C4PhysicalInfo *C4Def::GetFairCrewPhysicals()
{
	// if fair crew physicals have been created, assume they are valid
	if (!pFairCrewPhysical)
	{
		pFairCrewPhysical = new C4PhysicalInfo(Physical);
		// determine the rank
		int32_t iExpGain = Game.Parameters.FairCrewStrength;
		C4RankSystem *pRankSys = &Game.Rank;
		if (pRankNames) pRankSys = pRankNames;
		int32_t iRank = pRankSys->RankByExperience(iExpGain);
		// promote physicals for rank
		pFairCrewPhysical->PromotionUpdate(iRank, true, this);
	}
	return pFairCrewPhysical;
}

void C4Def::ClearFairCrewPhysicals()
{
	// invalidate physicals so the next call to GetFairCrewPhysicals will
	// reacreate them
	delete pFairCrewPhysical; pFairCrewPhysical = nullptr;
}

void C4Def::Synchronize()
{
	// because recreation of fair crew physicals does a script call, which *might* do a call to e.g. Random
	// fair crew physicals must be cleared and recalculated for everyone
	ClearFairCrewPhysicals();
}

#endif

// C4DefList

C4DefList::C4DefList()
{
	Default();
}

C4DefList::~C4DefList()
{
	Clear();
}

int32_t C4DefList::Load(C4Group &hGroup, uint32_t dwLoadWhat,
	const char *szLanguage,
	C4SoundSystem *pSoundSystem,
	bool fOverload,
	bool fSearchMessage, int32_t iMinProgress, int32_t iMaxProgress, bool fLoadSysGroups)
{
	int32_t iResult = 0;
	C4Def *nDef;
	char szEntryname[_MAX_FNAME + 1];
	C4Group hChild;
	bool fPrimaryDef = false;
	bool fThisSearchMessage = false;

	// This search message
	if (fSearchMessage)
		if (SEqualNoCase(GetExtension(hGroup.GetName()), "c4d")
			|| SEqualNoCase(GetExtension(hGroup.GetName()), "c4s")
			|| SEqualNoCase(GetExtension(hGroup.GetName()), "c4f"))
		{
			fThisSearchMessage = true;
			fSearchMessage = false;
		}

#ifdef C4ENGINE // Message
	if (fThisSearchMessage) { sprintf(OSTR, "%s...", GetFilename(hGroup.GetName())); Log(OSTR); }
#endif

	// Load primary definition
	if (nDef = new C4Def)
		if (nDef->Load(hGroup, dwLoadWhat, szLanguage, pSoundSystem) && Add(nDef, fOverload))
		{
			iResult++; fPrimaryDef = true;
		}
		else
		{
			delete nDef;
		}

	// Load sub definitions
	int i = 0;
	hGroup.ResetSearch();
	while (hGroup.FindNextEntry(C4CFN_DefFiles, szEntryname))
		if (hChild.OpenAsChild(&hGroup, szEntryname))
		{
			// Hack: Assume that there are sixteen sub definitions to avoid unnecessary I/O
			int iSubMinProgress = Min<int32_t>(iMaxProgress, iMinProgress + ((iMaxProgress - iMinProgress) * i) / 16);
			int iSubMaxProgress = Min<int32_t>(iMaxProgress, iMinProgress + ((iMaxProgress - iMinProgress) * (i + 1)) / 16);
			++i;
			iResult += Load(hChild, dwLoadWhat, szLanguage, pSoundSystem, fOverload, fSearchMessage, iSubMinProgress, iSubMaxProgress);
			hChild.Close();
		}

	// load additional system scripts for def groups only
#ifdef C4ENGINE
	C4Group SysGroup;
	char fn[_MAX_FNAME + 1] = { 0 };
	if (!fPrimaryDef && fLoadSysGroups) if (SysGroup.OpenAsChild(&hGroup, C4CFN_System))
	{
		C4LangStringTable SysGroupString;
		SysGroupString.LoadEx("StringTbl", SysGroup, C4CFN_ScriptStringTbl, Config.General.LanguageEx);
		// load all scripts in there
		SysGroup.ResetSearch();
		while (SysGroup.FindNextEntry(C4CFN_ScriptFiles, (char *)&fn, nullptr, nullptr, !!fn[0]))
		{
			// host will be destroyed by script engine, so drop the references
			C4ScriptHost *scr = new C4ScriptHost();
			scr->Reg2List(&Game.ScriptEngine, &Game.ScriptEngine);
			scr->Load(nullptr, SysGroup, fn, Config.General.LanguageEx, nullptr, &SysGroupString);
		}
		// if it's a physical group: watch out for changes
		if (!SysGroup.IsPacked() && Game.pFileMonitor)
			Game.pFileMonitor->AddDirectory(SysGroup.GetFullName().getData());
		SysGroup.Close();
	}
#endif

#ifdef C4ENGINE // Message
	if (fThisSearchMessage) { sprintf(OSTR, LoadResStr("IDS_PRC_DEFSLOADED"), iResult); Log(OSTR); }

	// progress (could go down one level of recursion...)
	if (iMinProgress != iMaxProgress) Game.SetInitProgress(float(iMaxProgress));
#endif

	return iResult;
}

int32_t C4DefList::Load(const char *szSearch,
	uint32_t dwLoadWhat, const char *szLanguage,
	C4SoundSystem *pSoundSystem,
	bool fOverload, int32_t iMinProgress, int32_t iMaxProgress)
{
	int32_t iResult = 0;

	// Empty
	if (!szSearch[0]) return iResult;

	// Segments
	char szSegment[_MAX_PATH + 1]; int32_t iGroupCount;
	if (iGroupCount = SCharCount(';', szSearch))
	{
		++iGroupCount; int32_t iPrg = iMaxProgress - iMinProgress;
		for (int32_t cseg = 0; SCopySegment(szSearch, cseg, szSegment, ';', _MAX_PATH); cseg++)
			iResult += Load(szSegment, dwLoadWhat, szLanguage, pSoundSystem, fOverload,
				iMinProgress + iPrg * cseg / iGroupCount, iMinProgress + iPrg * (cseg + 1) / iGroupCount);
		return iResult;
	}

	// Wildcard items
	if (SCharCount('*', szSearch))
	{
#ifdef _WIN32
		struct _finddata_t fdt; int32_t fdthnd;
		if ((fdthnd = _findfirst(szSearch, &fdt)) < 0) return false;
		do
		{
			iResult += Load(fdt.name, dwLoadWhat, szLanguage, pSoundSystem, fOverload);
		} while (_findnext(fdthnd, &fdt) == 0);
		_findclose(fdthnd);
#ifdef C4ENGINE
		// progress
		if (iMinProgress != iMaxProgress) Game.SetInitProgress(float(iMaxProgress));
#endif
#else
		fputs("FIXME: C4DefList::Load\n", stderr);
#endif
		return iResult;
	}

	// File specified with creation (currently not used)
	char szCreation[25 + 1];
	int32_t iCreation = 0;
	if (SCopyEnclosed(szSearch, '(', ')', szCreation, 25))
	{
		// Scan creation
		SClearFrontBack(szCreation);
		sscanf(szCreation, "%i", &iCreation);
		// Extract filename
		SCopyUntil(szSearch, szSegment, '(', _MAX_PATH);
		SClearFrontBack(szSegment);
		szSearch = szSegment;
	}

	// Load from specified file
	C4Group hGroup;
	if (!hGroup.Open(szSearch))
	{
		// Specified file not found (failure)
#ifdef C4ENGINE
		LogFatal(FormatString(LoadResStr("IDS_PRC_DEFNOTFOUND"), szSearch).getData());
#endif
		LoadFailure = true;
		return iResult;
	}
	iResult += Load(hGroup, dwLoadWhat, szLanguage, pSoundSystem, fOverload, true, iMinProgress, iMaxProgress);
	hGroup.Close();

#ifdef C4ENGINE
	// progress (could go down one level of recursion...)
	if (iMinProgress != iMaxProgress) Game.SetInitProgress(float(iMaxProgress));
#endif

	return iResult;
}

bool C4DefList::Add(C4Def *pDef, bool fOverload)
{
	if (!pDef) return false;

	// Check old def to overload
	C4Def *pLastDef = ID2Def(pDef->id);
	if (pLastDef && !fOverload) return false;

#ifdef C4ENGINE
	// Log overloaded def
	if (Config.Graphics.VerboseObjectLoading >= 1)
		if (pLastDef)
		{
			LogF(LoadResStr("IDS_PRC_DEFOVERLOAD"), pDef->GetName(), C4IdText(pLastDef->id));
			if (Config.Graphics.VerboseObjectLoading >= 2)
			{
				LogF("      Old def at %s", pLastDef->Filename);
				LogF("     Overload by %s", pDef->Filename);
			}
		}
#endif

	// Remove old def
	Remove(pDef->id);

	// Add new def
	pDef->Next = FirstDef;
	FirstDef = pDef;

	return true;
}

bool C4DefList::Remove(C4ID id)
{
	C4Def *cdef, *prev;
	for (cdef = FirstDef, prev = nullptr; cdef; prev = cdef, cdef = cdef->Next)
		if (cdef->id == id)
		{
			if (prev) prev->Next = cdef->Next;
			else FirstDef = cdef->Next;
			delete cdef;
			return true;
		}
	return false;
}

void C4DefList::Remove(C4Def *def)
{
	C4Def *cdef, *prev;
	for (cdef = FirstDef, prev = nullptr; cdef; prev = cdef, cdef = cdef->Next)
		if (cdef == def)
		{
			if (prev) prev->Next = cdef->Next;
			else FirstDef = cdef->Next;
			delete cdef;
			return;
		}
}

void C4DefList::Clear()
{
	C4Def *cdef, *next;
	for (cdef = FirstDef; cdef; cdef = next)
	{
		next = cdef->Next;
		delete cdef;
	}
	FirstDef = nullptr;
	// clear quick access table
	for (int32_t i = 0; i < 64; i++) if (Table[i]) { delete[] Table[i]; Table[i] = nullptr; }
	fTable = false;
}

C4Def *C4DefList::ID2Def(C4ID id)
{
	if (id == C4ID_None) return nullptr;
	if (!fTable)
	{
		// table not yet built: search list
		C4Def *cdef;
		for (cdef = FirstDef; cdef; cdef = cdef->Next)
			if (cdef->id == id) return cdef;
	}
	C4Def **ppDef, ***pppDef = Table;
	// get table entry to query
	int32_t iTblIndex = (id >> 24) - 32;
	if (Inside<int32_t>(iTblIndex, 0, 63)) pppDef += iTblIndex;
	// no entry matching?
	if (!(ppDef = *pppDef)) return nullptr;
	// search list
	for (C4Def *pDef = *ppDef; pDef = *ppDef; ppDef++)
	{
		if (pDef->id == id) return pDef;
	}
	// none found
	return nullptr;
}

int32_t C4DefList::GetIndex(C4ID id)
{
	C4Def *cdef;
	int32_t cindex;
	for (cdef = FirstDef, cindex = 0; cdef; cdef = cdef->Next, cindex++)
		if (cdef->id == id) return cindex;
	return -1;
}

int32_t C4DefList::GetDefCount(uint32_t dwCategory)
{
	C4Def *cdef; int32_t ccount = 0;
	for (cdef = FirstDef; cdef; cdef = cdef->Next)
		if (cdef->Category & dwCategory)
			ccount++;
	return ccount;
}

C4Def *C4DefList::GetDef(int32_t iIndex, uint32_t dwCategory)
{
	C4Def *pDef; int32_t iCurrentIndex;
	if (iIndex < 0) return nullptr;
	for (pDef = FirstDef, iCurrentIndex = -1; pDef; pDef = pDef->Next)
		if (pDef->Category & dwCategory)
		{
			iCurrentIndex++;
			if (iCurrentIndex == iIndex) return pDef;
		}
	return nullptr;
}

#ifdef C4ENGINE
C4Def *C4DefList::GetByPath(const char *szPath)
{
	// search defs
	const char *szDefPath;
	for (C4Def *pDef = FirstDef; pDef; pDef = pDef->Next)
		if (szDefPath = Config.AtExeRelativePath(pDef->Filename))
			if (SEqual2NoCase(szPath, szDefPath))
				// the definition itself?
				if (!szPath[SLen(szDefPath)])
					return pDef;
	// or a component?
				else if (szPath[SLen(szDefPath)] == '\\')
					if (!strchr(szPath + SLen(szDefPath) + 1, '\\'))
						return pDef;
	// not found
	return nullptr;
}
#endif

int32_t C4DefList::CheckEngineVersion(int32_t ver1, int32_t ver2, int32_t ver3, int32_t ver4)
{
	int32_t rcount = 0;
	C4Def *cdef, *prev, *next;
	for (cdef = FirstDef, prev = nullptr; cdef; cdef = next)
	{
		next = cdef->Next;
		if (CompareVersion(cdef->rC4XVer[0], cdef->rC4XVer[1], cdef->rC4XVer[2], cdef->rC4XVer[3], ver1, ver2, ver3, ver4) > 0)
		{
			if (prev) prev->Next = cdef->Next;
			else FirstDef = cdef->Next;
			delete cdef;
			rcount++;
		}
		else prev = cdef;
	}
	return rcount;
}

int32_t C4DefList::CheckRequireDef()
{
	int32_t rcount = 0, rcount2;
	C4Def *cdef, *prev, *next;
	do
	{
		rcount2 = rcount;
		for (cdef = FirstDef, prev = nullptr; cdef; cdef = next)
		{
			next = cdef->Next;
			for (int32_t i = 0; i < cdef->RequireDef.GetNumberOfIDs(); i++)
				if (GetIndex(cdef->RequireDef.GetID(i)) < 0)
				{
					(prev ? prev->Next : FirstDef) = cdef->Next;
					delete cdef;
					rcount++;
				}
		}
	} while (rcount != rcount2);
	return rcount;
}

int32_t C4DefList::ColorizeByMaterial(C4MaterialMap &rMats, uint8_t bGBM)
{
	C4Def *cdef;
	int32_t rval = 0;
	for (cdef = FirstDef; cdef; cdef = cdef->Next)
		if (cdef->ColorizeByMaterial(rMats, bGBM))
			rval++;
	return rval;
}

void C4DefList::Draw(C4ID id, C4Facet &cgo, bool fSelected, int32_t iColor)
{
	C4Def *cdef = ID2Def(id);
	if (cdef) cdef->Draw(cgo, fSelected, iColor);
}

void C4DefList::Default()
{
	FirstDef = nullptr;
	LoadFailure = false;
	ZeroMem(&Table, sizeof(Table));
	fTable = false;
}

bool C4DefList::Reload(C4Def *pDef, uint32_t dwLoadWhat, const char *szLanguage, C4SoundSystem *pSoundSystem)
{
	// Safety
	if (!pDef) return false;
#ifdef C4ENGINE
	// backup graphics names and pointers
	// GfxBackup-dtor will ensure that upon loading-failure all graphics are reset to default
	C4DefGraphicsPtrBackup GfxBackup(&pDef->Graphics);
	// clear any pointers into def (name)
	Game.Objects.ClearDefPointers(pDef);
#endif
	// Clear def
	pDef->Clear(); // Assume filename is being kept
	// Reload def
	C4Group hGroup;
	if (!hGroup.Open(pDef->Filename)) return false;
	if (!pDef->Load(hGroup, dwLoadWhat, szLanguage, pSoundSystem)) return false;
	hGroup.Close();
	// rebuild quick access table
	BuildTable();
#ifdef C4ENGINE
	// update script engine - this will also do include callbacks
	Game.ScriptEngine.ReLink(this);
#endif
#ifdef C4ENGINE
	// update definition pointers
	Game.Objects.UpdateDefPointers(pDef);
	// restore graphics
	GfxBackup.AssignUpdate(&pDef->Graphics);
#endif
	// Success
	return true;
}

void C4DefList::BuildTable()
{
	// clear any current table
	int32_t i;
	for (i = 0; i < 64; i++) if (Table[i]) { delete[] Table[i]; Table[i] = nullptr; }
	// build temp count list
	int32_t Counts[64]; ZeroMem(&Counts, sizeof(Counts));
	C4Def *pDef;
	for (pDef = FirstDef; pDef; pDef = pDef->Next)
		if (LooksLikeID(pDef->id))
			if (pDef->id < 10000)
				Counts[0]++;
			else
				Counts[(pDef->id >> 24) - 32]++;
	// get mem for table; !!! leave space for stop entry !!!
	for (i = 0; i < 64; i++) if (Counts[i])
	{
		C4Def **ppDef = (C4Def **)new long[Counts[i] + 1];
		Table[i] = ppDef;
		ZeroMem(ppDef, (Counts[i] + 1) * sizeof(long));
	}
	// build table
	for (pDef = FirstDef; pDef; pDef = pDef->Next)
		if (LooksLikeID(pDef->id))
		{
			C4Def **ppDef;
			if (pDef->id < 10000)
				ppDef = Table[0];
			else
				ppDef = Table[(pDef->id >> 24) - 32];
			while (*ppDef) ppDef++;
			*ppDef = pDef;
		}
	// done
	fTable = true;
	// use table for sorting now
	SortByID();
}

bool C4Def::LoadPortraits(C4Group &hGroup)
{
#ifdef C4ENGINE
	// reset any previous portraits
	Portraits = nullptr; PortraitCount = 0;
	// search for portraits within def graphics
	for (C4DefGraphics *pGfx = &Graphics; pGfx; pGfx = pGfx->GetNext())
		if (pGfx->IsPortrait())
		{
			// assign first portrait
			if (!Portraits) Portraits = pGfx->IsPortrait();
			// count
			++PortraitCount;
		}
#endif
	return true;
}

C4ValueArray *C4Def::GetCustomComponents(C4Value *pvArrayHolder, C4Object *pBuilder, C4Object *pObjInstance)
{
	// return custom components array if script function is defined and returns an array
#ifdef C4ENGINE
	if (Script.SFn_CustomComponents)
	{
		C4AulParSet pars(C4VObj(pBuilder));
		*pvArrayHolder = Script.SFn_CustomComponents->Exec(pObjInstance, &pars);
		return pvArrayHolder->getArray();
	}
#endif
	return nullptr;
}

int32_t C4Def::GetComponentCount(C4ID idComponent, C4Object *pBuilder)
{
	// script overload?
	C4Value vArrayHolder;
	C4ValueArray *pArray = GetCustomComponents(&vArrayHolder, pBuilder);
	if (pArray)
	{
		int32_t iCount = 0;
		for (int32_t i = 0; i < pArray->GetSize(); ++i)
			if (pArray->GetItem(i).getC4ID() == idComponent)
				++iCount;
		return iCount;
	}
	// no valid script overload: Assume definition components
	return Component.GetIDCount(idComponent);
}

C4ID C4Def::GetIndexedComponent(int32_t idx, C4Object *pBuilder)
{
	// script overload?
	C4Value vArrayHolder;
	C4ValueArray *pArray = GetCustomComponents(&vArrayHolder, pBuilder);
	if (pArray)
	{
		// assume that components are always returned ordered ([a, a, b], but not [a, b, a])
		if (!pArray->GetSize()) return 0;
		C4ID idLast = pArray->GetItem(0).getC4ID();
		if (!idx) return idLast;
		for (int32_t i = 1; i < pArray->GetSize(); ++i)
		{
			C4ID idCurr = pArray->GetItem(i).getC4ID();
			if (idCurr != idLast)
			{
				if (!--idx) return (idCurr);
				idLast = idCurr;
			}
		}
		// index out of bounds
		return 0;
	}
	// no valid script overload: Assume definition components
	return Component.GetID(idx);
}

void C4Def::GetComponents(C4IDList *pOutList, C4Object *pObjInstance, C4Object *pBuilder)
{
	assert(pOutList);
	assert(!pOutList->GetNumberOfIDs());
	// script overload?
	C4Value vArrayHolder;
	C4ValueArray *pArray = GetCustomComponents(&vArrayHolder, pBuilder, pObjInstance);
	if (pArray)
	{
		// transform array into IDList
		// assume that components are always returned ordered ([a, a, b], but not [a, b, a])
		C4ID idLast = 0; int32_t iCount = 0;
		for (int32_t i = 0; i < pArray->GetSize(); ++i)
		{
			C4ID idCurr = pArray->GetItem(i).getC4ID();
			if (!idCurr) continue;
			if (i && idCurr != idLast)
			{
				pOutList->SetIDCount(idLast, iCount, true);
				iCount = 0;
			}
			idLast = idCurr;
			++iCount;
		}
		if (iCount) pOutList->SetIDCount(idLast, iCount, true);
	}
	else
	{
#ifdef C4ENGINE
		// no valid script overload: Assume object or definition components
		if (pObjInstance)
			*pOutList = pObjInstance->Component;
		else
			*pOutList = Component;
#endif
	}
}

void C4Def::IncludeDefinition(C4Def *pIncludeDef)
{
#ifdef C4ENGINE
	// inherited rank infos and clonk names, if this definition doesn't have its own
	if (!fClonkNamesOwned) pClonkNames = pIncludeDef->pClonkNames;
	if (!fRankNamesOwned) pRankNames = pIncludeDef->pRankNames;
	if (!fRankSymbolsOwned) { pRankSymbols = pIncludeDef->pRankSymbols; iNumRankSymbols = pIncludeDef->iNumRankSymbols; }
#endif
}

void C4Def::ResetIncludeDependencies()
{
#ifdef C4ENGINE
	// clear all pointers into foreign defs
	if (!fClonkNamesOwned) pClonkNames = nullptr;
	if (!fRankNamesOwned) pRankNames = nullptr;
	if (!fRankSymbolsOwned) { pRankSymbols = nullptr; iNumRankSymbols = 0; }
#endif
}

// C4DefList

bool C4DefList::GetFontImage(const char *szImageTag, CFacet &rOutImgFacet)
{
#ifdef C4ENGINE
	// extended: images by game
	C4FacetExSurface fctOut;
	if (!Game.DrawTextSpecImage(fctOut, szImageTag)) return false;
	if (fctOut.Surface == &fctOut.GetFace()) return false; // cannot use facets that are drawn on the fly right now...
	rOutImgFacet.Set(fctOut.Surface, fctOut.X, fctOut.Y, fctOut.Wdt, fctOut.Hgt);
#endif
	// done, found
	return true;
}

#ifdef _WIN32
int __cdecl C4DefListSortFunc(const void *elem1, const void *elem2)
#else
int C4DefListSortFunc(const void *elem1, const void *elem2)
#endif
{
	return (*(C4Def * const *)elem1)->id - (*(C4Def * const *)elem2)->id;
}

void C4DefList::SortByID()
{
	// ID sorting will prevent some possible sync losses due to definition loading in different order
	// (it's still possible to cause desyncs by global script function or constant overloads, overloads
	//  within the same object pack and multiple appendtos with function overloads that depend on their
	//  order.)

	// Must be called directly after quick access table has been built.
	assert(fTable);
	// sort all quick access slots
	int i = sizeof(Table) / sizeof(C4Def **);
	FirstDef = nullptr;
	C4Def ***pppDef = Table, **ppDef, **ppDefCount, **ppCurrLastDef = &FirstDef;
	while (i--)
	{
		// only used slots
		if (ppDefCount = ppDef = *pppDef)
		{
			int cnt = 0; while (*ppDefCount++) ++cnt;
			if (cnt)
			{
				qsort(ppDef, cnt, sizeof(C4Def *), &C4DefListSortFunc);
				// build new linked list from sorted table
				// note this method also terminates the list!
				while (*ppCurrLastDef = *ppDef++) ppCurrLastDef = &((*ppCurrLastDef)->Next);
			}
		}
		++pppDef;
	}
}

#ifdef C4ENGINE
void C4DefList::Synchronize()
{
	C4Def *pDef;
	for (pDef = FirstDef; pDef; pDef = pDef->Next)
		pDef->Synchronize();
}
#endif

void C4DefList::ResetIncludeDependencies()
{
	C4Def *pDef;
	for (pDef = FirstDef; pDef; pDef = pDef->Next)
		pDef->ResetIncludeDependencies();
}
