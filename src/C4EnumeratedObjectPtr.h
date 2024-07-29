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

#include "StdAdaptors.h"
#include "StdCompiler.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

class C4Object;
class C4Section;

template<typename Traits>
class C4EnumeratedPtr
{
private:
	using Denum = Traits::Denumerated;
	using Enumerated = Traits::Enumerated;

	Denum *denumerated{};
	Enumerated number{};

public:
	C4EnumeratedPtr() = default;
	C4EnumeratedPtr(const C4EnumeratedPtr &) = default;
	C4EnumeratedPtr(C4EnumeratedPtr &&) = default;
	explicit C4EnumeratedPtr(Denum *denumerated) : denumerated{denumerated} {}

	constexpr Denum *operator->() const noexcept { return Denumerated(); }
	constexpr Denum *operator*() const noexcept { return Denumerated(); }
	constexpr Denum *Denumerated() const noexcept { return denumerated; }
	constexpr Enumerated Number() const noexcept { return number; }
	constexpr operator Denum *() const noexcept { return Denumerated(); }

	C4EnumeratedPtr &operator=(const C4EnumeratedPtr &) = default;
	C4EnumeratedPtr &operator=(C4EnumeratedPtr &&) = default;
	constexpr C4EnumeratedPtr &operator=(Denum *d) noexcept
	{
		SetDenumerated(d);
		return *this;
	}
	constexpr C4EnumeratedPtr &operator=(std::nullptr_t) noexcept
	{
		Reset();
		return *this;
	}

	constexpr void Reset() noexcept
	{
		denumerated = nullptr;
		number = {};
	}
	constexpr void SetNumber(Enumerated enumerated) noexcept
	{
		number = enumerated;
	}

	constexpr void SetDenumerated(Denum *d) noexcept
	{
		denumerated = d;
	}

	void CompileFunc(StdCompiler *compiler, bool intPack = false)
	{
		if (intPack)
		{
			compiler->Value(mkIntPackAdapt(number));
		}
		else
		{
			compiler->Value(number);
		}
	}

	void Enumerate()
	{
		SetNumber(Traits::Enumerate(Denumerated()));
	}

	template<typename... Args>
	void Denumerate(Args &&...args)
	{
		SetDenumerated(Traits::Denumerate(Number(), std::forward<Args>(args)...));
	}
};

struct C4EnumeratedObjectPtrTraits
{
	using Denumerated = C4Object;
	using Enumerated = std::int32_t;

	static std::int32_t Enumerate(C4Object *object);
	static C4Object *Denumerate(std::int32_t number, C4Section *section = nullptr);
};

using C4EnumeratedObjectPtr = C4EnumeratedPtr<C4EnumeratedObjectPtrTraits>;

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
