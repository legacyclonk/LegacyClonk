/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
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

/* Dynamic object list */

#include <C4Include.h>
#include <C4ObjectList.h>

#include <C4Object.h>
#include <C4Wrappers.h>
#include <C4Application.h>

#include "StdHelpers.h"

#include <algorithm>
#include <iterator>
#include <unordered_set>

namespace
{
	template<typename Container>
	auto UndeletedFilter(const Container &container) { return StdFilterator{container, [](const auto obj) { return obj->Status != C4OS_DELETED; }}; }
}

C4ObjectList::C4ObjectList()
{
	Clear();
}

C4ObjectList::C4ObjectList(const C4ObjectList &List) : C4ObjectList{}
{
	Copy(List);
}

C4ObjectList::~C4ObjectList()
{
	Clear();
}

void C4ObjectList::Clear()
{
	Objects.clear();
	Mass = 0;
	Enumerated.reset();
}


std::vector<C4ID> C4ObjectList::GetListIDs(int32_t dwCategory)
{
	std::unordered_set<C4ID> ids;
	std::vector<C4ID> ret;
	ret.reserve(100);

	for (const auto obj : UndeletedFilter(Objects))
	{
		assert(C4Id2Def(obj->Def->id) == obj->Def && obj->id == obj->Def->id);
		if (dwCategory == C4D_All || (obj->Def->Category & dwCategory))
		{
			const auto id = obj->id;
			if (ids.emplace(id).second)
			{
				ret.emplace_back(id);
			}
		}
	}

	return ret;
}

bool C4ObjectList::Add(C4Object *nObj, SortType eSort, C4ObjectList *pLstSorted)
{
	if (!nObj || !nObj->Def || !nObj->Status) return false;

#ifdef _DEBUG
	if (eSort == stMain)
	{
		CheckCategorySort();
		if (pLstSorted)
			assert(CheckSort(pLstSorted));
	}
#endif

	// dbg: don't do double links
	assert(GetLink(nObj) == Objects.cend());

	// no self-sort
	assert(pLstSorted != this);

	// Search insert position
	auto insertBefore = Objects.cend();
	auto searchMasterStart = Objects.cend();

	// Should sort?
	if (eSort == stReverse)
	{
		// reverse sort: Add to beginning of list
		insertBefore = Objects.cbegin();
	}
	else if (eSort)
	{
		// Sort override or line? Leave default as is.
		bool fUnsorted = nObj->Unsorted || nObj->Def->Line;
		if (!fUnsorted)
		{
			// Find successor by matching category / id
			// Sort by matching category/id is necessary for inventory shifting.
			bool found = false, fallbackFound = false;
			searchMasterStart = insertBefore = Objects.cbegin();
			auto insertBeforeFallback = Objects.cbegin();
			auto searchMasterStartFallback = Objects.cbegin();
			for (auto it = Objects.cbegin(); it != Objects.cend(); ++it)
			{
				// Unsorted objects are ignored in comparison.
				if ((*it)->Status && !(*it)->Unsorted)
				{
					if (!fallbackFound && ((*it)->Category & C4D_SortLimit) <= (nObj->Category & C4D_SortLimit))
					{
						searchMasterStartFallback = it;
						insertBeforeFallback = it;
						fallbackFound = true;
					}
					// It is not done for static back to allow multiobject outside structure.
					if (!(nObj->Category & C4D_StaticBack) && ((*it)->Category & C4D_SortLimit) == (nObj->Category & C4D_SortLimit) && (*it)->id == nObj->id)
					{
						searchMasterStart = it;
						insertBefore = it;
						found = true;
						break;
					}
				}
			}

			if (!found)
			{
				if (fallbackFound)
				{
					insertBefore = insertBeforeFallback;
					searchMasterStart = searchMasterStartFallback;
				}
				else
				{
					insertBefore = Objects.cbegin();
				}
			}

		}

		// Sort by master list?
		if (pLstSorted)
		{
			assert(CheckSort(pLstSorted));

			// Unsorted: Always search full list (start with first object in list)
			if (fUnsorted) { searchMasterStart = Objects.cbegin(); insertBefore = Objects.cend(); }

			// As cPrev is the last link in front of the first position where the object could be inserted,
			// the object should be after this point in the master list (given it's consistent).
			// If we're about to insert the object at the end of the list, there is obviously nothing to do.
#ifndef _DEBUG
			if (searchMasterStart != Objects.cend())
			{
#endif
				// TODO: optimize somehow?
				const auto origInsertBefore = insertBefore;
				const auto begin2 = pLstSorted->cbegin();
				auto it = Objects.cbegin();
				auto it2 = begin2;
				const auto end = Objects.cend();
				const auto end2 = pLstSorted->cend();
				for (; it != end && it2 != end2; ++it2)
				{
					if (*it2 == nObj)
					{
						insertBefore = it;
						break;
					}

					if (*it == *it2)
					{
						++it;
						if (it == end)
						{
							insertBefore = it;
							break;
						}
					}
				}

				// TODO: remove debugging duplicate
				// Insert new link after predecessor
				InsertLinkBefore(Objects.insert(insertBefore, nObj), insertBefore);

#ifdef _DEBUG
				// Debug: Check sort
				if (eSort == stMain)
				{
					CheckCategorySort();
					if (pLstSorted)
						assert(CheckSort(pLstSorted));
				}
#endif

				// Add mass
				Mass += nObj->Mass;

				return true;
#ifndef _DEBUG
			}
#endif
		}
	}

	// Insert new link after predecessor
	InsertLinkBefore(Objects.insert(insertBefore, nObj), insertBefore);

#ifdef _DEBUG
	// Debug: Check sort
	if (eSort == stMain)
	{
		CheckCategorySort();
		if (pLstSorted)
			assert(CheckSort(pLstSorted));
	}
#endif

	// Add mass
	Mass += nObj->Mass;

	return true;
}

bool C4ObjectList::Remove(C4Object *pObj)
{
	const auto it = GetLink(pObj);
	if (it == Objects.cend())
	{
		return false;
	}

	// Fix iterators
	for (const auto it : SafeIterators)
	{
		if (*it->pLink == pObj) ++it->pLink;
	}

	RemoveLink(it);
	Objects.erase(it);

#ifdef _DEBUG
	if (IsContained(pObj)) BREAKPOINT_HERE;
#endif

	// Remove mass
	Mass -= pObj->Mass; if (Mass < 0) Mass = 0;
	return true;
}

C4Object *C4ObjectList::Find(C4ID id, int owner, uint32_t dwOCF)
{
	if (const auto it = std::find_if(Objects.cbegin(), Objects.cend(), [id, owner, dwOCF](const auto obj)
		{
			assert(obj->id == obj->Def->id);
			return obj->Status && obj->id == id
				&& (owner == ANY_OWNER || obj->Owner == owner)
				&& (dwOCF & obj->OCF);
		}); it != Objects.cend())
	{
		return *it;
	}
	return nullptr;
}

C4Object *C4ObjectList::FindOther(C4ID id, int owner)
{
	if (const auto it = std::find_if(Objects.cbegin(), Objects.cend(), [id, owner](const auto obj)
	{
		assert(obj->id == obj->Def->id);
		return obj->Status && obj->id != id
		&& (owner == ANY_OWNER || obj->Owner == owner);
	}); it != Objects.cend())
	{
		return *it;
	}
	return nullptr;
}

// TODO: Kill this?!
C4Object *C4ObjectList::GetObject(int Index)
{
	if (Index < Objects.size())
	{
		for (const auto obj : UndeletedFilter(Objects))
		{
			if (Index == 0)
			{
				return obj;
			}
			--Index;
		}
	}
	return nullptr;
}

auto C4ObjectList::GetLink(C4Object *pObj) const -> decltype(Objects)::const_iterator
{

	if (!pObj) return Objects.cend();
	return std::find(Objects.cbegin(), Objects.cend(), pObj);
}

int C4ObjectList::ObjectCount(C4ID id, int32_t dwCategory) const
{
	return std::count_if(Objects.cbegin(), Objects.cend(), [id, dwCategory](const auto obj)
		{
			assert(obj->id == obj->Def->id);
			return obj->Status && (id == C4ID_None || obj->id == id) && (dwCategory == C4D_All || obj->Category & dwCategory);
		});
}

int C4ObjectList::MassCount()
{
	Mass = 0;
	for (const auto obj : UndeletedFilter(Objects))
	{
		Mass += obj->Mass;
	}
	return Mass;
}

void C4ObjectList::DrawIDList(C4Facet &cgo, int iSelection,
	C4DefList &rDefs, int32_t dwCategory,
	C4RegionList *pRegions, int iRegionCom,
	bool fDrawOneCounts)
{
	// Variables
	int32_t cSec = 0;
	int32_t iCount;
	C4Facet cgo2;
	C4Object *pFirstObj;
	char szCount[10];
	// objects are sorted in the list already, so just draw them!
	C4ObjectListIterator iter(*this);
	while (pFirstObj = iter.GetNext(&iCount))
	{
		// Section
		cgo2 = cgo.GetSection(cSec);
		// draw picture
		pFirstObj->DrawPicture(cgo2, cSec == iSelection);
		// Draw count
		sprintf(szCount, "%dx", iCount);
		if ((iCount != 1) || fDrawOneCounts)
			Application.DDraw->TextOut(szCount, Game.GraphicsResource.FontRegular, 1.0, cgo2.Surface, cgo2.X + cgo2.Wdt - 1, cgo2.Y + cgo2.Hgt - 1 - Game.GraphicsResource.FontRegular.GetLineHeight(), CStdDDraw::DEFAULT_MESSAGE_COLOR, ARight);
		// Region
		if (pRegions) pRegions->Add(cgo2.X, cgo2.Y, cgo2.Wdt, cgo2.Hgt, pFirstObj->GetName(), iRegionCom, pFirstObj, COM_None, COM_None, pFirstObj->Number);
		// Next section
		cSec++;
	}
}

int C4ObjectList::ClearPointers(C4Object *pObj)
{
	int rval = 0;
	for (auto it = Objects.cbegin(); it != Objects.cend(); )
	{
		// Clear all primary list pointers
		if (*it == pObj)
		{
			it = Objects.erase(it);
			++rval;
		}
		// Clear all sub pointers
		else
		{
			(*it)->ClearPointers(pObj);
			++it;
		}
	}
	return rval;
}

void C4ObjectList::DrawAll(C4FacetEx &cgo, int iPlayer)
{
	DrawIfCategory(cgo, iPlayer, C4D_All, false);
}

void C4ObjectList::DrawIfCategory(C4FacetEx &cgo, int iPlayer, uint32_t dwCat, bool fInvert)
{
	// Draw objects (base)
	for (const auto obj : StdReversed{Objects})
	{
		if (!(obj->Category & dwCat) == fInvert)
		{
			obj->Draw(cgo, iPlayer);
		}
	}
	// Draw objects (top face)
	for (const auto obj : StdReversed{Objects})
	{
		if (!(obj->Category & dwCat) == fInvert)
		{
			obj->DrawTopFace(cgo, iPlayer);
		}
	}
}

void C4ObjectList::Draw(C4FacetEx &cgo, int iPlayer)
{
	DrawIfCategory(cgo, iPlayer, C4D_BackgroundOrForeground, true);
}

void C4ObjectList::Enumerate()
{
	for (const auto obj : UndeletedFilter(Objects))
	{
		obj->EnumeratePointers();
	}
}

int32_t C4ObjectList::ObjectNumber(C4Object *pObj)
{
	// TODO: Optimize this or IsContained?
	if (pObj && IsContained(pObj))
	{
		return pObj->Number;
	}
	return 0;
}

bool C4ObjectList::IsContained(C4Object *pObj)
{
	return std::find(Objects.cbegin(), Objects.cend(), pObj) != Objects.cend();
}

bool C4ObjectList::IsClear() const
{
	return (ObjectCount() == 0);
}

bool C4ObjectList::DenumerateRead()
{
	if (!Enumerated) return false;
	// Denumerate all object pointers
	for (const auto num : *Enumerated)
		Add(Game.Objects.ObjectPointer(num), stNone); // Add to tail, unsorted
	// Delete old list
	Enumerated.reset();
	return true;
}

void C4ObjectList::Denumerate()
{
	for (const auto obj : UndeletedFilter(Objects))
	{
		obj->DenumeratePointers();
	}
}

void C4ObjectList::CompileFunc(StdCompiler *pComp, bool fSaveRefs, bool fSkipPlayerObjects)
{
	if (fSaveRefs)
	{
		// this mode not supported
		assert(!fSkipPlayerObjects);
		// (Re)create list
		Enumerated.reset(new decltype(Enumerated)::element_type);
		// Decompiling: Build list
		if (!pComp->isCompiler())
		{
			for (const auto obj : UndeletedFilter(Objects))
			{
				Enumerated->push_back(obj->Number);
			}
		}
		// Compile list
		pComp->Value(mkSTLContainerAdapt(*Enumerated, StdCompiler::SEP_SEP2));
		// Decompiling: Delete list
		if (!pComp->isCompiler())
		{
			Enumerated.reset();
		}
		// Compiling: Nothing to do - list will be denumerated later
	}
	else
	{
		if (pComp->isDecompiler())
		{
			// skipping player objects would screw object counting in non-naming compilers
			assert(!fSkipPlayerObjects || pComp->hasNaming());
			// Put object count
			int32_t iObjCnt = ObjectCount();
			pComp->Value(mkNamingCountAdapt(iObjCnt, "Object"));
			// Decompile all objects in reverse order
			for (const auto obj : UndeletedFilter(StdReversed{Objects}))
			{
				if (!fSkipPlayerObjects || !obj->IsUserPlayerObject())
				{
					pComp->Value(mkNamingAdapt(*obj, "Object"));
				}
			}
		}
		else
		{
			// this mode not supported
			assert(!fSkipPlayerObjects);
			// Remove previous data
			Clear();
			// Get "Object" section count
			int32_t iObjCnt;
			pComp->Value(mkNamingCountAdapt(iObjCnt, "Object"));
			// Load objects, add them to the list.
			for (int i = 0; i < iObjCnt; i++)
			{
				C4Object *pObj = nullptr;
				try
				{
					pComp->Value(mkNamingAdapt(mkPtrAdaptNoNull(pObj), "Object"));
					Add(pObj, stReverse);
				}
				catch (const StdCompiler::Exception &e)
				{
					// Failsafe object loading: If an error occurs during object loading, just skip that object and load the next one
					if (!e.Pos.getLength())
						LogF("ERROR: Object loading: %s", e.Msg.getData());
					else
						LogF("ERROR: Object loading(%s): %s", e.Pos.getData(), e.Msg.getData());
				}
			}
		}
	}
}

C4Object *C4ObjectList::ObjectPointer(int32_t iNumber)
{
	if (const auto it = std::find_if(Objects.cbegin(), Objects.cend(), [iNumber](const auto obj)
		{
			return obj->Number == iNumber;
		}); it != Objects.cend())
	{
		return *it;
	}
	return nullptr;
}

C4Object *C4ObjectList::SafeObjectPointer(int32_t iNumber)
{
	C4Object *pObj = ObjectPointer(iNumber);
	if (pObj && !pObj->Status) return nullptr;
	return pObj;
}

StdStrBuf C4ObjectList::GetNameList(C4DefList &rDefs, uint32_t dwCategory)
{
	C4ID c_id;
	bool first = true;
	StdStrBuf Buf;
	for (const auto c_id : GetListIDs(dwCategory))
	{
		if (const auto cdef = rDefs.ID2Def(c_id))
		{
			if (!first)
			{
				Buf.Append(", ");
			}
			else
			{
				first = false;
			}
			Buf.AppendFormat("%dx %s", ObjectCount(c_id), cdef->GetName());
		}
	}
	return Buf;
}

void C4ObjectList::ValidateOwners()
{
	for (const auto obj : UndeletedFilter(Objects))
	{
		obj->ValidateOwner();
	}
}

void C4ObjectList::AssignInfo()
{
	// the list seems to be traced backwards here, to ensure crew objects are added in correct order
	// (or semi-correct, because this will work only if the crew order matches the main object list order)
	// this is obsolete now, because the crew list is stored in the savegame
	for (const auto obj : UndeletedFilter(StdReversed{Objects}))
	{
		obj->AssignInfo();
	}
}

void C4ObjectList::AssignPlrViewRange()
{
	for (const auto obj : UndeletedFilter(StdReversed{Objects}))
	{
		obj->AssignPlrViewRange();
	}
}

void C4ObjectList::ClearInfo(C4ObjectInfo *pInfo)
{
	for (const auto obj : UndeletedFilter(Objects))
	{
		obj->ClearInfo(pInfo);
	}
}

void C4ObjectList::ClearDefPointers(C4Def *pDef)
{
	// clear all pointers into definition
	for (const auto obj : Objects)
	{
		if (obj->Def == pDef)
		{
			obj->Name.Clear();
		}
	}
}

void C4ObjectList::UpdateDefPointers(C4Def *pDef)
{
	for (const auto obj : Objects)
	{
		if (obj->Def == pDef)
		{
			obj->SetName(nullptr);
		}
	}
}

void C4NotifyingObjectList::InsertLinkBefore(const iterator &pLink, const iterator &pBefore)
{
	C4ObjectList::InsertLinkBefore(pLink, pBefore);
	ObjectListChangeListener.OnObjectAdded(this, pLink);
}

void C4NotifyingObjectList::InsertLink(const iterator &pLink, const iterator &pAfter)
{
	C4ObjectList::InsertLink(pLink, pAfter);
	ObjectListChangeListener.OnObjectAdded(this, pLink);
}

void C4NotifyingObjectList::RemoveLink(const iterator &pLnk)
{
	C4ObjectList::RemoveLink(pLnk);
	ObjectListChangeListener.OnObjectRemove(this, pLnk);
}

void C4ObjectList::SyncClearance()
{
	for (const auto obj : Objects)
	{
		obj->SyncClearance();
	}
}

void C4ObjectList::UpdateGraphics(bool fGraphicsChanged)
{
	for (const auto obj : UndeletedFilter(Objects))
	{
		obj->UpdateGraphics(fGraphicsChanged);
	}
}

void C4ObjectList::UpdateFaces(bool bUpdateShapes)
{
	for (const auto obj : UndeletedFilter(Objects))
	{
		obj->UpdateGraphics(bUpdateShapes);
	}
}

void C4ObjectList::DrawSelectMark(C4FacetEx &cgo)
{
	for (const auto obj : StdReversed{Objects})
	{
		obj->DrawSelectMark(cgo);
	}
}

void C4ObjectList::CloseMenus()
{
	for (const auto obj : Objects)
	{
		obj->CloseMenu(true);
	}
}

void C4ObjectList::SetOCF()
{
	for (const auto obj : UndeletedFilter(Objects))
	{
		obj->SetOCF();
	}
}

void C4ObjectList::Copy(const C4ObjectList &rList)
{
	Clear();
	for (const auto obj : rList.Objects)
	{
		Add(obj, C4ObjectList::stNone);
	}
}

void C4ObjectList::UpdateTransferZones()
{
	for (const auto obj : Objects)
	{
		obj->Call(PSF_UpdateTransferZone);
	}
}

void C4ObjectList::ResetAudibility()
{
	for (const auto obj : Objects)
	{
		obj->ResetAudibility();
	}
}

void C4ObjectList::SortByCategory()
{
	Objects.sort([](const auto a, const auto b)
	{
		return (a->Category & C4D_SortLimit) > (b->Category & C4D_SortLimit);
	});
}

bool C4ObjectList::OrderObjectBefore(C4Object *pObj1, C4Object *pObj2)
{
	// safety
	if (pObj1->Status != C4OS_NORMAL || pObj2->Status != C4OS_NORMAL) return false;
	// get links (and check whether the objects are part of this list!)
	const auto end = Objects.cend();
	const auto pLnk1 = GetLink(pObj1);
	if (pLnk1 == end) return false;
	const auto pLnk2 = GetLink(pObj2);
	if (pLnk2 == end) return false;

	// check if requirements are already fulfilled
	auto it = pLnk1;
	while (it != end && it != pLnk2)
	{
		++it;
	}

	// if not, reorder pLnk1 directly before pLnk2
	if (it == end)
	{
		Objects.splice(pLnk2, Objects, pLnk1);
	}
	// done, success
	return true;
}

bool C4ObjectList::OrderObjectAfter(C4Object *pObj1, C4Object *pObj2)
{
	// safety
	if (pObj1->Status != C4OS_NORMAL || pObj2->Status != C4OS_NORMAL) return false;
	// get links (and check whether the objects are part of this list!)
	const auto end = Objects.cend();
	const auto pLnk1 = GetLink(pObj1);
	if (pLnk1 == end) return false;
	const auto pLnk2 = GetLink(pObj2);
	if (pLnk2 == end) return false;

	// check if requirements are already fulfilled
	auto it = pLnk2;
	while (it != end && it != pLnk1)
	{
		++it;
	}

	// if not, reorder pLnk1 directly after pLnk2
	if (it == end)
	{
		Objects.splice(std::next(pLnk2), Objects, pLnk1);
	}
	// done, success
	return true;
}

bool C4ObjectList::ShiftContents(C4Object *pNewFirst)
{
	const auto it = GetLink(pNewFirst);
	if (it == Objects.cend())
	{
		return false;
	}
	// already at front? (would be undefined behavior for splice)
	if (it == Objects.cbegin())
	{
		return true;
	}
	Objects.splice(Objects.cbegin(), Objects, it, Objects.cend());
	return true;
}

void C4ObjectList::DeleteObjects()
{
	// delete links and objects
	for (auto it = Objects.cbegin(); it != Objects.cend(); )
	{
		const auto obj = *it;
		++it;
		Remove(obj);
		delete obj;
	}

	// reset mass
	Mass = 0;
}

// C4ObjectListIterator

C4Object *C4ObjectListIterator::GetNext(int32_t *piCount, uint32_t dwCategory)
{
	// end reached?
	if (pCurrID == rList.safeEnd()) return nullptr;
	// not yet started?
	if (pCurr == rList.safeEnd())
		// then start at first ID list head
		pCurr = pCurrID;
	else
		// next item
		if (++pCurr == rList.safeEnd()) return nullptr;
	// skip mismatched category
	if (dwCategory)
		while (!((*pCurr)->Category & dwCategory))
			if (++pCurr == rList.safeEnd()) return nullptr;
	// next ID section reached?
	if ((*pCurr)->id != (*pCurrID)->id)
		pCurrID = pCurr;
	else
	{
		// otherwise, it must be checked, whether this is a duplicate item already iterated
		// if so, advance the list
		for (auto pCheck = pCurrID; pCheck != pCurr; ++pCheck)
			if (!dwCategory || ((*pCheck)->Category & dwCategory))
				if ((*pCheck)->CanConcatPictureWith(*pCurr))
				{
					// next object of matching category
					if (++pCurr == rList.safeEnd()) return nullptr;
					if (dwCategory)
						while (!((*pCurr)->Category & dwCategory))
							if (++pCurr == rList.safeEnd()) return nullptr;
					// next ID chunk reached?
					if ((*pCurr)->id != (*pCurrID)->id)
					{
						// then break here
						pCurrID = pCurr;
						break;
					}
					// restart check for next object
					pCheck = pCurrID;
				}
	}
	if (piCount)
	{
		// default count
		*piCount = 1;
		// add additional objects of same ID to the count
		auto pCheck = pCurr;
		for (++pCheck; pCheck != rList.safeEnd() && (*pCheck)->id == (*pCurr)->id; ++pCheck)
			if (!dwCategory || ((*pCheck)->Category & dwCategory))
				if ((*pCheck)->CanConcatPictureWith(*pCurr))
					++*piCount;
	}
	// return found object
	return *pCurr;
}

void C4ObjectList::UpdateScriptPointers()
{
	for (const auto obj : Objects)
	{
		obj->UpdateScriptPointers();
	}
}

struct C4ObjectListDumpHelper
{
	C4ObjectList *pLst;

	void CompileFunc(StdCompiler *pComp) { pComp->Value(mkNamingAdapt(*pLst, "Objects")); }

	C4ObjectListDumpHelper(C4ObjectList *pLst) : pLst(pLst) {}
};

bool C4ObjectList::CheckSort(C4ObjectList *pList)
{
	for (auto it = Objects.cbegin(), otherIt = pList->Objects.cbegin(); it != Objects.cend(); )
	{
		if ((*it)->Unsorted)
		{
			++it;
			continue;
		}

		while (*otherIt != *it)
		{
			if (otherIt == pList->Objects.cend())
			{
				Log("CheckSort failure");
				LogSilent(DecompileToBuf<StdCompilerINIWrite>(mkNamingAdapt(C4ObjectListDumpHelper{this}, "SectorList")).getData());
				LogSilent(DecompileToBuf<StdCompilerINIWrite>(mkNamingAdapt(C4ObjectListDumpHelper{pList}, "MainList")).getData());
				return false;
			}
			++otherIt;
		}

		++it;
		++otherIt;
	}
	return true;
}

void C4ObjectList::CheckCategorySort()
{
	// debug: Check whether object list is sorted correctly
	C4Object *prev{nullptr};
	for (const auto obj : UndeletedFilter(Objects))
	{
		if (!obj->Unsorted)
		{
			assert(!prev || (prev->Category & C4D_SortLimit) >= (obj->Category & C4D_SortLimit));
			prev = obj;
		}
	}
}

C4ObjectList::SafeIterator::SafeIterator(C4ObjectList &List) :
	List(List), pLink(List.cbegin())
{
	List.AddIter(this);
}

C4ObjectList::SafeIterator::SafeIterator(C4ObjectList &List, const C4ObjectList::iterator &pLink) :
	List(List), pLink(pLink)
{
	List.AddIter(this);
}

C4ObjectList::SafeIterator::SafeIterator(const C4ObjectList::SafeIterator &iter) :
	List(iter.List), pLink(iter.pLink)
{
	List.AddIter(this);
}

C4ObjectList::SafeIterator::~SafeIterator()
{
	List.RemoveIter(this);
}

C4ObjectList::SafeIterator &C4ObjectList::SafeIterator::operator++()
{
	++pLink;
	return *this;
}

C4Object *C4ObjectList::SafeIterator::operator*()
{
	return pLink != List.cend() ? *pLink : nullptr;
}

bool C4ObjectList::SafeIterator::operator==(const SafeIterator &iter) const
{
	return &iter.List == &List && iter.pLink == pLink;
}

bool C4ObjectList::SafeIterator::operator!=(const SafeIterator &iter) const
{
	return &iter.List != &List || iter.pLink != pLink;
}

C4ObjectList::SafeIterator &C4ObjectList::SafeIterator::operator=(const SafeIterator &iter)
{
	// Can only assign iterators into the same list
	assert(&iter.List == &List);

	pLink = iter.pLink;
	return *this;
}

C4ObjectList::SafeIterator C4ObjectList::safeBegin()
{
	return SafeIterator(*this);
}

const C4ObjectList::SafeIterator C4ObjectList::safeEnd()
{
	return SafeIterator(*this, Objects.cend());
}

void C4ObjectList::AddIter(SafeIterator *iter)
{
	SafeIterators.push_back(iter);
}

void C4ObjectList::RemoveIter(SafeIterator *iter)
{
	SafeIterators.erase(std::remove(SafeIterators.begin(), SafeIterators.end(), iter), SafeIterators.end());
}
