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

#include <Standard.h>
#include <StdStringEncodingConverter.h>

#include "C4Windows.h"

#include <memory>
#include <stdexcept>
#include <string>

// Calls MultiByteToWideChar and throws an exception if it failed.
static int CallMultiByteToWideChar(
	const LPCCH lpMultiByteStr, const int cbMultiByte,
	const LPWSTR lpWideCharStr, const int cchWideChar)
{
	const auto result = MultiByteToWideChar(CP_ACP, 0,
		lpMultiByteStr, cbMultiByte,
		lpWideCharStr, cchWideChar);

	if (result == 0)
	{
		const DWORD errorNumber{GetLastError()};
		throw std::runtime_error{std::string() +
			"MultiByteToWideChar failed (error " + std::to_string(errorNumber) + ")"};
	}

	return result;
}

// Calls WideCharToMultiByte and throws an exception if it failed.
static int CallWideCharToMultiByte(
	const LPCWSTR lpWideCharStr, const int cchWideChar,
	const LPCH lpMultiByteStr, const int cbMultiByte)
{
	const auto result = WideCharToMultiByte(CP_ACP, 0,
		lpWideCharStr, cchWideChar,
		lpMultiByteStr, cbMultiByte,
		nullptr, nullptr);

	if (result == 0)
	{
		const DWORD errorNumber{GetLastError()};
		throw std::runtime_error(std::string{} +
			"WideCharToMultiByte failed (error " + std::to_string(errorNumber) + ")");
	}

	return result;
}

std::wstring StdStringEncodingConverter::WinAcpToUtf16(const LPCCH first, const LPCCH last) const
{
	// Get length of source string
	const auto sourceLen = (last ? last - first : lstrlenA(first));
	// Don't use MultiByteToWideChar if source string is empty
	if (sourceLen == 0) return {};

	// Get length of converted string and create array for it
	const auto convertedLen = CallMultiByteToWideChar(first, sourceLen, nullptr, 0);
	const auto converted = std::make_unique<wchar_t[]>(convertedLen);

	// Convert
	const auto resultMBTWC = CallMultiByteToWideChar(
		first, sourceLen, converted.get(), convertedLen);
	if (resultMBTWC != convertedLen)
	{
		throw std::runtime_error(std::string{} +
			"MultiByteToWideChar returned " + std::to_string(resultMBTWC) +
			" when it was expected to return " + std::to_string(convertedLen));
	}

	// Create wstring from array
	return std::wstring{converted.get(), static_cast<std::size_t>(convertedLen)};
}

std::string StdStringEncodingConverter::Utf16ToWinAcp(LPCWCH first, LPCWCH last) const
{
	// Get length of source string
	const auto sourceLen = (last ? last - first : lstrlenW(first));
	// Don't use MultiByteToWideChar if source string is empty
	if (sourceLen == 0) return {};

	// Get length of converted string and create array for it
	const auto convertedLen = CallWideCharToMultiByte(first, sourceLen, nullptr, 0);
	const auto converted = std::make_unique<char[]>(convertedLen);

	// Convert
	const auto resultWCTMB = CallWideCharToMultiByte(
			first, sourceLen, converted.get(), convertedLen);
	if (resultWCTMB != convertedLen)
	{
		throw std::runtime_error(std::string{} +
			"WideCharToMultiByte returned " + std::to_string(resultWCTMB) +
			" when it was expected to return " + std::to_string(convertedLen));
	}

	// Create wstring from array
	return std::string{converted.get(), static_cast<std::size_t>(convertedLen)};
}
