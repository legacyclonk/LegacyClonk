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

#include <Standard.h>
#include <StdResStr2.h>

#include <iostream>
#include <optional>
#include <string_view>
#include <unordered_map>

class ResTable
{
public:
	ResTable(std::string_view table)
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

	const char *GetEntry(const std::string& key) const
	{
		if (const auto it = entries.find(key); it != entries.end())
		{
			return it->second.c_str();
		}
		return nullptr;
	}

private:
	std::unordered_map<std::string, std::string> entries;
};

static std::optional<ResTable> Table;

void SetResStrTable(const char *pTable)
{
	Table.emplace(pTable);
}

void ClearResStrTable()
{
	Table.reset();
}

bool IsResStrTableLoaded() { return Table.has_value(); }

static std::string result;
static const char *GetResStr(const char *id, const std::optional<ResTable>& Table)
{
	if (!Table.has_value()) return "Language string table not loaded.";
	const char *r = Table->GetEntry(id);
	if (!r)
	{
		result = "[Undefined:";
		result += id;
		result += ']';
		return result.c_str();
	}
	return r;
}

const char *LoadResStr(const char *id)
{
	return GetResStr(id, Table);
}

const char *LoadResStrNoAmp(const char *id)
{
	result = LoadResStr(id);
	result.erase(std::remove(result.begin(), result.end(), '&'), result.end());
	return result.c_str();
}

const char *GetResStr(const char *id, const char *strTable)
{
	result = GetResStr(id, {ResTable{strTable}});
	return result.c_str();
}
