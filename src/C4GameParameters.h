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

/* Game parameters - game data that is valid before the game is started */

#pragma once

#include "C4DelegatedIterable.h"
#include "C4IDList.h"
#include "C4PlayerInfo.h"
#include "C4LangStringTable.h"
#include "C4Teams.h"

#include <memory>
#include <vector>

class C4GameRes
{
	friend class C4GameResList;

public:
	C4GameRes();
	C4GameRes(const C4GameRes &Res);
	~C4GameRes();

	C4GameRes &operator=(const C4GameRes &Res);

private:
	C4Network2ResType eType;
	StdStrBuf File;
	const C4Network2ResCore *pResCore;
	C4Network2Res::Ref pNetRes;

public:
	C4Network2ResType        getType()    const { return eType; }
	const char              *getFile()    const { return File.getData(); }
	bool                     isPresent()  const { return !!File; }
	const C4Network2ResCore *getResCore() const { return pResCore; }
	C4Network2Res::Ref       getNetRes()  const { return pNetRes; }

	void SetFile(C4Network2ResType eType, const char *szFile);
	void SetNetRes(C4Network2Res::Ref pRes);

	bool Publish(C4Network2ResList *pResList);
	bool Load(C4Network2ResList *pResList);
	bool InitNetwork(C4Network2ResList *pResList);

	void CalcHash();

	void Clear();

	void CompileFunc(StdCompiler *pComp);
};

class C4GameResList : public C4DelegatedIterable<C4GameResList>
{
private:
	std::vector<std::unique_ptr<C4GameRes>> resList;

public:
	class ResTypeIterator
	{
		using InternalIterator = std::vector<std::unique_ptr<C4GameRes>>::const_iterator;
		const std::vector<std::unique_ptr<C4GameRes>> &resList;
		C4Network2ResType type;
		InternalIterator it;

	public:
		ResTypeIterator(C4Network2ResType type, const std::vector<std::unique_ptr<C4GameRes>> &resList);

		ResTypeIterator &operator++();

		bool operator==(const ResTypeIterator &other) const;
		bool operator!=(const ResTypeIterator &other) const { return !(*this == other); }

		C4GameRes &operator*() const;
		C4GameRes *operator->() const;

		ResTypeIterator begin() const { return *this; }
		ResTypeIterator end() const;

	private:
		void filter();
	};

	using Iterable = ConstIterableMember<&C4GameResList::resList>;

	C4GameResList &operator=(const C4GameResList &List);

	ResTypeIterator iterRes(C4Network2ResType eType);

	void Clear();
	bool Load(const std::vector<std::string> &DefinitionFilenames); // host: create res cores by definition filenames

	C4GameRes *CreateByFile(C4Network2ResType eType, const char *szFile);
	bool InitNetwork(C4Network2ResList *pNetResList);

	void CalcHashes();

	bool RetrieveFiles(); // client: make sure all definition files are loaded

	void CompileFunc(StdCompiler *pComp);

protected:
	void Add(C4GameRes *pRes);
};

class C4GameParameters
{
public:
	C4GameParameters();
	~C4GameParameters();

	// League (empty if it's not a league game)
	StdStrBuf League;
	StdStrBuf LeagueAddress;
	StdStrBuf StreamAddress;

	// Random seed
	int32_t RandomSeed;

	// Maximum player count allowed, count at game start
	int32_t MaxPlayers, StartupPlayerCount;

	// Fair crew option
	bool UseFairCrew;
	bool FairCrewForced; // true for scenarios in which this setting may not be altered
	int32_t FairCrewStrength;

	// Original network game? Also set in replays of network games for sync safety
	bool IsNetworkGame;

	// Control rate
	int32_t ControlRate;

	// Automatic frame skip enabled for this game?
	bool AutoFrameSkip;

	// Allow debug mode?
	bool AllowDebug;

	// Scenario title
	ValidatedStdStrBuf<C4InVal::VAL_NameExNoEmpty> ScenarioTitle;

	// Active rules and goals
	C4IDList Rules;
	C4IDList Goals;

	// Game resources
	C4GameRes Scenario;
	C4GameResList GameRes;

	// Clients
	C4ClientList Clients;

	// Players & Teams
	C4PlayerInfoList PlayerInfos;
	C4PlayerInfoList RestorePlayerInfos;
	C4TeamList Teams;

	bool isLeague() const { return !!LeagueAddress.getLength(); }
	bool doStreaming() const { return !!StreamAddress.getLength(); }
	const char *getLeague() { return League.getData(); }
	StdStrBuf GetGameGoalString();
	void EnforceLeagueRules(class C4Scenario *pScenario);
	bool CheckLeagueRulesStart(bool fFixIt);

	void Clear();
	bool Load(C4Group &hGroup, C4Scenario *pDefault, const char *szGameText, C4LangStringTable *pLang, const std::vector<std::string> &DefinitionFilenames);
	bool InitNetwork(C4Network2ResList *pResList);
	bool Save(C4Group &hGroup, C4Scenario *pDefault);

	void CompileFunc(StdCompiler *pComp, C4Scenario *pScenario = nullptr);
};
