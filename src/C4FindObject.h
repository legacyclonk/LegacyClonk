/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
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

#pragma once

#include "C4ForwardDeclarations.h"
#include "C4Id.h"
#include "C4Sector.h"
#include "C4Shape.h"
#include "C4Value.h"
#include "C4Aul.h"

// Condition map
enum C4FindObjectCondID
{
	C4FO_Not = 1,
	C4FO_And = 2,
	C4FO_Or = 3,
	C4FO_Exclude = 5,
	C4FO_InRect = 10,
	C4FO_AtPoint = 11,
	C4FO_AtRect = 12,
	C4FO_OnLine = 13,
	C4FO_Distance = 14,
	C4FO_ID = 20,
	C4FO_OCF = 21,
	C4FO_Category = 22,
	C4FO_Action = 30,
	C4FO_ActionTarget = 31,
	C4FO_Container = 40,
	C4FO_AnyContainer = 41,
	C4FO_Owner = 50,
	C4FO_Controller = 51,
	C4FO_Func = 60,
	C4FO_Layer = 70,
};

// Sort map - using same values as C4FindObjectCondID!
enum C4SortObjectCondID
{
	C4SO_First = 100, // no sort condition smaller than this
	C4SO_Reverse = 101, // reverse sort order
	C4SO_Multiple = 102, // multiple sorts; high priority first; lower priorities if higher prio returned equal
	C4SO_Distance = 110, // nearest first
	C4SO_Random = 120, // random first
	C4SO_Speed = 130, // slowest first
	C4SO_Mass = 140, // lightest first
	C4SO_Value = 150, // cheapest first
	C4SO_Func = 160, // least return values first
	C4SO_Last = 200, // no sort condition larger than this
};

// Base class
class C4FindObject
{
	friend class C4FindObjectNot;
	friend class C4FindObjectAnd;
	friend class C4FindObjectOr;

	class C4SortObject *pSort;

public:
	C4FindObject() : pSort(nullptr) {}
	virtual ~C4FindObject();

	static C4FindObject *CreateByValue(const C4Value &Data, C4SortObject **ppSortObj = nullptr); // createFindObject or SortObject - if ppSortObj==nullptr, SortObject is not allowed

	int32_t Count(const C4ObjectList &Objs); // Counts objects for which the condition is true
	C4Object *Find(const C4ObjectList &Objs);   // Returns first object for which the condition is true
	C4ValueArray *FindMany(const C4ObjectList &Objs); // Returns all objects for which the condition is true

	int32_t Count(C4GameObjects &Objs, const C4LSectors &Sct); // Counts objects for which the condition is true
	C4Object *Find(C4GameObjects &Objs, const C4LSectors &Sct); // Returns first object for which the condition is true
	C4ValueArray *FindMany(C4GameObjects &Objs, const C4LSectors &Sct); // Returns all objects for which the condition is true

	void SetSort(C4SortObject *pToSort);

protected:
	// Overridables
	virtual bool Check(C4Object *pObj) = 0;
	virtual C4Rect *GetBounds() { return nullptr; }
	virtual bool UseShapes() { return false; }
	virtual bool IsImpossible() { return false; }
	virtual bool IsEnsured() { return false; }

private:
	void CheckObjectStatus(std::vector<C4Object *> &objects);
	void CheckObjectStatusAfterSort(std::vector<C4Object *> &objects);
};

// Combinators
class C4FindObjectNot : public C4FindObject
{
public:
	C4FindObjectNot(C4FindObject *pCond)
		: pCond(pCond) {}
	virtual ~C4FindObjectNot();

private:
	C4FindObject *pCond;

protected:
	virtual bool Check(C4Object *pObj) override;
	virtual bool IsImpossible() override { return pCond->IsEnsured(); }
	virtual bool IsEnsured() override { return pCond->IsImpossible(); }
};

class C4FindObjectAnd : public C4FindObject
{
public:
	C4FindObjectAnd(int32_t iCnt, C4FindObject **ppConds, bool fFreeArray = true);
	virtual ~C4FindObjectAnd();

private:
	int32_t iCnt;
	C4FindObject **ppConds; bool fFreeArray; bool fUseShapes;
	C4Rect Bounds; bool fHasBounds;

protected:
	virtual bool Check(C4Object *pObj) override;
	virtual C4Rect *GetBounds() override { return fHasBounds ? &Bounds : nullptr; }
	virtual bool UseShapes() override { return fUseShapes; }
	virtual bool IsEnsured() override { return !iCnt; }
	virtual bool IsImpossible() override;
};

class C4FindObjectOr : public C4FindObject
{
public:
	C4FindObjectOr(int32_t iCnt, C4FindObject **ppConds);
	virtual ~C4FindObjectOr();

private:
	int32_t iCnt;
	C4FindObject **ppConds;
	C4Rect Bounds; bool fHasBounds;

protected:
	virtual bool Check(C4Object *pObj) override;
	virtual C4Rect *GetBounds() override { return fHasBounds ? &Bounds : nullptr; }
	virtual bool IsEnsured() override;
	virtual bool IsImpossible() override { return !iCnt; }
};

// Primitive conditions
class C4FindObjectExclude : public C4FindObject
{
public:
	C4FindObjectExclude(C4Object *pExclude)
		: pExclude(pExclude) {}

private:
	C4Object *pExclude;

protected:
	virtual bool Check(C4Object *pObj) override;
};

class C4FindObjectID : public C4FindObject
{
public:
	C4FindObjectID(C4ID id)
		: id(id) {}

private:
	C4ID id;

protected:
	virtual bool Check(C4Object *pObj) override;
	virtual bool IsImpossible() override;
};

class C4FindObjectInRect : public C4FindObject
{
public:
	C4FindObjectInRect(const C4Rect &rect)
		: rect(rect) {}

private:
	C4Rect rect;

protected:
	virtual bool Check(C4Object *pObj) override;
	virtual C4Rect *GetBounds() override { return &rect; }
	virtual bool IsImpossible() override;
};

class C4FindObjectAtPoint : public C4FindObject
{
public:
	C4FindObjectAtPoint(int32_t x, int32_t y)
		: bounds(x, y, 1, 1) {}

private:
	C4Rect bounds;

protected:
	virtual bool Check(C4Object *pObj) override;
	virtual C4Rect *GetBounds() override { return &bounds; }
	virtual bool UseShapes() override { return true; }
};

class C4FindObjectAtRect : public C4FindObject
{
public:
	C4FindObjectAtRect(int32_t x, int32_t y, int32_t wdt, int32_t hgt)
		: bounds(x, y, wdt, hgt) {}

private:
	C4Rect bounds;

protected:
	virtual bool Check(C4Object *pObj) override;
	virtual C4Rect *GetBounds() override { return &bounds; }
	virtual bool UseShapes() override { return true; }
};

class C4FindObjectOnLine : public C4FindObject
{
public:
	C4FindObjectOnLine(int32_t x, int32_t y, int32_t x2, int32_t y2)
		: x(x), y(y), x2(x2), y2(y2), bounds(x, y, 1, 1)
	{
		bounds.Add(C4Rect(x2, y2, 1, 1));
	}

private:
	int32_t x, y, x2, y2;
	C4Rect bounds;

protected:
	virtual bool Check(C4Object *pObj) override;
	virtual C4Rect *GetBounds() override { return &bounds; }
	virtual bool UseShapes() override { return true; }
};

class C4FindObjectDistance : public C4FindObject
{
public:
	C4FindObjectDistance(int32_t x, int32_t y, int32_t r)
		: x(x), y(y), r2(r * r), bounds(x - r, y - r, 2 * r + 1, 2 * r + 1) {}

private:
	int32_t x, y, r2;
	C4Rect bounds;

protected:
	virtual bool Check(C4Object *pObj) override;
	virtual C4Rect *GetBounds() override { return &bounds; }
};

class C4FindObjectOCF : public C4FindObject
{
public:
	C4FindObjectOCF(int32_t ocf)
		: ocf(ocf) {}

private:
	int32_t ocf;

protected:
	virtual bool Check(C4Object *pObj) override;
	virtual bool IsImpossible() override;
};

class C4FindObjectCategory : public C4FindObject
{
public:
	C4FindObjectCategory(int32_t iCategory)
		: iCategory(iCategory) {}

private:
	int32_t iCategory;

protected:
	virtual bool Check(C4Object *pObj) override;
	virtual bool IsEnsured() override;
};

class C4FindObjectAction : public C4FindObject
{
public:
	C4FindObjectAction(const char *szAction)
		: szAction(szAction) {}

private:
	const char *szAction;

protected:
	virtual bool Check(C4Object *pObj) override;
};

class C4FindObjectActionTarget : public C4FindObject
{
public:
	C4FindObjectActionTarget(C4Object *pActionTarget, int index)
		: pActionTarget(pActionTarget), index(index) {}

private:
	C4Object *pActionTarget;
	int index;

protected:
	virtual bool Check(C4Object *pObj) override;
};

class C4FindObjectContainer : public C4FindObject
{
public:
	C4FindObjectContainer(C4Object *pContainer)
		: pContainer(pContainer) {}

private:
	C4Object *pContainer;

protected:
	virtual bool Check(C4Object *pObj) override;
};

class C4FindObjectAnyContainer : public C4FindObject
{
public:
	C4FindObjectAnyContainer() {}

protected:
	virtual bool Check(C4Object *pObj) override;
};

class C4FindObjectOwner : public C4FindObject
{
public:
	C4FindObjectOwner(int32_t iOwner)
		: iOwner(iOwner) {}

private:
	int32_t iOwner;

protected:
	virtual bool Check(C4Object *pObj) override;
	virtual bool IsImpossible() override;
};

class C4FindObjectFunc : public C4FindObject
{
public:
	C4FindObjectFunc(const char *szFunc);
	void SetPar(int i, const C4Value &val);

private:
	C4AulFunc *pFunc;
	C4AulParSet Pars;

protected:
	virtual bool Check(C4Object *pObj) override;
	virtual bool IsImpossible() override;
};

class C4FindObjectLayer : public C4FindObject
{
public:
	C4FindObjectLayer(C4Object *pLayer) : pLayer(pLayer) {}

private:
	C4Object *pLayer;

protected:
	virtual bool Check(C4Object *pObj) override;
	virtual bool IsImpossible() override;
};

class C4FindObjectController : public C4FindObject
{
public:
	C4FindObjectController(int32_t controller)
		: controller(controller) {}

private:
	int32_t controller;

protected:
	virtual bool Check(C4Object *pObj) override;
	virtual bool IsImpossible() override;
};

// result sorting
class C4SortObject
{
public:
	C4SortObject() {}
	virtual ~C4SortObject() {}

public:
	// Overridables
	virtual int32_t Compare(C4Object *pObj1, C4Object *pObj2) = 0; // return value <0 if obj1 is to be sorted before obj2

	virtual bool PrepareCache([[maybe_unused]] std::vector<C4Object *> &objects) { return false; }
	virtual int32_t CompareCache(int32_t iObj1, int32_t iObj2, C4Object *pObj1, C4Object *pObj2) { return Compare(pObj1, pObj2); }

public:
	static C4SortObject *CreateByValue(const C4Value &Data);
	static C4SortObject *CreateByValue(C4ValueInt iType, const C4ValueArray &Data);

	void SortObjects(std::vector<C4Object *> &result);
};

class C4SortObjectByValue : public C4SortObject
{
public:
	C4SortObjectByValue();
	~C4SortObjectByValue() override = default;

private:
	std::vector<std::int32_t> values;

public:
	// Overridables
	virtual int32_t Compare(C4Object *pObj1, C4Object *pObj2) override;
	virtual int32_t CompareGetValue(C4Object *pOf) = 0;

	virtual bool PrepareCache(std::vector<C4Object *> &objects) override;
	virtual int32_t CompareCache(int32_t iObj1, int32_t iObj2, C4Object *pObj1, C4Object *pObj2) override;
};

class C4SortObjectReverse : public C4SortObject // reverse sort
{
public:
	C4SortObjectReverse(C4SortObject *pSort)
		: C4SortObject(), pSort(pSort) {}
	virtual ~C4SortObjectReverse();

private:
	C4SortObject *pSort;

protected:
	int32_t Compare(C4Object *pObj1, C4Object *pObj2) override;

	virtual bool PrepareCache(std::vector<C4Object *> &objects) override;
	virtual int32_t CompareCache(int32_t iObj1, int32_t iObj2, C4Object *pObj1, C4Object *pObj2) override;
};

class C4SortObjectMultiple : public C4SortObject // apply next sort if previous compares to equality
{
public:
	C4SortObjectMultiple(int32_t iCnt, C4SortObject **ppSorts, bool fFreeArray = true)
		: C4SortObject(), iCnt(iCnt), ppSorts(ppSorts), fFreeArray(fFreeArray) {}
	virtual ~C4SortObjectMultiple();

private:
	bool fFreeArray;
	int32_t iCnt;
	C4SortObject **ppSorts;

protected:
	int32_t Compare(C4Object *pObj1, C4Object *pObj2) override;

	virtual bool PrepareCache(std::vector<C4Object *> &objects) override;
	virtual int32_t CompareCache(int32_t iObj1, int32_t iObj2, C4Object *pObj1, C4Object *pObj2) override;
};

class C4SortObjectDistance : public C4SortObjectByValue // sort by distance from point x/y
{
public:
	C4SortObjectDistance(int iX, int iY)
		: C4SortObjectByValue(), iX(iX), iY(iY) {}

private:
	int iX, iY;

protected:
	int32_t CompareGetValue(C4Object *pFor) override;
};

class C4SortObjectRandom : public C4SortObjectByValue // randomize order
{
public:
	C4SortObjectRandom() : C4SortObjectByValue() {}

protected:
	int32_t CompareGetValue(C4Object *pFor) override;
};

class C4SortObjectSpeed : public C4SortObjectByValue // sort by object xdir/ydir
{
public:
	C4SortObjectSpeed() : C4SortObjectByValue() {}

protected:
	int32_t CompareGetValue(C4Object *pFor) override;
};

class C4SortObjectMass : public C4SortObjectByValue // sort by mass
{
public:
	C4SortObjectMass() : C4SortObjectByValue() {}

protected:
	int32_t CompareGetValue(C4Object *pFor) override;
};

class C4SortObjectValue : public C4SortObjectByValue // sort by value
{
public:
	C4SortObjectValue() : C4SortObjectByValue() {}

protected:
	int32_t CompareGetValue(C4Object *pFor) override;
};

class C4SortObjectFunc : public C4SortObjectByValue // sort by script function
{
public:
	C4SortObjectFunc(const char *szFunc);
	void SetPar(int i, const C4Value &val);

private:
	C4AulFunc *pFunc;
	C4AulParSet Pars;

protected:
	int32_t CompareGetValue(C4Object *pFor) override;
};
