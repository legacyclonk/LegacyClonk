/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
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

// game object lists

#pragma once

#include <C4ObjectList.h>
#include <C4FindObject.h>
#include <C4Sector.h>

class C4ObjResort;

// main object list class
class C4GameObjects : public C4NotifyingObjectList
{
public:
	C4GameObjects();
	~C4GameObjects();
	void Default();
	void Init(int32_t iWidth, int32_t iHeight);
	void Clear(bool fClearInactive = true); // clear objects

private:
	uint32_t LastUsedMarker; // last used value for C4Object::Marker

public:
	C4LSectors Sectors; // section object lists
	C4ObjectList InactiveObjects; // inactive objects (Status=2)
	C4ObjResort *ResortProc; // current sheduled user resorts
	C4ObjectList BackObjects; // objects in background (C4D_Background)
	C4ObjectList ForeObjects; // objects in foreground (C4D_Foreground)

	bool Add(C4Object *nObj); // add object
	bool Remove(C4Object *pObj); // clear pointers to object

	C4ObjectList &ObjectsAt(int ix, int iy); // get object list for map pos

	void CrossCheck(); // various collision-checks
	C4Object *AtObject(int ctx, int cty, uint32_t &ocf, C4Object *exclude = nullptr); // find object at ctx/cty
	void Synchronize(); // network synchronization
	uint32_t GetNextMarker();

	C4Object *FindInternal(C4ID id); // find object in first sector
	virtual C4Object *ObjectPointer(int32_t iNumber) override; // object pointer by number
	std::int32_t ObjectNumber(C4Object *pObj); // object number by pointer

	C4ObjectList &ObjectsInt(); // return object list containing system objects

	void PutSolidMasks();
	void RemoveSolidMasks();

	int Load(C4Section &section, C4Group &hGroup, bool fKeepInactive);
	bool Save(C4Section &section, const char *szFilename, bool fSaveGame, bool fSaveInactive);
	bool Save(C4Section &section, C4Group &hGroup, bool fSaveGame, bool fSaveInactive);

	void UpdateScriptPointers(); // update pointers to C4AulScript *

	void UpdatePos(C4Object *pObj);
	void UpdatePosResort(C4Object *pObj);

	bool OrderObjectBefore(C4Object *pObj1, C4Object *pObj2); // order pObj1 before pObj2
	bool OrderObjectAfter(C4Object *pObj1, C4Object *pObj2); // order pObj1 after pObj2
	void FixObjectOrder(); // Called after loading: Resort any objects that are out of order
	void ResortUnsorted(); // resort any objects with unsorted-flag set into lists
	void ExecuteResorts(); // execute custom resort procs

	void DeleteObjects(); // delete all objects and links

	bool ValidateOwners();
	bool AssignInfo();

	void OnSectionMove(C4Object *obj, C4Section &newSection);
};

class C4AulFunc;

// sheduled resort holder
class C4ObjResort
{
public:
	C4ObjResort(C4GameObjects &objects);
	~C4ObjResort();

	void Execute(); // do the resort!
	void Sort(C4ObjectLink *pFirst, C4ObjectLink *pLast); // sort list between pFirst and pLast
	void SortObject(); // sort single object within its category

	C4GameObjects &Objects; // the object list to sort
	int Category; // object category mask to be sorted
	C4AulFunc *OrderFunc; // function determining new sort order
	C4ObjResort *Next; // next resort holder
	C4Object *pSortObj, *pObjBefore; // objects that are swapped if no OrderFunc is given
	bool fSortAfter; // if set, the sort object is sorted
};
