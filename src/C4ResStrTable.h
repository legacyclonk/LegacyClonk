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

#include "generated/C4ResStrTableGenerated.h"

#include <string>
#include <string_view>
#include <unordered_map>

#include <fmt/format.h>

const char *LoadResStr(C4ResStrTableKey id);
std::string LoadResStrNoAmp(C4ResStrTableKey id);

template<typename... Args>
std::string LoadResStr(const C4ResStrTableKey id, Args &&...args)
{
	return fmt::sprintf(LoadResStr(id), std::forward<Args>(args)...);
}

template<typename... Args>
std::string LoadResStrNoAmp(const C4ResStrTableKey id, Args &&...args)
{
	return fmt::sprintf(LoadResStrNoAmp(id), std::forward<Args>(args)...);
}

class C4ResStrTable
{
public:
	C4ResStrTable(std::string_view code, std::string_view table);

public:
	std::string_view GetEntry(C4ResStrTableKey key) const;

private:
	static std::unordered_map<std::string_view, C4ResStrTableKey> GetKeyStringMap();

private:
	std::array<std::string, static_cast<std::size_t>(C4ResStrTableKey::NumberOfEntries)> entries;
};
