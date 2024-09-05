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

// C4AulFun-based effects assigned to an object
/* Also contains some helper functions for various landscape effects */

#include <C4Include.h>

#include <C4Object.h>
#include <C4Random.h>
#include <C4Log.h>
#include <C4Game.h>
#include <C4Wrappers.h>

#include <format>
#include <numbers>

void C4Effect::AssignCallbackFunctions()
{
	C4AulScript *pSrcScript = GetCallbackScript();
	// compose function names and search them
	pFnStart  = pSrcScript->GetFuncRecursive(std::format(PSF_FxStart, Name).c_str());
	pFnStop   = pSrcScript->GetFuncRecursive(std::format(PSF_FxStop, Name).c_str());
	pFnTimer  = pSrcScript->GetFuncRecursive(std::format(PSF_FxTimer, Name).c_str());
	pFnEffect = pSrcScript->GetFuncRecursive(std::format(PSF_FxEffect, Name).c_str());
	pFnDamage = pSrcScript->GetFuncRecursive(std::format(PSF_FxDamage, Name).c_str());
}

C4AulScript *C4Effect::GetCallbackScript()
{
	// def script or global only?
	C4AulScript *pSrcScript; C4Def *pDef;
	if (pCommandTarget)
	{
		pSrcScript = &pCommandTarget->Def->Script;
		// overwrite ID for sync safety in runtime join
		idCommandTarget = pCommandTarget->id;
	}
	else if (idCommandTarget && (pDef = Game.Defs.ID2Def(idCommandTarget)))
		pSrcScript = &pDef->Script;
	else
		pSrcScript = &Game.ScriptEngine;
	return pSrcScript;
}

C4Effect::C4Effect(C4Object *pForObj, const char *szName, int32_t iPrio, int32_t iTimerIntervall, C4Object *pCmdTarget, C4ID idCmdTarget, const C4Value &rVal1, const C4Value &rVal2, const C4Value &rVal3, const C4Value &rVal4, bool fDoCalls, int32_t &riStoredAsNumber, bool passErrors)
	: EffectVars(0)
{
	C4Effect *pPrev, *pCheck;
	// assign values
	SCopy(szName, Name, C4MaxDefString);
	iPriority = 0; // effect is not yet valid; some callbacks to other effects are done before
	riStoredAsNumber = 0;
	iIntervall = iTimerIntervall;
	iTime = 0;
	pCommandTarget = pCmdTarget;
	pCommandTarget.Enumerate();
	idCommandTarget = idCmdTarget;
	AssignCallbackFunctions();
	// get effect target
	C4Effect **ppEffectList = pForObj ? &pForObj->pEffects : &Game.pGlobalEffects;
	// assign a unique number for that object
	iNumber = 1;
	for (pCheck = *ppEffectList; pCheck; pCheck = pCheck->pNext)
		if (pCheck->iNumber >= iNumber) iNumber = pCheck->iNumber + 1;
	// register into object
	pPrev = *ppEffectList;
	if (pPrev && Abs(pPrev->iPriority) < iPrio)
	{
		while (pCheck = pPrev->pNext)
			if (Abs(pCheck->iPriority) >= iPrio) break; else pPrev = pCheck;
		// insert after previous
		pNext = pPrev->pNext;
		pPrev->pNext = this;
	}
	else
	{
		// insert as first effect
		pNext = *ppEffectList;
		*ppEffectList = this;
	}
	// no calls to be done: finished here
	if (!fDoCalls) return;
	// ask all effects with higher priority first - except for prio 1 effects, which are considered out of the priority call chain (as per doc)
	try
	{
		bool fRemoveUpper = (iPrio != 1);
		// note that apart from denying the creation of this effect, higher priority effects may also remove themselves
		// or do other things with the effect list
		// (which does not quite make sense, because the effect might be denied by another effect)
		// so the priority is assigned after this call, marking this effect dead before it's definitely valid
		if (fRemoveUpper && pNext)
		{
			int32_t iResult = pNext->Check(pForObj, Name, iPrio, iIntervall, rVal1, rVal2, rVal3, rVal4, passErrors);
			if (iResult)
			{
				// effect denied (iResult = -1), added to an effect (iResult = Number of that effect)
				// or added to an effect that destroyed itself (iResult = -2)
				if (iResult != C4Fx_Effect_Deny) riStoredAsNumber = iResult;
				// effect is still marked dead
				return;
			}
		}
		// init effect
		// higher-priority effects must be deactivated temporarily, and then reactivated regarding the new effect
		// higher-level effects should not be inserted during the process of removing or adding a lower-level effect
		// because that would cause a wrong initialization order
		// (hardly ever causing trouble, however...)
		C4Effect *pLastRemovedEffect = nullptr;
		if (fRemoveUpper && pNext && pFnStart)
			TempRemoveUpperEffects(pForObj, false, &pLastRemovedEffect);
		// bad things may happen
		if (pForObj && !pForObj->Status) return; // this will be invalid!
		iPriority = iPrio; // validate effect now
		if (pFnStart)
			if (pFnStart->Exec(pCommandTarget, {C4VObj(pForObj), C4VInt(iNumber), C4VInt(0), rVal1, rVal2, rVal3, rVal4}, true, true).getInt() == C4Fx_Start_Deny)
				// the effect denied to start: assume it hasn't, and mark it dead
				SetDead();
		if (fRemoveUpper && pNext && pFnStart)
			TempReaddUpperEffects(pForObj, pLastRemovedEffect);
		if (pForObj && !pForObj->Status) return; // this will be invalid!
		// this effect has been created; hand back the number
		riStoredAsNumber = iNumber;
	}
	catch (...)
	{
		if (*ppEffectList == this)
		{
			*ppEffectList = pNext;
		}
		else
		{
			pPrev->pNext = pNext;
		}
		pNext = nullptr;
		throw;
	}
}

C4Effect::C4Effect(StdCompiler *pComp) : EffectVars(0)
{
	// defaults
	iNumber = iPriority = iTime = iIntervall = 0;
	pNext = nullptr;
	// compile
	pComp->Value(*this);
}

C4Effect::~C4Effect()
{
	// del following effects (not recursively)
	C4Effect *pEffect;
	while (pEffect = pNext)
	{
		pNext = pEffect->pNext;
		pEffect->pNext = nullptr;
		delete pEffect;
	}
}

void C4Effect::EnumeratePointers()
{
	// enum in all effects
	C4Effect *pEff = this;
	do
	{
		// command target
		pEff->pCommandTarget.Enumerate();
		// effect var denumeration: not necessary, because this is done while saving
	} while (pEff = pEff->pNext);
}

void C4Effect::DenumeratePointers()
{
	// denum in all effects
	C4Effect *pEff = this;
	do
	{
		// command target
		pEff->pCommandTarget.Denumerate();
		// variable pointers
		pEff->EffectVars.DenumeratePointers();
		// assign any callback functions
		pEff->AssignCallbackFunctions();
	} while (pEff = pEff->pNext);
}

void C4Effect::ClearPointers(C4Object *pObj)
{
	// clear pointers in all effects
	C4Effect *pEff = this;
	do
		// command target lost: effect dead w/o callback
		if (pEff->pCommandTarget == pObj)
		{
			pEff->SetDead();
			pEff->pCommandTarget = nullptr;
		}
	while (pEff = pEff->pNext);
}

C4Effect *C4Effect::Get(const char *szName, int32_t iIndex, int32_t iMaxPriority)
{
	// safety
	if (!szName) return nullptr;
	// check all effects
	C4Effect *pEff = this;
	do
	{
		// skip dead
		if (pEff->IsDead()) continue;
		// skip effects with too high priority
		if (iMaxPriority && pEff->iPriority > iMaxPriority) continue;
		// wildcard compare name
		const char *szEffectName = pEff->Name;
		if (!SWildcardMatchEx(szEffectName, szName)) continue;
		// effect name matches
		// check index
		if (iIndex--) continue;
		// effect found
		return pEff;
	} while (pEff = pEff->pNext);
	// nothing found
	return nullptr;
}

C4Effect *C4Effect::Get(int32_t iNumber, bool fIncludeDead, int32_t iMaxPriority)
{
	// check all effects
	C4Effect *pEff = this;
	do
		if (pEff->iNumber == iNumber)
		{
			if (!pEff->IsDead() || fIncludeDead)
				if (!iMaxPriority || pEff->iPriority <= iMaxPriority)
					return pEff;
			// effect found but denied
			return nullptr;
		}
	while (pEff = pEff->pNext);
	// nothing found
	return nullptr;
}

int32_t C4Effect::GetCount(const char *szMask, int32_t iMaxPriority)
{
	// count all matching effects
	int32_t iCnt = 0; C4Effect *pEff = this;
	do if (!pEff->IsDead())
		if (!szMask || SWildcardMatchEx(pEff->Name, szMask))
			if (!iMaxPriority || pEff->iPriority <= iMaxPriority)
				++iCnt;
	while (pEff = pEff->pNext);
	// return count
	return iCnt;
}

int32_t C4Effect::Check(C4Object *pForObj, const char *szCheckEffect, int32_t iPrio, int32_t iTimer, const C4Value &rVal1, const C4Value &rVal2, const C4Value &rVal3, const C4Value &rVal4, bool passErrors)
{
	// priority=1: always OK; no callbacks
	if (iPrio == 1) return 0;
	// check this and other effects
	C4Effect *pAddToEffect = nullptr; bool fDoTempCallsForAdd = false;
	C4Effect *pLastRemovedEffect = nullptr;
	for (C4Effect *pCheck = this; pCheck; pCheck = pCheck->pNext)
	{
		if (!pCheck->IsDead() && pCheck->pFnEffect && pCheck->iPriority >= iPrio)
		{
			int32_t iResult = pCheck->pFnEffect->Exec(pCheck->pCommandTarget, {C4VString(szCheckEffect), C4VObj(pForObj), C4VInt(pCheck->iNumber), C4Value(), rVal1, rVal2, rVal3, rVal4}, passErrors, true).getInt();
			if (iResult == C4Fx_Effect_Deny)
				// effect denied
				return C4Fx_Effect_Deny;
			// add to other effect
			if (iResult == C4Fx_Effect_Annul || iResult == C4Fx_Effect_AnnulCalls)
			{
				pAddToEffect = pCheck;
				fDoTempCallsForAdd = (iResult == C4Fx_Effect_AnnulCalls);
			}
		}
	}
	// adding to other effect?
	if (pAddToEffect)
	{
		// do temp remove calls if desired
		if (pAddToEffect->pNext && fDoTempCallsForAdd)
			pAddToEffect->TempRemoveUpperEffects(pForObj, false, &pLastRemovedEffect);
		C4Value Par1 = C4VString(szCheckEffect), Par2 = C4VInt(iTimer);
		int32_t iResult = pAddToEffect->DoCall(pForObj, PSFS_FxAdd, Par1, Par2, rVal1, rVal2, rVal3, rVal4).getInt();
		// do temp readd calls if desired
		if (pAddToEffect->pNext && fDoTempCallsForAdd)
			pAddToEffect->TempReaddUpperEffects(pForObj, pLastRemovedEffect);
		// effect removed by this call?
		if (iResult == C4Fx_Start_Deny)
		{
			pAddToEffect->Kill(pForObj);
			return C4Fx_Effect_Annul;
		}
		else
			// other effect is the target effect number
			return pAddToEffect->iNumber;
	}
	// added to no effect and not denied
	return 0;
}

void C4Effect::Execute(C4Object *pObj)
{
	// get effect list
	C4Effect **ppEffectList = pObj ? &pObj->pEffects : &Game.pGlobalEffects;
	// execute all effects not marked as dead
	C4Effect *pEffect = this, **ppPrevEffect = ppEffectList;
	do
	{
		// effect dead?
		if (pEffect->IsDead())
		{
			// delete it, then
			C4Effect *pNextEffect = pEffect->pNext;
			pEffect->pNext = nullptr;
			delete pEffect;
			// next effect
			*ppPrevEffect = pEffect = pNextEffect;
		}
		else
		{
			// execute effect: time elapsed
			++pEffect->iTime;
			// check timer execution
			if (pEffect->iIntervall && !(pEffect->iTime % pEffect->iIntervall))
				if (pEffect->pFnTimer)
				{
					if (pEffect->pFnTimer->Exec(pEffect->pCommandTarget, {C4VObj(pObj), C4VInt(pEffect->iNumber), C4VInt(pEffect->iTime)}, false, true).getInt() == C4Fx_Execute_Kill)
					{
						// safety: this class got deleted!
						if (pObj && !pObj->Status) return;
						// timer function decided to finish it
						pEffect->Kill(pObj);
					}
					// safety: this class got deleted!
					if (pObj && !pObj->Status) return;
				}
				else
					// no timer function: mark dead after time elapsed
					pEffect->Kill(pObj);
			// next effect
			ppPrevEffect = &pEffect->pNext;
			pEffect = pEffect->pNext;
		}
	} while (pEffect);
}

void C4Effect::Kill(C4Object *pObj)
{
	const auto deletionTracker = TrackDeletion();
	// active?
	C4Effect *pLastRemovedEffect = nullptr;
	if (IsActive())
	{
		// then temp remove all higher priority effects
		TempRemoveUpperEffects(pObj, false, &pLastRemovedEffect);
	}
	else
	{
		// otherwise: temp reactivate before real removal
		// this happens only if a lower priority effect removes an upper priority effect in its add- or removal-call
		if (pFnStart && iPriority != 1)
		{
			pFnStart->Exec(pCommandTarget, {C4VObj(pObj), C4VInt(iNumber), C4VInt(C4FxCall_TempAddForRemoval)}, false, true);
			if (deletionTracker.IsDeleted())
			{
				return;
			}
		}
	}
	// remove this effect
	int32_t iPrevPrio = iPriority; SetDead();
	if (pFnStop)
	{
		if (pFnStop->Exec(pCommandTarget, {C4VObj(pObj), C4VInt(iNumber)}, false, true).getInt() == C4Fx_Stop_Deny)
		{
			// effect denied to be removed: recover
			iPriority = iPrevPrio;
		}

		if (deletionTracker.IsDeleted())
		{
			return;
		}
	}
	// reactivate other effects
	TempReaddUpperEffects(pObj, pLastRemovedEffect);
}

void C4Effect::ClearAll(C4Object *pObj, int32_t iClearFlag)
{
	// simply remove access all effects recursively, and do removal calls
	// this does not regard lower-level effects being added in the removal calls,
	// because this could hang the engine with poorly coded effects
	if (pNext) pNext->ClearAll(pObj, iClearFlag);
	if ((pObj && !pObj->Status) || IsDead()) return;
	int32_t iPrevPrio = iPriority;
	SetDead();
	if (pFnStop)
		if (pFnStop->Exec(pCommandTarget, {C4VObj(pObj), C4VInt(iNumber), C4VInt(iClearFlag)}, false, true).getInt() == C4Fx_Stop_Deny)
		{
			// this stop-callback might have deleted the object and then denied its own removal
			// must not modify self in this case...
			if (pObj && !pObj->Status) return;
			// effect denied to be removed: recover it
			iPriority = iPrevPrio;
		}
}

void C4Effect::DoDamage(C4Object *pObj, int32_t &riDamage, int32_t iDamageType, int32_t iCausePlr)
{
	// ask all effects for damage adjustments
	C4Effect *pEff = this;
	do
	{
		if (!pEff->IsDead() && pEff->pFnDamage)
			riDamage = pEff->pFnDamage->Exec(pEff->pCommandTarget, {C4VObj(pObj), C4VInt(pEff->iNumber), C4VInt(riDamage), C4VInt(iDamageType), C4VInt(iCausePlr)}, false, true).getInt();
		if (pObj && !pObj->Status) return;
	} while ((pEff = pEff->pNext) && riDamage);
}

C4Value C4Effect::DoCall(C4Object *pObj, const char *szFn, const C4Value &rVal1, const C4Value &rVal2, const C4Value &rVal3, const C4Value &rVal4, const C4Value &rVal5, const C4Value &rVal6, const C4Value &rVal7, bool passErrors, bool convertNilToIntBool)
{
	// def script or global only?
	C4AulScript *pSrcScript; C4Def *pDef;
	if (pCommandTarget)
	{
		pSrcScript = &pCommandTarget->Def->Script;
		// overwrite ID for sync safety in runtime join
		idCommandTarget = pCommandTarget->id;
	}
	else if (idCommandTarget && (pDef = Game.Defs.ID2Def(idCommandTarget)))
		pSrcScript = &pDef->Script;
	else
		pSrcScript = &Game.ScriptEngine;
	// call it
	C4AulFunc *pFn = pSrcScript->GetFuncRecursive(std::format(PSF_FxCustom, Name, szFn).c_str());
	if (!pFn) return C4Value();
	return pFn->Exec(pCommandTarget, {C4VObj(pObj), C4VInt(iNumber), rVal1, rVal2, rVal3, rVal4, rVal5, rVal6, rVal7}, passErrors, true, convertNilToIntBool);
}

void C4Effect::OnObjectChangedDef(C4Object *pObj)
{
	// safety
	if (!pObj) return;
	// check all effects for reassignment
	C4Effect *pCheck = this;
	while (pCheck)
	{
		if (pCheck->pCommandTarget == pObj)
			pCheck->ReAssignCallbackFunctions();
		pCheck = pCheck->pNext;
	}
}

void C4Effect::TempRemoveUpperEffects(C4Object *pObj, bool fTempRemoveThis, C4Effect **ppLastRemovedEffect)
{
	if (pObj && !pObj->Status) return; // this will be invalid!
	// priority=1: no callbacks
	if (iPriority == 1) return;
	// remove from high to low priority
	// recursive implementation...
	C4Effect *pEff = pNext;
	while (pEff) if (pEff->IsActive()) break; else pEff = pEff->pNext;
	// temp remove active effects with higher priority
	if (pEff) pEff->TempRemoveUpperEffects(pObj, true, ppLastRemovedEffect);
	// temp remove this
	if (fTempRemoveThis)
	{
		FlipActive();
		// temp callbacks only for higher priority effects
		if (pFnStop && iPriority != 1) pFnStop->Exec(pCommandTarget, {C4VObj(pObj), C4VInt(iNumber), C4VInt(C4FxCall_Temp), C4VBool(true)}, false, true);
		if (!*ppLastRemovedEffect) *ppLastRemovedEffect = this;
	}
}

void C4Effect::TempReaddUpperEffects(C4Object *pObj, C4Effect *pLastReaddEffect)
{
	// nothing to do? - this will also happen if TempRemoveUpperEffects did nothing due to priority==1
	if (!pLastReaddEffect) return;
	if (pObj && !pObj->Status) return; // this will be invalid!
	// simply activate all following, inactive effects
	for (C4Effect *pEff = pNext; pEff; pEff = pEff->pNext)
	{
		if (pEff->IsInactiveAndNotDead())
		{
			pEff->FlipActive();
			if (pEff->pFnStart && pEff->iPriority != 1) pEff->pFnStart->Exec(pEff->pCommandTarget, {C4VObj(pObj), C4VInt(pEff->iNumber), C4VInt(C4FxCall_Temp)}, false, true);
		}
		// done?
		if (pEff == pLastReaddEffect) break;
	}
}

void C4Effect::CompileFunc(StdCompiler *pComp)
{
	// read name
	pComp->Value(mkStringAdaptMI(Name));
	pComp->Separator(StdCompiler::SEP_START); // '('
	// read number
	pComp->Value(iNumber); pComp->Separator();
	// read priority
	pComp->Value(iPriority); pComp->Separator();
	// read time and intervall
	pComp->Value(iTime); pComp->Separator();
	pComp->Value(iIntervall); pComp->Separator();
	// read object number
	pComp->Value(pCommandTarget); pComp->Separator();
	// read ID
	pComp->Value(mkC4IDAdapt(idCommandTarget));
	pComp->Separator(StdCompiler::SEP_END); // ')'
	// read variables
	if (pComp->isCompiler() || EffectVars.GetSize() > 0)
		if (pComp->Separator(StdCompiler::SEP_START2)) // '['
		{
			pComp->Value(EffectVars);
			pComp->Separator(StdCompiler::SEP_END2); // ']'
		}
		else
			EffectVars.Reset();
	// is there a next effect?
	bool fNext = !!pNext;
	if (pComp->hasNaming())
	{
		if (fNext || pComp->isCompiler())
			fNext = pComp->Separator();
	}
	else
		pComp->Value(fNext);
	if (!fNext) return;
	// read next
	pComp->Value(mkPtrAdapt(pNext, false));
	// denumeration and callback assignment will be done later
}

// Internal fire effect

C4Value &FxFireVarMode           (C4Effect *pEffect) { return pEffect->EffectVars[0]; }
C4Value &FxFireVarCausedBy       (C4Effect *pEffect) { return pEffect->EffectVars[1]; }
C4Value &FxFireVarBlasted        (C4Effect *pEffect) { return pEffect->EffectVars[2]; }
C4Value &FxFireVarIncineratingObj(C4Effect *pEffect) { return pEffect->EffectVars[3]; }

int32_t FnFxFireStart(C4AulContext *ctx, C4Object *pObj, int32_t iNumber, int32_t iTemp, int32_t iCausedBy, bool fBlasted, C4Object *pIncineratingObject)
{
	// safety
	if (!pObj) return -1;
	// temp readd
	if (iTemp) { pObj->SetOnFire(true); return 1; }
	// fail if already on fire
	if (pObj->GetOnFire()) return -1;
	// get associated effect
	C4Effect *pEffect;
	if (!(pEffect = pObj->pEffects)) return -1;
	if (!(pEffect = pEffect->Get(iNumber, true))) return -1;
	// structures must eject contents now, because DoCon is not guaranteed to be executed!
	// In extinguishing material
	bool fFireCaused = true;
	int32_t iMat;
	if (MatValid(iMat = GBackMat(pObj->x, pObj->y)))
		if (Game.Material.Map[iMat].Extinguisher)
		{
			// blasts should changedef in water, too!
			if (fBlasted) if (pObj->Def->BurnTurnTo != C4ID_None) pObj->ChangeDef(pObj->Def->BurnTurnTo);
			// no fire caused
			fFireCaused = false;
		}
	// BurnTurnTo
	if (fFireCaused) if (pObj->Def->BurnTurnTo != C4ID_None) pObj->ChangeDef(pObj->Def->BurnTurnTo);
	// eject contents
	C4Object *cobj;
	if (!pObj->Def->IncompleteActivity && !pObj->Def->NoBurnDecay)
		while (cobj = pObj->Contents.GetObject())
		{
			cobj->Controller = iCausedBy; // update controller, so incinerating a hut full of flints attributes the damage to the incinerator
			if (pObj->Contained) cobj->Enter(pObj->Contained);
			else cobj->Exit(cobj->x, cobj->y);
		}
	// Detach attached objects
	cobj = nullptr;
	if (!pObj->Def->IncompleteActivity && !pObj->Def->NoBurnDecay)
		while (cobj = Game.FindObject(0, 0, 0, 0, 0, OCF_All, nullptr, pObj, nullptr, nullptr, ANY_OWNER, cobj))
			if ((cobj->Action.Act > ActIdle) && (cobj->Def->ActMap[cobj->Action.Act].Procedure == DFA_ATTACH))
				cobj->SetAction(ActIdle);
	// fire caused?
	if (!fFireCaused)
	{
		// if object was blasted but not incinerated (i.e., inside extinguisher)
		// do a script callback
		if (fBlasted) pObj->Call(PSF_IncinerationEx, {C4VInt(iCausedBy)});
		return -1;
	}
	// determine fire appearance
	int32_t iFireMode;
	if (!(iFireMode = pObj->Call(PSF_FireMode).getInt()))
	{
		// set default fire modes
		uint32_t dwCat = pObj->Category;
		if (dwCat & (C4D_Living | C4D_StaticBack)) // Tiere, Bäume
			iFireMode = C4Fx_FireMode_LivingVeg;
		else if (dwCat & (C4D_Structure | C4D_Vehicle)) // Gebäude und Fahrzeuge sind unten meist kantig
			iFireMode = C4Fx_FireMode_StructVeh;
		else
			iFireMode = C4Fx_FireMode_Object;
	}
	else if (!Inside<int32_t>(iFireMode, 1, C4Fx_FireMode_Last))
	{
		DebugLog(spdlog::level::warn, "FireMode {} of object {} ({}) is invalid!", iFireMode, pObj->GetName(), pObj->Def->GetName());
		iFireMode = C4Fx_FireMode_Object;
	}
	// store causes in effect vars
	FxFireVarMode(pEffect).SetInt(iFireMode);
	FxFireVarCausedBy(pEffect).SetInt(iCausedBy); // used in C4Object::GetFireCause and timer!
	FxFireVarBlasted(pEffect).SetBool(fBlasted);
	FxFireVarIncineratingObj(pEffect).SetObject(pIncineratingObject);
	// Set values
	pObj->SetOnFire(true);
	pObj->FirePhase = Random(MaxFirePhase);
	if (pObj->Shape.Wdt * pObj->Shape.Hgt > 500) StartSoundEffect("Inflame", false, 100, pObj);
	if (pObj->Def->Mass >= 100) StartSoundEffect("Fire", true, 100, pObj);
	// Engine script call
	pObj->Call(PSF_Incineration, {C4VInt(iCausedBy)});
	// Done, success
	return C4Fx_OK;
}

int32_t FnFxFireTimer(C4AulContext *ctx, C4Object *pObj, int32_t iNumber, int32_t iTime)
{
	// safety
	if (!pObj) return C4Fx_Execute_Kill;

	// get cause
	int32_t iCausedByPlr = NO_OWNER; C4Effect *pEffect;
	if (pEffect = pObj->pEffects)
		if (pEffect = pEffect->Get(iNumber, true))
		{
			iCausedByPlr = FxFireVarCausedBy(pEffect).getInt();
			if (!ValidPlr(iCausedByPlr)) iCausedByPlr = NO_OWNER;
		}

	// causes on object
	pObj->ExecFire(iNumber, iCausedByPlr);

	// special effects only if loaded
	if (!Game.Particles.IsFireParticleLoaded()) return C4Fx_OK;

	// get effect: May be nullptr after object fire execution, in which case the fire has been extinguished
	if (!pObj->GetOnFire()) return C4Fx_Execute_Kill;
	if (!(pEffect = pObj->pEffects)) return C4Fx_Execute_Kill;
	if (!(pEffect = pEffect->Get(iNumber, true))) return C4Fx_Execute_Kill;

	/* Fire execution behaviour transferred from script (FIRE) */

	// get fire mode
	int32_t iFireMode = FxFireVarMode(pEffect).getInt();

	// special effects only each four frames, except for objects (e.g.: Projectiles)
	if (iTime % 4 && iFireMode != C4Fx_FireMode_Object) return C4Fx_OK;

	// no gfx for contained
	if (pObj->Contained) return C4Fx_OK;

	// some constant effect parameters for this object
	int32_t iWidth = std::max<int32_t>(pObj->Def->Shape.Wdt, 1),
		iHeight = pObj->Def->Shape.Hgt,
		iYOff = iHeight / 2 - pObj->Def->Shape.FireTop;

	int32_t iCount = int32_t(sqrt(double(iWidth * iHeight)) / 4); // Number of particles per execution
	const int32_t iBaseParticleSize = 30; // With of particles in pixels/10, w/o add of values below
	const int32_t iParticleSizeDiff = 10; // Size variation among particles
	const int32_t iRelParticleSize = 12; // Influence of object size on particle size

	// some varying effect parameters
	int32_t iX = pObj->x, iY = pObj->y;
	int32_t iXDir, iYDir, iCon, iWdtCon, iA, iSize;

	// get remainign size (%)
	iCon = iWdtCon = std::max<int32_t>((100 * pObj->GetCon()) / FullCon, 1);
	if (!pObj->Def->GrowthType)
		// fixed width for not-stretched-objects
		if (iWdtCon < 100) iWdtCon = 100;

	// regard non-center object offsets
	iX += pObj->Shape.x + pObj->Shape.Wdt / 2;
	iY += pObj->Shape.y + pObj->Shape.Hgt / 2;

	// apply rotation
	float fRot[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
	if (pObj->r && pObj->Def->Rotateable)
	{
		fRot[0] =  cosf(static_cast<float>(pObj->r * std::numbers::pi_v<float> / 180.0));
		fRot[1] = -sinf(static_cast<float>(pObj->r * std::numbers::pi_v<float> / 180.0));
		fRot[2] = -fRot[1];
		fRot[3] = fRot[0];
		// rotated objects usually better burn from the center
		if (iYOff > 0) iYOff = 0;
	}

	// Adjust particle number by con
	iCount = (std::max)(2, iCount * iWdtCon / 100);

	// calc base for particle size parameter
	iA = static_cast<int32_t>(sqrt(sqrt(double(iWidth * iHeight)) * (iCon + 20) / 120) * iRelParticleSize);

	// create a double set of particles; first quarter normal (Fire); remaining three quarters additive (Fire2)
	for (int32_t i = 0; i < iCount * 2; ++i)
	{
		// calc actual size to be used in this frame
		// Using Random instead of SafeRandom would be safe here
		// However, since it's just affecting particles there's no need to use synchronized random values
		iSize = SafeRandom(iParticleSizeDiff + 1) + iBaseParticleSize - iParticleSizeDiff / 2 - 1 + iA;

		// get particle target list
		C4ParticleList *pParticleList = SafeRandom(4) ? &(pObj->BackParticles) : &(pObj->FrontParticles);

		// get particle def and color
		C4ParticleDef *pPartDef; uint32_t dwClr;
		if (i < iCount / 2)
		{
			dwClr = 0x32004000 + ((SafeRandom(59) + 196) << 16);
			pPartDef = Game.Particles.pFire1;
		}
		else
		{
			dwClr = 0xffffff;
			pPartDef = Game.Particles.pFire2;
		}
		if (iFireMode == C4Fx_FireMode_Object) dwClr += 0x62000000;

		// get particle creation pos...
		int32_t iRandX = SafeRandom(iWidth + 1) - iWidth / 2 - 1;

		int32_t iPx = iRandX * iWdtCon / 100;
		int32_t iPy = iYOff * iCon / 100;
		if (iFireMode == C4Fx_FireMode_LivingVeg) iPy -= iPx * iPx * 100 / iWidth / iWdtCon; // parable form particle pos on livings

		// ...and movement speed
		if (iFireMode != C4Fx_FireMode_Object)
		{
			// ...for normal fire proc
			iXDir = iRandX * iCon / 400 - (iPx / 3) - int32_t(fixtof(pObj->xdir) * 3);
			iYDir = -SafeRandom(15 + iHeight * iCon / 300) - 1 - int32_t(fixtof(pObj->ydir) * 3);
		}
		else
		{
			// ...for objects
			iXDir = -int32_t(fixtof(pObj->xdir) * 3);
			iYDir = -int32_t(fixtof(pObj->ydir) * 3);
			if (!iYDir) iYDir = -SafeRandom(13 + iHeight / 4) - 1;
		}

		// OK; create it!
		Game.Particles.Create(pPartDef, float(iX) + fRot[0] * iPx + fRot[1] * iPy, float(iY) + fRot[2] * iPx + fRot[3] * iPy, iXDir / 10.0f, iYDir / 10.0f, iSize / 10.0f, dwClr, pParticleList, pObj);
	}

	return C4Fx_OK;
}

int32_t FnFxFireStop(C4AulContext *ctx, C4Object *pObj, int32_t iNumber, int32_t iReason, bool fTemp)
{
	// safety
	if (!pObj) return false;
	// only if real removal is done
	if (fTemp)
	{
		// but fake being not on fire, so higher-priority effects get the status right
		pObj->SetOnFire(false);
		return true;
	}
	// alter OnFire-flag
	pObj->SetOnFire(false);
	// stop sound
	if (pObj->Def->Mass >= 100) StopSoundEffect("Fire", pObj);
	// done, success
	return true;
}

C4String *FnFxFireInfo(C4AulContext *ctx, C4Object *pObj, int32_t iNumber)
{
	return new C4String(LoadResStr(C4ResStrTableKey::IDS_OBJ_BURNS), &Game.ScriptEngine.Strings);
}

// Some other, internal effects

void Splash(int32_t tx, int32_t ty, int32_t amt, C4Object *pByObj)
{
	// Splash only if there is free space above
	if (GBackSemiSolid(tx, ty - 15)) return;
	// get back mat
	int32_t iMat = GBackMat(tx, ty);
	// check liquid
	if (MatValid(iMat))
		if (DensityLiquid(Game.Material.Map[iMat].Density) && Game.Material.Map[iMat].Instable)
		{
			int32_t sy = ty;
			while (GBackLiquid(tx, sy) && sy > ty - 20 && sy >= 0) sy--;
			// Splash bubbles and liquid
			for (int32_t cnt = 0; cnt < amt; cnt++)
			{
				// force argument evaluation order
				const auto r2 = Random(16);
				const auto r1 = Random(16);
				BubbleOut(tx + r1 - 8, ty + r2 - 6);
				if (GBackLiquid(tx, ty) && !GBackSemiSolid(tx, sy))
				{
					// force argument evaluation order
					const auto r2 = FIXED100(-Random(200));
					const auto r1 = FIXED100(Random(151) - 75);
					Game.PXS.Create(Game.Landscape.ExtractMaterial(tx, ty),
						itofix(tx), itofix(sy),
						r1,
						r2);
				}
			}
		}
	// Splash sound
	if (amt >= 20)
		StartSoundEffect("Splash2", false, 100, pByObj);
	else if (amt > 1) StartSoundEffect("Splash1", false, 100, pByObj);
}

int32_t GetSmokeLevel()
{
	// Network active: enforce fixed smoke level
	if (Game.Control.SyncMode())
		return 150;
	// User-defined smoke level
	return Config.Graphics.SmokeLevel;
}

void BubbleOut(int32_t tx, int32_t ty)
{
	// No bubbles from nowhere
	if (!GBackSemiSolid(tx, ty)) return;
	// User-defined smoke level
	int32_t SmokeLevel = GetSmokeLevel();
	// Enough bubbles out there already
	if (Game.Objects.ObjectCount(C4Id("FXU1")) >= SmokeLevel) return;
	// Create bubble
	Game.CreateObject(C4Id("FXU1"), nullptr, NO_OWNER, tx, ty);
}

void Smoke(int32_t tx, int32_t ty, int32_t level, uint32_t dwClr)
{
	if (Game.Particles.pSmoke)
	{
		Game.Particles.Create(Game.Particles.pSmoke, float(tx), float(ty) - level / 2, 0.0f, 0.0f, float(level), dwClr);
		return;
	}
	// User-defined smoke level
	int32_t SmokeLevel = GetSmokeLevel();
	// Enough smoke out there already
	if (Game.Objects.ObjectCount(C4Id("FXS1")) >= SmokeLevel) return;
	// Create smoke
	level = BoundBy<int32_t>(level, 3, 32);
	C4Object *pObj;
	if (pObj = Game.CreateObjectConstruction(C4Id("FXS1"), nullptr, NO_OWNER, tx, ty, FullCon * level / 32))
		pObj->Call(PSF_Activate);
}

void Explosion(int32_t tx, int32_t ty, int32_t level, C4Object *inobj, int32_t iCausedBy, C4Object *pByObj, C4ID idEffect, const char *szEffect)
{
	int32_t grade = BoundBy((level / 10) - 1, 1, 3);
	// Sound
	StartSoundEffect(std::format("Blast{}", '0' + grade).c_str(), false, 100, pByObj);
	// Check blast containment
	C4Object *container = inobj;
	while (container && !container->Def->ContainBlast) container = container->Contained;
	// Uncontained blast effects
	if (!container)
	{
		// Incinerate landscape
		if (!Game.Landscape.Incinerate(tx, ty))
			if (!Game.Landscape.Incinerate(tx, ty - 10))
				if (!Game.Landscape.Incinerate(tx - 5, ty - 5))
					Game.Landscape.Incinerate(tx + 5, ty - 5);
		// Create blast object or particle
		C4Object *pBlast;
		C4ParticleDef *pPrtDef = Game.Particles.pBlast;
		// particle override
		if (szEffect)
		{
			C4ParticleDef *pPrtDef2 = Game.Particles.GetDef(szEffect);
			if (pPrtDef2) pPrtDef = pPrtDef2;
		}
		else if (idEffect) pPrtDef = nullptr;
		// create particle
		if (pPrtDef)
		{
			Game.Particles.Create(pPrtDef, static_cast<float>(tx), static_cast<float>(ty), 0.0f, 0.0f, static_cast<float>(level), 0);
			if (SEqual2(pPrtDef->Name.getData(), "Blast"))
				Game.Particles.Cast(Game.Particles.pFSpark, level / 5 + 1, static_cast<float>(tx), static_cast<float>(ty), level, level / 2 + 1.0f, 0x00ef0000, level + 1.0f, 0xffff1010);
		}
		else if (pBlast = Game.CreateObjectConstruction(idEffect ? idEffect : C4Id("FXB1"), pByObj, iCausedBy, tx, ty + level, FullCon * level / 20))
			pBlast->Call(PSF_Activate);
	}
	// Blast objects
	Game.BlastObjects(tx, ty, level, inobj, iCausedBy, pByObj);
	if (container != inobj) Game.BlastObjects(tx, ty, level, container, iCausedBy, pByObj);
	if (!container)
	{
		// Blast free landscape. After blasting objects so newly mined materials don't get flinged
		Game.Landscape.BlastFree(tx, ty, level, grade, iCausedBy);
	}
}
