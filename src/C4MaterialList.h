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

/* A primitive list to store one amount value per mapped material */

#pragma once

#include <C4Landscape.h>

class C4MaterialList
{
public:
	C4MaterialList();
	~C4MaterialList();

public:
	int32_t Amount[C4MaxMaterial];

public:
	void Default();
	void Clear();
	void Reset();
	void Add(int32_t iMaterial, int32_t iAmount);
};
