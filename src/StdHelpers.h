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
#include <iterator>
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


namespace detail
{
	template<typename T>
	struct PointerToMember;

	template<typename F, typename C>
	struct PointerToMember<F C::*>
	{
		using FieldType = F;
		using ClassType = C;
	};
}

template<auto Member>
class C4LinkedListIterator
{
public:
	using iterator_category = std::forward_iterator_tag;
	using value_type = typename detail::PointerToMember<decltype(Member)>::ClassType;
	using difference_type = std::ptrdiff_t;
	using pointer = value_type *;
	using reference = value_type &;

public:
	C4LinkedListIterator(const pointer value = nullptr) noexcept : value{value} {}

public:
	C4LinkedListIterator &operator++() noexcept { value = value->*Member; return *this; }
	C4LinkedListIterator operator++(int) noexcept { C4LinkedListIterator iterator{*this}; ++(*this); return iterator; }

	bool operator==(const C4LinkedListIterator &other) const noexcept { return value == other.value; }
	bool operator==(std::default_sentinel_t) const noexcept { return !value; }

	reference operator*() const noexcept { return *value; }
	pointer operator->() const noexcept { return value; }

private:
	pointer value;
};

template<auto Member>
[[nodiscard]] inline C4LinkedListIterator<Member> begin(const C4LinkedListIterator<Member> iter) noexcept
{
	return iter;
}

template<auto Member>
[[nodiscard]] inline C4LinkedListIterator<Member> end(const C4LinkedListIterator<Member>) noexcept
{
	return {};
}
