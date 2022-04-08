/*
 * LegacyClonk
 *
 * Copyright (c) 2020-2021, The LegacyClonk Team and contributors
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

#include <concepts>
#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

template<std::integral To, std::integral From>
To checked_cast(From from)
{
	if constexpr (std::is_signed_v<From>)
	{
		if constexpr (std::is_unsigned_v<To>)
		{
			if (from < 0)
			{
				throw std::runtime_error{"Conversion of negative value to unsigned type requested"};
			}
		}
		else if constexpr (std::numeric_limits<From>::min() < std::numeric_limits<To>::min())
		{
			if (from < std::numeric_limits<To>::min())
			{
				throw std::runtime_error{"Conversion of value requested that is smaller than the target-type minimum"};
			}
		}
	}

	if constexpr (std::numeric_limits<From>::max() > std::numeric_limits<To>::max())
	{
		if (std::cmp_greater(from, std::numeric_limits<To>::max()))
		{
			throw std::runtime_error{"Conversion of value requested that is bigger than the target-type maximum"};
		}
	}

	return static_cast<To>(from);
}

template<typename... T>
class StdOverloadedCallable : public T...
{
public:
	StdOverloadedCallable(T... bases) : T{bases}... {}
	using T::operator()...;
};

namespace detail
{
	template <typename Function>
	struct FunctionSingleArgument_s;

	template <typename Return, typename Argument>
	struct FunctionSingleArgument_s<Return(*)(Argument)>
	{
		using type = Argument;
	};

	template <auto function>
	using FunctionSingleArgument = typename FunctionSingleArgument_s<decltype(function)>::type;
}

template <auto function>
struct C4SingleArgumentFunctionFunctor
{
	using Arg = detail::FunctionSingleArgument<function>;

	void operator()(Arg arg)
	{
		std::invoke(function, std::forward<Arg>(arg));
	}
};

template <auto free>
using C4DeleterFunctionUniquePtr = std::unique_ptr<std::remove_pointer_t<detail::FunctionSingleArgument<free>>, C4SingleArgumentFunctionFunctor<free>>;

template<typename Enum> requires std::is_enum_v<Enum>
struct C4BitfieldOperators : std::false_type {};

template<typename Enum>
concept C4BitfieldOperatorsEnabled = C4BitfieldOperators<Enum>::value;

template<C4BitfieldOperatorsEnabled T>
constexpr T operator|(const T lhs, const T rhs) noexcept
{
	return static_cast<T>(std::to_underlying(lhs) | std::to_underlying(rhs));
}

template<C4BitfieldOperatorsEnabled T>
constexpr T operator&(const T lhs, const T rhs) noexcept
{
	return static_cast<T>(std::to_underlying(lhs) & std::to_underlying(rhs));
}

template<C4BitfieldOperatorsEnabled T>
constexpr T& operator|=(T& lhs, const T rhs) noexcept
{
	return lhs = (lhs | rhs);
}

template<C4BitfieldOperatorsEnabled T>
constexpr T& operator&=(T& lhs, const T rhs) noexcept
{
	return lhs = (lhs & rhs);
}
