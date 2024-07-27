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

#include "C4Include.h"
#include "C4ValueMap.h"

// *** C4ValueMapData ***

C4ValueMapData::C4ValueMapData()
	: pData(nullptr), pNames(nullptr), bTempNameList(false), pNext(nullptr) {}

C4ValueMapData &C4ValueMapData::operator=(const C4ValueMapData &DataToCopy)
{
	SetNameList(DataToCopy.pNames);
	if (pNames) for (int32_t i = 0; i < pNames->iSize; i++)
		pData[i].Set(DataToCopy.pData[i]);
	return *this;
}

bool C4ValueMapData::operator==(const C4ValueMapData &Data) const
{
	if (pNames != Data.pNames) return false;
	if (pNames)
		for (int i = 0; i < pNames->iSize; i++)
			if (pData[i] != Data.pData[i])
				return false;
	return true;
}

C4ValueMapData::~C4ValueMapData()
{
	Reset();
}

void C4ValueMapData::Reset()
{
	// unreg from name list (if using one)
	if (pNames) UnRegister();
	pNames = nullptr;
	// free data
	delete[] pData;
	pData = nullptr;
}

void C4ValueMapData::SetNameList(C4ValueMapNames *pnNames)
{
	if (pNames == pnNames) return;
	if (pNames)
	{
		// save name array from old name list
		char **pOldNames = pNames->pNames;
		int32_t iOldSize = pNames->iSize;

		// unreg from old name list
		// (manually, because Data::UnRegister() would destroy content)
		C4ValueMapNames *pNames = this->pNames;

		pNames->UnRegister(this);

		// register at new name list
		pnNames->Register(this);

		// call OnNameListChanged to copy data and realloc data array
		OnNameListChanged(pOldNames, iOldSize);

		// delete old names list, if it is temporary
		if (bTempNameList)
			delete pNames;
		bTempNameList = false;

		// ok
	}
	else
	{
		// simply register...
		Register(pnNames);
	}
}

void C4ValueMapData::Register(C4ValueMapNames *pnNames)
{
	// UnReg from old?
	if (pNames) UnRegister();

	if (pnNames) pnNames->Register(this);
	pNames = pnNames;

	// alloc data array
	ReAllocList();
}

void C4ValueMapData::UnRegister()
{
	if (!pNames) return;

	// safe pointer
	C4ValueMapNames *pNames = this->pNames;

	// unreg
	pNames->UnRegister(this);

	// delete name list (if it is temporary)
	if (bTempNameList)
		delete pNames;
	bTempNameList = false;

	// delete data array
	delete[] pData;
	pData = nullptr;
}

C4ValueMapNames *C4ValueMapData::CreateTempNameList()
{
	// create new list
	C4ValueMapNames *pTempNames = new C4ValueMapNames();

	// register (this func will unreg if necessary, too)
	Register(pTempNames);

	// error?
	if (pNames != pTempNames)
	{
		delete pTempNames;
		return nullptr;
	}

	// set flag
	bTempNameList = true;

	return pTempNames;
}

void C4ValueMapData::ReAllocList()
{
	if (!pNames)
	{
		Reset();
		return;
	}

	// delete old array
	delete[] pData;

	// create new one
	pData = new C4Value[pNames->iSize]();

	// ok...
}

void C4ValueMapData::OnNameListChanged(const char *const *pOldNames, int32_t iOldSize)
{
	if (!pNames)
	{
		Reset();
		return;
	}

	// this function does not use ReAllocList because the old values
	// have to be hold.

	// save pointer on old data
	C4Value *pOldData = pData;

	// create new data list
	pData = new C4Value[pNames->iSize]();

	// (try to) copy data
	int32_t i, j;
	for (i = 0; i < iOldSize; i++)
	{
		// FIXME: This optimization is ugly.
		if (i < pNames->iSize && SEqual(pNames->pNames[i], pOldNames[i]))
		{
			pOldData[i].Move(&pData[i]);
		}
		else for (j = 0; j < pNames->iSize; j++)
		{
			if (SEqual(pNames->pNames[j], pOldNames[i]))
			{
				pOldData[i].Move(&pData[j]);
				break; // in hope this will only break the last for...
			}
		}
	}
	// delete old data array
	delete[] pOldData;
}

C4Value *C4ValueMapData::GetItem(int32_t iNr)
{
	// the list is nothing without name list...
	if (!pNames) return nullptr;

	if (iNr >= pNames->iSize) return nullptr;

	return &pData[iNr];
}

C4Value *C4ValueMapData::GetItem(const char *strName)
{
	if (!pNames) return nullptr;

	int32_t iNr = pNames->GetItemNr(strName);

	if (iNr == -1) return nullptr;

	return &pData[iNr];
}

int32_t C4ValueMapData::GetAnzItems()
{
	if (!pNames) return 0;
	return pNames->iSize;
}

void C4ValueMapData::DenumeratePointers(C4Section *const section)
{
	if (!pNames) return;
	for (int32_t i = 0; i < pNames->iSize; i++)
		pData[i].DenumeratePointer(section);
}

void C4ValueMapData::CompileFunc(StdCompiler *pComp)
{
	bool fCompiler = pComp->isCompiler();
	if (fCompiler) Reset();
	// Compile item count
	int32_t iValueCnt;
	if (!fCompiler) iValueCnt = pNames ? pNames->iSize : 0;
	pComp->Value(mkDefaultAdapt(iValueCnt, 0));
	// nuthing 2do for no items
	if (!iValueCnt) return;
	// Separator (';')
	pComp->Separator(StdCompiler::SEP_SEP2);
	// Data
	char **ppNames = !fCompiler ? pNames->pNames : new char *[iValueCnt];
	if (fCompiler) for (int32_t i = 0; i < iValueCnt; i++) ppNames[i] = nullptr;
	C4Value *pValues = !fCompiler ? pData : new C4Value[iValueCnt];
	// Compile
	try
	{
		for (int32_t i = 0; i < iValueCnt; i++)
		{
			// Separate
			if (i) pComp->Separator();
			// Name
			StdStrBuf Name;
			if (!fCompiler) Name.Ref(ppNames[i]);
			pComp->Value(mkParAdapt(Name, StdCompiler::RCT_Idtf));
			if (fCompiler) ppNames[i] = Name.GrabPointer();
			// Separator ('=')
			pComp->Separator(StdCompiler::SEP_SET);
			// Value
			pComp->Value(pValues[i]);
		}
	}
	catch (...)
	{
		// make sure no mem is leaked on compiler error in name list
		if (fCompiler)
		{
			for (int32_t i = 0; i < iValueCnt; i++) free(ppNames[i]);
			delete[] ppNames;
			delete[] pValues;
		}
		throw;
	}
	// Set
	if (fCompiler)
	{
		C4ValueMapNames *pOldNames = pNames;
		// Set
		CreateTempNameList();
		pNames->SetNameArray(ppNames, iValueCnt);
		for (int32_t i = 0; i < iValueCnt; i++) free(ppNames[i]);
		delete[] ppNames; delete[] pData;
		pData = pValues;
		// Assign old name list
		if (pOldNames) SetNameList(pOldNames);
	}
}

// *** C4ValueMapNames ***

C4ValueMapNames::C4ValueMapNames()
	: pNames(nullptr), pExtra(nullptr), iSize(0), pFirst(nullptr) {}

C4ValueMapNames &C4ValueMapNames::operator=(C4ValueMapNames &NamesToCopy)
{
	ChangeNameList(NamesToCopy.pNames, NamesToCopy.pExtra, NamesToCopy.iSize);
	return *this;
}

C4ValueMapNames::~C4ValueMapNames()
{
	Reset();
}

void C4ValueMapNames::Reset()
{
	// unreg all data lists
	while (pFirst) UnRegister(pFirst);
	// free name list
	for (int32_t i = 0; i < iSize; i++)
		delete[] pNames[i];
	delete[] pNames;
	delete[] pExtra;
	pNames = nullptr; pExtra = nullptr;
	iSize = 0;
}

void C4ValueMapNames::Register(C4ValueMapData *pData)
{
	// add to begin of list
	pData->pNext = pFirst;
	pFirst = pData;
	// set name list
	pData->pNames = this;
}

void C4ValueMapNames::UnRegister(C4ValueMapData *pData)
{
	// find in list
	C4ValueMapData *pAktData = pFirst, *pLastData = nullptr;
	while (pAktData && pAktData != pData)
	{
		pLastData = pAktData;
		pAktData = pAktData->pNext;
	}

	if (!pAktData)
		// isn't registred here...
		return;

	// unreg
	if (pLastData)
		pLastData->pNext = pData->pNext;
	else
		pFirst = pData->pNext;
	pData->pNext = nullptr;

	pData->pNames = nullptr;
}

void C4ValueMapNames::ChangeNameList(const char *const *pnNames, int32_t *pnExtra, int32_t nSize)
{
	// safe old name list
	char **pOldNames = pNames;
	int32_t *pOldExtra = pExtra;
	int32_t iOldSize = iSize;

	// create new lists
	pNames = new char *[nSize];
	pExtra = new int32_t[nSize];

	// copy names
	int32_t i;
	for (i = 0; i < nSize; i++)
	{
		pNames[i] = new char[SLen(pnNames[i]) + 1];
		SCopy(pnNames[i], pNames[i], SLen(pnNames[i]) + 1);
		if (pnExtra) pExtra[i] = pnExtra[i];
	}

	if (!pnExtra) std::fill_n(pExtra, nSize, 0);

	// set new size
	iSize = nSize;

	// call OnNameListChanged list for all "child" lists
	C4ValueMapData *pAktData = pFirst;
	while (pAktData)
	{
		pAktData->OnNameListChanged(pOldNames, iOldSize);
		pAktData = pAktData->pNext;
	}

	// delete old list
	for (i = 0; i < iOldSize; i++)
		delete[] pOldNames[i];
	delete[] pOldNames;
	delete[] pOldExtra;

	// ok.
}

void C4ValueMapNames::SetNameArray(const char *const *pnNames, int32_t nSize)
{
	// simply pass it through...
	ChangeNameList(pnNames, nullptr, nSize);
}

int32_t C4ValueMapNames::AddName(const char *pnName, int32_t iExtra)
{
	// name already existing?
	int32_t iNr;
	if ((iNr = GetItemNr(pnName)) != -1)
		return iNr;

	// create new dummy lists
	const char **pDummyNames = new const char *[iSize + 1];
	int32_t *pDummyExtra = new int32_t[iSize + 1];

	// copy all data from old list
	// (danger! if ChangeNameList would now delete them before
	// creating the new list, this would cause cruel errors...)
	int32_t i;
	for (i = 0; i < iSize; i++)
	{
		pDummyNames[i] = pNames[i];
		pDummyExtra[i] = pExtra[i];
	}
	pDummyNames[i] = pnName;
	pDummyExtra[i] = iExtra;

	// change list
	ChangeNameList(pDummyNames, pDummyExtra, iSize + 1);

	// delete dummy arrays
	delete[] pDummyNames;
	delete[] pDummyExtra;

	// return index to new element (simply last element)
	return iSize - 1;
}

int32_t C4ValueMapNames::GetItemNr(const char *strName)
{
	for (int32_t i = 0; i < iSize; i++)
		if (SEqual(pNames[i], strName))
			return i;
	return -1;
}
