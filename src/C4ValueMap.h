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

#pragma once

#include <C4Value.h>

// implements a list of C4Values associated with a name list.
// the list is split in the two components (data/names) to make it possible
// to have multiple data lists using a single name list

class C4ValueMapNames;

// the data list
class C4ValueMapData
{
	friend class C4ValueMapNames;

public:
	// construction/destruction
	C4ValueMapData();
	C4ValueMapData(const C4ValueMapData &) = delete;
	virtual ~C4ValueMapData();
	C4ValueMapData &operator=(const C4ValueMapData &DataToCopy);

	bool operator==(const C4ValueMapData &Data) const;

	// data array
	C4Value *pData;

	// pointer on name list
	C4ValueMapNames *pNames;

	// using temporary name list?
	// (delete when changing list)
	bool bTempNameList;

	// returns the item specified or 0 if it doesn't exist
	C4Value *GetItem(const char *strName);
	C4Value *GetItem(int32_t iNr);

	// sets the name list
	void SetNameList(C4ValueMapNames *pnNames);

	// creates a new (empty) temporary name list for this data class
	C4ValueMapNames *CreateTempNameList();

	void Reset(); // resets content & unreg name list

	int32_t GetAnzItems();

	C4Value &operator[](int32_t iNr) { return *GetItem(iNr); }
	C4Value &operator[](const char *strName) { return *GetItem(strName); }

	void DenumeratePointers(C4Section *section = nullptr);

	void CompileFunc(StdCompiler *pComp);

private:
	// a list linking all data lists using the same name list together.
	C4ValueMapData *pNext;

	void Register(C4ValueMapNames *pnNames);
	void UnRegister();

	// called by names list to tell us that the name list
	// was changed and from SetNameList (the data list has to be rordered...)
	void OnNameListChanged(const char *const *pOldNames, int32_t iOldSize);

	// (Re)Allocs data list
	// old data will be deleted!
	// (size taken from pNames->iSize)
	void ReAllocList();
};

// the names list
class C4ValueMapNames
{
	friend class C4ValueMapData;

public:
	// construction/destruction
	C4ValueMapNames();
	C4ValueMapNames(C4ValueMapNames &) = delete;
	C4ValueMapNames &operator=(C4ValueMapNames &NamesToCopy);
	virtual ~C4ValueMapNames();

	// name array
	char **pNames;

	// extra data array
	int32_t *pExtra;

	// item count
	int32_t iSize;

	// set name array
	void SetNameArray(const char *const *pnNames, int32_t nSize);

	// add a name; return index of added element (or index of current if it's already in the list)
	int32_t AddName(const char *pnName, int32_t iExtra = 0);

	// returns the nr of the given name
	// (= nr of value in "child" data lists)
	// returns -1 if no item with given name exists
	int32_t GetItemNr(const char *strName);

	void Reset();

private:
	// points to first data list using this name list
	C4ValueMapData *pFirst;

	void Register(C4ValueMapData *pData);
	void UnRegister(C4ValueMapData *pData);

	// changes the name list
	void ChangeNameList(const char *const *pnNames, int32_t *pnExtra, int32_t nSize);
};
