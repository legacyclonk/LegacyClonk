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

/* 32-bit value to identify object definitions */

#pragma once

#include "Standard.h"
#include "StdAdaptors.h"

#include <string>
#include <string_view>

// Use 64 Bit for C4ID (on x86_64) to pass 64 bit for each script function
// parameter
using C4ID = unsigned long;

constexpr C4ID C4Id(const std::string_view str)
{
	using namespace std::string_view_literals;
	// longer is allowed for backward compatibility reasons, as this is used by the C4Id script function
	if (str.size() < 4 || str == "NONE"sv)
	{
		return 0;
	}

	C4ID id{0};
	bool numeric = true;
	for (const auto c : str)
	{
		if (!Inside(c, '0', '9'))
		{
			numeric = false;
			break;
		}

		id *= 10;
		id += c - '0';
	}

	if (numeric)
	{
		return id;
	}
	else
	{
		id = 0;
	}

	for (std::size_t i = 4; i > 0; --i)
	{
		id <<= 8;
		id |= static_cast<C4ID>(str[i - 1]);
	}
	return id;
}

constexpr bool LooksLikeID(const std::string_view str)
{
	if (str.size() != 4)
	{
		return false;
	}

	for (const auto c : str)
	{
		if (!(Inside(c, 'A', 'Z') || Inside(c, '0', '9') || (c == '_')))
		{
			return false;
		}
	}
	return true;
}

constexpr C4ID operator""_id(const char *str, std::size_t n)
{
	using namespace std::string_literals;
	const std::string_view idStr{str, n};

	if (n != 4)
	{
		throw std::runtime_error{"Invalid ID: "s + str + "; Must be 4 characters long."s};
	}

	if (!LooksLikeID(idStr))
	{
		throw std::runtime_error{"Invalid character in ID: "s + str + "; Only A-Z, 0-9 and _ are allowed."s};
	}

	return C4Id(idStr);
}

constexpr C4ID C4ID_None         = "NONE"_id,
               C4ID_Clonk        = "CLNK"_id,
               C4ID_Flag         = "FLAG"_id,
               C4ID_Conkit       = "CNKT"_id,
               C4ID_Gold         = "GOLD"_id,
               C4ID_Meteor       = "METO"_id,
               C4ID_Linekit      = "LNKT"_id,
               C4ID_PowerLine    = "PWRL"_id,
               C4ID_SourcePipe   = "SPIP"_id,
               C4ID_DrainPipe    = "DPIP"_id,
               C4ID_Energy       = "ENRG"_id,
               C4ID_CnMaterial   = "CNMT"_id,
               C4ID_FlagRemvbl   = "FGRV"_id,
               C4ID_TeamHomebase = "THBA"_id,
               C4ID_Contents     = 0x00002710; // 10000

void GetC4IdText(C4ID id, char *sBuf);
const char *C4IdText(C4ID id);
bool LooksLikeID(C4ID id);

// * C4ID Adaptor
struct C4IDAdapt
{
	C4ID &rValue;

	explicit C4IDAdapt(C4ID &rValue) : rValue(rValue) {}

	inline void CompileFunc(StdCompiler *pComp) const
	{
		char cC4ID[5];
		if (pComp->isDecompiler())
			GetC4IdText(rValue, cC4ID);

		pComp->Value(mkStringAdapt(cC4ID, 4, StdCompiler::RCT_ID));

		if (pComp->isCompiler())
		{
			if (strlen(cC4ID) != 4)
				rValue = C4ID_None;
			else
				rValue = C4Id(cC4ID);
		}
	}

	// Operators for default checking/setting
	inline bool operator==(const C4ID &nValue) const { return rValue == nValue; }
	inline C4IDAdapt &operator=(const C4ID &nValue) { rValue = nValue; return *this; }
};

inline C4IDAdapt mkC4IDAdapt(C4ID &rValue) { return C4IDAdapt(rValue); }
