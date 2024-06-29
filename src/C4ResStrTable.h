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
#include <utility>

#include <fmt/printf.h>

template<typename... Args>
struct C4ResStrTableKeyFormat
{
	consteval C4ResStrTableKeyFormat(const C4ResStrTableKey id) : Id{id}
	{
		if (sizeof...(Args) != C4ResStrTableKeyFormatStringArgsCount[std::to_underlying(id)])
		{
			throw id;
		}
	}

	C4ResStrTableKey Id;
};

const char *LoadResStrV(C4ResStrTableKey id);
std::string LoadResStrNoAmpV(C4ResStrTableKey id);

template<typename... Args>
std::string LoadResStr(const C4ResStrTableKeyFormat<std::type_identity_t<Args>...> id, Args &&...args)
{
	return fmt::sprintf(LoadResStrV(id.Id), std::forward<Args>(args)...);
}

inline const char *LoadResStr(const C4ResStrTableKeyFormat<> id)
{
	return LoadResStrV(id.Id);
}

template<typename... Args>
std::string LoadResStrChoice(const bool condition, const C4ResStrTableKeyFormat<std::type_identity_t<Args>...> ifTrue, const C4ResStrTableKeyFormat<std::type_identity_t<Args>...> ifFalse, Args &&...args)
{
	return fmt::sprintf(LoadResStrV(condition ? ifTrue.Id : ifFalse.Id), std::forward<Args>(args)...);
}

inline const char *LoadResStrChoice(const bool condition, const C4ResStrTableKeyFormat<> ifTrue, const C4ResStrTableKeyFormat<> ifFalse)
{
	return LoadResStrV(condition ? ifTrue.Id : ifFalse.Id);
}

template<typename... Args>
std::string LoadResStrNoAmp(const C4ResStrTableKeyFormat<std::type_identity_t<Args>...> id, Args &&...args)
{
	return fmt::sprintf(LoadResStrNoAmpV(id.Id), std::forward<Args>(args)...);
}

inline std::string LoadResStrNoAmp(const C4ResStrTableKeyFormat<> id)
{
	return LoadResStrNoAmpV(id.Id);
}

template<typename... Args>
std::string LoadResStrNoAmpChoice(const bool condition, const C4ResStrTableKeyFormat<std::type_identity_t<Args>...> ifTrue, const C4ResStrTableKeyFormat<std::type_identity_t<Args>...> ifFalse, Args &&...args)
{
	return fmt::sprintf(LoadResStrNoAmpV(condition ? ifTrue.Id : ifFalse.Id), std::forward<Args>(args)...);
}

inline std::string LoadResStrNoAmpChoice(const bool condition, const C4ResStrTableKeyFormat<> ifTrue, const C4ResStrTableKeyFormat<> ifFalse)
{
	return LoadResStrNoAmpV(condition ? ifTrue.Id : ifFalse.Id);
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
