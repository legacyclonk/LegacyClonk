/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
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

#include "C4Application.h"
#include "C4ResStrTable.h"

#include <Standard.h>

#include <format>
#include <optional>

C4ResStrTable::C4ResStrTable(const std::string_view code, std::string_view table)
{
	auto keyStringMap = GetKeyStringMap();

	for (auto pos = table.find_first_of('='); pos != std::string_view::npos; pos = table.find_first_of('='))
	{
		const auto key = table.substr(0, pos);
		table.remove_prefix(pos + 1);

		const auto endPos = table.find_first_not_of("\r\n", table.find_first_of("\r\n"));
		auto value = table.substr(0, endPos);
		table.remove_prefix(value.size());
		value = value.substr(0, value.find_last_not_of("\r\n") + 1);

		if (const auto it = keyStringMap.find(key); it != keyStringMap.end())
		{
			std::string &valueStr{entries[static_cast<std::size_t>(it->second)] = value};
			keyStringMap.erase(it);

			for (auto backslashPos = valueStr.find_first_of('\\'); backslashPos < valueStr.size() - 1; backslashPos = valueStr.find_first_of('\\', backslashPos + 1))
			{
				if (valueStr[backslashPos + 1] == 'n')
				{
					valueStr.replace(backslashPos, 2, "\r\n");
				}
			}
		}
	}

	for (const auto &[key, value] : keyStringMap)
	{
		if (!code.empty())
		{
			spdlog::warn("Missing entry in string table {} for key {}", code, key);
		}

		entries[static_cast<std::size_t>(value)] = std::format("[Undefined: {}]", key);
	}
}

std::string_view C4ResStrTable::GetEntry(const C4ResStrTableKey key) const
{
	return entries[static_cast<std::size_t>(key)];
}

const char *LoadResStrV(const C4ResStrTableKey id)
{
	if (!Application.ResStrTable.has_value()) return "Language string table not loaded.";
	return Application.ResStrTable->GetEntry(id).data();
}

std::string LoadResStrNoAmpV(const C4ResStrTableKey id)
{
	std::string result{LoadResStrV(id)};
	result.erase(std::remove(result.begin(), result.end(), '&'), result.end());
	return result;
}
