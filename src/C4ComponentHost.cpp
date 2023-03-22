/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
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

/* Holds a single text file component from a group */

#include <C4ComponentHost.h>
#include "C4Config.h"
#include <C4Application.h>
#include <C4Language.h>

C4ComponentHost::C4ComponentHost()
{
	Default();
}

C4ComponentHost::~C4ComponentHost()
{
	Clear();
}

void C4ComponentHost::Default()
{
	Data.Clear();
	Modified = false;
	Name[0] = 0;
	Filename[0] = 0;
	FilePath[0] = 0;
}

void C4ComponentHost::Clear()
{
	Data.Clear();
}

bool C4ComponentHost::Load(const char *szName,
	C4Group &hGroup,
	const char *szFilename,
	const char *szLanguage)
{
	// Clear any old stuff
	Clear();
	// Store name & filename
	SCopy(szName, Name);
	SCopy(szFilename, Filename);
	// Load component - try all segmented filenames
	char strEntry[_MAX_FNAME + 1], strEntryWithLanguage[_MAX_FNAME + 1];
	for (int iFilename = 0; SCopySegment(Filename, iFilename, strEntry, '|', _MAX_FNAME); iFilename++)
	{
		// Try to insert all language codes provided into the filename
		char strCode[3] = "";
		for (int iLang = 0; SCopySegment(szLanguage ? szLanguage : "", iLang, strCode, ',', 2); iLang++)
		{
			// Insert language code
			ssprintf(strEntryWithLanguage, strEntry, strCode);
			if (hGroup.LoadEntryString(strEntryWithLanguage, Data))
			{
				if (Config.General.fUTF8) Data.EnsureUnicode();
				// Store actual filename
				hGroup.FindEntry(strEntryWithLanguage, Filename);
				CopyFilePathFromGroup(hGroup);
				// Got it
				return true;
			}
			// Couldn't insert language code anyway - no point in trying other languages
			if (!SSearch(strEntry, "%s")) break;
		}
	}
	// Truncate any additional segments from stored filename
	SReplaceChar(Filename, '|', 0);
	CopyFilePathFromGroup(hGroup);
	// Not loaded
	return false;
}

bool C4ComponentHost::Load(const char *szName,
	C4GroupSet &hGroupSet,
	const char *szFilename,
	const char *szLanguage)
{
	// Clear any old stuff
	Clear();
	// Store name & filename
	SCopy(szName, Name);
	SCopy(szFilename, Filename);
	// Load component - try all segmented filenames
	char strEntry[_MAX_FNAME + 1], strEntryWithLanguage[_MAX_FNAME + 1];
	for (int iFilename = 0; SCopySegment(Filename, iFilename, strEntry, '|', _MAX_FNAME); iFilename++)
	{
		// Try to insert all language codes provided into the filename
		char strCode[3] = "";
		for (int iLang = 0; SCopySegment(szLanguage ? szLanguage : "", iLang, strCode, ',', 2); iLang++)
		{
			// Insert language code
			ssprintf(strEntryWithLanguage, strEntry, strCode);
			if (hGroupSet.LoadEntryString(strEntryWithLanguage, Data))
			{
				if (Config.General.fUTF8) Data.EnsureUnicode();
				// Store actual filename
				C4Group *pGroup = hGroupSet.FindEntry(strEntryWithLanguage);
				pGroup->FindEntry(strEntryWithLanguage, Filename);
				CopyFilePathFromGroup(*pGroup);
				// Got it
				return true;
			}
			// Couldn't insert language code anyway - no point in trying other languages
			if (!SSearch(strEntry, "%s")) break;
		}
	}
	// Truncate any additional segments from stored filename
	SReplaceChar(Filename, '|', 0);
	// skip full path (unknown)
	FilePath[0] = 0;
	// Not loaded
	return false;
}

bool C4ComponentHost::LoadEx(const char *szName,
	C4Group &hGroup,
	const char *szFilename,
	const char *szLanguage)
{
	// Load from a group set containing the provided group and
	// alternative groups for cross-loading from a language pack
	C4GroupSet hGroups;
	hGroups.RegisterGroup(hGroup, false, 1000, C4GSCnt_Component); // Provided group gets highest priority
	hGroups.RegisterGroups(Languages.GetPackGroups(Config.AtExeRelativePath(hGroup.GetFullName().getData())), C4GSCnt_Language);
	// Load from group set
	return Load(szName, hGroups, szFilename, szLanguage);
}

bool C4ComponentHost::LoadAppend(const char *szName,
	C4Group &hGroup, const char *szFilename,
	const char *szLanguage)
{
	Clear();

	// Store name & filename
	SCopy(szName, Name);
	SCopy(szFilename, Filename);

	// Load component (segmented filename)
	char str1[_MAX_FNAME + 1], str2[_MAX_FNAME + 1];
	size_t iFileCnt = 0, iFileSizeSum = 0;
	for (size_t cseg = 0; SCopySegment(Filename, cseg, str1, '|', _MAX_FNAME); cseg++)
	{
		char szLang[3] = "";
		for (size_t clseg = 0; SCopySegment(szLanguage ? szLanguage : "", clseg, szLang, ',', 2); clseg++)
		{
			ssprintf(str2, str1, szLang);
			// Check existance
			size_t iFileSize;
			if (hGroup.FindEntry(str2, nullptr, &iFileSize))
			{
				iFileCnt++;
				iFileSizeSum += 1 + iFileSize;
				break;
			}
			if (!SSearch(str1, "%s")) break;
		}
	}

	// No matching files found?
	if (!iFileCnt) return false;

	// Allocate memory
	Data.SetLength(iFileSizeSum);

	// Search files to read contents
	char *pPos = Data.getMData(); *pPos = 0;
	for (size_t cseg = 0; SCopySegment(Filename, cseg, str1, '|', _MAX_FNAME); cseg++)
	{
		char szLang[3] = "";
		for (size_t clseg = 0; SCopySegment(szLanguage ? szLanguage : "", clseg, szLang, ',', 2); clseg++)
		{
			ssprintf(str2, str1, szLang);
			// Load data
			char *pTemp;
			if (hGroup.LoadEntry(str2, &pTemp, nullptr, 1))
			{
				*pPos++ = '\n';
				SCopy(pTemp, pPos, Data.getPtr(Data.getLength()) - pPos);
				pPos += SLen(pPos);
				delete[] pTemp;
				break;
			}
			delete[] pTemp;
			if (!SSearch(str1, "%s")) break;
		}
	}

	SReplaceChar(Filename, '|', 0);
	CopyFilePathFromGroup(hGroup);
	return !!iFileCnt;
}

// Construct full path
void C4ComponentHost::CopyFilePathFromGroup(const C4Group &hGroup)
{
	SCopy(Config.AtExeRelativePath(hGroup.GetFullName().getData()), FilePath, _MAX_PATH - 1);
	SAppendChar(DirectorySeparator, FilePath);
	SAppend(Filename, FilePath, _MAX_PATH);
}

bool C4ComponentHost::Save(C4Group &hGroup)
{
	if (!Modified) return true;
	if (!Data) return hGroup.Delete(Filename);
	return hGroup.Add(Filename, Data);
}

bool C4ComponentHost::GetLanguageString(const char *szLanguage, StdStrBuf &rTarget)
{
	const char *cptr;
	// No good parameters
	if (!szLanguage || !Data) return false;
	// Search for two-letter language identifier in text body, i.e. "DE:"
	char langindex[4] = "";
	for (int clseg = 0; SCopySegment(szLanguage ? szLanguage : "", clseg, langindex, ',', 2); clseg++)
	{
		SAppend(":", langindex);
		if (cptr = SSearch(Data.getData(), langindex))
		{
			// Return the according string
			auto iEndPos = SCharPos('\r', cptr);
			if (iEndPos < 0) iEndPos = SCharPos('\n', cptr);
			if (iEndPos < 0) iEndPos = strlen(cptr);
			rTarget.Copy(cptr, iEndPos);
			return true;
		}
	}
	// Language string not found
	return false;
}

void C4ComponentHost::Close()
{
}

void C4ComponentHost::TrimSpaces()
{
	Data.TrimSpaces();
	Modified = true;
}
