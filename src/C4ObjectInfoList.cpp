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

/* Dynamic list for crew member info */

#include <C4Include.h>
#include <C4ObjectInfoList.h>

#include <C4ObjectInfo.h>
#include <C4Components.h>
#include <C4Game.h>
#include <C4RankSystem.h>
#include <C4Config.h>
#include <C4Wrappers.h>

C4ObjectInfoList::C4ObjectInfoList()
{
	Default();
}

C4ObjectInfoList::~C4ObjectInfoList()
{
	Clear();
}

void C4ObjectInfoList::Default()
{
	First = nullptr;
	iNumCreated = 0;
}

void C4ObjectInfoList::Clear()
{
	C4ObjectInfo *next;
	while (First)
	{
		next = First->Next;
		delete First;
		First = next;
	}
}

int32_t C4ObjectInfoList::Load(C4Group &hGroup, bool fLoadPortraits)
{
	int32_t infn = 0;
	char entryname[256 + 1];

	// Search all c4i files
	hGroup.ResetSearch();
	while (hGroup.FindNextEntry(C4CFN_ObjectInfoFiles, entryname))
	{
		auto info = std::make_unique<C4ObjectInfo>();
		if (info->Load(hGroup, entryname, fLoadPortraits))
		{
			Add(info.release());
			++infn;
		}
	}

	// Search subfolders
	hGroup.ResetSearch();
	while (hGroup.FindNextEntry("*", entryname))
	{
		C4Group ItemGroup;
		if (ItemGroup.OpenAsChild(&hGroup, entryname))
			Load(ItemGroup, fLoadPortraits);
	}

	return infn;
}

bool C4ObjectInfoList::Add(C4ObjectInfo *pInfo)
{
	if (!pInfo) return false;
	pInfo->Next = First;
	First = pInfo;
	return true;
}

void C4ObjectInfoList::MakeValidName(char *sName)
{
	char tstr[_MAX_PATH];
	int32_t iname, namelen = SLen(sName);
	for (iname = 2; NameExists(sName); iname++)
	{
		*std::to_chars(tstr, tstr + std::size(tstr) - 1, iname).ptr = '\0';
		SCopy(tstr, sName + std::min<int32_t>(namelen, C4MaxName - SLen(tstr)));
	}
}

bool C4ObjectInfoList::NameExists(const char *szName)
{
	C4ObjectInfo *cinf;
	for (cinf = First; cinf; cinf = cinf->Next)
		if (SEqualNoCase(szName, cinf->Name))
			return true;
	return false;
}

C4ObjectInfo *C4ObjectInfoList::GetIdle(C4ID c_id, C4DefList &rDefs)
{
	C4Def *pDef;
	C4ObjectInfo *pInfo;
	C4ObjectInfo *pHiExp = nullptr;

	// Search list
	for (pInfo = First; pInfo; pInfo = pInfo->Next)
		// Valid only
		if (pDef = rDefs.ID2Def(pInfo->id))
			// Use standard crew or matching id
			if ((!c_id && !pDef->NativeCrew) || (pInfo->id == c_id))
				// Participating and not in action
				if (pInfo->Participation) if (!pInfo->InAction)
					// Not dead
					if (!pInfo->HasDied)
						// Highest experience
						if (!pHiExp || (pInfo->Experience > pHiExp->Experience))
							// Set this
							pHiExp = pInfo;

	// Found
	if (pHiExp)
	{
		pHiExp->Recruit();
		return pHiExp;
	}

	return nullptr;
}

C4ObjectInfo *C4ObjectInfoList::New(C4ID n_id, C4DefList *pDefs, const char *cpNames)
{
	// Create new info object
	auto info = std::make_unique<C4ObjectInfo>();

	// Default type clonk if none specified
	if (n_id == C4ID_None) n_id = C4ID_Clonk;

	// Check type valid and def available
	C4Def *def{nullptr};
	if (!pDefs || !(def = pDefs->ID2Def(n_id)))
	{
		return nullptr;
	}

	// Override name source by definition
	if (def->pClonkNames)
	{
		cpNames = def->pClonkNames->GetData();
	}

	// Default by type
	static_cast<C4ObjectInfoCore *>(info.get())->Default(n_id, pDefs, cpNames);

	// Set birthday
	info->Birthday = static_cast<int32_t>(time(nullptr));

	// Make valid names
	MakeValidName(info->Name);

	// Add new portrait (permanently w/o copying file)
	if (Config.Graphics.AddNewCrewPortraits)
	{
		info->SetRandomPortrait(0, true, false);
	}

	// Add
	Add(info.get());
	++iNumCreated;

	return info.release();
}

void C4ObjectInfoList::Evaluate()
{
	C4ObjectInfo *cinf;
	for (cinf = First; cinf; cinf = cinf->Next)
		cinf->Evaluate();
}

bool C4ObjectInfoList::Save(C4Group &hGroup, bool fSavegame, bool fStoreTiny, C4DefList *pDefs)
{
	// Save in opposite order (for identical crew order on load)
	C4ObjectInfo *pInfo;
	for (pInfo = GetLast(); pInfo; pInfo = GetPrevious(pInfo))
	{
		// don't safe TemporaryCrew in regular player files
		if (!fSavegame)
		{
			C4Def *pDef = Game.Defs.ID2Def(pInfo->id);
			if (pDef) if (pDef->TemporaryCrew) continue;
		}
		// save
		if (!pInfo->Save(hGroup, fStoreTiny, pDefs))
			return false;
	}
	return true;
}

C4ObjectInfo *C4ObjectInfoList::GetIdle(const char *szByName)
{
	C4ObjectInfo *pInfo;
	// Find matching name, participating, alive and not in action
	for (pInfo = First; pInfo; pInfo = pInfo->Next)
		if (SEqualNoCase(pInfo->Name, szByName))
			if (pInfo->Participation) if (!pInfo->InAction)
				if (!pInfo->HasDied)
				{
					pInfo->Recruit();
					return pInfo;
				}
	return nullptr;
}

void C4ObjectInfoList::DetachFromObjects()
{
	C4ObjectInfo *cinf;
	for (cinf = First; cinf; cinf = cinf->Next)
	{
		std::ranges::for_each(Game.GetNotDeletedSections(), [cinf](C4ObjectList &objects) { objects.ClearInfo(cinf); }, &C4Section::Objects);
	}
}

C4ObjectInfo *C4ObjectInfoList::GetLast()
{
	C4ObjectInfo *cInfo = First;
	while (cInfo && cInfo->Next) cInfo = cInfo->Next;
	return cInfo;
}

C4ObjectInfo *C4ObjectInfoList::GetPrevious(C4ObjectInfo *pInfo)
{
	for (C4ObjectInfo *cInfo = First; cInfo; cInfo = cInfo->Next)
		if (cInfo->Next == pInfo)
			return cInfo;
	return nullptr;
}

bool C4ObjectInfoList::IsElement(C4ObjectInfo *pInfo)
{
	for (C4ObjectInfo *cInfo = First; cInfo; cInfo = cInfo->Next)
		if (cInfo == pInfo) return true;
	return false;
}

void C4ObjectInfoList::Strip(C4DefList &rDefs)
{
	C4ObjectInfo *pInfo, *pPrev;
	// Search list
	for (pInfo = First, pPrev = nullptr; pInfo;)
	{
		// Invalid?
		if (!rDefs.ID2Def(pInfo->id))
		{
			C4ObjectInfo *pDelete = pInfo;
			if (pPrev) pPrev->Next = pInfo->Next; else First = pInfo->Next;
			pInfo = pInfo->Next;
			delete pDelete;
		}
		else
		{
			pPrev = pInfo;
			pInfo = pInfo->Next;
		}
	}
}
