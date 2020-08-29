/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
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

/*
	Language module
	- handles external language packs
	- provides info on selectable languages by scanning string tables
	- loads and sets a language string table (ResStrTable) based on a specified language sequence
*/

#include <C4Include.h>
#include <C4Language.h>

#include <C4Components.h>
#include <C4Log.h>
#include <C4Config.h>
#ifdef C4ENGINE
#include <C4Game.h>
#endif

#ifdef HAVE_ICONV
#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#endif
#include <errno.h>
iconv_t C4Language::host_to_local = iconv_t(-1);
iconv_t C4Language::local_to_utf_8 = iconv_t(-1);
iconv_t C4Language::local_to_host = iconv_t(-1);
#endif

C4Language Languages;

C4Language::C4Language()
{
	Infos = nullptr;
	PackGroupLocation[0] = 0;
}

C4Language::~C4Language()
{
	Clear();
}

bool C4Language::Init()
{
	// Clear (to allow clean re-init)
	Clear();

	// Make sure Language.c4g is unpacked
	if (ItemExists(C4CFN_Languages))
		if (!DirectoryExists(C4CFN_Languages))
			C4Group_UnpackDirectory(C4CFN_Languages);

	// Look for available language packs in Language.c4g
	C4Group *pPack;
	char strPackFilename[_MAX_FNAME + 1], strEntry[_MAX_FNAME + 1];
	if (PackDirectory.Open(C4CFN_Languages))
		while (PackDirectory.FindNextEntry("*.c4g", strEntry))
		{
			sprintf(strPackFilename, "%s" DirSep "%s", C4CFN_Languages, strEntry);
			pPack = new C4Group();
			if (pPack->Open(strPackFilename))
			{
				Packs.RegisterGroup(*pPack, true, C4GSCnt_Language, false);
			}
			else
			{
				delete pPack;
			}
		}

	// Now create a pack group for each language pack (these pack groups are child groups
	// that browse along each pack to access requested data)
	for (int iPack = 0; pPack = Packs.GetGroup(iPack); iPack++)
		PackGroups.RegisterGroup(*(new C4Group), true, C4GSPrio_Base, C4GSCnt_Language);

	// Load language infos by scanning string tables (the engine doesn't really need this at the moment)
	InitInfos();

	// Done
	return true;
}

void C4Language::Clear()
{
	// Clear pack groups
	PackGroups.Clear();
	// Clear packs
	Packs.Clear();
	// Close pack directory
	PackDirectory.Close();
	// Clear infos
	C4LanguageInfo *pNext;
	while (Infos)
	{
		pNext = Infos->Next;
		delete Infos;
		Infos = pNext;
	}
	Infos = nullptr;
#ifdef HAVE_ICONV
	if (local_to_host != iconv_t(-1))
	{
		if (local_to_host == local_to_utf_8)
			local_to_utf_8 = iconv_t(-1);
		iconv_close(local_to_host);
		local_to_host = iconv_t(-1);
	}
	if (host_to_local != iconv_t(-1))
	{
		iconv_close(host_to_local);
		host_to_local = iconv_t(-1);
	}
	if (local_to_utf_8 != iconv_t(-1))
	{
		iconv_close(local_to_utf_8);
		local_to_utf_8 = iconv_t(-1);
	}

#endif
}

#ifdef HAVE_ICONV

StdStrBuf C4Language::Iconv(const char *string, iconv_t cd)
{
	if (cd == iconv_t(-1))
	{
		return StdStrBuf(string, true);
	}
	StdStrBuf r;
	size_t inlen = strlen(string);
	size_t outlen = strlen(string);
	r.SetLength(inlen);
	const char *inbuf = string;
	char *outbuf = r.getMData();
	while (inlen > 0)
	{
		// Hope that iconv does not change the inbuf...
		if ((size_t)(-1) == iconv(cd, const_cast<ICONV_CONST char * *>(&inbuf), &inlen, &outbuf, &outlen))
		{
			switch (errno)
			{
			// There is not sufficient room at *outbuf.
			case E2BIG:
			{
				size_t done = outbuf - r.getMData();
				r.Grow(inlen * 2);
				outbuf = r.getMData() + done;
				outlen += inlen * 2;
				break;
			}
			// An invalid multibyte sequence has been encountered in the input.
			case EILSEQ:
				++inbuf;
				--inlen;
				break;
			// An incomplete multibyte sequence has been encountered in the input.
			case EINVAL:
			default:
				if (outlen) r.Shrink(outlen);
				return r;
			}
		}
	}
	if (outlen) r.Shrink(outlen);
	// StdStrBuf has taken care of the terminating zero
	return r;
}

StdStrBuf C4Language::IconvSystem(const char *string)
{
	return Iconv(string, local_to_host);
}

StdStrBuf C4Language::IconvClonk(const char *string)
{
	return Iconv(string, host_to_local);
}

StdStrBuf C4Language::IconvUtf8(const char *string)
{
	return Iconv(string, local_to_utf_8);
}

#else

StdStrBuf C4Language::IconvSystem(const char *string)
{
	// Just copy through
	return StdStrBuf(string, true);
}

StdStrBuf C4Language::IconvClonk(const char *string)
{
	// Just copy through
	return StdStrBuf(string, true);
}

#endif

int C4Language::GetInfoCount()
{
	int iCount = 0;
	for (C4LanguageInfo *pInfo = Infos; pInfo; pInfo = pInfo->Next)
		iCount++;
	return iCount;
}

// Returns a set of groups at the specified relative path within all open language packs.

C4GroupSet &C4Language::GetPackGroups(const char *strRelativePath)
{
	// Variables
	char strTargetLocation[_MAX_PATH + 1];
	char strPackPath[_MAX_PATH + 1];
	char strPackGroupLocation[_MAX_PATH + 1];
	char strAdvance[_MAX_PATH + 1];

	// Store wanted target location
	SCopy(strRelativePath, strTargetLocation, _MAX_PATH);

#ifdef C4ENGINE
	// Adjust location by scenario origin
	if (Game.C4S.Head.Origin.getLength() && SEqualNoCase(GetExtension(Game.C4S.Head.Origin.getData()), "c4s"))
	{
		const char *szScenarioRelativePath = GetRelativePathS(strRelativePath, Config.AtExeRelativePath(Game.ScenarioFilename));
		if (szScenarioRelativePath != strRelativePath)
		{
			// this is a path within the scenario! Change to origin.
			size_t iRestPathLen = SLen(szScenarioRelativePath);
			if (Game.C4S.Head.Origin.getLength() + 1 + iRestPathLen <= _MAX_PATH)
			{
				SCopy(Game.C4S.Head.Origin.getData(), strTargetLocation);
				if (iRestPathLen)
				{
					SAppendChar(DirectorySeparator, strTargetLocation);
					SAppend(szScenarioRelativePath, strTargetLocation);
				}
			}
		}
	}
#endif

	// Target location has not changed: return last list of pack groups
	if (SEqualNoCase(strTargetLocation, PackGroupLocation))
		return PackGroups;

	// Process all language packs (and their respective pack groups)
	C4Group *pPack, *pPackGroup;
	for (int iPack = 0; (pPack = Packs.GetGroup(iPack)) && (pPackGroup = PackGroups.GetGroup(iPack)); iPack++)
	{
		// Get current pack group position within pack
		SCopy(pPack->GetFullName().getData(), strPackPath, _MAX_PATH);
		GetRelativePath(pPackGroup->GetFullName().getData(), strPackPath, strPackGroupLocation);

		// Pack group is at correct position within pack: continue with next pack
		if (SEqualNoCase(strPackGroupLocation, strTargetLocation))
			continue;

		// Try to backtrack until we can reach the target location as a relative child
		while (strPackGroupLocation[0]
			&& !GetRelativePath(strTargetLocation, strPackGroupLocation, strAdvance)
			&& pPackGroup->OpenMother())
		{
			// Update pack group location
			GetRelativePath(pPackGroup->GetFullName().getData(), strPackPath, strPackGroupLocation);
		}

		// We can reach the target location as a relative child
		if (strPackGroupLocation[0] && GetRelativePath(strTargetLocation, strPackGroupLocation, strAdvance))
		{
			// Advance pack group to relative child
			pPackGroup->OpenChild(strAdvance);
		}

		// Cannot reach by advancing: need to close and reopen (rewinding group file)
		else
		{
			// Close pack group (if it is open at all)
			pPackGroup->Close();
			// Reopen pack group to relative position in language pack if possible
			pPackGroup->OpenAsChild(pPack, strTargetLocation);
		}
	}

	// Store new target location
	SCopy(strTargetLocation, PackGroupLocation, _MAX_FNAME);

	// Return currently open pack groups
	return PackGroups;
}

void C4Language::InitInfos()
{
	C4Group hGroup;
	// First, look in System.c4g
	if (hGroup.Open(C4CFN_System))
	{
		LoadInfos(hGroup);
		hGroup.Close();
	}
	// Now look through the registered packs
	C4Group *pPack;
	for (int iPack = 0; pPack = Packs.GetGroup(iPack); iPack++)
		// Does it contain a System.c4g child group?
		if (hGroup.OpenAsChild(pPack, C4CFN_System))
		{
			LoadInfos(hGroup);
			hGroup.Close();
		}
}

void C4Language::LoadInfos(C4Group &hGroup)
{
	char strEntry[_MAX_FNAME + 1];
	char *strTable;
	// Look for language string tables
	hGroup.ResetSearch();
	while (hGroup.FindNextEntry(C4CFN_Language, strEntry))
		// For now, we will only load info on the first string table found for a given
		// language code as there is currently no handling for selecting different string tables
		// of the same code - the system always loads the first string table found for a given code
		if (!FindInfo(GetFilenameOnly(strEntry) + SLen(GetFilenameOnly(strEntry)) - 2))
			// Load language string table
			if (hGroup.LoadEntry(strEntry, &strTable, 0, 1))
			{
				// New language info
				C4LanguageInfo *pInfo = new C4LanguageInfo;
				// Get language code by entry name
				SCopy(GetFilenameOnly(strEntry) + SLen(GetFilenameOnly(strEntry)) - 2, pInfo->Code, 2);
				SCapitalize(pInfo->Code);
				// Get language name, info, fallback from table
				SCopy(GetResStr("IDS_LANG_NAME", (char *)strTable), pInfo->Name, C4MaxLanguageInfo);
				SCopy(GetResStr("IDS_LANG_INFO", (char *)strTable), pInfo->Info, C4MaxLanguageInfo);
				SCopy(GetResStr("IDS_LANG_FALLBACK", (char *)strTable), pInfo->Fallback, C4MaxLanguageInfo);
				SCopy(GetResStr("IDS_LANG_CHARSET", (char *)strTable), pInfo->Charset, C4MaxLanguageInfo);
				// Safety: pipe character is not allowed in any language info string
				SReplaceChar(pInfo->Name, '|', ' ');
				SReplaceChar(pInfo->Info, '|', ' ');
				SReplaceChar(pInfo->Fallback, '|', ' ');
				// Delete table
				delete[] strTable;
				// Add info to list
				pInfo->Next = Infos;
				Infos = pInfo;
			}
}

C4LanguageInfo *C4Language::GetInfo(int iIndex)
{
	for (C4LanguageInfo *pInfo = Infos; pInfo; pInfo = pInfo->Next)
		if (iIndex <= 0)
			return pInfo;
		else
			iIndex--;
	return nullptr;
}

C4LanguageInfo *C4Language::FindInfo(const char *strCode)
{
	for (C4LanguageInfo *pInfo = Infos; pInfo; pInfo = pInfo->Next)
		if (SEqualNoCase(pInfo->Code, strCode, 2))
			return pInfo;
	return nullptr;
}

bool C4Language::LoadLanguage(const char *strLanguages)
{
	// Clear old string table
	ClearLanguage();
	// Try to load string table according to language sequence
	char strLanguageCode[2 + 1];
	for (int i = 0; SCopySegment(strLanguages, i, strLanguageCode, ',', 2, true); i++)
		if (InitStringTable(strLanguageCode))
			return true;
	// No matching string table found: hardcoded fallback to US
	if (InitStringTable("US"))
		return true;
	// No string table present: this is really bad
	Log("Error loading language string table.");
	return false;
}

bool C4Language::InitStringTable(const char *strCode)
{
	C4Group hGroup;
	// First, look in System.c4g
	if (hGroup.Open(C4CFN_System))
	{
		if (LoadStringTable(hGroup, strCode))
		{
			hGroup.Close(); return true;
		}
		hGroup.Close();
	}
	// Now look through the registered packs
	C4Group *pPack;
	for (int iPack = 0; pPack = Packs.GetGroup(iPack); iPack++)
		// Does it contain a System.c4g child group?
		if (hGroup.OpenAsChild(pPack, C4CFN_System))
		{
			if (LoadStringTable(hGroup, strCode))
			{
				hGroup.Close(); return true;
			}
			hGroup.Close();
		}
	// No matching string table found
	return false;
}

bool C4Language::LoadStringTable(C4Group &hGroup, const char *strCode)
{
	// Compose entry name
	char strEntry[_MAX_FNAME + 1];
	sprintf(strEntry, "Language%s.txt", strCode); // ...should use C4CFN_Language here
	// Load string table
	StdStrBuf strTable;
	if (!hGroup.LoadEntryString(strEntry, strTable))
	{
		hGroup.Close(); return false;
	}
	// Set string table
	SetResStrTable(strTable.getData());
	// Close group
	hGroup.Close();
	// Set the internal charset
#ifdef C4ENGINE
	SCopy(LoadResStr("IDS_LANG_CHARSET"), Config.General.LanguageCharset);
#endif
#ifdef HAVE_ICONV
#ifdef HAVE_LANGINFO_H
	const char *const to_set = nl_langinfo(CODESET);
	if (local_to_host == iconv_t(-1))
		local_to_host = iconv_open(to_set ? to_set : "ASCII",
			GetCharsetCodeName(Config.General.LanguageCharset));
	if (host_to_local == iconv_t(-1))
		host_to_local = iconv_open(GetCharsetCodeName(Config.General.LanguageCharset),
			to_set ? to_set : "ASCII");
#else
	const char *const to_set = "";
#endif
	if (local_to_utf_8 == iconv_t(-1))
	{
		if (SEqual(to_set, "UTF-8"))
			local_to_utf_8 = local_to_host;
		else
			local_to_utf_8 = iconv_open("UTF-8",
				GetCharsetCodeName(Config.General.LanguageCharset));
	}
#endif
	// Success
	return true;
}

void C4Language::ClearLanguage()
{
	// Clear resource string table
	ClearResStrTable();
}
