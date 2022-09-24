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

#include <Standard.h>
#include <StdResStr2.h>

#include <iostream>
#include <optional>

StdResTable::StdResTable(std::string_view table)
{
	for (auto pos = table.find_first_of('='); pos != std::string_view::npos; pos = table.find_first_of('='))
	{
		const auto key = table.substr(0, pos);
		table.remove_prefix(pos + 1);

		const auto endPos = table.find_first_not_of("\r\n", table.find_first_of("\r\n"));
		auto value = table.substr(0, endPos);
		table.remove_prefix(value.size());
		value = value.substr(0, value.find_last_not_of("\r\n") + 1);

		const auto [it, inserted] = entries.emplace(key, value);
		if (!inserted)
		{
			std::cerr << "LanguageXX entry \"" << key << "\" not inserted (duplicate?)\n";
		}
		else
		{
			std::string &valueStr = it->second;
			for (auto backslashPos = valueStr.find_first_of('\\'); backslashPos < valueStr.size() - 1; backslashPos = valueStr.find_first_of('\\', backslashPos + 1))
			{
				if (valueStr[backslashPos + 1] == 'n')
				{
					valueStr.replace(backslashPos, 2, "\r\n");
				}
			}
		}
	}
}

static std::string result;

const char *StdResTable::GetResStr(const char *const id)
{
	const std::string_view key{id};
	if (const auto it = entries.find(key); it != entries.end())
	{
		return it->second.c_str();
	}

	result = std::string{"[Undefined:"} + id + ']';
	return result.c_str();
}

static std::optional<StdResTable> Table;

void SetResStrTable(const char *pTable)
{
	Table.emplace(pTable);
}

void ClearResStrTable()
{
	Table.reset();
}

bool IsResStrTableLoaded() { return Table.has_value(); }

const char *LoadResStr(const char *id)
{
	return Table ? Table->GetResStr(id) : "Language string table not loaded.";
}

const char *LoadResStrNoAmp(const char *id)
{
	result = LoadResStr(id);
	result.erase(std::remove(result.begin(), result.end(), '&'), result.end());
	return result.c_str();
}
