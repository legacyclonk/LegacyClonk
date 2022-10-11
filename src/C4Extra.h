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

#pragma once

#include <C4Group.h>

#include <memory>
#include <vector>

class C4Extra
{
public:
	C4Extra() { Default(); }
	~C4Extra() { Clear(); }
	void Default(); // zero fields
	void Clear(); // free class members

	void Init(); // init extra group, using scneario presets
	bool InitGroup(); // open extra group

	std::vector<std::unique_ptr<C4Group>> ExtraGroups;

protected:
	bool LoadDef(C4Group &hGroup, const char *szName); // load preset for definition
};
