/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2020, The LegacyClonk Team and contributors
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

/* Load strings from a primitive memory string table */

#pragma once

#include <string_view>
#include <unordered_map>

class StdResTable
{
public:
	StdResTable(std::string_view table);

public:
	const char *GetResStr(const char *id);

private:
	struct Hash : std::hash<std::string_view>
	{
		using is_transparent = int;
	};

	std::unordered_map<std::string, std::string, Hash, std::equal_to<void>> entries;
};

const char *LoadResStr(const char *id);
const char *LoadResStrNoAmp(const char *id);

void SetResStrTable(const char *pTable);
void ClearResStrTable();
bool IsResStrTableLoaded();
