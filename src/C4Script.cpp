/*
 * LegacyClonk
 *
 * Copyright (C) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017, The OpenClonk Team and contributors
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

/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */

/* Functions mapped by C4Script */

#include "C4FindObject.h"
#include <C4Include.h>
#include <C4Script.h>
#include <C4Version.h>

#include <C4Application.h>
#include <C4Object.h>
#include <C4ObjectInfo.h>
#include <C4ObjectCom.h>
#include <C4Random.h>
#include <C4Command.h>
#include <C4Console.h>
#include <C4Viewport.h>
#include <C4Wrappers.h>
#include <C4ObjectInfoList.h>
#include <C4Player.h>
#include <C4ObjectMenu.h>
#include <C4ValueHash.h>
#include <C4NetworkRestartInfos.h>
#include <C4ScriptHelpers.h>
#include <C4SoundSystem.h>

#include <memory>
#include <numbers>
#include <optional>
#include <utility>

#ifndef _WIN32
#include <sys/time.h>
#endif

using namespace C4ScriptHelpers;

const C4ValueInt MaxFnStringParLen = 500;

static std::string FnStringFormat(C4AulContext *cthr, const char *szFormatPar, C4Value *Par0 = nullptr, C4Value *Par1 = nullptr, C4Value *Par2 = nullptr, C4Value *Par3 = nullptr,
	C4Value *Par4 = nullptr, C4Value *Par5 = nullptr, C4Value *Par6 = nullptr, C4Value *Par7 = nullptr, C4Value *Par8 = nullptr, C4Value *Par9 = nullptr)
{
	C4Value *Par[11];
	Par[0] = Par0; Par[1] = Par1; Par[2] = Par2; Par[3] = Par3; Par[4] = Par4;
	Par[5] = Par5; Par[6] = Par6; Par[7] = Par7; Par[8] = Par8; Par[9] = Par9;
	Par[10] = nullptr;
	int cPar = 0;

	std::string stringBuf;
	const char *cpFormat = szFormatPar;
	const char *cpType;
	char szField[MaxFnStringParLen + 1];
	while (*cpFormat)
	{
		// Copy normal stuff
		while (*cpFormat && (*cpFormat != '%'))
			stringBuf += *cpFormat++;
		// Field
		if (*cpFormat == '%')
		{
			// Scan field type
			for (cpType = cpFormat + 1; *cpType && (*cpType == '.' || Inside(*cpType, '0', '9')); cpType++);
			// Copy field
			SCopy(cpFormat, szField, std::min<unsigned int>(sizeof(szField) - 1, cpType - cpFormat + 1));
			// Insert field by type
			switch (*cpType)
			{
			// number
			case 'd': case 'x': case 'X': case 'c':
			{
				if (!Par[cPar]) throw C4AulExecError(cthr->Obj, "format placeholder without parameter");
				stringBuf += fmt::sprintf(szField, Par[cPar++]->getInt());
				cpFormat += SLen(szField);
				break;
			}
			// C4ID
			case 'i':
			{
				if (!Par[cPar]) throw C4AulExecError(cthr->Obj, "format placeholder without parameter");
				C4ID id = Par[cPar++]->getC4ID();
				stringBuf += C4IdText(id);
				cpFormat += SLen(szField);
				break;
			}
			// C4Value
			case 'v':
			{
				if (!Par[cPar]) throw C4AulExecError(cthr->Obj, "format placeholder without parameter");
				if (!Par[cPar]->_getRaw() && !cthr->CalledWithStrictNil())
				{
					stringBuf += '0';
				}
				else
				{
					stringBuf += Par[cPar++]->GetDataString();
				}
				cpFormat += SLen(szField);
				break;
			}
			// String
			case 's':
			{
				// get string
				if (!Par[cPar]) throw C4AulExecError(cthr->Obj, "format placeholder without parameter");
				const char *szStr = "(null)";
				if (Par[cPar]->GetData())
				{
					C4String *pStr = Par[cPar++]->getStr();
					if (!pStr) throw C4AulExecError(cthr->Obj, "string format placeholder without string");
					szStr = pStr->Data.getData();
				}
				stringBuf += fmt::sprintf(szField, szStr);
				cpFormat += SLen(szField);
				break;
			}
			case '%':
				stringBuf += '%';
				cpFormat += SLen(szField);
				break;
			// Undefined / Empty
			default:
				stringBuf += '%';
				cpFormat++;
				break;
			}
		}
	}
	return stringBuf;
}

bool CheckEnergyNeedChain(C4Object *pObj, C4ObjectList &rEnergyChainChecked)
{
	if (!pObj) return false;

	// No recursion, flag check
	if (rEnergyChainChecked.GetLink(pObj)) return false;
	rEnergyChainChecked.Add(pObj, C4ObjectList::stNone);

	// This object needs energy
	if (pObj->Def->LineConnect & C4D_Power_Consumer)
		if (pObj->NeedEnergy)
			return true;

	// Check all power line connected structures
	C4Object *cline; C4ObjectLink *clnk;
	for (clnk = Game.Objects.First; clnk && (cline = clnk->Obj); clnk = clnk->Next)
		if (cline->Status) if (cline->Def->id == C4ID_PowerLine)
			if (cline->Action.Target == pObj)
				if (CheckEnergyNeedChain(cline->Action.Target2, rEnergyChainChecked))
					return true;

	return false;
}

// C4Script Functions

static C4Object *Fn_this(C4AulContext *cthr)
{
	return cthr->Obj;
}

static C4ValueInt Fn_goto(C4ValueInt iCounter)
{
	Game.Script.Counter = iCounter;
	return iCounter;
}

static bool FnChangeDef(C4ID to_id, Required<C4ObjectOrThis> pObj)
{
	return pObj->ChangeDef(to_id);
}

static bool FnExplode(C4ValueInt iLevel, Required<C4ObjectOrThis> pObj, C4ID idEffect, C4String *szEffect)
{
	pObj->Explode(iLevel, idEffect, FnStringPar(szEffect));
	return true;
}

static bool FnIncinerate(C4AulContext *cthr, Required<C4ObjectOrThis> pObj)
{
	C4ValueInt iCausedBy = NO_OWNER;
	if (cthr->Obj) iCausedBy = cthr->Obj->Controller;
	return pObj->Incinerate(iCausedBy);
}

static bool FnIncinerateLandscape(C4AulContext *cthr, C4ValueInt iX, C4ValueInt iY)
{
	MakePositionRelative(cthr, iX, iY);
	return Game.Landscape.Incinerate(iX, iY);
}

static bool FnExtinguish(Required<C4ObjectOrThis> pObj)
{
	// extinguish all fires
	return pObj->Extinguish(0);
}

static bool FnSetSolidMask(C4ValueInt iX, C4ValueInt iY, C4ValueInt iWdt, C4ValueInt iHgt, C4ValueInt iTX, C4ValueInt iTY, Required<C4ObjectOrThis> pObj)
{
	pObj->SetSolidMask(iX, iY, iWdt, iHgt, iTX, iTY);
	return true;
}

static void FnSetGravity(C4ValueInt iGravity)
{
	Game.Landscape.Gravity = itofix(BoundBy<C4ValueInt>(iGravity, -300, 300)) / 500;
}

static C4ValueInt FnGetGravity()
{
	return fixtoi(Game.Landscape.Gravity * 500);
}

template<int N>
static void DeathAnnounceMessageHelper(C4Object *const obj, const int n)
{
	if (N == n)
	{
		GameMsgObject(LoadResStr(static_cast<C4ResStrTableKey>(std::to_underlying(C4ResStrTableKey::IDS_OBJ_DEATH1) + N), obj->GetName()).c_str(), obj);
	}
	else if constexpr (N > 0)
	{
		DeathAnnounceMessageHelper<N - 1>(obj, n);
	}
}

static bool FnDeathAnnounce(C4AulContext *cthr)
{
	static constexpr int MaxDeathMsg{7};
	if (!cthr->Obj) return false;
	if (Game.C4S.Head.Film) return true;
	// Check if crew member has an own death message
	if (cthr->Obj->Info && *(cthr->Obj->Info->DeathMessage))
	{
		GameMsgObject(cthr->Obj->Info->DeathMessage, cthr->Obj);
	}
	else
	{
		DeathAnnounceMessageHelper<MaxDeathMsg - 1>(cthr->Obj, SafeRandom(MaxDeathMsg));
	}
	return true;
}

static bool FnGrabContents(C4Object &from, Required<C4ObjectOrThis> pTo)
{
	if (pTo == &from) return false;
	pTo->GrabContents(&from);
	return true;
}

static bool FnPunch(C4AulContext *cthr, C4Object *target, C4ValueInt punch)
{
	if (!cthr->Obj) return false;
	return ObjectComPunch(cthr->Obj, target, punch);
}

static bool FnKill(C4AulContext *cthr, Required<C4ObjectOrThis> pObj, bool fForced)
{
	if (!pObj->GetAlive()) return false;
	// Trace kills by player-owned objects
	// Do not trace for NO_OWNER, because that would include e.g. the Suicide-rule
	if (cthr->Obj && ValidPlr(cthr->Obj->Controller)) pObj->UpdatLastEnergyLossCause(cthr->Obj->Controller);
	// Do the kill
	pObj->AssignDeath(fForced);
	return true;
}

static bool FnFling(C4AulContext *cthr, C4Object &obj, C4ValueInt iXDir, C4ValueInt iYDir, Default<C4ValueInt, 1, true> iPrec, bool fAddSpeed)
{
	obj.Fling(itofix(iXDir, iPrec), itofix(iYDir, iPrec), fAddSpeed, cthr->Obj ? cthr->Obj->Controller : NO_OWNER);
	// unstick from ground, because Fling command may be issued in an Action-callback,
	// where attach-values have already been determined for that frame
	obj.Action.t_attach = 0;
	return true;
}

static bool FnJump(Required<C4ObjectOrThis> pObj)
{
	return ObjectComJump(pObj);
}

static bool FnEnter(C4Object &target, Required<C4ObjectOrThis> pObj)
{
	return pObj->Enter(&target);
}

static bool FnExit(C4AulContext *cthr, Required<C4ObjectOrThis> pObj, C4ValueInt tx, C4ValueInt ty, C4ValueInt tr, C4ValueInt txdir, C4ValueInt tydir, C4ValueInt trdir)
{
	MakePositionRelative(cthr, tx, ty);
	if (tr == -1) tr = Random(360);
	ObjectComCancelAttach(pObj);
	return pObj->Exit(tx,
		ty + pObj->Shape.y,
		tr,
		itofix(txdir), itofix(tydir),
		itofix(trdir) / 10);
}

static bool FnCollect(C4Object &item, Required<C4ObjectOrThis> pCollector)
{
	// Script function Collect ignores NoCollectDelay
	int32_t iOldNoCollectDelay = pCollector->NoCollectDelay;
	if (iOldNoCollectDelay)
	{
		pCollector->NoCollectDelay = 0;
		pCollector->UpdateOCF();
	}

	bool success = false;
	// check OCF of collector (MaxCarry)
	if (pCollector->OCF & OCF_Collection)
		// collect
		success = pCollector->Collect(&item);
	// restore NoCollectDelay
	if (iOldNoCollectDelay > pCollector->NoCollectDelay) pCollector->NoCollectDelay = iOldNoCollectDelay;
	// failure
	return success;
}

static bool FnSplit2Components(C4AulContext *cthr, Required<C4ObjectOrThis> pObj)
{
	C4Object *pThing, *pNew, *pContainer;
	size_t cnt, cnt2;
	// Store container
	pContainer = pObj->Contained;
	// Contents: exit / transfer to container
	while (pThing = pObj->Contents.GetObject())
		if (pContainer) pThing->Enter(pContainer);
		else pThing->Exit(pThing->x, pThing->y);
	// Destroy the object, create its components
	C4IDList ObjComponents;
	pObj->Def->GetComponents(&ObjComponents, pObj, cthr->Obj);
	if (pObj->Contained) pObj->Exit(pObj->x, pObj->y);
	for (cnt = 0; ObjComponents.GetID(cnt); cnt++)
	{
		for (cnt2 = 0; cnt2 < ObjComponents.GetCount(cnt); cnt2++)
		{
			// force argument evaluation order
			const auto r4 = itofix(Rnd3());
			const auto r3 = itofix(Rnd3());
			const auto r2 = itofix(Rnd3());
			const auto r1 = Random(360);
			if (pNew = Game.CreateObject(ObjComponents.GetID(cnt),
				pObj,
				pObj->Owner,
				pObj->x, pObj->y,
				r1, r2, r3, r4))
			{
				if (pObj->GetOnFire()) pNew->Incinerate(pObj->Owner);
				if (pContainer) pNew->Enter(pContainer);
			}
		}
	}
	pObj->AssignRemoval();
	return true;
}

static bool FnRemoveObject(Required<C4ObjectOrThis> pObj, bool fEjectContents)
{
	pObj->AssignRemoval(fEjectContents);
	return true;
}

static bool FnSetPosition(C4ValueInt iX, C4ValueInt iY, Required<C4ObjectOrThis> pObj, bool fCheckBounds)
{
	if (fCheckBounds)
	{
		// BoundsCheck takes ref to int32_t and not to C4ValueInt
		int32_t i_x = iX, i_y = iY;
		pObj->BoundsCheck(i_x, i_y);
		iX = i_x; iY = i_y;
	}
	pObj->ForcePosition(iX, iY);
	// update liquid
	pObj->UpdateInLiquid();
	return true;
}

static bool FnDoCon(C4ValueInt iChange, Required<C4ObjectOrThis> pObj) // in percent
{
	pObj->DoCon(FullCon * iChange / 100);
	return true;
}

static C4ValueInt FnGetCon(Required<C4ObjectOrThis> pObj) // in percent
{
	return 100 * pObj->GetCon() / FullCon;
}

static bool FnDoEnergy(C4AulContext *cthr, C4ValueInt iChange, Required<C4ObjectOrThis> pObj, bool fExact, Default<C4ValueInt, C4FxCall_EngScript, true> iEngType, C4ValueInt iCausedByPlusOne)
{
	C4ValueInt iCausedBy = iCausedByPlusOne - 1; if (!iCausedByPlusOne && cthr->Obj) iCausedBy = cthr->Obj->Controller;
	pObj->DoEnergy(iChange, !!fExact, iEngType, iCausedBy);
	return true;
}

static bool FnDoBreath(C4ValueInt iChange, Required<C4ObjectOrThis> pObj)
{
	pObj->DoBreath(iChange);
	return true;
}

static bool FnDoDamage(C4AulContext *cthr, C4ValueInt iChange, Required<C4ObjectOrThis> pObj, Default<C4ValueInt, C4FxCall_DmgScript, true> iDmgType, C4ValueInt iCausedByPlusOne)
{
	C4ValueInt iCausedBy = iCausedByPlusOne - 1; if (!iCausedByPlusOne && cthr->Obj) iCausedBy = cthr->Obj->Controller;
	pObj->DoDamage(iChange, iCausedBy, iDmgType);
	return true;
}

static bool FnDoMagicEnergy(C4ValueInt iChange, Required<C4ObjectOrThis> pObj, bool fAllowPartial)
{
	// Physical modification factor
	iChange *= MagicPhysicalFactor;
	// Maximum load
	if (iChange > 0)
		if (pObj->MagicEnergy + iChange > pObj->GetPhysical()->Magic)
		{
			if (!fAllowPartial) return false;
			iChange = pObj->GetPhysical()->Magic - pObj->MagicEnergy;
			if (!iChange) return false;
			// partial change to max allowed
		}
	// Insufficient load
	if (iChange < 0)
		if (pObj->MagicEnergy + iChange < 0)
		{
			if (!fAllowPartial) return false;
			iChange = -pObj->MagicEnergy;
			if (!iChange) return false;
			// partial change to zero allowed
		}
	// Change energy level
	pObj->MagicEnergy = BoundBy<C4ValueInt>(pObj->MagicEnergy + iChange, 0, pObj->GetPhysical()->Magic);
	pObj->ViewEnergy = C4ViewDelay;
	return true;
}

static C4ValueInt FnGetMagicEnergy(Required<C4ObjectOrThis> pObj)
{
	return pObj->MagicEnergy / MagicPhysicalFactor;
}

const int32_t PHYS_Current        = 0,
              PHYS_Permanent      = 1,
              PHYS_Temporary      = 2,
              PHYS_StackTemporary = 3;

static bool FnSetPhysical(C4String *szPhysical, C4ValueInt iValue, C4ValueInt iMode, Required<C4ObjectOrThis> pObj)
{
	// Get physical offset
	C4PhysicalInfo::Offset off;
	if (!C4PhysicalInfo::GetOffsetByName(FnStringPar(szPhysical), &off)) return false;
	// Set by mode
	switch (iMode)
	{
	// Currently active physical
	case PHYS_Current:
		// Info objects or temporary mode only
		if (!pObj->PhysicalTemporary) if (!pObj->Info || Game.Parameters.UseFairCrew) return false;
		// Set physical
		pObj->GetPhysical()->*off = iValue;
		return true;
	// Permanent physical
	case PHYS_Permanent:
		// Info objects only
		if (!pObj->Info) return false;
		// In fair crew mode, changing the permanent physicals is only allowed via TrainPhysical
		// Otherwise, stuff like SetPhysical(..., GetPhysical(...)+1, ...) would screw up the crew in fair crew mode
		if (Game.Parameters.UseFairCrew) return false;
		// Set physical
		pObj->Info->Physical.*off = iValue;
		return true;
	// Temporary physical
	case PHYS_Temporary:
	case PHYS_StackTemporary:
		// Automatically switch to temporary mode
		if (!pObj->PhysicalTemporary)
		{
			pObj->TemporaryPhysical = *(pObj->GetPhysical());
			pObj->PhysicalTemporary = true;
		}
		// if old value is to be remembered, register the change
		if (iMode == PHYS_StackTemporary)
			pObj->TemporaryPhysical.RegisterChange(off);
		// Set physical
		pObj->TemporaryPhysical.*off = iValue;
		return true;
	}
	// Invalid mode
	return false;
}

static bool FnTrainPhysical(C4String *szPhysical, C4ValueInt iTrainBy, C4ValueInt iMaxTrain, Required<C4ObjectOrThis> pObj)
{
	// Get physical offset
	C4PhysicalInfo::Offset off;
	if (!C4PhysicalInfo::GetOffsetByName(FnStringPar(szPhysical), &off)) return false;
	// train it
	return !!pObj->TrainPhysical(off, iTrainBy, iMaxTrain);
}

static bool FnResetPhysical(Required<C4ObjectOrThis> pObj, C4String *sPhysical)
{
	const char *szPhysical = FnStringPar(sPhysical);

	// Reset to permanent physical
	if (!pObj->PhysicalTemporary) return false;

	// reset specified physical only?
	if (szPhysical && *szPhysical)
	{
		C4PhysicalInfo::Offset off;
		if (!C4PhysicalInfo::GetOffsetByName(szPhysical, &off)) return false;
		if (!pObj->TemporaryPhysical.ResetPhysical(off)) return false;
		// if other physical changes remain, do not reset complete physicals
		if (pObj->TemporaryPhysical.HasChanges(pObj->GetPhysical(true))) return true;
	}

	// actual reset of temp physicals
	pObj->PhysicalTemporary = false;
	pObj->TemporaryPhysical.Default();

	return true;
}

static std::optional<C4ValueInt> FnGetPhysical(C4AulContext *cthr, C4String *szPhysical, C4ValueInt iMode, C4Object *pObj, C4ID idDef)
{
	// Get physical offset
	C4PhysicalInfo::Offset off;
	if (!C4PhysicalInfo::GetOffsetByName(FnStringPar(szPhysical), &off)) return {};
	// no object given?
	if (!pObj)
	{
		// def given?
		if (idDef)
		{
			// get def
			C4Def *pDef = Game.Defs.ID2Def(idDef); if (!pDef) return {};
			// return physical value
			return {pDef->Physical.*off};
		}
		// local call?
		pObj = cthr->Obj; if (!pObj) return {};
	}

	// Get by mode
	switch (iMode)
	{
	// Currently active physical
	case PHYS_Current:
		// Get physical
		return {pObj->GetPhysical()->*off};
	// Permanent physical
	case PHYS_Permanent:
		// Info objects only
		if (!pObj->Info) return {};
		// In fair crew mode, scripts may not read permanent physical values - fallback to fair def physical instead!
		if (Game.Parameters.UseFairCrew)
			if (pObj->Info->pDef)
				return {pObj->Info->pDef->GetFairCrewPhysicals()->*off};
			else
				return {pObj->Def->GetFairCrewPhysicals()->*off};
		// Get physical
		return {pObj->Info->Physical.*off};
	// Temporary physical
	case PHYS_Temporary:
		// Info objects only
		if (!pObj->Info) return {};
		// Only if in temporary mode
		if (!pObj->PhysicalTemporary) return {};
		// Get physical
		return {pObj->TemporaryPhysical.*off};
	}
	// Invalid mode
	return {};
}

static bool FnSetEntrance(bool enabled, Required<C4ObjectOrThis> pObj)
{
	pObj->EntranceStatus = enabled;
	return true;
}

static bool FnSetXDir(C4ValueInt nxdir, Required<C4ObjectOrThis> pObj, Default<C4ValueInt, 10, true> iPrec)
{
	// update xdir
	pObj->xdir = itofix(nxdir, iPrec);
	pObj->Mobile = 1;
	// success
	return true;
}

static bool FnSetRDir(C4ValueInt nrdir, Required<C4ObjectOrThis> pObj, Default<C4ValueInt, 10, true> iPrec)
{
	// update rdir
	pObj->rdir = itofix(nrdir, iPrec);
	pObj->Mobile = 1;
	// success
	return true;
}

static bool FnSetYDir(C4ValueInt nydir, Required<C4ObjectOrThis> pObj, Default<C4ValueInt, 10, true> iPrec)
{
	// update ydir
	pObj->ydir = itofix(nydir, iPrec);
	pObj->Mobile = 1;
	// success
	return true;
}

static bool FnSetR(C4ValueInt nr, Required<C4ObjectOrThis> pObj)
{
	// set rotation
	pObj->SetRotation(nr);
	// success
	return true;
}

static bool FnSetAction(C4AulContext *cthr, C4String &szAction,
	C4Object *pTarget, C4Object *pTarget2, bool fDirect)
{
	if (!cthr->Obj) return false;
	return !!cthr->Obj->SetActionByName(FnStringPar(szAction), pTarget, pTarget2,
		C4Object::SAC_StartCall | C4Object::SAC_AbortCall, !!fDirect);
}

static bool FnSetBridgeActionData(C4ValueInt iBridgeLength, bool fMoveClonk, bool fWall, C4ValueInt iBridgeMaterial, Required<C4ObjectOrThis> pObj)
{
	if (!pObj->Status) return false;
	// action must be BRIDGE
	if (pObj->Action.Act <= ActIdle) return false;
	if (pObj->Def->ActMap[pObj->Action.Act].Procedure != DFA_BRIDGE) return false;
	// set data
	pObj->Action.SetBridgeData(iBridgeLength, fMoveClonk, fWall, iBridgeMaterial);
	return true;
}

static bool FnSetActionData(C4ValueInt iData, Required<C4ObjectOrThis> pObj)
{
	if (!pObj->Status) return false;
	// bridge: Convert from old style
	if ((pObj->Action.Act > ActIdle) && (pObj->Def->ActMap[pObj->Action.Act].Procedure == DFA_BRIDGE))
		return FnSetBridgeActionData(0, false, false, iData, pObj);
	// attach: check for valid vertex indices
	if ((pObj->Action.Act > ActIdle) && (pObj->Def->ActMap[pObj->Action.Act].Procedure == DFA_ATTACH)) // Fixed Action.Act check here... matthes
		if (((iData & 255) >= C4D_MaxVertex) || ((iData >> 8) >= C4D_MaxVertex))
			return false;
	// set data
	pObj->Action.Data = iData;
	return true;
}

static bool FnObjectSetAction(C4Object &obj, C4String &szAction,
	C4Object *pTarget, C4Object *pTarget2, bool fDirect)
{
	// regular action change
	return !!obj.SetActionByName(FnStringPar(szAction), pTarget, pTarget2,
		C4Object::SAC_StartCall | C4Object::SAC_AbortCall, !!fDirect);
}

static bool FnSetComDir(C4ValueInt ncomdir, Required<C4ObjectOrThis> pObj)
{
	pObj->Action.ComDir = ncomdir;
	return true;
}

static bool FnSetDir(C4ValueInt ndir, Required<C4ObjectOrThis> pObj)
{
	pObj->SetDir(ndir);
	return true;
}

static bool FnSetCategory(C4ValueInt iCategory, Required<C4ObjectOrThis> pObj)
{
	if (!(iCategory & C4D_SortLimit)) iCategory |= (pObj->Category & C4D_SortLimit);
	pObj->SetCategory(iCategory);
	return true;
}

static bool FnSetAlive(bool nalv, Required<C4ObjectOrThis> pObj)
{
	pObj->SetAlive(nalv);
	return true;
}

static bool FnSetOwner(C4ValueInt iOwner, Required<C4ObjectOrThis> pObj)
{
	// Set owner
	return !!pObj->SetOwner(iOwner);
}

static bool FnSetPhase(C4ValueInt iVal, Required<C4ObjectOrThis> pObj)
{
	return !!pObj->SetPhase(iVal);
}

static bool FnExecuteCommand(Required<C4ObjectOrThis> pObj)
{
	return !!pObj->ExecuteCommand();
}

static bool FnSetCommand(Required<C4ObjectOrThis> pObj, C4String &szCommand, C4Object *pTarget, C4Value Tx, C4ValueInt iTy, C4Object *pTarget2, C4Value data, C4ValueInt iRetries)
{
	// Command
	C4ValueInt iCommand = CommandByName(FnStringPar(szCommand));
	if (!iCommand)
	{
		pObj->ClearCommands();
		return false;
	}
	// Special: convert iData to szText
	const char *szText = nullptr;
	int32_t iData = 0;
	if (iCommand == C4CMD_Call)
	{
		szText = FnStringPar(data.getStr());
	}
	else
	{
		iData = data.getIntOrID();
		Tx.ConvertTo(C4V_Int);
	}
	// Set
	pObj->SetCommand(iCommand, pTarget, Tx, iTy, pTarget2, false, iData, iRetries, szText);
	// Success
	return true;
}

static bool FnAddCommand(Required<C4ObjectOrThis> pObj, C4String &szCommand, C4Object *pTarget, C4Value Tx, C4ValueInt iTy, C4Object *pTarget2, C4ValueInt iUpdateInterval, C4Value data, C4ValueInt iRetries, C4ValueInt iBaseMode)
{
	// Command
	C4ValueInt iCommand = CommandByName(FnStringPar(szCommand));
	if (!iCommand) return false;
	// Special: convert iData to szText
	const char *szText = nullptr;
	int32_t iData = 0;
	if (iCommand == C4CMD_Call)
	{
		szText = FnStringPar(data.getStr());
	}
	else
	{
		iData = data.getIntOrID();
		Tx.ConvertTo(C4V_Int);
	}
	// Add
	return pObj->AddCommand(iCommand, pTarget, Tx, iTy, iUpdateInterval, pTarget2, true, iData, false, iRetries, szText, iBaseMode);
}

static bool FnAppendCommand(Required<C4ObjectOrThis> pObj, C4String &szCommand, C4Object *pTarget, C4Value Tx, C4ValueInt iTy, C4Object *pTarget2, C4ValueInt iUpdateInterval, C4Value Data, C4ValueInt iRetries, C4ValueInt iBaseMode)
{
	// Command
	C4ValueInt iCommand = CommandByName(FnStringPar(szCommand));
	if (!iCommand) return false;
	// Special: convert iData to szText
	const char *szText = nullptr;
	int32_t iData = 0;
	if (iCommand == C4CMD_Call)
	{
		szText = FnStringPar(Data.getStr());
	}
	else
	{
		iData = Data.getIntOrID();
		Tx.ConvertTo(C4V_Int);
	}
	// Add
	return pObj->AddCommand(iCommand, pTarget, Tx, iTy, iUpdateInterval, pTarget2, true, iData, true, iRetries, szText, iBaseMode);
}

static C4Value FnGetCommand(Required<C4ObjectOrThis> pObj, C4ValueInt iElement, C4ValueInt iCommandNum)
{
	C4Command *Command = pObj->Command;
	// Move through list to Command iCommandNum
	while (Command && iCommandNum--) Command = Command->Next;
	// Object has no command or iCommandNum was to high or < 0
	if (!Command) return C4VNull;
	// Return command element
	switch (iElement)
	{
	case 0: // Name
		return C4VString(CommandName(Command->Command));
	case 1: // Target
		return C4VObj(Command->Target);
	case 2: // Tx
		return Command->Tx;
	case 3: // Ty
		return C4VInt(Command->Ty);
	case 4: // Target2
		return C4VObj(Command->Target2);
	case 5: // Data
		return C4Value(Command->Data, C4V_Any);
	}
	// Undefined element
	return C4VNull;
}

static bool FnFinishCommand(Required<C4ObjectOrThis> pObj, bool fSuccess, C4ValueInt iCommandNum)
{
	C4Command *Command = pObj->Command;
	// Move through list to Command iCommandNum
	while (Command && iCommandNum--) Command = Command->Next;
	// Object has no command or iCommandNum was to high or < 0
	if (!Command) return false;
	if (!fSuccess) ++(Command->Failures);
	else Command->Finished = true;
	return true;
}

static bool FnPlayerObjectCommand(C4AulContext *cthr, C4Player &plr, C4String &szCommand, C4Object *pTarget, C4Value Tx, C4ValueInt iTy, C4Object *pTarget2, C4Value data)
{
	// Command
	C4ValueInt iCommand = CommandByName(FnStringPar(szCommand));
	if (!iCommand) return false;

	std::int32_t iData{0};
	const std::int32_t iTx{Tx.getInt()};
	if (iCommand == C4CMD_Call)
	{
		StrictError(cthr, C4AulScriptStrict::STRICT3, "PlayerObjectCommand: Command \"Call\" not supported");
	}
	else
	{
		iData = data.getIntOrID();
	}
	// Set
	plr.ObjectCommand(iCommand, pTarget, iTx, iTy, pTarget2, iData, C4P_Command_Set);
	// Success
	return true;
}

static C4String *FnGetAction(Required<C4ObjectOrThis> pObj)
{
	if (pObj->Action.Act <= ActIdle) return String("Idle");
	return String(pObj->Def->ActMap[pObj->Action.Act].Name);
}

static C4String *FnGetName(C4ObjectOrThis pObj, C4ID idDef)
{
	// Def name
	C4Def *pDef;
	if (idDef)
	{
		pDef = C4Id2Def(idDef);
		if (pDef) return String(pDef->GetName());
		return nullptr;
	}
	// Object name
	if (!pObj) return nullptr;
	return String(pObj->GetName());
}

static bool FnSetName(C4String *pNewName, C4ObjectOrThis pObj, C4ID idDef, bool fSetInInfo, bool fMakeValidIfExists)
{
	// safety
	if (fSetInInfo && idDef) return false;

	// Def name
	C4Def *pDef;

	if (idDef)
		if (pDef = C4Id2Def(idDef))
			pDef->Name.Copy(FnStringPar(pNewName));
		else
			return false;
	else
	{
		// Object name
		if (!pObj) return false;
		if (fSetInInfo)
		{
			// setting name in info
			C4ObjectInfo *pInfo = pObj->Info;
			if (!pInfo) return false;
			const char *szName = pNewName->Data.getData();
			// empty names are bad; e.g., could cause problems in savegames
			if (!szName || !*szName) return false;
			// name must not be too C4ValueInt
			if (SLen(szName) > C4MaxName) return false;
			// any change at all?
			if (SEqual(szName, pInfo->Name)) return true;
			// make sure names in info list aren't duplicated
			// querying owner info list here isn't 100% accurate, as infos might have been stolen by other players
			// however, there is no good way to track the original list ATM
			C4ObjectInfoList *pInfoList = nullptr;
			C4Player *pOwner = Game.Players.Get(pObj->Owner);
			if (pOwner) pInfoList = &pOwner->CrewInfoList;
			char NameBuf[C4MaxName + 1];
			if (pInfoList) if (pInfoList->NameExists(szName))
			{
				if (!fMakeValidIfExists) return false;
				SCopy(szName, NameBuf, C4MaxName);
				pInfoList->MakeValidName(NameBuf);
				szName = NameBuf;
			}
			SCopy(szName, pInfo->Name, C4MaxName);
			pObj->SetName(); // make sure object uses info name
		}
		else
		{
			if (!pNewName) pObj->SetName();
			else pObj->SetName(pNewName->Data.getData());
		}
	}
	return true;
}

static C4String *FnGetDesc(C4AulContext *cthr, C4Object *pObj, C4ID idDef)
{
	C4Def *pDef;
	// find def
	if (!pObj && !idDef) pObj = cthr->Obj;
	if (pObj)
		pDef = pObj->Def;
	else
		pDef = Game.Defs.ID2Def(idDef);
	// nothing found?
	if (!pDef) return nullptr;
	// return desc
	return String(pDef->GetDesc());
}

static C4String *FnGetPlayerName(C4Player &player)
{
	return String(player.GetName());
}

static C4String *FnGetTaggedPlayerName(C4Player &player)
{
	uint32_t dwClr = player.ColorDw; C4GUI::MakeColorReadableOnBlack(dwClr);
	static char szFnFormatBuf[1024 + 1];
	FormatWithNull(szFnFormatBuf, "<c {:x}>{}</c>", dwClr & 0xffffff, player.GetName());
	return String(szFnFormatBuf);
}

static C4ValueInt FnGetPlayerType(C4Player &player)
{
	return player.GetType();
}

static C4Object *FnGetActionTarget(C4ValueInt target_index, Required<C4ObjectOrThis> pObj)
{
	if (target_index == 0) return pObj->Action.Target;
	if (target_index == 1) return pObj->Action.Target2;
	return nullptr;
}

static bool FnSetActionTargets(C4Object *pTarget1, C4Object *pTarget2, Required<C4ObjectOrThis> pObj)
{
	// set targets
	pObj->Action.Target = pTarget1;
	pObj->Action.Target2 = pTarget2;
	return true;
}

static C4ValueInt FnGetDir(Required<C4ObjectOrThis> pObj)
{
	return pObj->Action.Dir;
}

static C4ValueInt FnGetEntrance(Required<C4ObjectOrThis> pObj)
{
	return pObj->EntranceStatus;
}

static std::optional<C4ValueInt> FnGetPhase(Required<C4ObjectOrThis> pObj)
{
	return {pObj->Action.Phase};
}

static C4ValueInt FnGetEnergy(Required<C4ObjectOrThis> pObj)
{
	return 100 * pObj->Energy / C4MaxPhysical;
}

static C4ValueInt FnGetBreath(Required<C4ObjectOrThis> pObj)
{
	return 100 * pObj->Breath / C4MaxPhysical;
}

static std::optional<C4ValueInt> FnGetMass(C4ObjectOrThis pObj, C4ID idDef)
{
	if (idDef)
	{
		C4Def *pDef = Game.Defs.ID2Def(idDef);
		if (!pDef) return {};
		return pDef->Mass;
	}
	if (!pObj) return {};
	return {pObj->Mass};
}

static C4ValueInt FnGetRDir(Required<C4ObjectOrThis> pObj, Default<C4ValueInt, 10, true> iPrec)
{
	return fixtoi(pObj->rdir, iPrec);
}

static C4ValueInt FnGetXDir(Required<C4ObjectOrThis> pObj, Default<C4ValueInt, 10, true> iPrec)
{
	return fixtoi(pObj->xdir, iPrec);
}

static C4ValueInt FnGetYDir(Required<C4ObjectOrThis> pObj, Default<C4ValueInt, 10, true> iPrec)
{
	return fixtoi(pObj->ydir, iPrec);
}

static C4ValueInt FnGetR(Required<C4ObjectOrThis> pObj)
{
	// Adjust range
	C4ValueInt iR = pObj->r;
	while (iR > 180) iR -= 360;
	while (iR < -180) iR += 360;
	return iR;
}

static C4ValueInt FnGetComDir(Required<C4ObjectOrThis> pObj)
{
	return pObj->Action.ComDir;
}

static C4ValueInt FnGetX(Required<C4ObjectOrThis> pObj)
{
	return pObj->x;
}

static C4ValueInt FnGetVertexNum(Required<C4ObjectOrThis> pObj)
{
	return pObj->Shape.VtxNum;
}

static const C4ValueInt VTX_X = 0, // vertex data indices
                        VTX_Y = 1,
                        VTX_CNAT = 2,
                        VTX_Friction = 3,
                        VTX_SetPermanent = 1,
                        VTX_SetPermanentUpd = 2;

static std::optional<C4ValueInt> FnGetVertex(C4ValueInt iIndex, C4ValueInt iValueToGet, Required<C4ObjectOrThis> pObj)
{
	if (pObj->Shape.VtxNum < 1) return {};
	iIndex = std::min<C4ValueInt>(iIndex, pObj->Shape.VtxNum - 1);
	switch (iValueToGet)
	{
	case VTX_X:        return {pObj->Shape.VtxX[iIndex]}; break;
	case VTX_Y:        return {pObj->Shape.VtxY[iIndex]}; break;
	case VTX_CNAT:     return {pObj->Shape.VtxCNAT[iIndex]}; break;
	case VTX_Friction: return {pObj->Shape.VtxFriction[iIndex]}; break;
	default:
		// old-style behaviour for any value != 0 (normally not used)
		DebugLog("GetVertex: Unknown vertex attribute: {}; getting VtxY", iValueToGet);
		return {pObj->Shape.VtxY[iIndex]};
		break;
	}
	// impossible mayhem!
	return {};
}

static bool FnSetVertex(C4ValueInt iIndex, C4ValueInt iValueToSet, C4ValueInt iValue, Required<C4ObjectOrThis> pObj, C4ValueInt iOwnVertexMode)
{
	// safety
	if (!pObj->Status) return false;
	// own vertex mode?
	if (iOwnVertexMode)
	{
		// enter own custom vertex mode if not already set
		if (!pObj->fOwnVertices)
		{
			pObj->Shape.CreateOwnOriginalCopy(pObj->Def->Shape);
			pObj->fOwnVertices = 1;
		}
		// set vertices at end of buffer
		iIndex += C4D_VertexCpyPos;
	}
	// range check
	if (!Inside<C4ValueInt>(iIndex, 0, C4D_MaxVertex - 1)) return false;
	// set desired value
	switch (iValueToSet)
	{
	case VTX_X: pObj->Shape.VtxX[iIndex] = iValue; break;
	case VTX_Y: pObj->Shape.VtxY[iIndex] = iValue; break;
	case VTX_CNAT: pObj->Shape.VtxCNAT[iIndex] = iValue; break;
	case VTX_Friction: pObj->Shape.VtxFriction[iIndex] = iValue; break;
	default:
		// old-style behaviour for any value != 0 (normally not used)
		pObj->Shape.VtxY[iIndex] = iValue;
		DebugLog("SetVertex: Unknown vertex attribute: {}; setting VtxY", iValueToSet);
		break;
	}
	// vertex update desired?
	if (iOwnVertexMode == VTX_SetPermanentUpd) pObj->UpdateShape(true);
	return true;
}

static bool FnAddVertex(C4ValueInt iX, C4ValueInt iY, Required<C4ObjectOrThis> pObj)
{
	return !!pObj->Shape.AddVertex(iX, iY);
}

static bool FnRemoveVertex(C4ValueInt iIndex, Required<C4ObjectOrThis> pObj)
{
	return !!pObj->Shape.RemoveVertex(iIndex);
}

static bool FnSetContactDensity(C4ValueInt iDensity, Required<C4ObjectOrThis> pObj)
{
	pObj->Shape.ContactDensity = iDensity;
	return true;
}

static C4ValueInt FnGetY(Required<C4ObjectOrThis> pObj)
{
	return pObj->y;
}

static C4ValueInt FnGetAlive(Required<C4ObjectOrThis> pObj)
{
	return pObj->GetAlive();
}

static C4ValueInt FnGetOwner(Required<C4ObjectOrThis, NO_OWNER> pObj)
{
	return pObj->Owner;
}

static C4ValueInt FnCrewMember(Required<C4ObjectOrThis> pObj)
{
	return pObj->Def->CrewMember;
}

static C4ValueInt FnGetController(Required<C4ObjectOrThis, NO_OWNER> pObj)
{
	return pObj->Controller;
}

static bool FnSetController(C4ValueInt iNewController, Required<C4ObjectOrThis> pObj)
{
	// validate player
	if (iNewController != NO_OWNER && !ValidPlr(iNewController)) return false;
	// Set controller
	pObj->Controller = iNewController;
	return true;
}

static C4ValueInt FnGetKiller(Required<C4ObjectOrThis, NO_OWNER> pObj)
{
	return pObj->LastEnergyLossCausePlayer;
}

static bool FnSetKiller(C4ValueInt iNewKiller, Required<C4ObjectOrThis> pObj)
{
	// validate player
	if (iNewKiller != NO_OWNER && !ValidPlr(iNewKiller)) return false;
	// set killer as last energy loss cause
	pObj->LastEnergyLossCausePlayer = iNewKiller;
	return true;
}

static std::optional<C4ValueInt> FnGetCategory(C4ObjectOrThis pObj, C4ID idDef)
{
	// Def category
	C4Def *pDef;
	if (idDef) if (pDef = C4Id2Def(idDef)) return {pDef->Category};
	// Object category
	if (!pObj) return {};
	return {pObj->Category};
}

static C4ValueInt FnGetOCF(Required<C4ObjectOrThis> pObj)
{
	return pObj->OCF;
}

static C4ValueInt FnGetDamage(Required<C4ObjectOrThis> pObj)
{
	return pObj->Damage;
}

static std::optional<C4ValueInt> FnGetValue(C4ObjectOrThis pObj, C4ID idDef, C4Object *pInBase, C4ValueInt iForPlayer)
{
	// Def value
	C4Def *pDef;
	if (idDef)
		// return Def value or 0 if def unloaded
		if (pDef = C4Id2Def(idDef)) return pDef->GetValue(pInBase, iForPlayer); else return {};
	// Object value
	if (!pObj) return {};
	return {pObj->GetValue(pInBase, iForPlayer)};
}

static std::optional<C4ValueInt> FnGetRank(Required<C4ObjectOrThis> pObj)
{
	if (!pObj->Info) return {};
	return {pObj->Info->Rank};
}

static std::optional<C4ValueInt> FnValue(C4ID id)
{
	C4Def *pDef = C4Id2Def(id);
	if (pDef) return {pDef->Value};
	return {};
}

static C4ValueInt FnGetActTime(Required<C4ObjectOrThis> pObj)
{
	return pObj->Action.Time;
}

static C4ID FnGetID(C4AulContext *cthr, C4Object *pObj)
{
	C4Def *pDef = pObj ? pObj->Def : cthr->Def;
	if (!pDef) return 0;
	// return id of object
	return pDef->id;
}

static C4ValueInt FnGetBase(Required<C4ObjectOrThis, NO_OWNER> pObj)
{
	return pObj->Base;
}

static C4ID FnGetMenu(Required<C4ObjectOrThis, static_cast<C4ID>(-1)> pObj)
{
	if (pObj->Menu && pObj->Menu->IsActive())
		return pObj->Menu->GetIdentification();
	return C4MN_None;
}

static bool FnCreateMenu(C4ID iSymbol, Required<C4ObjectOrThis> pMenuObj, C4ObjectOrThis pCommandObj,
	C4ValueInt iExtra, C4String *szCaption, C4ValueInt iExtraData,
	C4ValueInt iStyle, bool fPermanent, C4ID idMenuID)
{
	if (pCommandObj)
	{
		// object menu: Validate object
		if (!pCommandObj->Status) return false;
	}
	else
	{
		// scenario script callback: No command object OK
	}

	// Create symbol
	C4Def *pDef;
	C4FacetExSurface fctSymbol;
	fctSymbol.Create(C4SymbolSize, C4SymbolSize);
	if (pDef = C4Id2Def(iSymbol)) pDef->Draw(fctSymbol);

	// Clear any old menu, init new menu
	if (!pMenuObj->CloseMenu(false)) return false;
	if (!pMenuObj->Menu) pMenuObj->Menu = new C4ObjectMenu; else pMenuObj->Menu->ClearItems(true);
	pMenuObj->Menu->Init(fctSymbol, FnStringPar(szCaption), pCommandObj, iExtra, iExtraData, idMenuID ? idMenuID : iSymbol, iStyle, true);

	// Set permanent
	pMenuObj->Menu->SetPermanent(fPermanent);

	return true;
}

const C4ValueInt C4MN_Add_ImgRank     = 1,
                 C4MN_Add_ImgIndexed  = 2,
                 C4MN_Add_ImgObjRank  = 3,
                 C4MN_Add_ImgObject   = 4,
                 C4MN_Add_ImgTextSpec = 5,
                 C4MN_Add_ImgColor    = 6,
                 C4MN_Add_ImgIndexedColor = 7,
                 C4MN_Add_MaxImage    = 127, // mask for param which decides what to draw as the menu symbol
                 C4MN_Add_PassValue   = 128,
                 C4MN_Add_ForceCount  = 256,
                 C4MN_Add_ForceNoDesc = 512;

static bool FnAddMenuItem(C4AulContext *cthr, C4String *szCaption, C4String *szCommand, C4ID idItem, Required<C4ObjectOrThis> pMenuObj, C4ValueInt iCount, C4Value Parameter, C4String *szInfoCaption, C4ValueInt iExtra, C4Value XPar, C4Value XPar2)
{
	if (!pMenuObj->Menu) return false;

	char caption[256 + 1];
	char parameter[256 + 1];
	char dummy[256 + 1];
	std::string command;
	std::string command2;
	char infocaption[C4MaxTitle + 1];

	// get needed symbol size
	const auto iSymbolSize = pMenuObj->Menu->GetSymbolSize();

	// Check specified def
	C4Def *pDef = C4Id2Def(idItem);
	if (!pDef) pDef = pMenuObj->Def;

	// Compose caption with def name
	if (szCaption)
	{
		const char *s = FnStringPar(szCaption);
		const char *sep = strstr(s, "%s");
		if (sep)
		{
			strncpy(caption, s, std::min<intptr_t>(sep - s, 256));
			caption[std::min<intptr_t>(sep - s, 256)] = 0;
			strncat(caption, pDef->GetName(), 256);
			strncat(caption, sep + 2, 256);
		}
		else
		{
			strncpy(caption, s, 256);
			caption[256] = 0;
		}
	}
	else
		caption[0] = 0;

	// create string to include type-information in command
	switch (Parameter.GetType())
	{
	case C4V_Int:
		FormatWithNull(parameter, "{}", Parameter.getInt());
		break;
	case C4V_Bool:
		SCopy(Parameter.getBool() ? "true" : "false", parameter);
		break;
	case C4V_C4ID:
		FormatWithNull(parameter, "{}", C4IdText(Parameter.getC4ID()));
		break;
	case C4V_C4Object:
		FormatWithNull(parameter, "Object({})", Parameter.getObj()->Number);
		break;
	case C4V_String:
		// note this breaks if there is '"' in the string.
		parameter[0] = '"';
		SCopy(Parameter.getStr()->Data.getData(), parameter + 1, sizeof(parameter) - 3);
		SAppendChar('"', parameter);
		break;
	case C4V_Any:
		FormatWithNull(parameter, "CastAny({})", Parameter._getRaw());
		break;
	case C4V_Array:
		// Arrays were never allowed, so tell the scripter
		throw C4AulExecError(cthr->Obj, "array as parameter to AddMenuItem");
	case C4V_Map:
		// Maps are not allowed either
		throw C4AulExecError(cthr->Obj, "map as parameter to AddMenuItem");
	default:
		return false;
	}

	// own value
	bool fOwnValue = false; C4ValueInt iValue = 0;
	if (iExtra & C4MN_Add_PassValue)
	{
		fOwnValue = true;
		iValue = XPar2.getInt();
	}

	// New Style: native script command
	int i = 0;
	for (; i < SLen(FnStringPar(szCommand)); i++)
		if (!IsIdentifier(FnStringPar(szCommand)[i]))
			break;
	if (i < SLen(FnStringPar(szCommand)))
	{
		// Search for "%d" an replace it by "%s" for insertion of formatted parameter
		SCopy(FnStringPar(szCommand), dummy, 256);
		char *pFound = const_cast<char *>(SSearch(dummy, "%d"));
		if (pFound != nullptr)
			*(pFound - 1) = 's';
		// Compose left-click command
		command = fmt::sprintf(dummy, parameter, 0);
		command2 = fmt::sprintf(dummy, parameter, 1);
	}

	// Old style: function name with id and parameter
	else
	{
		const char *szScriptCom = FnStringPar(szCommand);
		if (szScriptCom && *szScriptCom)
		{
			if (iExtra & C4MN_Add_PassValue)
			{
				// with value
				command = std::format("{}({},{},0,{})", szScriptCom, C4IdText(idItem), +parameter, iValue);
				command2 = std::format("{}({},{},1,{})", szScriptCom, C4IdText(idItem), +parameter, iValue);
			}
			else
			{
				// without value
				command = std::format("{}({},{})", szScriptCom, C4IdText(idItem), +parameter);
				command2 = std::format("{}({},{},1)", szScriptCom, C4IdText(idItem), +parameter);
			}
		}
		else
		{
			// no command
		}
	}

	// Info caption
	SCopy(FnStringPar(szInfoCaption), infocaption, C4MaxTitle);
	// Default info caption by def desc
	if (!infocaption[0] && !(iExtra & C4MN_Add_ForceNoDesc)) SCopy(pDef->GetDesc(), infocaption, C4MaxTitle);

	// Create symbol
	C4FacetExSurface fctSymbol;
	switch (iExtra & C4MN_Add_MaxImage)
	{
	case C4MN_Add_ImgRank:
	{
		// symbol by rank
		C4FacetEx *pfctRankSym = &Game.GraphicsResource.fctRank;
		int32_t iRankSymNum = Game.GraphicsResource.iNumRanks;
		if (pDef && pDef->pRankSymbols)
		{
			pfctRankSym = pDef->pRankSymbols;
			iRankSymNum = pDef->iNumRankSymbols;
		}
		C4RankSystem::DrawRankSymbol(&fctSymbol, iCount, pfctRankSym, iRankSymNum, true);
		iCount = 0;
		break;
	}
	case C4MN_Add_ImgIndexed:
		// use indexed facet
		pDef->Picture2Facet(fctSymbol, 0, XPar.getInt());
		break;
	case C4MN_Add_ImgObjRank:
	{
		// draw current gfx of XPar_C4V including rank
		if (XPar.GetType() != C4V_C4Object) return false;
		C4Object *pGfxObj = XPar.getObj();
		if (pGfxObj && pGfxObj->Status)
		{
			// create graphics
			// get rank gfx
			C4FacetEx *pRankRes = &Game.GraphicsResource.fctRank;
			C4ValueInt iRankCnt = Game.GraphicsResource.iNumRanks;
			C4Def *pDef = pGfxObj->Def;
			if (pDef->pRankSymbols)
			{
				pRankRes = pDef->pRankSymbols;
				iRankCnt = pDef->iNumRankSymbols;
			}
			// context menu
			C4Facet fctRank;
			if (pMenuObj->Menu->IsContextMenu())
			{
				// context menu entry: left object gfx
				C4ValueInt C4MN_SymbolSize = pMenuObj->Menu->GetItemHeight();
				fctSymbol.Create(C4MN_SymbolSize * 2, C4MN_SymbolSize);
				fctSymbol.Wdt = C4MN_SymbolSize;
				pGfxObj->Def->Draw(fctSymbol, false, pGfxObj->Color, pGfxObj);
				// right of it the rank
				fctRank = fctSymbol;
				fctRank.X = C4MN_SymbolSize;
				fctSymbol.Wdt *= 2;
			}
			else
			{
				// regular menu: draw object picture
				fctSymbol.Create(iSymbolSize, iSymbolSize);
				pGfxObj->Def->Draw(fctSymbol, false, pGfxObj->Color, pGfxObj);
				// rank at top-right corner
				fctRank = fctSymbol;
				fctRank.X = fctRank.Wdt - pRankRes->Wdt;
				fctRank.Wdt = pRankRes->Wdt;
				fctRank.Hgt = pRankRes->Hgt;
			}
			// draw rank
			if (pGfxObj->Info)
			{
				C4Facet fctBackup = static_cast<const C4Facet &>(fctSymbol);
				fctSymbol.Set(fctRank);
				C4RankSystem::DrawRankSymbol(&fctSymbol, pGfxObj->Info->Rank, pRankRes, iRankCnt, true);
				fctSymbol.Set(fctBackup);
			}
		}
	}
	break;
	case C4MN_Add_ImgObject:
	{
		// draw object picture
		if (XPar.GetType() != C4V_C4Object) return false;
		C4Object *pGfxObj = XPar.getObj();
		fctSymbol.Wdt = fctSymbol.Hgt = iSymbolSize;
		pGfxObj->Picture2Facet(fctSymbol);
	}
	break;

	case C4MN_Add_ImgTextSpec:
	{
		C4FacetExSurface fctSymSpec;
		uint32_t dwClr = XPar.getInt();
		if (!szCaption || !Game.DrawTextSpecImage(fctSymSpec, caption, dwClr ? dwClr : 0xff))
			return false;
		fctSymbol.Create(iSymbolSize, iSymbolSize);
		fctSymSpec.Draw(fctSymbol, true);
		*caption = '\0';
	}
	break;

	case C4MN_Add_ImgColor:
		// set colored def facet
		pDef->Picture2Facet(fctSymbol, XPar.getInt());
		break;

	case C4MN_Add_ImgIndexedColor:
		if (iExtra & C4MN_Add_PassValue)
		{
			throw C4AulExecError{cthr->Obj, "AddMenuItem: C4MN_Add_ImgIndexedColor can not be used together with C4MN_Add_PassValue!"};
		}
		pDef->Picture2Facet(fctSymbol, XPar2.getInt(), XPar.getInt());
		break;

	default:
		// default: by def, if it is not specifically NONE
		if (idItem != C4Id("NONE"))
		{
			pDef->Picture2Facet(fctSymbol);
		}
		else
		{
			// otherwise: Clear symbol!
		}
		break;
	}

	// Convert default zero count to no count
	if (iCount == 0 && !(iExtra & C4MN_Add_ForceCount)) iCount = C4MN_Item_NoCount;

	// menuitems without commands are never selectable
	bool fIsSelectable = !command.empty();

	// Add menu item
	pMenuObj->Menu->Add(caption, fctSymbol, command.c_str(), iCount, nullptr, infocaption, idItem, command2.c_str(), fOwnValue, iValue, fIsSelectable);

	return true;
}

static bool FnSelectMenuItem(C4ValueInt iItem, Required<C4ObjectOrThis> pMenuObj)
{
	if (!pMenuObj->Menu) return false;
	return !!pMenuObj->Menu->SetSelection(iItem, false, true);
}

static bool FnSetMenuDecoration(C4ID idNewDeco, C4Object &menuObj)
{
	if (!menuObj.Menu) return false;
	C4GUI::FrameDecoration *pNewDeco = new C4GUI::FrameDecoration();
	if (!pNewDeco->SetByDef(idNewDeco))
	{
		delete pNewDeco;
		return false;
	}
	menuObj.Menu->SetFrameDeco(pNewDeco);
	return true;
}

static bool FnSetMenuTextProgress(C4ValueInt iNewProgress, C4Object &menuObj)
{
	if (!menuObj.Menu) return false;
	return menuObj.Menu->SetTextProgress(iNewProgress, false);
}

// Check / Status

static C4Object *FnContained(Required<C4ObjectOrThis> pObj)
{
	return pObj->Contained;
}

static C4Object *FnContents(C4ValueInt index, Required<C4ObjectOrThis> pObj, bool returnAttached)
{
	// Special: objects attaching to another object
	//          cannot be accessed by FnContents, unless returnAttached is true
	C4Object *cobj;
	while (cobj = pObj->Contents.GetObject(index))
	{
		if (cobj->GetProcedure() != DFA_ATTACH || returnAttached) return cobj;
		index++;
	}
	return nullptr;
}

static bool FnShiftContents(Required<C4ObjectOrThis> pObj, bool fShiftBack, C4ID idTarget, bool fDoCalls)
{
	// regular shift
	if (!idTarget) return !!pObj->ShiftContents(fShiftBack, fDoCalls);
	// check if ID is present within target
	C4Object *pNewFront = pObj->Contents.Find(idTarget);
	if (!pNewFront) return false;
	// select it
	pObj->DirectComContents(pNewFront, fDoCalls);
	// done, success
	return true;
}

static C4Object *FnScrollContents(Required<C4ObjectOrThis> pObj)
{
	C4ObjectOrThis pMove = pObj->Contents.GetObject();
	if (pMove)
	{
		pObj->Contents.Remove(pMove);
		pObj->Contents.Add(pMove, C4ObjectList::stNone);
	}

	return pObj->Contents.GetObject();
}

static C4ValueInt FnContentsCount(C4ID id, Required<C4ObjectOrThis> pObj)
{
	return pObj->Contents.ObjectCount(id);
}

static C4Object *FnFindContents(C4ID c_id, Required<C4ObjectOrThis> pObj)
{
	return pObj->Contents.Find(c_id);
}

static C4Object *FnFindOtherContents(C4ID c_id, Required<C4ObjectOrThis> pObj)
{
	return pObj->Contents.FindOther(c_id);
}

static bool FnActIdle(Required<C4ObjectOrThis> pObj)
{
	return pObj->Action.Act == ActIdle;
}

static bool FnCheckEnergyNeedChain(Required<C4ObjectOrThis> pObj)
{
	C4ObjectList EnergyChainChecked;
	return CheckEnergyNeedChain(pObj, EnergyChainChecked);
}

static bool FnEnergyCheck(C4ValueInt energy, Required<C4ObjectOrThis> pObj)
{
	if (!(Game.Rules & C4RULE_StructuresNeedEnergy)
		|| (pObj->Energy >= energy)
		|| !(pObj->Def->LineConnect & C4D_Power_Consumer))
	{
		pObj->NeedEnergy = 0; return true;
	}
	pObj->NeedEnergy = 1;
	return false;
}

static bool FnStuck(Required<C4ObjectOrThis> pObj)
{
	return !!pObj->Shape.CheckContact(pObj->x, pObj->y);
}

static bool FnInLiquid(Required<C4ObjectOrThis> pObj)
{
	return pObj->InLiquid;
}

static bool FnOnFire(Required<C4ObjectOrThis> pObj)
{
	if (pObj->GetOnFire()) return true;
	// check for effect
	if (!pObj->pEffects) return false;
	return !!pObj->pEffects->Get(C4Fx_AnyFire);
}

static bool FnComponentAll(C4AulContext *cthr, C4Object &obj, C4ID c_id)
{
	C4ValueInt cnt;
	C4IDList Components;
	obj.Def->GetComponents(&Components, &obj, cthr->Obj);
	for (cnt = 0; Components.GetID(cnt); cnt++)
		if (Components.GetID(cnt) != c_id)
			if (Components.GetCount(cnt) > 0)
				return false;
	return true;
}

static C4Object *FnCreateObject(C4AulContext *cthr,
	C4ID id, C4ValueInt iXOffset, C4ValueInt iYOffset, std::optional<C4ValueInt> iOwner)
{
	const auto obj = cthr->Obj;
	const auto strictness = cthr->Caller ? cthr->Caller->Func->pOrgScript->Strict : C4AulScriptStrict::NONSTRICT;
	const auto fallbackOwner = (strictness >= C4AulScriptStrict::STRICT3 ? (obj ? obj->Owner : NO_OWNER) : 0);
	auto owner = iOwner.value_or(fallbackOwner);
	if (obj) // Local object calls override
	{
		MakePositionRelative(obj, iXOffset, iYOffset);
		if (strictness == C4AulScriptStrict::NONSTRICT)
			owner = obj->Owner;
	}

	C4Object *pNewObj = Game.CreateObject(id, obj, owner, iXOffset, iYOffset);

	// Set initial controller to creating controller, so more complicated cause-effect-chains can be traced back to the causing player
	if (pNewObj && obj && obj->Controller > NO_OWNER) pNewObj->Controller = obj->Controller;

	return pNewObj;
}

static C4Object *FnCreateConstruction(C4AulContext *cthr,
	C4ID id, C4ValueInt iXOffset, C4ValueInt iYOffset, C4ValueInt iOwner,
	C4ValueInt iCompletion, bool fTerrain, bool fCheckSite)
{
	// Local object calls override position offset, owner
	const auto obj = cthr->Obj;
	if (obj)
	{
		MakePositionRelative(obj, iXOffset, iYOffset);
		if (!cthr->Caller || cthr->Caller->Func->Owner->Strict == C4AulScriptStrict::NONSTRICT)
			iOwner = obj->Owner;
	}

	// Check site
	if (fCheckSite)
		if (!ConstructionCheck(id, iXOffset, iYOffset, obj))
			return nullptr;

	// Create site object
	C4Object *pNewObj = Game.CreateObjectConstruction(id, obj, iOwner, iXOffset, iYOffset, iCompletion * FullCon / 100, fTerrain);

	// Set initial controller to creating controller, so more complicated cause-effect-chains can be traced back to the causing player
	if (pNewObj && obj && obj->Controller > NO_OWNER) pNewObj->Controller = obj->Controller;

	return pNewObj;
}

static C4Object *FnCreateContents(C4ID c_id, Required<C4ObjectOrThis> pObj, Default<C4ValueInt, 1, true> iCount)
{
	// create objects
	C4Object *pNewObj = nullptr;
	while (iCount-- > 0) pNewObj = pObj->CreateContents(c_id);
	// controller will automatically be set upon entrance
	// return last created
	return pNewObj;
}

static C4Object *FnComposeContents(C4ID c_id, Required<C4ObjectOrThis> pObj)
{
	return pObj->ComposeContents(c_id);
}

static std::optional<bool> FnFindConstructionSite(C4AulContext *cthr, C4ID id, C4ValueInt iVarX, C4ValueInt iVarY)
{
	// Get def (Old-style implementation (fixed)...)
	C4Def *pDef;
	if (!(pDef = C4Id2Def(id))) return {};
	// Var indices out of range
	if (!Inside<C4ValueInt>(iVarX, 0, C4AUL_MAX_Par - 1) || !Inside<C4ValueInt>(iVarY, 0, C4AUL_MAX_Par - 1)) return {};
	// Get thread vars
	if (!cthr->Caller) return {};
	C4Value &V1 = cthr->Caller->NumVars[iVarX];
	C4Value &V2 = cthr->Caller->NumVars[iVarY];
	// Construction check at starting position
	if (ConstructionCheck(id, V1.getInt(), V2.getInt()))
		return {true};
	// Search for real
	int32_t v1 = V1.getInt(), v2 = V2.getInt();
	bool result = !!FindConSiteSpot(v1, v2,
		pDef->Shape.Wdt, pDef->Shape.Hgt,
		pDef->Category,
		20);
	V1 = C4VInt(v1); V2 = C4VInt(v2);
	return {result};
}

static C4Object *FnFindBase(C4ValueInt iOwner, C4ValueInt iIndex)
{
	if (!ValidPlr(iOwner)) return nullptr;
	return Game.FindBase(iOwner, iIndex);
}

std::unique_ptr<C4FindObject> CreateCriterionsFromPars(const std::string_view forFunction, C4Object *const contextObj, const std::span<const C4Value> pPars, bool acceptSort = false)
{
	std::vector<std::unique_ptr<C4FindObject>> conds;
	std::vector<std::unique_ptr<C4SortObject>> sorts;

	// Read all parameters
	for (int i = 0; i < C4AUL_MAX_Par; i++)
	{
		const C4Value &Data = pPars[i].GetRefVal();
		// No data given?
		if (!Data) break;
		// Construct
		auto cond = C4FindObject::CreateByValue(Data, acceptSort ? &sorts : nullptr);
		if (cond)
		{
			conds.emplace_back(std::move(cond));
		}
	}
	// No criterions?
	if (conds.empty())
	{
		throw C4AulExecError{contextObj, std::format("{}: No valid search criterions supplied!", forFunction)};
	}
	// Create search object
	std::unique_ptr<C4FindObject> pFO;
	if (conds.size() == 1)
		pFO = std::move(conds.front());
	else
		pFO = std::make_unique<C4FindObjectAnd>(std::move(conds));


	// create sort criterion
	if (acceptSort && !sorts.empty())
	{
		if (sorts.size() == 1)
			pFO->SetSort(std::move(sorts.front()));
		else
			pFO->SetSort(std::make_unique<C4SortObjectMultiple>(std::move(sorts)));
	}

	return pFO;
}

static C4Value FnObjectCount2(C4AulContext *cthr, std::span<const C4Value> pPars)
{
	const auto pFO = CreateCriterionsFromPars("ObjectCount2", cthr->Obj, pPars);
	return C4VInt(pFO->Count(Game.Objects, Game.Objects.Sectors));
}

static C4Value FnFindObject2(C4AulContext *cthr, std::span<const C4Value> pPars)
{
	const auto pFO = CreateCriterionsFromPars("FindObject2", cthr->Obj, pPars, true);
	return C4VObj(pFO->Find(Game.Objects, Game.Objects.Sectors));
}

static C4Value FnFindObjects(C4AulContext *cthr, std::span<const C4Value> pPars)
{
	const auto pFO = CreateCriterionsFromPars("FindObjects", cthr->Obj, pPars, true);
	return C4VArray(pFO->FindMany(Game.Objects, Game.Objects.Sectors));
}

static C4ValueInt FnObjectCount(C4AulContext *cthr, C4ID id, C4ValueInt x, C4ValueInt y, C4ValueInt wdt, C4ValueInt hgt, C4ValueInt dwOCF, C4String *szAction, C4Object *pActionTarget, C4Value vContainer, C4ValueInt iOwner)
{
	MakePositionRelative(cthr, x, y, (x || y || wdt || hgt));
	// Adjust default ocf
	if (dwOCF == 0) dwOCF = OCF_All;
	// Adjust default owner
	if (iOwner == 0) iOwner = ANY_OWNER; // imcomplete useless implementation
	// NO_CONTAINER/ANY_CONTAINER
	C4Object *pContainer = vContainer.getObj();
	if (vContainer.getInt() == NO_CONTAINER)
		pContainer = reinterpret_cast<C4Object *>(NO_CONTAINER);
	if (vContainer.getInt() == ANY_CONTAINER)
		pContainer = reinterpret_cast<C4Object *>(ANY_CONTAINER);
	// Find object
	return Game.ObjectCount(id, x, y, wdt, hgt, dwOCF,
		FnStringPar(szAction), pActionTarget,
		cthr->Obj, // Local calls exclude self
		pContainer,
		iOwner);
}

static C4Object *FnFindObject(C4AulContext *cthr, C4ID id, C4ValueInt x, C4ValueInt y, C4ValueInt wdt, C4ValueInt hgt, C4ValueInt dwOCF, C4String *szAction, C4Object *pActionTarget, C4Value vContainer, C4Object *pFindNext)
{
	MakePositionRelative(cthr, x, y, (x || y || wdt || hgt));
	// Adjust default ocf
	if (dwOCF == 0) dwOCF = OCF_All;
	// NO_CONTAINER/ANY_CONTAINER
	C4Object *pContainer = vContainer.getObj();
	if (vContainer.getInt() == NO_CONTAINER)
		pContainer = reinterpret_cast<C4Object *>(NO_CONTAINER);
	if (vContainer.getInt() == ANY_CONTAINER)
		pContainer = reinterpret_cast<C4Object *>(ANY_CONTAINER);
	// Find object
	return Game.FindObject(id, x, y, wdt, hgt, dwOCF,
		FnStringPar(szAction), pActionTarget,
		cthr->Obj, // Local calls exclude self
		pContainer,
		ANY_OWNER,
		pFindNext);
}

static C4Object *FnFindObjectOwner(C4AulContext *cthr,
	C4ID id,
	C4ValueInt iOwner,
	C4ValueInt x, C4ValueInt y, C4ValueInt wdt, C4ValueInt hgt,
	C4ValueInt dwOCF,
	C4String *szAction, C4Object *pActionTarget,
	C4Object *pFindNext)
{
	// invalid owner?
	if (!ValidPlr(iOwner) && iOwner != NO_OWNER) return nullptr;
	MakePositionRelative(cthr, x, y, (x || y || wdt || hgt));
	// Adjust default ocf
	if (dwOCF == 0) dwOCF = OCF_All;
	// Find object
	return Game.FindObject(id, x, y, wdt, hgt, dwOCF,
		FnStringPar(szAction), pActionTarget,
		cthr->Obj, // Local calls exclude self
		nullptr,
		iOwner,
		pFindNext);
}

static bool FnMakeCrewMember(C4Object *pObj, C4Player &player)
{
	return player.MakeCrewMember(pObj);
}

static bool FnGrabObjectInfo(C4Object &from, Required<C4ObjectOrThis> pTo)
{
	// grab info
	return !!pTo->GrabInfo(&from);
}

static bool FnFlameConsumeMaterial(C4AulContext *cthr, C4ValueInt x, C4ValueInt y)
{
	MakePositionRelative(cthr, x, y);
	C4ValueInt mat = GBackMat(x, y);
	if (!MatValid(mat)) return false;
	if (!Game.Material.Map[mat].Inflammable) return false;
	if (Game.Landscape.ExtractMaterial(x, y) == MNone) return false;
	return true;
}

static void FnSmoke(C4AulContext *cthr, C4ValueInt tx, C4ValueInt ty, C4ValueInt level, C4ValueInt dwClr)
{
	MakePositionRelative(cthr, tx, ty);
	Smoke(tx, ty, level, dwClr);
}

static void FnBubble(C4AulContext *cthr, C4ValueInt tx, C4ValueInt ty)
{
	MakePositionRelative(cthr, tx, ty);
	BubbleOut(tx, ty);
}

static C4ValueInt FnExtractLiquid(C4AulContext *cthr, C4ValueInt x, C4ValueInt y)
{
	MakePositionRelative(cthr, x, y);
	if (!GBackLiquid(x, y)) return MNone;
	return Game.Landscape.ExtractMaterial(x, y);
}

static bool FnInsertMaterial(C4AulContext *cthr, C4ValueInt mat, C4ValueInt x, C4ValueInt y, C4ValueInt vx, C4ValueInt vy)
{
	MakePositionRelative(cthr, x, y);
	return !!Game.Landscape.InsertMaterial(mat, x, y, vx, vy);
}

static C4ValueInt FnGetMaterialCount(C4ValueInt iMaterial, bool fReal)
{
	if (!MatValid(iMaterial)) return -1;
	if (fReal || !Game.Material.Map[iMaterial].MinHeightCount)
		return Game.Landscape.MatCount[iMaterial];
	else
		return Game.Landscape.EffectiveMatCount[iMaterial];
}

static C4ValueInt FnGetMaterial(C4AulContext *cthr, C4ValueInt x, C4ValueInt y)
{
	MakePositionRelative(cthr, x, y);
	return GBackMat(x, y);
}

static C4String *FnGetTexture(C4ValueInt x, C4ValueInt y)
{
	// Get texture
	int32_t iTex = PixCol2Tex(GBackPix(x, y));
	if (!iTex) return nullptr;
	// Get material-texture mapping
	const C4TexMapEntry *pTex = Game.TextureMap.GetEntry(iTex);
	if (!pTex) return nullptr;
	// Return tex name
	return String(pTex->GetTextureName());
}

static bool FnGBackSolid(C4AulContext *cthr, C4ValueInt x, C4ValueInt y)
{
	MakePositionRelative(cthr, x, y);
	return GBackSolid(x, y);
}

static bool FnGBackSemiSolid(C4AulContext *cthr, C4ValueInt x, C4ValueInt y)
{
	MakePositionRelative(cthr, x, y);
	return GBackSemiSolid(x, y);
}

static bool FnGBackLiquid(C4AulContext *cthr, C4ValueInt x, C4ValueInt y)
{
	MakePositionRelative(cthr, x, y);
	return GBackLiquid(x, y);
}

static bool FnGBackSky(C4AulContext *cthr, C4ValueInt x, C4ValueInt y)
{
	MakePositionRelative(cthr, x, y);
	return !GBackIFT(x, y);
}

static C4ValueInt FnExtractMaterialAmount(C4AulContext *cthr, C4ValueInt x, C4ValueInt y, C4ValueInt mat, C4ValueInt amount)
{
	MakePositionRelative(cthr, x, y);
	C4ValueInt extracted = 0; for (; extracted < amount; extracted++)
	{
		if (GBackMat(x, y) != mat) return extracted;
		if (Game.Landscape.ExtractMaterial(x, y) != mat) return extracted;
	}
	return extracted;
}

static void FnBlastObjects(C4AulContext *cthr, C4ValueInt iX, C4ValueInt iY, C4ValueInt iLevel, C4Object *pInObj, C4ValueInt iCausedByPlusOne)
{
	C4ValueInt iCausedBy = iCausedByPlusOne - 1; if (!iCausedByPlusOne && cthr->Obj) iCausedBy = cthr->Obj->Controller;
	Game.BlastObjects(iX, iY, iLevel, pInObj, iCausedBy, cthr->Obj);
}

static bool FnBlastObject(C4AulContext *cthr, C4ValueInt iLevel, Required<C4ObjectOrThis> pObj, C4ValueInt iCausedByPlusOne)
{
	C4ValueInt iCausedBy = iCausedByPlusOne - 1; if (!iCausedByPlusOne && cthr->Obj) iCausedBy = cthr->Obj->Controller;
	if (!pObj->Status) return false;
	pObj->Blast(iLevel, iCausedBy);
	return true;
}

static void FnBlastFree(C4AulContext *cthr, C4ValueInt iX, C4ValueInt iY, C4ValueInt iLevel, C4ValueInt iCausedByPlusOne)
{
	C4ValueInt iCausedBy = iCausedByPlusOne - 1;
	if (!iCausedByPlusOne)
	{
		if (const auto obj = cthr->Obj; obj)
		{
			iCausedBy = obj->Controller;
			MakePositionRelative(obj, iX, iY);
		}
	}
	C4ValueInt grade = BoundBy<C4ValueInt>((iLevel / 10) - 1, 1, 3);
	Game.Landscape.BlastFree(iX, iY, iLevel, grade, iCausedBy);
}

static bool FnSound(C4AulContext *cthr, C4String *szSound, bool fGlobal, C4Object *pObj, Default<C4ValueInt, 100, true> iLevel, C4ValueInt iAtPlayer, C4ValueInt iLoop, bool fMultiple, C4ValueInt iCustomFalloffDistance)
{
	// play here?
	if (iAtPlayer)
	{
		// get player to play at
		C4Player *pPlr = Game.Players.Get(iAtPlayer - 1);
		// not existent? fail
		if (!pPlr) return false;
		// network client: don't play here
		// return true for network sync
		if (!pPlr->LocalControl && !Game.GraphicsSystem.GetViewport(iAtPlayer - 1)) return true;
	}
	// even less than nothing?
	if (iLevel < 0) return true;
	// default sound level
	if (iLevel > 100)
		iLevel = 100;
	// target object
	if (fGlobal) pObj = nullptr; else if (!pObj) pObj = cthr->Obj;
	// already playing?
	if (iLoop >= 0 && !fMultiple && IsSoundPlaying(FnStringPar(szSound), pObj))
		return true;
	// try to play effect
	if (iLoop >= 0)
		StartSoundEffect(FnStringPar(szSound), !!iLoop, iLevel, pObj, iCustomFalloffDistance);
	else
		StopSoundEffect(FnStringPar(szSound), pObj);
	// always return true (network safety!)
	return true;
}

static void FnMusic(C4String *szSongname, bool fLoop)
{
	if (!szSongname)
	{
		Game.IsMusicEnabled = false;
		Application.MusicSystem->Stop();
	}
	else
	{
		Game.IsMusicEnabled = true;
		Application.MusicSystem->Play(FnStringPar(szSongname), fLoop);
	}
}

static C4ValueInt FnMusicLevel(C4ValueInt iLevel)
{
	Game.SetMusicLevel(iLevel);
	return Game.iMusicLevel;
}

static std::optional<C4ValueInt> FnSetPlayList(C4String *szPlayList, bool fRestartMusic)
{
	C4ValueInt iFilesInPlayList = Application.MusicSystem->SetPlayList(FnStringPar(szPlayList));
	Game.PlayList.Copy(FnStringPar(szPlayList));
	if (fRestartMusic) Application.MusicSystem->Play();
	if (Game.Control.SyncMode()) return {};
	return {iFilesInPlayList};
}

static void FnSoundLevel(C4String *szSound, C4ValueInt iLevel, C4Object *pObj)
{
	SoundLevel(FnStringPar(szSound), pObj, iLevel);
}

static bool FnGameOver(C4ValueInt iGameOverValue /* provided for future compatibility */)
{
	return !!Game.DoGameOver();
}

static bool FnGainMissionAccess(C4String *szPassword)
{
	if (SLen(Config.General.MissionAccess) + SLen(FnStringPar(szPassword)) + 3 > CFG_MaxString) return false;
	SAddModule(Config.General.MissionAccess, FnStringPar(szPassword));
	return true;
}

static void FnLog(C4AulContext *cthr, C4String *szMessage, C4Value iPar0, C4Value iPar1, C4Value iPar2, C4Value iPar3, C4Value iPar4, C4Value iPar5, C4Value iPar6, C4Value iPar7, C4Value iPar8)
{
	LogNTr(FnStringFormat(cthr, FnStringPar(szMessage), &iPar0, &iPar1, &iPar2, &iPar3, &iPar4, &iPar5, &iPar6, &iPar7, &iPar8));
}

static void FnDebugLog(C4AulContext *cthr, C4String *szMessage, C4Value iPar0, C4Value iPar1, C4Value iPar2, C4Value iPar3, C4Value iPar4, C4Value iPar5, C4Value iPar6, C4Value iPar7, C4Value iPar8)
{
	DebugLog(FnStringFormat(cthr, FnStringPar(szMessage), &iPar0, &iPar1, &iPar2, &iPar3, &iPar4, &iPar5, &iPar6, &iPar7, &iPar8));
}

static C4String *FnFormat(C4AulContext *cthr, C4String *szFormat, C4Value iPar0, C4Value iPar1, C4Value iPar2, C4Value iPar3, C4Value iPar4, C4Value iPar5, C4Value iPar6, C4Value iPar7, C4Value iPar8)
{
	return String(StdStrBuf{FnStringFormat(cthr, FnStringPar(szFormat), &iPar0, &iPar1, &iPar2, &iPar3, &iPar4, &iPar5, &iPar6, &iPar7, &iPar8).c_str()});
}

static C4ID FnC4Id(C4String *szID)
{
	return C4Id(FnStringPar(szID));
}

static bool FnPlayerMessage(C4AulContext *cthr, C4ValueInt iPlayer, C4String &szMessage, C4Object *pObj, C4Value iPar0, C4Value iPar1, C4Value iPar2, C4Value iPar3, C4Value iPar4, C4Value iPar5, C4Value iPar6)
{
	char buf[MaxFnStringParLen + 1];

	// Speech
	bool fSpoken = false;
	if (SCopySegment(FnStringPar(szMessage), 1, buf, '$', MaxFnStringParLen))
		if (StartSoundEffect(buf, false, 100, pObj ? pObj : cthr->Obj))
			fSpoken = true;

	// Text
	if (!fSpoken)
		if (SCopySegment(FnStringFormat(cthr, FnStringPar(szMessage), &iPar0, &iPar1, &iPar2, &iPar3, &iPar4, &iPar5, &iPar6).c_str(), 0, buf, '$', MaxFnStringParLen))
			if (pObj) GameMsgObjectPlayer(buf, pObj, iPlayer);
			else GameMsgPlayer(buf, iPlayer);

	return true;
}

static bool FnMessage(C4AulContext *cthr, C4String &szMessage, C4Object *pObj, C4Value iPar0, C4Value iPar1, C4Value iPar2, C4Value iPar3, C4Value iPar4, C4Value iPar5, C4Value iPar6, C4Value iPar7)
{
	char buf[MaxFnStringParLen + 1];

	// Speech
	bool fSpoken = false;
	if (SCopySegment(FnStringPar(szMessage), 1, buf, '$', MaxFnStringParLen))
		if (StartSoundEffect(buf, false, 100, cthr->Obj))
			fSpoken = true;

	// Text
	if (!fSpoken)
		if (SCopySegment(FnStringFormat(cthr, FnStringPar(szMessage), &iPar0, &iPar1, &iPar2, &iPar3, &iPar4, &iPar5, &iPar6, &iPar7).c_str(), 0, buf, '$', MaxFnStringParLen))
			if (pObj) GameMsgObject(buf, pObj);
			else GameMsgGlobal(buf);

	return true;
}

static bool FnAddMessage(C4AulContext *cthr, C4String &szMessage, C4Object *pObj, C4Value iPar0, C4Value iPar1, C4Value iPar2, C4Value iPar3, C4Value iPar4, C4Value iPar5, C4Value iPar6, C4Value iPar7)
{
	if (pObj) Game.Messages.Append(C4GM_Target, FnStringFormat(cthr, FnStringPar(szMessage), &iPar0, &iPar1, &iPar2, &iPar3, &iPar4, &iPar5, &iPar6, &iPar7).c_str(), pObj, NO_OWNER, 0, 0, FWhite);
	else Game.Messages.Append(C4GM_Global, FnStringFormat(cthr, FnStringPar(szMessage), &iPar0, &iPar1, &iPar2, &iPar3, &iPar4, &iPar5, &iPar6, &iPar7).c_str(), nullptr, ANY_OWNER, 0, 0, FWhite);

	return true;
}

static bool FnPlrMessage(C4AulContext *cthr, C4String &szMessage, C4ValueInt iPlr, C4Value iPar0, C4Value iPar1, C4Value iPar2, C4Value iPar3, C4Value iPar4, C4Value iPar5, C4Value iPar6, C4Value iPar7)
{
	char buf[MaxFnStringParLen + 1];

	// Speech
	bool fSpoken = false;
	if (SCopySegment(FnStringPar(szMessage), 1, buf, '$', MaxFnStringParLen))
		if (StartSoundEffect(buf, false, 100, cthr->Obj))
			fSpoken = true;

	// Text
	if (!fSpoken)
		if (SCopySegment(FnStringFormat(cthr, FnStringPar(szMessage), &iPar0, &iPar1, &iPar2, &iPar3, &iPar4, &iPar5, &iPar6, &iPar7).c_str(), 0, buf, '$', MaxFnStringParLen))
			if (ValidPlr(iPlr)) GameMsgPlayer(buf, iPlr);
			else GameMsgGlobal(buf);

			return true;
}

static void FnScriptGo(bool go)
{
	Game.Script.Go = !!go;
}

static void FnCastPXS(C4AulContext *cthr, C4String *mat_name, C4ValueInt amt, C4ValueInt level, C4ValueInt tx, C4ValueInt ty)
{
	MakePositionRelative(cthr, tx, ty);
	Game.PXS.Cast(Game.Material.Get(FnStringPar(mat_name)), amt, tx, ty, level);
}

static void FnCastObjects(C4AulContext *cthr, C4ID id, C4ValueInt amt, C4ValueInt level, C4ValueInt tx, C4ValueInt ty)
{
	MakePositionRelative(cthr, tx, ty);
	Game.CastObjects(id, cthr->Obj, amt, level, tx, ty, cthr->Obj ? cthr->Obj->Owner : NO_OWNER, cthr->Obj ? cthr->Obj->Controller : NO_OWNER);
}

static C4ValueInt FnMaterial(C4String *mat_name)
{
	return Game.Material.Get(FnStringPar(mat_name));
}

C4Object *FnPlaceVegetation(C4AulContext *cthr, C4ID id, C4ValueInt iX, C4ValueInt iY, C4ValueInt iWdt, C4ValueInt iHgt, C4ValueInt iGrowth)
{
	MakePositionRelative(cthr, iX, iY);
	// Place vegetation
	return Game.PlaceVegetation(id, iX, iY, iWdt, iHgt, iGrowth);
}

static void FnDrawVolcanoBranch(C4ValueInt mat, C4ValueInt fx, C4ValueInt fy, C4ValueInt tx, C4ValueInt ty, C4ValueInt size)
{
	C4ValueInt cx, cx2, cy;
	for (cy = ty; cy < fy; cy++)
	{
		cx = fx + (tx - fx) * (cy - fy) / (ty - fy);
		for (cx2 = cx - size / 2; cx2 < cx + size / 2; cx2++)
			SBackPix(cx2, cy, Mat2PixColDefault(mat) + GBackIFT(cx2, cy));
	}
}

static bool FnHostile(C4ValueInt iPlr1, C4ValueInt iPlr2, bool fCheckOneWayOnly)
{
	if (fCheckOneWayOnly)
	{
		return Game.Players.HostilityDeclared(iPlr1, iPlr2);
	}
	else
		return !!Hostile(iPlr1, iPlr2);
}

static bool FnSetHostility(C4Player &plr1, C4ValueInt iPlr2, bool fHostile, bool fSilent, bool fNoCalls)
{
	// do rejection test first
	if (!fNoCalls)
	{
		if (Game.Script.GRBroadcast(PSF_RejectHostilityChange, {C4VInt(plr1.Number), C4VInt(iPlr2), C4VBool(fHostile)}, true, true))
			return false;
	}
	// OK; set hostility
	bool fOldHostility = Game.Players.HostilityDeclared(plr1.Number, iPlr2);
	if (!plr1.SetHostility(iPlr2, fHostile, fSilent)) return false;
	// calls afterwards
	Game.Script.GRBroadcast(PSF_OnHostilityChange, {C4VInt(plr1.Number), C4VInt(iPlr2), C4VBool(fHostile), C4VBool(fOldHostility)}, true);
	return true;
}

static bool FnSetPlrView(C4Player &player, C4Object *tobj)
{
	player.SetViewMode(C4PVM_Target, tobj);
	return true;
}

static bool FnSetPlrShowControl(C4Player &player, C4String *defstring)
{
	player.ShowControl = StringBitEval(FnStringPar(defstring));
	return true;
}

static bool FnSetPlrShowCommand(C4Player &player, C4ValueInt iCom)
{
	player.FlashCom = iCom;
	if (!Config.Graphics.ShowCommands) Config.Graphics.ShowCommands = true;
	return true;
}

static bool FnSetPlrShowControlPos(C4Player &player, C4ValueInt pos)
{
	player.ShowControlPos = pos;
	return true;
}

static C4String *FnGetPlrControlName(C4ValueInt iPlr, C4ValueInt iCon, bool fShort)
{
	return String(PlrControlKeyName(iPlr, iCon, fShort).c_str());
}

static C4ValueInt FnGetPlrJumpAndRunControl(Required<C4Player *, -1, true, true> plr)
{
	return plr->ControlStyle;
}

static C4ValueInt FnGetPlrViewMode(Required<C4Player *, -1, true, true> plr)
{
	return plr->ViewMode;
}

static C4Object *FnGetPlrView(C4Player &player)
{
	if (player.ViewMode != C4PVM_Target) return nullptr;
	return player.ViewTarget;
}

static bool FnDoHomebaseMaterial(C4Player &player, C4ID id, C4ValueInt iChange)
{
	// validity check
	C4Def *pDef = C4Id2Def(id);
	if (!pDef) return false;
	// add to material
	C4ValueInt iLastcount = player.HomeBaseMaterial.GetIDCount(id);
	if (!player.HomeBaseMaterial.SetIDCount(id, iLastcount + iChange, true)) return false;
	if (Game.Rules & C4RULE_TeamHombase) player.SyncHomebaseMaterialToTeam();
	return true;
}

static bool FnDoHomebaseProduction(C4Player &player, C4ID id, C4ValueInt iChange)
{
	C4Def *pDef = C4Id2Def(id);
	if (!pDef) return false;
	// add to material
	C4ValueInt iLastcount = player.HomeBaseProduction.GetIDCount(id);
	return player.HomeBaseProduction.SetIDCount(id, iLastcount + iChange, true);
}

static bool FnClearLastPlrCom(C4Player &player)
{
	// reset last coms
	player.LastCom = COM_None;
	player.LastComDownDouble = 0;
	// done, success
	return true;
}

static bool FnSetPlrKnowledge(C4Player &player, C4ID id, bool fRemove)
{
	if (fRemove)
	{
		C4ValueInt iIndex = player.Knowledge.GetIndex(id);
		if (iIndex < 0) return false;
		return player.Knowledge.DeleteItem(iIndex);
	}
	else
	{
		if (!C4Id2Def(id)) return false;
		return player.Knowledge.SetIDCount(id, 1, true);
	}
}

static bool FnSetComponent(C4ID idComponent, C4ValueInt iCount, Required<C4ObjectOrThis> pObj)
{
	return pObj->Component.SetIDCount(idComponent, iCount, true);
}

static C4Value FnGetPlrKnowledge(C4Player &player, C4ID id, C4ValueInt iIndex, C4ValueInt dwCategory)
{
	// Search by id, check if available, return bool
	if (id) return C4VBool(player.Knowledge.GetIDCount(id, 1) != 0);
	// Search indexed item of given category, return C4ID
	return C4VID(player.Knowledge.GetID(Game.Defs, dwCategory, iIndex));
}

static C4ID FnGetDefinition(C4ValueInt iIndex, Default<C4ValueInt, C4D_All, true> dwCategory)
{
	C4Def *pDef;
	// Get def
	if (!(pDef = Game.Defs.GetDef(iIndex, dwCategory))) return C4ID_None;
	// Return id
	return pDef->id;
}

static C4Value FnGetComponent(C4AulContext *cthr, C4ID idComponent, C4ValueInt iIndex, C4ObjectOrThis pObj, C4ID idDef)
{
	// Def component - as seen by scope object as builder
	if (idDef)
	{
		// Get def
		C4Def *pDef = C4Id2Def(idDef);
		if (!pDef) return C4VNull;
		// Component count
		if (idComponent) return C4VInt(pDef->GetComponentCount(idComponent, cthr->Obj));
		// Indexed component
		return C4VID(pDef->GetIndexedComponent(iIndex, cthr->Obj));
	}
	// Object component
	else
	{
		// Get object
		if (!pObj) return C4VNull;
		// Component count
		if (idComponent) return C4VInt(pObj->Component.GetIDCount(idComponent));
		// Indexed component
		return C4VID(pObj->Component.GetID(iIndex));
	}
}

static C4Value FnGetHomebaseMaterial(C4Player &player, C4ID id, C4ValueInt iIndex, C4ValueInt dwCategory)
{
	// Search by id, return available count
	if (id) return C4VInt(player.HomeBaseMaterial.GetIDCount(id));
	// Search indexed item of given category, return C4ID
	return C4VID(player.HomeBaseMaterial.GetID(Game.Defs, dwCategory, iIndex));
}

static C4Value FnGetHomebaseProduction(C4Player &player, C4ID id, C4ValueInt iIndex, C4ValueInt dwCategory)
{
	// Search by id, return available count
	if (id) return C4VInt(player.HomeBaseProduction.GetIDCount(id));
	// Search indexed item of given category, return C4ID
	return C4VID(player.HomeBaseProduction.GetID(Game.Defs, dwCategory, iIndex));
}

static C4ValueInt FnSetPlrMagic(C4Player &player, C4ID id, bool fRemove)
{
	if (fRemove)
	{
		C4ValueInt iIndex = player.Magic.GetIndex(id);
		if (iIndex < 0) return false;
		return player.Magic.DeleteItem(iIndex);
	}
	else
	{
		if (!C4Id2Def(id)) return false;
		return player.Magic.SetIDCount(id, 1, true);
	}
}

static C4Value FnGetPlrMagic(C4Player &player, C4ID id, C4ValueInt iIndex)
{
	// Search by id, check if available, return bool
	if (id) return C4VBool(player.Magic.GetIDCount(id, 1) != 0);
	// Search indexed item of given category, return C4ID
	return C4VID(player.Magic.GetID(Game.Defs, C4D_Magic, iIndex));
}

static bool FnSetWealth(C4Player &player, C4ValueInt iValue)
{
	player.Wealth = BoundBy<C4ValueInt>(iValue, 0, 100000);
	return true;
}

static C4Object *FnGetHiRank(C4Player &player)
{
	return player.GetHiRankActiveCrew(false);
}

static C4Object *FnGetCrew(C4Player &player, C4ValueInt index)
{
	return player.Crew.GetObject(index);
}

static C4ValueInt FnGetCrewCount(C4Player &player)
{
	return player.Crew.ObjectCount();
}

static C4ValueInt FnGetPlayerCount(C4ValueInt iType)
{
	if (!iType)
		return Game.Players.GetCount();
	else
		return Game.Players.GetCount(static_cast<C4PlayerType>(iType));
}

static C4ValueInt FnGetPlayerByIndex(C4ValueInt iIndex, C4ValueInt iType)
{
	C4Player *pPlayer;
	if (iType)
		pPlayer = Game.Players.GetByIndex(iIndex, static_cast<C4PlayerType>(iType));
	else
		pPlayer = Game.Players.GetByIndex(iIndex);
	if (!pPlayer) return NO_OWNER;
	return pPlayer->Number;
}

static C4ValueInt FnEliminatePlayer(C4Player &player, bool fRemoveDirect)
{
	// direct removal?
	if (fRemoveDirect)
	{
		// do direct removal (no fate)
		if (Game.Control.isCtrlHost()) Game.Players.CtrlRemove(player.Number, false);
		return true;
	}
	else
	{
		// do regular elimination
		if (player.Eliminated) return false;
		player.Eliminate();
	}
	return true;
}

static bool FnSurrenderPlayer(C4Player &player)
{
	if (player.Eliminated) return false;
	player.Surrender();
	return true;
}

static bool FnSetLeaguePerformance(C4ValueInt iScore, std::optional<C4PlayerInfo *> playerInfo)
{
	if (!Game.Parameters.isLeague()) return false;
	if (playerInfo && !*playerInfo) return false;
	Game.RoundResults.SetLeaguePerformance(iScore, playerInfo.transform(&C4PlayerInfo::GetID).value_or(0));
	return true;
}

static bool FnSetLeagueProgressData(C4String *pNewData, C4PlayerInfo &info)
{
	if (!Game.Parameters.League.getLength()) return false;
	info.SetLeagueProgressData(pNewData ? pNewData->Data.getData() : nullptr);
	return true;
}

static C4String *FnGetLeagueProgressData(C4PlayerInfo &info)
{
	if (!Game.Parameters.League.getLength()) return nullptr;
	return String(info.GetLeagueProgressData());
}

static const int32_t CSPF_FixedAttributes    = 1 << 0,
                     CSPF_NoScenarioInit     = 1 << 1,
                     CSPF_NoEliminationCheck = 1 << 2,
                     CSPF_Invisible          = 1 << 3;

static bool FnCreateScriptPlayer(C4String &szName, C4ValueInt dwColor, C4ValueInt idTeam, C4ValueInt dwFlags, C4ID idExtra)
{
	// safety
	if (!szName.Data.getLength()) return false;
	// this script command puts a new script player info into the list
	// the actual join will be delayed and synchronized via queue
	// processed by control host only - clients/replay/etc. will perform the join via queue
	if (!Game.Control.isCtrlHost()) return true;
	C4PlayerInfo *pScriptPlrInfo = new C4PlayerInfo();
	uint32_t dwInfoFlags = 0u;
	if (dwFlags & CSPF_FixedAttributes) dwInfoFlags |= C4PlayerInfo::PIF_AttributesFixed;
	if (dwFlags & CSPF_NoScenarioInit) dwInfoFlags |= C4PlayerInfo::PIF_NoScenarioInit;
	if (dwFlags & CSPF_NoEliminationCheck) dwInfoFlags |= C4PlayerInfo::PIF_NoEliminationCheck;
	if (dwFlags & CSPF_Invisible) dwInfoFlags |= C4PlayerInfo::PIF_Invisible;
	pScriptPlrInfo->SetAsScriptPlayer(szName.Data.getData(), dwColor, dwInfoFlags, idExtra);
	pScriptPlrInfo->SetTeam(idTeam);
	C4ClientPlayerInfos JoinPkt(nullptr, true, pScriptPlrInfo);
	// add to queue!
	Game.PlayerInfos.DoPlayerInfoUpdate(&JoinPkt);
	// always successful for sync reasons
	return true;
}

static C4Object *FnGetCursor(C4Player &player, C4ValueInt iIndex)
{
	// first index is always the cursor
	if (!iIndex) return player.Cursor;
	// iterate through selected crew for iIndex times
	// status needs not be checked, as dead objects are never in Crew list
	C4Object *pCrew;
	for (C4ObjectLink *pLnk = player.Crew.First; pLnk; pLnk = pLnk->Next)
		// get crew object
		if (pCrew = pLnk->Obj)
			// is it selected?
			if (pCrew->Select)
				// is it not the cursor? (which is always first)
				if (pCrew != player.Cursor)
					// enough searched?
					if (!--iIndex)
						// return it
						return pCrew;
	// nothing found at that index
	return nullptr;
}

static bool FnSetCursor(C4Player &player, C4Object &obj, bool fNoSelectMark, bool fNoSelectArrow, bool fNoSelectCrew)
{
	if (!obj.Status) return false;
	player.SetCursor(&obj, !fNoSelectMark, !fNoSelectArrow);
	if (!fNoSelectCrew) player.SelectCrew(&obj, true);
	return true;
}

static bool FnSetViewCursor(C4Player &player, C4Object *pObj)
{
	// set viewcursor
	player.ViewCursor = pObj;
	return true;
}

static bool FnSelectCrew(C4Player &player, C4Object &obj, bool fSelect, bool fNoCursorAdjust)
{
	if (fNoCursorAdjust)
	{
		if (fSelect) obj.DoSelect(); else obj.UnSelect();
	}
	else
		player.SelectCrew(&obj, fSelect);
	return true;
}

static C4ValueInt FnSetCrewStatus(C4Player &player, bool fInCrew, Required<C4ObjectOrThis> pObj)
{
	// set crew status
	return player.SetObjectCrewStatus(pObj, fInCrew);
}

static C4ValueInt FnGetWind(C4AulContext *cthr, C4ValueInt x, C4ValueInt y, bool fGlobal)
{
	// global wind
	if (fGlobal) return Game.Weather.Wind;
	MakePositionRelative(cthr, x, y);
	return Game.Weather.GetWind(x, y);
}

static void FnSetSkyFade(C4ValueInt iFromRed, C4ValueInt iFromGreen, C4ValueInt iFromBlue, C4ValueInt iToRed, C4ValueInt iToGreen, C4ValueInt iToBlue)
{
	// newgfx: set modulation
	uint32_t dwBack, dwMod = GetClrModulation(Game.Landscape.Sky.FadeClr1, C4RGB(iFromRed, iFromGreen, iFromBlue), dwBack);
	Game.Landscape.Sky.SetModulation(dwMod, dwBack);
}

static void FnSetSkyColor(C4ValueInt iIndex, C4ValueInt iRed, C4ValueInt iGreen, C4ValueInt iBlue)
{
	// set first index only
	if (iIndex) return;
	// get color difference
	uint32_t dwBack, dwMod = GetClrModulation(Game.Landscape.Sky.FadeClr1, C4RGB(iRed, iGreen, iBlue), dwBack);
	Game.Landscape.Sky.SetModulation(dwMod, dwBack);
	// success
}

static C4ValueInt FnGetSkyColor(C4ValueInt iIndex, C4ValueInt iRGB)
{
	// relict from OldGfx
	if (iIndex || !Inside<C4ValueInt>(iRGB, 0, 2)) return 0;
	uint32_t dwClr = Game.Landscape.Sky.FadeClr1;
	BltAlpha(dwClr, Game.Landscape.Sky.FadeClr2 | ((iIndex * 0xff / 19) << 24));
	switch (iRGB)
	{
	case 0: return (dwClr >> 16) & 0xff;
	case 1: return (dwClr >> 8) & 0xff;
	case 2: return dwClr & 0xff;
	default: return 0;
	}
}

static C4ValueInt FnLaunchVolcano(C4ValueInt x)
{
	return Game.Weather.LaunchVolcano(
		Game.Material.Get("Lava"),
		x, GBackHgt - 1,
		BoundBy(15 * GBackHgt / 500 + Random(10), 10, 60));
}

static void FnShakeObjects(C4AulContext *cthr, C4ValueInt x, C4ValueInt y, C4ValueInt rad)
{
	Game.ShakeObjects(x, y, rad, cthr->Obj ? cthr->Obj->Controller : NO_OWNER);
}

static void FnDigFree(C4AulContext *cthr, C4ValueInt x, C4ValueInt y, C4ValueInt rad, bool fRequest)
{
	Game.Landscape.DigFree(x, y, rad, fRequest, cthr->Obj);
}

static void FnDigFreeRect(C4AulContext *cthr, C4ValueInt iX, C4ValueInt iY, C4ValueInt iWdt, C4ValueInt iHgt, bool fRequest)
{
	Game.Landscape.DigFreeRect(iX, iY, iWdt, iHgt, fRequest, cthr->Obj);
}

static void FnFreeRect(C4ValueInt iX, C4ValueInt iY, C4ValueInt iWdt, C4ValueInt iHgt, C4ValueInt iFreeDensity)
{
	if (iFreeDensity)
		Game.Landscape.ClearRectDensity(iX, iY, iWdt, iHgt, iFreeDensity);
	else
		Game.Landscape.ClearRect(iX, iY, iWdt, iHgt);
}

static bool FnPathFree(C4ValueInt X1, C4ValueInt Y1, C4ValueInt X2, C4ValueInt Y2)
{
	return !!PathFree(X1, Y1, X2, Y2);
}

static bool FnPathFree2(C4Value *X1, C4Value *Y1, C4ValueInt X2, C4ValueInt Y2)
{
	int32_t x = -1, y = -1;
	// Do not use getInt on the references, because it destroys them.
	bool r = PathFree(X1->GetRefVal().getInt(), Y1->GetRefVal().getInt(), X2, Y2, &x, &y);
	if (!r)
	{
		*X1 = C4VInt(x);
		*Y1 = C4VInt(y);
	}
	return r;
}

static C4ValueInt FnSetTransferZone(C4ValueInt iX, C4ValueInt iY, C4ValueInt iWdt, C4ValueInt iHgt, Required<C4ObjectOrThis> pObj)
{
	MakePositionRelative(pObj, iX, iY);
	return Game.TransferZones.Set(iX, iY, iWdt, iHgt, pObj);
}

static bool FnNot(bool fCondition)
{
	return !fCondition;
}

static bool FnOr(bool fCon1, bool fCon2, bool fCon3, bool fCon4, bool fCon5)
{
	return (fCon1 || fCon2 || fCon3 || fCon4 || fCon5);
}

static bool FnAnd(bool fCon1, bool fCon2)
{
	return (fCon1 && fCon2);
}

static C4ValueInt FnBitAnd(C4ValueInt iVal1, C4ValueInt iVal2)
{
	return (iVal1 & iVal2);
}

static bool FnEqual(C4Value Val1, C4Value Val2)
{
	return Val1.GetData() == Val2.GetData();
}

static C4ValueInt FnLessThan(C4ValueInt iVal1, C4ValueInt iVal2)
{
	return (iVal1 < iVal2);
}

static C4ValueInt FnGreaterThan(C4ValueInt iVal1, C4ValueInt iVal2)
{
	return (iVal1 > iVal2);
}

static C4ValueInt FnSum(C4ValueInt iVal1, C4ValueInt iVal2, C4ValueInt iVal3, C4ValueInt iVal4)
{
	return (iVal1 + iVal2 + iVal3 + iVal4);
}

static C4ValueInt FnSub(C4ValueInt iVal1, C4ValueInt iVal2, C4ValueInt iVal3, C4ValueInt iVal4)
{
	return (iVal1 - iVal2 - iVal3 - iVal4);
}

static C4ValueInt FnMul(C4ValueInt iVal1, C4ValueInt iVal2)
{
	return (iVal1 * iVal2);
}

static C4ValueInt FnDiv(C4ValueInt iVal1, RequiredNonZero<C4ValueInt, 0> iVal2)
{
	return (iVal1 / *iVal2);
}

static C4ValueInt FnMod(C4ValueInt iVal1, RequiredNonZero<C4ValueInt, 0> iVal2)
{
	return (iVal1 % *iVal2);
}

static C4ValueInt FnPow(C4ValueInt iVal1, C4ValueInt iVal2)
{
	return Pow(iVal1, iVal2);
}

template<C4Fixed Function(const C4Fixed &)>
static C4ValueInt FnCircle(C4ValueInt angle, const C4ValueInt radius, Default<C4ValueInt, 1, true> precision)
{
	// Precalculate the modulo operation so the C4Fixed argument does not overflow
	angle %= 360 * precision;
	// Let itofix and fixtoi handle the division and multiplication because that can handle higher ranges
	return fixtoi(Function(itofix(angle, precision)), radius);
}

static C4ValueInt FnSqrt(C4ValueInt iValue)
{
	if (iValue < 0) return 0;
	C4ValueInt iSqrt = C4ValueInt(sqrt(double(iValue)));
	if (iSqrt * iSqrt < iValue) iSqrt++;
	if (iSqrt * iSqrt > iValue) iSqrt--;
	return iSqrt;
}

static C4ValueInt FnAngle(C4ValueInt iX1, C4ValueInt iY1, C4ValueInt iX2, C4ValueInt iY2, Default<C4ValueInt, 1, true> iPrec)
{
	C4ValueInt iAngle;

	C4ValueInt dx = iX2 - iX1, dy = iY2 - iY1;
	if (!dx) if (dy > 0) return 180 * iPrec; else return 0;
	if (!dy) if (dx > 0) return 90 * iPrec; else return 270 * iPrec;

	iAngle = static_cast<C4ValueInt>(180.0 * iPrec * atan2(static_cast<double>(Abs(dy)), static_cast<double>(Abs(dx))) * std::numbers::inv_pi);

	if (iX2 > iX1)
	{
		if (iY2 < iY1) iAngle = (90 * iPrec) - iAngle;
		else iAngle = (90 * iPrec) + iAngle;
	}
	else
	{
		if (iY2 < iY1) iAngle = (270 * iPrec) + iAngle;
		else iAngle = (270 * iPrec) - iAngle;
	}

	return iAngle;
}

template<double Function(double)>
static C4ValueInt FnArcus(const C4ValueInt value, const RequiredNonZero<C4ValueInt, 0> radius)
{
	if (value > radius) return 0;

	const double result{Function(static_cast<double>(value) / radius) * 180.0 * std::numbers::inv_pi};
	// return rounded angle
	return static_cast<C4ValueInt>(std::floor(result + 0.5));
}

static C4ValueInt FnMin(C4ValueInt iVal1, C4ValueInt iVal2)
{
	return (std::min)(iVal1, iVal2);
}

static C4ValueInt FnMax(C4ValueInt iVal1, C4ValueInt iVal2)
{
	return (std::max)(iVal1, iVal2);
}

static C4ValueInt FnObjectDistance(C4Object &obj2, Required<C4ObjectOrThis> pObj)
{
	return Distance(pObj->x, pObj->y, obj2.x, obj2.y);
}

static C4ValueInt FnObjectNumber(Required<C4ObjectOrThis> pObj)
{
	return pObj->Number;
}

static C4ValueInt FnShowInfo(C4AulContext *cthr, C4ObjectOrThis pObj)
{
	if (!cthr->Obj) return false;
	return cthr->Obj->ActivateMenu(C4MN_Info, 0, 0, 0, pObj);
}

static C4ValueInt FnSEqual(C4String *szString1, C4String *szString2)
{
	if (szString1 == szString2) return true;
	return SEqual(FnStringPar(szString1), FnStringPar(szString2));
}

static C4ValueInt FnRandom(C4ValueInt iRange)
{
	return Random(iRange);
}

static C4ValueInt FnAsyncRandom(C4ValueInt iRange)
{
	return SafeRandom(iRange);
}

static C4Value FnSetVar(C4AulContext *cthr, C4ValueInt iVarIndex, C4Value iValue)
{
	if (!cthr->Caller) return C4VNull;
	cthr->Caller->NumVars[iVarIndex] = iValue;
	return iValue;
}

static C4Value FnIncVar(C4AulContext *cthr, C4ValueInt iVarIndex)
{
	if (!cthr->Caller) return C4VNull;
	return ++cthr->Caller->NumVars[iVarIndex];
}

static C4Value FnDecVar(C4AulContext *cthr, C4ValueInt iVarIndex)
{
	if (!cthr->Caller) return C4VNull;
	return --cthr->Caller->NumVars[iVarIndex];
}

static C4Value FnVar(C4AulContext *cthr, C4ValueInt iVarIndex)
{
	if (!cthr->Caller) return C4VNull;
	// Referenz zurückgeben
	return cthr->Caller->NumVars[iVarIndex].GetRef();
}

static C4Value FnSetGlobal(C4ValueInt iVarIndex, C4Value iValue)
{
	Game.ScriptEngine.Global[iVarIndex] = iValue;
	return iValue;
}

static C4Value FnGlobal(C4ValueInt iVarIndex)
{
	return Game.ScriptEngine.Global[iVarIndex].GetRef();
}

static C4Value FnSetLocal(C4ValueInt iVarIndex, C4Value iValue, Required<C4ObjectOrThis> pObj)
{
	pObj->Local[iVarIndex] = iValue;
	return iValue;
}

static C4Value FnLocal(C4ValueInt iIndex, Required<C4ObjectOrThis> pObj)
{
	if (iIndex < 0) return C4VNull;
	return pObj->Local[iIndex].GetRef();
}

static C4Value FnCall(C4AulContext *cthr, C4String &szFunction,
	C4Value par0, C4Value par1, C4Value par2, C4Value par3, C4Value par4,
	C4Value par5, C4Value par6, C4Value par7, C4Value par8)
{
	if (!cthr->Obj) return C4VNull;
	C4AulParSet Pars;
	Copy2ParSet9(Pars, par);
	return cthr->Obj->Call(FnStringPar(szFunction), Pars, true, !cthr->CalledWithStrictNil());
}

static C4Value FnObjectCall(C4AulContext *cthr,
	C4Object &obj, C4String &szFunction,
	C4Value par0, C4Value par1, C4Value par2, C4Value par3, C4Value par4,
	C4Value par5, C4Value par6, C4Value par7)
{
	if (!obj.Def) return C4VNull;
	// get func
	C4AulFunc *f;
	if (!(f = obj.Def->Script.GetSFunc(FnStringPar(szFunction), AA_PUBLIC, true))) return C4VNull;
	// copy pars
	C4AulParSet Pars;
	Copy2ParSet8(Pars, par);
	// exec
	return f->Exec(&obj, Pars, true, true, !cthr->CalledWithStrictNil());
}

static C4Value FnDefinitionCall(C4AulContext *cthr,
	C4Def &def, C4String &szFunction,
	C4Value par0, C4Value par1, C4Value par2, C4Value par3, C4Value par4,
	C4Value par5, C4Value par6, C4Value par7)
{
	// Make failsafe
	char szFunc2[C4AUL_MAX_Identifier + 1];
	FormatWithNull(szFunc2, "~{}", FnStringPar(szFunction));
	// copy parameters
	C4AulParSet Pars;
	Copy2ParSet8(Pars, par);
	// Call
	return def.Script.Call(szFunc2, Pars, true, !cthr->CalledWithStrictNil());
}

static C4Value FnGameCall(C4AulContext *cthr,
	C4String &szFunction,
	C4Value par0, C4Value par1, C4Value par2, C4Value par3, C4Value par4,
	C4Value par5, C4Value par6, C4Value par7, C4Value par8)
{
	// Make failsafe
	char szFunc2[C4AUL_MAX_Identifier + 1];
	FormatWithNull(szFunc2, "~{}", FnStringPar(szFunction));
	// copy parameters
	C4AulParSet Pars;
	Copy2ParSet9(Pars, par);
	// Call
	return Game.Script.Call(szFunc2, Pars, true, !cthr->CalledWithStrictNil());
}

static C4Value FnGameCallEx(C4AulContext *cthr,
	C4String &szFunction,
	C4Value par0, C4Value par1, C4Value par2, C4Value par3, C4Value par4,
	C4Value par5, C4Value par6, C4Value par7, C4Value par8)
{
	// Make failsafe
	char szFunc2[C4AUL_MAX_Identifier + 1];
	FormatWithNull(szFunc2, "~{}", FnStringPar(szFunction));
	// copy parameters
	C4AulParSet Pars;
	Copy2ParSet9(Pars, par);
	// Call
	return Game.Script.GRBroadcast(szFunc2, Pars, true, false, !cthr->CalledWithStrictNil());
}

static C4Value FnProtectedCall(C4AulContext *cthr,
	C4Object &obj, C4String &szFunction,
	C4Value par0, C4Value par1, C4Value par2, C4Value par3, C4Value par4,
	C4Value par5, C4Value par6, C4Value par7)
{
	if (!obj.Def) return C4VNull;
	// get func
	C4AulScriptFunc *f;
	if (!(f = obj.Def->Script.GetSFunc(FnStringPar(szFunction), AA_PROTECTED, true))) return C4VNull;
	// copy parameters
	C4AulParSet Pars;
	Copy2ParSet8(Pars, par);
	// exec
	return f->Exec(&obj, Pars, true, !cthr->CalledWithStrictNil());
}

static C4Value FnPrivateCall(C4AulContext *cthr,
	C4Object &obj, C4String &szFunction,
	C4Value par0, C4Value par1, C4Value par2, C4Value par3, C4Value par4,
	C4Value par5, C4Value par6, C4Value par7)
{
	if (!obj.Def) return C4VNull;
	// get func
	C4AulScriptFunc *f;
	if (!(f = obj.Def->Script.GetSFunc(FnStringPar(szFunction), AA_PRIVATE, true))) return C4VNull;
	// copy parameters
	C4AulParSet Pars;
	Copy2ParSet8(Pars, par);
	// exec
	return f->Exec(&obj, Pars, true, !cthr->CalledWithStrictNil());
}

static C4Object *FnEditCursor()
{
	if (Game.Control.SyncMode()) return nullptr;
	return Console.EditCursor.GetTarget();
}

static void FnResort(C4ObjectOrThis pObj)
{
	// Resort single object
	if (pObj)
		pObj->Resort();
	// Resort object list
	else
		Game.Objects.SortByCategory();
}

static C4String *FnGetLeague(C4ValueInt idx)
{
	// get indexed league
	StdStrBuf sIdxLeague;
	if (!Game.Parameters.League.GetSection(idx, &sIdxLeague)) return nullptr;
	return String(sIdxLeague.getData());
}

static bool FnTestMessageBoard(C4Player &forPlayer, bool fTestIfInUse)
{
	// multi-query-MessageBoard is always available if the player is valid =)
	// (but it won't do anything in developer mode...)
	if (!fTestIfInUse) return true;
	// single query only if no query is scheduled
	return forPlayer.HasMessageBoardQuery();
}

static bool FnCallMessageBoard(Required<C4ObjectOrThis> pObj, bool fUpperCase, C4String *szQueryString, C4Player &forPlayer)
{
	if (!pObj->Status) return false;
	// remove any previous
	forPlayer.CallMessageBoard(pObj, StdStrBuf::MakeRef(FnStringPar(szQueryString)), !!fUpperCase);
	return true;
}

static bool FnAbortMessageBoard(C4ObjectOrThis pObj, C4Player &forPlayer)
{
	// close TypeIn if active
	Game.MessageInput.AbortMsgBoardQuery(pObj, forPlayer.Number);
	// abort for it
	return forPlayer.RemoveMessageBoardQuery(pObj);
}

static bool FnOnMessageBoardAnswer(C4Object *pObj, C4Player &forPlayer, C4String *szAnswerString)
{
	// remove query
	// fail if query doesn't exist to prevent any doubled answers
	if (!forPlayer.RemoveMessageBoardQuery(pObj)) return false;
	// if no answer string is provided, the user did not answer anything
	// just remove the query
	if (!szAnswerString || !szAnswerString->Data.getData()) return true;
	// get script
	C4ScriptHost *scr;
	if (pObj) scr = &pObj->Def->Script; else scr = &Game.Script;
	// exec func
	return static_cast<bool>(scr->ObjectCall(nullptr, pObj, PSF_InputCallback, {C4VString(FnStringPar(szAnswerString)), C4VInt(forPlayer.Number)}, true));
}

static C4ValueInt FnSetMass(C4ValueInt iValue, Required<C4ObjectOrThis> pObj)
{
	pObj->OwnMass = iValue - pObj->Def->Mass;
	pObj->UpdateMass();
	return true;
}

static C4ValueInt FnSetColor(C4ValueInt iValue, Required<C4ObjectOrThis> pObj)
{
	if (!Inside<C4ValueInt>(iValue, 0, C4MaxColor - 1)) return false;
	iValue = Application.DDraw->Pal.GetClr(FColors[FPlayer + iValue]);
	pObj->Color = iValue;
	pObj->UpdateGraphics(false);
	pObj->UpdateFace(false);
	return true;
}

static C4ValueInt FnGetColorDw(Required<C4ObjectOrThis> pObj)
{
	return pObj->Color;
}

static bool FnSetColorDw(C4ValueInt iValue, Required<C4ObjectOrThis> pObj)
{
	pObj->Color = iValue;
	pObj->UpdateGraphics(false);
	pObj->UpdateFace(false);
	return true;
}

static C4ValueInt FnSetFoW(bool fEnabled, C4Player &player)
{
	// set enabled
	player.SetFoW(fEnabled);
	// success
	return true;
}

static C4ValueInt FnSetPlrViewRange(C4ValueInt iRange, Required<C4ObjectOrThis> pObj, bool fExact)
{
	// backwards compatibility for low ranges
	if (!fExact && iRange < 128 && iRange > 0) iRange = 128;
	// set range
	pObj->SetPlrViewRange(iRange);
	// success
	return true;
}

static C4ValueInt FnSetMaxPlayer(C4ValueInt iTo)
{
	// think positive! :)
	if (iTo < 0) return false;
	// script functions don't need to pass ControlQueue!
	Game.Parameters.MaxPlayers = iTo;
	// success
	return true;
}

static C4ValueInt FnSetPicture(C4ValueInt iX, C4ValueInt iY, C4ValueInt iWdt, C4ValueInt iHgt, Required<C4ObjectOrThis> pObj)
{
	// set new picture rect
	pObj->PictureRect.Set(iX, iY, iWdt, iHgt);
	// success
	return true;
}

static C4String *FnGetProcedure(Required<C4ObjectOrThis> pObj)
{
	// no action?
	if (pObj->Action.Act <= ActIdle) return nullptr;
	// get proc
	C4ValueInt iProc = pObj->Def->ActMap[pObj->Action.Act].Procedure;
	// NONE?
	if (iProc <= DFA_NONE) return nullptr;
	// return procedure name
	return String(ProcedureName[iProc]);
}

static C4Object *FnBuy(C4AulContext *cthr, C4ID idBuyObj, C4Player &forPlayer, C4Player &payPlayer, C4Object *pToBase, bool fShowErrors)
{
	// buy
	C4Object *pThing;
	if (!(pThing = payPlayer.Buy(idBuyObj, fShowErrors, forPlayer.Number, pToBase ? pToBase : cthr->Obj))) return nullptr;
	// enter object, if supplied
	if (pToBase)
	{
		pThing->Enter(pToBase, true);
	}
	else
	{
		// no target object? get local pos
		if (cthr->Obj) pThing->ForcePosition(cthr->Obj->x, cthr->Obj->y);
	}
	// done
	return pThing;
}

static bool FnSell(C4Player &toPlayer, Required<C4ObjectOrThis> pObj)
{
	// sell
	return toPlayer.Sell2Home(pObj);
}

// ** additional funcs for references/type info

static C4Value FnSet(C4Value *Dest, C4Value Src)
{
	*Dest = Src;
	return *Dest;
}

static C4Value FnInc(C4Value *Value, C4Value Diff)
{
	if (!Value->GetRefVal().ConvertTo(C4V_Int))
		return C4VNull;

	*Value += Diff.getInt();

	return *Value;
}

static C4Value FnDec(C4Value *Value, C4Value Diff)
{
	if (!Value->GetRefVal().ConvertTo(C4V_Int))
		return C4VNull;

	*Value -= Diff.getInt();

	return *Value;
}

static bool FnIsRef(C4Value Value)
{
	return Value.IsRef();
}

static C4ValueInt FnGetType(C4AulContext *cthr, C4Value Value)
{
	if (!Value._getBool() && cthr->Caller && !cthr->CalledWithStrictNil()) return C4V_Any;
	return Value.GetType();
}

static C4ValueArray *FnCreateArray(C4ValueInt iSize)
{
	return new C4ValueArray(iSize);
}

static C4ValueInt FnGetLength(C4AulContext *cthr, Required<C4Value> pPar)
{
	if (auto map = (*pPar).getMap())
		return map->size();
	C4ValueArray *pArray = (*pPar).getArray();
	if (pArray)
		return pArray->GetSize();
	C4String *pStr = (*pPar).getStr();
	if (pStr)
		return pStr->Data.getLength();
	throw C4AulExecError(cthr->Obj, "func \"GetLength\" par 0 cannot be converted to string or array or map");
}

static C4ValueInt FnGetIndexOf(C4AulContext *cthr, C4Value searchVal, Required<C4ValueArray *, -1> pArray)
{
	// find first occurance of first parameter in array
	// find the element by comparing data only - this may result in bogus results if an object ptr array is searched for an int
	// however, that's rather unlikely and strange scripting style
	int32_t iSize = pArray->GetSize();

	if (!cthr->Caller || cthr->Caller->Func->pOrgScript->Strict >= C4AulScriptStrict::STRICT2)
	{
		auto strict = cthr->Caller->Func->pOrgScript->Strict;
		const auto &val = searchVal.GetRefVal();
		for (int32_t i = 0; i < iSize; ++i)
			if (val.Equals(pArray->GetItem(i), strict))
				// element found
				return i;
	}
	else
	{
		const auto cmp = searchVal._getRaw();
		for (int32_t i = 0; i < iSize; ++i)
			if (cmp == pArray->GetItem(i)._getRaw())
				// element found
				return i;
	}
	// element not found
	return -1;
}

static void FnSetLength(C4AulContext *cthr, C4Value *pArrayRef, C4ValueInt iNewSize)
{
	// safety
	if (iNewSize < 0 || iNewSize > C4ValueList::MaxSize)
		throw C4AulExecError(cthr->Obj, std::format("SetLength: invalid array size ({})", iNewSize));

	// set new size
	pArrayRef->SetArrayLength(iNewSize, cthr);
}

static bool FnSetVisibility(C4ValueInt iVisibility, Required<C4ObjectOrThis> pObj)
{
	pObj->Visibility = iVisibility;
	return true;
}

static C4ValueInt FnGetVisibility(Required<C4ObjectOrThis> pObj)
{
	return pObj->Visibility;
}

static bool FnSetClrModulation(C4ValueInt dwClr, Required<C4ObjectOrThis> pObj, C4ValueInt iOverlayID)
{
	// overlay?
	if (iOverlayID)
	{
		C4GraphicsOverlay *pOverlay = pObj->GetGraphicsOverlay(iOverlayID, false);
		if (!pOverlay)
		{
			DebugLog(spdlog::level::err, "SetClrModulation: Overlay {} not defined for object {} ({})", static_cast<int>(iOverlayID), static_cast<int>(pObj->Number), pObj->GetName());
			return false;
		}
		pOverlay->SetClrModulation(dwClr);
	}
	else
	{
		// set it
		pObj->ColorMod = dwClr;
	}
	// success
	return true;
}

static std::optional<C4ValueInt> FnGetClrModulation(Required<C4ObjectOrThis> pObj, C4ValueInt iOverlayID)
{
	// overlay?
	if (iOverlayID)
	{
		C4GraphicsOverlay *pOverlay = pObj->GetGraphicsOverlay(iOverlayID, false);
		if (!pOverlay)
		{
			DebugLog(spdlog::level::err, "GetClrModulation: Overlay {} not defined for object {} ({})", static_cast<int>(iOverlayID), static_cast<int>(pObj->Number), pObj->GetName());
			return {};
		}
		return {pOverlay->GetClrModulation()};
	}
	else
		// get it
		return {pObj->ColorMod};
}

static bool FnGetMissionAccess(C4String &strMissionAccess)
{
	// non-sync mode: warn
	if (Game.Control.SyncMode())
		LogNTr(spdlog::level::warn, "using GetMissionAccess may cause desyncs when playing records!");

	return SIsModule(Config.General.MissionAccess, FnStringPar(strMissionAccess));
}

// Helper to read or write a value from/to a structure. Must be two
class C4ValueCompiler : public StdCompiler
{
public:
	C4ValueCompiler(const char **pszNames, int iNameCnt, int iEntryNr)
		: pszNames(pszNames), iNameCnt(iNameCnt), iEntryNr(iEntryNr) {}

	virtual bool isCompiler() override { return false; }
	virtual bool hasNaming() override { return true; }
	virtual bool isVerbose() override { return false; }

	virtual NameGuard Name(const char *szName) override
	{
		// match possible? (no match yet / continued match)
		if (!iMatchStart || haveCurrentMatch())
			// already got all names?
			if (!haveCompleteMatch())
				// check name
				if (SEqual(pszNames[iMatchCount], szName))
				{
					// got match
					if (!iMatchCount) iMatchStart = iDepth + 1;
					iMatchCount++;
				}
		iDepth++;
		return {this, true};
	}

	virtual bool Default(const char *szName) override
	{
		// Always process values even if they are default!
		return false;
	}

	virtual void NameEnd(bool fBreak = false) override
	{
		// end of matched name section?
		if (haveCurrentMatch())
		{
			iMatchCount--;
			if (!iMatchCount) iMatchStart = 0;
		}
		iDepth--;
	}

	virtual void Begin() override
	{
		// set up
		iDepth = iMatchStart = iMatchCount = 0;
	}

protected:
	// value function forward to be overwritten by get or set compiler
	virtual void ProcessInt(int32_t &rInt) = 0;
	virtual void ProcessBool(bool &rBool) = 0;
	virtual void ProcessChar(char &rChar) = 0;
	virtual void ProcessString(char *szString, size_t iMaxLength, bool fIsID) = 0;
	virtual void ProcessString(std::string &str, bool isID) = 0;

public:
	// value functions
	virtual void QWord(int64_t &rInt)   override { if (haveCompleteMatch()) if (!iEntryNr--) { auto i = static_cast<int32_t>(rInt); ProcessInt(i); rInt = i; } }
	virtual void QWord(uint64_t &rInt)  override { if (haveCompleteMatch()) if (!iEntryNr--) { auto i = static_cast<int32_t>(rInt); ProcessInt(i); rInt = i; } }
	virtual void DWord(int32_t &rInt)   override { if (haveCompleteMatch()) if (!iEntryNr--) ProcessInt(rInt); }
	virtual void DWord(uint32_t &rInt)  override { if (haveCompleteMatch()) if (!iEntryNr--) { int32_t i = rInt;   ProcessInt(i); rInt = i; } }
	virtual void Word(int16_t &rShort)  override { if (haveCompleteMatch()) if (!iEntryNr--) { int32_t i = rShort; ProcessInt(i); rShort = i; } }
	virtual void Word(uint16_t &rShort) override { if (haveCompleteMatch()) if (!iEntryNr--) { int32_t i = rShort; ProcessInt(i); rShort = i; } }
	virtual void Byte(int8_t &rByte)    override { if (haveCompleteMatch()) if (!iEntryNr--) { int32_t i = rByte;  ProcessInt(i); rByte = i; } }
	virtual void Byte(uint8_t &rByte)   override { if (haveCompleteMatch()) if (!iEntryNr--) { int32_t i = rByte;  ProcessInt(i); rByte = i; } }
	virtual void Boolean(bool &rBool)   override { if (haveCompleteMatch()) if (!iEntryNr--) ProcessBool(rBool); }
	virtual void Character(char &rChar) override { if (haveCompleteMatch()) if (!iEntryNr--) ProcessChar(rChar); }

	// The C4ID-Adaptor will set RCT_ID for it's strings (see C4Id.h), so we don't have to guess the type.
	virtual void String(char *szString, size_t iMaxLength, RawCompileType eType) override
	{
		if (haveCompleteMatch()) if (!iEntryNr--) ProcessString(szString, iMaxLength, eType == StdCompiler::RCT_ID);
	}
	virtual void String(std::string &str, RawCompileType type) override
	{
		if (haveCompleteMatch()) if (!iEntryNr--) ProcessString(str, type == StdCompiler::RCT_ID);
	}
	virtual void Raw(void *pData, size_t iSize, RawCompileType eType = RCT_Escaped) override
	{
		/* C4Script can't handle this */
	}

private:
	// Name(s) of the entry to read
	const char **pszNames;
	int iNameCnt;
	// Number of the element that is to be read
	int iEntryNr;

	// current depth
	int iDepth;
	// match start (where did the first name match?),
	// match count (how many names did match, from that point?)
	int iMatchStart, iMatchCount;

private:
	// match active?
	bool haveCurrentMatch() const { return iDepth + 1 == iMatchStart + iMatchCount; }
	// match complete?
	bool haveCompleteMatch() const { return haveCurrentMatch() && iMatchCount == iNameCnt; }
};

class C4ValueGetCompiler : public C4ValueCompiler
{
private:
	// Result
	C4Value Res;

public:
	C4ValueGetCompiler(const char **pszNames, int iNameCnt, int iEntryNr)
		: C4ValueCompiler(pszNames, iNameCnt, iEntryNr) {}

	// Result-getter
	const C4Value &getResult() const { return Res; }

protected:
	// get values as C4Value
	virtual void ProcessInt(int32_t &rInt) override { Res = C4VInt(rInt); }
	virtual void ProcessBool(bool &rBool) override { Res = C4VBool(rBool); }
	virtual void ProcessChar(char &rChar) override { Res = C4VString(std::format("{}", rChar).c_str()); }

	virtual void ProcessString(char *szString, size_t iMaxLength, bool fIsID) override
	{
		Res = (fIsID ? C4VID(C4Id(szString)) : C4VString(szString));
	}
	virtual void ProcessString(std::string &str, bool fIsID) override
	{
		Res = (fIsID ? C4VID(C4Id(str.c_str())) : C4VString(str.c_str()));
	}
};

class C4ValueSetCompiler : public C4ValueCompiler
{
private:
	C4Value Val; // value to which the setting should be set
	bool fSuccess; // set if the value could be set successfully
	int32_t iRuntimeWriteAllowed; // if >0, runtime writing of values is allowed

public:
	C4ValueSetCompiler(const char **pszNames, int iNameCnt, int iEntryNr, const C4Value &rSetVal)
		: C4ValueCompiler(pszNames, iNameCnt, iEntryNr), Val(rSetVal), fSuccess(false), iRuntimeWriteAllowed(0) {}

	// Query successful setting
	bool getSuccess() const { return fSuccess; }

	virtual void setRuntimeWritesAllowed(int32_t iChange) override { iRuntimeWriteAllowed += iChange; }

protected:
	// set values as C4Value, only if type matches or is convertible
	virtual void ProcessInt(int32_t &rInt) override { if (iRuntimeWriteAllowed > 0 && Val.ConvertTo(C4V_Int)) { rInt = Val.getInt(); fSuccess = true; } }
	virtual void ProcessBool(bool &rBool) override { if (iRuntimeWriteAllowed > 0 && Val.ConvertTo(C4V_Bool)) { rBool = Val.getBool(); fSuccess = true; } }
	virtual void ProcessChar(char &rChar) override { C4String *s; if (iRuntimeWriteAllowed > 0 && (s = Val.getStr())) { rChar = s->Data.getData() ? *s->Data.getData() : 0; fSuccess = true; } }

	virtual void ProcessString(char *szString, size_t iMaxLength, bool fIsID) override
	{
		if (iRuntimeWriteAllowed <= 0 || !Val.ConvertTo(fIsID ? C4V_C4ID : C4V_String)) return;
		if (fIsID)
		{
			assert(iMaxLength >= 4); // fields that should carry an ID are guaranteed to have a buffer that's large enough
			GetC4IdText(Val.getC4ID(), szString);
		}
		else
		{
			C4String *s = Val.getStr(); if (!s) return;
			StdStrBuf &rsBuf = s->Data;
			if (rsBuf.getData()) SCopy(rsBuf.getData(), szString, iMaxLength); else *szString = 0;
		}
		fSuccess = true;
	}

	virtual void ProcessString(std::string &str, bool isID) override
	{
		if (iRuntimeWriteAllowed <= 0 || !Val.ConvertTo(isID ? C4V_C4ID : C4V_String)) return;
		if (isID)
		{
			assert(str.size() >= 4); // fields that should carry an ID are guaranteed to have a buffer that's large enough
			char buf[5];
			GetC4IdText(Val.getC4ID(), buf);
			str = buf;
		}
		else
		{
			C4String *s = Val.getStr(); if (!s) return;
			str = s->Data.getData();
		}
		fSuccess = true;
	}
};

// Use the compiler to find a named value in a structure
template <class T>
C4Value GetValByStdCompiler(const char *strEntry, const char *strSection, int iEntryNr, const T &rFrom)
{
	// Set up name array, create compiler
	const char *szNames[2] = { strSection ? strSection : strEntry, strSection ? strEntry : nullptr };
	C4ValueGetCompiler Comp(szNames, strSection ? 2 : 1, iEntryNr);

	// Compile
	try
	{
		Comp.Decompile(rFrom);
		return Comp.getResult();
	}
	// Should not happen, catch it anyway.
	catch (const StdCompiler::Exception &)
	{
		return C4VNull;
	}
}

// Use the compiler to set a named value in a structure
template <class T>
bool SetValByStdCompiler(const char *strEntry, const char *strSection, int iEntryNr, const T &rTo, const C4Value &rvNewVal)
{
	// Set up name array, create compiler
	const char *szNames[2] = { strSection ? strSection : strEntry, strSection ? strEntry : nullptr };
	C4ValueSetCompiler Comp(szNames, strSection ? 2 : 1, iEntryNr, rvNewVal);

	// Compile
	try
	{
		Comp.Decompile(rTo);
		return Comp.getSuccess();
	}
	// Should not happen, catch it anyway.
	catch (const StdCompiler::Exception &)
	{
		return false;
	}
}

static C4Value FnGetDefCoreVal(C4AulContext *cthr, C4String *strEntry, C4String *section, C4ID idDef, C4ValueInt iEntryNr)
{
	const char *strSection = FnStringPar(section);
	if (strSection && !*strSection) strSection = nullptr;

	if (!idDef && cthr->Def) idDef = cthr->Def->id;
	if (!idDef) return C4VNull;

	C4Def *pDef = C4Id2Def(idDef);
	if (!pDef) return C4VNull;

	return GetValByStdCompiler(FnStringPar(strEntry), strSection, iEntryNr, mkNamingAdapt(*pDef, "DefCore"));
}

static C4Value FnGetObjectVal(C4String *strEntry, C4String *section, Required<C4ObjectOrThis> pObj, C4ValueInt iEntryNr)
{
	const char *strSection = FnStringPar(section);
	if (!*strSection) strSection = nullptr;

	// get value
	return GetValByStdCompiler(FnStringPar(strEntry), strSection, iEntryNr, mkNamingAdapt(**pObj, "Object"));
}

static C4Value FnGetObjectInfoCoreVal(C4String *strEntry, C4String *section, Required<C4ObjectOrThis> pObj, C4ValueInt iEntryNr)
{
	const char *strSection = FnStringPar(section);
	if (strSection && !*strSection) strSection = nullptr;

	// get obj info
	C4ObjectInfo *pObjInfo = pObj->Info;

	if (!pObjInfo) return C4VNull;

	// get obj info core
	C4ObjectInfoCore *pObjInfoCore = static_cast<C4ObjectInfoCore *>(pObjInfo);

	// get value
	return GetValByStdCompiler(FnStringPar(strEntry), strSection, iEntryNr, mkNamingAdapt(*pObjInfoCore, "ObjectInfo"));
}

static C4Value FnGetActMapVal(C4AulContext *cthr, C4String *strEntry, C4String *action, C4ID idDef, C4ValueInt iEntryNr)
{
	if (!idDef && cthr->Def) idDef = cthr->Def->id;
	if (!idDef) return C4VNull;

	C4Def *pDef = C4Id2Def(idDef);

	if (!pDef) return C4VNull;

	C4ActionDef *pAct = pDef->ActMap;

	if (!pAct) return C4VNull;

	const char *strAction = FnStringPar(action);
	C4ValueInt iAct;
	for (iAct = 0; iAct < pDef->ActNum; iAct++, pAct++)
		if (SEqual(pAct->Name, strAction))
			break;

	// not found?
	if (iAct >= pDef->ActNum)
		return C4VNull;

	// get value
	return GetValByStdCompiler(FnStringPar(strEntry), nullptr, iEntryNr, *pAct);
}

static C4Value FnGetScenarioVal(C4String *strEntry, C4String *section, C4ValueInt iEntryNr)
{
	const char *strSection = FnStringPar(section);
	if (strSection && !*strSection) strSection = nullptr;

	return GetValByStdCompiler(FnStringPar(strEntry), strSection, iEntryNr, mkParAdapt(Game.C4S, false));
}

static C4Value FnGetPlayerVal(C4String *strEntry, C4String *section, C4Player &player, C4ValueInt iEntryNr)
{
	const char *strSection = FnStringPar(section);
	if (strSection && !*strSection) strSection = nullptr;

	// get value
	return GetValByStdCompiler(FnStringPar(strEntry), strSection, iEntryNr, mkNamingAdapt(player, "Player"));
}

static C4Value FnGetPlayerInfoCoreVal(C4String *strEntry, C4String *section, C4Player &player, C4ValueInt iEntryNr)
{
	const char *strSection = FnStringPar(section);
	if (strSection && !*strSection) strSection = nullptr;

	// get plr info core
	C4PlayerInfoCore &playerInfoCore = static_cast<C4PlayerInfoCore &>(player);

	// get value
	return GetValByStdCompiler(FnStringPar(strEntry), strSection, iEntryNr, playerInfoCore);
}

static C4Value FnGetMaterialVal(C4String *strEntry, C4String *section, C4ValueInt iMat, C4ValueInt iEntryNr)
{
	const char *strSection = FnStringPar(section);
	if (strSection && !*strSection) strSection = nullptr;

	if (iMat < 0 || iMat >= Game.Material.Num) return C4VNull;

	// get material
	C4Material *pMaterial = &Game.Material.Map[iMat];

	// get plr info core
	C4MaterialCore *pMaterialCore = static_cast<C4MaterialCore *>(pMaterial);

	// material core implicates section "Material"
	if (!SEqual(strSection, "Material")) return C4VNull;

	// get value
	return GetValByStdCompiler(FnStringPar(strEntry), nullptr, iEntryNr, *pMaterialCore);
}

static bool FnCloseMenu(Required<C4ObjectOrThis> pObj)
{
	return pObj->CloseMenu(true);
}

static C4ValueInt FnGetMenuSelection(Required<C4ObjectOrThis, -1> pObj)
{
	if (!pObj->Menu || !pObj->Menu->IsActive()) return -1;
	return pObj->Menu->GetSelection();
}

static bool FnResortObjects(C4AulContext *cthr, C4String &szFunc, Default<C4ValueInt, C4D_SortLimit, true> Category)
{
	// safety
	if (!cthr->Caller) return false;
	// get function
	C4AulFunc *pFn = cthr->Caller->Func->GetLocalSFunc(FnStringPar(szFunc));
	if (!pFn)
		throw C4AulExecError(cthr->Obj, std::format("ResortObjects: Resort function {} not found", FnStringPar(szFunc)));
	// create object resort
	C4ObjResort *pObjRes = new C4ObjResort();
	pObjRes->Category = Category;
	pObjRes->OrderFunc = pFn;
	// insert into game resort proc list
	pObjRes->Next = Game.Objects.ResortProc;
	Game.Objects.ResortProc = pObjRes;
	// success, so far
	return true;
}

static bool FnResortObject(C4AulContext *cthr, C4String &szFunc, Required<C4ObjectOrThis> pObj)
{
	// safety
	if (!cthr->Caller) return false;
	// get function
	C4AulFunc *pFn = cthr->Caller->Func->GetLocalSFunc(FnStringPar(szFunc));
	if (!pFn)
		throw C4AulExecError(cthr->Obj, std::format("ResortObjects: Resort function {} not found", FnStringPar(szFunc)));
	// create object resort
	C4ObjResort *pObjRes = new C4ObjResort();
	pObjRes->OrderFunc = pFn;
	pObjRes->pSortObj = pObj;
	// insert into game resort proc list
	pObjRes->Next = Game.Objects.ResortProc;
	Game.Objects.ResortProc = pObjRes;
	// success, so far
	return true;
}

static C4ValueInt FnGetChar(C4String &string, C4ValueInt iIndex)
{
	const char *szText = FnStringPar(string);
	// loop and check for end of string
	for (C4ValueInt i = 0; i < iIndex; i++, szText++)
		if (!*szText) return 0;
	// return indiced character value
	return static_cast<unsigned char>(*szText);
}

static bool FnSetGraphics(C4String *pGfxName, Required<C4ObjectOrThis> pObj, C4ID idSrcGfx, C4ValueInt iOverlayID, C4ValueInt iOverlayMode, C4String *pAction, C4ValueInt dwBlitMode, C4Object *pOverlayObject)
{
	if (!pObj->Status) return false;
	// get def for source graphics
	C4Def *pSrcDef = nullptr;
	if (idSrcGfx) if (!(pSrcDef = C4Id2Def(idSrcGfx))) return false;
	// setting overlay?
	if (iOverlayID)
	{
		// any overlays must be positive for now
		if (iOverlayID < 0) { LogNTr(spdlog::level::err, "SetGraphics: Background overlays not implemented!"); return false; }
		// deleting overlay?
		C4DefGraphics *pGrp;
		if (iOverlayMode == C4GraphicsOverlay::MODE_Object)
		{
			if (!pOverlayObject) return pObj->RemoveGraphicsOverlay(iOverlayID);
		}
		else
		{
			if (!pSrcDef) return pObj->RemoveGraphicsOverlay(iOverlayID);
			pGrp = pSrcDef->Graphics.Get(FnStringPar(pGfxName));
			if (!pGrp) return false;
		}
		// adding/setting
		C4GraphicsOverlay *pOverlay = pObj->GetGraphicsOverlay(iOverlayID, true);
		switch (iOverlayMode)
		{
		case C4GraphicsOverlay::MODE_Base:
			pOverlay->SetAsBase(pGrp, dwBlitMode);
			break;

		case C4GraphicsOverlay::MODE_Action:
			pOverlay->SetAsAction(pGrp, FnStringPar(pAction), dwBlitMode);
			break;

		case C4GraphicsOverlay::MODE_IngamePicture:
			pOverlay->SetAsIngamePicture(pGrp, dwBlitMode);
			break;

		case C4GraphicsOverlay::MODE_Picture:
			pOverlay->SetAsPicture(pGrp, dwBlitMode);
			break;

		case C4GraphicsOverlay::MODE_Object:
			if (pOverlayObject && !pOverlayObject->Status) pOverlayObject = nullptr;
			pOverlay->SetAsObject(pOverlayObject, dwBlitMode);
			break;

		case C4GraphicsOverlay::MODE_ExtraGraphics:
			pOverlay->SetAsExtraGraphics(pGrp, dwBlitMode);
			break;

		default:
			DebugLog("SetGraphics: Invalid overlay mode");
			pOverlay->SetAsBase(nullptr, 0); // make invalid, so it will be removed
			break;
		}
		// remove if invalid
		if (!pOverlay->IsValid(pObj))
		{
			pObj->RemoveGraphicsOverlay(iOverlayID);
			return false;
		}
		// Okay, valid overlay set!
		return true;
	}
	// no overlay: Base graphics
	// set graphics - pSrcDef==nullptr defaults to pObj->pDef
	return pObj->SetGraphics(FnStringPar(pGfxName), pSrcDef);
}

static C4ValueInt FnGetDefBottom(Required<C4ObjectOrThis> pObj)
{
	return pObj->y + pObj->Def->Shape.y + pObj->Def->Shape.Hgt;
}

static bool FnSetMaterialColor(C4ValueInt iMat, C4ValueInt iClr1R, C4ValueInt iClr1G, C4ValueInt iClr1B, C4ValueInt iClr2R, C4ValueInt iClr2G, C4ValueInt iClr2B, C4ValueInt iClr3R, C4ValueInt iClr3G, C4ValueInt iClr3B)
{
	// get mat
	if (!MatValid(iMat)) return false;
	C4Material *pMat = &Game.Material.Map[iMat];
	// newgfx: emulate by landscape modulation - enlightment not allowed...
	uint32_t dwBack, dwMod = GetClrModulation(C4RGB(pMat->Color[0], pMat->Color[1], pMat->Color[2]), C4RGB(iClr1R, iClr1G, iClr1B), dwBack);
	dwMod &= 0xffffff;
	if (!dwMod) dwMod = 1;
	if (dwMod == 0xffffff) dwMod = 0;
	Game.Landscape.SetModulation(dwMod);
	// done
	return true;
}

static std::optional<C4ValueInt> FnGetMaterialColor(C4ValueInt iMat, C4ValueInt iNum, C4ValueInt iChannel)
{
	// get mat
	if (!MatValid(iMat)) return {};
	C4Material *pMat = &Game.Material.Map[iMat];
	// get color
	return pMat->Color[iNum * 3 + iChannel];
}

static C4String *FnMaterialName(C4ValueInt iMat)
{
	// mat valid?
	if (!MatValid(iMat)) return nullptr;
	// return mat name
	return String(Game.Material.Map[iMat].Name);
}

static bool FnSetMenuSize(C4ValueInt iCols, C4ValueInt iRows, Required<C4ObjectOrThis> pObj)
{
	// get menu
	C4Menu *pMnu = pObj->Menu;
	if (!pMnu || !pMnu->IsActive()) return false;
	pMnu->SetSize(BoundBy<C4ValueInt>(iCols, 0, 50), BoundBy<C4ValueInt>(iRows, 0, 50));
	return true;
}

static C4String *FnGetNeededMatStr(C4AulContext *cthr, Required<C4ObjectOrThis> pObj)
{
	return String(pObj->GetNeededMatStr(cthr->Obj).c_str());
}

static C4Value FnEval(C4AulContext *cthr, C4String *strScript)
{
	// execute script in the same object
	C4AulScriptStrict Strict = C4AulScriptStrict::MAXSTRICT;
	if (cthr->Caller)
		Strict = cthr->Caller->Func->pOrgScript->Strict;
	if (cthr->Obj)
		return cthr->Obj->Def->Script.DirectExec(cthr->Obj, FnStringPar(strScript), "eval", true, Strict);
	else if (cthr->Def)
		return cthr->Def->Script.DirectExec(nullptr, FnStringPar(strScript), "eval", true, Strict);
	else
		return Game.Script.DirectExec(nullptr, FnStringPar(strScript), "eval", true, Strict);
}

static bool FnLocateFunc(C4AulContext *cthr, C4String *funcname, C4Object *pObj, C4ID idDef)
{
	// safety
	if (!funcname || !funcname->Data.getData())
	{
		LogNTr(spdlog::level::err, "No func name");
		return false;
	}
	// determine script context
	C4AulScript *pCheckScript;
	if (pObj)
	{
		pCheckScript = &pObj->Def->Script;
	}
	else if (idDef)
	{
		C4Def *pDef = C4Id2Def(idDef);
		if (!pDef) { LogNTr(spdlog::level::err, "Invalid or unloaded def"); return false; }
		pCheckScript = &pDef->Script;
	}
	else
	{
		if (!cthr || !cthr->Caller || !cthr->Caller->Func || !cthr->Caller->Func->Owner)
		{
			LogNTr(spdlog::level::err, "No valid script context");
			return false;
		}
		pCheckScript = cthr->Caller->Func->Owner;
	}
	// get function by name
	C4AulFunc *pFunc = pCheckScript->GetFuncRecursive(funcname->Data.getData());
	if (!pFunc)
	{
		LogNTr(spdlog::level::err, "Func {} not found", funcname->Data.getData());
	}
	else
	{
		const char *szPrefix = "";
		while (pFunc)
		{
			C4AulScriptFunc *pSFunc = pFunc->SFunc();
			if (!pSFunc)
			{
				LogNTr("{}{} (engine)", szPrefix, +pFunc->Name);
			}
			else if (!pSFunc->pOrgScript)
			{
				LogNTr("{}{} (no owner)", szPrefix, +pSFunc->Name);
			}
			else
			{
				int32_t iLine = SGetLine(pSFunc->pOrgScript->GetScript(), pSFunc->Script);
				LogNTr("{}{} ({}:{})", szPrefix, +pFunc->Name, pSFunc->pOrgScript->ScriptName.c_str(), static_cast<int>(iLine));
			}
			// next func in overload chain
			pFunc = pSFunc ? pSFunc->OwnerOverloaded : nullptr;
			szPrefix = "overloads ";
		}
	}
	return true;
}

static C4Value FnVarN(C4AulContext *cthr, C4String *name)
{
	const char *strName = FnStringPar(name);

	if (!cthr->Caller) return C4VNull;

	// find variable
	int32_t iID = cthr->Caller->Func->VarNamed.GetItemNr(strName);
	if (iID < 0)
		return C4VNull;

	// return reference on variable
	return cthr->Caller->Vars[iID].GetRef();
}

static C4Value FnLocalN(C4String *name, Required<C4ObjectOrThis> pObj)
{
	const char *strName = FnStringPar(name);

	// find variable
	C4Value *pVarN = pObj->LocalNamed.GetItem(strName);

	if (!pVarN) return C4VNull;

	// return reference on variable
	return pVarN->GetRef();
}

static C4Value FnGlobalN(C4String *name)
{
	const char *strName = FnStringPar(name);

	// find variable
	C4Value *pVarN = Game.ScriptEngine.GlobalNamed.GetItem(strName);

	if (!pVarN) return C4VNull;

	// return reference on variable
	return pVarN->GetRef();
}

static std::optional<C4ValueInt> FnGetTime(C4AulContext *)
{
	// check network, record, etc
	if (Game.Control.SyncMode()) return {};
	return {timeGetTime()};
}

static std::optional<C4ValueInt> FnGetSystemTime(C4ValueInt iWhat)
{
	// check network, record, etc
	if (Game.Control.SyncMode()) return {};
	// check bounds
	if (!Inside<C4ValueInt>(iWhat, 0, 7)) return {};
#ifdef _WIN32
	SYSTEMTIME time;
	GetLocalTime(&time);
	// return queried value
	return {*(((WORD *)&time) + iWhat)};
#else
	struct timeval tv;
	if (gettimeofday(&tv, nullptr)) return {};
	if (iWhat == 7) return tv.tv_usec / 1000;
	struct tm *time;
	time = localtime(&tv.tv_sec);
	switch (iWhat)
	{
		case 0: return {time->tm_year + 1900};
		case 1: return {time->tm_mon + 1};
		case 2: return {time->tm_wday};
		case 3: return {time->tm_mday};
		case 4: return {time->tm_hour};
		case 5: return {time->tm_min};
		case 6: return {time->tm_sec};
	}

	return {};
#endif
}

static C4Value FnSetPlrExtraData(C4Player &player, C4String *DataName, C4Value Data)
{
	const char *strDataName = FnStringPar(DataName);

	if (!strDataName || !strDataName[0]) return C4VNull;
	if (!StdCompiler::IsIdentifier(strDataName))
	{
		StdStrBuf name{strDataName};
		name.EscapeString();
		DebugLog(spdlog::level::warn, "SetPlrExtraData: Ignoring invalid data name \"{}\"! Only alphanumerics, _ and - are allowed.", name.getData());
		return C4VNull;
	}

	// do not allow data type C4V_String or C4V_C4Object
	if (Data.GetType() != C4V_Any &&
		Data.GetType() != C4V_Int &&
		Data.GetType() != C4V_Bool &&
		Data.GetType() != C4V_C4ID) return C4VNull;
	// no name list created yet?
	if (!player.ExtraData.pNames)
		// create name list
		player.ExtraData.CreateTempNameList();
	// data name already exists?
	C4ValueInt ival;
	if ((ival = player.ExtraData.pNames->GetItemNr(strDataName)) != -1)
		player.ExtraData[ival] = Data;
	else
	{
		// add name
		player.ExtraData.pNames->AddName(strDataName);
		// get val id & set
		if ((ival = player.ExtraData.pNames->GetItemNr(strDataName)) == -1) return C4VNull;
		player.ExtraData[ival] = Data;
	}
	// ok, return the value that has been set
	return Data;
}

static C4Value FnGetPlrExtraData(C4Player &player, C4String *DataName)
{
	const char *strDataName = FnStringPar(DataName);
	// no name list?
	if (!player.ExtraData.pNames) return C4VNull;
	C4ValueInt ival;
	if ((ival = player.ExtraData.pNames->GetItemNr(strDataName)) == -1) return C4VNull;
	// return data
	return player.ExtraData[ival];
}

static C4Value FnSetCrewExtraData(Required<C4ObjectOrThis> pCrew, C4String *dataName, C4Value Data)
{
	const char *strDataName = FnStringPar(dataName);

	if (!strDataName || !strDataName[0]) return C4VNull;
	if (!StdCompiler::IsIdentifier(strDataName))
	{
		StdStrBuf name{strDataName};
		name.EscapeString();
		DebugLog(spdlog::level::err, "SetCrewExtraData: Ignoring invalid data name \"{}\"! Only alphanumerics, _ and - are allowed.", name.getData());
		return C4VNull;
	}

	// valid crew with info? (for great nullpointer prevention)
	if (!pCrew->Info) return C4VNull;
	// do not allow data type C4V_String or C4V_C4Object
	if (Data.GetType() != C4V_Any &&
		Data.GetType() != C4V_Int &&
		Data.GetType() != C4V_Bool &&
		Data.GetType() != C4V_C4ID) return C4VNull;
	// get pointer on info...
	C4ObjectInfo *pInfo = pCrew->Info;
	// no name list created yet?
	if (!pInfo->ExtraData.pNames)
		// create name list
		pInfo->ExtraData.CreateTempNameList();
	// data name already exists?
	C4ValueInt ival;
	if ((ival = pInfo->ExtraData.pNames->GetItemNr(strDataName)) != -1)
		pInfo->ExtraData[ival] = Data;
	else
	{
		// add name
		pInfo->ExtraData.pNames->AddName(strDataName);
		// get val id & set
		if ((ival = pInfo->ExtraData.pNames->GetItemNr(strDataName)) == -1) return C4VNull;
		pInfo->ExtraData[ival] = Data;
	}
	// ok, return the value that has been set
	return Data;
}

static C4Value FnGetCrewExtraData(Required<C4ObjectOrThis> pCrew, C4String *dataName)
{
	const char *strDataName = FnStringPar(dataName);
	// valid crew with info?
	if (!pCrew->Info) return C4VNull;
	// get pointer on info...
	C4ObjectInfo *pInfo = pCrew->Info;
	// no name list?
	if (!pInfo->ExtraData.pNames) return C4VNull;
	C4ValueInt ival;
	if ((ival = pInfo->ExtraData.pNames->GetItemNr(strDataName)) == -1) return C4VNull;
	// return data
	return pInfo->ExtraData[ival];
}

static C4ValueInt FnDrawMatChunks(C4ValueInt tx, C4ValueInt ty, C4ValueInt twdt, C4ValueInt thgt, C4ValueInt icntx, C4ValueInt icnty, C4String *strMaterial, C4String *strTexture, bool bIFT)
{
	return Game.Landscape.DrawChunks(tx, ty, twdt, thgt, icntx, icnty, FnStringPar(strMaterial), FnStringPar(strTexture), bIFT != 0);
}

static bool FnGetCrewEnabled(Required<C4ObjectOrThis> pObj)
{
	// return status
	return !pObj->CrewDisabled;
}

static bool FnSetCrewEnabled(bool fEnabled, Required<C4ObjectOrThis> pObj)
{
	// set status
	pObj->CrewDisabled = !fEnabled;
	// deselect
	if (!fEnabled)
	{
		pObj->Select = false;
		C4Player *pOwner;
		if (pOwner = Game.Players.Get(pObj->Owner))
		{
			// if viewed player cursor gets deactivated and no new cursor is found, follow the old in target mode
			bool fWasCursorMode = (pOwner->ViewMode == C4PVM_Cursor);
			if (pOwner->Cursor == *pObj)
				pOwner->AdjustCursorCommand();
			if (!pOwner->ViewCursor && !pOwner->Cursor && fWasCursorMode)
				pOwner->SetViewMode(C4PVM_Target, pObj);
		}
	}
	// success
	return true;
}

static C4ValueInt FnDrawMap(C4ValueInt iX, C4ValueInt iY, C4ValueInt iWdt, C4ValueInt iHgt, C4String *szMapDef)
{
	// draw it!
	return Game.Landscape.DrawMap(iX, iY, iWdt, iHgt, FnStringPar(szMapDef));
}

static C4ValueInt FnDrawDefMap(C4ValueInt iX, C4ValueInt iY, C4ValueInt iWdt, C4ValueInt iHgt, C4String *szMapDef)
{
	// draw it!
	return Game.Landscape.DrawDefMap(iX, iY, iWdt, iHgt, FnStringPar(szMapDef));
}

static bool FnCreateParticle(C4AulContext *cthr, C4String *szName, C4ValueInt iX, C4ValueInt iY, C4ValueInt iXDir, C4ValueInt iYDir, C4ValueInt a, C4ValueInt b, C4Object *pObj, bool fBack)
{
	// safety
	if (pObj && !pObj->Status) return false;
	MakePositionRelative(cthr, iX, iY);
	// get particle
	C4ParticleDef *pDef = Game.Particles.GetDef(FnStringPar(szName));
	if (!pDef) return false;
	// create
	Game.Particles.Create(pDef, static_cast<float>(iX), static_cast<float>(iY), static_cast<float>(iXDir) / 10.0f, static_cast<float>(iYDir) / 10.0f, static_cast<float>(a) / 10.0f, b, pObj ? (fBack ? &pObj->BackParticles : &pObj->FrontParticles) : nullptr, pObj);
	// success, even if not created
	return true;
}

static bool FnCastAParticles(C4AulContext *cthr, C4String *szName, C4ValueInt iAmount, C4ValueInt iLevel, C4ValueInt iX, C4ValueInt iY, C4ValueInt a0, C4ValueInt a1, C4ValueInt b0, C4ValueInt b1, C4Object *pObj, bool fBack)
{
	// safety
	if (pObj && !pObj->Status) return false;
	MakePositionRelative(cthr, iX, iY);
	// get particle
	C4ParticleDef *pDef = Game.Particles.GetDef(FnStringPar(szName));
	if (!pDef) return false;
	// cast
	Game.Particles.Cast(pDef, iAmount, static_cast<float>(iX), static_cast<float>(iY), iLevel, static_cast<float>(a0) / 10.0f, b0, static_cast<float>(a1) / 10.0f, b1, pObj ? (fBack ? &pObj->BackParticles : &pObj->FrontParticles) : nullptr, pObj);
	// success, even if not created
	return true;
}

static bool FnCastParticles(C4AulContext *cthr, C4String *szName, C4ValueInt iAmount, C4ValueInt iLevel, C4ValueInt iX, C4ValueInt iY, C4ValueInt a0, C4ValueInt a1, C4ValueInt b0, C4ValueInt b1, C4Object *pObj)
{
	return FnCastAParticles(cthr, szName, iAmount, iLevel, iX, iY, a0, a1, b0, b1, pObj, false);
}

static bool FnCastBackParticles(C4AulContext *cthr, C4String *szName, C4ValueInt iAmount, C4ValueInt iLevel, C4ValueInt iX, C4ValueInt iY, C4ValueInt a0, C4ValueInt a1, C4ValueInt b0, C4ValueInt b1, C4Object *pObj)
{
	return FnCastAParticles(cthr, szName, iAmount, iLevel, iX, iY, a0, a1, b0, b1, pObj, true);
}

static bool FnPushParticles(C4String *szName, C4ValueInt iAX, C4ValueInt iAY)
{
	// particle given?
	C4ParticleDef *pDef = nullptr;
	if (szName)
	{
		pDef = Game.Particles.GetDef(FnStringPar(szName));
		if (!pDef) return false;
	}
	// push them
	Game.Particles.Push(pDef, static_cast<float>(iAX) / 10.0f, static_cast<float>(iAY) / 10.0f);
	// success
	return true;
}

static bool FnClearParticles(C4String *szName, C4Object *pObj)
{
	// particle given?
	C4ParticleDef *pDef = nullptr;
	if (szName)
	{
		pDef = Game.Particles.GetDef(FnStringPar(szName));
		if (!pDef) return false;
	}
	// delete them
	if (pObj)
	{
		pObj->FrontParticles.Remove(pDef);
		pObj->BackParticles.Remove(pDef);
	}
	else
		Game.Particles.GlobalParticles.Remove(pDef);
	// success
	return true;
}

#define SkyPar_KEEP -163764

static void FnSetSkyParallax(C4ValueInt iMode, C4ValueInt iParX, C4ValueInt iParY, C4ValueInt iXDir, C4ValueInt iYDir, C4ValueInt iX, C4ValueInt iY)
{
	// set all parameters that aren't SkyPar_KEEP
	if (iMode != SkyPar_KEEP)
		if (Inside<C4ValueInt>(iMode, 0, 1)) Game.Landscape.Sky.ParallaxMode = iMode;
	if (iParX != SkyPar_KEEP && iParX) Game.Landscape.Sky.ParX = iParX;
	if (iParY != SkyPar_KEEP && iParY) Game.Landscape.Sky.ParY = iParY;
	if (iXDir != SkyPar_KEEP) Game.Landscape.Sky.xdir = itofix(iXDir);
	if (iYDir != SkyPar_KEEP) Game.Landscape.Sky.ydir = itofix(iYDir);
	if (iX != SkyPar_KEEP) Game.Landscape.Sky.x = itofix(iX);
	if (iY != SkyPar_KEEP) Game.Landscape.Sky.y = itofix(iY);
}

static bool FnDoCrewExp(C4ValueInt iChange, Required<C4ObjectOrThis> pObj)
{
	// do exp
	pObj->DoExperience(iChange);
	// success
	return true;
}

static C4ValueInt FnReloadDef(C4AulContext *ctx, C4ID idDef)
{
	// get def
	C4Def *pDef = nullptr;
	if (!idDef)
	{
		// no def given: local def
		if (ctx->Obj) pDef = ctx->Obj->Def;
	}
	else
		// def by ID
		pDef = Game.Defs.ID2Def(idDef);
	// safety
	if (!pDef) return false;
	// perform reload
	return Game.ReloadDef(pDef->id, C4D_Load_RX);
}

static C4ValueInt FnReloadParticle(C4String *szParticleName)
{
	// perform reload
	return Game.ReloadParticle(FnStringPar(szParticleName));
}

static void FnResetGamma(C4ValueInt iRampIndex)
{
	Game.GraphicsSystem.SetGamma(0x000000, 0x808080, 0xffffff, iRampIndex);
}

static C4ValueHash *FnGetPath(C4ValueInt iFromX, C4ValueInt iFromY, C4ValueInt iToX, C4ValueInt iToY)
{
	struct Waypoint
	{
		int32_t x = 0;
		int32_t y = 0;
		C4Object *obj = nullptr;
	};

	struct PathInfo
	{
		std::vector<Waypoint> path;
		int32_t length = 0;
	};

	auto SetWaypoint = [](int32_t x, int32_t y, intptr_t transferTarget, intptr_t pathInfo) -> bool
	{
		auto *target = reinterpret_cast<C4Object *>(transferTarget);
		auto *pathinfo = reinterpret_cast<PathInfo *>(pathInfo);

		const Waypoint &last = pathinfo->path.back();
		pathinfo->length += Distance(last.x, last.y, x, y);

		pathinfo->path.push_back({x, y, target});
		return true;
	};

	PathInfo pathinfo;
	pathinfo.path.push_back({static_cast<int32_t>(iFromX), static_cast<int32_t>(iFromY), nullptr});

	if (!Game.PathFinder.Find(iFromX, iFromY, iToX, iToY, SetWaypoint, reinterpret_cast<intptr_t>(&pathinfo)))
	{
		return nullptr;
	}

	SetWaypoint(static_cast<int32_t>(iToX), static_cast<int32_t>(iToY), reinterpret_cast<intptr_t>(nullptr), reinterpret_cast<intptr_t>(&pathinfo));

	auto *hash = new C4ValueHash;
	(*hash)[C4VString("Length")] = C4VInt(pathinfo.length);

	auto *array = new C4ValueArray(static_cast<int32_t>(pathinfo.path.size()));

	if (!pathinfo.path.empty())
	{
		for (size_t i = 0; i < pathinfo.path.size(); ++i)
		{
			auto *waypoint = new C4ValueHash;
			(*waypoint)[C4VString("X")] = C4VInt(pathinfo.path[i].x);
			(*waypoint)[C4VString("Y")] = C4VInt(pathinfo.path[i].y);
			if (pathinfo.path[i].obj)
				(*waypoint)[C4VString("TransferTarget")] = C4VObj(pathinfo.path[i].obj);

			(*array)[static_cast<int32_t>(i)] = C4VMap(waypoint);
		}
	}

	(*hash)[C4VString("Waypoints")] = C4VArray(array);

	return hash;
}

static C4ValueInt FnSetTextureIndex(C4String *psMatTex, C4ValueInt iNewIndex, bool fInsert)
{
	if (!Inside(iNewIndex, C4ValueInt{0}, C4ValueInt{255})) return false;
	return Game.Landscape.SetTextureIndex(FnStringPar(psMatTex), uint8_t(iNewIndex), !!fInsert);
}

static void FnSetLandscapePixel(C4AulContext *ctx, C4ValueInt iX, C4ValueInt iY, C4ValueInt dwValue)
{
	MakePositionRelative(ctx, iX, iY);
	// set pixel in 32bit-sfc only
	Game.Landscape.SetPixDw(iX, iY, dwValue);
}

static bool FnSetObjectOrder(C4Object &objBeforeOrAfter, Required<C4ObjectOrThis> pSortObj, bool fSortAfter)
{
	// don’t sort an object before or after itself, it messes up the object list and causes infinite loops
	if (&objBeforeOrAfter == pSortObj) return false;
	// note that no category check is done, so this call might corrupt the main list!
	// the scripter must be wise enough not to call it for objects with different categories
	// create object resort
	C4ObjResort *pObjRes = new C4ObjResort();
	pObjRes->pSortObj = pSortObj;
	pObjRes->pObjBefore = &objBeforeOrAfter;
	pObjRes->fSortAfter = fSortAfter;
	// insert into game resort proc list
	pObjRes->Next = Game.Objects.ResortProc;
	Game.Objects.ResortProc = pObjRes;
	// done, success so far
	return true;
}

static bool FnDrawMaterialQuad(C4String *szMaterial, C4ValueInt iX1, C4ValueInt iY1, C4ValueInt iX2, C4ValueInt iY2, C4ValueInt iX3, C4ValueInt iY3, C4ValueInt iX4, C4ValueInt iY4, bool fSub)
{
	const char *szMat = FnStringPar(szMaterial);
	return !!Game.Landscape.DrawQuad(iX1, iY1, iX2, iY2, iX3, iY3, iX4, iY4, szMat, fSub);
}

static bool FnFightWith(C4Object &target, Required<C4ObjectOrThis> pClonk)
{
	// check OCF
	if (~(target.OCF & pClonk->OCF) & OCF_FightReady) return false;
	// RejectFight callback
	if (target.Call(PSF_RejectFight, {C4VObj(&target)}, true).getBool()) return false;
	if (pClonk->Call(PSF_RejectFight, {C4VObj(pClonk)}, true).getBool()) return false;
	// begin fighting
	ObjectActionFight(pClonk, &target);
	ObjectActionFight(&target, pClonk);
	// success
	return true;
}

static bool FnSetFilmView(C4ValueInt iToPlr)
{
	// check player
	if (!ValidPlr(iToPlr) && iToPlr != NO_OWNER) return false;
	// real switch in replays only
	if (!Game.Control.isReplay()) return true;
	// set new target plr
	if (const auto &viewports = Game.GraphicsSystem.GetViewports(); viewports.size() > 0)
	{
		viewports.front()->Init(iToPlr, true);
	}
	// done, always success (sync)
	return true;
}

static bool FnClearMenuItems(Required<C4ObjectOrThis> pObj)
{
	// check menu
	if (!pObj->Menu) return false;
	// clear the items
	pObj->Menu->ClearItems(true);
	// success
	return true;
}

static C4Object *FnGetObjectLayer(Required<C4ObjectOrThis> pObj)
{
	// get layer object
	return pObj->pLayer;
}

static bool FnSetObjectLayer(C4Object *pNewLayer, Required<C4ObjectOrThis> pObj)
{
	// set layer object
	pObj->pLayer = pNewLayer;
	// set for all contents as well
	for (C4ObjectLink *pLnk = pObj->Contents.First; pLnk; pLnk = pLnk->Next)
		if (C4Object *obj = pLnk->Obj; obj && obj->Status)
			obj->pLayer = pNewLayer;
	// success
	return true;
}

static bool FnSetShape(C4ValueInt iX, C4ValueInt iY, C4ValueInt iWdt, C4ValueInt iHgt, Required<C4ObjectOrThis> pObj)
{
	// update shape
	pObj->Shape.x = iX;
	pObj->Shape.y = iY;
	pObj->Shape.Wdt = iWdt;
	pObj->Shape.Hgt = iHgt;
	// section list needs refresh
	pObj->UpdatePos();
	// done, success
	return true;
}

static bool FnAddMsgBoardCmd(C4AulContext *ctx, C4String &strCommand, C4String &strScript, C4ValueInt iRestriction)
{
	// unrestricted commands cannot be set by direct-exec script (like /script).
	if (iRestriction != C4MessageBoardCommand::C4MSGCMDR_Identifier)
		if (!ctx->Caller || !*ctx->Caller->Func->Name)
			return false;
	C4MessageBoardCommand::Restriction eRestriction;
	switch (iRestriction)
	{
	case C4MessageBoardCommand::C4MSGCMDR_Escaped:    eRestriction = C4MessageBoardCommand::C4MSGCMDR_Escaped;    break;
	case C4MessageBoardCommand::C4MSGCMDR_Plain:      eRestriction = C4MessageBoardCommand::C4MSGCMDR_Plain;      break;
	case C4MessageBoardCommand::C4MSGCMDR_Identifier: eRestriction = C4MessageBoardCommand::C4MSGCMDR_Identifier; break;
	default: return false;
	}
	// add command
	Game.MessageInput.AddCommand(FnStringPar(strCommand), FnStringPar(strScript), eRestriction);
	return true;
}

static bool FnSetGameSpeed(C4AulContext *ctx, Default<C4ValueInt, 38, true> iSpeed)
{
	// league games: disable direct exec (like /speed)
	if (Game.Parameters.isLeague())
		if (!ctx->Caller || ctx->Caller->TemporaryScript)
			return false;
	// safety
	if (!Inside<C4ValueInt>(iSpeed, 1, 1000)) return false;
	// set speed, restart timer
	Application.SetGameTickDelay(1000 / iSpeed);
	return true;
}

static bool FnSetObjDrawTransform(C4ValueInt iA, C4ValueInt iB, C4ValueInt iC, C4ValueInt iD, C4ValueInt iE, C4ValueInt iF, Required<C4ObjectOrThis> pObj, C4ValueInt iOverlayID)
{
	C4DrawTransform *pTransform;
	// overlay?
	if (iOverlayID)
	{
		// set overlay transform
		C4GraphicsOverlay *pOverlay = pObj->GetGraphicsOverlay(iOverlayID, false);
		if (!pOverlay) return false;
		pTransform = pOverlay->GetTransform();
	}
	else
	{
		// set base transform
		pTransform = pObj->pDrawTransform;
		// del transform?
		if (!iB && !iC && !iD && !iF && iA == iE && (!iA || iA == 1000))
		{
			// identity/0 and no transform defined: nothing to do
			if (!pTransform) return true;
			// transform has no flipdir?
			if (pTransform->FlipDir == 1)
			{
				// kill identity-transform, then
				delete pTransform;
				pObj->pDrawTransform = nullptr;
				return true;
			}
			// flipdir must remain: set identity
			pTransform->Set(1, 0, 0, 0, 1, 0, 0, 0, 1);
			return true;
		}
		// create draw transform if not already present
		if (!pTransform) pTransform = pObj->pDrawTransform = new C4DrawTransform();
	}
	// assign values
	pTransform->Set(static_cast<float>(iA) / 1000, static_cast<float>(iB) / 1000, static_cast<float>(iC) / 1000, static_cast<float>(iD) / 1000, static_cast<float>(iE) / 1000, static_cast<float>(iF) / 1000, 0, 0, 1);
	// done, success
	return true;
}

static bool FnSetObjDrawTransform2(C4AulContext *ctx, C4ValueInt iA, C4ValueInt iB, C4ValueInt iC, C4ValueInt iD, C4ValueInt iE, C4ValueInt iF, C4ValueInt iG, C4ValueInt iH, C4ValueInt iI, C4ValueInt iOverlayID)
{
	// local call / safety
	C4Object *pObj = ctx->Obj;
	if (!pObj) return false;
	C4DrawTransform *pTransform;
	// overlay?
	if (iOverlayID)
	{
		// set overlay transform
		C4GraphicsOverlay *pOverlay = pObj->GetGraphicsOverlay(iOverlayID, false);
		if (!pOverlay) return false;
		pTransform = pOverlay->GetTransform();
	}
	else
	{
		// set base transform
		pTransform = pObj->pDrawTransform;
		// create draw transform if not already present
		if (!pTransform) pTransform = pObj->pDrawTransform = new C4DrawTransform(1);
	}
	// assign values
#define L2F(l) (static_cast<float>(l)/1000)
	CBltTransform matrix;
	matrix.Set(L2F(iA), L2F(iB), L2F(iC), L2F(iD), L2F(iE), L2F(iF), L2F(iG), L2F(iH), L2F(iI));
	*pTransform *= matrix;
#undef L2F
	// done, success
	return true;
}

bool SimFlight(C4Fixed &x, C4Fixed &y, C4Fixed &xdir, C4Fixed &ydir, int32_t iDensityMin, int32_t iDensityMax, int32_t iIter);

static std::optional<bool> FnSimFlight(C4Value * pvrX, C4Value * pvrY, C4Value * pvrXDir, C4Value * pvrYDir, Default<C4ValueInt, C4M_Solid> iDensityMin, Default<C4ValueInt, 100> iDensityMax, Default<C4ValueInt, -1> iIter, Default<C4ValueInt, 10> iPrec)
{
	// convert to C4Fixed
	C4Fixed x = itofix(pvrX->getInt()), y = itofix(pvrY->getInt()),
	xdir = itofix(pvrXDir->getInt(), iPrec), ydir = itofix(pvrYDir->getInt(), iPrec);

	// simulate
	if (!SimFlight(x, y, xdir, ydir, iDensityMin, iDensityMax, iIter))
		return {false};

	// write results back
	*pvrX = C4VInt(fixtoi(x)); *pvrY = C4VInt(fixtoi(y));
	*pvrXDir = C4VInt(fixtoi(xdir * iPrec)); *pvrYDir = C4VInt(fixtoi(ydir * iPrec));

	return {true};
}

static bool FnSetPortrait(C4String &strPortrait, Required<C4ObjectOrThis> pTarget, C4ID idSourceDef, bool fPermanent, bool fCopyGfx)
{
	// safety
	const char *szPortrait;
	if (!*(szPortrait = FnStringPar(strPortrait))) return false;
	if (!pTarget->Status || !pTarget->Info) return false;
	// special case: clear portrait
	if (SEqual(szPortrait, C4Portrait_None)) return pTarget->Info->ClearPortrait(!!fPermanent);
	// get source def for portrait
	C4Def *pSourceDef;
	if (idSourceDef) pSourceDef = Game.Defs.ID2Def(idSourceDef); else pSourceDef = pTarget->Def;
	if (!pSourceDef) return false;
	// special case: random portrait
	if (SEqual(szPortrait, C4Portrait_Random)) return pTarget->Info->SetRandomPortrait(pSourceDef->id, !!fPermanent, !!fCopyGfx);
	// try to set portrait
	return pTarget->Info->SetPortrait(szPortrait, pSourceDef, !!fPermanent, !!fCopyGfx);
}

static C4Value FnGetPortrait(Required<C4ObjectOrThis> pObj, bool fGetID, bool fGetPermanent)
{
	// check valid object with info section
	if (!pObj->Status || !pObj->Info) return C4VNull;
	// get portrait to examine
	C4Portrait *pPortrait;
	if (fGetPermanent)
	{
		// permanent: new portrait assigned?
		if (!(pPortrait = pObj->Info->pNewPortrait))
		{
			// custom portrait?
			if (pObj->Info->pCustomPortrait)
				if (fGetID) return C4VNull;
				else
					return C4VString(C4Portrait_Custom);
			// portrait string from info?
			const char *szPortrait = pObj->Info->PortraitFile;
			// no portrait string: portrait undefined ("none" would mean no portrait)
			if (!*szPortrait) return C4VNull;
			// evaluate portrait string
			C4ID idPortraitSource = 0;
			szPortrait = C4Portrait::EvaluatePortraitString(szPortrait, idPortraitSource, pObj->Info->id, nullptr);
			// return desired value
			if (fGetID)
				return idPortraitSource ? C4VID(idPortraitSource) : C4VNull;
			else
				return szPortrait ? C4VString(szPortrait) : C4VNull;
		}
	}
	else
		// get current portrait
		pPortrait = &(pObj->Info->Portrait);
	// get portrait graphics
	C4DefGraphics *pPortraitGfx = pPortrait->GetGfx();
	// no portrait?
	if (!pPortraitGfx) return C4VNull;
	// get def or name
	if (fGetID)
		return (pPortraitGfx->pDef ? C4VID(pPortraitGfx->pDef->id) : C4VNull);
	else
	{
		const char *szPortraitName = pPortraitGfx->GetName();
		return C4VString(szPortraitName ? szPortraitName : C4Portrait_Custom);
	}
}

static C4ValueInt FnLoadScenarioSection(C4String &strSection, C4ValueInt dwFlags)
{
	// safety
	const char *szSection;
	if (!*(szSection = FnStringPar(strSection))) return false;
	// try to load it
	return Game.LoadScenarioSection(szSection, dwFlags);
}

static bool FnSetObjectStatus(C4ValueInt iNewStatus, Required<C4ObjectOrThis> pObj, bool fClearPointers)
{
	if (!pObj->Status) return false;
	// no change
	if (pObj->Status == iNewStatus) return true;
	// set new status
	switch (iNewStatus)
	{
	case C4OS_NORMAL: return pObj->StatusActivate(); break;
	case C4OS_INACTIVE: return pObj->StatusDeactivate(!!fClearPointers); break;
	default: return false; // status unknown
	}
}

static C4ValueInt FnGetObjectStatus(Required<C4ObjectOrThis> pObj)
{
	return pObj->Status;
}

static bool FnAdjustWalkRotation(C4ValueInt iRangeX, C4ValueInt iRangeY, C4ValueInt iSpeed, Required<C4ObjectOrThis> pObj)
{
	// must be rotateable and attached to solid ground
	if (!pObj->Def->Rotateable || ~pObj->Action.t_attach & CNAT_Bottom || pObj->Shape.AttachMat == MNone)
		return false;
	// adjust rotation
	return pObj->AdjustWalkRotation(iRangeX, iRangeY, iSpeed);
}

static C4ValueInt FnAddEffect(C4String &sEffectName, C4Object *pTarget, C4ValueInt iPrio, C4ValueInt iTimerIntervall, C4Object *pCmdTarget, C4ID idCmdTarget, C4Value pvVal1, C4Value pvVal2, C4Value pvVal3, C4Value pvVal4)
{
	const char *szEffect = FnStringPar(sEffectName);
	// safety
	if (pTarget && !pTarget->Status) return 0;
	if (!*szEffect || !iPrio) return 0;
	// create effect
	int32_t iEffectNumber;
	new C4Effect(pTarget, szEffect, iPrio, iTimerIntervall, pCmdTarget, idCmdTarget, pvVal1, pvVal2, pvVal3, pvVal4, true, iEffectNumber, true);
	// return assigned effect number - may be 0 if he effect has been denied by another effect
	// may also be the number of another effect
	return iEffectNumber;
}

static C4Value FnGetEffect(C4String *psEffectName, C4Object *pTarget, C4ValueInt iIndex, C4ValueInt iQueryValue, C4ValueInt iMaxPriority)
{
	const char *szEffect = FnStringPar(psEffectName);
	// get effects
	C4Effect *pEffect = pTarget ? pTarget->pEffects : Game.pGlobalEffects;
	if (!pEffect) return C4VNull;
	// name/wildcard given: find effect by name and index
	if (szEffect && *szEffect)
		pEffect = pEffect->Get(szEffect, iIndex, iMaxPriority);
	else
		// otherwise, get by number
		pEffect = pEffect->Get(iIndex, true, iMaxPriority);
	// effect found?
	if (!pEffect) return C4VNull;
	// evaluate desired value
	switch (iQueryValue)
	{
	case 0: return C4VInt(pEffect->iNumber);        // 0: number
	case 1: return C4VString(pEffect->Name);        // 1: name
	case 2: return C4VInt(Abs(pEffect->iPriority)); // 2: priority (may be negative for deactivated effects)
	case 3: return C4VInt(pEffect->iIntervall);     // 3: timer intervall
	case 4: return C4VObj(pEffect->pCommandTarget); // 4: command target
	case 5: return C4VID(pEffect->idCommandTarget); // 5: command target ID
	case 6: return C4VInt(pEffect->iTime);          // 6: effect time
	}
	// invalid data queried
	return C4VNull;
}

static bool FnRemoveEffect(C4String *psEffectName, C4Object *pTarget, C4ValueInt iIndex, bool fDoNoCalls)
{
	// evaluate parameters
	const char *szEffect = FnStringPar(psEffectName);
	// get effects
	C4Effect *pEffect = pTarget ? pTarget->pEffects : Game.pGlobalEffects;
	if (!pEffect) return false;
	// name/wildcard given: find effect by name and index
	if (szEffect && *szEffect)
		pEffect = pEffect->Get(szEffect, iIndex);
	else
		// otherwise, get by number
		pEffect = pEffect->Get(iIndex, false);
	// effect found?
	if (!pEffect) return false;
	// kill it
	if (fDoNoCalls)
		pEffect->SetDead();
	else
		pEffect->Kill(pTarget);
	// done, success
	return true;
}

static bool FnChangeEffect(C4String *psEffectName, C4Object *pTarget, C4ValueInt iIndex, C4String &sNewEffectName, C4ValueInt iNewTimer)
{
	// evaluate parameters
	const char *szEffect = FnStringPar(psEffectName);
	const char *szNewEffect = FnStringPar(sNewEffectName);
	if (!*szNewEffect) return false;
	// get effects
	C4Effect *pEffect = pTarget ? pTarget->pEffects : Game.pGlobalEffects;
	if (!pEffect) return false;
	// name/wildcard given: find effect by name and index
	if (szEffect && *szEffect)
		pEffect = pEffect->Get(szEffect, iIndex);
	else
		// otherwise, get by number
		pEffect = pEffect->Get(iIndex, false);
	// effect found?
	if (!pEffect) return false;
	// set new name
	SCopy(szNewEffect, pEffect->Name, C4MaxName);
	pEffect->ReAssignCallbackFunctions();
	// set new timer
	if (iNewTimer >= 0)
	{
		pEffect->iIntervall = iNewTimer;
		pEffect->iTime = 0;
	}
	// done, success
	return true;
}

static std::optional<C4ValueInt> FnCheckEffect(C4String &sEffectName, C4Object *pTarget, C4ValueInt iPrio, C4ValueInt iTimerIntervall, C4Value pvVal1, C4Value pvVal2, C4Value pvVal3, C4Value pvVal4)
{
	const char *szEffect = FnStringPar(sEffectName);
	// safety
	if (pTarget && !pTarget->Status) return {};
	if (!*szEffect) return {};
	// get effects
	C4Effect *pEffect = pTarget ? pTarget->pEffects : Game.pGlobalEffects;
	if (!pEffect) return {};
	// let them check
	return {pEffect->Check(pTarget, szEffect, iPrio, iTimerIntervall, pvVal1, pvVal2, pvVal3, pvVal4, true)};
}

static C4ValueInt FnGetEffectCount(C4String *psEffectName, C4Object *pTarget, C4ValueInt iMaxPriority)
{
	// evaluate parameters
	const char *szEffect = FnStringPar(psEffectName);
	// get effects
	C4Effect *pEffect = pTarget ? pTarget->pEffects : Game.pGlobalEffects;
	if (!pEffect) return false;
	// count effects
	if (!*szEffect) szEffect = nullptr;
	return pEffect->GetCount(szEffect, iMaxPriority);
}

static C4Value FnEffectVar(C4ValueInt iVarIndex, C4Object *pObj, C4ValueInt iEffectNumber)
{
	// safety
	if (iVarIndex < 0) return C4VNull;
	// get effect
	C4Effect *pEffect = pObj ? pObj->pEffects : Game.pGlobalEffects;
	if (!pEffect) return C4VNull;
	if (!(pEffect = pEffect->Get(iEffectNumber, true))) return C4VNull;
	// return ref to var
	return pEffect->EffectVars[iVarIndex].GetRef();
}

static C4Value FnEffectCall(C4AulContext *ctx, C4Object *pTarget, C4ValueInt iNumber, C4String &sCallFn, C4Value vVal1, C4Value vVal2, C4Value vVal3, C4Value vVal4, C4Value vVal5, C4Value vVal6, C4Value vVal7)
{
	const char *szCallFn = FnStringPar(sCallFn);
	// safety
	if (pTarget && !pTarget->Status) return C4VNull;
	if (!*szCallFn) return C4VNull;
	// get effect
	C4Effect *pEffect = pTarget ? pTarget->pEffects : Game.pGlobalEffects;
	if (!pEffect) return C4VNull;
	if (!(pEffect = pEffect->Get(iNumber, true))) return C4VNull;
	// do call
	return pEffect->DoCall(pTarget, szCallFn, vVal1, vVal2, vVal3, vVal4, vVal5, vVal6, vVal7, true, !ctx->CalledWithStrictNil());
}

static C4ValueInt FnModulateColor(Default<C4ValueInt, 0xffffff> iClr1, C4ValueInt iClr2)
{
	// default color
	uint32_t dwClr1 = iClr1;
	uint32_t dwClr2 = iClr2;
	// get alpha
	C4ValueInt iA1 = dwClr1 >> 24, iA2 = dwClr2 >> 24;
	// modulate color values; mod alpha upwards
	uint32_t r = ((dwClr1 & 0xff) * (dwClr2 & 0xff)) >> 8 | // blue
		((dwClr1 >> 8 & 0xff) * (dwClr2 >> 8 & 0xff)) & 0xff00 | // green
		((dwClr1 >> 16 & 0xff) * (dwClr2 >> 8 & 0xff00)) & 0xff0000 | // red
		std::min<C4ValueInt>(iA1 + iA2 - ((iA1 * iA2) >> 8), 255) << 24; // alpha
	return r;
}

static C4ValueInt FnWildcardMatch(C4String *psString, C4String *psWildcard)
{
	return SWildcardMatchEx(FnStringPar(psString), FnStringPar(psWildcard));
}

static std::optional<C4ValueInt> FnGetContact(C4Object &obj, C4ValueInt iVertex, C4ValueInt dwCheck)
{
	// vertex not specified: check all
	if (iVertex == -1)
	{
		C4ValueInt iResult = 0;
		for (std::int32_t i = 0; i < obj.Shape.VtxNum; ++i)
			iResult |= obj.Shape.GetVertexContact(i, dwCheck, obj.x, obj.y);
		return iResult;
	}
	// vertex specified: check it
	if (!Inside<C4ValueInt>(iVertex, 0, obj.Shape.VtxNum - 1)) return {};
	return obj.Shape.GetVertexContact(iVertex, dwCheck, obj.x, obj.y);
}

static std::optional<C4ValueInt> FnSetObjectBlitMode(C4ValueInt dwNewBlitMode, Required<C4ObjectOrThis> pObj, C4ValueInt iOverlayID)
{
	// overlay?
	if (iOverlayID)
	{
		C4GraphicsOverlay *pOverlay = pObj->GetGraphicsOverlay(iOverlayID, false);
		if (!pOverlay)
		{
			DebugLog(spdlog::level::err, "SetObjectBlitMode: Overlay {} not defined for object {} ({})", static_cast<int>(iOverlayID), static_cast<int>(pObj->Number), pObj->GetName());
			return {};
		}
		pOverlay->SetBlitMode(dwNewBlitMode);
		return true;
	}
	// get prev blit mode
	uint32_t dwPrevMode = pObj->BlitMode;
	// iNewBlitMode = 0: reset to definition default
	if (!dwNewBlitMode)
		pObj->BlitMode = pObj->Def->BlitMode;
	else
		// otherwise, set the desired value
		// also ensure that the custom flag is set
		pObj->BlitMode = dwNewBlitMode | C4GFXBLIT_CUSTOM;
	// return previous value
	return dwPrevMode;
}

static std::optional<C4ValueInt> FnGetObjectBlitMode(Required<C4ObjectOrThis> pObj, C4ValueInt iOverlayID)
{
	// overlay?
	if (iOverlayID)
	{
		C4GraphicsOverlay *pOverlay = pObj->GetGraphicsOverlay(iOverlayID, false);
		if (!pOverlay)
		{
			DebugLog(spdlog::level::err, "SetObjectBlitMode: Overlay {} not defined for object {} ({})", static_cast<int>(iOverlayID), static_cast<int>(pObj->Number), pObj->GetName());
			return {};
		}
		return {pOverlay->GetBlitMode()};
	}
	// get blitting mode
	return {pObj->BlitMode};
}

static bool FnSetViewOffset(C4ValueInt iPlayer, C4ValueInt iX, C4ValueInt iY)
{
	if (!ValidPlr(iPlayer)) return false;
	// get player viewport
	C4Viewport *pView = Game.GraphicsSystem.GetViewport(iPlayer);
	if (!pView) return true; // sync safety
	// set
	pView->ViewOffsX = iX;
	pView->ViewOffsY = iY;
	// ok
	return true;
}

static bool FnSetPreSend(C4ValueInt iToVal, C4String *pNewName)
{
	if (iToVal < 0) return false;
	if (!Game.Control.isNetwork()) return true;
	if (iToVal == 0) iToVal = C4GameControlNetwork::DefaultTargetFPS;
	// dbg: manual presend
	const char *szClient = FnStringPar(pNewName);
	if (!szClient || !*szClient || WildcardMatch(szClient, Game.Clients.getLocalName()))
	{
		Game.Control.Network.setTargetFPS(iToVal);
		Game.GraphicsSystem.FlashMessage(("TargetFPS: " + std::to_string(iToVal)).c_str());
	}
	return true;
}

static C4ValueInt FnGetPlayerTeam(C4Player &player)
{
	// search team containing this player
	C4Team *pTeam = Game.Teams.GetTeamByPlayerID(player.ID);
	if (pTeam) return pTeam->GetID();
	// special value of -1 indicating that the team is still to be chosen
	if (player.IsChosingTeam()) return -1;
	// No team.
	return 0;
}

static bool FnSetPlayerTeam(C4Player &player, C4ValueInt idNewTeam, bool fNoCalls)
{
	// no team changing in league games
	if (Game.Parameters.isLeague()) return false;
	C4PlayerInfo *pPlrInfo = player.GetInfo();
	if (!pPlrInfo) return false;
	// already in that team?
	if (player.Team == idNewTeam) return true;
	// ask team setting if it's allowed (also checks for valid team)
	if (!Game.Teams.IsJoin2TeamAllowed(idNewTeam)) return false;
	// ask script if it's allowed
	if (!fNoCalls)
	{
		if (Game.Script.GRBroadcast(PSF_RejectTeamSwitch, {C4VInt(player.Number), C4VInt(idNewTeam)}, true, true))
			return false;
	}
	// exit previous team
	C4Team *pOldTeam = Game.Teams.GetTeamByPlayerID(player.ID);
	int32_t idOldTeam = 0;
	if (pOldTeam)
	{
		idOldTeam = pOldTeam->GetID();
		pOldTeam->RemovePlayerByID(player.ID);
	}
	// enter new team
	if (idNewTeam)
	{
		C4Team *pNewTeam = Game.Teams.GetGenerateTeamByID(idNewTeam);
		if (pNewTeam)
		{
			pNewTeam->AddPlayer(*pPlrInfo, true);
			idNewTeam = pNewTeam->GetID();
			// Update common home base material
			if (Game.Rules & C4RULE_TeamHombase && !fNoCalls) player.SyncHomebaseMaterialFromTeam();
		}
		else
		{
			// unknown error
			player.Team = idNewTeam = 0;
		}
	}
	// update hositlities if this is not a "silent" change
	if (!fNoCalls)
	{
		player.SetTeamHostility();
	}
	// do callback to reflect change in scenario
	if (!fNoCalls)
		Game.Script.GRBroadcast(PSF_OnTeamSwitch, {C4VInt(player.Number), C4VInt(idNewTeam), C4VInt(idOldTeam)}, true);
	return true;
}

static std::optional<C4ValueInt> FnGetTeamConfig(C4ValueInt iConfigValue)
{
	// query value
	switch (iConfigValue)
	{
	case C4TeamList::TEAM_Custom:               return {Game.Teams.IsCustom()};
	case C4TeamList::TEAM_Active:               return {Game.Teams.IsMultiTeams()};
	case C4TeamList::TEAM_AllowHostilityChange: return {Game.Teams.IsHostilityChangeAllowed()};
	case C4TeamList::TEAM_Dist:                 return {Game.Teams.GetTeamDist()};
	case C4TeamList::TEAM_AllowTeamSwitch:      return {Game.Teams.IsTeamSwitchAllowed()};
	case C4TeamList::TEAM_AutoGenerateTeams:    return {Game.Teams.IsAutoGenerateTeams()};
	case C4TeamList::TEAM_TeamColors:           return {Game.Teams.IsTeamColors()};
	}
	// undefined value
	DebugLog(spdlog::level::err, "GetTeamConfig: Unknown config value: {}", iConfigValue);
	return {};
}

static C4String *FnGetTeamName(C4ValueInt iTeam)
{
	C4Team *pTeam = Game.Teams.GetTeamByID(iTeam);
	if (!pTeam) return nullptr;
	return String(pTeam->GetName());
}

static std::optional<C4ValueInt> FnGetTeamColor(C4ValueInt iTeam)
{
	C4Team *pTeam = Game.Teams.GetTeamByID(iTeam);
	return pTeam ? std::make_optional(pTeam->GetColor()) : std::nullopt;
}

static std::optional<C4ValueInt> FnGetTeamByIndex(C4ValueInt iIndex)
{
	C4Team *pTeam = Game.Teams.GetTeamByIndex(iIndex);
	return pTeam ? std::make_optional(pTeam->GetID()) : std::nullopt;
}

static bool FnOnOwnerRemoved(C4AulContext *cthr)
{
	// safety
	C4Object *pObj = cthr->Obj; if (!pObj) return false;
	C4Player *pPlr = Game.Players.Get(pObj->Owner); if (!pPlr) return false;
	if (pPlr->Crew.IsContained(pObj))
	{
		// crew members: Those are removed later (AFTER the player has been removed, for backwards compatiblity with relaunch scripting)
	}
	else if ((~pObj->Category & C4D_StaticBack) || (pObj->id == C4ID_Flag))
	{
		// Regular objects: Try to find a new, suitable owner from the same team
		// Ignore StaticBack, because this would not be backwards compatible with many internal objects such as team account
		// Do not ignore flags which might be StaticBack if being attached to castle parts
		int32_t iNewOwner = NO_OWNER;
		C4Team *pTeam;
		if (pPlr->Team) if (pTeam = Game.Teams.GetTeamByID(pPlr->Team))
		{
			for (int32_t i = 0; i < pTeam->GetPlayerCount(); ++i)
			{
				int32_t iPlrID = pTeam->GetIndexedPlayer(i);
				if (iPlrID && iPlrID != pPlr->ID)
				{
					C4PlayerInfo *pPlrInfo = Game.PlayerInfos.GetPlayerInfoByID(iPlrID);
					if (pPlrInfo) if (pPlrInfo->IsJoined())
					{
						// this looks like a good new owner
						iNewOwner = pPlrInfo->GetInGameNumber();
						break;
					}
				}
			}
		}
		// if noone from the same team was found, try to find another non-hostile player
		// (necessary for cooperative rounds without teams)
		if (iNewOwner == NO_OWNER)
			for (C4Player *pOtherPlr = Game.Players.First; pOtherPlr; pOtherPlr = pOtherPlr->Next)
				if (pOtherPlr != pPlr) if (!pOtherPlr->Eliminated)
					if (!Game.Players.Hostile(pOtherPlr->Number, pPlr->Number))
						iNewOwner = pOtherPlr->Number;

		// set this owner
		pObj->SetOwner(iNewOwner);
	}
	return true;
}

static void FnSetScoreboardData(C4ValueInt iRowID, C4ValueInt iColID, C4String *pText, C4ValueInt iData)
{
	Game.Scoreboard.SetCell(iColID, iRowID, pText ? pText->Data.getData() : nullptr, iData);
}

static C4String *FnGetScoreboardString(C4ValueInt iRowID, C4ValueInt iColID)
{
	return String(Game.Scoreboard.GetCellString(iColID, iRowID));
}

static int32_t FnGetScoreboardData(C4ValueInt iRowID, C4ValueInt iColID)
{
	return Game.Scoreboard.GetCellData(iColID, iRowID);
}

static bool FnDoScoreboardShow(C4ValueInt iChange, C4ValueInt iForPlr)
{
	C4Player *pPlr;
	if (iForPlr)
	{
		// abort if the specified player is not local - but always return if the player exists,
		// to ensure sync safety
		if (!(pPlr = Game.Players.Get(iForPlr - 1))) return false;
		if (!pPlr->LocalControl) return true;
	}
	Game.Scoreboard.DoDlgShow(iChange, false);
	return true;
}

static bool FnAddEvaluationData(C4String &text, std::optional<C4PlayerInfo *> playerInfo)
{
	// safety
	if (!text.Data.getLength()) return false;
	if (playerInfo && !*playerInfo) return false;
	// add data
	Game.RoundResults.AddCustomEvaluationString(text.Data.getData(), playerInfo.transform(&C4PlayerInfo::GetID).value_or(0));
	return true;
}

static C4ValueInt FnGetUnusedOverlayID(RequiredNonZero<C4ValueInt> iBaseIndex, Required<C4ObjectOrThis> pObj)
{
	// find search first unused index from there on
	int iSearchDir = (*iBaseIndex < 0) ? -1 : 1;
	while (pObj->GetGraphicsOverlay(iBaseIndex, false)) *iBaseIndex += iSearchDir;
	return iBaseIndex;
}

static C4ValueInt FnActivateGameGoalMenu(C4Player &player)
{
	// open menu
	return player.Menu.ActivateGoals(player.Number, player.LocalControl && !Game.Control.isReplay());
}

static void FnFatalError(C4AulContext *ctx, C4String *pErrorMsg)
{
	throw C4AulExecError(ctx->Obj, std::format("User error: {}", pErrorMsg ? pErrorMsg->Data.getData() : "(no error)"));
}

static bool FnStartScriptProfiler(C4ID idScript)
{
	// get script to profile
	C4AulScript *pScript;
	if (idScript)
	{
		C4Def *pDef = C4Id2Def(idScript);
		if (!pDef) return false;
		pScript = &pDef->Script;
	}
	else
		pScript = &Game.ScriptEngine;
	// profile it
	C4AulProfiler::StartProfiling(pScript);
	return true;
}

static bool FnCustomMessage(C4AulContext *ctx, C4String &msg, C4Object *pObj, C4ValueInt iOwner, C4ValueInt iOffX, C4ValueInt iOffY, Default<C4ValueInt, 0xffffff> clr, C4ID idDeco, C4String *sPortrait, C4ValueInt dwFlags, C4ValueInt iHSize)
{
	// safeties
	if (pObj && !pObj->Status) return false;
	const char *szMsg = msg.Data.getData();
	if (!szMsg) return false;
	if (idDeco && !C4Id2Def(idDeco)) return false;
	// only one positioning flag per direction allowed
	uint32_t hpos = dwFlags & (C4GM_Left | C4GM_HCenter | C4GM_Right);
	uint32_t vpos = dwFlags & (C4GM_Top | C4GM_VCenter | C4GM_Bottom);
	if (((hpos | hpos - 1) + 1) >> 1 != hpos)
	{
		throw C4AulExecError(ctx->Obj, "CustomMessage: Only one horizontal positioning flag allowed!");
	}
	if (((vpos | vpos - 1) + 1) >> 1 != vpos)
	{
		throw C4AulExecError(ctx->Obj, "CustomMessage: Only one vertical positioning flag allowed!");
	}

	uint32_t alignment = dwFlags & (C4GM_ALeft | C4GM_ACenter | C4GM_ARight);
	if (((alignment | alignment - 1) + 1) >> 1 != alignment)
	{
		throw C4AulExecError(ctx->Obj, "CustomMessage: Only one text alignment flag allowed!");
	}

	// message color
	const auto dwClr = InvertRGBAAlpha(clr);
	// message type
	int32_t iType;
	if (pObj)
		if (iOwner != NO_OWNER)
			iType = C4GM_TargetPlayer;
		else
			iType = C4GM_Target;
	else if (iOwner != NO_OWNER)
		iType = C4GM_GlobalPlayer;
	else
		iType = C4GM_Global;
	// remove speech?
	StdStrBuf sMsg;
	sMsg.Ref(szMsg);
	if (dwFlags & C4GM_DropSpeech) sMsg.SplitAtChar('$', nullptr);
	// create it!
	return Game.Messages.New(iType, sMsg, pObj, iOwner, iOffX, iOffY, static_cast<uint32_t>(dwClr), idDeco, sPortrait ? sPortrait->Data.getData() : nullptr, dwFlags, iHSize);
}

static void FnPauseGame(bool fToggle)
{
	// not in replay (film)
	if (Game.Control.isReplay()) return;
	// script method for halting game (for films)
	if (fToggle)
		Console.TogglePause();
	else
		Console.DoHalt();
}

static void FnSetNextMission(C4String *szNextMission, C4String *szNextMissionText, C4String *szNextMissionDesc)
{
	if (!szNextMission || !szNextMission->Data.getLength())
	{
		// param empty: clear next mission
		Game.NextMission.Clear();
		Game.NextMissionText.Clear();
	}
	else
	{
		// set next mission, button and button desc if given
		Game.NextMission.Copy(szNextMission->Data);
		if (szNextMissionText && szNextMissionText->Data.getData())
		{
			Game.NextMissionText.Copy(szNextMissionText->Data);
		}
		else
		{
			Game.NextMissionText.Copy(LoadResStr(C4ResStrTableKey::IDS_BTN_NEXTSCENARIO));
		}
		if (szNextMissionDesc && szNextMissionDesc->Data.getData())
		{
			Game.NextMissionDesc.Copy(szNextMissionDesc->Data);
		}
		else
		{
			Game.NextMissionDesc.Copy(LoadResStr(C4ResStrTableKey::IDS_DESC_NEXTSCENARIO));
		}
	}
}

static C4ValueArray *FnGetKeys(C4AulContext *ctx, C4ValueHash *map)
{
	if (!map) throw C4AulExecError(ctx->Obj, "GetKeys(): map expected, got 0");

	C4ValueArray *keys = new C4ValueArray(map->size());

	size_t i = 0;
	for (const auto &[key, value] : *map)
	{
		(*keys)[i] = key;
		++i;
	}

	return keys;
}

static C4ValueArray *FnGetValues(C4AulContext *ctx, C4ValueHash *map)
{
	if (!map) throw C4AulExecError(ctx->Obj, "GetValues(): map expected, got 0");

	C4ValueArray *keys = new C4ValueArray(map->size());

	size_t i = 0;
	for (const auto &[key, value] : *map)
	{
		(*keys)[i] = value;
		++i;
	}

	return keys;
}

static void FnSetRestoreInfos(C4ValueInt what)
{
	Game.RestartRestoreInfos.What = static_cast<std::underlying_type_t<C4NetworkRestartInfos::RestoreInfo>>(what);
}

static constexpr C4ScriptConstDef C4ScriptConstMap[] =
{
	{ "C4D_All",         C4V_Int, C4D_All },
	{ "C4D_StaticBack",  C4V_Int, C4D_StaticBack },
	{ "C4D_Structure",   C4V_Int, C4D_Structure },
	{ "C4D_Vehicle",     C4V_Int, C4D_Vehicle },
	{ "C4D_Living",      C4V_Int, C4D_Living },
	{ "C4D_Object",      C4V_Int, C4D_Object },
	{ "C4D_Goal",        C4V_Int, C4D_Goal },
	{ "C4D_Environment", C4V_Int, C4D_Environment },
	{ "C4D_Knowledge",   C4V_Int, C4D_SelectKnowledge },
	{ "C4D_Magic",       C4V_Int, C4D_Magic },
	{ "C4D_Rule",        C4V_Int, C4D_Rule },
	{ "C4D_Background",  C4V_Int, C4D_Background },
	{ "C4D_Parallax",    C4V_Int, C4D_Parallax },
	{ "C4D_MouseSelect", C4V_Int, C4D_MouseSelect },
	{ "C4D_Foreground",  C4V_Int, C4D_Foreground },
	{ "C4D_MouseIgnore", C4V_Int, C4D_MouseIgnore },
	{ "C4D_IgnoreFoW",   C4V_Int, C4D_IgnoreFoW },

	{ "C4D_GrabGet", C4V_Int, C4D_Grab_Get },
	{ "C4D_GrabPut", C4V_Int, C4D_Grab_Put },

	{ "C4D_LinePower",     C4V_Int, C4D_Line_Power },
	{ "C4D_LineSource",    C4V_Int, C4D_Line_Source },
	{ "C4D_LineDrain",     C4V_Int, C4D_Line_Drain },
	{ "C4D_LineLightning", C4V_Int, C4D_Line_Lightning },
	{ "C4D_LineVolcano",   C4V_Int, C4D_Line_Volcano },
	{ "C4D_LineRope",      C4V_Int, C4D_Line_Rope },
	{ "C4D_LineColored",   C4V_Int, C4D_Line_Colored },
	{ "C4D_LineVertex",    C4V_Int, C4D_Line_Vertex },

	{ "C4D_PowerInput",     C4V_Int, C4D_Power_Input },
	{ "C4D_PowerOutput",    C4V_Int, C4D_Power_Output },
	{ "C4D_LiquidInput",    C4V_Int, C4D_Liquid_Input },
	{ "C4D_LiquidOutput",   C4V_Int, C4D_Liquid_Output },
	{ "C4D_PowerGenerator", C4V_Int, C4D_Power_Generator },
	{ "C4D_PowerConsumer",  C4V_Int, C4D_Power_Consumer },
	{ "C4D_LiquidPump",     C4V_Int, C4D_Liquid_Pump },
	{ "C4D_EnergyHolder",   C4V_Int, C4D_EnergyHolder },

	{ "C4V_Any",      C4V_Int, C4V_Any },
	{ "C4V_Int",      C4V_Int, C4V_Int },
	{ "C4V_Bool",     C4V_Int, C4V_Bool },
	{ "C4V_C4ID",     C4V_Int, C4V_C4ID },
	{ "C4V_C4Object", C4V_Int, C4V_C4Object },
	{ "C4V_String",   C4V_Int, C4V_String },
	{ "C4V_Array",    C4V_Int, C4V_Array },
	{ "C4V_Map",      C4V_Int, C4V_Map },

	{ "COMD_None",      C4V_Int, COMD_None },
	{ "COMD_Stop",      C4V_Int, COMD_Stop },
	{ "COMD_Up",        C4V_Int, COMD_Up },
	{ "COMD_UpRight",   C4V_Int, COMD_UpRight },
	{ "COMD_Right",     C4V_Int, COMD_Right },
	{ "COMD_DownRight", C4V_Int, COMD_DownRight },
	{ "COMD_Down",      C4V_Int, COMD_Down },
	{ "COMD_DownLeft",  C4V_Int, COMD_DownLeft },
	{ "COMD_Left",      C4V_Int, COMD_Left },
	{ "COMD_UpLeft",    C4V_Int, COMD_UpLeft },

	{ "DIR_Left",  C4V_Int, DIR_Left },
	{ "DIR_Right", C4V_Int, DIR_Right },

	{ "CON_CursorLeft",   C4V_Int, CON_CursorLeft },
	{ "CON_CursorToggle", C4V_Int, CON_CursorToggle },
	{ "CON_CursorRight",  C4V_Int, CON_CursorRight },
	{ "CON_Throw",        C4V_Int, CON_Throw },
	{ "CON_Up",           C4V_Int, CON_Up },
	{ "CON_Dig",          C4V_Int, CON_Dig },
	{ "CON_Left",         C4V_Int, CON_Left },
	{ "CON_Down",         C4V_Int, CON_Down },
	{ "CON_Right",        C4V_Int, CON_Right },
	{ "CON_Menu",         C4V_Int, CON_Menu },
	{ "CON_Special",      C4V_Int, CON_Special },
	{ "CON_Special2",     C4V_Int, CON_Special2 },

	{ "OCF_Construct",        C4V_Int, OCF_Construct },
	{ "OCF_Grab",             C4V_Int, OCF_Grab },
	{ "OCF_Collectible",      C4V_Int, OCF_Carryable },
	{ "OCF_OnFire",           C4V_Int, OCF_OnFire },
	{ "OCF_HitSpeed1",        C4V_Int, OCF_HitSpeed1 },
	{ "OCF_Fullcon",          C4V_Int, OCF_FullCon },
	{ "OCF_Inflammable",      C4V_Int, OCF_Inflammable },
	{ "OCF_Chop",             C4V_Int, OCF_Chop },
	{ "OCF_Rotate",           C4V_Int, OCF_Rotate },
	{ "OCF_Exclusive",        C4V_Int, OCF_Exclusive },
	{ "OCF_Entrance",         C4V_Int, OCF_Entrance },
	{ "OCF_HitSpeed2",        C4V_Int, OCF_HitSpeed2 },
	{ "OCF_HitSpeed3",        C4V_Int, OCF_HitSpeed3 },
	{ "OCF_Collection",       C4V_Int, OCF_Collection },
	{ "OCF_Living",           C4V_Int, OCF_Living },
	{ "OCF_HitSpeed4",        C4V_Int, OCF_HitSpeed4 },
	{ "OCF_FightReady",       C4V_Int, OCF_FightReady },
	{ "OCF_LineConstruct",    C4V_Int, OCF_LineConstruct },
	{ "OCF_Prey",             C4V_Int, OCF_Prey },
	{ "OCF_AttractLightning", C4V_Int, OCF_AttractLightning },
	{ "OCF_NotContained",     C4V_Int, OCF_NotContained },
	{ "OCF_CrewMember",       C4V_Int, OCF_CrewMember },
	{ "OCF_Edible",           C4V_Int, OCF_Edible },
	{ "OCF_InLiquid",         C4V_Int, OCF_InLiquid },
	{ "OCF_InSolid",          C4V_Int, OCF_InSolid },
	{ "OCF_InFree",           C4V_Int, OCF_InFree },
	{ "OCF_Available",        C4V_Int, OCF_Available },
	{ "OCF_PowerConsumer",    C4V_Int, OCF_PowerConsumer },
	{ "OCF_PowerSupply",      C4V_Int, OCF_PowerSupply },
	{ "OCF_Container",        C4V_Int, OCF_Container },
	{ "OCF_Alive",            C4V_Int, static_cast<C4ValueInt>(OCF_Alive) },

	{ "VIS_All",         C4V_Int, VIS_All },
	{ "VIS_None",        C4V_Int, VIS_None },
	{ "VIS_Owner",       C4V_Int, VIS_Owner },
	{ "VIS_Allies",      C4V_Int, VIS_Allies },
	{ "VIS_Enemies",     C4V_Int, VIS_Enemies },
	{ "VIS_Local",       C4V_Int, VIS_Local },
	{ "VIS_God",         C4V_Int, VIS_God },
	{ "VIS_LayerToggle", C4V_Int, VIS_LayerToggle },
	{ "VIS_OverlayOnly", C4V_Int, VIS_OverlayOnly },

	{ "C4X_Ver1",     C4V_Int, C4XVER1 },
	{ "C4X_Ver2",     C4V_Int, C4XVER2 },
	{ "C4X_Ver3",     C4V_Int, C4XVER3 },
	{ "C4X_Ver4",     C4V_Int, C4XVER4 },
	{ "C4X_VerBuild", C4V_Int, C4XVERBUILD },

	{ "SkyPar_Keep", C4V_Int, SkyPar_KEEP },

	{ "C4MN_Style_Normal",          C4V_Int, C4MN_Style_Normal },
	{ "C4MN_Style_Context",         C4V_Int, C4MN_Style_Context },
	{ "C4MN_Style_Info",            C4V_Int, C4MN_Style_Info },
	{ "C4MN_Style_Dialog",          C4V_Int, C4MN_Style_Dialog },
	{ "C4MN_Style_EqualItemHeight", C4V_Int, C4MN_Style_EqualItemHeight },

	{ "C4MN_Extra_None",                C4V_Int, C4MN_Extra_None },
	{ "C4MN_Extra_Components",          C4V_Int, C4MN_Extra_Components },
	{ "C4MN_Extra_Value",               C4V_Int, C4MN_Extra_Value },
	{ "C4MN_Extra_MagicValue",          C4V_Int, C4MN_Extra_MagicValue },
	{ "C4MN_Extra_Info",                C4V_Int, C4MN_Extra_Info },
	{ "C4MN_Extra_ComponentsMagic",     C4V_Int, C4MN_Extra_ComponentsMagic },
	{ "C4MN_Extra_LiveMagicValue",      C4V_Int, C4MN_Extra_LiveMagicValue },
	{ "C4MN_Extra_ComponentsLiveMagic", C4V_Int, C4MN_Extra_ComponentsLiveMagic },

	{ "C4MN_Add_ImgRank",     C4V_Int, C4MN_Add_ImgRank },
	{ "C4MN_Add_ImgIndexed",  C4V_Int, C4MN_Add_ImgIndexed },
	{ "C4MN_Add_ImgObjRank",  C4V_Int, C4MN_Add_ImgObjRank },
	{ "C4MN_Add_ImgObject",   C4V_Int, C4MN_Add_ImgObject },
	{ "C4MN_Add_ImgTextSpec", C4V_Int, C4MN_Add_ImgTextSpec },
	{ "C4MN_Add_ImgColor",    C4V_Int, C4MN_Add_ImgColor },
	{ "C4MN_Add_ImgIndexedColor",    C4V_Int, C4MN_Add_ImgIndexedColor },
	{ "C4MN_Add_PassValue",   C4V_Int, C4MN_Add_PassValue },
	{ "C4MN_Add_ForceCount",  C4V_Int, C4MN_Add_ForceCount },
	{ "C4MN_Add_ForceNoDesc", C4V_Int, C4MN_Add_ForceNoDesc },

	{ "FX_OK",                  C4V_Int, C4Fx_OK }, // generic standard behaviour for all effect callbacks
	{ "FX_Effect_Deny",         C4V_Int, C4Fx_Effect_Deny }, // delete effect
	{ "FX_Effect_Annul",        C4V_Int, C4Fx_Effect_Annul }, // delete effect, because it has annulled a countereffect
	{ "FX_Effect_AnnulDoCalls", C4V_Int, C4Fx_Effect_AnnulCalls }, // delete effect, because it has annulled a countereffect; temp readd countereffect
	{ "FX_Execute_Kill",        C4V_Int, C4Fx_Execute_Kill }, // execute callback: Remove effect now
	{ "FX_Stop_Deny",           C4V_Int, C4Fx_Stop_Deny }, // deny effect removal
	{ "FX_Start_Deny",          C4V_Int, C4Fx_Start_Deny }, // deny effect start

	{ "FX_Call_Normal",            C4V_Int, C4FxCall_Normal }, // normal call; effect is being added or removed
	{ "FX_Call_Temp",              C4V_Int, C4FxCall_Temp }, // temp call; effect is being added or removed in responce to a lower-level effect change
	{ "FX_Call_TempAddForRemoval", C4V_Int, C4FxCall_TempAddForRemoval }, // temp call; effect is being added because it had been temp removed and is now removed forever
	{ "FX_Call_RemoveClear",       C4V_Int, C4FxCall_RemoveClear }, // effect is being removed because object is being removed
	{ "FX_Call_RemoveDeath",       C4V_Int, C4FxCall_RemoveDeath }, // effect is being removed because object died - return -1 to avoid removal
	{ "FX_Call_DmgScript",         C4V_Int, C4FxCall_DmgScript }, // damage through script call
	{ "FX_Call_DmgBlast",          C4V_Int, C4FxCall_DmgBlast }, // damage through blast
	{ "FX_Call_DmgFire",           C4V_Int, C4FxCall_DmgFire }, // damage through fire
	{ "FX_Call_DmgChop",           C4V_Int, C4FxCall_DmgChop }, // damage through chopping
	{ "FX_Call_Energy",            C4V_Int, 32 }, // bitmask for generic energy loss
	{ "FX_Call_EngScript",         C4V_Int, C4FxCall_EngScript }, // energy loss through script call
	{ "FX_Call_EngBlast",          C4V_Int, C4FxCall_EngBlast }, // energy loss through blast
	{ "FX_Call_EngObjHit",         C4V_Int, C4FxCall_EngObjHit }, // energy loss through object hitting the living
	{ "FX_Call_EngFire",           C4V_Int, C4FxCall_EngFire }, // energy loss through fire
	{ "FX_Call_EngBaseRefresh",    C4V_Int, C4FxCall_EngBaseRefresh }, // energy reload in base (also by base object, but that's normally not called)
	{ "FX_Call_EngAsphyxiation",   C4V_Int, C4FxCall_EngAsphyxiation }, // energy loss through asphyxiaction
	{ "FX_Call_EngCorrosion",      C4V_Int, C4FxCall_EngCorrosion }, // energy loss through corrosion (acid)
	{ "FX_Call_EngStruct",         C4V_Int, C4FxCall_EngStruct }, // regular structure energy loss (normally not called)
	{ "FX_Call_EngGetPunched",     C4V_Int, C4FxCall_EngGetPunched }, // energy loss during fighting

	{ "GFXOV_MODE_None",          C4V_Int, C4GraphicsOverlay::MODE_None }, // gfx overlay modes
	{ "GFXOV_MODE_Base",          C4V_Int, C4GraphicsOverlay::MODE_Base },
	{ "GFXOV_MODE_Action",        C4V_Int, C4GraphicsOverlay::MODE_Action },
	{ "GFXOV_MODE_Picture",       C4V_Int, C4GraphicsOverlay::MODE_Picture },
	{ "GFXOV_MODE_IngamePicture", C4V_Int, C4GraphicsOverlay::MODE_IngamePicture },
	{ "GFXOV_MODE_Object",        C4V_Int, C4GraphicsOverlay::MODE_Object },
	{ "GFXOV_MODE_ExtraGraphics", C4V_Int, C4GraphicsOverlay::MODE_ExtraGraphics },
	{ "GFX_Overlay",              C4V_Int, 1 }, // default overlay index
	{ "GFXOV_Clothing",           C4V_Int, 1000 }, // overlay indices for clothes on Clonks, etc.
	{ "GFXOV_Tools",              C4V_Int, 2000 }, // overlay indices for tools, weapons, etc.
	{ "GFXOV_ProcessTarget",      C4V_Int, 3000 }, // overlay indices for objects processed by a Clonk
	{ "GFXOV_Misc",               C4V_Int, 5000 }, // overlay indices for other stuff
	{ "GFXOV_UI",                 C4V_Int, 6000 }, // overlay indices for user interface
	{ "GFX_BLIT_Additive",        C4V_Int, C4GFXBLIT_ADDITIVE }, // blit modes
	{ "GFX_BLIT_Mod2",            C4V_Int, C4GFXBLIT_MOD2 },
	{ "GFX_BLIT_ClrSfc_OwnClr",   C4V_Int, C4GFXBLIT_CLRSFC_OWNCLR },
	{ "GFX_BLIT_ClrSfc_Mod2",     C4V_Int, C4GFXBLIT_CLRSFC_MOD2 },
	{ "GFX_BLIT_Custom",          C4V_Int, C4GFXBLIT_CUSTOM },
	{ "GFX_BLIT_Parent",          C4V_Int, C4GFXBLIT_PARENT },

	{ "NO_OWNER", C4V_Int, NO_OWNER }, // invalid player number

	// contact attachment
	{ "CNAT_None",        C4V_Int, CNAT_None },
	{ "CNAT_Left",        C4V_Int, CNAT_Left },
	{ "CNAT_Right",       C4V_Int, CNAT_Right },
	{ "CNAT_Top",         C4V_Int, CNAT_Top },
	{ "CNAT_Bottom",      C4V_Int, CNAT_Bottom },
	{ "CNAT_Center",      C4V_Int, CNAT_Center },
	{ "CNAT_MultiAttach", C4V_Int, CNAT_MultiAttach },
	{ "CNAT_NoCollision", C4V_Int, CNAT_NoCollision },

	// vertex data
	{ "VTX_X",        C4V_Int, VTX_X },
	{ "VTX_Y",        C4V_Int, VTX_Y },
	{ "VTX_CNAT",     C4V_Int, VTX_CNAT },
	{ "VTX_Friction", C4V_Int, VTX_Friction },

	// vertex set mode
	{ "VTX_SetPermanent",    C4V_Int, VTX_SetPermanent },
	{ "VTX_SetPermanentUpd", C4V_Int, VTX_SetPermanentUpd },

	// material density
	{ "C4M_Vehicle",    C4V_Int, C4M_Vehicle },
	{ "C4M_Solid",      C4V_Int, C4M_Solid },
	{ "C4M_SemiSolid",  C4V_Int, C4M_SemiSolid },
	{ "C4M_Liquid",     C4V_Int, C4M_Liquid },
	{ "C4M_Background", C4V_Int, C4M_Background },

	// scoreboard
	{ "SBRD_Caption", C4V_Int, C4Scoreboard::TitleKey }, // used to set row/coloumn headers

	// teams - constants for GetTeamConfig
	{ "TEAM_Custom",               C4V_Int, C4TeamList::TEAM_Custom },
	{ "TEAM_Active",               C4V_Int, C4TeamList::TEAM_Active },
	{ "TEAM_AllowHostilityChange", C4V_Int, C4TeamList::TEAM_AllowHostilityChange },
	{ "TEAM_Dist",                 C4V_Int, C4TeamList::TEAM_Dist },
	{ "TEAM_AllowTeamSwitch",      C4V_Int, C4TeamList::TEAM_AllowTeamSwitch },
	{ "TEAM_AutoGenerateTeams",    C4V_Int, C4TeamList::TEAM_AutoGenerateTeams },
	{ "TEAM_TeamColors",           C4V_Int, C4TeamList::TEAM_TeamColors },

	{ "C4OS_DELETED",  C4V_Int, C4OS_DELETED },
	{ "C4OS_NORMAL",   C4V_Int, C4OS_NORMAL },
	{ "C4OS_INACTIVE", C4V_Int, C4OS_INACTIVE },

	{ "C4MSGCMDR_Escaped",    C4V_Int, C4MessageBoardCommand::C4MSGCMDR_Escaped },
	{ "C4MSGCMDR_Plain",      C4V_Int, C4MessageBoardCommand::C4MSGCMDR_Plain },
	{ "C4MSGCMDR_Identifier", C4V_Int, C4MessageBoardCommand::C4MSGCMDR_Identifier },

	{ "BASEFUNC_Default",          C4V_Int, BASEFUNC_Default },
	{ "BASEFUNC_AutoSellContents", C4V_Int, BASEFUNC_AutoSellContents },
	{ "BASEFUNC_RegenerateEnergy", C4V_Int, BASEFUNC_RegenerateEnergy },
	{ "BASEFUNC_Buy",              C4V_Int, BASEFUNC_Buy },
	{ "BASEFUNC_Sell",             C4V_Int, BASEFUNC_Sell },
	{ "BASEFUNC_RejectEntrance",   C4V_Int, BASEFUNC_RejectEntrance },
	{ "BASEFUNC_Extinguish",       C4V_Int, BASEFUNC_Extinguish },

	{ "C4FO_Not",          C4V_Int, C4FO_Not },
	{ "C4FO_And",          C4V_Int, C4FO_And },
	{ "C4FO_Or",           C4V_Int, C4FO_Or },
	{ "C4FO_Exclude",      C4V_Int, C4FO_Exclude },
	{ "C4FO_InRect",       C4V_Int, C4FO_InRect },
	{ "C4FO_AtPoint",      C4V_Int, C4FO_AtPoint },
	{ "C4FO_AtRect",       C4V_Int, C4FO_AtRect },
	{ "C4FO_OnLine",       C4V_Int, C4FO_OnLine },
	{ "C4FO_Distance",     C4V_Int, C4FO_Distance },
	{ "C4FO_ID",           C4V_Int, C4FO_ID },
	{ "C4FO_OCF",          C4V_Int, C4FO_OCF },
	{ "C4FO_Category",     C4V_Int, C4FO_Category },
	{ "C4FO_Action",       C4V_Int, C4FO_Action },
	{ "C4FO_ActionTarget", C4V_Int, C4FO_ActionTarget },
	{ "C4FO_Container",    C4V_Int, C4FO_Container },
	{ "C4FO_AnyContainer", C4V_Int, C4FO_AnyContainer },
	{ "C4FO_Owner",        C4V_Int, C4FO_Owner },
	{ "C4FO_Controller",   C4V_Int, C4FO_Controller },
	{ "C4FO_Func",         C4V_Int, C4FO_Func },
	{ "C4FO_Layer",        C4V_Int, C4FO_Layer },

	{ "C4SO_Reverse",  C4V_Int, C4SO_Reverse },
	{ "C4SO_Multiple", C4V_Int, C4SO_Multiple },
	{ "C4SO_Distance", C4V_Int, C4SO_Distance },
	{ "C4SO_Random",   C4V_Int, C4SO_Random },
	{ "C4SO_Speed",    C4V_Int, C4SO_Speed },
	{ "C4SO_Mass",     C4V_Int, C4SO_Mass },
	{ "C4SO_Value",    C4V_Int, C4SO_Value },
	{ "C4SO_Func",     C4V_Int, C4SO_Func },

	{ "PHYS_Current",        C4V_Int, PHYS_Current },
	{ "PHYS_Permanent",      C4V_Int, PHYS_Permanent },
	{ "PHYS_Temporary",      C4V_Int, PHYS_Temporary },
	{ "PHYS_StackTemporary", C4V_Int, PHYS_StackTemporary },

	{ "C4CMD_Base",       C4V_Int, C4CMD_Mode_Base },
	{ "C4CMD_SilentBase", C4V_Int, C4CMD_Mode_SilentBase },
	{ "C4CMD_Sub",        C4V_Int, C4CMD_Mode_Sub },
	{ "C4CMD_SilentSub",  C4V_Int, C4CMD_Mode_SilentSub },

	{ "C4CMD_MoveTo_NoPosAdjust", C4V_Int, C4CMD_MoveTo_NoPosAdjust },
	{ "C4CMD_MoveTo_PushTarget",  C4V_Int, C4CMD_MoveTo_PushTarget },
	{ "C4CMD_Enter_PushTarget",   C4V_Int, C4CMD_Enter_PushTarget },

	{ "C4SECT_SaveLandscape", C4V_Int, C4S_SAVE_LANDSCAPE },
	{ "C4SECT_SaveObjects",   C4V_Int, C4S_SAVE_OBJECTS },
	{ "C4SECT_KeepEffects",   C4V_Int, C4S_KEEP_EFFECTS },

	{ "TEAMID_New", C4V_Int, TEAMID_New },

	{ "MSG_NoLinebreak", C4V_Int, C4GM_NoBreak },
	{ "MSG_Bottom",      C4V_Int, C4GM_Bottom },
	{ "MSG_Multiple",    C4V_Int, C4GM_Multiple },
	{ "MSG_Top",         C4V_Int, C4GM_Top },
	{ "MSG_Left",        C4V_Int, C4GM_Left },
	{ "MSG_Right",       C4V_Int, C4GM_Right },
	{ "MSG_HCenter",     C4V_Int, C4GM_HCenter },
	{ "MSG_VCenter",     C4V_Int, C4GM_VCenter },
	{ "MSG_DropSpeech",  C4V_Int, C4GM_DropSpeech },
	{ "MSG_WidthRel",    C4V_Int, C4GM_WidthRel },
	{ "MSG_XRel",        C4V_Int, C4GM_XRel },
	{ "MSG_YRel",        C4V_Int, C4GM_YRel },
	{ "MSG_ALeft",       C4V_Int, C4GM_ALeft },
	{ "MSG_ACenter",     C4V_Int, C4GM_ACenter },
	{ "MSG_ARight",      C4V_Int, C4GM_ARight },

	{ "C4PT_User",   C4V_Int, C4PT_User },
	{ "C4PT_Script", C4V_Int, C4PT_Script },

	{ "CSPF_FixedAttributes",    C4V_Int, CSPF_FixedAttributes },
	{ "CSPF_NoScenarioInit",     C4V_Int, CSPF_NoScenarioInit },
	{ "CSPF_NoEliminationCheck", C4V_Int, CSPF_NoEliminationCheck },
	{ "CSPF_Invisible",          C4V_Int, CSPF_Invisible },

	{ "RESTORE_None",          C4V_Int, C4NetworkRestartInfos::None },
	{ "RESTORE_ScriptPlayers", C4V_Int, C4NetworkRestartInfos::ScriptPlayers },
	{ "RESTORE_PlayerTeams",   C4V_Int, C4NetworkRestartInfos::PlayerTeams },

	{ "C4PVM_Cursor",    C4V_Int, C4PVM_Cursor },
	{ "C4PVM_Target",    C4V_Int, C4PVM_Target },
	{ "C4PVM_Scrolling", C4V_Int, C4PVM_Scrolling },
};

void InitFunctionMap(C4AulScriptEngine *pEngine)
{
	// add all def constants (all Int)
	for (const auto &def : C4ScriptConstMap)
		Game.ScriptEngine.RegisterGlobalConstant(def.Identifier, C4Value(def.Data, def.ValType));

	AddFunc(pEngine, "goto", Fn_goto);
	AddFunc(pEngine, "this", Fn_this);
	AddFunc(pEngine, "Equal", FnEqual);
	AddFunc(pEngine, "Var", FnVar);
	AddFunc(pEngine, "AssignVar", FnSetVar, false);
	AddFunc(pEngine, "SetVar", FnSetVar);
	AddFunc(pEngine, "IncVar", FnIncVar);
	AddFunc(pEngine, "DecVar", FnDecVar);
	AddFunc(pEngine, "SetGlobal", FnSetGlobal);
	AddFunc(pEngine, "Global", FnGlobal);
	AddFunc(pEngine, "SetLocal", FnSetLocal);
	AddFunc(pEngine, "Local", FnLocal);
	AddFunc(pEngine, "Explode", FnExplode);
	AddFunc(pEngine, "Incinerate", FnIncinerate);
	AddFunc(pEngine, "IncinerateLandscape", FnIncinerateLandscape);
	AddFunc(pEngine, "Extinguish", FnExtinguish);
	AddFunc(pEngine, "GrabContents", FnGrabContents);
	AddFunc(pEngine, "Punch", FnPunch);
	AddFunc(pEngine, "Kill", FnKill);
	AddFunc(pEngine, "Fling", FnFling);
	AddFunc(pEngine, "Jump", FnJump);
	AddFunc(pEngine, "ChangeDef", FnChangeDef);
	AddFunc(pEngine, "Exit", FnExit);
	AddFunc(pEngine, "Enter", FnEnter);
	AddFunc(pEngine, "Collect", FnCollect);
	AddFunc(pEngine, "Split2Components", FnSplit2Components);
	AddFunc(pEngine, "PlayerObjectCommand", FnPlayerObjectCommand);
	AddFunc(pEngine, "SetCommand", FnSetCommand);
	AddFunc(pEngine, "AddCommand", FnAddCommand);
	AddFunc(pEngine, "AppendCommand", FnAppendCommand);
	AddFunc(pEngine, "GetCommand", FnGetCommand);
	AddFunc(pEngine, "DeathAnnounce", FnDeathAnnounce);
	AddFunc(pEngine, "FindObject", FnFindObject);
	AddFunc(pEngine, "ObjectCount", FnObjectCount);
	AddFunc(pEngine, "ObjectCall", FnObjectCall);
	AddFunc(pEngine, "ProtectedCall", FnProtectedCall);
	AddFunc(pEngine, "PrivateCall", FnPrivateCall);
	AddFunc(pEngine, "GameCall", FnGameCall);
	AddFunc(pEngine, "GameCallEx", FnGameCallEx);
	AddFunc(pEngine, "DefinitionCall", FnDefinitionCall);
	AddFunc(pEngine, "Call", FnCall, false);
	AddFunc(pEngine, "GetPlrKnowledge", FnGetPlrKnowledge);
	AddFunc(pEngine, "GetPlrMagic", FnGetPlrMagic);
	AddFunc(pEngine, "GetComponent", FnGetComponent);
	AddFunc(pEngine, "PlayerMessage", FnPlayerMessage);
	AddFunc(pEngine, "Message", FnMessage);
	AddFunc(pEngine, "AddMessage", FnAddMessage);
	AddFunc(pEngine, "PlrMessage", FnPlrMessage);
	AddFunc(pEngine, "Log", FnLog);
	AddFunc(pEngine, "DebugLog", FnDebugLog);
	AddFunc(pEngine, "Format", FnFormat);
	AddFunc(pEngine, "EditCursor", FnEditCursor);
	AddFunc(pEngine, "AddMenuItem", FnAddMenuItem);
	AddFunc(pEngine, "SetSolidMask", FnSetSolidMask);
	AddFunc(pEngine, "SetGravity", FnSetGravity);
	AddFunc(pEngine, "GetGravity", FnGetGravity);
	AddFunc(pEngine, "GetHomebaseMaterial", FnGetHomebaseMaterial);
	AddFunc(pEngine, "GetHomebaseProduction", FnGetHomebaseProduction);
	AddFunc(pEngine, "IsRef", FnIsRef);
	AddFunc(pEngine, "GetType", FnGetType);
	AddFunc(pEngine, "CreateArray", FnCreateArray);
	AddFunc(pEngine, "GetLength", FnGetLength);
	AddFunc(pEngine, "GetIndexOf", FnGetIndexOf);
	AddFunc(pEngine, "GetDefCoreVal", FnGetDefCoreVal);
	AddFunc(pEngine, "GetActMapVal", FnGetActMapVal);
	AddFunc(pEngine, "GetObjectVal", FnGetObjectVal);
	AddFunc(pEngine, "GetObjectInfoCoreVal", FnGetObjectInfoCoreVal);
	AddFunc(pEngine, "GetScenarioVal", FnGetScenarioVal);
	AddFunc(pEngine, "GetPlayerVal", FnGetPlayerVal);
	AddFunc(pEngine, "GetPlayerInfoCoreVal", FnGetPlayerInfoCoreVal);
	AddFunc(pEngine, "GetMaterialVal", FnGetMaterialVal);
	AddFunc(pEngine, "SetPlrExtraData", FnSetPlrExtraData);
	AddFunc(pEngine, "GetPlrExtraData", FnGetPlrExtraData);
	AddFunc(pEngine, "SetCrewExtraData", FnSetCrewExtraData);
	AddFunc(pEngine, "GetCrewExtraData", FnGetCrewExtraData);
	AddFunc(pEngine, "GetPortrait", FnGetPortrait);
	AddFunc(pEngine, "AddEffect", FnAddEffect);
	AddFunc(pEngine, "GetEffect", FnGetEffect);
	AddFunc(pEngine, "CheckEffect", FnCheckEffect);
	AddFunc(pEngine, "EffectCall", FnEffectCall);
	AddFunc(pEngine, "eval", FnEval);
	AddFunc(pEngine, "VarN", FnVarN);
	AddFunc(pEngine, "LocalN", FnLocalN);
	AddFunc(pEngine, "GlobalN", FnGlobalN);
	AddFunc(pEngine, "Set", FnSet);
	AddFunc(pEngine, "Inc", FnInc);
	AddFunc(pEngine, "Dec", FnDec);
	AddFunc(pEngine, "SetLength", FnSetLength);
	AddFunc(pEngine, "SimFlight", FnSimFlight);
	AddFunc(pEngine, "EffectVar", FnEffectVar);
	AddFunc(pEngine, "Or",                              FnOr,                              false);
	AddFunc(pEngine, "Not",                             FnNot,                             false);
	AddFunc(pEngine, "And",                             FnAnd,                             false);
	AddFunc(pEngine, "BitAnd",                          FnBitAnd,                          false);
	AddFunc(pEngine, "Sum",                             FnSum,                             false);
	AddFunc(pEngine, "Sub",                             FnSub,                             false);
	AddFunc(pEngine, "Abs",                             Abs<C4ValueInt>);
	AddFunc(pEngine, "Min",                             FnMin);
	AddFunc(pEngine, "Max",                             FnMax);
	AddFunc(pEngine, "Mul",                             FnMul,                             false);
	AddFunc(pEngine, "Div",                             FnDiv,                             false);
	AddFunc(pEngine, "Mod",                             FnMod,                             false);
	AddFunc(pEngine, "Pow",                             FnPow,                             false);
	AddFunc(pEngine, "Sin",                             FnCircle<Sin>);
	AddFunc(pEngine, "Cos",                             FnCircle<Cos>);
	AddFunc(pEngine, "Sqrt",                            FnSqrt);
	AddFunc(pEngine, "ArcSin",                          FnArcus<std::asin>);
	AddFunc(pEngine, "ArcCos",                          FnArcus<std::acos>);
	AddFunc(pEngine, "LessThan",                        FnLessThan,                        false);
	AddFunc(pEngine, "GreaterThan",                     FnGreaterThan,                     false);
	AddFunc(pEngine, "BoundBy",                         BoundBy<C4ValueInt>);
	AddFunc(pEngine, "Inside",                          Inside<C4ValueInt>);
	AddFunc(pEngine, "SEqual",                          FnSEqual,                          false);
	AddFunc(pEngine, "Random",                          FnRandom);
	AddFunc(pEngine, "AsyncRandom",                     FnAsyncRandom);
	AddFunc(pEngine, "DoCon",                           FnDoCon);
	AddFunc(pEngine, "GetCon",                          FnGetCon);
	AddFunc(pEngine, "DoDamage",                        FnDoDamage);
	AddFunc(pEngine, "DoEnergy",                        FnDoEnergy);
	AddFunc(pEngine, "DoBreath",                        FnDoBreath);
	AddFunc(pEngine, "DoMagicEnergy",                   FnDoMagicEnergy);
	AddFunc(pEngine, "GetMagicEnergy",                  FnGetMagicEnergy);
	AddFunc(pEngine, "EnergyCheck",                     FnEnergyCheck);
	AddFunc(pEngine, "CheckEnergyNeedChain",            FnCheckEnergyNeedChain);
	AddFunc(pEngine, "GetEnergy",                       FnGetEnergy);
	AddFunc(pEngine, "OnFire",                          FnOnFire);
	AddFunc(pEngine, "Smoke",                           FnSmoke);
	AddFunc(pEngine, "Stuck",                           FnStuck);
	AddFunc(pEngine, "InLiquid",                        FnInLiquid);
	AddFunc(pEngine, "Bubble",                          FnBubble);
	AddFunc(pEngine, "SetAction",                       FnSetAction);
	AddFunc(pEngine, "SetActionData",                   FnSetActionData);
	AddFunc(pEngine, "SetBridgeActionData",             FnSetBridgeActionData);
	AddFunc(pEngine, "GetAction",                       FnGetAction);
	AddFunc(pEngine, "GetActTime",                      FnGetActTime);
	AddFunc(pEngine, "GetOwner",                        FnGetOwner);
	AddFunc(pEngine, "GetMass",                         FnGetMass);
	AddFunc(pEngine, "GetBreath",                       FnGetBreath);
	AddFunc(pEngine, "GetX",                            FnGetX);
	AddFunc(pEngine, "GetY",                            FnGetY);
	AddFunc(pEngine, "GetBase",                         FnGetBase);
	AddFunc(pEngine, "GetMenu",                         FnGetMenu);
	AddFunc(pEngine, "GetVertexNum",                    FnGetVertexNum);
	AddFunc(pEngine, "GetVertex",                       FnGetVertex);
	AddFunc(pEngine, "SetVertex",                       FnSetVertex);
	AddFunc(pEngine, "AddVertex",                       FnAddVertex);
	AddFunc(pEngine, "RemoveVertex",                    FnRemoveVertex);
	AddFunc(pEngine, "SetContactDensity",               FnSetContactDensity,               false);
	AddFunc(pEngine, "AnyContainer",                    Constant<ANY_CONTAINER>);
	AddFunc(pEngine, "NoContainer",                     Constant<NO_CONTAINER>);
	AddFunc(pEngine, "GetController",                   FnGetController);
	AddFunc(pEngine, "SetController",                   FnSetController);
	AddFunc(pEngine, "GetKiller",                       FnGetKiller);
	AddFunc(pEngine, "SetKiller",                       FnSetKiller);
	AddFunc(pEngine, "GetPhase",                        FnGetPhase);
	AddFunc(pEngine, "SetPhase",                        FnSetPhase);
	AddFunc(pEngine, "GetCategory",                     FnGetCategory);
	AddFunc(pEngine, "GetOCF",                          FnGetOCF);
	AddFunc(pEngine, "SetAlive",                        FnSetAlive);
	AddFunc(pEngine, "GetAlive",                        FnGetAlive);
	AddFunc(pEngine, "GetDamage",                       FnGetDamage);
	AddFunc(pEngine, "CrewMember",                      FnCrewMember);
	AddFunc(pEngine, "ObjectSetAction",                 FnObjectSetAction,                 false);
	AddFunc(pEngine, "ComponentAll",                    FnComponentAll);
	AddFunc(pEngine, "SetComDir",                       FnSetComDir);
	AddFunc(pEngine, "GetComDir",                       FnGetComDir);
	AddFunc(pEngine, "SetDir",                          FnSetDir);
	AddFunc(pEngine, "GetDir",                          FnGetDir);
	AddFunc(pEngine, "SetEntrance",                     FnSetEntrance);
	AddFunc(pEngine, "GetEntrance",                     FnGetEntrance);
	AddFunc(pEngine, "SetCategory",                     FnSetCategory);
	AddFunc(pEngine, "FinishCommand",                   FnFinishCommand);
	AddFunc(pEngine, "GetDefinition",                   FnGetDefinition);
	AddFunc(pEngine, "ActIdle",                         FnActIdle);
	AddFunc(pEngine, "SetRDir",                         FnSetRDir);
	AddFunc(pEngine, "GetRDir",                         FnGetRDir);
	AddFunc(pEngine, "GetXDir",                         FnGetXDir);
	AddFunc(pEngine, "GetYDir",                         FnGetYDir);
	AddFunc(pEngine, "GetR",                            FnGetR);
	AddFunc(pEngine, "GetName",                         FnGetName);
	AddFunc(pEngine, "SetName",                         FnSetName);
	AddFunc(pEngine, "GetDesc",                         FnGetDesc);
	AddFunc(pEngine, "GetPlayerName",                   FnGetPlayerName);
	AddFunc(pEngine, "GetTaggedPlayerName",             FnGetTaggedPlayerName);
	AddFunc(pEngine, "GetPlayerType",                   FnGetPlayerType);
	AddFunc(pEngine, "SetXDir",                         FnSetXDir);
	AddFunc(pEngine, "SetYDir",                         FnSetYDir);
	AddFunc(pEngine, "SetR",                            FnSetR);
	AddFunc(pEngine, "SetOwner",                        FnSetOwner);
	AddFunc(pEngine, "CreateObject",                    FnCreateObject);
	AddFunc(pEngine, "MakeCrewMember",                  FnMakeCrewMember);
	AddFunc(pEngine, "GrabObjectInfo",                  FnGrabObjectInfo);
	AddFunc(pEngine, "CreateContents",                  FnCreateContents);
	AddFunc(pEngine, "ShiftContents",                   FnShiftContents);
	AddFunc(pEngine, "ComposeContents",                 FnComposeContents);
	AddFunc(pEngine, "CreateConstruction",              FnCreateConstruction);
	AddFunc(pEngine, "GetID",                           FnGetID);
	AddFunc(pEngine, "Contents",                        FnContents);
	AddFunc(pEngine, "ScrollContents",                  FnScrollContents);
	AddFunc(pEngine, "Contained",                       FnContained);
	AddFunc(pEngine, "ContentsCount",                   FnContentsCount);
	AddFunc(pEngine, "FindContents",                    FnFindContents);
	AddFunc(pEngine, "FindConstructionSite",            FnFindConstructionSite);
	AddFunc(pEngine, "FindOtherContents",               FnFindOtherContents);
	AddFunc(pEngine, "FindBase",                        FnFindBase);
	AddFunc(pEngine, "Sound",                           FnSound);
	AddFunc(pEngine, "Music",                           FnMusic);
	AddFunc(pEngine, "MusicLevel",                      FnMusicLevel);
	AddFunc(pEngine, "SetPlayList",                     FnSetPlayList);
	AddFunc(pEngine, "SoundLevel",                      FnSoundLevel,                      false);
	AddFunc(pEngine, "FindObjectOwner",                 FnFindObjectOwner);
	AddFunc(pEngine, "RemoveObject",                    FnRemoveObject);
	AddFunc(pEngine, "GetActionTarget",                 FnGetActionTarget);
	AddFunc(pEngine, "SetActionTargets",                FnSetActionTargets);
	AddFunc(pEngine, "SetPlrView",                      FnSetPlrView);
	AddFunc(pEngine, "SetPlrKnowledge",                 FnSetPlrKnowledge);
	AddFunc(pEngine, "SetPlrMagic",                     FnSetPlrMagic);
	AddFunc(pEngine, "GetPlrDownDouble",                &C4Player::LastComDownDouble);
	AddFunc(pEngine, "ClearLastPlrCom",                 FnClearLastPlrCom);
	AddFunc(pEngine, "GetPlrViewMode",                  FnGetPlrViewMode);
	AddFunc(pEngine, "GetPlrView",                      FnGetPlrView);
	AddFunc(pEngine, "GetWealth",                       &C4Player::Wealth);
	AddFunc(pEngine, "SetWealth",                       FnSetWealth);
	AddFunc(pEngine, "SetComponent",                    FnSetComponent);
	AddFunc(pEngine, "DoScore",                         &C4Player::DoPoints);
	AddFunc(pEngine, "GetScore",                        &C4Player::Points);
	AddFunc(pEngine, "GetPlrValue",                     &C4Player::Value);
	AddFunc(pEngine, "GetPlrValueGain",                 &C4Player::ValueGain);
	AddFunc(pEngine, "SetPlrShowControl",               FnSetPlrShowControl);
	AddFunc(pEngine, "SetPlrShowControlPos",            FnSetPlrShowControlPos);
	AddFunc(pEngine, "GetPlrControlName",               FnGetPlrControlName);
	AddFunc(pEngine, "GetPlrJumpAndRunControl",         FnGetPlrJumpAndRunControl);
	AddFunc(pEngine, "SetPlrShowCommand",               FnSetPlrShowCommand);
	AddFunc(pEngine, "GetWind",                         FnGetWind);
	AddFunc(pEngine, "SetWind",                         Game.Weather, &C4Weather::SetWind);
	AddFunc(pEngine, "SetSkyFade",                      FnSetSkyFade);
	AddFunc(pEngine, "SetSkyColor",                     FnSetSkyColor);
	AddFunc(pEngine, "GetSkyColor",                     FnGetSkyColor);
	AddFunc(pEngine, "GetTemperature",                  Game.Weather, &C4Weather::GetTemperature);
	AddFunc(pEngine, "SetTemperature",                  Game.Weather, &C4Weather::SetTemperature);
	AddFunc(pEngine, "LaunchLightning",                 Game.Weather, &C4Weather::LaunchLightning);
	AddFunc(pEngine, "LaunchVolcano",                   FnLaunchVolcano);
	AddFunc(pEngine, "LaunchEarthquake",                Game.Weather, &C4Weather::LaunchEarthquake);
	AddFunc(pEngine, "ShakeFree",                       Game.Landscape, &C4Landscape::ShakeFree);
	AddFunc(pEngine, "ShakeObjects",                    FnShakeObjects);
	AddFunc(pEngine, "DigFree",                         FnDigFree);
	AddFunc(pEngine, "FreeRect",                        FnFreeRect);
	AddFunc(pEngine, "DigFreeRect",                     FnDigFreeRect);
	AddFunc(pEngine, "CastPXS",                         FnCastPXS);
	AddFunc(pEngine, "CastObjects",                     FnCastObjects);
	AddFunc(pEngine, "Hostile",                         FnHostile);
	AddFunc(pEngine, "SetHostility",                    FnSetHostility);
	AddFunc(pEngine, "PlaceVegetation",                 FnPlaceVegetation);
	AddFunc(pEngine, "PlaceAnimal",                     Game, &C4Game::PlaceAnimal);
	AddFunc(pEngine, "GameOver",                        FnGameOver);
	AddFunc(pEngine, "C4Id",                            FnC4Id);
	AddFunc(pEngine, "ScriptGo",                        FnScriptGo);
	AddFunc(pEngine, "GetHiRank",                       FnGetHiRank);
	AddFunc(pEngine, "GetCrew",                         FnGetCrew);
	AddFunc(pEngine, "GetCrewCount",                    FnGetCrewCount);
	AddFunc(pEngine, "GetPlayerCount",                  FnGetPlayerCount);
	AddFunc(pEngine, "GetPlayerByIndex",                FnGetPlayerByIndex);
	AddFunc(pEngine, "EliminatePlayer",                 FnEliminatePlayer);
	AddFunc(pEngine, "SurrenderPlayer",                 FnSurrenderPlayer);
	AddFunc(pEngine, "SetLeaguePerformance",            FnSetLeaguePerformance);
	AddFunc(pEngine, "SetLeagueProgressData",           FnSetLeagueProgressData);
	AddFunc(pEngine, "GetLeagueProgressData",           FnGetLeagueProgressData);
	AddFunc(pEngine, "CreateScriptPlayer",              FnCreateScriptPlayer);
	AddFunc(pEngine, "GetCursor",                       FnGetCursor);
	AddFunc(pEngine, "GetViewCursor",                   &C4Player::ViewCursor);
	AddFunc(pEngine, "GetCaptain",                      &C4Player::Captain);
	AddFunc(pEngine, "SetCursor",                       FnSetCursor);
	AddFunc(pEngine, "SetViewCursor",                   FnSetViewCursor);
	AddFunc(pEngine, "SelectCrew",                      FnSelectCrew);
	AddFunc(pEngine, "GetSelectCount",                  &C4Player::SelectCount);
	AddFunc(pEngine, "SetCrewStatus",                   FnSetCrewStatus,                   false);
	AddFunc(pEngine, "SetPosition",                     FnSetPosition);
	AddFunc(pEngine, "ExtractLiquid",                   FnExtractLiquid);
	AddFunc(pEngine, "GetMaterial",                     FnGetMaterial);
	AddFunc(pEngine, "GetTexture",                      FnGetTexture);
	AddFunc(pEngine, "GetMaterialCount",                FnGetMaterialCount);
	AddFunc(pEngine, "GBackSolid",                      FnGBackSolid);
	AddFunc(pEngine, "GBackSemiSolid",                  FnGBackSemiSolid);
	AddFunc(pEngine, "GBackLiquid",                     FnGBackLiquid);
	AddFunc(pEngine, "GBackSky",                        FnGBackSky);
	AddFunc(pEngine, "Material",                        FnMaterial);
	AddFunc(pEngine, "BlastObjects",                    FnBlastObjects);
	AddFunc(pEngine, "BlastObject",                     FnBlastObject);
	AddFunc(pEngine, "BlastFree",                       FnBlastFree);
	AddFunc(pEngine, "InsertMaterial",                  FnInsertMaterial);
	AddFunc(pEngine, "DrawVolcanoBranch",               FnDrawVolcanoBranch,               false);
	AddFunc(pEngine, "FlameConsumeMaterial",            FnFlameConsumeMaterial,            false);
	AddFunc(pEngine, "LandscapeWidth",                  Game.Landscape, &C4Landscape::Width);
	AddFunc(pEngine, "LandscapeHeight",                 Game.Landscape, &C4Landscape::Height);
	AddFunc(pEngine, "Resort",                          FnResort);
	AddFunc(pEngine, "CreateMenu",                      FnCreateMenu);
	AddFunc(pEngine, "SelectMenuItem",                  FnSelectMenuItem);
	AddFunc(pEngine, "SetMenuDecoration",               FnSetMenuDecoration);
	AddFunc(pEngine, "SetMenuTextProgress",             FnSetMenuTextProgress);
	AddFunc(pEngine, "SetSeason",                       Game.Weather, &C4Weather::SetSeason);
	AddFunc(pEngine, "GetSeason",                       Game.Weather, &C4Weather::GetSeason);
	AddFunc(pEngine, "SetClimate",                      Game.Weather, &C4Weather::SetClimate);
	AddFunc(pEngine, "GetClimate",                      Game.Weather, &C4Weather::GetClimate);
	AddFunc(pEngine, "Distance",                        Distance);
	AddFunc(pEngine, "ObjectDistance",                  FnObjectDistance);
	AddFunc(pEngine, "GetValue",                        FnGetValue);
	AddFunc(pEngine, "GetRank",                         FnGetRank);
	AddFunc(pEngine, "Value",                           FnValue);
	AddFunc(pEngine, "Angle",                           FnAngle);
	AddFunc(pEngine, "DoHomebaseMaterial",              FnDoHomebaseMaterial);
	AddFunc(pEngine, "DoHomebaseProduction",            FnDoHomebaseProduction);
	AddFunc(pEngine, "GainMissionAccess",               FnGainMissionAccess);
	AddFunc(pEngine, "SetPhysical",                     FnSetPhysical);
	AddFunc(pEngine, "TrainPhysical",                   FnTrainPhysical);
	AddFunc(pEngine, "GetPhysical",                     FnGetPhysical);
	AddFunc(pEngine, "ResetPhysical",                   FnResetPhysical);
	AddFunc(pEngine, "SetTransferZone",                 FnSetTransferZone);
	AddFunc(pEngine, "IsNetwork",                       Game.Parameters, &C4GameParameters::IsNetworkGame);
	AddFunc(pEngine, "GetLeague",                       FnGetLeague);
	AddFunc(pEngine, "TestMessageBoard",                FnTestMessageBoard,                false);
	AddFunc(pEngine, "CallMessageBoard",                FnCallMessageBoard,                false);
	AddFunc(pEngine, "AbortMessageBoard",               FnAbortMessageBoard,               false);
	AddFunc(pEngine, "OnMessageBoardAnswer",            FnOnMessageBoardAnswer,            false);
	AddFunc(pEngine, "ScriptCounter",                   Game.Script, &C4GameScriptHost::Counter);
	AddFunc(pEngine, "SetMass",                         FnSetMass);
	AddFunc(pEngine, "GetColor",                        Constant<0, C4Object *>); // oldgfx
	AddFunc(pEngine, "SetColor",                        FnSetColor);
	AddFunc(pEngine, "SetFoW",                          FnSetFoW);
	AddFunc(pEngine, "SetPlrViewRange",                 FnSetPlrViewRange);
	AddFunc(pEngine, "GetMaxPlayer",                    Game.Parameters, &C4GameParameters::MaxPlayers);
	AddFunc(pEngine, "SetMaxPlayer",                    FnSetMaxPlayer);
	AddFunc(pEngine, "SetPicture",                      FnSetPicture);
	AddFunc(pEngine, "Buy",                             FnBuy);
	AddFunc(pEngine, "Sell",                            FnSell);
	AddFunc(pEngine, "GetProcedure",                    FnGetProcedure);
	AddFunc(pEngine, "GetChar",                         FnGetChar);
	AddFunc(pEngine, "ActivateGameGoalMenu",            FnActivateGameGoalMenu);
	AddFunc(pEngine, "SetGraphics",                     FnSetGraphics);
	AddFunc(pEngine, "Object",                          Game.Objects, &C4GameObjects::SafeObjectPointer);
	AddFunc(pEngine, "ObjectNumber",                    FnObjectNumber);
	AddFunc(pEngine, "ShowInfo",                        FnShowInfo);
	AddFunc(pEngine, "GetTime",                         FnGetTime);
	AddFunc(pEngine, "GetSystemTime",                   FnGetSystemTime,                   false);
	AddFunc(pEngine, "SetVisibility",                   FnSetVisibility);
	AddFunc(pEngine, "GetVisibility",                   FnGetVisibility);
	AddFunc(pEngine, "SetClrModulation",                FnSetClrModulation);
	AddFunc(pEngine, "GetClrModulation",                FnGetClrModulation);
	AddFunc(pEngine, "GetMissionAccess",                FnGetMissionAccess);
	AddFunc(pEngine, "CloseMenu",                       FnCloseMenu);
	AddFunc(pEngine, "GetMenuSelection",                FnGetMenuSelection);
	AddFunc(pEngine, "ResortObjects",                   FnResortObjects);
	AddFunc(pEngine, "ResortObject",                    FnResortObject);
	AddFunc(pEngine, "GetDefBottom",                    FnGetDefBottom);
	AddFunc(pEngine, "SetMaterialColor",                FnSetMaterialColor);
	AddFunc(pEngine, "GetMaterialColor",                FnGetMaterialColor);
	AddFunc(pEngine, "MaterialName",                    FnMaterialName);
	AddFunc(pEngine, "SetMenuSize",                     FnSetMenuSize);
	AddFunc(pEngine, "GetNeededMatStr",                 FnGetNeededMatStr);
	AddFunc(pEngine, "GetCrewEnabled",                  FnGetCrewEnabled);
	AddFunc(pEngine, "SetCrewEnabled",                  FnSetCrewEnabled);
	AddFunc(pEngine, "UnselectCrew",                    &C4Player::UnselectCrew);
	AddFunc(pEngine, "DrawMap",                         FnDrawMap);
	AddFunc(pEngine, "DrawDefMap",                      FnDrawDefMap);
	AddFunc(pEngine, "CreateParticle",                  FnCreateParticle);
	AddFunc(pEngine, "CastParticles",                   FnCastParticles);
	AddFunc(pEngine, "CastBackParticles",               FnCastBackParticles);
	AddFunc(pEngine, "PushParticles",                   FnPushParticles);
	AddFunc(pEngine, "ClearParticles",                  FnClearParticles);
	AddFunc(pEngine, "IsNewgfx",                        Constant<true>,                        false);
	AddFunc(pEngine, "SetSkyAdjust",                    Game.Landscape.Sky, &C4Sky::SetModulation);
	AddFunc(pEngine, "SetMatAdjust",                    Game.Landscape, &C4Landscape::SetModulation);
	AddFunc(pEngine, "GetSkyAdjust",                    Game.Landscape.Sky, &C4Sky::GetModulation);
	AddFunc(pEngine, "GetMatAdjust",                    Game.Landscape, &C4Landscape::GetModulation);
	AddFunc(pEngine, "SetSkyParallax",                  FnSetSkyParallax);
	AddFunc(pEngine, "DoCrewExp",                       FnDoCrewExp);
	AddFunc(pEngine, "ReloadDef",                       FnReloadDef);
	AddFunc(pEngine, "ReloadParticle",                  FnReloadParticle);
	AddFunc(pEngine, "SetGamma",                        Game.GraphicsSystem, &C4GraphicsSystem::SetGamma);
	AddFunc(pEngine, "ResetGamma",                      FnResetGamma);
	AddFunc(pEngine, "FrameCounter",                    Game, &C4Game::FrameCounter);
	AddFunc(pEngine, "SetLandscapePixel",               FnSetLandscapePixel);
	AddFunc(pEngine, "SetObjectOrder",                  FnSetObjectOrder);
	AddFunc(pEngine, "SetColorDw",                      FnSetColorDw);
	AddFunc(pEngine, "GetColorDw",                      FnGetColorDw);
	AddFunc(pEngine, "GetPlrColorDw",                   &C4Player::ColorDw);
	AddFunc(pEngine, "DrawMaterialQuad",                FnDrawMaterialQuad);
	AddFunc(pEngine, "FightWith",                       FnFightWith);
	AddFunc(pEngine, "SetFilmView",                     FnSetFilmView);
	AddFunc(pEngine, "ClearMenuItems",                  FnClearMenuItems);
	AddFunc(pEngine, "GetObjectLayer",                  FnGetObjectLayer,                  false);
	AddFunc(pEngine, "SetObjectLayer",                  FnSetObjectLayer,                  false);
	AddFunc(pEngine, "SetShape",                        FnSetShape);
	AddFunc(pEngine, "AddMsgBoardCmd",                  FnAddMsgBoardCmd);
	AddFunc(pEngine, "SetGameSpeed",                    FnSetGameSpeed,                    false);
	AddFunc(pEngine, "DrawMatChunks",                   FnDrawMatChunks,                   false);
	AddFunc(pEngine, "GetPath",                         FnGetPath);
	AddFunc(pEngine, "SetTextureIndex",                 FnSetTextureIndex,                 false);
	AddFunc(pEngine, "RemoveUnusedTexMapEntries",       Game.Landscape, &C4Landscape::RemoveUnusedTexMapEntries,       false);
	AddFunc(pEngine, "SetObjDrawTransform",             FnSetObjDrawTransform);
	AddFunc(pEngine, "SetObjDrawTransform2",            FnSetObjDrawTransform2,            false);
	AddFunc(pEngine, "SetPortrait",                     FnSetPortrait);
	AddFunc(pEngine, "LoadScenarioSection",             FnLoadScenarioSection,             false);
	AddFunc(pEngine, "SetObjectStatus",                 FnSetObjectStatus,                 false);
	AddFunc(pEngine, "GetObjectStatus",                 FnGetObjectStatus,                 false);
	AddFunc(pEngine, "AdjustWalkRotation",              FnAdjustWalkRotation,              false);
	AddFunc(pEngine, "FxFireStart",                     FnFxFireStart,                     false);
	AddFunc(pEngine, "FxFireTimer",                     FnFxFireTimer,                     false);
	AddFunc(pEngine, "FxFireStop",                      FnFxFireStop,                      false);
	AddFunc(pEngine, "FxFireInfo",                      FnFxFireInfo,                      false);
	AddFunc(pEngine, "RemoveEffect",                    FnRemoveEffect);
	AddFunc(pEngine, "ChangeEffect",                    FnChangeEffect);
	AddFunc(pEngine, "ModulateColor",                   FnModulateColor);
	AddFunc(pEngine, "WildcardMatch",                   FnWildcardMatch);
	AddFunc(pEngine, "GetContact",                      FnGetContact);
	AddFunc(pEngine, "SetObjectBlitMode",               FnSetObjectBlitMode);
	AddFunc(pEngine, "GetObjectBlitMode",               FnGetObjectBlitMode);
	AddFunc(pEngine, "SetViewOffset",                   FnSetViewOffset);
	AddFunc(pEngine, "SetPreSend",                      FnSetPreSend,                      false);
	AddFunc(pEngine, "GetPlayerID",                     &C4Player::ID,                     false);
	AddFunc(pEngine, "GetPlayerTeam",                   FnGetPlayerTeam);
	AddFunc(pEngine, "SetPlayerTeam",                   FnSetPlayerTeam);
	AddFunc(pEngine, "GetTeamConfig",                   FnGetTeamConfig);
	AddFunc(pEngine, "GetTeamName",                     FnGetTeamName);
	AddFunc(pEngine, "GetTeamColor",                    FnGetTeamColor);
	AddFunc(pEngine, "GetTeamByIndex",                  FnGetTeamByIndex);
	AddFunc(pEngine, "GetTeamCount",                    Game.Teams, &C4TeamList::GetTeamCount);
	AddFunc(pEngine, "InitScenarioPlayer",              &C4Player::ScenarioAndTeamInit,              false);
	AddFunc(pEngine, PSF_OnOwnerRemoved,                FnOnOwnerRemoved,                  false);
	AddFunc(pEngine, "SetScoreboardData",               FnSetScoreboardData,               false);
	AddFunc(pEngine, "GetScoreboardString",             FnGetScoreboardString,             false);
	AddFunc(pEngine, "GetScoreboardData",               FnGetScoreboardData,               false);
	AddFunc(pEngine, "DoScoreboardShow",                FnDoScoreboardShow,                false);
	AddFunc(pEngine, "SortScoreboard",                  Game.Scoreboard, &C4Scoreboard::SortBy,                  false);
	AddFunc(pEngine, "AddEvaluationData",               FnAddEvaluationData,               false);
	AddFunc(pEngine, "GetLeagueScore",                  &C4PlayerInfo::getLeagueScore,                  false);
	AddFunc(pEngine, "HideSettlementScoreInEvaluation", Game.RoundResults, &C4RoundResults::HideSettlementScore, false);
	AddFunc(pEngine, "GetUnusedOverlayID",              FnGetUnusedOverlayID,              false);
	AddFunc(pEngine, "FatalError",                      FnFatalError,                      false);
	AddFunc(pEngine, "ExtractMaterialAmount",           FnExtractMaterialAmount);
	AddFunc(pEngine, "GetEffectCount",                  FnGetEffectCount);
	extern void C4AulStartTrace();
	AddFunc(pEngine, "StartCallTrace",                  C4AulStartTrace);
	AddFunc(pEngine, "StartScriptProfiler",             FnStartScriptProfiler);
	AddFunc(pEngine, "StopScriptProfiler",              C4AulProfiler::StopProfiling);
	AddFunc(pEngine, "CustomMessage",                   FnCustomMessage);
	AddFunc(pEngine, "PauseGame",                       FnPauseGame);
	AddFunc(pEngine, "ExecuteCommand",                  FnExecuteCommand);
	AddFunc(pEngine, "LocateFunc",                      FnLocateFunc);
	AddFunc(pEngine, "PathFree",                        FnPathFree);
	AddFunc(pEngine, "PathFree2",                       FnPathFree2);
	AddFunc(pEngine, "SetNextMission",                  FnSetNextMission);
	AddFunc(pEngine, "GetKeys",                         FnGetKeys);
	AddFunc(pEngine, "GetValues",                       FnGetValues);
	AddFunc(pEngine, "SetRestoreInfos",                 FnSetRestoreInfos);
	new C4AulDefCastFunc<C4V_C4ID, C4V_Int>{pEngine, "ScoreboardCol"};
	new C4AulDefCastFunc<C4V_Any, C4V_Int>{pEngine, "CastInt"};
	new C4AulDefCastFunc<C4V_Any, C4V_Bool>{pEngine, "CastBool"};
	new C4AulDefCastFunc<C4V_Any, C4V_C4ID>{pEngine, "CastC4ID"};
	new C4AulDefCastFunc<C4V_Any, C4V_Any>{pEngine, "CastAny"};

	new C4AulEngineFuncParArray<10, C4V_C4Object>{pEngine, "FindObject2",  FnFindObject2,  C4V_Array,    C4V_Array,    C4V_Array,    C4V_Array,    C4V_Array,    C4V_Array,    C4V_Array,    C4V_Array,    C4V_Array, C4V_Array};
	new C4AulEngineFuncParArray<10, C4V_Array>   {pEngine, "FindObjects",  FnFindObjects,  C4V_Array,    C4V_Any,      C4V_Any,      C4V_Any,      C4V_Any,      C4V_Any,      C4V_Any,      C4V_Any,      C4V_Any,   C4V_Any};
	new C4AulEngineFuncParArray<10, C4V_Int>     {pEngine, "ObjectCount2", FnObjectCount2, C4V_Array,    C4V_Array,    C4V_Array,    C4V_Array,    C4V_Array,    C4V_Array,    C4V_Array,    C4V_Array,    C4V_Array, C4V_Array};
}
