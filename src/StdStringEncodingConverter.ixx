/*
 * LegacyClonk
 *
 * Copyright (c) 2017-2023, The LegacyClonk Team and contributors
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

module;

#include <string>
#include <string_view>

export module StdStringEncodingConverter;

export class StdStringEncodingConverter
{
public:
	static std::wstring WinAcpToUtf16(std::string_view multiByte);
	static std::string Utf16ToWinAcp(std::wstring_view wide);
};
