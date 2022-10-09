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

/* A static list of strings and integer values, i.e. for material amounts */

#pragma once

#include "C4Constants.h"
#include "C4ForwardDeclarations.h"

#include <cstring>

class C4NameList
{
public:
	C4NameList();

public:
	char Name[C4MaxNameList][C4MaxName + 1];
	int32_t Count[C4MaxNameList];

public:
	void Clear();

public:
	bool operator==(const C4NameList &rhs)
	{
		return std::memcmp(this, &rhs, sizeof(C4NameList)) == 0;
	}

	void CompileFunc(StdCompiler *pComp, bool fValues = true);
};
