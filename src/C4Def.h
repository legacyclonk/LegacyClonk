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

/* Object definition */

#pragma once

#include <C4Shape.h>
#include <C4InfoCore.h>
#include <C4IDList.h>
#include <C4ValueMap.h>
#include <C4Facet.h>
#include <C4Surface.h>
#include <C4ComponentHost.h>
#include <C4ScriptHost.h>
#include <C4DefGraphics.h>
#include "C4LangStringTable.h"

#include "C4DelegatedIterable.h"

#include "StdFont.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

const int32_t C4D_None                   = 0,
              C4D_All                    = ~C4D_None,

              C4D_StaticBack             = 1 << 0,
              C4D_Structure              = 1 << 1,
              C4D_Vehicle                = 1 << 2,
              C4D_Living                 = 1 << 3,
              C4D_Object                 = 1 << 4,

              C4D_SortLimit              = C4D_StaticBack | C4D_Structure | C4D_Vehicle | C4D_Living | C4D_Object,

              C4D_Goal                   = 1 << 5,
              C4D_Environment            = 1 << 6,

              C4D_SelectBuilding         = 1 << 7,
              C4D_SelectVehicle          = 1 << 8,
              C4D_SelectMaterial         = 1 << 9,
              C4D_SelectKnowledge        = 1 << 10,
              C4D_SelectHomebase         = 1 << 11,
              C4D_SelectAnimal           = 1 << 12,
              C4D_SelectNest             = 1 << 13,
              C4D_SelectInEarth          = 1 << 14,
              C4D_SelectVegetation       = 1 << 15,

              C4D_TradeLiving            = 1 << 16,
              C4D_Magic                  = 1 << 17,
              C4D_CrewMember             = 1 << 18,

              C4D_Rule                   = 1 << 19,

              C4D_Background             = 1 << 20,
              C4D_Parallax               = 1 << 21,
              C4D_MouseSelect            = 1 << 22,
              C4D_Foreground             = 1 << 23,
              C4D_MouseIgnore            = 1 << 24,
              C4D_IgnoreFoW              = 1 << 25,

              C4D_BackgroundOrForeground = (C4D_Background | C4D_Foreground);

const int32_t C4D_Grab_Put = 1,
              C4D_Grab_Get = 2,

              C4D_Border_Sides  = 1,
              C4D_Border_Top    = 2,
              C4D_Border_Bottom = 4,
              C4D_Border_Layer  = 8,

              C4D_Line_Power     = 1,
              C4D_Line_Source    = 2,
              C4D_Line_Drain     = 3,
              C4D_Line_Lightning = 4,
              C4D_Line_Volcano   = 5,
              C4D_Line_Rope      = 6,
              C4D_Line_Colored   = 7,
              C4D_Line_Vertex    = 8,

              C4D_Power_Input     = 1,
              C4D_Power_Output    = 2,
              C4D_Liquid_Input    = 4,
              C4D_Liquid_Output   = 8,
              C4D_Power_Generator = 16,
              C4D_Power_Consumer  = 32,
              C4D_Liquid_Pump     = 64,
              C4D_Connect_Rope    = 128,
              C4D_EnergyHolder    = 256,

              C4D_Place_Surface = 0,
              C4D_Place_Liquid  = 1,
              C4D_Place_Air     = 2;

const int32_t C4D_VehicleControl_None    = 0,
              C4D_VehicleControl_Outside = 1,
              C4D_VehicleControl_Inside  = 2;

const int32_t C4D_Sell     = C4D_StaticBack | C4D_Structure | C4D_Vehicle | C4D_Object | C4D_TradeLiving,
              C4D_Get      = C4D_Sell,
              C4D_Activate = C4D_Get;

const uint32_t C4D_Load_None       = 0,
               C4D_Load_Picture    = 1,
               C4D_Load_Bitmap     = 2,
               C4D_Load_Script     = 4,
               C4D_Load_Desc       = 8,
               C4D_Load_ActMap     = 16,
               C4D_Load_Image      = 32,
               C4D_Load_Sounds     = 64,
               C4D_Load_ClonkNames = 128,
               C4D_Load_RankNames  = 256,
               C4D_Load_RankFaces  = 512,
               C4D_Load_RX         = C4D_Load_Bitmap | C4D_Load_Script | C4D_Load_ClonkNames | C4D_Load_Desc | C4D_Load_ActMap | C4D_Load_Sounds | C4D_Load_RankNames | C4D_Load_RankFaces;

#define C4D_Blit_Normal   0
#define C4D_Blit_Additive 1
#define C4D_Blit_ModAdd   2

#define C4DGFXMODE_NEWGFX 1
#define C4DGFXMODE_OLDGFX 2

const int32_t ActIdle = -1;
const int32_t ActHold = -2;

class C4ActionDef
{
public:
	char Name[C4D_MaxIDLen + 1]{};
	char ProcedureName[C4D_MaxIDLen + 1]{};
	int32_t Procedure; // Mapped by C4Def::Load
	int32_t Directions{1};
	int32_t FlipDir{};
	int32_t Length{1};
	int32_t Delay{};
	int32_t Attach{};
	char NextActionName[C4D_MaxIDLen + 1]{};
	int32_t NextAction{ActIdle}; // Mapped by C4Def::Load
	char InLiquidAction[C4D_MaxIDLen + 1]{};
	char TurnAction[C4D_MaxIDLen + 1]{};
	int32_t FacetBase{};
	C4TargetRect Facet{};
	int32_t FacetTopFace{};
	int32_t NoOtherAction{};
	int32_t Disabled{};
	int32_t DigFree{};
	int32_t FacetTargetStretch{};
	char Sound[C4D_MaxIDLen + 1]{};
	int32_t EnergyUsage{};
	int32_t Reverse{};
	int32_t Step{1};
	char SStartCall[C4D_MaxIDLen + 1]{};
	char SEndCall[C4D_MaxIDLen + 1]{};
	char SAbortCall[C4D_MaxIDLen + 1]{};
	char SPhaseCall[C4D_MaxIDLen + 1]{};
	class C4AulScriptFunc *StartCall{};
	C4AulScriptFunc *EndCall{};
	C4AulScriptFunc *AbortCall{};
	C4AulScriptFunc *PhaseCall{};

public:
	C4ActionDef();
	void Default();
	void CompileFunc(StdCompiler *pComp);
};

class C4DefCore
{
public:
	C4DefCore();

public:
	C4ID id;
	int32_t rC4XVer[5];
	StdStrBuf Name;
	C4IDList RequireDef;
	C4PhysicalInfo Physical;
	C4Shape Shape;
	C4Rect Entrance;
	C4Rect Collection;
	C4Rect PictureRect;
	C4TargetRect SolidMask;
	C4TargetRect TopFace;
	C4IDList Component;
	C4ID BurnTurnTo;
	C4ID BuildTurnTo;
	int32_t GrowthType;
	int32_t Basement;
	int32_t CanBeBase;
	int32_t CrewMember;
	int32_t NativeCrew;
	int32_t Mass;
	int32_t Value;
	int32_t Exclusive;
	int32_t Category;
	int32_t Growth;
	int32_t Rebuyable;
	int32_t ContactIncinerate; // 0 off 1 high - 5 low
	int32_t BlastIncinerate; // 0 off 1 - x if > damage
	int32_t Constructable;
	int32_t Grab; // 0 not 1 push 2 grab only
	int32_t Carryable;
	int32_t Rotateable;
	int32_t Chopable;
	int32_t Float;
	int32_t ColorByOwner;
	int32_t NoHorizontalMove;
	int32_t BorderBound;
	int32_t LiftTop;
	int32_t CollectionLimit;
	int32_t GrabPutGet;
	int32_t ContainBlast;
	int32_t UprightAttach;
	int32_t ContactFunctionCalls;
	int32_t MaxUserSelect;
	int32_t Line;
	int32_t LineConnect;
	int32_t LineIntersect;
	int32_t NoBurnDecay;
	int32_t IncompleteActivity;
	int32_t Placement;
	int32_t Prey;
	int32_t Edible;
	int32_t AttractLightning;
	int32_t Oversize;
	int32_t Fragile;
	int32_t Projectile;
	int32_t Explosive;
	int32_t NoPushEnter;
	int32_t DragImagePicture;
	int32_t VehicleControl;
	int32_t Pathfinder;
	int32_t MoveToRange;
	int32_t Timer;
	int32_t NoComponentMass;
	int32_t NoStabilize;
	char STimerCall[C4D_MaxIDLen];
	char ColorByMaterial[C4M_MaxName + 1];
	int32_t ClosedContainer; // if set, contained objects are not damaged by lava/acid etc. 1: Contained objects can't view out; 2: They can
	int32_t SilentCommands; // if set, no command failure messages are printed
	int32_t NoBurnDamage; // if set, the object won't take damage when burning
	int32_t TemporaryCrew; // if set, info objects are not written into regular player files
	int32_t SmokeRate; // amount of smoke produced when on fire. 100 is default
	int32_t BlitMode; // special blit mode for objects of this def. C4D_Blit_X
	int32_t NoBreath; // object does not need to breath, although it's living
	int32_t ConSizeOff; // number of pixels to be subtracted from the needed height for this building
	int32_t NoSell; // if set, object can't be sold (doesn't even appear in sell-menu)
	int32_t NoGet; // if set, object can't be taken out of a containers manually (doesn't appear in get/activate-menus)
	int32_t NoFight; // if set, object is never OCF_FightReady
	int32_t RotatedSolidmasks; // if set, solidmasks can be rotated
	int32_t NeededGfxMode; // if set, the def will only be loaded in given gfx mode
	int32_t RotatedEntrance; // 0 entrance not rotateable, 1 entrance always, 2-360 entrance within this rotation
	int32_t NoTransferZones;
	int32_t AutoContextMenu; // automatically open context menu for this object
	int32_t AllowPictureStack; // allow stacking of multiple items in menus even if some attributes do not match. APS_*-values
	int32_t HideHUDBars; // A bit mask to selectively hide some of the Energy, Magic Energy and Breath bars.
	int32_t HideHUDElements; // A bit mask to selectively hide clonk portrait, clonk name, clonk rank, clonk rank image, captain icon
	uint32_t Scale; // graphics scale
	bool BaseAutoSell; // if set, the object will automatically be sold in a base if the corresponding base functionality is enabled

public:
	enum HideBar
	{
		HB_Energy = 0x1,
		HB_MagicEnergy = 0x2,
		HB_Breath = 0x4,
		HB_All = HB_Energy | HB_MagicEnergy | HB_Breath
	};

	enum HideHud
	{
		HH_Portrait = 0x1,
		HH_Captain = 0x2,
		HH_Name = 0x4,
		HH_Rank = 0x8,
		HH_RankImage = 0x10,
		HH_Inventory = 0x20,
		HH_All = HH_Portrait | HH_Captain | HH_Name | HH_Rank | HH_RankImage | HH_Inventory
	};

	void Default();
	bool Load(C4Group &hGroup);
	void CompileFunc(StdCompiler *pComp);
	const char *GetName() const { return Name.getData(); }

protected:
	bool Compile(const char *szSource, const char *szName);
};

class C4Def : public C4DefCore
{
	friend class C4DefList;

public:
	C4Def();
	~C4Def();

public:
	int32_t ActNum; C4ActionDef *ActMap;
	char Maker[C4MaxName + 1];
	char Filename[_MAX_FNAME + 1];
	int32_t Creation;
	int32_t Count; // number of instantiations
	C4AulScriptFunc *TimerCall;
	C4ComponentHost Desc;
	C4DefScriptHost Script;
	C4LangStringTable StringTable;

	// clonknames are simply not needed in frontend
	C4ComponentHost *pClonkNames; bool fClonkNamesOwned;

	// neither are ranknames nor the symbols...yet!
	C4RankSystem *pRankNames; bool fRankNamesOwned;
	C4FacetExSurface *pRankSymbols; bool fRankSymbolsOwned;
	int32_t iNumRankSymbols; // number of rank symbols available, if loaded
	C4DefGraphics Graphics; // base graphics. points to additional graphics
	int32_t PortraitCount;
	C4PortraitGraphics *Portraits; // Portraits (linked list of C4AdditionalDefGraphics)
	float Scale;

protected:
	// copy of the physical info used in FairCrew-mode
	C4PhysicalInfo *pFairCrewPhysical;

	C4Facet MainFace;

public:
	void Clear();
	void Default();
	bool Load(C4Group &hGroup,
		uint32_t dwLoadWhat, const char *szLanguage,
		class C4SoundSystem *pSoundSystem = nullptr);
	void Draw(C4Facet &cgo, bool fSelected = false, uint32_t iColor = 0, C4Object *pObj = nullptr, int32_t iPhaseX = 0, int32_t iPhaseY = 0);
	inline C4Facet &GetMainFace(C4DefGraphics *pGraphics, uint32_t dwClr = 0) { MainFace.Surface = pGraphics->GetBitmap(dwClr); return MainFace; }
	int32_t GetValue(C4Section &section, C4Object *pInBase, int32_t iBuyPlayer); // get value of def; calling script functions if defined
	C4PhysicalInfo *GetFairCrewPhysicals(C4Section &section); // get fair crew physicals at current fair crew strength
	void ClearFairCrewPhysicals(); // remove cached fair crew physicals, will be created fresh on demand
	void Synchronize();
	const char *GetDesc() { return Desc.GetData(); }

protected:
	bool LoadPortraits(C4Group &hGroup);
	bool ColorizeByMaterial(class C4MaterialMap &rMats, uint8_t bGBM);
	bool LoadActMap(C4Group &hGroup);
	void CrossMapActMap();

private:
	C4ValueArray *GetCustomComponents(C4Value *pvArrayHolder, C4Section &section, C4Object *pBuilder, C4Object *pObjInstance = nullptr);

public:
	// return def components - may be overloaded by script callback
	int32_t GetComponentCount(C4ID idComponent, C4Section &section, C4Object *pBuilder = nullptr);
	C4ID GetIndexedComponent(int32_t idx, C4Section &section, C4Object *pBuilder = nullptr);
	void GetComponents(C4IDList *pOutList, C4Section &section, C4Object *pObjInstance = nullptr, C4Object *pBuilder = nullptr);

	void IncludeDefinition(C4Def *pIncludeDef); // inherit components from other definition
	void ResetIncludeDependencies(); // resets all pointers into foreign definitions caused by include chains

	void Picture2Facet(C4FacetExSurface &cgo, uint32_t color = 0, int32_t xPhase = 0);
};

class C4DefList : public C4DelegatedIterable<C4DefList>, public CStdFont::CustomImages
{
public:
	C4DefList();
	virtual ~C4DefList();

public:
	bool LoadFailure;

public:
	void Clear();
	int32_t Load(C4Group &hGroup,
		uint32_t dwLoadWhat, const char *szLanguage,
		C4SoundSystem *pSoundSystem = nullptr,
		bool fOverload = false,
		bool fSearchMessage = false, int32_t iMinProgress = 0, int32_t iMaxProgress = 0, bool fLoadSysGroups = true);
	int32_t Load(const char *szSearch,
		uint32_t dwLoadWhat, const char *szLanguage,
		C4SoundSystem *pSoundSystem = nullptr,
		bool fOverload = false, int32_t iMinProgress = 0, int32_t iMaxProgress = 0);
	C4Def *ID2Def(C4ID id);
	C4Def *GetDef(std::size_t index, std::uint32_t category = C4D_All);
	C4Def *GetByPath(const char *szPath);
	int32_t GetIndex(C4ID id);
	int32_t ColorizeByMaterial(C4MaterialMap &rMats, uint8_t bGBM);
	int32_t CheckEngineVersion(int32_t ver1, int32_t ver2, int32_t ver3, int32_t ver4, int32_t ver5);
	int32_t CheckRequireDef();
	void Draw(C4ID id, C4Facet &cgo, bool fSelected, int32_t iColor);
	void Remove(C4Def *def);
	bool Remove(C4ID id);
	bool Reload(C4Def *pDef, uint32_t dwLoadWhat, const char *szLanguage, C4SoundSystem *pSoundSystem = nullptr);
	bool Add(C4Def *ndef, bool fOverload);
	void ResetIncludeDependencies(); // resets all pointers into foreign definitions caused by include chains
	void SortByID();
	void Synchronize();

	// callback from font renderer: get ID image
	virtual bool GetFontImage(const char *szImageTag, C4Facet &rOutImgFacet) override;

private:
	std::vector<std::unique_ptr<C4Def>>::iterator FindDefByID(C4ID id);

	std::vector<std::unique_ptr<C4Def>> Defs;
	bool Sorted;

public:
	using Iterable = ConstIterableMember<&C4DefList::Defs>;
};

// Default Action Procedures

#define DFA_NONE    -1
#define DFA_WALK     0
#define DFA_FLIGHT   1
#define DFA_KNEEL    2
#define DFA_SCALE    3
#define DFA_HANGLE   4
#define DFA_DIG      5
#define DFA_SWIM     6
#define DFA_THROW    7
#define DFA_BRIDGE   8
#define DFA_BUILD    9
#define DFA_PUSH    10
#define DFA_CHOP    11
#define DFA_LIFT    12
#define DFA_FLOAT   13
#define DFA_ATTACH  14
#define DFA_FIGHT   15
#define DFA_CONNECT 16
#define DFA_PULL    17

#define C4D_MaxDFA  18

// procedure name table
extern const char *ProcedureName[C4D_MaxDFA];
