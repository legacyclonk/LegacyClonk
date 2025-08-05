/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
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
#include <C4Game.h>
#include "C4TextEncoding.h"

C4Language Languages;

C4Language::C4Language()
{
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
			FormatWithNull(strPackFilename, "{}" DirSep "{}", +C4CFN_Languages, +strEntry);
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
	Infos.clear();
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

	// Target location has not changed: return last list of pack groups
	if (SEqualNoCase(strTargetLocation, PackGroupLocation))
		return PackGroups;

	// Process all language packs (and their respective pack groups)
	C4Group *pPack, *newPackGroup, **pPackGroupPtr;
	for (int iPack = 0; (pPack = Packs.GetGroup(iPack)) && (pPackGroupPtr = PackGroups.GetGroupPtr(iPack)); iPack++)
	{
		// Get current pack group position within pack
		SCopy(pPack->GetFullName().getData(), strPackPath, _MAX_PATH);
		GetRelativePath((*pPackGroupPtr)->GetFullName().getData(), strPackPath, strPackGroupLocation);

		// Pack group is at correct position within pack: continue with next pack
		if (SEqualNoCase(strPackGroupLocation, strTargetLocation))
			continue;

		// Try to backtrack until we can reach the target location as a relative child
		while (strPackGroupLocation[0]
			&& !GetRelativePath(strTargetLocation, strPackGroupLocation, strAdvance))
		{
			newPackGroup = (*pPackGroupPtr)->GrabMother();
			if (newPackGroup)
			{
				delete *pPackGroupPtr; // Pack groups are always owned
				*pPackGroupPtr = newPackGroup;
			}
			else
			{
				break;
			}

			// Update pack group location
			GetRelativePath((*pPackGroupPtr)->GetFullName().getData(), strPackPath, strPackGroupLocation);
		}

		// We can reach the target location as a relative child
		if (strPackGroupLocation[0] && GetRelativePath(strTargetLocation, strPackGroupLocation, strAdvance))
		{
			// Advance pack group to relative child
			auto group = std::make_unique<C4Group>();
			if (group->OpenAsChild(*pPackGroupPtr, strAdvance, true))
			{
				(*pPackGroupPtr) = group.release();
			}
		}

		// Cannot reach by advancing: need to close and reopen (rewinding group file)
		else
		{
			// Close pack group (if it is open at all)
			(*pPackGroupPtr)->Close();
			// Reopen pack group to relative position in language pack if possible
			(*pPackGroupPtr)->OpenAsChild(pPack, strTargetLocation);
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
			if (hGroup.LoadEntry(strEntry, &strTable, nullptr, 1))
			{
				// New language info
				C4LanguageInfo info;
				// Get language code by entry name
				std::strncpy(info.Code.data(), GetFilenameOnly(strEntry) + SLen(GetFilenameOnly(strEntry)) - 2, 2);
				info.Code[0] = CharCapital(info.Code[0]);
				info.Code[1] = CharCapital(info.Code[1]);
				// Get language name, info, fallback from table
				C4ResStrTable table{std::string_view{}, strTable};
				info.Name = table.GetEntry(C4ResStrTableKey::IDS_LANG_NAME);
				info.Info = table.GetEntry(C4ResStrTableKey::IDS_LANG_INFO);
				info.Fallback = table.GetEntry(C4ResStrTableKey::IDS_LANG_FALLBACK);
				info.Charset = table.GetEntry(C4ResStrTableKey::IDS_LANG_CHARSET);
				// Safety: pipe character is not allowed in any language info string
				SReplaceChar(info.Name.data(), '|', ' ');
				SReplaceChar(info.Info.data(), '|', ' ');
				SReplaceChar(info.Fallback.data(), '|', ' ');
				// Delete table
				delete[] strTable;
				// Add info to list
				Infos.emplace_back(std::move(info));
			}
}

const C4LanguageInfo *C4Language::FindInfo(const char *const code)
{
	if (const auto it = std::find_if(begin(), end(), [code](auto &info)
	{
		if (CharCapital(info.Code[0]) != CharCapital(*code)) return false;
		return *code && CharCapital(info.Code[1]) == CharCapital(code[1]);
	}); it != end())
	{
		return &*it;
	}

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
	LogNTr(spdlog::level::err, "Error loading language string table.");
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
	FormatWithNull(strEntry, "Language{}.txt", strCode); // ...should use C4CFN_Language here
	// Load string table
	StdStrBuf strTable;
	if (!hGroup.LoadEntryString(strEntry, strTable))
	{
		hGroup.Close(); return false;
	}
	// Set string table
	Application.ResStrTable.emplace(strCode, strTable.getData());
	// Close group
	hGroup.Close();
	// Set the internal charset
	SCopy(LoadResStr(C4ResStrTableKey::IDS_LANG_CHARSET), Config.General.LanguageCharset);

#ifdef _WIN32
	Application.LogSystem.SetConsoleInputCharset(C4Config::GetCharsetCodePage(Config.General.LanguageCharset));
#else
	TextEncodingConverter.CreateConverters(C4Config::GetCharsetCodeName(Config.General.LanguageCharset));
#endif
	// Success
	return true;
}

void C4Language::ClearLanguage()
{
	// Clear resource string table
	Application.ResStrTable.reset();
}
