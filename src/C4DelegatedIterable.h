/*
 * LegacyClonk
 *
 * Copyright (c) 2020, The LegacyClonk Team and contributors
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

#include <functional>
#include <type_traits>

// Provides all standard variants of begin and end
// The iterable member must be designated by publicly using Iterable = [Const]IterableMember<&Class::Member>; in the class

template<class Class>
struct C4DelegatedIterable
{
	auto begin()
	{
		return std::invoke(Class::Iterable::Iterable, static_cast<Class *>(this)).begin();
	}

	auto end()
	{
		return std::invoke(Class::Iterable::Iterable, static_cast<Class *>(this)).end();
	}

	auto cbegin() const
	{
		return std::invoke(Class::Iterable::Iterable, static_cast<const Class *>(this)).cbegin();
	}

	auto cend() const
	{
		return std::invoke(Class::Iterable::Iterable, static_cast<const Class *>(this)).cend();
	}

	auto begin() const
	{
		return cbegin();
	}

	auto end() const
	{
		return cend();
	}

	auto rbegin()
	{
		return std::invoke(Class::Iterable::Iterable, static_cast<Class *>(this)).rbegin();
	}

	auto rend()
	{
		return std::invoke(Class::Iterable::Iterable, static_cast<Class *>(this)).rend();
	}

	auto crbegin() const
	{
		return std::invoke(Class::Iterable::Iterable, static_cast<const Class *>(this)).crbegin();
	}

	auto crend() const
	{
		return std::invoke(Class::Iterable::Iterable, static_cast<const Class *>(this)).crend();
	}

	auto rbegin() const
	{
		return crbegin();
	}

	auto rend() const
	{
		return crend();
	}

protected:
	template<auto member>
	class IterableMember
	{
		static constexpr auto Iterable = member;
		friend C4DelegatedIterable<Class>;
	};

	template<auto member>
	class ConstIterableMember
	{
		using Member = std::decay_t<std::invoke_result_t<decltype(member), Class>>;
		static constexpr const Member Class:: *Iterable = member;
		friend C4DelegatedIterable<Class>;
	};
};
