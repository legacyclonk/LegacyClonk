/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2008, Sven2
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

// Round result information to be displayed in game over dialog
// Collects information from:
// -Instances of C4Player as they are evaluated (EvaluatePlayer)
// -Game evaluation (EvaluateGame)
// -League evaluation (EvaluateLeague)
// -Script calls to AddCustomEvaluationString

#pragma once

#include "C4Constants.h"
#include "C4Components.h"
#include "C4EnumInfo.h"
#include "C4ForwardDeclarations.h"
#include "C4IDList.h"
#include "C4PacketBase.h"
#include "C4FacetEx.h"

// Contains additional data not present in C4PlayerInfo
class C4RoundResultsPlayer
{
private:
	// player ID of linked structure in Game.PlayerInfos
	int32_t id;

	// player icon
	// not compiled, so it will be lost for eliminated players after savegame resume,
	//  or for runtime joiners
	C4FacetExSurface fctBigIcon;

	// game data
	uint32_t iTotalPlayingTime; // total playing time in seconds
	int32_t iScoreOld, iScoreNew;
	StdStrBuf sCustomEvaluationStrings; // scenario specific

	// league data
	int32_t iLeagueScoreNew;      // score on league server after this round - -1 for unknown
	int32_t iLeagueScoreGain;     // league score gained by this round - -1 for unknown
	int32_t iLeagueRankNew;       // rank on league server after this round
	int32_t iLeagueRankSymbolNew; // rank symbol on league server after this round
	int32_t iLeaguePerformance;   // script-set performance value, effect league-dependent
	StdStrBuf sLeagueProgressData; // scenario-specific data to store more proigress info (which levels were done, etc.)

public:
	enum LeagueStatus : std::uint8_t
	{
		RRPLS_Unknown = 0, RRPLS_Lost, RRPLS_Won
	} eLeagueStatus; // whether player lost or won

	C4RoundResultsPlayer() : id(0), iTotalPlayingTime(0), iScoreOld(-1), iScoreNew(-1), iLeagueScoreNew(-1), iLeagueScoreGain(0), iLeagueRankNew(0), iLeagueRankSymbolNew(0), iLeaguePerformance(0), sLeagueProgressData(), eLeagueStatus(RRPLS_Unknown) {}
	C4RoundResultsPlayer(const C4RoundResultsPlayer &cpy) { *this = cpy; }

	void CompileFunc(StdCompiler *pComp);

	int32_t GetID() const { return id; }
	C4FacetEx &GetBigIcon() { return fctBigIcon; }
	uint32_t GetTotalPlayingTime() const { return iTotalPlayingTime; }
	int32_t GetScoreOld() const { return iScoreOld; }
	bool IsScoreNewValid() const { return iScoreNew >= 0; }
	int32_t GetScoreNew() const { return iScoreNew; }
	const char *GetCustomEvaluationStrings() { return sCustomEvaluationStrings.getData(); }
	int32_t GetLeagueScoreNew() const { return iLeagueScoreNew; } // returns score number on league server after round evaluation (0 for not assigned)
	bool IsLeagueScoreNewValid() const { return iLeagueScoreNew >= 0; }
	int32_t GetLeagueScoreGain() const { return iLeagueScoreGain; }
	int32_t GetLeagueRankSymbolNew() const { return iLeagueRankSymbolNew; }
	int32_t GetLeaguePerformance() const { return iLeaguePerformance; }

	void EvaluateLeague(C4RoundResultsPlayer *pLeaguePlayer); // called from league evaluation; set league fields
	void EvaluatePlayer(C4Player *pPlr); // called from C4Player::Evaluate; set fields by player

	void SetID(int32_t idNew) { id = idNew; }
	void AddCustomEvaluationString(const char *szCustomString);
	void SetLeaguePerformance(int32_t iNewPerf) { iLeaguePerformance = iNewPerf; }

	bool operator==(const C4RoundResultsPlayer &cmp) const;
	C4RoundResultsPlayer &operator=(const C4RoundResultsPlayer &cpy);
};

template<>
struct C4EnumInfo<C4RoundResultsPlayer::LeagueStatus>
{
	using E = C4RoundResultsPlayer::LeagueStatus;
	static inline constexpr auto data = mkEnumInfo<E>("RRPLS_",
		{
			{ E::RRPLS_Unknown, "" },
			{ E::RRPLS_Lost,    "Lost" },
			{ E::RRPLS_Won,     "Won" }
		}
	);
};

// player list in round results (std::vector<C4RoundResultsPlayer>...)
class C4RoundResultsPlayers
{
private:
	// players
	C4RoundResultsPlayer **ppPlayers;
	int32_t iPlayerCount, iPlayerCapacity;

public:
	C4RoundResultsPlayers() : ppPlayers(nullptr), iPlayerCount(0), iPlayerCapacity(0) {}
	C4RoundResultsPlayers(const C4RoundResultsPlayers &cpy) : ppPlayers(nullptr), iPlayerCount(0), iPlayerCapacity(0) { *this = cpy; }
	~C4RoundResultsPlayers() { Clear(); }

	void Clear();

	void CompileFunc(StdCompiler *pComp);

private:
	void GrowList(size_t iByVal);

public:
	C4RoundResultsPlayer *GetByIndex(int32_t idx) const;
	C4RoundResultsPlayer *GetByID(int32_t id) const;
	int32_t GetCount() const { return iPlayerCount; }

	void Add(C4RoundResultsPlayer *pNewPlayer);
	C4RoundResultsPlayer *GetCreateByID(int32_t id);

	bool operator==(const C4RoundResultsPlayers &cmp) const;
	C4RoundResultsPlayers &operator=(const C4RoundResultsPlayers &cpy);
};

class C4RoundResults
{
public:
	enum NetResult : std::uint8_t
	{
		NR_None = 0,    // undefined
		NR_LeagueOK,    // league evaluated
		NR_LeagueError, // league evaluation error
		NR_NetError,    // network disconnect
	};

private:
	// player list
	C4RoundResultsPlayers Players;

	// game data
	C4IDList Goals; // Goals at time of evaluation
	C4IDList FulfilledGoals; // only those goals that are fulfilled
	uint32_t iPlayingTime; // game time in seconds
	int32_t iLeaguePerformance; // settlement league performance points
	bool fHideSettlementScore; // to hide the score in the evaluation dialogue (for melees)

	// league/network result
	StdStrBuf sNetResult;
	NetResult eNetResult;

	// scenario-specific
	StdStrBuf sCustomEvaluationStrings;

public:
	C4RoundResults() : iPlayingTime(0) {}
	~C4RoundResults() { Clear(); }

	void Clear();
	void Init();

	void CompileFunc(StdCompiler *pComp);

public:
	// fill GoalList with current goal status - also called by goal menu!
	static void EvaluateGoals(C4IDList &GoalList, C4IDList &FulfilledGoalList, int32_t iPlayerNumber);

	// Evaluation called by C4Game::Evaluate
	// Caution: This does script callbacks for goal fulfillment check and must be called in sync,
	//  i.e. even for dedicated server
	void EvaluateGame();

	// Evaluation called by league: Sets new league scores and ranks
	void EvaluateLeague(const char *szResultMsg, bool fSuccess, const C4RoundResultsPlayers &rLeagueInfo);

	// Evaluation called by player when it's evaluated
	void EvaluatePlayer(C4Player *pPlr);

	// Evaluation by network: Disconnect or league info
	void EvaluateNetwork(NetResult eResult, const char *szResultsString);

	// Set custom string to be shown in game over dialog
	// idPlayer==0 for global strings
	void AddCustomEvaluationString(const char *szCustomString, int32_t idPlayer);

	// to hide the settlement score in melees
	void HideSettlementScore(bool fHide = true);
	bool SettlementScoreIsHidden();

	// Set league performance
	// Used for special league scenarios, e.g. settlement scenarios that wish to use a
	// measure different from the elapsed game time
	void SetLeaguePerformance(int32_t iNewPerf, int32_t idPlayer = 0);
	int32_t GetLeaguePerformance(int32_t idPlayer = 0) const;

	const C4RoundResultsPlayers &GetPlayers() const { return Players; }
	const char *GetCustomEvaluationStrings() const { return sCustomEvaluationStrings.getData(); }
	NetResult GetNetResult() const { return eNetResult; }
	const char *GetNetResultString() const { return sNetResult.getData(); }
	bool HasNetResult() const { return eNetResult != NR_None; }

	bool Load(C4Group &hGroup, const char *szFilename = C4CFN_RoundResults);
	bool Save(C4Group &hGroup, const char *szFilename = C4CFN_RoundResults);

	const C4IDList &GetGoals() const { return Goals; }
	const C4IDList &GetFulfilledGoals() const { return FulfilledGoals; }
};

template<>
struct C4EnumInfo<C4RoundResults::NetResult>
{
	using E = C4RoundResults::NetResult;
	static inline constexpr auto data = mkEnumInfo<E>("NR_",
		{
			{ E::NR_None,        "" },
			{ E::NR_LeagueOK,    "LeagueOK" },
			{ E::NR_LeagueError, "LeagueError" },
			{ E::NR_NetError,    "NetError" }
		}
	);
};

// * PID_LeagueRoundResults
// packet containing league round results for all participating players
// sent from host to client after league evaluation
class C4PacketLeagueRoundResults : public C4PacketBase
{
public:
	C4RoundResultsPlayers Players; // league info for players
	StdStrBuf sResultsString; // league result string - or error message
	bool fSuccess; // whether result was successful or not

	C4PacketLeagueRoundResults() : fSuccess(false) {}
	C4PacketLeagueRoundResults(const char *szResultsString, bool fSuccess, const C4RoundResultsPlayers &Players) : Players(Players), sResultsString(szResultsString), fSuccess(fSuccess) {}
	C4PacketLeagueRoundResults(const char *szResultsString, bool fSuccess) : sResultsString(szResultsString), fSuccess(fSuccess) {}

	virtual void CompileFunc(StdCompiler *pComp) override;
};
