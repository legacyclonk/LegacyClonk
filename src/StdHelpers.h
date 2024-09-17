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

// based on boost container_hash's hashCombine
constexpr void HashCombine(std::size_t &hash, std::size_t nextHash)
{
	if constexpr (sizeof(std::size_t) == 4)
	{
#define rotateLeft32(x, r) (x << r) | (x >> (32 - r))
		constexpr std::size_t c1 = 0xcc9e2d51;
		constexpr std::size_t c2 = 0x1b873593;

		nextHash *= c1;
		nextHash = rotateLeft32(nextHash, 15);
		nextHash *= c2;

		hash ^= nextHash;
		hash = rotateLeft32(hash, 13);
		hash = hash * 5 + 0xe6546b64;
#undef rotateLeft32
	}
	else if constexpr (sizeof(std::size_t) == 8)
	{
		constexpr std::size_t m = 0xc6a4a7935bd1e995;
		constexpr int r = 47;

		nextHash *= m;
		nextHash ^= nextHash >> r;
		nextHash *= m;

		hash ^= nextHash;
		hash *= m;

		// Completely arbitrary number, to prevent 0's
		// from hashing to 0.
		hash += 0xe6546b64;
	}
	else
	{
		hash ^= nextHash + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	}
}

template<typename... Args>
constexpr void HashCombineArguments(std::size_t &hash, Args &&...args)
{
	(..., HashCombine(hash, std::hash<std::decay_t<Args>>{}(args)));
}

template<typename... Args>
constexpr std::size_t HashArguments(Args &&...args)
{
	std::size_t result{0};
	(..., HashCombine(result, std::hash<std::decay_t<Args>>{}(args)));
	return result;
}

struct C4TransparentHash
{
	using is_transparent = void;

	template<typename T>
	std::size_t operator()(const T &t) const noexcept(noexcept(std::hash<T>{}(t)))
	{
		return std::hash<T>{}(t);
	}
};
