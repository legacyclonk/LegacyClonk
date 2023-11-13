/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2004, Sven2
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

// game saving functionality

#include <C4GameSave.h>

#include <C4Components.h>
#include <C4Game.h>
#include "C4Version.h"
#include "StdMarkup.h"
#include <C4Console.h>
#include <C4Log.h>
#include <C4Player.h>
#include <C4RTF.h>

#include <format>
#include <utility>

// *** C4GameSave main class

bool C4GameSave::SaveCreateGroup(const char *szFilename, C4Group &hUseGroup)
{
	// erase any previous item (2do: work in C4Groups?)
	EraseItem(szFilename);
	// copy from previous group?
	if (GetCopyScenario())
		if (!ItemIdentical(Game.ScenarioFilename, szFilename))
			if (!C4Group_CopyItem(Game.ScenarioFilename, szFilename))
			{
				Log(C4ResStrTableKey::IDS_CNS_SAVEASERROR, szFilename); return false;
			}
	// open it
	if (!hUseGroup.Open(szFilename, !GetCopyScenario()))
	{
		EraseItem(szFilename);
		Log(C4ResStrTableKey::IDS_CNS_SAVEASERROR, szFilename);
		return false;
	}
	// done, success
	return true;
}

bool C4GameSave::SaveCore()
{
	// base on original, current core
	rC4S = Game.C4S;
	// Always mark current engine version
	rC4S.Head.C4XVer[0] = C4XVER1; rC4S.Head.C4XVer[1] = C4XVER2;
	rC4S.Head.C4XVer[2] = C4XVER3; rC4S.Head.C4XVer[3] = C4XVER4;
	// Some flags are not to be set for initial settings:
	//  They depend on whether specific runtime data is present, which may simply not be stored into initial
	//  saves, because they rely on any data present and up-to-date within the scenario!
	if (!fInitial)
	{
		// NoInitialize: Marks whether object data is contained and not to be created from core
		rC4S.Head.NoInitialize = true;
		// the SaveGame-value, despite it's name, marks whether exact runtime data is contained
		// the flag must not be altered for pure
		rC4S.Head.SaveGame = GetSaveRuntimeData() && IsExact();
	}
	// reset some network flags
	rC4S.Head.NetworkGame = 0;
	// Title in language game was started in (not: save scenarios and net references)
	if (!GetKeepTitle()) SCopy(Game.Parameters.ScenarioTitle.getData(), rC4S.Head.Title, C4MaxTitle);
	// some adjustments for everything but saved scenarios
	if (IsExact())
	{
		// Store used definitions
		rC4S.Definitions.SetModules(Game.DefinitionFilenames, Config.General.ExePath, Config.General.DefinitionPath);
		// Save game parameters
		if (!Game.Parameters.Save(*pSaveGroup, &Game.C4S)) return false;
	}
	// clear MissionAccess in save games and records (sulai)
	*rC4S.Head.MissionAccess = 0;
	// OldGfx is no longer supported
	// checks for IsExact() || ExactLandscape wouldn't catch scenarios using more than 23 materials, so let's make it easy
	rC4S.Head.ForcedGfxMode = C4SGFXMODE_NEWGFX;
	// store origin
	if (GetSaveOrigin())
	{
		// keep if assigned already (e.g., when doing a record of a savegame)
		if (!rC4S.Head.Origin.getLength())
		{
			rC4S.Head.Origin.Copy(Game.ScenarioFilename);
			Config.ForceRelativePath(&rC4S.Head.Origin);
		}
	}
	else if (GetClearOrigin())
		rC4S.Head.Origin.Clear();
	// adjust specific values (virtual call)
	AdjustCore(rC4S);
	// Save scenario core
	return !!rC4S.Save(*pSaveGroup);
}

bool C4GameSave::SaveScenarioSections()
{
	// any scenario sections?
	if (!Game.pScenarioSections) return true;
	// prepare section filename
	int iWildcardPos = SCharPos('*', C4CFN_ScenarioSections);
	char fn[_MAX_FNAME + 1];
	// save all modified sections
	for (C4ScenarioSection *pSect = Game.pScenarioSections; pSect; pSect = pSect->pNext)
	{
		// compose section filename
		SCopy(C4CFN_ScenarioSections, fn);
		SDelete(fn, 1, iWildcardPos); SInsert(fn, pSect->GetName(), iWildcardPos);
		// do not save self, because that is implied in CurrentScenarioSection and the main landscape/object data
		if (pSect == Game.pCurrentScenarioSection)
			pSaveGroup->DeleteEntry(fn);
		else if (pSect->fModified)
		{
			// modified section: delete current
			pSaveGroup->DeleteEntry(fn);
			// replace by new
			pSaveGroup->Add(pSect->GetTempFilename(), fn);
		}
	}
	// done, success
	return true;
}

bool C4GameSave::SaveLandscape(C4Section &section)
{
	// exact?
	if (section.Landscape.Mode == C4LSC_Exact || GetForceExactLandscape())
	{
		C4DebugRecOff DBGRECOFF;
		// Landscape
		section.Objects.RemoveSolidMasks();
		bool fSuccess;
		if (section.Landscape.Mode == C4LSC_Exact)
			fSuccess = !!section.Landscape.Save(*pSaveGroup);
		else
			fSuccess = !!section.Landscape.SaveDiff(*pSaveGroup, !IsSynced());
		section.Objects.PutSolidMasks();
		if (!fSuccess) return false;
		DBGRECOFF.Clear();
		// PXS
		if (!section.PXS.Save(*pSaveGroup)) return false;
		// MassMover (create copy, may not modify running data)
		C4MassMoverSet MassMoverSet{section};
		MassMoverSet.Copy(section.MassMover);
		if (!MassMoverSet.Save(*pSaveGroup)) return false;
		// Material enumeration
		if (!section.Material.SaveEnumeration(*pSaveGroup)) return false;
	}
	// static / dynamic
	if (section.Landscape.Mode == C4LSC_Static)
	{
		// static map
		// remove old-style landscape.bmp
		pSaveGroup->DeleteEntry(C4CFN_Landscape);
		// save materials if not already done
		if (!GetForceExactLandscape())
		{
			// save map
			if (!section.Landscape.SaveMap(*pSaveGroup)) return false;
			// save textures (if changed)
			if (!section.Landscape.SaveTextures(*pSaveGroup)) return false;
		}
	}
	else if (section.Landscape.Mode != C4LSC_Exact)
	{
		// dynamic map by landscape.txt or scenario core: nothing to save
		// in fact, it doesn't even make much sense to save the Objects.txt
		// but the user pressed save after all...
	}
	return true;
}

bool C4GameSave::SaveRuntimeData()
{
	// scenario sections (exact only)
	if (IsExact()) if (!SaveScenarioSections())
	{
		Log(C4ResStrTableKey::IDS_ERR_SAVE_SCENSECTIONS); return false;
	}

	for (const auto &section : Game.Sections)
	{
		if (!SaveSection(*section))
		{
			return false;
		}
	}

	// Strings
	Game.ScriptEngine.Strings.EnumStrings();
	if (!Game.ScriptEngine.Strings.Save((*pSaveGroup)))
	{
		Log(C4ResStrTableKey::IDS_ERR_SAVE_SCRIPTSTRINGS); return false;
	}
	// Round results
	if (GetSaveUserPlayers()) if (!Game.RoundResults.Save(*pSaveGroup))
	{
		Log(C4ResStrTableKey::IDS_ERR_ERRORSAVINGROUNDRESULTS); return false;
	}
	// Teams
	if (!Game.Teams.Save(*pSaveGroup))
	{
		Log(C4ResStrTableKey::IDS_ERR_ERRORSAVINGTEAMS); return false;
	}
	// some scenario components possiby modified in console mode
	// such modifications cannot possibly be done before game start
	// so it's runtime data
	// Script
	if (!Game.Script.Save((*pSaveGroup))) Log(C4ResStrTableKey::IDS_ERR_SAVE_SCRIPT); /* nofail */
	// Title - unexact only, because in savegames, the title will be set in core
	if (!IsExact()) if (!Game.Title.Save((*pSaveGroup))) Log(C4ResStrTableKey::IDS_ERR_SAVE_TITLE); /* nofail */
	// Info
	if (!Game.Info.Save((*pSaveGroup))) Log(C4ResStrTableKey::IDS_ERR_SAVE_INFO); /* nofail */
	if (GetSaveUserPlayers() || GetSaveScriptPlayers())
	{
		// player infos
		// the stored player info filenames will point into the scenario file, and no ressource information
		// will be saved. PlayerInfo must be saved first, because those will generate the storage filenames to be used by
		// C4PlayerList
		C4PlayerInfoList RestoreInfos;
		RestoreInfos.SetAsRestoreInfos(Game.PlayerInfos, GetSaveUserPlayers(), GetSaveScriptPlayers(), GetSaveUserPlayerFiles(), GetSaveScriptPlayerFiles());
		if (!RestoreInfos.Save(*pSaveGroup, C4CFN_SavePlayerInfos))
		{
			Log(C4ResStrTableKey::IDS_ERR_SAVE_RESTOREPLAYERINFOS); return false;
		}
		// Players
		// this will save the player files to the savegame scenario group only
		// synchronization to the original player files will be done in global game
		// synchronization (via control queue)
		if (GetSaveUserPlayerFiles() || GetSaveScriptPlayerFiles())
		{
			if (!Game.Players.Save((*pSaveGroup), GetCreateSmallFile(), RestoreInfos))
			{
				Log(C4ResStrTableKey::IDS_ERR_SAVE_PLAYERS); return false;
			}
		}
	}
	else
	{
		// non-exact runtime data: remove any exact files
		// No Game.txt
		pSaveGroup->Delete(C4CFN_Game);
		// No player files
		pSaveGroup->Delete(C4CFN_PlayerInfos);
		pSaveGroup->Delete(C4CFN_SavePlayerInfos);
	}
	// done, success
	return true;
}

bool C4GameSave::SaveSection(C4Section &section)
{
	// landscape
	if (!SaveLandscape(section))
	{
		Log(C4ResStrTableKey::IDS_ERR_SAVE_LANDSCAPE);
		return false;
	}

	// Objects
	if (!section.Objects.Save(section, (*pSaveGroup), IsExact(), true))
	{
		Log(C4ResStrTableKey::IDS_ERR_SAVE_OBJECTS);
		return false;
	}

	return true;
}

bool C4GameSave::SaveDesc(C4Group &hToGroup)
{
	// Scenario title
	StdStrBuf title{Game.Parameters.ScenarioTitle};
	CMarkup::StripMarkup(&title);

	// Unfortunately, there's no way to prealloc the buffer in an appropriate size
	std::string desc{std::format(
					"{{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang1031{{\\fonttbl {{\\f0\\fnil\\fcharset{} Times New Roman;}}}}" LineFeed
					"\\uc1\\pard\\ulnone\\b\\f0\\fs20 {}\\par" LineFeed "\\b0\\fs16\\par" LineFeed,
					C4Config::GetCharsetCode(Config.General.LanguageCharset),
					RtfEscape(title.getData())
					)};

	// OK; each specializations has its own desc format
	WriteDesc(desc);

	// End of file
	desc.append(LineFeed "}" LineFeed EndOfFile);

	// Generate Filename
	char szLang[3];
	SCopyUntil(Config.General.Language, szLang, ',', 2);

	// Save to file
	StdStrBuf buf{desc.c_str(), desc.size()};
	return !!hToGroup.Add(std::vformat(C4CFN_ScenarioDesc, std::make_format_args(szLang)).c_str(), buf, false, true);
}

void C4GameSave::WriteDescLineFeed(std::string &desc)
{
	// paragraph end + cosmetics
	desc.append("\\par" LineFeed);
}

void C4GameSave::WriteDescDate(std::string &desc, bool fRecord)
{
	// write local time/date
	time_t tTime; time(&tTime);
	struct tm *pLocalTime;
	pLocalTime = localtime(&tTime);

	std::string msg;
	if (fRecord)
	{
		msg = LoadResStr(C4ResStrTableKey::IDS_DESC_DATEREC, pLocalTime->tm_mday, pLocalTime->tm_mon + 1, pLocalTime->tm_year + 1900, pLocalTime->tm_hour, pLocalTime->tm_min);
	}
	else
	{
		msg = LoadResStrChoice(Game.Network.isEnabled(), C4ResStrTableKey::IDS_DESC_DATENET, C4ResStrTableKey::IDS_DESC_DATE, pLocalTime->tm_mday, pLocalTime->tm_mon + 1, pLocalTime->tm_year + 1900, pLocalTime->tm_hour, pLocalTime->tm_min);
	}

	desc.append(RtfEscape(msg));
	WriteDescLineFeed(desc);
}

void C4GameSave::WriteDescGameTime(std::string &desc)
{
	// Write game duration
	if (Game.Time)
	{
		desc.append(RtfEscape(LoadResStr(C4ResStrTableKey::IDS_DESC_DURATION,
			Game.Time / 3600, (Game.Time % 3600) / 60, Game.Time % 60)));
		WriteDescLineFeed(desc);
	}
}

void C4GameSave::WriteDescEngine(std::string &desc)
{
	desc.append(RtfEscape(LoadResStr(C4ResStrTableKey::IDS_DESC_VERSION, std::format("{:03}", C4XVERBUILD))));
	WriteDescLineFeed(desc);
}

void C4GameSave::WriteDescLeague(std::string &desc, bool fLeague, const char *strLeagueName)
{
	if (fLeague)
	{
		desc.append(RtfEscape(LoadResStr(C4ResStrTableKey::IDS_PRC_LEAGUE, strLeagueName)));
		WriteDescLineFeed(desc);
	}
}

void C4GameSave::WriteDescDefinitions(std::string &desc)
{
	// Definition specs
	if (Game.DefinitionFilenames.size())
	{
		// Desc
		desc.append(RtfEscape(LoadResStr(C4ResStrTableKey::IDS_DESC_DEFSPECS)));
		// Get definition modules
		int32_t i = 0;
		for (const auto &def : Game.DefinitionFilenames)
		{
			// Get exe relative path
			StdStrBuf sDefFilename;
			sDefFilename.Copy(Config.AtExeRelativePath(def.c_str()));
			// Convert rtf backslashes
			sDefFilename.Replace("\\", "\\\\");
			// Append comma
			if (i++ > 0) desc.append(", ");
			// Apend to desc
			desc.append(RtfEscape(sDefFilename.getData()));
		}
		// End of line
		WriteDescLineFeed(desc);
	}
}

void C4GameSave::WriteDescNetworkClients(std::string &desc)
{
	// Desc
	desc.append(RtfEscape(LoadResStr(C4ResStrTableKey::IDS_DESC_CLIENTS)).c_str());
	// Client names
	bool comma{false};
	for (C4Network2Client *pClient = Game.Network.Clients.GetNextClient(nullptr); pClient; pClient = Game.Network.Clients.GetNextClient(pClient))
	{
		if (comma)
		{
			desc.append(", ");
		}

		desc.append(RtfEscape(pClient->getName()));
		comma = true;
	}
	// End of line
	WriteDescLineFeed(desc);
}

void C4GameSave::WriteDescPlayers(std::string &desc, bool fByTeam, int32_t idTeam)
{
	// write out all players; only if they match the given team if specified
	C4PlayerInfo *pPlr; bool fAnyPlrWritten = false;
	for (int i = 0; pPlr = Game.PlayerInfos.GetPlayerInfoByIndex(i); i++)
		if (pPlr->HasJoined() && !pPlr->IsRemoved() && !pPlr->IsInvisible())
		{
			if (fByTeam)
				if (idTeam)
				{
					// match team
					if (pPlr->GetTeam() != idTeam) continue;
				}
				else
				{
					// must be in no known team
					if (Game.Teams.GetTeamByID(pPlr->GetTeam())) continue;
				}
			if (fAnyPlrWritten)
				desc.append(", ");
			else if (fByTeam && idTeam)
			{
				C4Team *pTeam = Game.Teams.GetTeamByID(idTeam);
				if (pTeam) desc.append(RtfEscape(std::format("{}: ", pTeam->GetName())));
			}
			desc.append(RtfEscape(pPlr->GetName()));
			fAnyPlrWritten = true;
		}
	if (fAnyPlrWritten) WriteDescLineFeed(desc);
}

void C4GameSave::WriteDescPlayers(std::string &desc)
{
	// New style using Game.PlayerInfos
	if (Game.PlayerInfos.GetPlayerCount())
	{
		desc.append(RtfEscape(LoadResStr(C4ResStrTableKey::IDS_DESC_PLRS)));
		if (Game.Teams.IsMultiTeams() && !Game.Teams.IsAutoGenerateTeams())
		{
			// Teams defined: Print players sorted by teams
			WriteDescLineFeed(desc);
			C4Team *pTeam; int32_t i = 0;
			while (pTeam = Game.Teams.GetTeamByIndex(i++))
			{
				WriteDescPlayers(desc, true, pTeam->GetID());
			}
			// Finally, print out players outside known teams (those can only be achieved by script using SetPlayerTeam)
			WriteDescPlayers(desc, true, 0);
		}
		else
		{
			// No teams defined: Print all players that have ever joined
			WriteDescPlayers(desc, false, 0);
		}
	}
}

bool C4GameSave::Save(const char *szFilename)
{
	// close any previous
	Close();
	// create group
	C4Group *pLSaveGroup = new C4Group();
	if (!SaveCreateGroup(szFilename, *pLSaveGroup))
	{
		Log(C4ResStrTableKey::IDS_ERR_SAVE_TARGETGRP, szFilename ? szFilename : "nullptr!");
		delete pLSaveGroup;
		return false;
	}
	// save to it
	return Save(*pLSaveGroup, true);
}

bool C4GameSave::Save(C4Group &hToGroup, bool fKeepGroup)
{
	// close any previous
	Close();
	// set group
	pSaveGroup = &hToGroup; fOwnGroup = fKeepGroup;
	// PreSave-actions (virtual call)
	if (!OnSaving()) return false;
	// always save core
	if (!SaveCore()) { Log(C4ResStrTableKey::IDS_ERR_SAVE_CORE); return false; }
	// cleanup group
	pSaveGroup->Delete(C4CFN_PlayerFiles);
	// remove: Title text, image and icon if specified
	if (!GetKeepTitle())
	{
		pSaveGroup->Delete(C4CFN_ScenarioTitle);
		pSaveGroup->Delete(C4CFN_ScenarioIcon);
		pSaveGroup->Delete(std::vformat(C4CFN_ScenarioDesc, std::make_format_args("*")).c_str());
		pSaveGroup->Delete(C4CFN_Titles);
		pSaveGroup->Delete(C4CFN_Info);
	}
	// Always save Game.txt; even for saved scenarios, because global effects need to be saved
	if (!Game.SaveData(*pSaveGroup, false, fInitial, IsExact()))
	{
		Log(C4ResStrTableKey::IDS_ERR_SAVE_RUNTIMEDATA); return false;
	}
	// save additional runtime data
	if (GetSaveRuntimeData()) if (!SaveRuntimeData()) return false;
	// Desc
	if (GetSaveDesc())
		if (!SaveDesc(*pSaveGroup))
			Log(C4ResStrTableKey::IDS_ERR_SAVE_DESC); /* nofail */
	// save specialized components (virtual call)
	if (!SaveComponents()) return false;
	// done, success
	return true;
}

bool C4GameSave::Close()
{
	bool fSuccess = true;
	// any group open?
	if (pSaveGroup)
	{
		// sort group
		const char *szSortOrder = GetSortOrder();
		if (szSortOrder) pSaveGroup->Sort(szSortOrder);
		// close if owned group
		if (fOwnGroup)
		{
			fSuccess = !!pSaveGroup->Close();
			delete pSaveGroup;
			fOwnGroup = false;
		}
		pSaveGroup = nullptr;
	}
	return fSuccess;
}

// *** C4GameSaveSavegame

bool C4GameSaveSavegame::OnSaving()
{
	if (!Game.IsRunning) return true;
	// synchronization to sync player files on all clients
	// this resets playing times and stores them in the players?
	// but doing so would be too late when the queue is executed!
	// TODO: remove it? (-> PeterW ;))
	if (Game.Network.isEnabled())
		Game.Input.Add(CID_Synchronize, new C4ControlSynchronize(true));
	else
		Game.Players.SynchronizeLocalFiles();
	// OK; save now
	return true;
}

void C4GameSaveSavegame::AdjustCore(C4Scenario &rC4S)
{
	// Determine save game index from trailing number in group file name
	int iSaveGameIndex = GetTrailingNumber(GetFilenameOnly(pSaveGroup->GetFullName().getData()));
	// Looks like a decent index: set numbered icon
	if (Inside(iSaveGameIndex, 1, 10))
		rC4S.Head.Icon = 2 + (iSaveGameIndex - 1);
	// Else: set normal script icon
	else
		rC4S.Head.Icon = 29;
}

bool C4GameSaveSavegame::SaveComponents()
{
	// special for savegames: save a screenshot
	if (!Game.SaveGameTitle((*pSaveGroup)))
		Log(C4ResStrTableKey::IDS_ERR_SAVE_GAMETITLE); /* nofail */
	// done, success
	return true;
}

bool C4GameSaveSavegame::WriteDesc(std::string &desc)
{
	// compose savegame desc
	WriteDescDate(desc);
	WriteDescGameTime(desc);
	WriteDescDefinitions(desc);
	if (Game.Network.isEnabled()) WriteDescNetworkClients(desc);
	WriteDescPlayers(desc);
	// done, success
	return true;
}

// *** C4GameSaveRecord

void C4GameSaveRecord::AdjustCore(C4Scenario &rC4S)
{
	// specific recording flags
	rC4S.Head.Replay = true;
	rC4S.Head.Icon = 29;
	// default record title
	std::array<char, C4MaxTitle + 1> buf;
	FormatWithNull(buf, "{:03} {} [{}]", iNum, Game.Parameters.ScenarioTitle.getData(), C4XVERBUILD);
	SCopy(buf.data(), rC4S.Head.Title, C4MaxTitle);
}

bool C4GameSaveRecord::SaveComponents()
{
	// special: records need player infos even if done initially
	if (fInitial) Game.PlayerInfos.Save((*pSaveGroup), C4CFN_PlayerInfos);
	// for !fInitial, player infos will be saved as regular runtime data
	// done, success
	return true;
}

bool C4GameSaveRecord::WriteDesc(std::string &desc)
{
	// compose record desc
	WriteDescDate(desc, true);
	WriteDescGameTime(desc);
	WriteDescEngine(desc);
	WriteDescDefinitions(desc);
	WriteDescLeague(desc, fLeague, Game.Parameters.League.getData());
	if (Game.Network.isEnabled()) WriteDescNetworkClients(desc);
	WriteDescPlayers(desc);
	// done, success
	return true;
}

// *** C4GameSaveNetwork

void C4GameSaveNetwork::AdjustCore(C4Scenario &rC4S)
{
	// specific dynamic flags
	rC4S.Head.NetworkGame = true;
	rC4S.Head.NetworkRuntimeJoin = !fInitial;
}
