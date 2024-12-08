/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
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

#include <C4Include.h>
#include <C4FindObject.h>

#include <C4Object.h>
#include <C4Game.h>
#include <C4Wrappers.h>
#include <C4Random.h>

#include <algorithm>
#include <numeric>
#include <ranges>
#include <utility>
#include <vector>

// *** C4FindObject

std::unique_ptr<C4FindObject> C4FindObject::CreateByValue(const C4Value &DataVal, std::vector<std::unique_ptr<C4SortObject>>* sorts)
{
	// Must be an array
	C4ValueArray *pArray = C4Value(DataVal).getArray();
	if (!pArray) return nullptr;

	const C4ValueArray &Data = *pArray;
	const auto iType = Data[0].getInt();
	if (Inside<decltype(iType)>(iType, C4SO_First, C4SO_Last))
	{
		// this is not a FindObject but a sort condition!
		// sort condition not desired here?
		if (!sorts) return nullptr;

		auto sort = C4SortObject::CreateByValue(iType, Data);
		if (sort)
		{
			sorts->emplace_back(std::move(sort));
		}
		// done
		return nullptr;
	}

	switch (iType)
	{
	case C4FO_Not:
	{
		// Create child condition
		auto cond = C4FindObject::CreateByValue(Data[1]);
		if (!cond) return nullptr;
		// wrap
		return std::make_unique<C4FindObjectNot>(std::move(cond));
	}

	case C4FO_And: case C4FO_Or:
	{
		// Trivial case (one condition)
		if (Data.GetSize() == 2)
			return C4FindObject::CreateByValue(Data[1]);

		// Create all childs
		std::vector<std::unique_ptr<C4FindObject>> conds;
		conds.reserve(Data.GetSize() - 1);
		for (int32_t i = 0; i < Data.GetSize() - 1; i++)
		{
			auto cond = C4FindObject::CreateByValue(Data[i + 1]);
			if (cond)
			{
				conds.emplace_back(std::move(cond));
			}
		}

		// Create
		if (iType == C4FO_And)
			return std::make_unique<C4FindObjectAnd>(std::move(conds));
		else
			return std::make_unique<C4FindObjectOr>(std::move(conds));
	}

	case C4FO_Exclude:
		return std::make_unique<C4FindObjectExclude>(Data[1].getObj());

	case C4FO_ID:
		return std::make_unique<C4FindObjectID>(Data[1].getC4ID());

	case C4FO_InRect:
		return std::make_unique<C4FindObjectInRect>(C4Rect(Data[1].getInt(), Data[2].getInt(), Data[3].getInt(), Data[4].getInt()));

	case C4FO_AtPoint:
		return std::make_unique<C4FindObjectAtPoint>(Data[1].getInt(), Data[2].getInt());

	case C4FO_AtRect:
		return std::make_unique<C4FindObjectAtRect>(Data[1].getInt(), Data[2].getInt(), Data[3].getInt(), Data[4].getInt());

	case C4FO_OnLine:
		return std::make_unique<C4FindObjectOnLine>(Data[1].getInt(), Data[2].getInt(), Data[3].getInt(), Data[4].getInt());

	case C4FO_Distance:
		return std::make_unique<C4FindObjectDistance>(Data[1].getInt(), Data[2].getInt(), Data[3].getInt());

	case C4FO_OCF:
		return std::make_unique<C4FindObjectOCF>(Data[1].getInt());

	case C4FO_Category:
		return std::make_unique<C4FindObjectCategory>(Data[1].getInt());

	case C4FO_Action:
	{
		C4String *pStr = Data[1].getStr();
		if (!pStr) return nullptr;
		// Don't copy, it should be safe
		return std::make_unique<C4FindObjectAction>(pStr->Data.getData());
	}

	case C4FO_Func:
	{
		// Get function name
		C4String *pStr = Data[1].getStr();
		if (!pStr) return nullptr;
		// Construct
		auto pFO = std::make_unique<C4FindObjectFunc>(pStr->Data.getData());
		// Add parameters
		for (int i = 2; i < Data.GetSize(); i++)
			pFO->SetPar(i - 2, Data[i]);
		// Done
		return pFO;
	}

	case C4FO_ActionTarget:
	{
		int32_t index = 0;
		if (Data.GetSize() >= 3)
			index = static_cast<decltype(index)>(BoundBy<C4ValueInt>(Data[2].getInt(), 0, 1));
		return std::make_unique<C4FindObjectActionTarget>(Data[1].getObj(), index);
	}

	case C4FO_Container:
		return std::make_unique<C4FindObjectContainer>(Data[1].getObj());

	case C4FO_AnyContainer:
		return std::make_unique<C4FindObjectAnyContainer>();

	case C4FO_Owner:
		return std::make_unique<C4FindObjectOwner>(Data[1].getInt());

	case C4FO_Controller:
		return std::make_unique<C4FindObjectController>(Data[1].getInt());

	case C4FO_Layer:
		return std::make_unique<C4FindObjectLayer>(Data[1].getObj());
	}
	return nullptr;
}

int32_t C4FindObject::Count(const C4ObjectList &Objs)
{
	// Trivial cases
	if (IsImpossible())
		return 0;
	if (IsEnsured())
		return Objs.ObjectCount();
	// Count
	int32_t iCount = 0;
	for (C4ObjectLink *pLnk = Objs.First; pLnk; pLnk = pLnk->Next)
		if (pLnk->Obj->Status)
			if (Check(pLnk->Obj))
				iCount++;
	return iCount;
}

C4Object *C4FindObject::Find(const C4ObjectList &Objs)
{
	// Trivial case
	if (IsImpossible())
		return nullptr;
	// Search
	// Double-check object status, as object might be deleted after Check()!
	C4Object *pBestResult = nullptr;
	for (C4ObjectLink *pLnk = Objs.First; pLnk; pLnk = pLnk->Next)
		if (pLnk->Obj->Status)
			if (Check(pLnk->Obj))
				if (pLnk->Obj->Status)
				{
					// no sorting: Use first object found
					if (!pSort) return pLnk->Obj;
					// Sorting: Check if found object is better
					if (!pBestResult || pSort->Compare(pLnk->Obj, pBestResult) > 0)
						if (pLnk->Obj->Status)
							pBestResult = pLnk->Obj;
				}
	return pBestResult;
}

C4ValueArray *C4FindObject::FindMany(const C4ObjectList &Objs)
{
	// Trivial case
	if (IsImpossible())
		return new C4ValueArray();
	// Set up array
	std::vector<C4Object *> result;
	// Search
	for (C4ObjectLink *pLnk = Objs.First; pLnk; pLnk = pLnk->Next)
		if (pLnk->Obj->Status)
			if (Check(pLnk->Obj))
			{
				result.push_back(pLnk->Obj);
			}
	// Recheck object status (may shrink array again)
	CheckObjectStatus(result);
	// Apply sorting
	if (pSort)
	{
		pSort->SortObjects(result);
		CheckObjectStatusAfterSort(result);
	}
	return new C4ValueArray{std::span{result}};
}

int32_t C4FindObject::Count(const C4ObjectList &Objs, const C4LSectors &Sct)
{
	// Trivial cases
	if (IsImpossible())
		return 0;
	if (IsEnsured())
		return Objs.ObjectCount();
	// Check bounds
	C4Rect *pBounds = GetBounds();
	if (!pBounds)
		return Count(Objs);
	else if (UseShapes())
	{
		// Get area
		C4LArea Area(&Game.Objects.Sectors, *pBounds); C4LSector *pSct;
		C4ObjectList *pLst = Area.FirstObjectShapes(&pSct);
		// Check if a single-sector check is enough
		if (!Area.Next(pSct))
			return Count(pSct->ObjectShapes);
		// Create marker, count over all areas
		uint32_t iMarker = ::Game.Objects.GetNextMarker();
		int32_t iCount = 0;
		for (; pLst; pLst = Area.NextObjectShapes(pLst, &pSct))
			for (C4ObjectLink *pLnk = pLst->First; pLnk; pLnk = pLnk->Next)
				if (pLnk->Obj->Status)
					if (pLnk->Obj->Marker != iMarker)
					{
						pLnk->Obj->Marker = iMarker;
						if (Check(pLnk->Obj))
							iCount++;
					}
		return iCount;
	}
	else
	{
		// Count objects per area
		C4LArea Area(&Game.Objects.Sectors, *pBounds); C4LSector *pSct;
		int32_t iCount = 0;
		for (C4ObjectList *pLst = Area.FirstObjects(&pSct); pLst; pLst = Area.NextObjects(pLst, &pSct))
			iCount += Count(*pLst);
		return iCount;
	}
}

C4Object *C4FindObject::Find(const C4ObjectList &Objs, const C4LSectors &Sct)
{
	// Trivial case
	if (IsImpossible())
		return nullptr;
	C4Object *pBestResult = nullptr;
	// Check bounds
	C4Rect *pBounds = GetBounds();
	if (!pBounds)
		return Find(Objs);
	// Traverse areas, return first matching object w/o sort or best with sort
	else if (UseShapes())
	{
		C4LArea Area(&Game.Objects.Sectors, *pBounds); C4LSector *pSct;
		C4Object *pObj;
		for (C4ObjectList *pLst = Area.FirstObjectShapes(&pSct); pLst; pLst = Area.NextObjectShapes(pLst, &pSct))
			if (pObj = Find(*pLst))
				if (!pSort)
					return pObj;
				else if (!pBestResult || pSort->Compare(pObj, pBestResult) > 0)
					if (pObj->Status)
						pBestResult = pObj;
	}
	else
	{
		C4LArea Area(&Game.Objects.Sectors, *pBounds); C4LSector *pSct;
		C4Object *pObj;
		for (C4ObjectList *pLst = Area.FirstObjects(&pSct); pLst; pLst = Area.NextObjects(pLst, &pSct))
			if (pObj = Find(*pLst))
				if (!pSort)
					return pObj;
				else if (!pBestResult || pSort->Compare(pObj, pBestResult) > 0)
					if (pObj->Status)
						pBestResult = pObj;
	}
	return pBestResult;
}

C4ValueArray *C4FindObject::FindMany(const C4ObjectList &Objs, const C4LSectors &Sct)
{
	// Trivial case
	if (IsImpossible())
		return new C4ValueArray();
	C4Rect *pBounds = GetBounds();
	if (!pBounds)
		return FindMany(Objs);

	std::vector<C4Object *> result;
	// Check shape lists?
	if (UseShapes())
	{
		// Get area
		C4LArea Area(&Game.Objects.Sectors, *pBounds); C4LSector *pSct;
		C4ObjectList *pLst = Area.FirstObjectShapes(&pSct);
		// Check if a single-sector check is enough
		if (!Area.Next(pSct))
			return FindMany(pSct->ObjectShapes);
		// Set up array
		// Create marker, search all areas
		uint32_t iMarker = ::Game.Objects.GetNextMarker();
		for (; pLst; pLst = Area.NextObjectShapes(pLst, &pSct))
			for (C4ObjectLink *pLnk = pLst->First; pLnk; pLnk = pLnk->Next)
				if (pLnk->Obj->Status)
					if (pLnk->Obj->Marker != iMarker)
					{
						pLnk->Obj->Marker = iMarker;
						if (Check(pLnk->Obj))
						{
							result.push_back(pLnk->Obj);
						}
					}
	}
	else
	{
		// Search
		C4LArea Area(&Game.Objects.Sectors, *pBounds); C4LSector *pSct;
		for (C4ObjectList *pLst = Area.FirstObjects(&pSct); pLst; pLst = Area.NextObjects(pLst, &pSct))
			for (C4ObjectLink *pLnk = pLst->First; pLnk; pLnk = pLnk->Next)
				if (pLnk->Obj->Status)
					if (Check(pLnk->Obj))
					{
						result.push_back(pLnk->Obj);
					}
	}
	// Recheck object status (may shrink array again)
	CheckObjectStatus(result);
	// Apply sorting
	if (pSort)
	{
		pSort->SortObjects(result);
		CheckObjectStatusAfterSort(result);
	}
	return new C4ValueArray{std::span{result}};
}

void C4FindObject::CheckObjectStatus(std::vector<C4Object *> &objects)
{
	std::erase_if(objects, [](C4Object *const obj) { return !obj->Status; });
}

void C4FindObject::CheckObjectStatusAfterSort(std::vector<C4Object *> &objects)
{
	std::ranges::replace_if(objects, [](C4Object *const obj) { return !obj->Status; }, nullptr);
}

void C4FindObject::SetSort(std::unique_ptr<C4SortObject> pToSort)
{
	pSort = std::move(pToSort);
}

// *** C4FindObjectNot

bool C4FindObjectNot::Check(C4Object *obj)
{
	return !cond->Check(obj);
}

// *** C4FindObjectAnd

C4FindObjectAnd::C4FindObjectAnd(std::vector<std::unique_ptr<C4FindObject>> conds)
	: conds(std::move(conds)), fHasBounds(false), fUseShapes(false)
{
	// Filter ensured entries
	std::erase_if(conds, [](const std::unique_ptr<C4FindObject> &cond) { return cond->IsEnsured(); });

	// Intersect all child bounds
	for (const auto &cond : conds)
	{
		C4Rect *pChildBounds = cond->GetBounds();
		if (pChildBounds)
		{
			// some objects might be in an rect and at a point not in that rect
			// so do not intersect an atpoint bound with an rect bound
			fUseShapes = cond->UseShapes();
			if (fUseShapes)
			{
				Bounds = *pChildBounds;
				fHasBounds = true;
				break;
			}
			if (fHasBounds)
				Bounds.Intersect(*pChildBounds);
			else
			{
				Bounds = *pChildBounds;
				fHasBounds = true;
			}
		}
	}
}

bool C4FindObjectAnd::Check(C4Object *obj)
{
	return std::ranges::all_of(conds, [obj](const std::unique_ptr<C4FindObject> &cond){ return cond->Check(obj); });
}

bool C4FindObjectAnd::IsImpossible()
{
	return std::ranges::any_of(conds, [](const std::unique_ptr<C4FindObject> &cond) { return cond->IsImpossible(); });
}

// *** C4FindObjectOr

C4FindObjectOr::C4FindObjectOr(std::vector<std::unique_ptr<C4FindObject>> conds)
	: conds(std::move(conds)), fHasBounds(false)
{
	// Filter impossible entries
	std::erase_if(conds, [](const std::unique_ptr<C4FindObject> &cond) { return cond->IsImpossible(); });

	// Sum up all child bounds
	for (const auto &cond : conds)
	{
		C4Rect *pChildBounds = cond->GetBounds();
		if (!pChildBounds)
		{
			fHasBounds = false;
			break;
		}

		// Do not optimize atpoint: It could lead to having to search multiple
		// sectors. An object's shape can be in multiple sectors. We do not want
		// to find the same object twice.
		if (cond->UseShapes())
		{
			fHasBounds = false;
			break;
		}
		if (fHasBounds)
			Bounds.Add(*pChildBounds);
		else
		{
			Bounds = *pChildBounds;
			fHasBounds = true;
		}
	}
}

bool C4FindObjectOr::Check(C4Object *obj)
{
	return std::ranges::any_of(conds, [obj](const std::unique_ptr<C4FindObject> &cond){ return cond->Check(obj); });
}

bool C4FindObjectOr::IsEnsured()
{
	return std::ranges::any_of(conds, [](const std::unique_ptr<C4FindObject> &cond) { return cond->IsEnsured(); });
}

// *** C4FindObject* (primitive conditions)

bool C4FindObjectExclude::Check(C4Object *pObj)
{
	return pObj != pExclude;
}

bool C4FindObjectID::Check(C4Object *pObj)
{
	return pObj->id == id;
}

bool C4FindObjectID::IsImpossible()
{
	C4Def *pDef = C4Id2Def(id);
	return !pDef || !pDef->Count;
}

bool C4FindObjectInRect::Check(C4Object *pObj)
{
	return rect.Contains(pObj->x, pObj->y);
}

bool C4FindObjectInRect::IsImpossible()
{
	return !rect.Wdt || !rect.Hgt;
}

bool C4FindObjectAtPoint::Check(C4Object *pObj)
{
	return pObj->Shape.Contains(bounds.x - pObj->x, bounds.y - pObj->y);
}

bool C4FindObjectAtRect::Check(C4Object *pObj)
{
	C4Rect rcShapeBounds = pObj->Shape;
	rcShapeBounds.x += pObj->x; rcShapeBounds.y += pObj->y;
	return !!rcShapeBounds.Overlap(bounds);
}

bool C4FindObjectOnLine::Check(C4Object *pObj)
{
	return pObj->Shape.IntersectsLine(x - pObj->x, y - pObj->y, x2 - pObj->x, y2 - pObj->y);
}

bool C4FindObjectDistance::Check(C4Object *pObj)
{
	return (pObj->x - x) * (pObj->x - x) + (pObj->y - y) * (pObj->y - y) <= r2;
}

bool C4FindObjectOCF::Check(C4Object *pObj)
{
	return !!(pObj->OCF & ocf);
}

bool C4FindObjectOCF::IsImpossible()
{
	return !ocf;
}

bool C4FindObjectCategory::Check(C4Object *pObj)
{
	return !!(pObj->Category & iCategory);
}

bool C4FindObjectCategory::IsEnsured()
{
	return !iCategory;
}

bool C4FindObjectAction::Check(C4Object *pObj)
{
	return SEqual(pObj->Action.Name, szAction);
}

bool C4FindObjectActionTarget::Check(C4Object *pObj)
{
	assert(index >= 0 && index <= 1);
	if (index == 0)
		return pObj->Action.Target == pActionTarget;
	else if (index == 1)
		return pObj->Action.Target2 == pActionTarget;
	else
		return false;
}

bool C4FindObjectContainer::Check(C4Object *pObj)
{
	return pObj->Contained == pContainer;
}

bool C4FindObjectAnyContainer::Check(C4Object *pObj)
{
	return !!pObj->Contained;
}

bool C4FindObjectOwner::Check(C4Object *pObj)
{
	return pObj->Owner == iOwner;
}

bool C4FindObjectOwner::IsImpossible()
{
	return iOwner != NO_OWNER && !ValidPlr(iOwner);
}

bool C4FindObjectController::Check(C4Object *pObj)
{
	return pObj->Controller == controller;
}

bool C4FindObjectController::IsImpossible()
{
	return controller != NO_OWNER && !ValidPlr(controller);
}

// *** C4FindObjectFunc

C4FindObjectFunc::C4FindObjectFunc(const char *szFunc)
{
	pFunc = Game.ScriptEngine.GetFirstFunc(szFunc);
}

void C4FindObjectFunc::SetPar(int i, const C4Value &Par)
{
	// Over limit?
	if (i >= C4AUL_MAX_Par) return;
	// Set parameter
	Pars[i] = Par;
}

bool C4FindObjectFunc::Check(C4Object *pObj)
{
	// Function not found?
	if (!pFunc) return false;
	// Search same-name-list for appropriate function
	C4AulFunc *pCallFunc = pFunc->FindSameNameFunc(pObj->Def);
	if (!pCallFunc) return false;
	// Call
	return static_cast<bool>(pCallFunc->Exec(pObj, Pars, true));
}

bool C4FindObjectFunc::IsImpossible()
{
	return !pFunc;
}

// *** C4FindObjectLayer

bool C4FindObjectLayer::Check(C4Object *pObj)
{
	return pObj->pLayer == pLayer;
}

bool C4FindObjectLayer::IsImpossible()
{
	return false;
}

// *** C4SortObject

std::unique_ptr<C4SortObject> C4SortObject::CreateByValue(const C4Value &DataVal)
{
	// Must be an array
	const C4ValueArray *pArray = C4Value(DataVal).getArray();
	if (!pArray) return nullptr;
	const C4ValueArray &Data = *pArray;
	return CreateByValue(Data[0].getInt(), Data);
}

std::unique_ptr<C4SortObject> C4SortObject::CreateByValue(C4ValueInt iType, const C4ValueArray &Data)
{
	switch (iType)
	{
	case C4SO_Reverse:
	{
		// create child sort
		auto childSort = C4SortObject::CreateByValue(Data[1]);
		if (!childSort) return nullptr;
		// wrap
		return std::make_unique<C4SortObjectReverse>(std::move(childSort));
	}

	case C4SO_Multiple:
	{
		// Trivial case (one sort)
		if (Data.GetSize() == 2)
		{
			return C4SortObject::CreateByValue(Data[1]);
		}
		// Create all children
		std::vector<std::unique_ptr<C4SortObject>> sorts;
		sorts.reserve(Data.GetSize() - 1);
		for (int32_t i = 0; i < Data.GetSize() - 1; i++)
		{
			auto sort = C4SortObject::CreateByValue(Data[i + 1]);
			if (sort)
			{
				sorts.emplace_back(std::move(sort));
			}
		}
		// Create
		return std::make_unique<C4SortObjectMultiple>(std::move(sorts));
	}

	case C4SO_Distance:
		return std::make_unique<C4SortObjectDistance>(Data[1].getInt(), Data[2].getInt());

	case C4SO_Random:
		return std::make_unique<C4SortObjectRandom>();

	case C4SO_Speed:
		return std::make_unique<C4SortObjectSpeed>();

	case C4SO_Mass:
		return std::make_unique<C4SortObjectMass>();

	case C4SO_Value:
		return std::make_unique<C4SortObjectValue>();

	case C4SO_Func:
	{
		// Get function name
		C4String *pStr = Data[1].getStr();
		if (!pStr) return nullptr;
		// Construct
		auto sort = std::make_unique<C4SortObjectFunc>(pStr->Data.getData());
		// Add parameters
		for (int i = 2; i < Data.GetSize(); i++)
			sort->SetPar(i - 2, Data[i]);
		// Done
		return sort;
	}
	}
	return nullptr;
}

namespace
{
	class C4SortObjectSTL
	{
	private:
		C4SortObject &rSorter;

	public:
		C4SortObjectSTL(C4SortObject &rSorter) : rSorter(rSorter) {}
		bool operator()(C4Object *const v1, C4Object *const v2) { return rSorter.Compare(v1, v2) > 0; }
	};

	class C4SortObjectSTLCache
	{
	private:
		C4SortObject &rSorter;
		std::vector<C4Object *> &values;

	public:
		C4SortObjectSTLCache(C4SortObject &rSorter, std::vector<C4Object *> &values) : rSorter(rSorter), values{values} {}
		bool operator()(int32_t n1, int32_t n2) { return rSorter.CompareCache(n1, n2, values[n1], values[n2]) > 0; }
	};
}

void C4SortObject::SortObjects(std::vector<C4Object *> &result)
{
	if (PrepareCache(result))
	{
		auto positions = std::make_unique_for_overwrite<std::intptr_t[]>(result.size());
		std::span positionsSpan{positions.get(), result.size()};

		// Initialize position array
		std::iota(positionsSpan.begin(), positionsSpan.end(), 0);
		// Sort
		std::ranges::stable_sort(positionsSpan, C4SortObjectSTLCache{*this, result});
		// Save actual object pointers in array (hacky).
		for (std::size_t i{0}; i < result.size(); ++i)
		{
			positions[i] = reinterpret_cast<std::intptr_t>(result[positions[i]]);
		}
		// Set the values
		for (std::size_t i{0}; i < result.size(); ++i)
		{
			result[i] = reinterpret_cast<C4Object *>(positions[i]);
		}
	}
	else
	{
		// Be sure to use stable sort, as otherweise the algorithm isn't garantueed
		// to produce identical results on all platforms!
		std::ranges::stable_sort(result, C4SortObjectSTL{*this});
	}
}

// *** C4SortObjectByValue

C4SortObjectByValue::C4SortObjectByValue()
	: C4SortObject() {}

bool C4SortObjectByValue::PrepareCache(std::vector<C4Object *> &objects)
{
	// Clear old cache

	values.clear();
	values.reserve(objects.size());

	std::ranges::copy(objects | std::views::transform([this](C4Object *const obj)
	{
		return CompareGetValue(obj);
	}), std::back_inserter(values));

	return true;
}

int32_t C4SortObjectByValue::Compare(C4Object *pObj1, C4Object *pObj2)
{
	// this is rather slow, should only be called in special cases

	// make sure to hardcode the call order, as it might lead to desyncs otherwise [Icewing]
	int32_t iValue1 = CompareGetValue(pObj1);
	int32_t iValue2 = CompareGetValue(pObj2);
	return iValue2 - iValue1;
}

int32_t C4SortObjectByValue::CompareCache(int32_t iObj1, int32_t iObj2, C4Object *pObj1, C4Object *pObj2)
{
	assert(iObj1 >= 0 && std::cmp_less(iObj1, values.size())); assert(iObj2 >= 0 && std::cmp_less(iObj2, values.size()));
	// Might overflow for large values...!
	return values[iObj2] - values[iObj1];
}

int32_t C4SortObjectReverse::Compare(C4Object *pObj1, C4Object *pObj2)
{
	return sort->Compare(pObj2, pObj1);
}

bool C4SortObjectReverse::PrepareCache(std::vector<C4Object *> &objects)
{
	return sort->PrepareCache(objects);
}

int32_t C4SortObjectReverse::CompareCache(int32_t iObj1, int32_t iObj2, C4Object *pObj1, C4Object *pObj2)
{
	return sort->CompareCache(iObj2, iObj1, pObj2, pObj1);
}

int32_t C4SortObjectMultiple::Compare(C4Object *pObj1, C4Object *pObj2)
{
	// return first comparison that's nonzero
	for (const auto &sort : sorts)
	{
		const auto result = sort->Compare(pObj1, pObj2);
		if (result != 0)
		{
			return result;
		}
	}
	// all comparisons equal
	return 0;
}

bool C4SortObjectMultiple::PrepareCache(std::vector<C4Object *> &objects)
{
	bool fCaches = false;
	for (const auto &sort : sorts)
	{
		fCaches |= sort->PrepareCache(objects);
	}
	// return wether a sort citerion uses a cache
	return fCaches;
}

int32_t C4SortObjectMultiple::CompareCache(int32_t iObj1, int32_t iObj2, C4Object *pObj1, C4Object *pObj2)
{
	// return first comparison that's nonzero
	for (const auto &sort : sorts)
	{
		const auto result = sort->CompareCache(iObj1, iObj2, pObj1, pObj2);
		if (result != 0)
		{
			return result;
		}
	}
	// all comparisons equal
	return 0;
}

int32_t C4SortObjectDistance::CompareGetValue(C4Object *pFor)
{
	int32_t dx = pFor->x - iX, dy = pFor->y - iY;
	return dx * dx + dy * dy;
}

int32_t C4SortObjectRandom::CompareGetValue(C4Object *pFor)
{
	return Random(1 << 16);
}

int32_t C4SortObjectSpeed::CompareGetValue(C4Object *pFor)
{
	return pFor->xdir * pFor->xdir + pFor->ydir * pFor->ydir;
}

int32_t C4SortObjectMass::CompareGetValue(C4Object *pFor)
{
	return pFor->Mass;
}

int32_t C4SortObjectValue::CompareGetValue(C4Object *pFor)
{
	return pFor->GetValue(nullptr, NO_OWNER);
}

C4SortObjectFunc::C4SortObjectFunc(const char *szFunc)
{
	pFunc = Game.ScriptEngine.GetFirstFunc(szFunc);
}

void C4SortObjectFunc::SetPar(int i, const C4Value &Par)
{
	// Over limit?
	if (i >= C4AUL_MAX_Par) return;
	// Set parameter
	Pars[i] = Par;
}

int32_t C4SortObjectFunc::CompareGetValue(C4Object *pObj)
{
	// Function not found?
	if (!pFunc) return false;
	// Search same-name-list for appropriate function
	C4AulFunc *pCallFunc = pFunc->FindSameNameFunc(pObj->Def);
	if (!pCallFunc) return false;
	// Call
	return pCallFunc->Exec(pObj, Pars, true).getInt();
}
