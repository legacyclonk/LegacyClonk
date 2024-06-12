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

#include <string>
#include <string_view>
#include <unordered_map>

const char *LoadResStr(const char *id);
const char *LoadResStrNoAmp(const char *id);
const char *GetResStr(const char *id, const char *strTable);

class C4ResStrTable
{
private:
	struct TransparentHash
	{
		using is_transparent = int;

		template<typename T>
		std::size_t operator()(const T &t) const
		{
			return std::hash<T>{}(t);
		}
	};

public:
	C4ResStrTable(std::string_view table);

public:
	const char *GetEntry(std::string_view key) const;

private:
	std::unordered_map<std::string, std::string, TransparentHash, std::equal_to<>> entries;
};
