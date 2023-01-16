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

#include "StdStringEncodingConverter.h"

#include <memory>
#include <stdexcept>
#include <string>

#include <fmt/format.h>

namespace
{
	template<typename T>
	struct ConversionFuncHelper;

	template<typename Ret, typename... Args>
	struct ConversionFuncHelper<Ret(__stdcall *)(Args...)>
	{
		using ReturnType = Ret;
		using ArgumentTypes = std::tuple<Args...>;
		using InputType = std::remove_const_t<std::remove_pointer_t<std::tuple_element_t<2, ArgumentTypes>>>;
		using OutputType = std::remove_pointer_t<std::tuple_element_t<4, ArgumentTypes>>;
	};

	template<auto ConversionFunc, typename... Args>
	auto Convert(const std::basic_string_view<typename ConversionFuncHelper<decltype(ConversionFunc)>::InputType> input, Args &&...args)
	{
		using ReturnType = std::basic_string<typename ConversionFuncHelper<decltype(ConversionFunc)>::OutputType>;

		if (input.empty()) return ReturnType{};
		if (std::cmp_greater(input.size(), std::numeric_limits<int>::max()))
		{
			throw std::out_of_range{"Input size out of range"};
		}

		const int convertedSize{ConversionFunc(CP_ACP, 0, input.data(), static_cast<int>(input.size()), nullptr, 0, std::forward<Args>(args)...)};
		if (!convertedSize)
		{
			throw std::runtime_error{fmt::format("Querying output size failed: {:x}", GetLastError())};
		}

		const auto converted = std::make_unique_for_overwrite<typename ReturnType::value_type[]>(static_cast<std::size_t>(convertedSize));
		const int result{ConversionFunc(CP_ACP, 0, input.data(), static_cast<int>(input.size()), converted.get(), convertedSize, std::forward<Args>(args)...)};

		if (result != convertedSize)
		{
			throw std::runtime_error{fmt::format("Conversion returned {} when it was expected to return {}", result, convertedSize)};
		}

		return ReturnType{converted.get(), static_cast<std::size_t>(convertedSize)};
	}
}

std::wstring StdStringEncodingConverter::WinAcpToUtf16(const std::string_view multiByte)
{
	return Convert<MultiByteToWideChar>(multiByte);
}

std::string StdStringEncodingConverter::Utf16ToWinAcp(const std::wstring_view wide)
{
	return Convert<WideCharToMultiByte>(wide, nullptr, nullptr);
}
