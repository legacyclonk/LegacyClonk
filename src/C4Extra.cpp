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

#ifndef BIG_C4INCLUDE
#include <C4Config.h>
#include <C4Components.h>
#include <C4Game.h>
#include <C4Log.h>
#endif

void C4Extra::Default()
{
	// zero fields
}

void C4Extra::Clear()
{
	// free class members
	ExtraGrp.Close();
}

bool C4Extra::InitGroup()
{
	// exists?
	if (!ItemExists(Config.AtExePath(C4CFN_Extra))) return false;
	// open extra group
	if (!ExtraGrp.Open(Config.AtExePath(C4CFN_Extra))) return false;
	// register extra root into game group set
	Game.GroupSet.RegisterGroup(ExtraGrp, false, C4GSPrio_ExtraRoot, C4GSCnt_ExtraRoot);
	// done, success
	return true;
}

bool C4Extra::Init()
{
	// no group: OK
	if (!ExtraGrp.IsOpen()) return true;
	// load from all definitions that are activated
	// add first definition first, so the priority will be lowest
	// (according to definition load/overload order)
	char szSegment[_MAX_PATH + 1];
	bool fAnythingLoaded = false;
	for (int cseg = 0; SCopySegment(Game.DefinitionFilenames, cseg, szSegment, ';', _MAX_PATH); cseg++)
		if (LoadDef(ExtraGrp, GetFilename(szSegment))) fAnythingLoaded = true;
	// done, success
	return true;
}

bool C4Extra::LoadDef(C4Group &hGroup, const char *szName)
{
	// check if file exists
	if (!hGroup.FindEntry(szName)) return false;
	// log that extra group is loaded
	LogF(LoadResStr("IDS_PRC_LOADEXTRA"), ExtraGrp.GetName(), szName);
	// open and add group to set
	C4Group *pGrp = new C4Group;
	if (!pGrp->OpenAsChild(&hGroup, szName)) { Log(LoadResStr("IDS_ERR_FAILURE")); delete pGrp; return false; }
	Game.GroupSet.RegisterGroup(*pGrp, true, C4GSPrio_Extra, C4GSCnt_Extra);
	// done, success
	return true;
}
