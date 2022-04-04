/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
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

#include "C4Include.h"
#include "C4GameParameters.h"

#include "C4Log.h"
#include "C4Components.h"
#include "C4Game.h"
#include "C4Gui.h"
#include "C4Wrappers.h"

#include <iterator>

// C4GameRes

C4GameRes::C4GameRes()
	: eType(NRT_Null), pResCore(nullptr), pNetRes(nullptr) {}

C4GameRes::C4GameRes(const C4GameRes &Res)
	: eType(Res.getType()), File(Res.getFile()), pResCore(Res.getResCore()), pNetRes(Res.getNetRes())
{
	if (pResCore && !pNetRes)
		pResCore = new C4Network2ResCore(*pResCore);
}

C4GameRes::~C4GameRes()
{
	Clear();
}

C4GameRes &C4GameRes::operator=(const C4GameRes &Res)
{
	Clear();
	eType = Res.getType();
	File = Res.getFile();
	pResCore = Res.getResCore();
	pNetRes = Res.getNetRes();
	if (pResCore && !pNetRes)
		pResCore = new C4Network2ResCore(*pResCore);
	return *this;
}

void C4GameRes::Clear()
{
	eType = NRT_Null;
	File.Clear();
	if (!pNetRes) delete pResCore;
	pResCore = nullptr;
	pNetRes = nullptr;
}

void C4GameRes::SetFile(C4Network2ResType enType, const char *sznFile)
{
	assert(!pNetRes && !pResCore);
	eType = enType;
	File = sznFile;
}

void C4GameRes::SetNetRes(C4Network2Res::Ref pnNetRes)
{
	Clear();
	pNetRes = pnNetRes;
	eType = pNetRes->getType();
	File = pNetRes->getFile();
	pResCore = &pNetRes->getCore();
}

void C4GameRes::CompileFunc(StdCompiler *pComp)
{
	bool fCompiler = pComp->isCompiler();
	// Clear previous data for compiling
	if (fCompiler) Clear();
	// Core is needed to decompile something meaningful
	if (!fCompiler) assert(pResCore);
	// De-/Compile core
	pComp->Value(mkPtrAdaptNoNull(const_cast<C4Network2ResCore *&>(pResCore)));
	// Compile: Set type accordingly
	if (fCompiler)
		eType = pResCore->getType();
}

bool C4GameRes::Publish(C4Network2ResList *pNetResList)
{
	assert(isPresent());
	// Already present?
	if (pNetRes) return true;
	// determine whether it's loadable
	bool fAllowUnloadable = false;
	if (eType == NRT_Definitions) fAllowUnloadable = true;
	// Add to network resource list
	C4Network2Res::Ref pNetRes = pNetResList->AddByFile(File.getData(), false, eType, -1, nullptr, fAllowUnloadable);
	if (!pNetRes) return false;
	// Set resource
	SetNetRes(pNetRes);
	return true;
}

bool C4GameRes::Load(C4Network2ResList *pNetResList)
{
	assert(pResCore);
	// Already present?
	if (pNetRes) return true;
	// Add to network resource list
	C4Network2Res::Ref pNetRes = pNetResList->AddByCore(*pResCore);
	if (!pNetRes) return false;
	// Set resource
	SetNetRes(pNetRes);
	return true;
}

bool C4GameRes::InitNetwork(C4Network2ResList *pNetResList)
{
	// Already initialized?
	if (getNetRes())
		return true;
	// Present? [Host]
	if (isPresent())
	{
		// Publish on network
		if (!Publish(pNetResList))
		{
			LogFatal(FormatString(LoadResStr("IDS_NET_NOFILEPUBLISH"), getFile()).getData());
			return false;
		}
	}
	// Got a core? [Client]
	else if (pResCore)
	{
		// Search/Load it
		if (!Load(pNetResList))
		{
			// Give some hints to why this might happen.
			const char *szFilename = pResCore->getFileName();
			if (!pResCore->isLoadable())
				if (pResCore->getType() == NRT_System)
					LogFatal(FormatString(LoadResStr("IDS_NET_NOSAMESYSTEM"), szFilename).getData());
				else
					LogFatal(FormatString(LoadResStr("IDS_NET_NOSAMEANDTOOLARGE"), szFilename).getData());
			// Should not happen
			else
				LogFatal(FormatString(LoadResStr("IDS_NET_NOVALIDCORE"), szFilename).getData());
			return false;
		}
	}
	// Okay
	return true;
}

void C4GameRes::CalcHash()
{
	if (!pNetRes) return;
	pNetRes->CalculateSHA();
}

// C4GameResList

C4GameResList &C4GameResList::operator=(const C4GameResList &List)
{
	Clear();
	resList.reserve(List.resList.size());
	std::transform(List.resList.begin(), List.resList.end(), std::back_inserter(resList), [](const auto &res)
	{
		return std::make_unique<C4GameRes>(*res);
	});
	return *this;
}

C4GameResList::ResTypeIterator C4GameResList::iterRes(C4Network2ResType type)
{
	return {type, resList};
}

void C4GameResList::Clear()
{
	resList.clear();
}

bool C4GameResList::Load(const std::vector<std::string> &DefinitionFilenames)
{
	// clear any prev
	Clear();
	// no defs to be added? that's OK (LocalOnly)
	if (DefinitionFilenames.size())
	{
		for (const auto &def : DefinitionFilenames)
		{
			C4Group Def;
			if (!Def.Open(def.c_str()))
			{
				LogFatal(FormatString(LoadResStr("IDS_PRC_DEFNOTFOUND"), def.c_str()).getData());
				Def.Close();
				return false;
			}
			Def.Close();
			CreateByFile(NRT_Definitions, def.c_str());
		}
	}
	// add System.c4g
	CreateByFile(NRT_System, C4CFN_System);
	// add all instances of Material.c4g, except those inside the scenario file
	C4Group *pMatParentGrp = nullptr;
	while (pMatParentGrp = Game.GroupSet.FindGroup(C4GSCnt_Material, pMatParentGrp))
		if (pMatParentGrp != &Game.ScenarioFile)
		{
			CreateByFile(NRT_Material, (pMatParentGrp->GetFullName() + DirSep C4CFN_Material).getData());
		}
	// add global Material.c4g
	CreateByFile(NRT_Material, C4CFN_Material);
	// done; success
	return true;
}

C4GameRes *C4GameResList::CreateByFile(C4Network2ResType eType, const char *szFile)
{
	// Create & set
	C4GameRes *pRes = new C4GameRes;
	pRes->SetFile(eType, szFile);
	// Add to list
	Add(pRes);
	return pRes;
}

bool C4GameResList::InitNetwork(C4Network2ResList *pNetResList)
{
	// Check all resources without attached network resource object
	for (const auto &it : resList)
	{
		if (!it->InitNetwork(pNetResList))
			return false;
	}
	// Success
	return true;
}

void C4GameResList::CalcHashes()
{
	for (const auto &it : resList)
		it->CalcHash();
}

bool C4GameResList::RetrieveFiles()
{
	// wait for all resources
	for (const auto &it : resList)
	{
		if (const C4Network2ResCore *const core{it->getResCore()}; core)
		{
			StdStrBuf ResNameBuf = FormatString("%s: %s", LoadResStr("IDS_DLG_DEFINITION"), GetFilename(core->getFileName()));
			if (!Game.Network.RetrieveRes(*core, C4NetResRetrieveTimeout, ResNameBuf.getData()))
				return false;
		}
		else
		{
			return false;
		}
	}
	return true;
}

void C4GameResList::Add(C4GameRes *pRes)
{
	resList.emplace_back(pRes);
}

void C4GameResList::CompileFunc(StdCompiler *pComp)
{
	bool fCompiler = pComp->isCompiler();
	// Clear previous data
	int resCount = resList.size();
	// Compile resource count
	pComp->Value(mkNamingCountAdapt(resCount, "Resource"));
	// Create list
	if (fCompiler)
	{
		Clear();
		resList.resize(resCount);
	}
	// Compile list
	pComp->Value(
		mkNamingAdapt(
			mkArrayAdaptS(resList.data(), resCount),
			"Resource"));
}

C4GameResList::ResTypeIterator::ResTypeIterator(C4Network2ResType type, const std::vector<std::unique_ptr<C4GameRes>> &resList) : resList{resList}, type{type}, it{resList.begin()}
{
	filter();
}

void C4GameResList::ResTypeIterator::filter()
{
	while (it != resList.end() && (*it)->getType() != type)
	{
		++it;
	}
}

C4GameResList::ResTypeIterator &C4GameResList::ResTypeIterator::operator++()
{
	++it;
	filter();
	return *this;
}

C4GameRes &C4GameResList::ResTypeIterator::operator*() const
{
	return **it;
}

C4GameRes *C4GameResList::ResTypeIterator::operator->() const
{
	return it->get();
}

bool C4GameResList::ResTypeIterator::operator==(const ResTypeIterator &other) const
{
	return type == other.type && it == other.it;
}

C4GameResList::ResTypeIterator C4GameResList::ResTypeIterator::end() const
{
	auto ret = *this;
	ret.it = resList.end();
	return ret;
}

// *** C4GameParameters

C4GameParameters::C4GameParameters() {}

C4GameParameters::~C4GameParameters() {}

void C4GameParameters::Clear()
{
	League.Clear();
	LeagueAddress.Clear();
	Rules.Clear();
	Goals.Clear();
	ScenarioTitle.Ref("No title");
	Scenario.Clear();
	GameRes.Clear();
	Clients.Clear();
	PlayerInfos.Clear();
	RestorePlayerInfos.Clear();
	Teams.Clear();
}

bool C4GameParameters::Load(C4Group &hGroup, C4Scenario *pScenario, const char *szGameText, C4LangStringTable *pLang, const std::vector<std::string> &DefinitionFilenames)
{
	// Clear previous data
	Clear();

	// Scenario
	Scenario.SetFile(NRT_Scenario, hGroup.GetFullName().getData());

	// Additional game resources
	if (!GameRes.Load(DefinitionFilenames))
		return false;

	// Player infos (replays only)
	if (pScenario->Head.Replay)
		if (hGroup.FindEntry(C4CFN_PlayerInfos))
			PlayerInfos.Load(hGroup, C4CFN_PlayerInfos);

	// Savegame restore infos: Used for savegames to rejoin joined players
	if (hGroup.FindEntry(C4CFN_SavePlayerInfos))
	{
		// load to savegame info list
		RestorePlayerInfos.Load(hGroup, C4CFN_SavePlayerInfos, pLang);
		// transfer counter to allow for additional player joins in savegame resumes
		PlayerInfos.SetIDCounter(RestorePlayerInfos.GetIDCounter());
		// in network mode, savegame players may be reassigned in the lobby
		// in any mode, the final player restoration will be done in InitPlayers()
		// dropping any players that could not be restored
	}
	else if (pScenario->Head.SaveGame)
	{
		// maybe there should be a player info file? (old-style savegame)
		if (szGameText)
		{
			// then recreate the player infos to be restored from game text
			RestorePlayerInfos.LoadFromGameText(szGameText);
			// transfer counter
			PlayerInfos.SetIDCounter(RestorePlayerInfos.GetIDCounter());
		}
	}

	// Load teams
	if (!Teams.Load(hGroup, pScenario, pLang))
	{
		LogFatal(LoadResStr("IDS_PRC_ERRORLOADINGTEAMS")); return false;
	}

	// Compile data
	StdStrBuf Buf;
	if (hGroup.LoadEntryString(C4CFN_Parameters, Buf))
	{
		if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(
			mkNamingAdapt(mkParAdapt(*this, pScenario), "Parameters"),
			Buf,
			C4CFN_Parameters))
			return false;
	}
	else
	{
		// Set default values
		StdCompilerNull DefaultCompiler;
		DefaultCompiler.Compile(mkParAdapt(*this, pScenario));

		// Set random seed
		RandomSeed = static_cast<int32_t>(time(nullptr));

		// Set control rate default
		if (ControlRate < 0)
			ControlRate = Config.Network.ControlRate;

		// network game?
		IsNetworkGame = Game.NetworkActive;

		// FairCrew-flag by command line
		if (!FairCrewForced)
			UseFairCrew = !!Config.General.FairCrew;
		if (!FairCrewStrength && UseFairCrew)
			FairCrewStrength = Config.General.FairCrewStrength;

		// Auto frame skip by options
		AutoFrameSkip = ::Config.Graphics.AutoFrameSkip;
	}

	// enforce league settings
	if (isLeague()) EnforceLeagueRules(pScenario);

	// Done
	return true;
}

void C4GameParameters::EnforceLeagueRules(C4Scenario *pScenario)
{
	Scenario.CalcHash();
	GameRes.CalcHashes();
	Teams.EnforceLeagueRules();
	AllowDebug = false;
	// Fair crew enabled in league, if not explicitely disabled by scenario
	// Fair crew strengt to a moderately high value
	if (!Game.Parameters.FairCrewForced)
	{
		Game.Parameters.UseFairCrew = true;
		Game.Parameters.FairCrewForced = true;
		Game.Parameters.FairCrewStrength = 20000;
	}
	if (pScenario) MaxPlayers = pScenario->Head.MaxPlayerLeague;
}

bool C4GameParameters::CheckLeagueRulesStart(bool fFixIt)
{
	// Additional checks for start parameters that are illegal in league games.

	if (!isLeague()) return true;

	bool fError = false;
	StdStrBuf Error;

	// league games: enforce one team per client
	C4ClientPlayerInfos *pClient; C4PlayerInfo *pInfo;
	for (int iClient = 0; pClient = Game.PlayerInfos.GetIndexedInfo(iClient); iClient++)
	{
		bool fHaveTeam = false; int32_t iClientTeam; const char *szFirstPlayer;
		for (int iInfo = 0; pInfo = pClient->GetPlayerInfo(iInfo); iInfo++)
		{
			// Actual human players only
			if (pInfo->GetType() != C4PT_User) continue;

			int32_t iTeam = pInfo->GetTeam();
			if (!fHaveTeam)
			{
				iClientTeam = iTeam;
				szFirstPlayer = pInfo->GetName();
				fHaveTeam = true;
			}
			else if ((!Teams.IsCustom() && Game.C4S.Game.IsMelee()) || iTeam != iClientTeam)
			{
				Error.Format(LoadResStr("IDS_MSG_NOSPLITSCREENINLEAGUE"), szFirstPlayer, pInfo->GetName());
				if (!fFixIt)
				{
					fError = true;
				}
				else
				{
					C4Client *pClient2 = Game.Clients.getClientByID(pClient->GetClientID());
					if (!pClient2 || pClient2->isHost())
						fError = true;
					else
						Game.Clients.CtrlRemove(pClient2, Error.getData());
				}
			}
		}
	}

	// Error?
	if (fError)
	{
		if (Game.pGUI)
			Game.pGUI->ShowMessageModal(Error.getData(), LoadResStr("IDS_NET_ERR_LEAGUE"), C4GUI::MessageDialog::btnOK, C4GUI::Ico_MeleeLeague);
		else
			Log(Error.getData());
		return false;
	}
	// All okay
	return true;
}

bool C4GameParameters::Save(C4Group &hGroup, C4Scenario *pScenario)
{
	// Write Parameters.txt
	StdStrBuf ParData = DecompileToBuf<StdCompilerINIWrite>(
		mkNamingAdapt(mkParAdapt(*this, pScenario), "Parameters"));
	if (!hGroup.Add(C4CFN_Parameters, ParData, false, true))
		return false;

	// Done
	return true;
}

bool C4GameParameters::InitNetwork(C4Network2ResList *pResList)
{
	// Scenario & material resource
	if (!Scenario.InitNetwork(pResList))
		return false;

	// Other game resources
	if (!GameRes.InitNetwork(pResList))
		return false;

	// Done
	return true;
}

void C4GameParameters::CompileFunc(StdCompiler *pComp, C4Scenario *pScenario)
{
	pComp->Value(mkNamingAdapt(RandomSeed,         "RandomSeed",         !pScenario ? 0 : pScenario->Head.RandomSeed));
	pComp->Value(mkNamingAdapt(StartupPlayerCount, "StartupPlayerCount", 0));
	pComp->Value(mkNamingAdapt(MaxPlayers,         "MaxPlayers",         !pScenario ? 0 : pScenario->Head.MaxPlayer));
	pComp->Value(mkNamingAdapt(UseFairCrew,        "UseFairCrew",        !pScenario ? false : (pScenario->Head.ForcedFairCrew == C4SFairCrew_FairCrew)));
	pComp->Value(mkNamingAdapt(FairCrewForced,     "FairCrewForced",     !pScenario ? false : (pScenario->Head.ForcedFairCrew != C4SFairCrew_Free)));
	pComp->Value(mkNamingAdapt(FairCrewStrength,   "FairCrewStrength",   !pScenario ? 0 : pScenario->Head.FairCrewStrength));
	pComp->Value(mkNamingAdapt(AllowDebug,         "AllowDebug",         true));
	pComp->Value(mkNamingAdapt(IsNetworkGame,      "IsNetworkGame",      false));
	pComp->Value(mkNamingAdapt(ControlRate,        "ControlRate",        -1));
	pComp->Value(mkNamingAdapt(AutoFrameSkip,      "AutoFrameSkip",      false));
	pComp->Value(mkNamingAdapt(Rules,              "Rules",              !pScenario ? C4IDList() : pScenario->Game.Rules));
	pComp->Value(mkNamingAdapt(Goals,              "Goals",              !pScenario ? C4IDList() : pScenario->Game.Goals));
	pComp->Value(mkNamingAdapt(League,             "League",             StdStrBuf()));

	// These values are either stored separately (see Load/Save) or
	// don't make sense for savegames.
	if (!pScenario)
	{
		pComp->Value(mkNamingAdapt(LeagueAddress, "LeagueAddress", ""));

		pComp->Value(mkNamingAdapt(ScenarioTitle, "Title", "No title"));
		pComp->Value(mkNamingAdapt(Scenario, "Scenario"));
		pComp->Value(GameRes);

		pComp->Value(mkNamingAdapt(PlayerInfos,        "PlayerInfos"));
		pComp->Value(mkNamingAdapt(RestorePlayerInfos, "RestorePlayerInfos"));
		pComp->Value(mkNamingAdapt(Teams,              "Teams"));
	}

	pComp->Value(Clients);
}

StdStrBuf C4GameParameters::GetGameGoalString()
{
	// getting game goals from the ID list
	// unfortunately, names cannot be deduced before object definitions are loaded
	StdStrBuf sResult;
	C4ID idGoal;
	for (int32_t i = 0; i < Goals.GetNumberOfIDs(); ++i)
		if (idGoal = Goals.GetID(i)) if (idGoal != C4ID_None)
		{
			if (Game.IsRunning)
			{
				C4Def *pDef = C4Id2Def(idGoal);
				if (pDef)
				{
					if (sResult.getLength()) sResult.Append(", ");
					sResult.Append(pDef->GetName());
				}
			}
			else
			{
				if (sResult.getLength()) sResult.Append(", ");
				sResult.Append(C4IdText(idGoal));
			}
		}
	// Max length safety
	if (sResult.getLength() > C4MaxTitle) sResult.SetLength(C4MaxTitle);
	// Compose desc string
	if (sResult.getLength())
		return FormatString("%s: %s", LoadResStr("IDS_MENU_CPGOALS"), sResult.getData());
	else
		return StdStrBuf(LoadResStr("IDS_CTL_NOGOAL"));
}
