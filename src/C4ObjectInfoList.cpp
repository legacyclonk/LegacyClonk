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

int32_t C4ObjectInfoList::Load(CppC4Group &group, bool fLoadPortraits)
{
	size_t infoCount = 0;

	// Search all c4i files
	CppC4Group_ForEachEntryByWildcard(group, "", C4CFN_ObjectInfoFiles, [&group, &fLoadPortraits, &infoCount, this](const auto &info)
	{
		Load(group, fLoadPortraits, infoCount, info);
		return true;
	});

	return infoCount;
}


void C4ObjectInfoList::Load(CppC4Group &group, const bool &fLoadPortraits, size_t &infoCount, const CppC4Group::EntryInfo &info)
{
	auto *objectInfo = new C4ObjectInfo;
	if (objectInfo->Load(group, info.fileName.c_str(), fLoadPortraits))
	{
		//Add(objectInfo);
		++infoCount;
	}
	else
	{
		delete objectInfo;
	}

	if (info.directory)
	{
		auto infos = group.getEntryInfos(info.fileName);
		if (infos)
		{
			for (const auto &i : *infos)
			{
				auto grp = group.openAsChild(info.fileName / i.fileName);
				if (grp)
				{
					Load(*grp, true);
				}
			}
		}
	}
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
		sprintf(tstr, " %d", iname);
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
	C4ObjectInfo *pInfo;
	// Create new info object
	if (!(pInfo = new C4ObjectInfo)) return nullptr;
	// Default type clonk if none specified
	if (n_id == C4ID_None) n_id = C4ID_Clonk;
	// Check type valid and def available
	C4Def *pDef = nullptr;
	if (pDefs)
		if (!(pDef = pDefs->ID2Def(n_id)))
		{
			delete pInfo; return nullptr;
		}
	// Override name source by definition
	if (pDef->pClonkNames)
		cpNames = pDef->pClonkNames->GetData();
	// Default by type
	((C4ObjectInfoCore *)pInfo)->Default(n_id, pDefs, cpNames);
	// Set birthday
	pInfo->Birthday = static_cast<int32_t>(time(nullptr));
	// Make valid names
	MakeValidName(pInfo->Name);
	// Add new portrait (permanently w/o copying file)
	if (Config.Graphics.AddNewCrewPortraits)
		pInfo->SetRandomPortrait(0, true, false);
	// Add
	Add(pInfo);
	++iNumCreated;
	return pInfo;
}

void C4ObjectInfoList::Evaluate()
{
	C4ObjectInfo *cinf;
	for (cinf = First; cinf; cinf = cinf->Next)
		cinf->Evaluate();
}

bool C4ObjectInfoList::Save(CppC4Group &group, bool fSavegame, bool fStoreTiny, C4DefList *pDefs)
{
	// Save in opposite order (for identical crew order on load)
	C4ObjectInfo *pInfo;
	for (pInfo = GetLast(); pInfo; pInfo = GetPrevious(pInfo))
	{
		// don't safe TemporaryCrew in regular player files
		if (!fSavegame)
		{
			C4Def *pDef = C4Id2Def(pInfo->id);
			if (pDef) if (pDef->TemporaryCrew) continue;
		}
		// save
		if (!pInfo->Save(group, fStoreTiny, pDefs))
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
		Game.Objects.ClearInfo(cinf);
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
