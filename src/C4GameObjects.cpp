/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
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

// game object lists

#include <C4Include.h>
#include <C4GameObjects.h>

#include <C4Object.h>
#include <C4ObjectCom.h>
#include <C4Random.h>
#include <C4SolidMask.h>
#include <C4Network2Stats.h>
#include <C4Game.h>
#include <C4Wrappers.h>

#include "StdHelpers.h"

#include <iterator>

C4GameObjects::C4GameObjects()
{
	Default();
}

C4GameObjects::~C4GameObjects()
{
	Sectors.Clear();
}

void C4GameObjects::Default()
{
	ResortProc = nullptr;
	Sectors.Clear();
	LastUsedMarker = 0;
}

void C4GameObjects::Init(int32_t iWidth, int32_t iHeight)
{
	// init sectors
	Sectors.Init(iWidth, iHeight);
}

bool C4GameObjects::Add(C4Object *nObj)
{
	// add inactive objects to the inactive list only
	if (nObj->Status == C4OS_INACTIVE)
		return InactiveObjects.Add(nObj, C4ObjectList::stMain);
	// if this is a background object, add it to the list
	if (nObj->Category & C4D_Background)
		Game.BackObjects.Add(nObj, C4ObjectList::stMain);
	// if this is a foreground object, add it to the list
	if (nObj->Category & C4D_Foreground)
		Game.ForeObjects.Add(nObj, C4ObjectList::stMain);
	// manipulate main list
	if (!C4ObjectList::Add(nObj, C4ObjectList::stMain))
		return false;
	// add to sectors
	Sectors.Add(nObj, this);
	return true;
}

bool C4GameObjects::Remove(C4Object *pObj)
{
	// if it's an inactive object, simply remove from the inactiv elist
	if (pObj->Status == C4OS_INACTIVE) return InactiveObjects.Remove(pObj);
	// remove from sectors
	Sectors.Remove(pObj);
	// remove from backlist
	Game.BackObjects.Remove(pObj);
	// remove from forelist
	Game.ForeObjects.Remove(pObj);
	// manipulate main list
	return C4ObjectList::Remove(pObj);
}

C4ObjectList &C4GameObjects::ObjectsAt(int ix, int iy)
{
	return Sectors.SectorAt(ix, iy)->ObjectShapes;
}

void C4GameObjects::CrossCheck() // Every Tick1 by ExecObjects
{
	C4Object *obj1, *obj2;
	uint32_t ocf1, ocf2, focf, tocf;

	// AtObject-Check: Checks for first match of obj1 at obj2

	// Checks for this frame
	focf = tocf = OCF_None;
	// Medium level: Fight
	if (!Tick5)
	{
		focf |= OCF_FightReady; tocf |= OCF_FightReady;
	}
	// Very low level: Incineration
	if (!Tick35)
	{
		focf |= OCF_OnFire; tocf |= OCF_Inflammable;
	}

	if (focf && tocf)
		for (auto iter = safeBegin(); iter != safeEnd() && (obj1 = *iter); ++iter)
			if (obj1->Status && !obj1->Contained)
				if (obj1->OCF & focf)
				{
					ocf1 = obj1->OCF; ocf2 = tocf;
					if (obj2 = AtObject(obj1->x, obj1->y, ocf2, obj1))
					{
						// Incineration
						if ((ocf1 & OCF_OnFire) && (ocf2 & OCF_Inflammable))
							if (!Random(obj2->Def->ContactIncinerate))
							{
								obj2->Incinerate(obj1->GetFireCausePlr(), false, obj1); continue;
							}
						// Fight
						if ((ocf1 & OCF_FightReady) && (ocf2 & OCF_FightReady))
							if (Game.Players.Hostile(obj1->Owner, obj2->Owner))
							{
								// RejectFight callback
								if (obj1->Call(PSF_RejectFight, {C4VObj(obj2)}).getBool()) continue;
								if (obj2->Call(PSF_RejectFight, {C4VObj(obj1)}).getBool()) continue;
								ObjectActionFight(obj1, obj2);
								ObjectActionFight(obj2, obj1);
								continue;
							}
					}
				}

	// Reverse area check: Checks for all obj2 at obj1

	focf = tocf = OCF_None;
	// High level: Collection, Hit
	if (!Tick3)
	{
		focf |= OCF_Collection; tocf |= OCF_Carryable;
	}
	focf |= OCF_Alive; tocf |= OCF_HitSpeed2;

	if (focf && tocf)
		for (auto iter = safeBegin(); iter != safeEnd() && (obj1 = *iter); ++iter)
			if (obj1->Status && !obj1->Contained && (obj1->OCF & focf))
			{
				uint32_t Marker = GetNextMarker();
				C4LSector *pSct;
				for (C4ObjectList *pLst = obj1->Area.FirstObjects(&pSct); pLst; pLst = obj1->Area.NextObjects(pLst, &pSct))
					for (auto iter2 = pLst->safeBegin(); iter2 != pLst->safeEnd() && (obj2 = *iter2); ++iter2)
						if (obj2->Status && !obj2->Contained && (obj2 != obj1) && (obj2->OCF & tocf))
							if (Inside<int32_t>(obj2->x - (obj1->x + obj1->Shape.x), 0, obj1->Shape.Wdt - 1))
								if (Inside<int32_t>(obj2->y - (obj1->y + obj1->Shape.y), 0, obj1->Shape.Hgt - 1))
									if (obj1->pLayer == obj2->pLayer)
									{
										// handle collision only once
										if (obj2->Marker == Marker) continue;
										obj2->Marker = Marker;
										// Hit
										if ((obj2->OCF & OCF_HitSpeed2) && (obj1->OCF & OCF_Alive) && (obj2->Category & C4D_Object))
											if (!obj1->Call(PSF_QueryCatchBlow, {C4VObj(obj2)}))
											{
												// "realistic" hit energy
												FIXED dXDir = obj2->xdir - obj1->xdir, dYDir = obj2->ydir - obj1->ydir;
												int32_t iHitEnergy = fixtoi((dXDir * dXDir + dYDir * dYDir) * obj2->Mass / 5);
												iHitEnergy = std::max<int32_t>(iHitEnergy / 3, !!iHitEnergy); // hit energy reduced to 1/3rd, but do not drop to zero because of this division
												obj1->DoEnergy(-iHitEnergy / 5, false, C4FxCall_EngObjHit, obj2->Controller);
												int tmass = std::max<int32_t>(obj1->Mass, 50);
												if (!Tick3 || (obj1->Action.Act >= 0 && obj1->Def->ActMap[obj1->Action.Act].Procedure != DFA_FLIGHT))
													obj1->Fling(obj2->xdir * 50 / tmass, -Abs(obj2->ydir / 2) * 50 / tmass, false, obj2->Controller);
												obj1->Call(PSF_CatchBlow, {C4VInt(-iHitEnergy / 5),
													C4VObj(obj2)});
												// obj1 might have been tampered with
												if (!obj1->Status || obj1->Contained || !(obj1->OCF & focf))
													goto out1;
												continue;
											}
										// Collection
										if ((obj1->OCF & OCF_Collection) && (obj2->OCF & OCF_Carryable))
											if (Inside<int32_t>(obj2->x - (obj1->x + obj1->Def->Collection.x), 0, obj1->Def->Collection.Wdt - 1))
												if (Inside<int32_t>(obj2->y - (obj1->y + obj1->Def->Collection.y), 0, obj1->Def->Collection.Hgt - 1))
												{
													obj1->Collect(obj2);
													// obj1 might have been tampered with
													if (!obj1->Status || obj1->Contained || !(obj1->OCF & focf))
														goto out1;
												}
									}
			out1:;
			}

	// Contained-Check: Checks for matching Contained

	// Checks for this frame
	focf = tocf = OCF_None;
	// Low level: Fight
	if (!Tick10)
	{
		focf |= OCF_FightReady; tocf |= OCF_FightReady;
	}

	if (focf && tocf)
		for (auto iter = safeBegin(); iter != safeEnd() && (obj1 = *iter); ++iter)
			if (obj1->Status && obj1->Contained && (obj1->OCF & focf))
			{
				for (auto iter2 = obj1->Contained->Contents.safeBegin(); iter2 != safeEnd() && (obj2 = *iter2); ++iter2)
					if (obj2->Status && obj2->Contained && (obj2 != obj1) && (obj2->OCF & tocf))
						if (obj1->pLayer == obj2->pLayer)
						{
							ocf1 = obj1->OCF; ocf2 = obj2->OCF;
							// Fight
							if ((ocf1 & OCF_FightReady) && (ocf2 & OCF_FightReady))
								if (Game.Players.Hostile(obj1->Owner, obj2->Owner))
								{
									ObjectActionFight(obj1, obj2);
									ObjectActionFight(obj2, obj1);
									// obj1 might have been tampered with
									if (!obj1->Status || obj1->Contained || !(obj1->OCF & focf))
										goto out2;
									continue;
								}
						}
			out2:;
			}
}

C4Object *C4GameObjects::AtObject(int ctx, int cty, uint32_t &ocf, C4Object *exclude)
{
	uint32_t cocf;
	for (const auto cObj : ObjectsAt(ctx, cty))
	{
		if (!exclude || (cObj != exclude && exclude->pLayer == cObj->pLayer)) if (cObj->Status)
		{
			cocf = ocf | OCF_Exclusive;
			if (cObj->At(ctx, cty, cocf))
			{
				// Search match
				if (cocf & ocf) { ocf = cocf; return cObj; }
				// EXCLUSIVE block
				else return nullptr;
			}
		}
	}
	return nullptr;
}

void C4GameObjects::Synchronize()
{
	// synchronize unsorted objects
	ResortUnsorted();
	ExecuteResorts();
	// synchronize solidmasks
	RemoveSolidMasks();
	PutSolidMasks();
}

C4Object *C4GameObjects::FindInternal(C4ID id)
{
	// search list of system objects (searches global list)
	return ObjectsInt().Find(id);
}

C4Object *C4GameObjects::ObjectPointer(int32_t iNumber)
{
	// search own list
	C4Object *pObj = C4ObjectList::ObjectPointer(iNumber);
	if (pObj) return pObj;
	// search deactivated
	return InactiveObjects.ObjectPointer(iNumber);
}

long C4GameObjects::ObjectNumber(C4Object *pObj)
{
	// search own list
	long iNum = C4ObjectList::ObjectNumber(pObj);
	if (iNum) return iNum;
	// search deactivated
	return InactiveObjects.ObjectNumber(pObj);
}

C4ObjectList &C4GameObjects::ObjectsInt()
{
	return *this;
}

void C4GameObjects::RemoveSolidMasks()
{
	for (const auto obj : *this)
	{
		if (obj->Status && obj->pSolidMaskData)
		{
				obj->pSolidMaskData->Remove(false, false);
		}
	}
}

void C4GameObjects::PutSolidMasks()
{
	for (const auto obj : *this)
	{
		if (obj->Status)
		{
			obj->UpdateSolidMask(false);
		}
	}
}

void C4GameObjects::Clear(bool fClearInactive)
{
	DeleteObjects();
	if (fClearInactive)
		InactiveObjects.Clear();
	ResortProc = nullptr;
	LastUsedMarker = 0;
}

/* C4ObjResort */

C4ObjResort::C4ObjResort()
{
	Category = 0;
	OrderFunc = nullptr;
	Next = nullptr;
	pSortObj = pObjBefore = nullptr;
	fSortAfter = false;
}

C4ObjResort::~C4ObjResort() {}

void C4ObjResort::Execute()
{
	// no order func: resort given objects
	if (!OrderFunc)
	{
		// no objects given?
		if (!pSortObj || !pObjBefore) return;
		// object to be resorted died or changed category
		if (pSortObj->Status != C4OS_NORMAL || pSortObj->Unsorted) return;
		// exchange
		if (fSortAfter)
			Game.Objects.OrderObjectAfter(pSortObj, pObjBefore);
		else
			Game.Objects.OrderObjectBefore(pSortObj, pObjBefore);
		// done
		return;
	}
	else if (pSortObj)
	{
		// sort single object
		SortObject();
		return;
	}
	// get first link to start sorting
	auto it = Game.Objects.rbegin();
	const auto end = Game.Objects.rend();
	if (it == end) return;
	// sort all categories given; one by one (sort by category is ensured by C4ObjectList::Add)
	for (int iCat = 1; iCat < C4D_SortLimit; iCat <<= 1)
	{
		if (iCat & Category)
		{
			// get first link of this category
			while (!((*it)->Status && ((*it)->Category & iCat)))
			{
				if (++it == end)
				{
					// no more objects to sort: done
					break;
				}
			}
			// first link found?
			if (it != end)
			{
				// get last link of this category
				auto nextIt = it;
				while (!(*nextIt)->Status || ((*nextIt)->Category & iCat))
				{
					if (++nextIt == end)
						// no more objects: end of list reached
						break;
				}
				// get previous link, which is the last in the list of this category
				auto lastIt = nextIt;
				--lastIt;
				// get next valid (there must be at least one: pLnk; so this loop should be safe)
				while (!(*lastIt)->Status) --lastIt;
				// now sort this portion of the list
				Sort(std::prev(lastIt.base()), std::prev(it.base()));
				// start searching at end of this list next time
				// if the end has already been reached: stop here
				it = nextIt;
				if (it == end) return;
			}
			// continue with next category
		}
	}
}

void C4ObjResort::SortObject()
{
	// safety
	if (pSortObj->Status != C4OS_NORMAL || pSortObj->Unsorted) return;
	// pre-build parameters
	C4AulParSet Pars;
	Pars[1].Set(C4VObj(pSortObj));
	// first, check forward in list
	auto pLnk = Game.Objects.GetLink(pSortObj);
	const auto endLnk = Game.Objects.cend();
	if (pLnk == endLnk) return;
	auto pLnkBck = pLnk;
	auto pMoveLink = endLnk;
	int iResult;
	while (++pLnk != endLnk)
	{
		// get object
		const auto pObj2 = *pLnk;
		if (!pObj2->Status) continue;
		// does the category still match?
		if (!(pObj2->Category & pSortObj->Category)) break;
		// perform the check
		Pars[0].Set(C4VObj(pObj2));
		iResult = OrderFunc->Exec(nullptr, Pars).getInt();
		if (iResult > 0) break;
		if (iResult < 0) pMoveLink = pLnk;
	}
	// check if movement has to be done
	if (pMoveLink != endLnk)
	{
		// move link directly after pMoveLink
		// FIXME: Inform C4ObjectList that this is a reorder, not a remove+insert
		// move out of current position
		Game.Objects.RemoveLink(pLnkBck);
		// put into new position
		Game.Objects.InsertLink(pLnkBck, pMoveLink);
	}
	else
	{
		const auto begin = Game.Objects.cbegin();
		auto pMoveLink = pLnk;
		if (pLnkBck != begin)
		{
			// no movement yet: check backwards in list
			Pars[0].Set(C4VObj(pSortObj));
			auto pLnk = pLnkBck;
			for (--pLnk; ; --pLnk)
			{
				// get object
				const auto pObj2 = *pLnk;
				if (pObj2->Status)
				{
					// does the category still match?
					if (!(pObj2->Category & pSortObj->Category)) break;
					// perform the check
					Pars[1].Set(C4VObj(pObj2));
					iResult = OrderFunc->Exec(nullptr, Pars).getInt();
					if (iResult > 0) break;
					if (iResult < 0) pMoveLink = pLnk;
				}
				if (pLnk == begin)
				{
					break;
				}
			}
		}
		// no movement to be done? finish
		if (pMoveLink == endLnk) return;
		// move link directly before pMoveLink
		Game.Objects.MoveLinkBefore(pLnkBck, pMoveLink);
	}
	// object has been resorted: resort into area lists, too
	Game.Objects.UpdatePosResort(pSortObj);
	// done
}

void C4ObjResort::Sort(C4ObjectList::mutable_iterator pFirst, const C4ObjectList::mutable_iterator &pLast)
{
#ifdef _DEBUG
	assert(Game.Objects.Sectors.CheckSort());
#endif
	// do a simple insertion-like sort
	const auto pFirstBck = pFirst; // backup of first link

	// pre-build parameters
	C4AulParSet Pars;

	// loop until there's nothing left to sort
	while (pFirst != pLast)
	{
		// start from the very end of the list
		auto pCurr = pLast, pNewFirst = pLast;
		// loop the checks up to the first list item to check
		while (pCurr != pFirst)
		{
			const auto pCurrBck = pCurr;
			// get second check item
			--pCurr;
			while (!(*pCurr)->Status) --pCurr;
			// perform the check
			Pars[0].Set(C4VObj(*pCurrBck)); Pars[1].Set(C4VObj(*pCurr));
			if (OrderFunc->Exec(nullptr, Pars).getInt() < 0)
			{
				// so there's something to be reordered: swap the links
				// FIXME: Inform C4ObjectList about this reorder
				std::swap(*pCurrBck, *pCurr);
				// and readd to sector lists
				(*pCurrBck)->Unsorted = (*pCurr)->Unsorted = true;
				// grow list section to scan next
				pNewFirst = pCurrBck;
			}
		}
		// reduce area to be checked
		pFirst = pNewFirst;
	}
#ifdef _DEBUG
	assert(Game.Objects.Sectors.CheckSort());
#endif
	// resort objects in sector lists
	for (auto pCurr = pFirstBck; pCurr != std::next(pLast); ++pCurr)
	{
		C4Object *pObj = *pCurr;
		if (pObj->Status && pObj->Unsorted)
		{
			pObj->Unsorted = false;
			Game.Objects.UpdatePosResort(pObj);
		}
	}
#ifdef _DEBUG
	assert(Game.Objects.Sectors.CheckSort());
#endif
}

int C4GameObjects::Load(C4Group &hGroup, bool fKeepInactive)
{
	// Load data component
	StdStrBuf Source;
	if (!hGroup.LoadEntryString(C4CFN_ScenarioObjects, Source))
		return 0;

	// Compile
	StdStrBuf Name = hGroup.GetFullName() + DirSep C4CFN_ScenarioObjects;
	if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(
		mkParAdapt(*this, false),
		Source,
		Name.getData()))
		return 0;

	// Process objects
	bool fObjectNumberCollision = false;
	int32_t iMaxObjectNumber = 0;
	for (const auto pObj : StdReversed{*this})
	{
		// check object number collision with inactive list
		if (fKeepInactive)
		{
			for (const auto obj : InactiveObjects)
			{
				if (obj->Number == pObj->Number) fObjectNumberCollision = true;
				break;
			}
		}
		// keep track of numbers
		iMaxObjectNumber = std::max<long>(iMaxObjectNumber, pObj->Number);
		// add to list of backobjects
		if (pObj->Category & C4D_Background)
			Game.BackObjects.Add(pObj, C4ObjectList::stMain, this);
		// add to list of foreobjects
		if (pObj->Category & C4D_Foreground)
			Game.ForeObjects.Add(pObj, C4ObjectList::stMain, this);
		// Unterminate end
	}

	// Denumerate pointers
	// if object numbers collideded, numbers will be adjusted afterwards
	// so fake inactive object list empty meanwhile
	C4ObjectList inactiveBackup;
	if (fObjectNumberCollision) { inactiveBackup.Copy(InactiveObjects); InactiveObjects.Clear(); }
	// denumerate pointers
	Denumerate();
	// update object enumeration index now, because calls like UpdateTransferZone might create objects
	Game.ObjectEnumerationIndex = (std::max)(Game.ObjectEnumerationIndex, iMaxObjectNumber);
	// end faking and adjust object numbers
	if (fObjectNumberCollision)
	{
		InactiveObjects.Copy(inactiveBackup);
		// simply renumber all inactive objects
		for (const auto obj : InactiveObjects)
		{
			if (obj->Status)
			{
				obj->Number = ++Game.ObjectEnumerationIndex;
			}
		}
	}

	// special checks:
	// -contained/contents-consistency
	// -StaticBack-objects zero speed
	for (const auto pObj : *this)
	{
		if (pObj->Status)
		{
			// staticback must not have speed
			if (pObj->Category & C4D_StaticBack)
			{
				pObj->xdir = pObj->ydir = 0;
			}
			// contained must be in contents list
			if (pObj->Contained)
				if (!pObj->Contained->Contents.IsContained(pObj))
				{
					DebugLogF("Error in Objects.txt: Container of #%d is #%d, but not found in contents list!", pObj->Number, pObj->Contained->Number);
					pObj->Contained->Contents.Add(pObj, C4ObjectList::stContents);
				}
			// all contents must have contained set; otherwise, remove them!
			C4Object *pObj2;
			for (auto it = pObj->Contents.begin(); it != pObj->Contents.end(); ++it)
			{
				// check double links
				if (pObj->Contents.GetLink(*it) != it)
				{
					DebugLogF("Error in Objects.txt: Double containment of #%d by #%d!", (*it)->Number, pObj->Number);
					// this remove-call will only remove the previous (dobuled) link, so cLnkCont should be save
					pObj->Contents.Remove(*it);
					// contents checked already
					continue;
				}
				// check contents/contained-relation
				if (const auto pObj2 = *it; pObj2->Status)
					if (pObj2->Contained != pObj)
					{
						DebugLogF("Error in Objects.txt: Object #%d not in container #%d as referenced!", pObj2->Number, pObj->Number);
						pObj2->Contained = pObj;
					}
			}
		}
	}
	// sort out inactive objects
	for (auto it = begin(); it != end(); )
	{
		if ((*it)->Status == C4OS_INACTIVE)
		{
			const auto obj = *it;
			it = Objects.erase(it);
			Mass -= obj->Mass;

			InactiveObjects.Add(obj, C4ObjectList::stNone);
		}
		else
		{
			++it;
		}
	}

	{
		C4DebugRecOff DBGRECOFF; // - script callbacks that would kill DebugRec-sync for runtime start
		// update graphics
		UpdateGraphics(false);
		// Update faces
		UpdateFaces(false);
		// Update ocf
		SetOCF();
	}

	// make sure list is sorted by category - after sorting out inactives, because inactives aren't sorted into the main list
	FixObjectOrder();

	// misc updates
	for (const auto obj : *this)
		if (obj->Status)
		{
			// add to plrview
			obj->PlrFoWActualize();
			// update flipdir (for old objects.txt with no flipdir defined)
			// assigns Action.DrawDir as well
			obj->UpdateFlipDir();
		}
	// Done
	return ObjectCount();
}

bool C4GameObjects::Save(C4Group &hGroup, bool fSaveGame, bool fSaveInactive)
{
	// Save to temp file
	char szFilename[_MAX_PATH + 1]; SCopy(Config.AtTempPath(C4CFN_ScenarioObjects), szFilename);
	if (!Save(szFilename, fSaveGame, fSaveInactive)) return false;

	// Move temp file to group
	hGroup.Move(szFilename, nullptr); // check?
	// Success
	return true;
}

bool C4GameObjects::Save(const char *szFilename, bool fSaveGame, bool fSaveInactive)
{
	// Enumerate
	Enumerate();
	InactiveObjects.Enumerate();
	Game.ScriptEngine.Strings.EnumStrings();

	// Decompile objects to buffer
	StdStrBuf Buffer;
	bool fSuccess = DecompileToBuf_Log<StdCompilerINIWrite>(mkParAdapt(*this, false, !fSaveGame), &Buffer, szFilename);

	// Decompile inactives
	if (fSaveInactive)
	{
		StdStrBuf InactiveBuffer;
		fSuccess &= DecompileToBuf_Log<StdCompilerINIWrite>(mkParAdapt(InactiveObjects, false, !fSaveGame), &InactiveBuffer, szFilename);
		Buffer.Append("\r\n");
		Buffer.Append(InactiveBuffer);
	}

	// Denumerate
	InactiveObjects.Denumerate();
	Denumerate();

	// Error?
	if (!fSuccess)
		return false;

	// Write
	return Buffer.SaveToFile(szFilename);
}

void C4GameObjects::UpdateScriptPointers()
{
	// call in sublists
	C4ObjectList::UpdateScriptPointers();
	InactiveObjects.UpdateScriptPointers();
	// adjust global effects
	if (Game.pGlobalEffects) Game.pGlobalEffects->ReAssignAllCallbackFunctions();
}

void C4GameObjects::UpdatePos(C4Object *pObj)
{
	// Position might have changed. Update sector lists
	Sectors.Update(pObj, this);
}

void C4GameObjects::UpdatePosResort(C4Object *pObj)
{
	// Object order for this object was changed. Readd object to sectors
	Sectors.Remove(pObj);
	Sectors.Add(pObj, this);
}

bool C4GameObjects::OrderObjectBefore(C4Object *pObj1, C4Object *pObj2)
{
	// check that this won't screw the category sort
	if ((pObj1->Category & C4D_SortLimit) < (pObj2->Category & C4D_SortLimit))
		return false;
	// reorder
	if (!C4ObjectList::OrderObjectBefore(pObj1, pObj2))
		return false;
	// update area lists
	UpdatePosResort(pObj1);
	// done, success
	return true;
}

bool C4GameObjects::OrderObjectAfter(C4Object *pObj1, C4Object *pObj2)
{
	// check that this won't screw the category sort
	if ((pObj1->Category & C4D_SortLimit) > (pObj2->Category & C4D_SortLimit))
		return false;
	// reorder
	if (!C4ObjectList::OrderObjectAfter(pObj1, pObj2))
		return false;
	// update area lists
	UpdatePosResort(pObj1);
	// done, success
	return true;
}

void C4GameObjects::FixObjectOrder()
{
	// fixes the object order so it matches the global object order sorting constraints
	const auto begin = Objects.begin();
	const auto end = Objects.end();
	auto pLnk0 = begin, pLnkL = end;

	// empty?
	if (begin == end)
	{
		return;
	}
	--pLnkL;
	while (pLnk0 != pLnkL)
	{
		auto pLnk1stUnsorted = end, pLnkLastUnsorted = end, pLnkPrev = end;
		C4Object *pLastWarnObj = nullptr;
		// forward fix
		uint32_t dwLastCategory = C4D_SortLimit;
		for (auto pLnk = pLnk0; pLnk != std::next(pLnkL); ++pLnk)
		{
			C4Object *pObj = *pLnk;
			if (pObj->Unsorted || !pObj->Status) continue;
			uint32_t dwCategory = pObj->Category & C4D_SortLimit;
			// must have exactly one SortOrder-bit set
			if (!dwCategory)
			{
				DebugLogF("Objects.txt: Object #%d is missing sorting category!", static_cast<int>(pObj->Number));
				++pObj->Category; dwCategory = 1;
			}
			else
			{
				uint32_t dwCat2 = dwCategory, i;
				for (i = 0; ~dwCat2 & 1; ++i) { dwCat2 >>= 1; }
				if (dwCat2 != 1)
				{
					DebugLogF("Objects.txt: Object #%d has invalid sorting category %x!", static_cast<int>(pObj->Number), static_cast<unsigned int>(dwCategory));
					dwCategory = (1 << i);
					pObj->Category = (pObj->Category & ~C4D_SortLimit) | dwCategory;
				}
			}
			// fix order
			if (dwCategory > dwLastCategory)
			{
				// SORT ERROR! (note that pLnkPrev can't be 0)
				if (*pLnkPrev != pLastWarnObj)
				{
					DebugLogF("Objects.txt: Wrong object order of #%d-#%d! (down)", static_cast<int>(pObj->Number), static_cast<int>((*pLnkPrev)->Number));
					pLastWarnObj = *pLnkPrev;
				}
				std::swap(*pLnk, *pLnkPrev);
				pLnkLastUnsorted = pLnkPrev;
			}
			else
				dwLastCategory = dwCategory;
			pLnkPrev = pLnk;
		}
		if (pLnkLastUnsorted == end) break; // done
		pLnkL = pLnkLastUnsorted;
		// backwards fix
		dwLastCategory = 0;
		for (auto pLnk = pLnkL; ; --pLnk)
		{
			C4Object *pObj = *pLnk;
			if (pObj->Unsorted || !pObj->Status)
			{
				if (pLnk == pLnk0)
				{
					break;
				}
				continue;
			}
			uint32_t dwCategory = pObj->Category & C4D_SortLimit;
			if (dwCategory < dwLastCategory)
			{
				// SORT ERROR! (note that pLnkPrev can't be 0)
				if (*pLnkPrev != pLastWarnObj)
				{
					DebugLogF("Objects.txt: Wrong object order of #%d-#%d! (up)", static_cast<int>(pObj->Number), static_cast<int>((*pLnkPrev)->Number));
					pLastWarnObj = *pLnkPrev;
				}
				std::swap(*pLnk, *pLnkPrev);
				pLnk1stUnsorted = pLnkPrev;
			}
			else
				dwLastCategory = dwCategory;
			pLnkPrev = pLnk;

			if (pLnk == pLnk0) break;
		}
		if (pLnk1stUnsorted == end) break; // done
		pLnk0 = pLnk1stUnsorted;
	}
	// objects fixed!
}

void C4GameObjects::ClearDefPointers(C4Def *pDef)
{
	// call in sublists
	C4ObjectList::ClearDefPointers(pDef);
	InactiveObjects.ClearDefPointers(pDef);
}

void C4GameObjects::UpdateDefPointers(C4Def *pDef)
{
	// call in sublists
	C4ObjectList::UpdateDefPointers(pDef);
	InactiveObjects.UpdateDefPointers(pDef);
}

void C4GameObjects::ResortUnsorted()
{
	for (auto it = begin(); it != end(); )
	{
		const auto cObj = *it;
		++it;
		if (cObj->Unsorted)
		{
			// readd to main object list
			Remove(cObj);
			cObj->Unsorted = false;
			if (!Add(cObj))
			{
				// readd failed: Better kill object to prevent leaking...
				Game.ClearPointers(cObj);
				delete cObj;
			}
		}
	}
}

void C4GameObjects::ExecuteResorts()
{
	// custom object sort
	C4ObjResort *pRes = ResortProc;
	while (pRes)
	{
		C4ObjResort *pNextRes = pRes->Next;
		pRes->Execute();
		delete pRes;
		pRes = pNextRes;
	}
	ResortProc = nullptr;
}

bool C4GameObjects::ValidateOwners()
{
	// validate in sublists
	C4ObjectList::ValidateOwners();
	InactiveObjects.ValidateOwners();
	return true;
}

bool C4GameObjects::AssignInfo()
{
	// assign in sublists
	C4ObjectList::AssignInfo();
	InactiveObjects.AssignInfo();
	return true;
}

void C4GameObjects::MoveLinkBefore(const C4ObjectList::iterator &before, const C4ObjectList::iterator &what)
{
	Objects.splice(before, Objects, what);

	// trigger callbacks
	RemoveLink(what);
	InsertLinkBefore(before, what);
}

uint32_t C4GameObjects::GetNextMarker()
{
	// Get a new marker.
	uint32_t marker = ++LastUsedMarker;
	// If all markers are exceeded, restart marker at 1 and reset all object markers to zero.
	if (!marker)
	{
		for (const auto cobj : *this)
		{
			cobj->Marker = 0;
		}
		marker = ++LastUsedMarker;
	}
	return marker;
}
