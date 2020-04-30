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

// user-customizable multimedia package Extra.c4g

#include <C4Include.h>
#include <C4Extra.h>

#include <C4Config.h>
#include <C4Components.h>
#include <C4Game.h>
#include <C4Log.h>

void C4Extra::Default()
{
	// zero fields
}

void C4Extra::Clear()
{
	// free class members
	delete ExtraGrp;
	ExtraGrp = nullptr;
}

bool C4Extra::InitGroup()
{
	// exists?
	if (!ItemExists(Config.AtExePath(C4CFN_Extra))) return false;
	// open extra group
	ExtraGrp = new CppC4Group;
	if (!ExtraGrp->openExisting(Config.AtExePath(C4CFN_Extra)))
	{
		Clear();
		return false;
	}

	// register extra root into game group set
	Game.GroupSet.RegisterGroup(*ExtraGrp, "", false, C4GSPrio_ExtraRoot, C4GSCnt_ExtraRoot);
	// done, success
	return true;
}

bool C4Extra::Init()
{
	// no group: OK
	if (!ExtraGrp) return true;
	// load from all definitions that are activated
	// add first definition first, so the priority will be lowest
	// (according to definition load/overload order)
	bool fAnythingLoaded = false;
	for (const auto &def : Game.DefinitionFilenames)
	{
		if (LoadDef(GetFilename(def.c_str())))
		{
			fAnythingLoaded = true;
		}
	}
	// done, success
	return true;
}

bool C4Extra::LoadDef(const char *szName)
{
	// check if file exists
	if (!ExtraGrp->getEntryData(szName)) return false;

	// log that extra group is loaded
	LogF(LoadResStr("IDS_PRC_LOADEXTRA"), C4CFN_Extra, szName);

	// open and add group to set
	if (auto grp = ExtraGrp->openAsChild(szName); grp)
	{
		auto *group = new CppC4Group{std::move(*grp)};
		Game.GroupSet.RegisterGroup(*group, "", true, C4GSPrio_Extra, C4GSCnt_Extra);
	}
	else
	{
		Log(LoadResStr("IDS_ERR_FAILURE"));
		return false;
	}
	// done, success
	return true;
}
