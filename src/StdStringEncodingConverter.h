/*
 * LegacyClonk
 *
 * Copyright (c) 2017, The LegacyClonk Team and contributors
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

#ifdef WIN32

#include "C4Windows.h"
#include <string>

class StdStringEncodingConverter
{
public:
	/* Converts a string encoded in the system default ACP to UTF-16.
	   If "last" is nullptr, the string specified by "first" must be null-terminated. */
	std::wstring WinAcpToUtf16(LPCCH first, LPCCH last = nullptr) const;

	/* Converts a string encoded in UTF-16 to the system default ACP.
	   If "last" is nullptr, the string specified by "first" must be null-terminated. */
	std::string Utf16ToWinAcp(LPCWCH first, LPCWCH last = nullptr) const;
};

#endif
