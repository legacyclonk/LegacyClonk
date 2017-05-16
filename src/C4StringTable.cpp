/* string table: holds all strings used by script engine */

#include <C4Include.h>
#include <C4StringTable.h>

#ifndef BIG_C4INCLUDE
#include <C4Group.h>
#include <C4Components.h>
#include <C4Aul.h>
#endif

// *** C4String

C4String::C4String(StdStrBuf strString, C4StringTable *pnTable)
	: iRefCnt(0), Hold(false), iEnumID(-1), pTable(NULL)
{
	// take string
	Data.Take(strString);
	// reg
	Reg(pnTable);
}

C4String::C4String(const char *strString, C4StringTable *pnTable)
	: iRefCnt(0), Hold(false), iEnumID(-1), pTable(NULL)
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
	Next = NULL;

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

	pTable = NULL;

	// delete hold flag if table is lost and check for delete
	Hold = false;
	if (iRefCnt <= 0)
		delete this;
}

// *** C4StringTable

C4StringTable::C4StringTable()
	: First(NULL), Last(NULL) {}

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
		if (!pAct->Hold || pAct->iRefCnt)
			pAct->iEnumID = iCurrID++;
		else
			pAct->iEnumID = -1;
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
	return NULL;
}

C4String *C4StringTable::FindString(C4String *pString)
{
	for (C4String *pAct = First; pAct; pAct = pAct->Next)
		if (pAct == pString)
			return pAct;
	return NULL;
}

C4String *C4StringTable::FindString(int iEnumID)
{
	for (C4String *pAct = First; pAct; pAct = pAct->Next)
		if (pAct->iEnumID == iEnumID)
			return pAct;
	return NULL;
}

bool C4StringTable::Load(C4Group &ParentGroup)
{
	// read data
	char *pData;
	if (!ParentGroup.LoadEntry(C4CFN_Strings, &pData, NULL, 1))
		return false;
	// read all strings
	char strBuf[C4AUL_MAX_String + 1];
	for (int i = 0; SCopySegment(pData, i, strBuf, 0x0A, C4AUL_MAX_String); i++)
	{
		SReplaceChar(strBuf, 0x0D, 0x00);
		// add string to list
		C4String *pnString;
		if (!(pnString = FindString(strBuf)))
			pnString = RegString(strBuf);
		pnString->iEnumID = i;
	}
	// delete data
	delete[] pData;
	return true;
}

bool C4StringTable::Save(C4Group &ParentGroup)
{
	// no tbl entries?
	if (!First) return true;

	// calc needed space for string table
	int iTableSize = 1;
	C4String *pAct;
	for (pAct = First; pAct; pAct = pAct->Next)
		if (pAct->iEnumID > -1)
			iTableSize += SLen(pAct->Data.getData()) + 2;

	// no entries?
	if (iTableSize <= 1) return true;

	char *pData = new char[iTableSize], *pPos = pData;
	*pData = 0;
	for (pAct = First; pAct; pAct = pAct->Next)
		if (pAct->iEnumID > -1)
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

	// write in group
	return !!ParentGroup.Add(C4CFN_Strings, pData, iTableSize, FALSE, TRUE);
}
