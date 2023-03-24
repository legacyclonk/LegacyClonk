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

#include <cstddef>
#include <cstdint>
#include <type_traits>

class C4Object;
class StdCompiler;

class C4EnumeratedObjectPtr
{
public:
	using Enumerated = std::int32_t;

private:
	C4Object *object{};
	Enumerated number{};

public:
	C4EnumeratedObjectPtr() = default;
	C4EnumeratedObjectPtr(const C4EnumeratedObjectPtr &) = default;
	C4EnumeratedObjectPtr(C4EnumeratedObjectPtr &&) = default;
	explicit C4EnumeratedObjectPtr(C4Object *object) : object{object} {}

	constexpr C4Object *operator->() const noexcept { return Object(); }
	constexpr C4Object *operator*() const noexcept { return Object(); }
	constexpr C4Object *Object() const noexcept { return object; }
	constexpr Enumerated Number() const noexcept { return number; }
	constexpr operator C4Object *() const noexcept { return Object(); }

	C4EnumeratedObjectPtr &operator=(const C4EnumeratedObjectPtr &) = default;
	C4EnumeratedObjectPtr &operator=(C4EnumeratedObjectPtr &&) = default;
	constexpr C4EnumeratedObjectPtr &operator=(C4Object *obj) noexcept
	{
		object = obj;
		return *this;
	}
	constexpr C4EnumeratedObjectPtr &operator=(std::nullptr_t) noexcept
	{
		Reset();
		return *this;
	}

	constexpr void Reset() noexcept
	{
		object = nullptr;
		number = {};
	}
	constexpr void SetNumber(Enumerated enumerated) noexcept
	{
		number = enumerated;
	}

	void Enumerate();
	void Denumerate();

	void CompileFunc(StdCompiler *compiler, bool intPack = false);
};

namespace
{
	template<typename... Args>
	constexpr void CheckArgs(Args &...args) noexcept
	{
		static_assert(sizeof...(args) > 0, "At least one argument is required");
		static_assert((std::is_same_v<Args, C4EnumeratedObjectPtr> && ...), "Only C4EnumeratedObjectPtr& can be passed as arguments");
	}
}

template<typename... Args>
void EnumerateObjectPtrs(Args &...args)
{
	CheckArgs(args...);
	(args.Enumerate(), ...);
}

template<typename... Args>
void DenumerateObjectPtrs(Args &...args)
{
	CheckArgs(args...);
	(args.Denumerate(), ...);
}
