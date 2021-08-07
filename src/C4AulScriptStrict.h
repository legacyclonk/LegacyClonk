/*
 * LegacyClonk
 *
 * Copyright (c) 2019, The LegacyClonk Team and contributors
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
#include <cstdint>

enum class C4AulScriptStrict : std::uint8_t
{
	NONSTRICT = 0,
	STRICT1 = 1,
	STRICT2 = 2,
	STRICT3 = 3,
	MAXSTRICT = STRICT3
};
