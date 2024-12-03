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

#include "C4WinRT.h"
#include "StdStringEncodingConverter.h"

#include <stdexcept>
#include <string>

std::wstring StdStringEncodingConverter::WinAcpToUtf16(const std::string_view multiByte)
{
	if (multiByte.empty())
	{
		return L"";
	}

	std::wstring result;
	result.resize_and_overwrite(MultiByteToWideChar(CP_ACP, multiByte, {}), [&multiByte](wchar_t *const ptr, const std::size_t size)
	{
		return MultiByteToWideChar(CP_ACP, multiByte, {ptr, size});
	});

	return result;
}

std::string StdStringEncodingConverter::Utf16ToWinAcp(const std::wstring_view wide)
{
	if (wide.empty())
	{
		return "";
	}

	std::string result;
	result.resize_and_overwrite(WideCharToMultiByte(CP_ACP, wide, {}), [&wide](char *const ptr, const std::size_t size)
	{
		return WideCharToMultiByte(CP_ACP, wide, {ptr, size});
	});

	return result;
}

std::size_t StdStringEncodingConverter::MultiByteToWideChar(const std::uint32_t codePage, const std::span<const char> input, const std::span<wchar_t> output)
{
	if (!std::in_range<int>(input.size()))
	{
		throw std::runtime_error{"Input size out of range"};
	}

	if (!std::in_range<int>(output.size()))
	{
		throw std::runtime_error{"Output size out of range"};
	}

	const int result{::MultiByteToWideChar(codePage, 0, input.data(), static_cast<int>(input.size()), output.data(), static_cast<int>(output.size()))};
	if (!result)
	{
		if (const auto error = GetLastError())
		{
			MapHResultError([error] { winrt::throw_hresult(HRESULT_FROM_WIN32(error)); });
		}
	}

	return static_cast<std::size_t>(result);
}

std::size_t StdStringEncodingConverter::WideCharToMultiByte(const std::uint32_t codePage, const std::span<const wchar_t> input, const std::span<char> output)
{
	if (!std::in_range<int>(input.size()))
	{
		throw std::runtime_error{"Input size out of range"};
	}

	if (!std::in_range<int>(output.size()))
	{
		throw std::runtime_error{"Output size out of range"};
	}

	const int result{::WideCharToMultiByte(codePage, 0, input.data(), static_cast<int>(input.size()), output.data(), static_cast<int>(output.size()), nullptr, nullptr)};
	if (!result)
	{
		if (const auto error = GetLastError())
		{
			MapHResultError([error] { winrt::throw_hresult(HRESULT_FROM_WIN32(error)); });
		}
	}

	return static_cast<std::size_t>(result);
}
