/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
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

// a set of group files
// manages system file overwriting by scearios or folders

#include <C4Include.h>
#include <C4GroupSet.h>

#ifdef C4ENGINE
#include <C4Game.h>
#include <C4Log.h>
#else
#include <C4Group.h>
#include <C4Components.h>
#endif

#include <filesystem>

C4GroupSetNode::C4GroupSetNode(C4GroupSet &rParent, C4GroupSetNode *pPrev, CppC4Group &group, bool fGrpOwned, int32_t id)
{
	// set parent
	pParent = &rParent;
	// link into list
	this->pPrev = pPrev;
	if (pPrev) { pNext = pPrev->pNext; pPrev->pNext = this; }
	else { pNext = pParent->pFirst; pParent->pFirst = this; }
	if (pNext) pNext->pPrev = this; else pParent->pLast = this;
	// set group
	pGroup = &group;
	this->fGrpOwned = fGrpOwned;
	// set id
	this->id = id;
}

C4GroupSetNode::~C4GroupSetNode()
{
	// unlink from list
	(pPrev ? pPrev->pNext : pParent->pFirst) = pNext;
	(pNext ? pNext->pPrev : pParent->pLast) = pPrev;
}

void C4GroupSet::Clear()
{
	// clear nodes
	while (pFirst) delete pFirst;
	pFirst = nullptr;
}

void C4GroupSet::Default()
{
	// zero fields
	pFirst = pLast = nullptr;
	// do not reset index here, because group set IDs are meant to be unique
	// for each instance of the engine
	// see also C4GraphicsResource::RegisterGlobalGraphics
}

C4GroupSet::C4GroupSet()
{
	// zero fields
	Default();
	iIndex = 0;
}

C4GroupSet::~C4GroupSet()
{
	// clear nodes
	Clear();
}

bool C4GroupSet::RegisterGroup(CppC4Group &group, bool fOwnGrp, int32_t Priority, int32_t Contents, bool fCheckContent)
{
	// get node to sort in
	// begin at back end and search for higher priority
	C4GroupSetNode *pNode;
	for (pNode = pLast; pNode; pNode = pNode->pPrev)
		if (pNode->Priority > Priority) break;
	// create new node
	C4GroupSetNode *pNewNode = new C4GroupSetNode(*this, pNode, group, fOwnGrp, ++iIndex);
	// check content
	if (fCheckContent) Contents = CheckGroupContents(group, Contents);
	// set priority and contents mask
	pNewNode->Priority = Priority;
	pNewNode->Contents = Contents;

#if defined(C4ENGINE) && !defined(USE_CONSOLE)
	// always add fonts directly
	if (Contents & C4GSCnt_FontDefs)
		Game.FontLoader.LoadDefs(group, Config);
	// success
#endif

	return true;
}

int32_t C4GroupSet::CheckGroupContents(CppC4Group &group, int32_t Contents)
{
	// update mask
	if (Contents & C4GSCnt_Graphics) if (!group.getEntryInfo(C4CFN_Graphics)) Contents = Contents & ~C4GSCnt_Graphics;
	if (Contents & C4GSCnt_Loaders)
	{
		if (!group.getEntryInfo("Loader*.bmp")
			&& !group.getEntryInfo("Loader*.png")
			&& !group.getEntryInfo("Loader*.jpg")
			&& !group.getEntryInfo("Loader*.jpeg"))
		{
			Contents = Contents & ~C4GSCnt_Loaders;
		}
	}
	if (Contents & C4GSCnt_Material) if (!group.getEntryInfo(C4CFN_Material)) Contents = Contents & ~C4GSCnt_Material;
	if (Contents & C4GSCnt_Music) if (!group.getEntryInfo(C4CFN_Music)) Contents = Contents & ~C4GSCnt_Music;
	if (Contents & C4GSCnt_Definitions) if (!group.getEntryInfo(C4CFN_DefFiles)) Contents = Contents & ~C4GSCnt_Definitions;
	if (Contents & C4GSCnt_FontDefs) if (!group.getEntryInfo( C4CFN_FontFiles)) if (!group.getEntryInfo(C4CFN_FontDefs)) Contents = Contents & ~C4GSCnt_FontDefs;
	// return it
	return Contents;
}

bool C4GroupSet::RegisterGroups(C4GroupSet &rCopy, int32_t Contents, const char *szFilename, int32_t iMaxSkipID)
{
	// get all groups of rCopy
	int32_t Contents2;
	for (C4GroupSetNode *pNode = rCopy.pFirst; pNode; pNode = pNode->pNext)
	{
		if (Contents2 = pNode->Contents & Contents)
		{
			if (pNode->id > iMaxSkipID)
			{
				if (!szFilename)
				{
					// add group but don't check the content again!
					RegisterGroup(*pNode->pGroup, false, pNode->Priority, Contents2, false);
				}
				else
				{
					// if a filename is given, open the child group
					if (auto group = pNode->pGroup->openAsChild(szFilename); group)
					{
						// add the child group to the local list; contents equal Contents2
						// but this flag is not likely to be used
						RegisterGroup(*(new CppC4Group{std::move(*group)}), true, pNode->Priority, Contents2, false);
					}
				}
			}
		}
	}
	// done, success
	return true;
}

CppC4Group * C4GroupSet::FindGroup(int32_t Contents, CppC4Group *pAfter, bool fSamePrio)
{
	// get priority
	int32_t iPriority = -1;
	// find group by matching content mask
	for (C4GroupSetNode *pNode = pFirst; pNode; pNode = pNode->pNext)
	{
		// check contents
		if (!pAfter && (pNode->Contents & Contents))
			// check priority
			if (iPriority == -1 || iPriority == pNode->Priority)
				// success, found an entry
				return pNode->pGroup;
		// find next clear flag
		if (pNode->pGroup == pAfter) { pAfter = nullptr; if (fSamePrio) iPriority = pNode->Priority; }
	}
	// nothing found
	return nullptr;
}

std::tuple<CppC4Group *, std::string> C4GroupSet::FindEntry(const char *szWildcard, int32_t *pPriority, int32_t *pID)
{
	// find group that has this entry
	for (C4GroupSetNode *pNode = pFirst; pNode; pNode = pNode->pNext)
	{
		auto infos = pNode->pGroup->getEntryInfos();
		for (const auto &info : *infos)
		{
			if (WildcardMatch(szWildcard, info.fileName.c_str()))
			{
				// assign priority and ID, if ptrs is given
				if (pPriority) *pPriority = pNode->Priority;
				if (pID) *pID = pNode->id;
				// return found group
				return {pNode->pGroup, info.fileName};
			}
		}

	}
	// nothing found
	return {};
}

bool C4GroupSet::LoadEntryString(const char *szEntryName, StdStrBuf &rBuf)
{
	// Load the entry from the first group that has it
	if (auto [group, fileName] = FindEntry(szEntryName); group)
	{
		return CppC4Group_LoadEntryString(*group, fileName, rBuf);
	}
	// Didn't find it
	return false;
}

bool C4GroupSet::CloseFolders()
{
	// close everything that has folder-priority
	for (C4GroupSetNode *pNode = pFirst, *pNext; pNode; pNode = pNext)
	{
		// get next, as pNode might be destroyed
		pNext = pNode->pNext;
		// check if priority matches
		if (Inside<int32_t>(pNode->Priority, C4GSPrio_Folder, C4GSPrio_Folder2) || pNode->Priority == C4GSPrio_Scenario)
			// clear it!
			delete pNode;
	}
	// done, success
	return true;
}

CppC4Group *C4GroupSet::GetGroup(int32_t iIndex)
{
	// Invalid index
	if (iIndex < 0)
		return nullptr;
	// Find indicated group
	for (C4GroupSetNode *pNode = pFirst; pNode; pNode = pNode->pNext)
		if (iIndex == 0)
			return pNode->pGroup;
		else
			iIndex--;
	// Indicated group not found
	return nullptr;
}

#ifdef C4ENGINE
CppC4Group *C4GroupSet::RegisterParentFolders(const char *szScenFilename)
{
	// the scenario filename may be a scenario or directly a group folder
	CppC4Group *parentGroup = nullptr; bool fParentC4F;
	char parentFolder[_MAX_PATH + 1];

	if (SEqualNoCase(GetExtension(szScenFilename), "c4f"))
	{
		fParentC4F = true;
		SCopy(szScenFilename, parentFolder, _MAX_PATH);
	}
	else
	{
		GetParentPath(szScenFilename, parentFolder);
		fParentC4F = SEqualNoCase(GetExtension(parentFolder), "c4f");
	}

	if (fParentC4F)
	{
		std::filesystem::path parent{parentFolder};
		std::vector<std::string> parts;

		// trace back until the file extension is no more .c4f
		while (parent.extension() == ".c4f")
		{
			parts.emplace_back(parent.filename());
			parent = parent.parent_path();
		}

		size_t groupIndex = 0;

		for (auto it = parts.rbegin(); it != parts.rend(); ++it)
		{
			parent /= *it;

			CppC4Group *group = nullptr;
			if (parentGroup)
			{
				if (auto grp = parentGroup->openAsChild(*it); !grp)
				{
					LogFatal(FormatString("%s: %s", LoadResStr("IDS_PRC_FILENOTFOUND"), parent.c_str()).getData());
					return nullptr;
				}
				else
				{
					group = new CppC4Group{std::move(*grp)};
				}
			}
			else if (!(group = new CppC4Group{false})->openExisting(parent))
			{
				LogFatal(FormatString("%s: %s", LoadResStr("IDS_PRC_FILENOTFOUND"), parent.c_str()).getData());
				delete group;
				return nullptr;
			}

			parentGroup = group;

			// add to group set, if this is a true scenario folder
			uint32_t contentsMask = WildcardMatch(C4CFN_FolderFiles, it->c_str()) ? C4GSCnt_Folder : C4GSCnt_Directory;
			if (!RegisterGroup(*group, true, C4GSPrio_Folder + groupIndex++, contentsMask))
			{
				CloseFolders();
				LogFatal("RegGrp: internal error");
				return nullptr;
			}
		}
	}

	return parentGroup;
}
#endif
