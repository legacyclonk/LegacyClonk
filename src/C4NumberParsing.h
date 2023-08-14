/*
 * LegacyClonk
 *
 * Copyright (c) 2023, The LegacyClonk Team and contributors
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

#include <charconv>
#include <concepts>
#include <format>
#include <stdexcept>
#include <string>
#include <utility>

template <typename T>
struct NumberRangeError : public std::range_error
{
	NumberRangeError(std::string message, const T atRangeLimit) : std::range_error{std::move(message)}, AtRangeLimit{atRangeLimit} {}

	T AtRangeLimit;
};

template <std::integral T>
T ParseNumber(std::string_view s, const unsigned base = 10)
{
	if (s.starts_with('+'))
	{
		s.remove_prefix(1);
	}

	T result{};
	const auto sEnd = s.data() + s.size();
	const auto [end, ec] = std::from_chars(s.data(), sEnd, result, base);
	if (ec == std::errc{} && end == sEnd)
	{
		return result;
	}

	if (ec == std::errc{} && end != sEnd)
	{
		throw std::invalid_argument{std::format("ParseNumber: Unexpected garbage after integer: \"{}\"", std::string_view{end, sEnd})};
	}

	if (ec == std::errc::result_out_of_range)
	{
		using limits = std::numeric_limits<T>;
		const auto limit = (std::signed_integral<T> && s.starts_with('-')) ? limits::min() : limits::max();
		throw NumberRangeError<T>{std::format("ParseNumber: Value out of range: \"{}\"", s), limit};
	}

	throw std::invalid_argument{std::format("ParseNumber: Not a number: \"{}\"", s)};
}
