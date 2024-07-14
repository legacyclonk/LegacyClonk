/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
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

/* Structures for object and player info components */

#pragma once

#include "C4Constants.h"
#include "C4ForwardDeclarations.h"
#include <C4Id.h>
#include <C4ValueMap.h>
#include <Fixed.h>

#include <vector>

class C4RankSystem;

const int32_t C4MaxPhysical = 100000,
              C4MaxDeathMsg = 75;

class C4PhysicalInfo
{
public:
	C4PhysicalInfo();

	typedef int32_t C4PhysicalInfo::*Offset;

public:
	int32_t Energy;
	int32_t Breath;
	int32_t Walk;
	int32_t Jump;
	int32_t Scale;
	int32_t Hangle;
	int32_t Dig;
	int32_t Swim;
	int32_t Throw;
	int32_t Push;
	int32_t Fight;
	int32_t Magic;
	int32_t CanScale;
	int32_t CanHangle;
	int32_t CanDig;
	int32_t CanConstruct;
	int32_t CanChop;
	int32_t CanFly;
	int32_t CorrosionResist;
	int32_t BreatheWater;
	int32_t Float;

public:
	void Default();
	void PromotionUpdate(int32_t iRank, bool fUpdateTrainablePhysicals = false, C4Section *section = nullptr, class C4Def *pTrainDef = nullptr);
	void CompileFunc(StdCompiler *pComp);

	// conversion of physical names to member pointers and vice versa
	static bool GetOffsetByName(const char *szPhysicalName, Offset *pmpiOut);
	static const char *GetNameByOffset(Offset mpiOff);
	const char *GetNameByIndex(int32_t iIdx, Offset *pmpiOut = nullptr);

	// comparison
	bool operator==(const C4PhysicalInfo &cmp) const;

	// physical training
protected:
	static void TrainValue(int32_t *piVal, int32_t iTrainBy, int32_t iMaxTrain);

public:
	void Train(Offset mpiOffset, int32_t iTrainBy, int32_t iMaxTrain);
};

class C4PhysicalChange
{
public:
	int32_t PrevVal;
	C4PhysicalInfo::Offset mpiOffset;

	C4PhysicalChange() : PrevVal(0), mpiOffset(nullptr) {}
	C4PhysicalChange(int32_t iPrevVal, C4PhysicalInfo::Offset mpiOffset)
		: PrevVal(iPrevVal), mpiOffset(mpiOffset) {}
	C4PhysicalChange(const C4PhysicalChange &rCpy) : PrevVal(rCpy.PrevVal), mpiOffset(rCpy.mpiOffset) {}

	bool operator==(const C4PhysicalChange &rCmp) const
	{
		return PrevVal == rCmp.PrevVal && mpiOffset == rCmp.mpiOffset;
	}

	C4PhysicalChange &operator=(const C4PhysicalChange &rSet)
	{
		PrevVal = rSet.PrevVal; mpiOffset = rSet.mpiOffset; return *this;
	}

	void CompileFunc(StdCompiler *pComp);
};

class C4TempPhysicalInfo : public C4PhysicalInfo
{
private:
	// changes done to the original physicals; used for backtracing
	std::vector<C4PhysicalChange> Changes;

public:
	void Clear()
	{
		Changes.clear();
	}

	void Default() { Clear(); C4PhysicalInfo::Default(); } // clears
	void CompileFunc(StdCompiler *pComp);

	void RegisterChange(C4PhysicalInfo::Offset mpiOffset); // append physical change to list
	bool ResetPhysical(C4PhysicalInfo::Offset mpiOffset);  // undo given physical change

	bool HasChanges(C4PhysicalInfo *pRefPhysical); // return true if changes list is not empty

	// also trains any change buffered physicals
	void Train(Offset mpiOffset, int32_t iTrainBy, int32_t iMaxTrain);

	C4PhysicalInfo &operator=(const C4PhysicalInfo &rSet)
	{
		Clear(); static_cast<C4PhysicalInfo &>(*this) = rSet; return *this;
	}
};

class C4ObjectInfoCore
{
public:
	C4ObjectInfoCore();

public:
	C4ID id;
	char Name[C4MaxName + 1];
	int32_t Participation;
	int32_t Rank;
	StdStrBuf sRankName;
	StdStrBuf sNextRankName;
	int32_t NextRankExp; // EXP_NoPromotion for no more promotion; 0 if standard rank system is used
	int32_t Experience, Rounds;
	int32_t DeathCount;
	char TypeName[C4MaxName + 1 + 1];
	int32_t Birthday, TotalPlayingTime;
	int32_t Age;
	char DeathMessage[C4MaxDeathMsg + 1];
	char PortraitFile[C4MaxName + 2 + 4 + 1]; // used portrait
	C4PhysicalInfo Physical;
	C4ValueMapData ExtraData;

public:
	bool Save(C4Group &hGroup, class C4DefList *pDefs);
	bool Load(C4Group &hGroup);
	void Default(C4ID n_id = C4ID_None, class C4DefList *pDefs = nullptr, const char *cpNames = nullptr);
	void Promote(int32_t iRank, C4RankSystem &rRanks, bool fForceRankName);
	bool GetNextRankInfo(C4RankSystem &rDefaultRanks, int32_t *piNextRankExp, StdStrBuf *psNextRankName);
	void CompileFunc(StdCompiler *pComp);

protected:
	bool Compile(const char *szSource);

	void UpdateCustomRanks(C4DefList *pDefs); // sets NextRankName and NextRankExp
};

class C4RoundResult
{
public:
	StdStrBuf Title{};
	uint32_t Date{};
	int32_t Duration{};
	int32_t Won{};
	int32_t Score{}, FinalScore{}, TotalScore{};
	int32_t Bonus{};
	int32_t Level{};

public:
	void Default();
	void CompileFunc(StdCompiler *pComp);
};

class C4PlayerInfoCore
{
public:
	C4PlayerInfoCore();

public:
	// Player Info
	char PrefName[C4MaxName + 1];
	char Comment[C4MaxComment + 1];
	int32_t Rank;
	char RankName[C4MaxName + 1];
	int32_t Score;
	int32_t Rounds, RoundsWon, RoundsLost;
	int32_t TotalPlayingTime;
	int32_t PrefMouse;
	C4RoundResult LastRound;
	C4ValueMapData ExtraData;
	// Preferences
	int32_t PrefColor;
	uint32_t PrefColorDw, PrefColor2Dw;
	int32_t PrefControl;
	int32_t PrefPosition;
	int32_t PrefControlStyle;
	int32_t PrefAutoContextMenu; // enable automatically opened context menus in structures

public:
	void Default(C4RankSystem *pRanks = nullptr);
	bool Load(C4Group &hGroup);
	bool Save(C4Group &hGroup);
	static uint32_t GetPrefColorValue(int32_t iPrefColor);
	void CompileFunc(StdCompiler *pComp);
};

inline C4Fixed ValByPhysical(int32_t iPercent, int32_t iPhysical) // get percentage of max physical value
{
	return itofix(iPhysical * (iPercent / 5), C4MaxPhysical * 20);
}
