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

#pragma once

#include <C4Group.h>

class C4Extra
{
public:
	C4Extra() { Default(); };
	~C4Extra() { Clear(); };
	void Default(); // zero fields
	void Clear(); // free class members

	bool Init(); // init extra group, using scneario presets
	bool InitGroup(); // open extra group

	C4Group ExtraGrp; // extra.c4g root folder

protected:
	bool LoadDef(C4Group &hGroup, const char *szName); // load preset for definition
};
