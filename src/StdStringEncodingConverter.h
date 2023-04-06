/*
 * LegacyClonk
 *
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

#pragma once

#ifdef _WIN32

#include "C4Windows.h"

#include <string>
#include <string_view>

class StdStringEncodingConverter
{
public:
	static std::wstring WinAcpToUtf16(std::string_view multiByte);
	static std::string Utf16ToWinAcp(std::wstring_view wide);
};

#endif
