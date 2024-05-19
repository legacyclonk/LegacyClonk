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

/* Player data at runtime */

#pragma once

#include "C4EnumeratedObjectPtr.h"
#include "C4MainMenu.h"
#include <C4ObjectInfoList.h>
#include <C4InfoCore.h>
#include <C4ObjectList.h>

const int32_t C4PVM_Cursor    = 0,
              C4PVM_Target    = 1,
              C4PVM_Scrolling = 2;

const int32_t C4P_Number_None = -5;

const int32_t C4MaxPlayer = 5000; // ought to be enough for everybody (used to catch invalid player counts)
const int32_t C4MaxClient = 5000; // ought to be enough for everybody (used to catch invalid client counts)

class C4Player : public C4PlayerInfoCore
{
public:
	// possible player controls used for statistics
	enum ControlType
	{
		PCID_None,      // no control
		PCID_Command,   // mouse control
		PCID_DirectCom, // menu or keyboard control
	};
	// possible status
	enum StatusTypes
	{
		PS_None = 0,          // player disabled
		PS_Normal = 1,        // normal playing
		PS_TeamSelection = 2, // team selection
		PS_TeamSelectionPending = 3, // waiting for team selection packet to come through
	};

	friend class C4PlayerList;

public:
	C4Player();
	~C4Player();

public:
	char Filename[_MAX_PATH + 1];
	StdStrBuf Name;
	int32_t Status;
	int32_t Eliminated;
	int32_t Surrendered;
	bool Evaluated;
	int32_t Number;
	int32_t ID; // unique player ID
	int32_t Team; // team ID - may be 0 for no teams
	int32_t Color; // OldGfx color index
	uint32_t ColorDw; // color as DWord for newgfx
	int32_t Control;
	int32_t MouseControl;
	int32_t Position;
	int32_t PlrStartIndex;
	int32_t RetireDelay;
	int32_t GameJoinTime; // Local-NoSave - set in Init; reset in LocalSync
	int32_t AtClient;
	char AtClientName[C4MaxTitle + 1];
	bool LocalControl; // Local-NoSave //
	bool LeagueEvaluated; // Local-NoSave //
	bool NoEliminationCheck; // Local-NoSave. Always set in init and restore by info //
	// Menu
	C4MainMenu Menu; // Local-NoSave //
	// View
	int32_t ViewMode;
	int32_t ViewX, ViewY;
	C4Object *ViewTarget; // NoSave //
	int32_t ViewWealth, ViewValue;
	bool ShowStartup;
	int32_t ShowControl, ShowControlPos;
	int32_t FlashCom; // NoSave //
	bool fFogOfWar;
	bool bForceFogOfWar;
	bool fFogOfWarInitialized; // No Save //
	C4ObjectList FoWViewObjs; // No Save //
	// Game
	int32_t Wealth, Points;
	int32_t Value, InitialValue, ValueGain;
	int32_t ObjectsOwned;
	C4EnumeratedObjectPtr Captain;
	C4IDList Hostility;
	// Home Base
	C4IDList HomeBaseMaterial;
	C4IDList HomeBaseProduction;
	int32_t ProductionDelay, ProductionUnit;
	// Crew
	C4ObjectInfoList CrewInfoList; // No Save //
	C4ObjectList Crew; // Save new in 4.95.2 (for sync reasons)
	int32_t CrewCnt; // No Save //
	// Knowledge
	C4IDList Knowledge;
	C4IDList Magic;
	// Control
	int32_t AutoContextMenu;
	int32_t ControlStyle;
	bool HasExplicitJumpKey;
	C4EnumeratedObjectPtr Cursor, ViewCursor;
	int32_t SelectCount;
	int32_t SelectFlash, CursorFlash;
	int32_t LastCom;
	int32_t LastComDelay;
	int32_t LastComDownDouble;
	int32_t PressedComs;
	int32_t CursorSelection, CursorToggled;
	class C4GamePadOpener *pGamepad;
	// Message
	int32_t MessageStatus;
	char MessageBuf[256 + 1];
	class C4MessageBoardQuery *pMsgBoardQuery;
	// BigIcon
	C4FacetExSurface BigIcon;
	// Dynamic list
	C4Player *Next;

	// statistics
	class C4TableGraph *pstatControls, *pstatActions;
	int32_t ControlCount; // controls issued since value was last recorded
	int32_t ActionCount;  // non-doubled controls since value was last recorded
	ControlType LastControlType; int32_t LastControlID; // last control to capture perma-pressers in stats

public:
	const char *GetName() const { return Name.getData(); }
	C4PlayerType GetType() const;

public:
	void Eliminate();
	void SelectCrew(C4Object *pObj, bool fSelect);
	void Default();
	void Clear();
	void ClearPointers(C4Object *tptr, bool fDeath);
	void Execute();
	void ExecuteControl();
	void UpdateValue();
	void SetViewMode(int32_t iMode, C4Object *pTarget = nullptr);
	void ResetCursorView(); // reset view to cursor if any cursor exists
	void Evaluate();
	void Surrender();
	void ScrollView(int32_t iX, int32_t iY, int32_t ViewWdt, int32_t ViewHgt);
	void SelectCrew(C4ObjectList &rList);
	void SetCursor(C4Object *pObj, bool fSelectFlash, bool fSelectArrow);
	void RemoveCrewObjects();
	void NotifyOwnedObjects();
	void DefaultRuntimeData();
	void DrawHostility(C4Facet &cgo, int32_t iIndex);
	void AdjustCursorCommand();
	void CursorRight();
	void CursorLeft();
	void UnselectCrew();
	void SelectSingleByCursor();
	void CursorToggle();
	void SelectAllCrew();
	void UpdateSelectionToggleStatus();
	void DirectCom(uint8_t byCom, int32_t iData);
	void InCom(uint8_t byCom, int32_t iData);
	bool ObjectCom(uint8_t byCom, int32_t iData);
	void ClearPressedComsSynced();
	bool ObjectCommand(int32_t iCommand, C4Object *pTarget, int32_t iTx, int32_t iTy, C4Object *pTarget2 = nullptr, int32_t iData = 0, int32_t iAddMode = C4P_Command_Set);
	void ObjectCommand2Obj(C4Object *cObj, int32_t iCommand, C4Object *pTarget, int32_t iX, int32_t iY, C4Object *pTarget2, int32_t iData, int32_t iMode);
	bool DoPoints(int32_t iChange);
	bool Init(int32_t iNumber, int32_t iAtClient, const char *szAtClientName, const char *szFilename, bool fScenarioInit, class C4PlayerInfo *pInfo);
	bool ScenarioAndTeamInit(int32_t idTeam);
	bool ScenarioInit();
	bool FinalInit(bool fInitialValue);
	bool Save();
	bool Save(C4Group &hGroup, bool fSavegame, bool fStoreTiny);
	bool MakeCrewMember(C4Object *pObj, bool fForceInfo = true, bool fDoCalls = true);
	bool Load(const char *szFilename, bool fSavegame, bool fLoadPortraits);
	static bool Strip(const char *szFilename, bool fAggressive);
	bool ObjectInCrew(C4Object *tobj);
	C4Object *Buy(C4ID id, bool fShowErrors, int32_t iForPlr, C4Object *pBuyObj = nullptr); // buy object of player's wealth
	bool Sell2Home(C4Object *tobj);
	bool CanSell(C4Object *obj) const;
	bool DoWealth(int32_t change);
	bool SetHostility(int32_t iOpponent, int32_t iHostility, bool fSilent = false);
	void CompileFunc(StdCompiler *pComp);
	bool LoadRuntimeData(C4Group &hGroup);
	bool ActivateMenuMain();
	bool ActivateMenuTeamSelection(bool fFromMain);
	void DoTeamSelection(int32_t idTeam);
	C4Object *GetHiRankActiveCrew(bool fSelectedOnly);
	void SetFoW(bool fEnable);
	int32_t ActiveCrewCount();
	int32_t GetSelectedCrewCount();
	bool LocalSync(); // sync InAction et. al. back o local player file
	bool SetObjectCrewStatus(C4Object *pCrew, bool fNewStatus); // add/remove object from crew
	bool IsChosingTeam() const { return Status == PS_TeamSelection || Status == PS_TeamSelectionPending; }
	bool IsInvisible() const;
	void SyncHomebaseMaterialToTeam();
	void SyncHomebaseMaterialFromTeam();

protected:
	void InitControl();
	void DenumeratePointers();
	void EnumeratePointers();
	void UpdateView();
	void CheckElimination();
	void UpdateCounts();
	void ExecHomeBaseProduction();
	void PlaceReadyBase(int32_t &tx, int32_t &ty, C4Object **pFirstBase);
	void PlaceReadyVehic(int32_t tx1, int32_t tx2, int32_t ty, C4Object *FirstBase);
	void PlaceReadyMaterial(int32_t tx1, int32_t tx2, int32_t ty, C4Object *FirstBase);
	void PlaceReadyCrew(int32_t tx1, int32_t tx2, int32_t ty, C4Object *FirstBase);

public:
	void SetTeamHostility(); // if Team!=0: Set hostile to all players in other teams and allied to all others (both ways)

	void CloseMenu(); // close all player menus (keep sync object menus!)

	void EvaluateLeague(bool fDisconnected, bool fWon);

	void FoW2Map(CClrModAddMap &rMap, int iOffX, int iOffY);
	void FoWGenerators2Map(CClrModAddMap &rMap, int iOffX, int iOffY);
	bool FoWIsVisible(int32_t x, int32_t y); // check whether a point in the landscape is visible

	// runtime statistics
	void CreateGraphs();
	void ClearGraphs();
	void CountControl(ControlType eType, int32_t iID, int32_t iCntAdd = 1);

	class C4PlayerInfo *GetInfo(); // search info by ID

private:
	// messageboard-calls for this player
	void ExecMsgBoardQueries();

public:
	void ToggleMouseControl();
	void CallMessageBoard(C4Object *pForObj, const StdStrBuf &sQueryString, bool fUppercase);
	bool RemoveMessageBoardQuery(C4Object *pForObj);
	bool MarkMessageBoardQueryAnswered(C4Object *pForObj);
	bool HasMessageBoardQuery(); // return whether any object has a messageboard-query

	// callback by script execution of team selection: Restart team selection if the team turned out to be not available
	void OnTeamSelectionFailed();

	// when the player changes team, his color changes. Relfect this in player objects
	void SetPlayerColor(uint32_t dwNewClr);

	void ApplyForcedControl();
};
