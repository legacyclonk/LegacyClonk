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

#include <C4Include.h>
#include <C4Player.h>

#include <C4Application.h>
#include <C4Object.h>
#include <C4ObjectInfo.h>
#include <C4Command.h>
#include <C4Network2Stats.h>
#include <C4MessageInput.h>
#include <C4GamePadCon.h>
#include <C4Wrappers.h>
#include <C4Random.h>
#include <C4Log.h>
#include <C4FullScreen.h>
#include <C4GameOverDlg.h>
#include <C4ObjectMenu.h>

static constexpr std::int32_t C4FOW_Def_View_RangeX{500};

C4Player::C4Player() : C4PlayerInfoCore()
{
	Default();
}

C4Player::~C4Player()
{
	Clear();
}

bool C4Player::ObjectInCrew(C4Object *tobj)
{
	C4Object *cobj; C4ObjectLink *clnk;
	if (!tobj) return false;
	for (clnk = Crew.First; clnk && (cobj = clnk->Obj); clnk = clnk->Next)
		if (cobj == tobj) return true;
	return false;
}

void C4Player::ClearPointers(C4Object *pObj, bool fDeath)
{
	// Game
	if (Captain == pObj) Captain = nullptr;
	// Crew
	while (Crew.Remove(pObj));
	// Cursor
	if (Cursor == pObj)
	{
		// object is to be deleted; do NOT do script calls (like in Cursor->UnSelect(true))
		Cursor = nullptr; AdjustCursorCommand(); // also selects and eventually does a script call!
	}
	// View-Cursor
	if (ViewCursor == pObj) ViewCursor = nullptr;
	// View
	if (ViewTarget == pObj) ViewTarget = nullptr;
	// FoW
	// (do not clear locals!)
	// no clear when death to do normal decay
	if (!fDeath)
		while (FoWViewObjs.Remove(pObj));
	// Menu
	Menu.ClearPointers(pObj);
	// messageboard-queries
	RemoveMessageBoardQuery(pObj);
}

void C4Player::UpdateValue()
{
	int32_t lval = ValueGain, lobj = ObjectsOwned;
	Value = 0; ObjectsOwned = 0;

	// Points
	Value += Points;

	// Wealth
	Value += Wealth;

	// Asset all owned objects
	C4Object *cobj; C4ObjectLink *clnk;
	for (clnk = Game.Objects.First; clnk && (cobj = clnk->Obj); clnk = clnk->Next)
		if (cobj->Owner == Number && cobj->Status)
		{
			ObjectsOwned++;
			Value += cobj->GetValue(nullptr, Number);
		}

	// Value gain (always positive)
	ValueGain = Value - InitialValue;

	// Update
	if ((ValueGain != lval) || (ObjectsOwned != lobj)) ViewValue = C4ViewDelay;
}

bool C4Player::ScenarioAndTeamInit(int32_t idTeam)
{
	C4PlayerInfo *pInfo = GetInfo();
	if (!pInfo) return false;
	C4Team *pTeam;
	if (idTeam == TEAMID_New)
	{
		// creation of a new team only if allowed by scenario
		if (!Game.Teams.IsAutoGenerateTeams())
			pTeam = nullptr;
		else
		{
			if (pTeam = Game.Teams.GetGenerateTeamByID(idTeam)) idTeam = pTeam->GetID();
		}
	}
	else
	{
		// uage of an existing team
		pTeam = Game.Teams.GetTeamByID(idTeam);
	}
	C4Team *pPrevTeam = Game.Teams.GetTeamByID(Team);
	// check if join to team is possible; e.g. not too many players
	if (pPrevTeam != pTeam && idTeam)
	{
		if (!Game.Teams.IsJoin2TeamAllowed(idTeam))
		{
			pTeam = nullptr;
		}
	}
	if (!pTeam && idTeam)
	{
		OnTeamSelectionFailed();
		return false;
	}
	// team selection OK; execute it!
	if (pPrevTeam) pPrevTeam->RemovePlayerByID(pInfo->GetID());
	if (pTeam) pTeam->AddPlayer(*pInfo, true);
	if (!ScenarioInit()) return false;
	if (Game.Rules & C4RULE_TeamHombase) SyncHomebaseMaterialFromTeam();
	if (!FinalInit(false)) return false;
	return true;
}

void C4Player::Execute()
{
	if (!Status) return;

	// Open/refresh team menu if desired
	if (Status == PS_TeamSelection)
	{
		int32_t idSelectedTeam;
		if (idSelectedTeam = Game.Teams.GetForcedTeamSelection(ID))
		{
			// There's only one team left to join? Join there immediately.
			if (Menu.IsActive() && Menu.GetIdentification() == C4MN_TeamSelection) Menu.TryClose(false, false);
			if (LocalControl && !Game.Control.isReplay())
			{
				// team selection done through queue because TeamSelection-status may not be in sync (may be TeamSelectionPending!)
				DoTeamSelection(idSelectedTeam);
			}
		}
		else if (!Menu.IsActive()) ActivateMenuTeamSelection(false);
		else
		{
			// during team selection: Update view to selected team, if it has a position assigned
			C4MenuItem *pSelectedTeamItem;
			if (pSelectedTeamItem = Menu.GetSelectedItem())
			{
				int32_t idSelectedTeam = int32_t(pSelectedTeamItem->GetC4ID());
				if (idSelectedTeam)
				{
					C4Team *pSelectedTeam;
					if (pSelectedTeam = Game.Teams.GetTeamByID(idSelectedTeam))
					{
						int32_t iPlrStartIndex = pSelectedTeam->GetPlrStartIndex();
						if (iPlrStartIndex && Inside<int32_t>(iPlrStartIndex, 1, C4S_MaxPlayer))
						{
							if (Game.C4S.PlrStart[iPlrStartIndex - 1].Position[0] > -1)
							{
								// player has selected a team that has a valid start position assigned
								// set view to this position!
								ViewX = Game.C4S.PlrStart[iPlrStartIndex - 1].Position[0] * Game.Landscape.MapZoom;
								ViewY = Game.C4S.PlrStart[iPlrStartIndex - 1].Position[1] * Game.Landscape.MapZoom;
							}
						}
					}
				}
			}
		}
	}
	else if (Menu.IsActive() && Menu.GetIdentification() == C4MN_TeamSelection)
	{
		Menu.TryClose(false, false);
	}

	// Tick1
	UpdateCounts();
	UpdateView();
	ExecuteControl();
	Menu.Execute();
	if (Cursor)
		Cursor->AutoContextMenu(-1);

	// decay of dead viewtargets
	C4ObjectLink *pLnkNext = FoWViewObjs.First, *pLnk;
	while (pLnk = pLnkNext)
	{
		pLnkNext = pLnk->Next;
		C4Object *pDeadClonk = pLnk->Obj;
		if (!pDeadClonk->GetAlive() && (pDeadClonk->Category & C4D_Living) && pDeadClonk->Status)
		{
			pDeadClonk->PlrViewRange -= 10;
			if (pDeadClonk->PlrViewRange <= 0)
				FoWViewObjs.Remove(pDeadClonk);
		}
	}

	// Tick35
	if (!Tick35 && Status == PS_Normal)
	{
		ExecHomeBaseProduction();
		UpdateValue();
		CheckElimination();
		if (pMsgBoardQuery && LocalControl) ExecMsgBoardQueries();
	}

	// Delays
	if (MessageStatus > 0) MessageStatus--;
	if (RetireDelay > 0) RetireDelay--;
	if (ViewWealth > 0) ViewWealth--;
	if (ViewValue > 0) ViewValue--;
	if (CursorFlash > 0) CursorFlash--;
	if (SelectFlash > 0) SelectFlash--;
}

bool C4Player::Init(int32_t iNumber, int32_t iAtClient, const char *szAtClientName,
	const char *szFilename, bool fScenarioInit, class C4PlayerInfo *pInfo)
{
	// safety
	if (!pInfo)
	{
		LogF("ERROR: Init player %s failed: No info!", szFilename);
		assert(false);
		return false;
	}
	// Status init
	Status = PS_Normal;
	if (szFilename) SCopy(szFilename, Filename); else *Filename = '\0';
	Number = iNumber;
	ID = pInfo->GetID();
	Team = pInfo->GetTeam();
	NoEliminationCheck = pInfo->IsNoEliminationCheck();

	// At client
	AtClient = iAtClient; SCopy(szAtClientName, AtClientName, C4MaxTitle);

	if (szFilename)
	{
		// Load core & crew info list
		// do not load portraits for remote players
		// this will prevent portraits from being shown for "remotely controlled"-Clonks of other players
		bool fLoadPortraits = (AtClient == C4ClientIDUnknown) || SEqualNoCase(AtClientName, Game.Clients.getLocalName());
		// fLoadPortraits = true
		if (!Load(szFilename, !fScenarioInit, fLoadPortraits)) return false;
	}
	else
	{
		// no core file present: Keep defaults
		// This can happen for script players only
		assert(pInfo->GetType() == C4PT_Script);
	}

	// Take player name from player info; forcing overloads by the league or because of doubled player names
	Name.Copy(pInfo->GetName());

	// view pos init: Start at center pos
	ViewX = GBackWdt / 2; ViewY = GBackHgt / 2;

	// Scenario init
	if (fScenarioInit)
	{
		// mark player join in player info list
		// for non-scenarioinit, player should already be marked as joined
		pInfo->SetJoined(iNumber);

		// Number might have changed: Recheck list sorting before scenarioinit, which will do script calls
		Game.Players.RecheckPlayerSort(this);

		// check for a postponed scenario init, if no team is specified (post-lobby-join in network, or simply non-network)
		C4Team *pTeam = nullptr;
		if (Team)
		{
			if (Game.Teams.IsAutoGenerateTeams())
				pTeam = Game.Teams.GetGenerateTeamByID(Team);
			else
				pTeam = Game.Teams.GetTeamByID(Team);
		}
		if (!pTeam && Game.Teams.IsRuntimeJoinTeamChoice())
		{
			if (pInfo->GetType() == C4PT_Script)
			{
				// script player without team: This can usually not happen, because RecheckPlayerInfoTeams should have been executed
				// just leave this player without the team
				assert(false);
			}
			else
			{
				// postponed init: Chose team first
				Status = PS_TeamSelection;
			}
		}

		// Init control method before scenario init, because script callbacks may need to know it!
		InitControl();

		// Special: Script players may skip scenario initialization altogether, and just desire a single callback to all objects
		// of a given ID
		if (!pInfo->IsScenarioInitDesired())
		{
			// only initialization that's done anyway is team hostility
			if (Team) SetTeamHostility();
			// ...and home base material from team
			if (Game.Rules & C4RULE_TeamHombase) SyncHomebaseMaterialFromTeam();
			// callback definition passed?
			C4ID idCallback = pInfo->GetScriptPlayerExtraID();
			C4Def *pDefCallback;
			if (idCallback && (pDefCallback = C4Id2Def(idCallback)))
			{
				pDefCallback->Script.Call(PSF_InitializeScriptPlayer, {C4VInt(Number), C4VInt(Team)});
			}
		}
		else
		{
			// player preinit: In case a team needs to be chosen first, no InitializePlayer-broadcast is done
			// this callback shall give scripters a chance to do stuff like starting an intro or enabling FoW, which might need to be done
			Game.Script.GRBroadcast(PSF_PreInitializePlayer, {C4VInt(Number)});
			// direct init
			if (Status != PS_TeamSelection) if (!ScenarioInit()) return false;
			// ...and home base material from team
			if (Game.Rules & C4RULE_TeamHombase) SyncHomebaseMaterialFromTeam();
		}
	}

	// Load runtime data
	else
	{
		assert(pInfo->IsJoined());
		// (compile using DefaultRuntimeData) - also check if compilation returned sane results, i.e. ID assigned
		if (!LoadRuntimeData(Game.ScenarioFile) || !ID)
		{
			// for script players in non-savegames, this is OK - it means they get restored using default values
			// this happens when the users saves a scenario using the "Save scenario"-option while a script player
			// was joined
			if (!Game.C4S.Head.SaveGame && pInfo->GetType() == C4PT_Script)
			{
				Number = pInfo->GetInGameNumber();
				ColorDw = pInfo->GetColor();
				ID = pInfo->GetID();
				Team = pInfo->GetTeam();
			}
			else
				return false;
		}
		// Reset values default-overriden by old runtime data load (safety?)
		if (Number == C4P_Number_None) Number = iNumber;
		if (szFilename) SCopy(szFilename, Filename); else *Filename = '\0';
		// NET2: Direct joins always send accurate client IDs and names in params
		// do not overwrite them with savegame data, because players might as well
		// change clients
		// (only call should be savegame recreation by C4PlayerInfoList::RecreatePlayers)
		AtClient = iAtClient;
		SCopy(szAtClientName, AtClientName, C4MaxTitle);
		// Number might have changed: Recheck list sorting
		Game.Players.RecheckPlayerSort(this);
		// Init control after loading runtime data, because control init will overwrite some of the values
		InitControl();
	}

	// store game joining time
	GameJoinTime = Game.Time;

	// Init FoW-viewobjects: NO_OWNER-FoW-repellers might need to be added
	for (C4ObjectLink *pLnk = Game.Objects.First; pLnk; pLnk = pLnk->Next)
	{
		C4Object *pObj = pLnk->Obj;
		if (pObj->PlrViewRange && pObj->Owner == NO_OWNER)
			pObj->PlrFoWActualize();
	}

	// init graphs
	if (Game.pNetworkStatistics) CreateGraphs();

	return true;
}

bool C4Player::Save()
{
	C4Group hGroup;
	// Regular player saving need not be done for script players
	if (GetType() == C4PT_Script) return false;
	// Remote players need not be saved if they cannot resume
	if (!LocalControl)
	{
		// This is the case in league
		if (Game.Parameters.isLeague()) return false;
		// And also if max player count is set to zero to prevent runtime joins
		if (Game.Parameters.MaxPlayers <= 0) return false;
	}
	// Log
	LogF(LoadResStr("IDS_PRC_SAVEPLR"), Config.AtExeRelativePath(Filename));
	Game.GraphicsSystem.MessageBoard.EnsureLastMessage();
	// copy player to save somewhere else
	char szPath[_MAX_PATH + 1];
	SCopy(Config.AtTempPath(C4CFN_TempPlayer), szPath, _MAX_PATH);
	MakeTempFilename(szPath);
	// For local players, we save over the old player file, as there might
	// be all kinds of non-essential stuff in it. For non-local players, we
	// just re-create it every time (it's temporary anyway).
	if (LocalControl)
	{
		// But make sure to copy it first so full hard (flgr stupid) disks
		// won't corrupt any player files...
		C4Group_CopyItem(Filename, szPath);
	}
	else
	{
		// For non-local players, we can actually use the loaded definition
		// list to strip out all non-existent definitions. This is only valid
		// because we know the file to be temporary.
		CrewInfoList.Strip(Game.Defs);
	}
	// Open group
	if (!hGroup.Open(szPath, true))
		return false;
	// Save
	if (!Save(hGroup, false, !LocalControl))
	{
		hGroup.Close(); return false;
	}
	// Close group
	if (!hGroup.Close()) return false;
	// resource
	C4Network2Res::Ref pRes = Game.Network.ResList.getRefRes(Filename),
		pDRes = nullptr;
	bool fOfficial = pRes && Game.Control.isCtrlHost();
	if (pRes) pDRes = pRes->Derive();
	// move back
	if (ItemExists(Filename)) EraseItem(Filename);
	if (!C4Group_MoveItem(szPath, Filename)) return false;
	// finish update
	if (pDRes && fOfficial) pDRes->FinishDerive();
	// Success
	return true;
}

bool C4Player::Save(C4Group &hGroup, bool fSavegame, bool fStoreTiny)
{
	// Save core
	if (!C4PlayerInfoCore::Save(hGroup))
		return false;
	// Save crew
	if (!CrewInfoList.Save(hGroup, fSavegame, fStoreTiny, &Game.Defs))
	{
		hGroup.Close(); return false;
	}
	// Sort
	hGroup.Sort(C4FLS_Player);
	return true;
}

void C4Player::PlaceReadyCrew(int32_t tx1, int32_t tx2, int32_t ty, C4Object *FirstBase)
{
	int32_t cnt, crewnum, ctx, cty;
	C4Object *nobj;
	C4ObjectInfo *pInfo;
	C4Def *pDef;

	// Set name source
	const char *cpNames = Game.Names.GetData();

	// Old specification
	if (Game.C4S.PlrStart[PlrStartIndex].ReadyCrew.IsClear())
	{
		// Target number of ready crew
		crewnum = Game.C4S.PlrStart[PlrStartIndex].Crew.Evaluate();
		// Place crew
		for (cnt = 0; cnt < crewnum; cnt++)
		{
			// Set standard crew
			C4ID idStdCrew = Game.C4S.PlrStart[PlrStartIndex].NativeCrew;
			// Select member from home crew, add new if necessary
			while (!(pInfo = CrewInfoList.GetIdle(idStdCrew, Game.Defs)))
				if (!CrewInfoList.New(idStdCrew, &Game.Defs, cpNames))
					break;
			// Crew placement location
			if (!pInfo || !(pDef = C4Id2Def(pInfo->id))) continue;
			ctx = tx1 + Random(tx2 - tx1); cty = ty;
			if (!Game.C4S.PlrStart[PlrStartIndex].EnforcePosition)
				FindSolidGround(ctx, cty, pDef->Shape.Wdt * 3);
			// Create object
			if (nobj = Game.CreateInfoObject(pInfo, Number, ctx, cty))
			{
				// Add object to crew
				Crew.Add(nobj, C4ObjectList::stMain);
				// add visibility range
				nobj->SetPlrViewRange(C4FOW_Def_View_RangeX);
				// If base is present, enter base
				if (FirstBase) { nobj->Enter(FirstBase); nobj->SetCommand(C4CMD_Exit); }
				// OnJoinCrew callback
#ifndef DEBUGREC_RECRUITMENT
				C4DebugRecOff DBGRECOFF;
#endif
				nobj->Call(PSF_OnJoinCrew, {C4VInt(Number)});
			}
		}
	}

	// New specification
	else
	{
		// Place crew
		int32_t id, iCount;
		for (cnt = 0; id = Game.C4S.PlrStart[PlrStartIndex].ReadyCrew.GetID(cnt, &iCount); cnt++)
		{
			// Minimum one clonk if empty id
			iCount = std::max<int32_t>(iCount, 1);

			for (int32_t cnt2 = 0; cnt2 < iCount; cnt2++)
			{
				// Select member from home crew, add new if necessary
				while (!(pInfo = CrewInfoList.GetIdle(id, Game.Defs)))
					if (!CrewInfoList.New(id, &Game.Defs, cpNames))
						break;
				// Safety
				if (!pInfo || !(pDef = C4Id2Def(pInfo->id))) continue;
				// Crew placement location
				ctx = tx1 + Random(tx2 - tx1); cty = ty;
				if (!Game.C4S.PlrStart[PlrStartIndex].EnforcePosition)
					FindSolidGround(ctx, cty, pDef->Shape.Wdt * 3);
				// Create object
				if (nobj = Game.CreateInfoObject(pInfo, Number, ctx, cty))
				{
					// Add object to crew
					Crew.Add(nobj, C4ObjectList::stMain);
					// add visibility range
					nobj->SetPlrViewRange(C4FOW_Def_View_RangeX);
					// If base is present, enter base
					if (FirstBase) { nobj->Enter(FirstBase); nobj->SetCommand(C4CMD_Exit); }
					// OnJoinCrew callback
					{
#ifndef DEBUGREC_RECRUITMENT
						C4DebugRecOff DbgRecOff;
#endif
						nobj->Call(PSF_OnJoinCrew, {C4VInt(Number)});
					}
				}
			}
		}
	}
}

C4Object *CreateLine(C4ID linetype, int32_t owner, C4Object *fobj, C4Object *tobj);

bool CreatePowerConnection(C4Object *fbase, C4Object *tbase)
{
	if (CreateLine(C4ID_PowerLine, fbase->Owner, fbase, tbase)) return true;
	return false;
}

void C4Player::PlaceReadyBase(int32_t &tx, int32_t &ty, C4Object **pFirstBase)
{
	int32_t cnt, cnt2, ctx, cty;
	C4Def *def;
	C4ID cid;
	C4Object *cbase, *fpower = nullptr;
	// Create ready base structures
	for (cnt = 0; (cid = Game.C4S.PlrStart[PlrStartIndex].ReadyBase.GetID(cnt)); cnt++)
	{
		if (def = C4Id2Def(cid))
			for (cnt2 = 0; cnt2 < Game.C4S.PlrStart[PlrStartIndex].ReadyBase.GetCount(cnt); cnt2++)
			{
				ctx = tx; cty = ty;
				if (Game.C4S.PlrStart[PlrStartIndex].EnforcePosition
					|| FindConSiteSpot(ctx, cty, def->Shape.Wdt, def->Shape.Hgt, def->Category, 20))
					if (cbase = Game.CreateObjectConstruction(cid, nullptr, Number, ctx, cty, FullCon, true))
					{
						// FirstBase
						if (!(*pFirstBase)) if (cbase->Def->CanBeBase)
						{
							*pFirstBase = cbase; tx = (*pFirstBase)->x; ty = (*pFirstBase)->y;
						}
						// First power plant
						if (cbase->Def->LineConnect & C4D_Power_Generator)
							if (!fpower) fpower = cbase;
					}
			}
	}

	// Power connections
	C4ObjectLink *clnk; C4Object *cobj;
	if (Game.Rules & C4RULE_StructuresNeedEnergy)
		if (fpower)
			for (clnk = Game.Objects.First; clnk && (cobj = clnk->Obj); clnk = clnk->Next)
				if (cobj->Owner == Number)
					if (cobj->Def->LineConnect & C4D_Power_Consumer)
						CreatePowerConnection(fpower, cobj);
}

void C4Player::PlaceReadyVehic(int32_t tx1, int32_t tx2, int32_t ty, C4Object *FirstBase)
{
	int32_t cnt, cnt2, ctx, cty;
	C4Def *def; C4ID cid; C4Object *cobj;
	for (cnt = 0; (cid = Game.C4S.PlrStart[PlrStartIndex].ReadyVehic.GetID(cnt)); cnt++)
	{
		if (def = C4Id2Def(cid))
			for (cnt2 = 0; cnt2 < Game.C4S.PlrStart[PlrStartIndex].ReadyVehic.GetCount(cnt); cnt2++)
			{
				ctx = tx1 + Random(tx2 - tx1); cty = ty;
				if (!Game.C4S.PlrStart[PlrStartIndex].EnforcePosition)
					FindLevelGround(ctx, cty, def->Shape.Wdt, 6);
				if (cobj = Game.CreateObject(cid, nullptr, Number, ctx, cty))
				{
					if (FirstBase) // First base overrides target location
					{
						cobj->Enter(FirstBase); cobj->SetCommand(C4CMD_Exit);
					}
				}
			}
	}
}

void C4Player::PlaceReadyMaterial(int32_t tx1, int32_t tx2, int32_t ty, C4Object *FirstBase)
{
	int32_t cnt, cnt2, ctx, cty;
	C4Def *def; C4ID cid;

	// In base
	if (FirstBase)
	{
		FirstBase->CreateContentsByList(Game.C4S.PlrStart[PlrStartIndex].ReadyMaterial);
	}

	// Outside
	else
	{
		for (cnt = 0; (cid = Game.C4S.PlrStart[PlrStartIndex].ReadyMaterial.GetID(cnt)); cnt++)
		{
			if (def = C4Id2Def(cid))
				for (cnt2 = 0; cnt2 < Game.C4S.PlrStart[PlrStartIndex].ReadyMaterial.GetCount(cnt); cnt2++)
				{
					ctx = tx1 + Random(tx2 - tx1); cty = ty;
					if (!Game.C4S.PlrStart[PlrStartIndex].EnforcePosition)
						FindSolidGround(ctx, cty, def->Shape.Wdt);
					Game.CreateObject(cid, nullptr, Number, ctx, cty);
				}
		}
	}
}

bool C4Player::ScenarioInit()
{
	int32_t ptx, pty;

	// player start index by team, if specified. Otherwise by player number
	PlrStartIndex = Number % C4S_MaxPlayer;
	C4Team *pTeam; int32_t i;
	if (Team && (pTeam = Game.Teams.GetTeamByID(Team))) if (i = pTeam->GetPlrStartIndex()) PlrStartIndex = i - 1;

	// Set color
	int32_t iColor = BoundBy<int32_t>(PrefColor, 0, C4MaxColor - 1);
	while (Game.Players.ColorTaken(iColor))
	{
		++iColor %= C4MaxColor; if (iColor == PrefColor) break;
	}
	Color = iColor;

	C4PlayerInfo *pInfo = GetInfo();
	if (!pInfo) { assert(false); LogF("Internal error: ScenarioInit for ghost player %s!", GetName()); return false; }

	// set color by player info class
	// re-setting, because runtime team choice may have altered color
	ColorDw = pInfo->GetColor();

	// any team selection is over now
	Status = PS_Normal;

	// Wealth, home base materials, abilities
	Wealth = Game.C4S.PlrStart[PlrStartIndex].Wealth.Evaluate();
	HomeBaseMaterial = Game.C4S.PlrStart[PlrStartIndex].HomeBaseMaterial;
	HomeBaseMaterial.ConsolidateValids(Game.Defs);
	HomeBaseProduction = Game.C4S.PlrStart[PlrStartIndex].HomeBaseProduction;
	HomeBaseProduction.ConsolidateValids(Game.Defs);
	Knowledge = Game.C4S.PlrStart[PlrStartIndex].BuildKnowledge;
	Knowledge.ConsolidateValids(Game.Defs);
	Magic = Game.C4S.PlrStart[PlrStartIndex].Magic;
	Magic.ConsolidateValids(Game.Defs);
	if (Magic.IsClear()) Magic.Load(Game.Defs, C4D_Magic); // All magic default if empty
	Magic.SortByValue(Game.Defs);

	// Starting position
	ptx = Game.C4S.PlrStart[PlrStartIndex].Position[0];
	pty = Game.C4S.PlrStart[PlrStartIndex].Position[1];

	// Zoomed position
	if (ptx > -1) ptx = BoundBy<int32_t>(ptx * Game.C4S.Landscape.MapZoom.Evaluate(), 0, GBackWdt - 1);
	if (pty > -1) pty = BoundBy<int32_t>(pty * Game.C4S.Landscape.MapZoom.Evaluate(), 0, GBackHgt - 1);

	// Standard position (PrefPosition)
	if (ptx < 0)
		if (Game.Parameters.StartupPlayerCount >= 2)
		{
			int32_t iMaxPos = Game.Parameters.StartupPlayerCount;
			// Map preferred position to available positions
			int32_t iStartPos = BoundBy(PrefPosition * iMaxPos / C4P_MaxPosition, 0, iMaxPos - 1);
			int32_t iPosition = iStartPos;
			// Distribute according to availability
			while (Game.Players.PositionTaken(iPosition))
			{
				++iPosition %= iMaxPos; if (iPosition == iStartPos) break;
			}
			Position = iPosition;
			// Set x position
			ptx = BoundBy(16 + Position * (GBackWdt - 32) / (iMaxPos - 1), 0, GBackWdt - 16);
		}

	// All-random position
	if (ptx < 0) ptx = 16 + Random(GBackWdt - 32);
	if (pty < 0) pty = 16 + Random(GBackHgt - 32);

	// Place to solid ground
	if (!Game.C4S.PlrStart[PlrStartIndex].EnforcePosition)
	{
		// Use nearest above-ground...
		FindSolidGround(ptx, pty, 30);
		// Might have hit a small lake, or similar: Seach a real site spot from here
		FindConSiteSpot(ptx, pty, 30, 50, C4D_Structure, 400);
	}

	// Place Readies
	C4Object *FirstBase = nullptr;
	PlaceReadyBase(ptx, pty, &FirstBase);
	PlaceReadyMaterial(ptx - 10, ptx + 10, pty, FirstBase);
	PlaceReadyVehic(ptx - 30, ptx + 30, pty, FirstBase);
	PlaceReadyCrew(ptx - 30, ptx + 30, pty, FirstBase);

	// set initial hostility by team info
	if (Team) SetTeamHostility();

	// Mouse control: init fog of war, if not specified otherwise by script
	if (MouseControl && !bForceFogOfWar)
		if (!fFogOfWarInitialized)
		{
			fFogOfWar = fFogOfWarInitialized = true;
			// reset view objects
			Game.Objects.AssignPlrViewRange();
		}

	// Scenario script initialization
	Game.Script.GRBroadcast(PSF_InitializePlayer, {C4VInt(Number),
		C4VInt(ptx),
		C4VInt(pty),
		C4VObj(FirstBase),
		C4VInt(Team),
		C4VID(GetInfo()->GetScriptPlayerExtraID())});
	return true;
}

bool C4Player::FinalInit(bool fInitialValue)
{
	if (!Status) return true;

	// Init player's mouse control
	if (LocalControl)
		if (MouseControl)
			Game.MouseControl.Init(Number);

	// Set initial value
	if (fInitialValue)
	{
		UpdateValue(); InitialValue = Value;
	}

	// Cursor
	if (!Cursor) AdjustCursorCommand();

	// Assign Captain
	if (Game.Objects.Find(C4Id("KILC")))
		if (!Captain) Captain = GetHiRankActiveCrew(false);

	// Update counts, pointers, views, value
	UpdateValue();
	Execute();

	// Restore FoW after savegame
	if (fFogOfWar && !fFogOfWarInitialized)
	{
		fFogOfWarInitialized = true;
		// reset view objects
		Game.Objects.AssignPlrViewRange();
	}

	return true;
}

void C4Player::SetFoW(bool fEnable)
{
	// enable FoW
	if (fEnable && !fFogOfWarInitialized)
		Game.Objects.AssignPlrViewRange();
	// set flag
	fFogOfWar = fFogOfWarInitialized = fEnable;
	// forced (not activated by mouse)
	bForceFogOfWar = true;
}

C4Object *C4Player::Buy(C4ID id, bool fShowErrors, int32_t iForPlr, C4Object *pBuyObj)
{
	int32_t iAvailable; C4Def *pDef; C4Object *pThing;
	// Base owner eliminated
	if (Eliminated)
	{
		if (!fShowErrors) return nullptr;
		StartSoundEffect("Error", false, 100, pBuyObj);
		GameMsgPlayer(FormatString(LoadResStr("IDS_PLR_ELIMINATED"), GetName()).getData(), Number); return nullptr;
	}
	// Get def (base owner's homebase material)
	iAvailable = HomeBaseMaterial.GetIDCount(id);
	if (!(pDef = C4Id2Def(id))) return nullptr;
	// Object not available
	if (iAvailable <= 0) return nullptr;
	// get value
	int32_t iValue = pDef->GetValue(pBuyObj, Number);
	// Not enough wealth (base owner's wealth)
	if (iValue > Wealth)
	{
		if (!fShowErrors) return nullptr;
		GameMsgPlayer(LoadResStr("IDS_PLR_NOWEALTH"), Number);
		StartSoundEffect("Error", false, 100, pBuyObj); return nullptr;
	}
	// Decrease homebase material count
	HomeBaseMaterial.DecreaseIDCount(id, false);
	if (Game.Rules & C4RULE_TeamHombase) SyncHomebaseMaterialToTeam();
	// Reduce wealth
	DoWealth(-iValue);
	// Create object (for player)
	if (!(pThing = Game.CreateObject(id, pBuyObj, iForPlr))) return nullptr;
	// Make crew member
	if (pDef->CrewMember) if (ValidPlr(iForPlr))
		Game.Players.Get(iForPlr)->MakeCrewMember(pThing);
	// success
	pThing->Call(PSF_Purchase, {C4VInt(Number), C4VObj(pBuyObj)});
	if (!pThing->Status) return nullptr;
	return pThing;
}

bool C4Player::Sell2Home(C4Object *pObj)
{
	C4Object *cObj;
	if (!CanSell(pObj)) return false;
	// Sell contents first
	while (cObj = pObj->Contents.GetObject())
	{
		if (pObj->Contained) cObj->Enter(pObj->Contained); else cObj->Exit(cObj->x, cObj->y);
		Sell2Home(cObj);
	}
	// Do transaction
	DoWealth(+pObj->GetValue(pObj->Contained, Number));
	// script handling of SellTo
	C4ID id = pObj->Def->id;
	C4Def *pSellDef = pObj->Def;
	if (C4AulScriptFunc *f = pObj->Def->Script.SFn_SellTo)
	{
		id = f->Exec(pObj, {C4VInt(Number)}).getC4ID();
		pSellDef = C4Id2Def(id);
	}
	// Add to homebase material
	if (pSellDef)
	{
		HomeBaseMaterial.IncreaseIDCount(id, !!pSellDef->Rebuyable);
		if (Game.Rules & C4RULE_TeamHombase) SyncHomebaseMaterialToTeam();
	}
	// Remove object, eject any crew members
	if (pObj->Contained) pObj->Exit();
	pObj->Call(PSF_Sale, {C4VInt(Number)});
	pObj->AssignRemoval(true);
	// Done
	return true;
}

bool C4Player::CanSell(C4Object *const obj) const
{
	return !Eliminated && obj && obj->Status && !(obj->OCF & OCF_CrewMember);
}

bool C4Player::DoWealth(int32_t iChange)
{
	Wealth = BoundBy<int32_t>(Wealth + iChange, 0, 10000);
	if (LocalControl)
	{
		if (iChange > 0) StartSoundEffect("Cash");
		if (iChange < 0) StartSoundEffect("UnCash");
	}
	ViewWealth = C4ViewDelay;
	return true;
}

void C4Player::SetViewMode(int32_t iMode, C4Object *pTarget)
{
	// safe back
	ViewMode = iMode; ViewTarget = pTarget;
}

void C4Player::ResetCursorView()
{
	// reset view to cursor if any cursor exists
	if (!ViewCursor && !Cursor) return;
	SetViewMode(C4PVM_Cursor);
}

void C4Player::Evaluate()
{
	// do not evaluate twice
	if (Evaluated) return;

	const int32_t SuccessBonus = 100;

	// Set last round
	LastRound.Title = Game.Parameters.ScenarioTitle;
	time(reinterpret_cast<time_t *>(&LastRound.Date));
	LastRound.Duration = Game.Time;
	LastRound.Won = !Eliminated;
	// Melee: personal value gain score ...check Game.Objects(C4D_Goal)
	if (Game.C4S.Game.IsMelee()) LastRound.Score = std::max<int32_t>(ValueGain, 0);
	// Cooperative: shared score
	else LastRound.Score = (std::max)(Game.Players.AverageValueGain(), 0);
	LastRound.Level = 0; // unknown...
	LastRound.Bonus = SuccessBonus * LastRound.Won;
	LastRound.FinalScore = LastRound.Score + LastRound.Bonus;
	LastRound.TotalScore = Score + LastRound.FinalScore;

	// Update player
	Rounds++;
	if (LastRound.Won) RoundsWon++; else RoundsLost++;
	Score = LastRound.TotalScore;
	TotalPlayingTime += Game.Time - GameJoinTime;

	// Crew
	CrewInfoList.Evaluate();

	// league
	if (Game.Parameters.isLeague())
		EvaluateLeague(false, Game.GameOver && !Eliminated);

	// Player is now evaluated
	Evaluated = true;

	// round results
	Game.RoundResults.EvaluatePlayer(this);
}

void C4Player::Surrender()
{
	if (Surrendered) return;
	Surrendered = true;
	Eliminated = true;
	RetireDelay = C4RetireDelay;
	StartSoundEffect("Eliminated");
	LogF(LoadResStr("IDS_PRC_PLRSURRENDERED"), GetName());
}

bool C4Player::SetHostility(int32_t iOpponent, int32_t iHostility, bool fSilent)
{
	// Check opponent valid
	if (!ValidPlr(iOpponent) || (iOpponent == Number)) return false;
	// Set hostility
	Hostility.SetIDCount(iOpponent + 1, iHostility, true);
	// no announce in first frame, or if specified
	if (!Game.FrameCounter || fSilent) return true;
	// Announce
	StartSoundEffect("Trumpet");
	LogF(LoadResStr(iHostility ? "IDS_PLR_HOSTILITY" : "IDS_PLR_NOHOSTILITY"),
		GetName(), Game.Players.Get(iOpponent)->GetName());
	// Success
	return true;
}

C4Object *C4Player::GetHiRankActiveCrew(bool fSelectOnly)
{
	C4ObjectLink *clnk;
	C4Object *cobj, *hirank = nullptr;
	int32_t iHighestRank = -2, iRank;
	for (clnk = Crew.First; clnk && (cobj = clnk->Obj); clnk = clnk->Next)
		if (!cobj->CrewDisabled)
			if (!fSelectOnly || cobj->Select)
			{
				if (cobj->Info) iRank = cobj->Info->Rank; else iRank = -1;
				if (!hirank || (iRank > iHighestRank))
				{
					hirank = cobj;
					iHighestRank = iRank;
				}
			}
	return hirank;
}

void C4Player::SetTeamHostility()
{
	// team only
	if (!Team) return;
	// set hostilities
	for (C4Player *pPlr = Game.Players.First; pPlr; pPlr = pPlr->Next)
		if (pPlr != this)
		{
			bool fHostile = (pPlr->Team != Team);
			SetHostility(pPlr->Number, fHostile, true);
			pPlr->SetHostility(Number, fHostile, true);
		}
}

void C4Player::Clear()
{
	ClearGraphs();
	Crew.Clear();
	CrewInfoList.Clear();
	Menu.Clear();
	BigIcon.Clear();
	fFogOfWar = false; bForceFogOfWar = false;
	FoWViewObjs.Clear();
	fFogOfWarInitialized = false;
	while (pMsgBoardQuery)
	{
		C4MessageBoardQuery *pNext = pMsgBoardQuery->pNext;
		delete pMsgBoardQuery;
		pMsgBoardQuery = pNext;
	}
	delete pGamepad;
	pGamepad = nullptr;
	Status = 0;
}

void C4Player::Default()
{
	Filename[0] = 0;
	Status = 0;
	Number = C4P_Number_None;
	ID = 0;
	Team = 0;
	DefaultRuntimeData();
	Menu.Default();
	Crew.Default();
	CrewInfoList.Default();
	LocalControl = false;
	BigIcon.Default();
	Next = nullptr;
	fFogOfWar = false; fFogOfWarInitialized = false;
	bForceFogOfWar = false;
	FoWViewObjs.Default();
	LeagueEvaluated = false;
	GameJoinTime = 0; // overwritten in Init
	pstatControls = pstatActions = nullptr;
	ControlCount = ActionCount = 0;
	AutoContextMenu = ControlStyle = 0;
	HasExplicitJumpKey = false;
	LastControlType = PCID_None;
	LastControlID = 0;
	PressedComs = 0;
	pMsgBoardQuery = nullptr;
	pGamepad = nullptr;
	NoEliminationCheck = false;
	Evaluated = false;
	ColorDw = 0;
}

bool C4Player::Load(const char *szFilename, bool fSavegame, bool fLoadPortraits)
{
	C4Group hGroup;
	// Open group
	if (!hGroup.Open(szFilename)) return false;
	// Load core
	if (!C4PlayerInfoCore::Load(hGroup))
	{
		hGroup.Close(); return false;
	}
	// Load BigIcon
	if (hGroup.FindEntry(C4CFN_BigIcon)) BigIcon.Load(hGroup, C4CFN_BigIcon);
	// Load crew info list
	CrewInfoList.Load(hGroup, fLoadPortraits);
	// Close group
	hGroup.Close();
	// Success
	return true;
}

bool C4Player::Strip(const char *szFilename, bool fAggressive)
{
	// Opem group
	C4Group Grp;
	if (!Grp.Open(szFilename))
		return false;
	// Which type of stripping?
	if (!fAggressive)
	{
		// remove portrais
		Grp.Delete(C4CFN_Portraits, true);
		// remove bigicon, if the file size is too large
		size_t iBigIconSize = 0;
		if (Grp.FindEntry(C4CFN_BigIcon, nullptr, &iBigIconSize))
			if (iBigIconSize > C4NetResMaxBigicon * 1024)
				Grp.Delete(C4CFN_BigIcon);
		Grp.Close();
	}
	else
	{
		// Load info core and crew info list
		C4PlayerInfoCore PlrInfoCore;
		C4ObjectInfoList CrewInfoList;
		if (!PlrInfoCore.Load(Grp) || !CrewInfoList.Load(Grp, false))
			return false;
		// Strip crew info list (remove object infos that are invalid for this scenario)
		CrewInfoList.Strip(Game.Defs);
		// Create a new group that receives the bare essentials
		Grp.Close();
		if (!EraseItem(szFilename) ||
			!Grp.Open(szFilename, true))
			return false;
		// Save info core & crew info list to newly-created file
		if (!PlrInfoCore.Save(Grp) || !CrewInfoList.Save(Grp, true, true, &Game.Defs))
			return false;
		Grp.Close();
	}
	return true;
}

void C4Player::DrawHostility(C4Facet &cgo, int32_t iIndex)
{
	C4Player *pPlr;
	if (pPlr = Game.Players.GetByIndex(iIndex))
	{
		// Portrait
		if (Config.Graphics.ShowPortraits && pPlr->BigIcon.Surface)
			pPlr->BigIcon.Draw(cgo);
		// Standard player image
		else
			Game.GraphicsResource.fctCrewClr.DrawClr(cgo, true, pPlr->ColorDw);
		// Other player and hostile
		if (pPlr != this)
			if (Hostility.GetIDCount(pPlr->Number + 1))
				Game.GraphicsResource.fctMenu.GetPhase(7).Draw(cgo);
	}
}

bool C4Player::MakeCrewMember(C4Object *pObj, bool fForceInfo, bool fDoCalls)
{
	C4ObjectInfo *cInf = nullptr;
	if (!pObj || !pObj->Def->CrewMember || !pObj->Status) return false;

	// only if info is not yet assigned
	if (!pObj->Info && fForceInfo)
	{
		// Find crew info by name
		if (pObj->nInfo)
			cInf = CrewInfoList.GetIdle(pObj->nInfo.getData());

		// Set name source
		const char *cpNames = nullptr;
		if (pObj->Def->pClonkNames) cpNames = pObj->Def->pClonkNames->GetData();
		if (!cpNames) cpNames = Game.Names.GetData();

		// Find crew info by id
		if (!cInf)
			while (!(cInf = CrewInfoList.GetIdle(pObj->id, Game.Defs)))
				if (!CrewInfoList.New(pObj->id, &Game.Defs, cpNames))
					return false;

		// Set object info
		pObj->Info = cInf;
	}

	// Add to crew
	if (!Crew.GetLink(pObj))
		Crew.Add(pObj, C4ObjectList::stMain);

	// add plr view
	if (!pObj->PlrViewRange) pObj->SetPlrViewRange(C4FOW_Def_View_RangeX); else pObj->PlrFoWActualize();

	// controlled by the player
	pObj->Controller = Number;

	// OnJoinCrew callback
	if (fDoCalls)
	{
		pObj->Call(PSF_OnJoinCrew, {C4VInt(Number)});
	}

	return true;
}

void C4Player::ExecuteControl()
{
	// LastCom
	if (LastCom != COM_None)
	{
		// Advance delay counter
		LastComDelay++;
		// Check for COM_Single com (after delay)
		if (LastComDelay > C4DoubleClick)
		{
			// Pass additional COM_Single com (unless it already was a single com in the first place)
			if (!(LastCom & COM_Single))
				DirectCom(LastCom | COM_Single, 0); // Currently, com data is not stored for single coms...
			LastCom = COM_None;
			LastComDelay = 0;
		}
	}

	// LastComDownDouble
	if (LastComDownDouble > 0) LastComDownDouble--;
}

void C4Player::AdjustCursorCommand()
{
	// Reset view
	ResetCursorView();
	// Set cursor to hirank Select clonk
	C4Object *pHiRank = nullptr;
	// Find hirank Select
	pHiRank = GetHiRankActiveCrew(true);
	// If none, check non-Selects as well
	if (!pHiRank)
		pHiRank = GetHiRankActiveCrew(false);
	// The cursor is on someone else: set the cursor to the hirank
	C4Object *pPrev = Cursor;
	if (Cursor != pHiRank)
	{
		Cursor = pHiRank;
		UpdateView();
	}
	// UnSelect previous cursor
	if (pPrev && pPrev != Cursor) pPrev->UnSelect(true);
	// We have a cursor: do select it
	if (Cursor) Cursor->DoSelect();
	// Updates
	CursorFlash = 30;
}

void C4Player::CursorRight()
{
	C4ObjectLink *cLnk;
	// Get next crew member
	if (cLnk = Crew.GetLink(Cursor))
		for (cLnk = cLnk->Next; cLnk; cLnk = cLnk->Next)
			if (cLnk->Obj->Status && !cLnk->Obj->CrewDisabled) break;
	if (!cLnk)
		for (cLnk = Crew.First; cLnk; cLnk = cLnk->Next)
			if (cLnk->Obj->Status && !cLnk->Obj->CrewDisabled) break;
	if (cLnk) SetCursor(cLnk->Obj, false, true);
	// Updates
	CursorFlash = 30;
	CursorSelection = 1;
	UpdateView();
}

void C4Player::CursorLeft()
{
	C4ObjectLink *cLnk;
	// Get prev crew member
	if (cLnk = Crew.GetLink(Cursor))
		for (cLnk = cLnk->Prev; cLnk; cLnk = cLnk->Prev)
			if (cLnk->Obj->Status && !cLnk->Obj->CrewDisabled) break;
	if (!cLnk)
		for (cLnk = Crew.Last; cLnk; cLnk = cLnk->Prev)
			if (cLnk->Obj->Status && !cLnk->Obj->CrewDisabled) break;
	if (cLnk) SetCursor(cLnk->Obj, false, true);
	// Updates
	CursorFlash = 30;
	CursorSelection = 1;
	UpdateView();
}

void C4Player::UnselectCrew()
{
	C4Object *cObj; C4ObjectLink *cLnk; bool fCursorDeselected = false;
	for (cLnk = Crew.First; cLnk && (cObj = cLnk->Obj); cLnk = cLnk->Next)
	{
		if (Cursor == cObj) fCursorDeselected = true;
		if (cObj->Status)
			cObj->UnSelect();
	}
	// if cursor is outside crew (done by some scenarios), unselect that one, too! (script callback)
	if (Cursor && !fCursorDeselected) Cursor->UnSelect();
}

void C4Player::SelectSingleByCursor()
{
	// Unselect crew
	UnselectCrew();
	// Select cursor
	if (Cursor) Cursor->DoSelect();
	// Updates
	SelectFlash = 30;
	AdjustCursorCommand();
}

void C4Player::CursorToggle()
{
	C4ObjectLink *clnk;
	// Selection mode: toggle cursor select
	if (CursorSelection)
	{
		if (Cursor)
			if (Cursor->Select) Cursor->UnSelect(); else Cursor->DoSelect();
		CursorToggled = 1;
	}
	// Pure toggle: toggle all Select
	else
	{
		for (clnk = Crew.First; clnk; clnk = clnk->Next)
			if (!clnk->Obj->CrewDisabled)
				if (clnk->Obj->Select) clnk->Obj->UnSelect(); else clnk->Obj->DoSelect();
		AdjustCursorCommand();
	}
	// Updates
	SelectFlash = 30;
}

void C4Player::SelectAllCrew()
{
	C4ObjectLink *clnk;
	// Select all crew
	for (clnk = Crew.First; clnk; clnk = clnk->Next)
		clnk->Obj->DoSelect();
	// Updates
	AdjustCursorCommand();
	CursorSelection = CursorToggled = 0;
	SelectFlash = 30;
	// Game display
	if (LocalControl) StartSoundEffect("Ding");
}

void C4Player::UpdateSelectionToggleStatus()
{
	if (CursorSelection)
		// Select toggled: cursor to hirank
		if (CursorToggled)
			AdjustCursorCommand();
	// Cursor select only: single control
		else
			SelectSingleByCursor();
	CursorSelection = 0;
	CursorToggled = 0;
}

bool C4Player::ObjectCom(uint8_t byCom, int32_t iData) // By DirectCom
{
	if (Eliminated) return false;
#ifdef DEBUGREC_OBJCOM
	C4RCObjectCom rc = { byCom, iData, Number };
	AddDbgRec(RCT_PlrCom, &rc, sizeof(C4RCObjectCom));
#endif
	// Hide startup
	ShowStartup = false;
	// If regular com, update cursor & selection status
	if (!(byCom & COM_Single) && !(byCom & COM_Double) && (byCom < COM_ReleaseFirst || byCom > COM_ReleaseLast))
		UpdateSelectionToggleStatus();
	// Apply direct com to cursor object
	if (Cursor)
	{
		// update controller
		Cursor->Controller = Number;
		// send com
		Cursor->DirectCom(byCom, iData);
	}
	// Done
	return true;
}

void C4Player::ClearPressedComsSynced()
{
	Game.Input.Add(CID_PlrControl, new C4ControlPlayerControl(Number, COM_ClearPressedComs, 0));
}

bool C4Player::ObjectCommand(int32_t iCommand, C4Object *pTarget, int32_t iX, int32_t iY, C4Object *pTarget2, int32_t iData, int32_t iMode)
{
	// Eliminated
	if (Eliminated) return false;
	// Hide startup
	if (ShowStartup) ShowStartup = false;
	// Update selection & toggle status
	UpdateSelectionToggleStatus();
	// Apply to all selected crew members (in cursor range) except pTarget.
	// Set, Add, Append mode flags may be combined and have a priority order.
	C4ObjectLink *cLnk; C4Object *cObj; bool fCursorProcessed = false;
	for (cLnk = Crew.First; cLnk && (cObj = cLnk->Obj); cLnk = cLnk->Next)
	{
		if (cObj == Cursor) fCursorProcessed = true;
		if (cObj->Status) if (cObj->Select)
			if (!(iMode & C4P_Command_Range) || (Cursor && Inside<int32_t>(cObj->x - Cursor->x, -15, +15) && Inside<int32_t>(cObj->y - Cursor->y, -15, +15)))
				if (cObj != pTarget)
				{
					// Put command with unspecified put object
					if ((iCommand == C4CMD_Put) && !pTarget2)
					{
						// Special: the put command is only applied by this function if the clonk actually
						// has something to put. Also, the put count is adjusted to that the clonk will not try to put
						// more items than he actually has. This workaround is needed so the put command can be used
						// to tell all selected clonks to put when in a container, simulating the old all-throw behavior.
						if (cObj->Contents.ObjectCount(iData))
							ObjectCommand2Obj(cObj, iCommand, pTarget, std::min<int32_t>(iX, cObj->Contents.ObjectCount(iData)), iY, pTarget2, iData, iMode);
					}
					// Other command
					else
						ObjectCommand2Obj(cObj, iCommand, pTarget, iX, iY, pTarget2, iData, iMode);
					// don't issue multiple Construct-commands - store previous Clonk as target for next command object
					// note that if three Clonks get the command, and the middle one gets deleted (the Clonk, not the command), the third will start to build anyway
					// that is very unlikely, though...could be catched in ClearPointers, if need be?
					// also, if one Clonk of the chain aborts his command (controlled elsewhere, for instance), the following ones will fail their commands
					// It's not a perfect solution, after all. But certainly better than placing tons of construction sites in the first place
					if (iCommand == C4CMD_Construct) pTarget = cObj;
				}
	}
	// Always apply to cursor, even if it's not in the crew
	if (Cursor && !fCursorProcessed)
		if (Cursor->Status && Cursor != pTarget)
			ObjectCommand2Obj(Cursor, iCommand, pTarget, iX, iY, pTarget2, iData, iMode);

	// Success
	return true;
}

void C4Player::ObjectCommand2Obj(C4Object *cObj, int32_t iCommand, C4Object *pTarget, int32_t iX, int32_t iY, C4Object *pTarget2, int32_t iData, int32_t iMode)
{
	// forward to object
	if (iMode & C4P_Command_Append) cObj->AddCommand(iCommand, pTarget, iX, iY, 0, pTarget2, true, iData, true, 0, nullptr, C4CMD_Mode_Base); // append: by Shift-click and for dragging of multiple objects (all independent; thus C4CMD_Mode_Base)
	else if (iMode & C4P_Command_Add) cObj->AddCommand(iCommand, pTarget, iX, iY, 0, pTarget2, true, iData, false, 0, nullptr, C4CMD_Mode_Base); // append: by context menu and keyboard throw command (all independent; thus C4CMD_Mode_Base)
	else if (iMode & C4P_Command_Set) cObj->SetCommand(iCommand, pTarget, iX, iY, pTarget2, true, iData);
}

void C4Player::DirectCom(uint8_t byCom, int32_t iData) // By InCom or ExecuteControl
{
	switch (byCom)
	{
		case COM_CursorLeft:
		case COM_CursorLeft_D:
		case COM_CursorRight:
		case COM_CursorRight_D:
		case COM_CursorToggle:
		case COM_CursorToggle_D:
			if (!Eliminated && Cursor)
			{
				Cursor->Controller = Number;
				if (Cursor->CallControl(this, byCom))
				{
					if (!(byCom & COM_Double))
					{
						UpdateSelectionToggleStatus();
					}
					return;
				}
			}
			break;
		default:
			break;
	}
	switch (byCom)
	{
	case COM_CursorLeft:  case COM_CursorLeft_D:  CursorLeft();    break;
	case COM_CursorRight: case COM_CursorRight_D: CursorRight();   break;
	case COM_CursorToggle:   CursorToggle();  break;
	case COM_CursorToggle_D: SelectAllCrew(); break;

	default: ObjectCom(byCom, iData); break;
	}
}

void C4Player::InCom(uint8_t byCom, int32_t iData)
{
#ifdef DEBUGREC_OBJCOM
	C4RCObjectCom rc = { byCom, iData, Number };
	AddDbgRec(RCT_PlrInCom, &rc, sizeof(C4RCObjectCom));
#endif
	if (byCom == COM_ClearPressedComs)
	{
		PressedComs = 0;
		LastCom = COM_None;
		return;
	}
	// Cursor object menu active: convert regular com to menu com
	if (Cursor) if (Cursor->Menu)
	{
		int32_t iCom = byCom;
		Cursor->Menu->ConvertCom(iCom, iData, false);
		byCom = iCom;
	}
	// Menu control: no single/double processing
	if (Inside(byCom, COM_MenuFirst, COM_MenuLast))
	{
		DirectCom(byCom, iData); return;
	}
	// Ignore KeyRelease for Single/Double
	if (!Inside(byCom, COM_ReleaseFirst, COM_ReleaseLast))
	{
		// Reset view
		ResetCursorView();
		// Update state
		if (Inside<int>(byCom, COM_ReleaseFirst - 16, COM_ReleaseLast - 16))
			PressedComs |= 1 << byCom;
		// Check LastCom buffer for prior COM_Single
		if (LastCom != COM_None)
			if (LastCom != byCom)
			{
				DirectCom(LastCom | COM_Single, iData);
				// AutoStopControl uses a single COM_Down instead of DOM_Down_D for drop
				// So a COM_Down_S does what a COM_Down_D normally does, if generated by another key
				// instead of a timeout
				if (ControlStyle && LastCom == COM_Down) LastComDownDouble = C4DoubleClick;
			}
		// Check LastCom buffer for COM_Double
		if (LastCom == byCom) byCom |= COM_Double;
		// LastCom/Del process
		// this is set before issuing the DirectCom, so DirectCom-scripts may delete it
		LastCom = byCom; LastComDelay = 0;
	}
	else
	{
		// Update state
		if (Inside(byCom, COM_ReleaseFirst, COM_ReleaseLast))
		{
			if (!(PressedComs & (1 << (byCom - 16))))
			{
				return;
			}
			PressedComs &= ~(1 << (byCom - 16));
		}
	}
	// Pass regular/COM_Double byCom to player
	DirectCom(byCom, iData);
	// LastComDownDouble process
	if (byCom == COM_Down_D) LastComDownDouble = C4DoubleClick;
}

void C4Player::CompileFunc(StdCompiler *pComp)
{
	assert(ID);

	pComp->Value(mkNamingAdapt(Status,                   "Status",            0));
	pComp->Value(mkNamingAdapt(AtClient,                 "AtClient",          C4ClientIDUnknown));
	pComp->Value(mkNamingAdapt(toC4CStr(AtClientName),   "AtClientName",      "Local"));
	pComp->Value(mkNamingAdapt(Number,                   "Index",             C4P_Number_None));
	pComp->Value(mkNamingAdapt(ID,                       "ID",                0));
	pComp->Value(mkNamingAdapt(Eliminated,               "Eliminated",        0));
	pComp->Value(mkNamingAdapt(Surrendered,              "Surrendered",       0));
	pComp->Value(mkNamingAdapt(Evaluated,                "Evaluated",         false));
	pComp->Value(mkNamingAdapt(Color,                    "Color",             -1));
	pComp->Value(mkNamingAdapt(ColorDw,                  "ColorDw",           0u));
	pComp->Value(mkNamingAdapt(Control,                  "Control",           0));
	pComp->Value(mkNamingAdapt(MouseControl,             "MouseControl",      0));
	pComp->Value(mkNamingAdapt(AutoContextMenu,          "AutoContextMenu",   0));
	pComp->Value(mkNamingAdapt(ControlStyle,             "AutoStopControl",   0));
	pComp->Value(mkNamingAdapt(HasExplicitJumpKey,       "HasExplicitJumpKey", 0));
	pComp->Value(mkNamingAdapt(Position,                 "Position",          0));
	pComp->Value(mkNamingAdapt(ViewMode,                 "ViewMode",          C4PVM_Cursor));
	pComp->Value(mkNamingAdapt(ViewX,                    "ViewX",             0));
	pComp->Value(mkNamingAdapt(ViewY,                    "ViewY",             0));
	pComp->Value(mkNamingAdapt(ViewWealth,               "ViewWealth",        0));
	pComp->Value(mkNamingAdapt(ViewValue,                "ViewValue",         0));
	pComp->Value(mkNamingAdapt(fFogOfWar,                "FogOfWar",          false));
	pComp->Value(mkNamingAdapt(bForceFogOfWar,           "ForceFogOfWar",     false));
	pComp->Value(mkNamingAdapt(ShowStartup,              "ShowStartup",       false));
	pComp->Value(mkNamingAdapt(ShowControl,              "ShowControl",       0));
	pComp->Value(mkNamingAdapt(ShowControlPos,           "ShowControlPos",    0));
	pComp->Value(mkNamingAdapt(Wealth,                   "Wealth",            0));
	pComp->Value(mkNamingAdapt(Points,                   "Points",            0));
	pComp->Value(mkNamingAdapt(Value,                    "Value",             0));
	pComp->Value(mkNamingAdapt(InitialValue,             "InitialValue",      0));
	pComp->Value(mkNamingAdapt(ValueGain,                "ValueGain",         0));
	pComp->Value(mkNamingAdapt(ObjectsOwned,             "ObjectsOwned",      0));
	pComp->Value(mkNamingAdapt(Hostility,                "Hostile"));
	pComp->Value(mkNamingAdapt(ProductionDelay,          "ProductionDelay",   0));
	pComp->Value(mkNamingAdapt(ProductionUnit,           "ProductionUnit",    0));
	pComp->Value(mkNamingAdapt(SelectCount,              "SelectCount",       0));
	pComp->Value(mkNamingAdapt(SelectFlash,              "SelectFlash",       0));
	pComp->Value(mkNamingAdapt(CursorFlash,              "CursorFlash",       0));
	pComp->Value(mkNamingAdapt(reinterpret_cast<int32_t &>(Cursor),        "Cursor",            0));
	pComp->Value(mkNamingAdapt(reinterpret_cast<int32_t &>(ViewCursor),    "ViewCursor",        0));
	pComp->Value(mkNamingAdapt(reinterpret_cast<int32_t &>(Captain),       "Captain",           0));
	pComp->Value(mkNamingAdapt(LastCom,                  "LastCom",           0));
	pComp->Value(mkNamingAdapt(LastComDelay,             "LastComDel",        0));
	pComp->Value(mkNamingAdapt(PressedComs,              "PressedComs",       0));
	pComp->Value(mkNamingAdapt(LastComDownDouble,        "LastComDownDouble", 0));
	pComp->Value(mkNamingAdapt(CursorSelection,          "CursorSelection",   0));
	pComp->Value(mkNamingAdapt(CursorToggled,            "CursorToggled",     0));
	pComp->Value(mkNamingAdapt(MessageStatus,            "MessageStatus",     0));
	pComp->Value(mkNamingAdapt(toC4CStr(MessageBuf),     "MessageBuf",        ""));
	pComp->Value(mkNamingAdapt(HomeBaseMaterial,         "HomeBaseMaterial"));
	pComp->Value(mkNamingAdapt(HomeBaseProduction,       "HomeBaseProduction"));
	pComp->Value(mkNamingAdapt(Knowledge,                "Knowledge"));
	pComp->Value(mkNamingAdapt(Magic,                    "Magic"));
	pComp->Value(mkNamingAdapt(Crew,                     "Crew"));
	pComp->Value(mkNamingAdapt(CrewInfoList.iNumCreated, "CrewCreated",       0));
	pComp->Value(mkNamingPtrAdapt(pMsgBoardQuery,        "MsgBoardQueries"));
}

bool C4Player::LoadRuntimeData(C4Group &hGroup)
{
	const char *pSource;
	// Use loaded game text component
	if (!(pSource = Game.GameText.GetData())) return false;
	// safety: Do nothing if playeer section is not even present (could kill initialized values)
	if (!SSearch(pSource, FormatString("[Player%i]", ID).getData())) return false;
	// Compile (Search player section - runtime data is stored by unique player ID)
	assert(ID);
	if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(
		mkNamingAdapt(*this, FormatString("Player%i", ID).getData()),
		StdStrBuf::MakeRef(pSource),
		Game.GameText.GetFilePath()))
		return false;
	// Denumerate pointers
	DenumeratePointers();
	// Success
	return true;
}

void C4Player::ExecHomeBaseProduction()
{
	const int32_t MaxHomeBaseProduction = 25;
	ProductionDelay++;
	if (ProductionDelay >= 60) // Minute Production Unit
	{
		// team home base: Only execute production on first player of team
		C4Team *pTeam;
		if (Game.Rules & C4RULE_TeamHombase)
			if (pTeam = Game.Teams.GetTeamByID(Team))
				if (pTeam->GetFirstActivePlayerID() != ID)
					// other players will be synced by first player
					return;
		// do production
		bool fAnyChange = false;
		ProductionDelay = 0; ProductionUnit++;
		for (int32_t cnt = 0; HomeBaseProduction.GetID(cnt); cnt++)
			if (HomeBaseProduction.GetCount(cnt) > 0)
				if (ProductionUnit % BoundBy<int32_t>(11 - HomeBaseProduction.GetCount(cnt), 1, 10) == 0)
					if (HomeBaseMaterial.GetIDCount(HomeBaseProduction.GetID(cnt)) < MaxHomeBaseProduction)
					{
						HomeBaseMaterial.IncreaseIDCount(HomeBaseProduction.GetID(cnt));
						fAnyChange = true;
					}
		// All team members get same production if rule is active
		if (fAnyChange) if (Game.Rules & C4RULE_TeamHombase)
			SyncHomebaseMaterialToTeam();
	}
}

void C4Player::UpdateCounts()
{
	int32_t nclkcnt, nselcnt;
	C4Object *cobj; C4ObjectLink *clnk;
	nclkcnt = nselcnt = 0;
	for (clnk = Crew.First; clnk && (cobj = clnk->Obj); clnk = clnk->Next)
	{
		nclkcnt++; if (cobj->Select) nselcnt++;
	}
	if (CrewCnt != nclkcnt) CrewCnt = nclkcnt;
	if (SelectCount != nselcnt) SelectCount = nselcnt;
}

void C4Player::CheckElimination()
{
	// Standard elimination: no crew
	if (CrewCnt <= 0)
		// Already eliminated safety
		if (!Eliminated)
			// No automatic elimination desired?
			if (!NoEliminationCheck)
				// Do elimination!
				Eliminate();
}

void C4Player::UpdateView()
{
	// view target/cursor
	switch (ViewMode)
	{
	case C4PVM_Cursor:
	{
		C4Object *pViewObj;
		if (!(pViewObj = ViewCursor)) pViewObj = Cursor;
		if (pViewObj)
		{
			ViewX = pViewObj->x; ViewY = pViewObj->y;
		}
		break;
	}
	case C4PVM_Target:
		if (ViewTarget)
		{
			ViewX = ViewTarget->x; ViewY = ViewTarget->y;
		}
		break;
	case C4PVM_Scrolling:
		break;
	}
}

void C4Player::DefaultRuntimeData()
{
	Status = 0;
	Eliminated = 0;
	Surrendered = 0;
	AtClient = C4ClientIDUnknown;
	SCopy("Local", AtClientName);
	Color = -1;
	Control = C4P_Control_None;
	MouseControl = false;
	Position = -1;
	PlrStartIndex = 0;
	RetireDelay = 0;
	ViewMode = C4PVM_Cursor;
	ViewX = ViewY = 0;
	ViewTarget = nullptr;
	CursorSelection = CursorToggled = 0;
	ShowStartup = true;
	CrewCnt = 0;
	ViewWealth = ViewValue = 0;
	ShowControl = ShowControlPos = 0;
	Wealth = 0;
	Points = 0;
	Value = InitialValue = ValueGain = 0;
	ObjectsOwned = 0;
	Captain = nullptr;
	ProductionDelay = ProductionUnit = 0;
	Cursor = ViewCursor = nullptr;
	SelectCount = 0;
	SelectFlash = CursorFlash = 30;
	LastCom = 0;
	LastComDelay = 0;
	LastComDownDouble = 0;
	CursorSelection = CursorToggled = 0;
	MessageStatus = 0;
	MessageBuf[0] = 0;
	Hostility.Clear();
	HomeBaseMaterial.Clear();
	HomeBaseProduction.Clear();
	Knowledge.Clear();
	Magic.Clear();
	FlashCom = 0;
}

bool C4Player::ActivateMenuTeamSelection(bool fFromMain)
{
	// Menu symbol/init
	bool fSwitch = !(Status == PS_TeamSelection);
	Menu.InitRefSym(C4GUI::Icon::GetIconFacet(C4GUI::Ico_Team), LoadResStr("IDS_MSG_SELTEAM"), Number, C4MN_Extra_None, 0, fSwitch ? C4MN_TeamSwitch : C4MN_TeamSelection);
	Menu.SetAlignment(fSwitch ? C4MN_Align_Left | C4MN_Align_Bottom : 0);
	Menu.Refill();
	// Go back to options menu on close
	if (fFromMain) Menu.SetCloseCommand("ActivateMenu:Main");
	return true;
}

void C4Player::DoTeamSelection(int32_t idTeam)
{
	// stop team selection. This might close the menu forever if the control gets lost
	// let's hope it doesn't!
	Status = PS_TeamSelectionPending;
	Game.Control.DoInput(CID_InitScenarioPlayer, new C4ControlInitScenarioPlayer(Number, idTeam), CDT_Queue);
}

void C4Player::EnumeratePointers()
{
	EnumerateObjectPtrs(Cursor, ViewCursor, Captain);
	for (C4MessageBoardQuery *pCheck = pMsgBoardQuery; pCheck; pCheck = pCheck->pNext)
		pCheck->pCallbackObj.Enumerate();
}

void C4Player::DenumeratePointers()
{
	DenumerateObjectPtrs(Cursor, ViewCursor, Captain);
	// Crew
	Crew.DenumerateRead();
	// messageboard-queries
	for (C4MessageBoardQuery *pCheck = pMsgBoardQuery; pCheck; pCheck = pCheck->pNext)
		pCheck->pCallbackObj.Denumerate();
}

void C4Player::RemoveCrewObjects()
{
	C4Object *pCrew;

	// Remove all crew objects
	while (pCrew = Crew.GetObject()) pCrew->AssignRemoval(true);
}

void C4Player::NotifyOwnedObjects()
{
	C4Object *cobj; C4ObjectLink *clnk;

	// notify objects in all object lists
	for (C4ObjectList *pList = &Game.Objects; pList; pList = ((pList == &Game.Objects) ? &Game.Objects.InactiveObjects : nullptr))
		for (clnk = pList->First; clnk && (cobj = clnk->Obj); clnk = clnk->Next)
			if (cobj->Status)
				if (cobj->Owner == Number)
				{
					C4AulFunc *pFn = cobj->Def->Script.GetFuncRecursive(PSF_OnOwnerRemoved);
					// PSF_OnOwnerRemoved has an internal fallback function
					assert(pFn);
					if (pFn) pFn->Exec(cobj);
				}
}

bool C4Player::DoPoints(int32_t iChange)
{
	Points = BoundBy<int32_t>(Points + iChange, -100000, 100000);
	ViewValue = C4ViewDelay;
	return true;
}

void C4Player::SetCursor(C4Object *pObj, bool fSelectFlash, bool fSelectArrow)
{
	// check disabled
	if (pObj) if (pObj->CrewDisabled) return;
	bool fChanged = pObj != Cursor;
	C4Object *pPrev = Cursor;
	// Set cursor
	Cursor = pObj;
	// unselect previous
	if (pPrev && fChanged) pPrev->UnSelect(true);
	// Select object
	if (Cursor) Cursor->DoSelect(true);
	// View flash
	if (fSelectArrow) CursorFlash = 30;
	if (fSelectFlash) SelectFlash = 30;
}

void C4Player::SelectCrew(C4ObjectList &rList)
{
	// Unselect
	UnselectCrew();
	// Select (does not check whether objects are in crew)
	C4ObjectLink *clnk;
	for (clnk = rList.First; clnk; clnk = clnk->Next)
		if (clnk->Obj->Status)
			clnk->Obj->DoSelect();
	// Updates
	AdjustCursorCommand();
	CursorSelection = CursorToggled = 0;
	SelectFlash = 30;
}

void C4Player::ScrollView(int32_t iX, int32_t iY, int32_t ViewWdt, int32_t ViewHgt)
{
	SetViewMode(C4PVM_Scrolling);
	int32_t ViewportScrollBorder = Application.isFullScreen ? C4ViewportScrollBorder : 0;
	ViewX = BoundBy<int32_t>(ViewX + iX, ViewWdt / 2 - ViewportScrollBorder, GBackWdt + ViewportScrollBorder - ViewWdt / 2);
	ViewY = BoundBy<int32_t>(ViewY + iY, ViewHgt / 2 - ViewportScrollBorder, GBackHgt + ViewportScrollBorder - ViewHgt / 2);
}

void C4Player::InitControl()
{
	// Check local control
	LocalControl = false;
	if (AtClient == Game.Control.ClientID())
		if (!GetInfo() || GetInfo()->GetType() == C4PT_User)
			LocalControl = true;
	// Set control
	Control = C4P_Control_None;
	// Preferred control
	int32_t iControl = PrefControl;
	// gamepad control safety
	if (Inside<int32_t>(iControl, C4P_Control_GamePad1, C4P_Control_GamePadMax) && !Config.General.GamepadEnabled)
	{
		iControl = C4P_Control_Keyboard1;
	}
	// Choose next while control taken
	if (Game.Players.ControlTaken(iControl))
	{
		// Preferred control taken, search for available keyboard control
		for (iControl = C4P_Control_Keyboard1; iControl <= C4P_Control_Keyboard4; iControl++)
			if (!Game.Players.ControlTaken(iControl)) // Available control found
				break;
		// No available control found
		if (iControl > C4P_Control_Keyboard4)
			iControl = C4P_Control_None;
	}
	// Set control
	Control = iControl;
	ApplyForcedControl();
	// init gamepad
	delete pGamepad; pGamepad = nullptr;
	if (Inside<int32_t>(Control, C4P_Control_GamePad1, C4P_Control_GamePadMax))
	{
		pGamepad = new C4GamePadOpener(Control - C4P_Control_GamePad1);
		// Check if the control set has a COM_Jump key configured, and if found, disable the jump function on COM_Up
		HasExplicitJumpKey = Config.Gamepads[Control - C4P_Control_GamePad1].HasExplicitJumpButton();
	}
	// Mouse
	if (PrefMouse && !Game.Control.isReplay())
		if (!Game.C4S.Head.DisableMouse)
			if (Inside<int32_t>(Control, C4P_Control_Keyboard1, C4P_Control_GamePadMax))
				if (!Game.Players.MouseControlTaken())
					MouseControl = true;
	// no controls issued yet
	ControlCount = ActionCount = 0;
	LastControlType = PCID_None;
	LastControlID = 0;
	PressedComs = 0;
}

void C4Player::FoW2Map(CClrModAddMap &rMap, int iOffX, int iOffY)
{
	// No fog of war
	if (!fFogOfWar) return;
	// Add view for all FoW-repellers - keep track of FoW-generators, which should be avaluated finally
	// so they override repellers
	bool fAnyGenerators = false;
	C4Object *cobj; C4ObjectLink *clnk;
	for (clnk = FoWViewObjs.First; clnk && (cobj = clnk->Obj); clnk = clnk->Next)
		if (!cobj->Contained || cobj->Contained->Def->ClosedContainer != 1)
			if (cobj->PlrViewRange > 0)
				rMap.ReduceModulation(cobj->x + iOffX, cobj->y + iOffY, cobj->PlrViewRange * 2 / 3, cobj->PlrViewRange);
			else
				fAnyGenerators = true;
	// Add view for target view object
	if (ViewMode == C4PVM_Target)
		if (ViewTarget)
			if (!ViewTarget->Contained || ViewTarget->Contained->Def->ClosedContainer != 1)
			{
				int iRange = ViewTarget->PlrViewRange;
				if (!iRange && Cursor) iRange = Cursor->PlrViewRange;
				if (!iRange) iRange = C4FOW_Def_View_RangeX;
				rMap.ReduceModulation(ViewTarget->x + iOffX, ViewTarget->y + iOffY, iRange * 2 / 3, iRange);
			}
	// apply generators
	// do this check, be cause in 99% of all normal scenarios, there will be no FoW-generators
	if (fAnyGenerators) FoWGenerators2Map(rMap, iOffX, iOffY);
}

void C4Player::FoWGenerators2Map(CClrModAddMap &rMap, int iOffX, int iOffY)
{
	// add fog to any generator pos (view range
	C4Object *cobj; C4ObjectLink *clnk;
	for (clnk = FoWViewObjs.First; clnk && (cobj = clnk->Obj); clnk = clnk->Next)
		if (!cobj->Contained || cobj->Contained->Def->ClosedContainer != 1)
			if (cobj->PlrViewRange < 0)
				rMap.AddModulation(cobj->x + iOffX, cobj->y + iOffY, -cobj->PlrViewRange, -cobj->PlrViewRange + 200, cobj->ColorMod >> 24);
}

bool C4Player::FoWIsVisible(int32_t x, int32_t y)
{
	// check repellers and generators and ViewTarget
	bool fSeen = false;
	C4Object *cobj = nullptr; C4ObjectLink *clnk;
	clnk = FoWViewObjs.First;
	int32_t iRange;
	for (;;)
	{
		if (clnk)
		{
			cobj = clnk->Obj;
			clnk = clnk->Next;
			iRange = cobj->PlrViewRange;
		}
		else if (ViewMode != C4PVM_Target || !ViewTarget || ViewTarget == cobj)
			break;
		else
		{
			cobj = ViewTarget;
			iRange = cobj->PlrViewRange;
			if (!iRange && Cursor) iRange = Cursor->PlrViewRange;
			if (!iRange) iRange = C4FOW_Def_View_RangeX;
		}
		if (!cobj->Contained || cobj->Contained->Def->ClosedContainer != 1)
			if (Distance(cobj->x, cobj->y, x, y) < Abs(iRange))
				if (iRange < 0)
				{
					if (!(cobj->ColorMod & 0xff000000)) // faded generators generate darkness only; no FoW blocking
						return false; // shadowed by FoW-generator
				}
				else
					fSeen = true; // made visible by FoW-repeller
	}
	return fSeen;
}

void C4Player::SelectCrew(C4Object *pObj, bool fSelect)
{
	// Not a valid crew member
	if (!pObj || !Crew.GetLink(pObj)) return;
	// Select/unselect
	if (fSelect) pObj->DoSelect();
	else pObj->UnSelect();
	// Updates
	SelectFlash = 30;
	CursorSelection = CursorToggled = 0;
	AdjustCursorCommand();
}

void C4Player::CloseMenu()
{
	// cancel all player menus
	Menu.Close(false);
}

void C4Player::Eliminate()
{
	if (Eliminated) return;
	Eliminated = true;
	RetireDelay = C4RetireDelay;
	StartSoundEffect("Eliminated");
	LogF(LoadResStr("IDS_PRC_PLRELIMINATED"), GetName());

	// Early client deactivation check
	if (Game.Control.isCtrlHost() && AtClient > C4ClientIDHost)
	{
		// Check: Any player left at this client?
		C4Player *pPlr = nullptr;
		for (int i = 0; pPlr = Game.Players.GetAtClient(AtClient, i); i++)
			if (!pPlr->Eliminated)
				break;
		// If not, deactivate the client
		if (!pPlr)
			Game.Control.DoInput(CID_ClientUpdate,
				new C4ControlClientUpdate(AtClient, CUT_Activate, false),
				CDT_Sync);
	}
}

int32_t C4Player::ActiveCrewCount()
{
	// get number of objects in crew that is not disabled
	int32_t iNum = 0;
	C4Object *cObj;
	for (C4ObjectLink *cLnk = Crew.First; cLnk; cLnk = cLnk->Next)
		if (cObj = cLnk->Obj)
			if (!cObj->CrewDisabled)
				++iNum;
	// return it
	return iNum;
}

int32_t C4Player::GetSelectedCrewCount()
{
	int32_t iNum = 0;
	C4Object *cObj;
	for (C4ObjectLink *cLnk = Crew.First; cLnk; cLnk = cLnk->Next)
		if (cObj = cLnk->Obj)
			if (!cObj->CrewDisabled)
				if (cObj->Select)
					++iNum;
	// return it
	return iNum;
}

void C4Player::EvaluateLeague(bool fDisconnected, bool fWon)
{
	// already evaluated?
	if (LeagueEvaluated) return; LeagueEvaluated = true;
	// set fate
	C4PlayerInfo *pInfo = GetInfo();
	if (pInfo)
	{
		if (fDisconnected)
			pInfo->SetDisconnected();
		if (fWon)
			pInfo->SetWinner();
	}
}

bool C4Player::LocalSync()
{
	// local sync not necessary for script players
	if (GetType() == C4PT_Script) return true;
	// evaluate total playing time
	TotalPlayingTime += Game.Time - GameJoinTime;
	GameJoinTime = Game.Time;
	// evaluate total playing time of all the crew
	for (C4ObjectInfo *pInf = CrewInfoList.GetFirst(); pInf; pInf = pInf->Next)
		if (pInf->InAction)
		{
			pInf->TotalPlayingTime += (Game.Time - pInf->InActionTime);
			pInf->InActionTime = Game.Time;
		}
	// save player
	if (!Save())
		return false;

	// done, success
	return true;
}

C4PlayerInfo *C4Player::GetInfo()
{
	return Game.PlayerInfos.GetPlayerInfoByID(ID);
}

bool C4Player::SetObjectCrewStatus(C4Object *pCrew, bool fNewStatus)
{
	// either add...
	if (fNewStatus)
	{
		// is in crew already?
		if (Crew.IsContained(pCrew)) return true;
		return MakeCrewMember(pCrew, false);
	}
	else
	{
		// already outside?
		if (!Crew.IsContained(pCrew)) return true;
		// ...or remove
		Crew.Remove(pCrew);
		// make sure it's not selected any more
		if (pCrew->Select) pCrew->UnSelect();
		// remove info, if assigned to this player
		// theoretically, info objects could remain when the player is deleted
		// but then they would be reassigned to the player crew when loaded in a savegame
		//  by the crew-assignment code kept for backwards compatibility with pre-4.95.2-savegames
		if (pCrew->Info && CrewInfoList.IsElement(pCrew->Info))
		{
			pCrew->Info->Retire();
			pCrew->Info = nullptr;
		}
	}
	// done, success
	return true;
}

void C4Player::CreateGraphs()
{
	// del prev
	ClearGraphs();
	// create graphs
	if (Game.pNetworkStatistics)
	{
		uint32_t dwGraphClr = ColorDw;
		C4PlayerInfo *pInfo;
		if (ID && (pInfo = Game.PlayerInfos.GetPlayerInfoByID(ID)))
		{
			// set color by player info class
			dwGraphClr = pInfo->GetColor();
		}
		C4GUI::MakeColorReadableOnBlack(dwGraphClr); dwGraphClr &= 0xffffff;
		pstatControls = new C4TableGraph(C4TableGraph::DefaultBlockLength * 20, Game.pNetworkStatistics->ControlCounter);
		pstatControls->SetColorDw(dwGraphClr);
		pstatControls->SetTitle(GetName());
		pstatActions = new C4TableGraph(C4TableGraph::DefaultBlockLength * 20, Game.pNetworkStatistics->ControlCounter);
		pstatActions->SetColorDw(dwGraphClr);
		pstatActions->SetTitle(GetName());
		// register into
		Game.pNetworkStatistics->statControls.AddGraph(pstatControls);
		Game.pNetworkStatistics->statActions.AddGraph(pstatActions);
	}
}

void C4Player::ClearGraphs()
{
	// del all assigned graphs
	if (pstatControls && Game.pNetworkStatistics)
	{
		Game.pNetworkStatistics->statControls.RemoveGraph(pstatControls);
	}
	delete pstatControls;
	pstatControls = nullptr;
	if (pstatActions && Game.pNetworkStatistics)
	{
		Game.pNetworkStatistics->statActions.RemoveGraph(pstatActions);
	}
	delete pstatActions;
	pstatActions = nullptr;
}

void C4Player::CountControl(ControlType eType, int32_t iID, int32_t iCntAdd)
{
	// count it
	ControlCount += iCntAdd;
	// catch doubles
	if (eType == LastControlType && iID == LastControlID) return;
	// no double: count as action
	LastControlType = eType;
	LastControlID = iID;
	ActionCount += iCntAdd;
	// and give experience
	if (Cursor && Cursor->Info)
	{
		if (Cursor->Info)
		{
			Cursor->Info->ControlCount++; if ((Cursor->Info->ControlCount % 5) == 0) Cursor->DoExperience(+1);
		}
	}
}

void C4Player::ExecMsgBoardQueries()
{
	// query now possible?
	if (!C4GUI::IsGUIValid()) return;
	// already active?
	if (Game.MessageInput.IsTypeIn()) return;
	// find an un-evaluated query
	C4MessageBoardQuery *pCheck = pMsgBoardQuery;
	while (pCheck) if (!pCheck->fAnswered) break; else pCheck = pCheck->pNext;
	if (!pCheck) return;
	// open it
	Game.MessageInput.StartTypeIn(true, pCheck->pCallbackObj, pCheck->fIsUppercase, C4ChatInputDialog::All, Number, pCheck->sInputQuery);
}

void C4Player::CallMessageBoard(C4Object *pForObj, const StdStrBuf &sQueryString, bool fIsUppercase)
{
	// remove any previous query for the same object
	RemoveMessageBoardQuery(pForObj);
	// sort new query to end of list
	C4MessageBoardQuery **ppTarget = &pMsgBoardQuery;
	while (*ppTarget) ppTarget = &((*ppTarget)->pNext);
	*ppTarget = new C4MessageBoardQuery(pForObj, sQueryString, fIsUppercase);
}

bool C4Player::RemoveMessageBoardQuery(C4Object *pForObj)
{
	// get matching query
	C4MessageBoardQuery **ppCheck = &pMsgBoardQuery, *pFound;
	while (*ppCheck) if ((*ppCheck)->pCallbackObj == pForObj) break; else ppCheck = &((*ppCheck)->pNext);
	pFound = *ppCheck;
	if (!pFound) return false;
	// remove it
	*ppCheck = (*ppCheck)->pNext;
	delete pFound;
	return true;
}

bool C4Player::MarkMessageBoardQueryAnswered(C4Object *pForObj)
{
	// get matching query
	C4MessageBoardQuery *pCheck = pMsgBoardQuery;
	while (pCheck) if (pCheck->pCallbackObj == pForObj && !pCheck->fAnswered) break; else pCheck = pCheck->pNext;
	if (!pCheck) return false;
	// mark it
	pCheck->fAnswered = true;
	return true;
}

bool C4Player::HasMessageBoardQuery()
{
	// return whether any object has a messageboard-query
	return !!pMsgBoardQuery;
}

void C4Player::OnTeamSelectionFailed()
{
	// looks like a selected team was not available: Go back to team selection if this is not a mislead call
	if (Status == PS_TeamSelectionPending)
		Status = PS_TeamSelection;
}

void C4Player::SetPlayerColor(uint32_t dwNewClr)
{
	// no change?
	if (dwNewClr == ColorDw) return;
	// reflect change in all active, player-owned objects
	// this can never catch everything (thinking of overlays, etc.); scenarios that allow team changes should take care of the rest
	uint32_t dwOldClr = ColorDw;
	ColorDw = dwNewClr;
	C4Object *pObj;
	for (C4ObjectLink *pLnk = Game.Objects.First; pLnk; pLnk = pLnk->Next)
		if (pObj = pLnk->Obj)
			if (pObj->Status)
				if (pObj->Owner == Number)
				{
					if ((pObj->Color & 0xffffff) == (dwOldClr & 0xffffff))
						pObj->Color = (pObj->Color & 0xff000000u) | (dwNewClr & 0xffffff);
				}
}

C4PlayerType C4Player::GetType() const
{
	// type by info
	C4PlayerInfo *pInfo = Game.PlayerInfos.GetPlayerInfoByID(ID);
	if (pInfo) return pInfo->GetType(); else { assert(false); return C4PT_User; }
}

bool C4Player::IsInvisible() const
{
	// invisible by info
	C4PlayerInfo *pInfo = Game.PlayerInfos.GetPlayerInfoByID(ID);
	if (pInfo) return pInfo->IsInvisible(); else { assert(false); return false; }
}

void C4Player::ToggleMouseControl()
{
	// Activate mouse control if it's available
	if (!MouseControl && !Game.Players.MouseControlTaken())
	{
		Game.MouseControl.Init(Number);
		MouseControl = true;
		// init fog of war
		if (!fFogOfWar && !bForceFogOfWar)
		{
			fFogOfWar = fFogOfWarInitialized = true;
			Game.Objects.AssignPlrViewRange();
		}
	}
	// Deactivate mouse control
	else if (MouseControl)
	{
		Game.MouseControl.Clear();
		Game.MouseControl.Default();
		MouseControl = 0;
		// Scrolling isn't possible any more
		if (ViewMode == C4PVM_Scrolling)
			SetViewMode(C4PVM_Cursor);
		if (!bForceFogOfWar)
		{
			// delete fog of war
			fFogOfWarInitialized = fFogOfWar = false;
		}
	}
}

bool C4Player::ActivateMenuMain()
{
	// Not during game over dialog
	if (C4GameOverDlg::IsShown()) return false;
	// Open menu
	return !!Menu.ActivateMain(Number);
}

void C4Player::SyncHomebaseMaterialToTeam()
{
	// Copy own home base material to all team members
	C4Team *pTeam;
	if (Team && ((pTeam = Game.Teams.GetTeamByID(Team))))
	{
		int32_t iTeamSize = pTeam->GetPlayerCount();
		for (int32_t i = 0; i < iTeamSize; ++i)
		{
			int32_t iTeammate = pTeam->GetIndexedPlayer(i);
			C4Player *pTeammate;
			if (iTeammate != ID && ((pTeammate = Game.Players.GetByInfoID(iTeammate))))
			{
				pTeammate->HomeBaseMaterial = HomeBaseMaterial;
			}
		}
	}
}

void C4Player::SyncHomebaseMaterialFromTeam()
{
	// Copy own home base material from team's first player (unless it's ourselves)
	C4Team *pTeam;
	if (Team && ((pTeam = Game.Teams.GetTeamByID(Team))))
	{
		int32_t iTeamCaptain = pTeam->GetIndexedPlayer(0);
		C4Player *pTeamCaptain;
		if (iTeamCaptain != ID && ((pTeamCaptain = Game.Players.GetByInfoID(iTeamCaptain))))
		{
			HomeBaseMaterial = pTeamCaptain->HomeBaseMaterial;
		}
	}
}

void C4Player::ApplyForcedControl()
{
	const auto oldControl = ControlStyle;

	ControlStyle = ((Game.C4S.Head.ForcedControlStyle > -1) ? Game.C4S.Head.ForcedControlStyle : PrefControlStyle);
	AutoContextMenu = ((Game.C4S.Head.ForcedAutoContextMenu > -1) ? Game.C4S.Head.ForcedAutoContextMenu : PrefAutoContextMenu);

	if (oldControl != ControlStyle)
	{
		LastCom = COM_None;
		PressedComs = 0;
		if (ControlStyle) // AutoStopControl
		{
			for (auto objLink = Game.Objects.InactiveObjects.First; objLink; objLink = objLink->Next)
			{
				const auto obj = objLink->Obj;
				if (obj->Def->CrewMember && obj->Owner == Number)
				{
					obj->Action.ComDir = COMD_None;
				}
			}
		}
	}
}
