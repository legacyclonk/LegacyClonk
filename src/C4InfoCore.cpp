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

/* Structures for object and player info components */

#include <C4Include.h>
#include <C4InfoCore.h>

#ifndef BIG_C4INCLUDE
#include <C4Random.h>
#include <C4RankSystem.h>
#include <C4Group.h>
#include <C4Components.h>
#ifdef C4ENGINE
#include <C4Wrappers.h>
#endif
#endif

#include <C4Random.h>

#ifdef C4GROUP
#include "C4CompilerWrapper.h"
#include "C4Def.h"
#endif

// Name File Handling

char GetANameBuffer[C4MaxName + 1];

const char *GetAName(const char *szNameFile)
{
	// always eat the Random-value, so having or not having a Names.txt makes no difference
	int iName = Random(1000);

	FILE *hNamefile;

	if (!szNameFile) return "Clonk";
	if (!(hNamefile = fopen(szNameFile, "r"))) return "Clonk";

	for (int iCnt = 0; iCnt < iName; iCnt++)
		AdvanceFileLine(hNamefile);
	GetANameBuffer[0] = 0; int iLoops = 0;
	do
	{
		if (!ReadFileLine(hNamefile, GetANameBuffer, C4MaxName))
		{
			rewind(hNamefile); iLoops++;
		}
	} while ((iLoops < 2) && (!GetANameBuffer[0] || (GetANameBuffer[0] == '#') || (GetANameBuffer[0] == ' ')));
	fclose(hNamefile);
	if (iLoops >= 2) return "Clonk";
	return GetANameBuffer;
}

// Player Info

C4PlayerInfoCore::C4PlayerInfoCore()
{
	std::memset(this, 0, sizeof(C4PlayerInfoCore));
	Default();
}

void C4PlayerInfoCore::Default(C4RankSystem *pRanks)
{
	std::memset(this, 0, sizeof(C4PlayerInfoCore));
	Rank = 0;
	SCopy("Neuling", PrefName);
	if (pRanks) SCopy(pRanks->GetRankName(Rank, false).getData(), RankName);
	else SCopy("Rang", RankName);
	PrefColor = 0;
	PrefColorDw = 0xff;
	PrefColor2Dw = 0;
	PrefControl = C4P_Control_Keyboard1;
	PrefPosition = 0;
	PrefMouse = 1;
	PrefControlStyle = 0;
	PrefAutoContextMenu = 0;
	ExtraData.Reset();
}

uint32_t C4PlayerInfoCore::GetPrefColorValue(int32_t iPrefColor)
{
	uint32_t valRGB[12] =
	{
		0x0000E8, 0xF40000, 0x00C800, 0xFCF41C,
		0xC48444, 0x784830, 0xA04400, 0xF08050,
		0x848484, 0xFFFFFF, 0x0094F8, 0xBC00C0
	};
	if (Inside<int32_t>(iPrefColor, 0, 11))
		return valRGB[iPrefColor];
	return 0xAAAAAA;
}

bool C4PlayerInfoCore::Load(C4Group &hGroup)
{
	// New version
	StdStrBuf Source;
	if (hGroup.LoadEntryString(C4CFN_PlayerInfoCore, Source))
	{
		// Compile
		StdStrBuf GrpName = hGroup.GetFullName(); GrpName.Append(DirSep C4CFN_PlayerInfoCore);
		if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(*this, Source, GrpName.getData()))
			return false;
		// Pref for AutoContextMenus is still undecided: default by player's control style
		if (PrefAutoContextMenu == -1)
			PrefAutoContextMenu = PrefControlStyle;
		// Determine true color from indexed pref color
		if (!PrefColorDw)
			PrefColorDw = GetPrefColorValue(PrefColor);
		// Validate colors
		PrefColorDw &= 0xffffff;
		PrefColor2Dw &= 0xffffff;
		// Validate name
#ifdef C4ENGINE
		CMarkup::StripMarkup(PrefName);
#endif
		// Success
		return true;
	}

	// Old version no longer supported - sorry
	return false;
}

bool C4PlayerInfoCore::Save(C4Group &hGroup)
{
	StdStrBuf Source, Name = hGroup.GetFullName(); Name.Append(DirSep C4CFN_PlayerInfoCore);
	if (!DecompileToBuf_Log<StdCompilerINIWrite>(*this, &Source, Name.getData()))
		return false;
	if (!hGroup.Add(C4CFN_PlayerInfoCore, Source, false, true))
		return false;
	hGroup.Delete("C4Player.c4b");
	return true;
}

void C4PlayerInfoCore::CompileFunc(StdCompiler *pComp)
{
	pComp->Name("Player");
	pComp->Value(mkNamingAdapt(toC4CStr(PrefName), "Name",             "Neuling"));
	pComp->Value(mkNamingAdapt(toC4CStr(Comment),  "Comment",          ""));
	pComp->Value(mkNamingAdapt(Rank,               "Rank",             0));
	pComp->Value(mkNamingAdapt(toC4CStr(RankName), "RankName",         LoadResStr("IDS_MSG_RANK")));
	pComp->Value(mkNamingAdapt(Score,              "Score",            0));
	pComp->Value(mkNamingAdapt(Rounds,             "Rounds",           0));
	pComp->Value(mkNamingAdapt(RoundsWon,          "RoundsWon",        0));
	pComp->Value(mkNamingAdapt(RoundsLost,         "RoundsLost",       0));
	pComp->Value(mkNamingAdapt(TotalPlayingTime,   "TotalPlayingTime", 0));
	pComp->Value(mkNamingAdapt(ExtraData,          "ExtraData",        C4ValueMapData()));
	pComp->NameEnd();

	pComp->Name("Preferences");
	pComp->Value(mkNamingAdapt(PrefColor,           "Color",            0));
	pComp->Value(mkNamingAdapt(PrefColorDw,         "ColorDw",          0xffu));
	pComp->Value(mkNamingAdapt(PrefColor2Dw,        "AlternateColorDw", 0u));
	pComp->Value(mkNamingAdapt(PrefControl,         "Control",          C4P_Control_Keyboard2));
	pComp->Value(mkNamingAdapt(PrefControlStyle,    "AutoStopControl",  0));
	pComp->Value(mkNamingAdapt(PrefAutoContextMenu, "AutoContextMenu",  -1)); // compiling default is -1 (if this is detected, AutoContextMenus will be defaulted by control style)
	pComp->Value(mkNamingAdapt(PrefPosition,        "Position",         0));
	pComp->Value(mkNamingAdapt(PrefMouse,           "Mouse",            1));
	pComp->NameEnd();

	pComp->Value(mkNamingAdapt(LastRound, "LastRound"));
}

// Physical Info

struct C4PhysInfoNameMap_t { const char *szName; C4PhysicalInfo::Offset off; } C4PhysInfoNameMap[] =
{
	{ "Energy",          &C4PhysicalInfo::Energy },
	{ "Breath",          &C4PhysicalInfo::Breath },
	{ "Walk",            &C4PhysicalInfo::Walk },
	{ "Jump",            &C4PhysicalInfo::Jump },
	{ "Scale",           &C4PhysicalInfo::Scale },
	{ "Hangle",          &C4PhysicalInfo::Hangle },
	{ "Dig",             &C4PhysicalInfo::Dig },
	{ "Swim",            &C4PhysicalInfo::Swim },
	{ "Throw",           &C4PhysicalInfo::Throw },
	{ "Push",            &C4PhysicalInfo::Push },
	{ "Fight",           &C4PhysicalInfo::Fight },
	{ "Magic",           &C4PhysicalInfo::Magic },
	{ "Float",           &C4PhysicalInfo::Float },
	{ "CanScale",        &C4PhysicalInfo::CanScale },
	{ "CanHangle",       &C4PhysicalInfo::CanHangle },
	{ "CanDig",          &C4PhysicalInfo::CanDig },
	{ "CanConstruct",    &C4PhysicalInfo::CanConstruct },
	{ "CanChop",         &C4PhysicalInfo::CanChop },
	{ "CanFly",          &C4PhysicalInfo::CanFly },
	{ "CorrosionResist", &C4PhysicalInfo::CorrosionResist },
	{ "BreatheWater",    &C4PhysicalInfo::BreatheWater },
	{ nullptr, nullptr }
};

void C4PhysicalInfo::PromotionUpdate(int32_t iRank, bool fUpdateTrainablePhysicals, C4Def *pTrainDef)
{
#ifdef C4ENGINE
	if (iRank >= 0) { CanDig = 1; CanChop = 1; CanConstruct = 1; }
	if (iRank >= 0) { CanScale = 1; }
	if (iRank >= 0) { CanHangle = 1; }
	Energy = std::max<int32_t>(Energy, (50 + 5 * BoundBy<int32_t>(iRank, 0, 10)) * C4MaxPhysical / 100);
	if (fUpdateTrainablePhysicals && pTrainDef)
	{
		// do standard training: Expect everything to be trained fully at rank 20
		int32_t iTrainRank = BoundBy<int32_t>(iRank, 0, 20);
		Scale  = pTrainDef->Physical.Scale  + (C4MaxPhysical - pTrainDef->Physical.Scale)  * iTrainRank / 20;
		Hangle = pTrainDef->Physical.Hangle + (C4MaxPhysical - pTrainDef->Physical.Hangle) * iTrainRank / 20;
		Swim   = pTrainDef->Physical.Swim   + (C4MaxPhysical - pTrainDef->Physical.Swim)   * iTrainRank / 20;
		Fight  = pTrainDef->Physical.Fight  + (C4MaxPhysical - pTrainDef->Physical.Fight)  * iTrainRank / 20;
		// do script updates for any physicals as required (this will train stuff like magic)
		const char *szPhysName; C4PhysicalInfo::Offset PhysOff;
		for (int32_t iPhysIdx = 0; szPhysName = GetNameByIndex(iPhysIdx, &PhysOff); ++iPhysIdx)
		{
			C4Value PhysVal(this->*PhysOff, C4V_Int);
			C4AulParSet Pars(C4VString(szPhysName), C4VInt(iRank), C4VRef(&PhysVal));
			if (!!pTrainDef->Script.Call(PSF_GetFairCrewPhysical, &Pars))
			{
				this->*PhysOff = PhysVal.getInt();
			}
		}
	}
#endif
}

C4PhysicalInfo::C4PhysicalInfo()
{
	Default();
}

void C4PhysicalInfo::Default()
{
	std::memset(this, 0, sizeof(C4PhysicalInfo));
}

bool C4PhysicalInfo::GetOffsetByName(const char *szPhysicalName, Offset *pmpiOut)
{
	// query map
	for (C4PhysInfoNameMap_t *entry = C4PhysInfoNameMap; entry->szName; ++entry)
		if (SEqual(entry->szName, szPhysicalName))
		{
			*pmpiOut = entry->off;
			return true;
		}
	return false;
}

const char *C4PhysicalInfo::GetNameByOffset(Offset mpiOff)
{
	// query map
	for (C4PhysInfoNameMap_t *entry = C4PhysInfoNameMap; entry->szName; ++entry)
		if (entry->off == mpiOff)
			return entry->szName;
	return nullptr;
}

const char *C4PhysicalInfo::GetNameByIndex(int32_t iIdx, Offset *pmpiOut)
{
	// query map
	if (!Inside<int32_t>(iIdx, 0, sizeof(C4PhysInfoNameMap) / sizeof(C4PhysInfoNameMap_t))) return nullptr;
	if (pmpiOut) *pmpiOut = C4PhysInfoNameMap[iIdx].off;
	return C4PhysInfoNameMap[iIdx].szName;
}

void C4PhysicalInfo::CompileFunc(StdCompiler *pComp)
{
	for (C4PhysInfoNameMap_t *entry = C4PhysInfoNameMap; entry->szName; ++entry)
		pComp->Value(mkNamingAdapt((this->*(entry->off)), entry->szName, 0));
}

void C4PhysicalInfo::TrainValue(int32_t *piVal, int32_t iTrainBy, int32_t iMaxTrain)
{
	// only do training if value was nonzero before (e.g., Magic for revaluated Clonks)
	if (*piVal)
		// do train value: Do not increase above maximum, but never decrease either
		*piVal = (std::max)((std::min)(*piVal + iTrainBy, iMaxTrain), *piVal);
}

void C4PhysicalInfo::Train(Offset mpiOffset, int32_t iTrainBy, int32_t iMaxTrain)
{
	// train own value
	TrainValue(&(this->*mpiOffset), iTrainBy, iMaxTrain);
}

bool C4PhysicalInfo::operator==(const C4PhysicalInfo &cmp) const
{
	// all fields must be equal
	for (C4PhysInfoNameMap_t *entry = C4PhysInfoNameMap; entry->szName; ++entry)
		if (this->*(entry->off) != cmp.*(entry->off))
			return false;
	return true;
}

void C4TempPhysicalInfo::CompileFunc(StdCompiler *pComp)
{
	C4PhysicalInfo::CompileFunc(pComp);
#ifdef C4ENGINE
	pComp->Value(mkNamingAdapt(mkSTLContainerAdapt(Changes), "Changes", std::vector<C4PhysicalChange>()));
#endif
}

void C4TempPhysicalInfo::Train(Offset mpiOffset, int32_t iTrainBy, int32_t iMaxTrain)
{
	// train own value
	C4PhysicalInfo::Train(mpiOffset, iTrainBy, iMaxTrain);
	// train all temp values
#ifdef C4ENGINE
	for (std::vector<C4PhysicalChange>::iterator i = Changes.begin(); i != Changes.end(); ++i)
		if (i->mpiOffset == mpiOffset)
			TrainValue(&(i->PrevVal), iTrainBy, iMaxTrain);
#endif
}

bool C4TempPhysicalInfo::HasChanges(C4PhysicalInfo *pRefPhysical)
{
#ifdef C4ENGINE
	// always return true if there are temp changes
	if (!Changes.empty()) return true;
	// also return true if any value deviates from the reference
	if (pRefPhysical)
	{
		if (!(*pRefPhysical == *this)) return true;
	}
#endif
	// no change known
	return false;
}

void C4TempPhysicalInfo::RegisterChange(C4PhysicalInfo::Offset mpiOffset)
{
	// append physical change to list
#ifdef C4ENGINE
	Changes.push_back(C4PhysicalChange(this->*mpiOffset, mpiOffset));
#endif
}

bool C4TempPhysicalInfo::ResetPhysical(C4PhysicalInfo::Offset mpiOffset)
{
#ifdef C4ENGINE
	// search last matching physical check (should always be last if well scripted)
	for (std::vector<C4PhysicalChange>::reverse_iterator i = Changes.rbegin(); i != Changes.rend(); ++i)
		if ((*i).mpiOffset == mpiOffset)
		{
			this->*mpiOffset = (*i).PrevVal;
			Changes.erase((i + 1).base());
			return true;
		}
#endif
	return false;
}

void C4PhysicalChange::CompileFunc(StdCompiler *pComp)
{
	// name=oldval
	char phyn[C4MaxName + 1];
	const char *szPhyn = C4PhysicalInfo::GetNameByOffset(mpiOffset);
	if (szPhyn) SCopy(szPhyn, phyn, C4MaxName); else *phyn = '\0';
	pComp->Value(mkStringAdapt(phyn, C4MaxName, StdCompiler::RCT_Idtf));
	if (!C4PhysicalInfo::GetOffsetByName(phyn, &mpiOffset)) pComp->excNotFound("Physical change name \"%s\" not found.");
	pComp->Seperator(StdCompiler::SEP_SET);
	pComp->Value(PrevVal);
}

// Object Info

C4ObjectInfoCore::C4ObjectInfoCore()
{
	Default();
}

void C4ObjectInfoCore::Default(C4ID n_id,
	C4DefList *pDefs,
	const char *cpNames)
{
	// Def
	C4Def *pDef = nullptr;
	if (pDefs) pDef = pDefs->ID2Def(n_id);

	// Defaults
	id = n_id;
	Participation = 1;
	Rank = 0;
	Experience = 0;
	Rounds = 0;
	DeathCount = 0;
	Birthday = 0;
	TotalPlayingTime = 0;
	SCopy("Clonk", Name, C4MaxName);
	SCopy("Clonk", TypeName, C4MaxName);
	sRankName.Copy("Clonk");
	sNextRankName.Clear();
	NextRankExp = 0;
	DeathMessage[0] = '\0';
	*PortraitFile = 0;
	Age = 0;
	ExtraData.Reset();

	// Type
	if (pDef) SCopy(pDef->GetName(), TypeName, C4MaxName);

	// Name
	if (cpNames)
	{
		// Name file reference
		if (SSearchNoCase(cpNames, C4CFN_Names))
			SCopy(GetAName(cpNames), Name, C4MaxName);
		// Name list
		else
		{
			SCopySegment(cpNames, Random(SCharCount(0x0A, cpNames)), Name, 0x0A, C4MaxName + 1);
			SClearFrontBack(Name);
			SReplaceChar(Name, 0x0D, 0x00);
		}
		if (!Name[0]) SCopy("Clonk", Name, C4MaxName);
	}

#ifdef C4ENGINE
	if (pDefs) UpdateCustomRanks(pDefs);
#endif

	// Physical
	Physical.Default();
	if (pDef) Physical = pDef->Physical;
	Physical.PromotionUpdate(Rank);

	// Old format
}

void C4ObjectInfoCore::Promote(int32_t iRank, C4RankSystem &rRanks, bool fForceRankName)
{
	Rank = iRank;
	Physical.PromotionUpdate(Rank);
	// copy new rank name if defined only, or forced to use highest defined rank for too high info ranks
	StdStrBuf sNewRank(rRanks.GetRankName(Rank, fForceRankName));
	if (sNewRank) sRankName.Copy(sNewRank);
}

#ifdef C4ENGINE
void C4ObjectInfoCore::UpdateCustomRanks(C4DefList *pDefs)
{
	assert(pDefs);
	C4Def *pDef = pDefs->ID2Def(id);
	if (!pDef) return;
	if (pDef->pRankNames)
	{
		StdStrBuf sRank(pDef->pRankNames->GetRankName(Rank, false));
		if (sRank) sRankName.Copy(sRank);
		// next rank data
		StdStrBuf sNextRank(pDef->pRankNames->GetRankName(Rank + 1, false));
		if (sNextRank)
		{
			sNextRankName.Copy(sNextRank);
			NextRankExp = pDef->pRankNames->Experience(Rank + 1);
		}
		else
		{
			// no more promotion possible by custom rank system
			sNextRankName.Clear();
			NextRankExp = C4RankSystem::EXP_NoPromotion;
		}
	}
	else
	{
		// definition does not have custom rank names
		sNextRankName.Clear();
		NextRankExp = 0;
	}
}
#endif

bool C4ObjectInfoCore::GetNextRankInfo(C4RankSystem &rDefaultRanks, int32_t *piNextRankExp, StdStrBuf *psNextRankName)
{
	int32_t iNextRankExp;
	// custom rank assigned?
	if (NextRankExp)
	{
		iNextRankExp = NextRankExp;
		if (psNextRankName) psNextRankName->Copy(sNextRankName);
	}
	else
	{
		// no custom rank: Get from default set
		StdStrBuf sRank(rDefaultRanks.GetRankName(Rank + 1, false));
		if (sRank)
		{
			iNextRankExp = rDefaultRanks.Experience(Rank + 1);
			if (psNextRankName) psNextRankName->Copy(sRank);
		}
		else
			// no more promotion
			iNextRankExp = C4RankSystem::EXP_NoPromotion;
	}
	// return result
	if (piNextRankExp) *piNextRankExp = iNextRankExp;
	// return value is whether additional promotion is possible
	return iNextRankExp != C4RankSystem::EXP_NoPromotion;
}

bool C4ObjectInfoCore::Load(C4Group &hGroup)
{
	StdStrBuf Source;
	return hGroup.LoadEntryString(C4CFN_ObjectInfoCore, Source) &&
		Compile(Source.getData());
}

bool C4ObjectInfoCore::Save(C4Group &hGroup, C4DefList *pDefs)
{
#ifdef C4ENGINE
	// rank overload by def: Update any NextRank-stuff
	if (pDefs) UpdateCustomRanks(pDefs);
#endif
	char *Buffer; size_t BufferSize;
	if (!Decompile(&Buffer, &BufferSize))
		return false;
	if (!hGroup.Add(C4CFN_ObjectInfoCore, Buffer, BufferSize, false, true))
	{
		delete[] Buffer; return false;
	}
	return true;
}

void C4ObjectInfoCore::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkC4IDAdapt(id),        "id",               C4ID_None));
	pComp->Value(mkNamingAdapt(toC4CStr(Name),         "Name",             "Clonk"));
	pComp->Value(mkNamingAdapt(toC4CStr(DeathMessage), "DeathMessage",     ""));
	pComp->Value(mkNamingAdapt(toC4CStr(PortraitFile), "PortraitFile",     ""));
	pComp->Value(mkNamingAdapt(Rank,                   "Rank",             0));
	pComp->Value(mkNamingAdapt(sRankName,              "RankName",         "Clonk"));
	pComp->Value(mkNamingAdapt(sNextRankName,          "NextRankName",     ""));
	pComp->Value(mkNamingAdapt(toC4CStr(TypeName),     "TypeName",         "Clonk"));
	pComp->Value(mkNamingAdapt(Participation,          "Participation",    1));
	pComp->Value(mkNamingAdapt(Experience,             "Experience",       0));
	pComp->Value(mkNamingAdapt(NextRankExp,            "NextRankExp",      0));
	pComp->Value(mkNamingAdapt(Rounds,                 "Rounds",           0));
	pComp->Value(mkNamingAdapt(DeathCount,             "DeathCount",       0));
	pComp->Value(mkNamingAdapt(Birthday,               "Birthday",         0));
	pComp->Value(mkNamingAdapt(TotalPlayingTime,       "TotalPlayingTime", 0));
	pComp->Value(mkNamingAdapt(Age,                    "Age",              0));
	pComp->Value(mkNamingAdapt(ExtraData,              "ExtraData",        C4ValueMapData()));

	pComp->FollowName("Physical");
	pComp->Value(Physical);
}

bool C4ObjectInfoCore::Compile(const char *szSource)
{
	bool ret = CompileFromBuf_LogWarn<StdCompilerINIRead>(
		mkNamingAdapt(*this, "ObjectInfo"),
		StdStrBuf(szSource),
		"ObjectInfo");
	// Do a promotion update to set physicals right
	Physical.PromotionUpdate(Rank);
	// DeathMessages are not allowed to stay forever
	if ('@' == DeathMessage[0]) DeathMessage[0] = ' ';
	return ret;
}

bool C4ObjectInfoCore::Decompile(char **ppOutput, size_t *ipSize)
{
	StdStrBuf Buf;
	if (!DecompileToBuf_Log<StdCompilerINIWrite>(
		mkNamingAdapt(*this, "ObjectInfo"),
		&Buf,
		"ObjectInfo"))
	{
		if (ppOutput) *ppOutput = nullptr;
		if (ipSize) *ipSize = 0;
		return false;
	}
	if (ppOutput) *ppOutput = Buf.GrabPointer();
	if (ipSize) *ipSize = Buf.getSize();
	return true;
}

// Round Info

C4RoundResult::C4RoundResult()
{
	Default();
}

void C4RoundResult::Default()
{
	std::memset(this, 0, sizeof(C4RoundResult));
}

void C4RoundResult::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(Title,      "Title",      ""));
	pComp->Value(mkNamingAdapt(Date,       "Date",       0u));
	pComp->Value(mkNamingAdapt(Duration,   "Duration",   0));
	pComp->Value(mkNamingAdapt(Won,        "Won",        0));
	pComp->Value(mkNamingAdapt(Score,      "Score",      0));
	pComp->Value(mkNamingAdapt(FinalScore, "FinalScore", 0));
	pComp->Value(mkNamingAdapt(TotalScore, "TotalScore", 0));
	pComp->Value(mkNamingAdapt(Bonus,      "Bonus",      0));
	pComp->Value(mkNamingAdapt(Level,      "Level",      0));
}
