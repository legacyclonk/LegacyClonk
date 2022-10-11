/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
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

// user-customizable multimedia package Extra.c4g

#include <C4Include.h>
#include <C4Extra.h>

#include <C4Config.h>
#include <C4Components.h>
#include <C4Game.h>
#include <C4Log.h>
#include "C4Wrappers.h"

void C4Extra::Default()
{
	// zero fields
}

void C4Extra::Clear()
{
	// free class members
	ExtraGroups.clear();
}

bool C4Extra::InitGroup()
{
	for (const auto &pathInfo : Reloc)
	{
		auto group = std::make_unique<C4Group>();
		if (group->Open((pathInfo.Path / C4CFN_Extra).string().c_str()))
		{
			ExtraGroups.emplace_back(std::move(group));
		}
	}

	// done, success
	return true;
}

void C4Extra::Init()
{
	// no group: OK
	if (ExtraGroups.empty()) return;
	// load from all definitions that are activated
	// add first definition first, so the priority will be lowest
	// (according to definition load/overload order)
	bool fAnythingLoaded = false;
	for (const auto &def : Game.DefinitionFilenames)
	{
		for (const auto &group : ExtraGroups)
		{
			if (LoadDef(*group.get(), GetFilename(def.c_str())))
			{
				fAnythingLoaded = true;
				break;
			}
		}
	}
}

bool C4Extra::LoadDef(C4Group &hGroup, const char *szName)
{
	// check if file exists
	if (!hGroup.FindEntry(szName)) return false;
	// log that extra group is loaded
	LogF(LoadResStr("IDS_PRC_LOADEXTRA"), hGroup.GetName(), szName);
	// open and add group to set
	C4Group *pGrp = new C4Group;
	if (!pGrp->OpenAsChild(&hGroup, szName)) { Log(LoadResStr("IDS_ERR_FAILURE")); delete pGrp; return false; }
	Game.GroupSet.RegisterGroup(*pGrp, true, C4GSPrio_Extra, C4GSCnt_Extra);
	// done, success
	return true;
}
