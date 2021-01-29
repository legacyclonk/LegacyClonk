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

#include <type_traits>

namespace
{
	template<typename T>
	constexpr decltype(auto) adlBegin(T &&container)
	{
		using std::begin;
		return begin(container);
	}

	template<typename T>
	constexpr decltype(auto) adlEnd(T &&container)
	{
		using std::end;
		return end(container);
	}
}

template<typename... T>
class StdOverloadedCallable : public T...
{
public:
	StdOverloadedCallable(T... bases) : T{bases}... {}
	using T::operator()...;
};

template<typename Container, typename Filter, typename Iterator = std::decay_t<decltype(adlBegin(std::declval<Container>()))>, typename Derived = void>
class StdFilterator {
	using ClassType = StdFilterator<Container, Filter, Iterator, Derived>;
	using Filterator = std::conditional_t<std::is_same_v<Derived, void>, ClassType, Derived>;
	constexpr Filterator &this_() { return *static_cast<Filterator *>(this); }
	constexpr const Filterator &this_() const { return *static_cast<const Filterator *>(this); }

public:
	constexpr StdFilterator(Container &container, const Filter &filter, const Iterator &iterator) : container{container}, filter{filter}, iterator{iterator}
	{
		checkFilter();
	}

	constexpr StdFilterator(Container &container, const Filter &filter) : ClassType{container, filter, adlBegin(container)} {}

	constexpr Filterator begin() const { return this_(); }
	constexpr Filterator end() const { return {container, filter, adlEnd(container)}; }

	constexpr Filterator &operator++()
	{
		++iterator;
		checkFilter();
		return this_();
	}

	constexpr bool operator!=(const ClassType &other) const { return iterator != other.iterator; }
	constexpr bool operator==(const ClassType &other) const { return iterator == other.iterator; }

	constexpr decltype(auto) operator*() const { return *iterator; }
	constexpr decltype(auto) operator->() const { return iterator; }

protected:
	using internal_iterator = Iterator;

private:
	Filter filter;
	Container &container;
	Iterator iterator;

	constexpr void checkFilter()
	{
		while (iterator != adlEnd(container) && !filter(*iterator))
		{
			++iterator;
		}
	}
};

template <typename Container, typename Filter>
StdFilterator(Container &, const Filter &) -> StdFilterator<Container, Filter>;

template <typename Container, typename Filter, typename Iterator>
StdFilterator(Container &, const Filter &, const Iterator &) -> StdFilterator<Container, Filter, Iterator>;
