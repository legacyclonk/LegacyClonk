/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2005, Sven2
 * Copyright (c) 2017-2021, The LegacyClonk Team and contributors
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

// player team management for teamwork melees

#pragma once

#include "C4Constants.h"
#include "C4EnumInfo.h"
#include "C4ForwardDeclarations.h"
#include "StdBuf.h"

#include <cstdint>

class C4TeamList;

// class predec
namespace C4GUI { class ComboBox_FillCB; }

// constant used by lobby to indicate invisible, random team
const int32_t TEAMID_Unknown = -1;

// constant used by InitScenarioPlayer() to indicate creation of a new team
const int32_t TEAMID_New = -1;

// one player team
class C4Team
{
private:
	// std::vector...
	// containing player info IDs
	int32_t *piPlayers;
	int32_t iPlayerCount;
	int32_t iPlayerCapacity;

public:
	// copying
	C4Team(const C4Team &rCopy);
	C4Team &operator=(const C4Team &rCopy);

protected:
	// team identification; usually > 0 for a valid team
	int32_t iID;
	char Name[C4MaxName + 1];
	int32_t iPlrStartIndex; // 0 for unassigned; 1 to 4 if all players of that team shall be assigned a specific [Player*]-section in the Scenario.txt
	uint32_t dwClr; // team color
	StdStrBuf sIconSpec; // icon drawing specification for offline or runtime team selection dialog
	int32_t iMaxPlayer; // maximum number of players allowed in this team - 0 for infinite

	friend class C4TeamList;

public:
	C4Team() : piPlayers(nullptr), iPlayerCount(0), iPlayerCapacity(0), iID(0), iPlrStartIndex(0), dwClr(0), iMaxPlayer(0) { *Name = 0; }
	~C4Team() { Clear(); }

	void Clear();
	void AddPlayer(class C4PlayerInfo &rInfo, bool fAdjustPlayer); // add player by info; adjusts ID in info and at any joined player
	void RemoveIndexedPlayer(int32_t iIndex); // remove info at index; this changes the local list only
	void RemovePlayerByID(int32_t iID);

	int32_t GetPlayerCount() const { return iPlayerCount; }
	const char *GetName() const { return Name; }
	int32_t GetID() const { return iID; }
	bool IsPlayerIDInTeam(int32_t iID); // search list for a player with the given ID
	int32_t GetFirstUnjoinedPlayerID() const; // search for a player that does not have the join-flag set
	int32_t GetFirstActivePlayerID() const; // search for a player that is still in the game
	int32_t GetPlrStartIndex() const { return iPlrStartIndex; }
	uint32_t GetColor() const { return dwClr; }
	const char *GetIconSpec() const { return sIconSpec.getData(); }
	bool IsFull() const { return iMaxPlayer && (iPlayerCount >= iMaxPlayer); } // whether no more players may join this team
	StdStrBuf GetNameWithParticipants() const; // compose team name like "Team 1 (boni, GhostBear, Clonko)"
	bool HasWon() const; // return true if any member player of the team has won

	int32_t GetIndexedPlayer(int32_t iIndex) const { return Inside<int32_t>(iIndex, 0, iPlayerCount - 1) ? piPlayers[iIndex] : 0; }

	void CompileFunc(StdCompiler *pComp);

	// this rechecks teams for all (not removed) players; sets players here by team selection in player infos
	void RecheckPlayers();

	// this assigns a team color if it's still zero
	void RecheckColor(C4TeamList &rForList);
};

// global team list
class C4TeamList
{
public:
	// team config constants
	enum ConfigValue
	{
		TEAM_None = 0,
		TEAM_Custom = 1,
		TEAM_Active = 2,
		TEAM_AllowHostilityChange = 3,
		TEAM_Dist = 4,
		TEAM_AllowTeamSwitch = 5,
		TEAM_AutoGenerateTeams = 6,
		TEAM_TeamColors = 7,
	};

	// team distribution configuration
	enum TeamDist : std::uint8_t
	{
		TEAMDIST_First = 0,
		TEAMDIST_Free = 0, // anyone can choose teams
		TEAMDIST_Host = 1, // host decides teams
		TEAMDIST_None = 2, // no teams
		TEAMDIST_Random = 3, // fixed random teams
		TEAMDIST_RandomInv = 4, // fixed random teams invisible in lobby
		TEAMDIST_Last = 4,
	};

private:
	// std::vector...
	C4Team **ppList;
	int32_t iTeamCount;
	int32_t iTeamCapacity;

	int32_t iLastTeamID;
	bool fAllowHostilityChange; // hostility not fixed
	bool fAllowTeamSwitch; // teams not fixed
	bool fActive;
	bool fCustom; // set if read from team file or changed in scenario
	bool fTeamColors; // if set, player colors are determined by team colors
	bool fAutoGenerateTeams; // teams are generated automatically so there's enough teams for everyone
	TeamDist eTeamDist;
	int32_t iMaxScriptPlayers; // maximum number of script players to be added in the lobby
	StdStrBuf sScriptPlayerNames; // default script player names
	int32_t randomTeamCount; // count of teams used when generating random teams; unlimited if less than 2

public:
	C4TeamList() : ppList(nullptr), iTeamCount(0), iTeamCapacity(0), iLastTeamID(0), fAllowHostilityChange(true), fAllowTeamSwitch(false),
		fActive(true), fCustom(false), eTeamDist(TEAMDIST_Free), fTeamColors(false), fAutoGenerateTeams(false), iMaxScriptPlayers(0) {}
	~C4TeamList() { Clear(); }
	void Clear();

	C4TeamList &operator=(const C4TeamList &rCopy);

private:
	// no copying
	C4TeamList(const C4TeamList &rCopy);

private:
	void AddTeam(C4Team *pNewTeam); // add a team; grow list if necessary
	void ClearTeams(); // delete all teams
	bool GenerateDefaultTeams(int32_t iUpToID); // generate Team 1, Team 2, etc.
	int32_t GetGenerateTeamCount() const;

public:
	C4Team *GetTeamByID(int32_t iID) const; // get team by ID
	C4Team *GetGenerateTeamByID(int32_t iID); // get team by ID; generate if not existent. Always generate for TEAMID_New.
	C4Team *GetTeamByIndex(int32_t iIndex) const; // get team by list index, to enumerate all teams
	C4Team *GetTeamByPlayerID(int32_t iID) const; // get team by player ID (not number!)
	int32_t GetLargestTeamID() const;
	C4Team *GetRandomSmallestTeam(bool limitRandomTeamCount = false) const; // get team with least amount of players in it
	int32_t GetTeamCount() const { return iTeamCount; }
	int32_t GetRandomTeamCount() const { return randomTeamCount; }

	C4Team *CreateTeam(const char *szName); // create a custom team

	bool IsMultiTeams() const { return fActive; } // teams can be selected
	bool IsCustom() const { return fCustom; } // whether teams are not generated as default
	bool IsHostilityChangeAllowed() const { return fAllowHostilityChange; } // allow (temporary) hostility changes at runtime
	bool IsTeamSwitchAllowed() const { return fAllowTeamSwitch; } // allow permanent team changes at runtime
	bool CanLocalChooseTeam() const; // whether the local host can determine teams (e.g., not if teams are random, or if all except the player's current team are full)
	bool CanLocalChooseTeam(int32_t idPlayer) const; // whether the local host can determine teams (e.g., not if teams are random, or if all except the player's current team are full)
	bool CanLocalSeeTeam() const;
	bool IsTeamColors() const { return fTeamColors; } // whether team colors are enabled
	bool IsRandomTeam() const { return eTeamDist == TEAMDIST_Random || eTeamDist == TEAMDIST_RandomInv; } // whether a random team mode is selected
	bool IsJoin2TeamAllowed(int32_t idTeam); // checks whether a team ID is valid and still available for new joins
	bool IsAutoGenerateTeams() const { return fAutoGenerateTeams; }
	bool IsRuntimeJoinTeamChoice() const { return IsCustom() && IsMultiTeams(); } // whether players joining at runtime must select a team first
	int32_t GetMaxScriptPlayers() const { return iMaxScriptPlayers; } // return max number of script players to be added inthe lobby
	int32_t GetForcedTeamSelection(int32_t idForPlayer) const; // if there's only one team for the player to join, return that team ID
	StdStrBuf GetScriptPlayerName() const; // get a name to assign to a new script player. Try to avoid name conflicts
	bool IsTeamVisible() const; // teams invisible during lobby time if TEAMDIST_RandomInv

	void EnforceLeagueRules(); // enforce some league settings, such as disallowing runtime team change

	// assign a team ID to player info; recheck if any user-set team infos are valid within the current team options
	// creates a new team for melee; assigns the least-used team for teamwork melee
	// host/single-call only; using SafeRandom!
	bool RecheckPlayerInfoTeams(C4PlayerInfo &rNewJoin, bool fByHost);

	// compiler
	void CompileFunc(StdCompiler *pComp);
	bool Load(C4Group &hGroup, class C4Scenario *pInitDefault, class C4LangStringTable *pLang); // clear self and load from group file (C4CFN_Teams); init default by scen if no team fiel is present
	bool Save(C4Group &hGroup); // save to group file (C4CFN_Teams)

	// this rechecks teams for all (not removed) players; sets players here by team selection in player infos
	void RecheckPlayers();

	// this reorders any unjoined players to teams if they are unevenly filled and team distribution is random
	// any changed players will be flagged "updated"
	void RecheckTeams();

	// marks all unjoined players as not-in-team and reassigns a team for them
	// also automatically flags all affected player infos as updated
	void ReassignAllTeams();

private:
	// team distribution configuration
	std::string GetTeamDistName(TeamDist eTeamDist) const;

public:
	void FillTeamDistOptions(C4GUI::ComboBox_FillCB *pFiller) const;
	void SendSetTeamDist(TeamDist eNewDist);
	TeamDist GetTeamDist() const { return eTeamDist; }
	std::string GetTeamDistString() const;
	bool HasTeamDistOptions() const;
	void SetTeamDistribution(TeamDist eToVal);
	void SendSetTeamColors(bool fEnabled);
	void SetTeamColors(bool fEnabled);
	void SetRandomTeamCount(int32_t count);
};

template<>
struct C4EnumInfo<C4TeamList::TeamDist>
{
	using E = C4TeamList::TeamDist;
	static inline constexpr auto data = mkEnumInfo<E>("TEAMDIST_",
		{
			{ E::TEAMDIST_Free,      "Free" },
			{ E::TEAMDIST_Host,      "Host" },
			{ E::TEAMDIST_None,      "None" },
			{ E::TEAMDIST_Random,    "Random" },
			{ E::TEAMDIST_RandomInv, "RandomInv" }
		}
	);
};
