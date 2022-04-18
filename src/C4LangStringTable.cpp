/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
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

// Loads StringTbl* and replaces $..$-strings by localized versions

#include "C4Include.h"
#include "C4LangStringTable.h"
#include "C4Log.h"

struct C4StringTableEntry
{
	const char *pName, *pEntry;

	C4StringTableEntry(const char *pName, const char *pEntry)
		: pName(pName), pEntry(pEntry) {}
};

void C4LangStringTable::ReplaceStrings(const StdStrBuf &rBuf, StdStrBuf &rTarget, const char *szParentFilePath)
{
	if (!rBuf.getLength())
	{
		return;
	}
	// grab char ptr from buf
	const char *Data = rBuf.getData();

	// string table
	std::vector<C4StringTableEntry> Entries;

	// read string table
	char *pStrTblBuf = nullptr;
	if (GetData())
	{
		// copy data
		pStrTblBuf = new char[GetDataSize() + 1];
		SCopy(GetData(), pStrTblBuf, GetDataSize());
		// find entries
		const char *pLine = pStrTblBuf;
		bool found_eq = false;
		for (char *pPos = pStrTblBuf; *pPos; pPos++)
			if (*pPos == '\n' || *pPos == '\r')
			{
				found_eq = false;
				*pPos = '\0'; pLine = pPos + 1;
			}
			else if (*pPos == '=' && !found_eq)
			{
				*pPos = '\0';
				// We found an '=' sign, so parse everything to end of line from now on, ignoring more '=' signs. Bug #2327.
				found_eq = true;
				// add entry
				Entries.push_back(C4StringTableEntry(pLine, pPos + 1));
			}
	}

	// Find Replace Positions
	auto iScriptLen = SLen(Data);
	struct RP { const char *Pos, *String; unsigned int Len; RP *Next; } *pRPList = nullptr, *pRPListEnd = nullptr;
	for (const char *pPos = SSearch(Data, "$"); pPos; pPos = SSearch(pPos, "$"))
	{
		// Get name
		char szStringName[C4MaxName + 1];
		SCopyUntil(pPos, szStringName, '$', C4MaxName); pPos += SLen(szStringName) + 1;
		if (*(pPos - 1) != '$') continue;
		// valid?
		const char *pPos2 = szStringName;
		while (*pPos2)
			if (!IsIdentifier(*(pPos2++)))
				break;
		if (*pPos2) continue;
		// check termination
		// search in string table
		const char *pStrTblEntry = nullptr;
		for (unsigned int i = 0; i < Entries.size(); i++)
			if (SEqual(szStringName, Entries[i].pName))
			{
				pStrTblEntry = Entries[i].pEntry; break;
			}
		// found?
		if (!pStrTblEntry)
		{
			LogF("%s: string table entry not found: \"%s\"", FilePath[0] ? FilePath : (szParentFilePath ? szParentFilePath : "Unknown"), szStringName);
			continue;
		}
		// add new replace-position entry
		RP *pnRP = new RP;
		pnRP->Pos = pPos - SLen(szStringName) - 2;
		pnRP->String = pStrTblEntry;
		pnRP->Len = SLen(szStringName) + 2;
		pnRP->Next = nullptr;
		pRPListEnd = (pRPListEnd ? pRPListEnd->Next : pRPList) = pnRP;
		// calculate new script length
		iScriptLen += SLen(pStrTblEntry) - pnRP->Len;
	}
	// Alloc new Buffer
	char *pNewBuf;
	StdStrBuf sNewBuf;
	sNewBuf.SetLength(iScriptLen);
	pNewBuf = sNewBuf.getMData();
	// Copy data
	const char *pRPos = Data; char *pWPos = pNewBuf;
	for (RP *pRPPos = pRPList; pRPPos; pRPPos = pRPPos->Next)
	{
		// copy preceding string data
		SCopy(pRPos, pWPos, pRPPos->Pos - pRPos);
		pWPos += pRPPos->Pos - pRPos;
		// copy string
		SCopyUntil(pRPPos->String, pWPos, '\n');
		SReplaceChar(pWPos, '\r', '\0');
		// advance
		pRPos = pRPPos->Pos + pRPPos->Len;
		pWPos += SLen(pWPos);
	}
	SCopy(pRPos, pWPos);

	while (pRPList)
	{
		RP *pRP = pRPList;
		pRPList = pRP->Next;
		delete pRP;
	}

	// free buffer
	delete[] pStrTblBuf;

	// assign this buf
	rTarget.Clear();
	rTarget.Take(sNewBuf);
}

void C4LangStringTable::ReplaceStrings(StdStrBuf &rBuf)
{
	ReplaceStrings(rBuf, rBuf, nullptr);
}
