/*
 * LegacyClonk
 *
 * Copyright (c) 2022, The LegacyClonk Team and contributors
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

#include <array>
#include <ranges>
#include <string_view>
#include <utility>
#include <type_traits>

#include "StdHelpers.h"

enum class C4EnumValueScope
{
	None = 0,
	Script = 0x1,
	Serialization = 0x2,
	Both = Script | Serialization
};

template<>
struct C4BitfieldOperators<C4EnumValueScope> : std::true_type {};

template<typename Enum>
struct C4EnumValue
{
	Enum value{};
	std::string_view name{};
	std::string_view scriptName{};
	C4EnumValueScope scope{C4EnumValueScope::Both};

	constexpr C4EnumValue() noexcept = default;

	constexpr C4EnumValue(Enum value, std::string_view name, C4EnumValueScope scope = C4EnumValueScope::Both) noexcept
		: value{value}, name{name}, scriptName{name}, scope{scope} {}

	constexpr C4EnumValue(Enum value, std::string_view name, std::string_view scriptName) noexcept
		: value{value}, name{name}, scriptName{scriptName}, scope{C4EnumValueScope::Both} {}
};

template<typename Enum, std::size_t N>
struct C4EnumInfoData
{
	std::string_view prefix;
	std::array<C4EnumValue<Enum>, N> values;

	constexpr C4EnumInfoData(std::string_view prefix, const C4EnumValue<Enum> (&values)[N]) noexcept : C4EnumInfoData{prefix, values, std::make_index_sequence<N>()} {}

	constexpr auto scopedValues(C4EnumValueScope scope) const
	{
		return values | std::views::filter([scope](const C4EnumValue<Enum>& value) constexpr
		{
			return (value.scope & scope) != C4EnumValueScope::None;
		});
	}

private:
	template<std::size_t... indices>
	constexpr C4EnumInfoData(std::string_view prefix, const C4EnumValue<Enum> (&values)[N], std::index_sequence<indices...>) noexcept : prefix{prefix}, values{values[indices]...} {}
};

template<typename Enum, std::size_t N>
constexpr auto mkEnumInfo(std::string_view prefix, const C4EnumValue<Enum> (&values)[N]) noexcept { return C4EnumInfoData<Enum, N>{prefix, values}; }

// separate type so the bitfield adapter can’t be used on non-bitfields
template<typename Enum, std::size_t N>
struct C4BitfieldInfoData : C4EnumInfoData<Enum, N>
{
	using C4EnumInfoData<Enum, N>::C4EnumInfoData;
};

template<typename Enum, std::size_t N>
constexpr auto mkBitfieldInfo(std::string_view prefix, const C4EnumValue<Enum> (&values)[N]) noexcept { return C4BitfieldInfoData<Enum, N>{prefix, values}; }

template<typename Enum>
struct C4EnumInfo;
