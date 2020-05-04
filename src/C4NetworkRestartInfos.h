/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2020, The LegacyClonk Team and contributors
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

#pragma once

#include "C4Constants.h"

#include <map>
#include <string>
#include <type_traits>

namespace C4NetworkRestartInfos
{
enum RestoreInfo
{
	None = 0,
	ScriptPlayers = 0x1, // includes teams of script players
	PlayerTeams = 0x2 // for normal players
};

struct Player
{
	const std::string name;
	const C4PlayerType type;
	const int32_t team;
	const uint32_t color;

	Player(const std::string& name, const C4PlayerType type, const uint32_t color, const int32_t team) : name{name}, type{type}, team{team}, color{color} { }
};

struct Infos
{
	std::underlying_type_t<RestoreInfo> What{None};
	std::map<std::string, Player> Players;
	void Clear()
	{
		*this = Infos{};
	}
};
}
