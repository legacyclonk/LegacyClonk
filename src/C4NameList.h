/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* A static list of strings and integer values, i.e. for material amounts */

#pragma once

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
