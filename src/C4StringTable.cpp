/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
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

/* string table: holds all strings used by script engine */

#include <C4Include.h>
#include <C4StringTable.h>

#include <C4Group.h>
#include <C4Components.h>
#include <C4Aul.h>

// *** C4String

C4String::C4String(StdStrBuf &&strString, C4StringTable *pnTable)
	: iRefCnt(0), Hold(false), iEnumID(-1), pTable(nullptr)
{
	// take string
	Data.Take(strString);
	// reg
	Reg(pnTable);
}

C4String::C4String(const char *strString, C4StringTable *pnTable)
	: iRefCnt(0), Hold(false), iEnumID(-1), pTable(nullptr)
{
	// copy string
	Data = strString;
	// reg
	Reg(pnTable);
}

C4String::~C4String()
{
	// unreg
	iRefCnt = 1;
	if (pTable) UnReg();
}

void C4String::IncRef()
{
	++iRefCnt;
}

void C4String::DecRef()
{
	--iRefCnt;

	// delete if ref cnt is 0 and the Hold-Flag isn't set
	if (iRefCnt <= 0 && !Hold)
		delete this;
}

void C4String::Reg(C4StringTable *pnTable)
{
	if (pTable) UnReg();

	// add string to tail of table
	Prev = pnTable->Last;
	Next = nullptr;

	if (Prev)
		Prev->Next = this;
	else
		pnTable->First = this;
	pnTable->Last = this;

	pTable = pnTable;
}

void C4String::UnReg()
{
	if (!pTable) return;

	if (Next)
		Next->Prev = Prev;
	else
		pTable->Last = Prev;
	if (Prev)
		Prev->Next = Next;
	else
		pTable->First = Next;

	pTable = nullptr;

	// delete hold flag if table is lost and check for delete
	Hold = false;
	if (iRefCnt <= 0)
		delete this;
}

// *** C4StringTable

C4StringTable::C4StringTable()
	: First(nullptr), Last(nullptr) {}

C4StringTable::~C4StringTable()
{
	// unreg all remaining strings
	// (hold strings will delete themselves)
	while (First) First->UnReg();
}

void C4StringTable::Clear()
{
	bool bContinue;
	do
	{
		bContinue = false;
		// find string to delete / unreg
		for (C4String *pAct = First; pAct; pAct = pAct->Next)
			if (pAct->Hold)
			{
				pAct->UnReg();
				bContinue = true;
				break;
			}
	} while (bContinue);
}

int C4StringTable::EnumStrings()
{
	int iCurrID = 0;
	for (C4String *pAct = First; pAct; pAct = pAct->Next)
	{
		if (!pAct->Hold || pAct->iRefCnt)
		{
			C4String *same;
			if ((same = FindSaveString(pAct)) == pAct)
			{
				pAct->iEnumID = iCurrID++;
			}
			else
			{
				pAct->iEnumID = same->iEnumID;
			}
		}
		else
		{
			pAct->iEnumID = -1;
		}
	}
	return iCurrID;
}

C4String *C4StringTable::RegString(const char *strString)
{
	return new C4String(strString, this);
}

C4String *C4StringTable::FindString(const char *strString)
{
	for (C4String *pAct = First; pAct; pAct = pAct->Next)
		if (SEqual(pAct->Data.getData(), strString))
			return pAct;
	return nullptr;
}

C4String *C4StringTable::FindString(C4String *pString)
{
	for (C4String *pAct = First; pAct; pAct = pAct->Next)
		if (pAct == pString)
			return pAct;
	return nullptr;
}

C4String *C4StringTable::FindString(int iEnumID)
{
	for (C4String *pAct = First; pAct; pAct = pAct->Next)
		if (pAct->iEnumID == iEnumID)
			return pAct;
	return nullptr;
}

C4String *C4StringTable::FindSaveString(C4String *pString)
{
	for (C4String *pAct = First; pAct; pAct = pAct->Next)
	{
		if (SEqual(pAct->Data.getData(), pString->Data.getData()) && (!pAct->Hold || pAct->iRefCnt))
		{
			return pAct;
		}
	}

	return nullptr;
}

bool C4StringTable::Load(CppC4Group &parentGroup)
{
	// read data
	if (std::string data; !CppC4Group_LoadEntryString(parentGroup, C4CFN_Strings, data))
	{
		return false;
	}
	else
	{
		// read all strings
		char strBuf[C4AUL_MAX_String + 1];
		for (int i = 0; SCopySegment(data.c_str(), i, strBuf, 0x0A, C4AUL_MAX_String); i++)
		{
			SReplaceChar(strBuf, 0x0D, 0x00);
			// add string to list
			C4String *pnString;
			if (!(pnString = FindString(strBuf)))
				pnString = RegString(strBuf);
			pnString->iEnumID = i;
		}
		return true;
	}

}

bool C4StringTable::Save(CppC4Group &parentGroup)
{
	// no tbl entries?
	if (!First) return true;

	// calc needed space for string table
	int iTableSize = 1;
	C4String *pAct;
	for (pAct = First; pAct; pAct = pAct->Next)
	{
		if (pAct->iEnumID > -1 && FindSaveString(pAct) == pAct)
		{
			iTableSize += SLen(pAct->Data.getData()) + 2;
		}
	}

	// no entries?
	if (iTableSize <= 1) return true;

	char *pData = new char[iTableSize], *pPos = pData;
	*pData = 0;
	for (pAct = First; pAct; pAct = pAct->Next)
	{
		if (pAct->iEnumID > -1 && FindSaveString(pAct) == pAct)
		{
			SCopy(pAct->Data.getData(), pPos);
			if (strchr(pPos, 10) || strchr(pPos, 13))
			{
				// delete feeds
				char *pCharPos = pPos;
				while (pCharPos = strchr(pCharPos, 10)) memmove(pCharPos, pCharPos + 1, SLen(pCharPos + 1) + 1);
				// and replace breaks (by C4Script-"escapes")
				SReplaceChar(pPos, 13, '|');
			}
			SAppendChar(0xD, pPos);
			SAppendChar(0xA, pPos);
			pPos += SLen(pPos);
		}
	}

	// write in group
	if (!(parentGroup.createFile(C4CFN_Strings) && parentGroup.setEntryData(C4CFN_Strings, pData, iTableSize, CppC4Group::MemoryManagement::Take)))
	{
		delete[] pData;
		return false;
	}

	return true;
}
