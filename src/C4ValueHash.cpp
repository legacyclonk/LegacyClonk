/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2017-2019, The LegacyClonk Team and contributors
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

#include "C4ValueHash.h"
#include "C4StringTable.h"

#include <optional>
#include <stdexcept>

C4ValueHash::C4ValueHash() { }

C4ValueHash::C4ValueHash(const C4ValueHash &other)
{
	*this = other;
}

C4ValueHash::~C4ValueHash()
{
	clear();
}

void C4ValueHash::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkSTLMapAdapt(*this));
}

void C4ValueHash::DenumeratePointers()
{
	for (auto [key, value] : *this)
	{
		const_cast<C4Value&>(key).DenumeratePointer();
		value.DenumeratePointer();
	}
}

void C4ValueHash::removeKey(const C4Value &key)
{
	const auto &value = map[key];
	emptyValues.push_front(value.value);
	keyOrder.erase(value.keyOrderIterator);
	map.erase(key);
}

void C4ValueHash::removeValue(C4Value *value)
{
	bool found = false;
	for (auto it = map.begin(); it != map.end(); )
	{
		if (it->second.value == value)
		{
			keyOrder.erase(it->second.keyOrderIterator);
			map.erase(it++);
			found = true;
		}
		else ++it;
	}
	if (found) emptyValues.push_front(value);
}

bool C4ValueHash::contains(const C4Value &key) const
{
	return map.find(key) != map.end();
}

void C4ValueHash::clear()
{
	for (auto &[key, value] : map) delete value.value;
	map.clear();
	for (auto &value : emptyValues) delete value;
	emptyValues.clear();
}

C4ValueHash &C4ValueHash::operator=(const C4ValueHash &other)
{
	for (const auto key : other.keyOrder)
	{
		(*this)[*key].Set(*other.map.at(*key).value);
	}
	return *this;
}

bool C4ValueHash::operator==(const C4ValueHash &other) const
{
	if (other.size() != size()) return false;

	for (const auto &it : map)
	{
		if (!other.contains(it.first) || other[it.first] != *it.second.value)
			return false;
	}

	return true;
}

bool C4ValueHash::hasIndex(const C4Value &key) const
{
	return contains(key);
}

C4Value &C4ValueHash::operator[](const C4Value &key)
{
	try
	{
		return *map.at(key).value;
	}
	catch (const std::out_of_range &)
	{
		C4Value *value;
		if (emptyValues.empty()) value = C4Value::OfMap(this);
		else
		{
			value = emptyValues.front();
			emptyValues.pop_front();
		}

		const auto &inserted = map.emplace(std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple(MapEntry{value, {}})).first;
		inserted->second.keyOrderIterator = keyOrder.insert(keyOrder.end(), &inserted->first);
		return *value;
	}
}

const C4Value &C4ValueHash::operator[](const C4Value &key) const
{
	try
	{
		return *map.at(key).value;
	}
	catch (const std::out_of_range &)
	{
		return C4VNull;
	}
}

C4ValueHash::Iterator C4ValueHash::begin()
{
	return Iterator(this, keyOrder.begin(), keyOrder.end());
}

C4ValueHash::Iterator C4ValueHash::end()
{
	return Iterator(this, keyOrder.end(), keyOrder.end());
}

C4ValueHash::Iterator::Iterator(C4ValueHash *map, C4ValueHash::Iterator::iterator_type it, C4ValueHash::Iterator::iterator_type end) : it(it), end(end), map(map)
{
	update();
}

void C4ValueHash::Iterator::update()
{
	if (it != end)
	{
		current = std::make_unique<pair_type>(**it, (*map)[**it]);
	}
	else
	{
		current.reset();
	}
}

C4ValueHash::Iterator &C4ValueHash::Iterator::operator++()
{
	++it;
	update();
	return *this;
}

C4ValueHash::Iterator::pair_type &C4ValueHash::Iterator::operator*()
{
	return *current;
}

bool C4ValueHash::Iterator::operator==(const C4ValueHash::Iterator &other)
{
	return it == other.it;
}

bool C4ValueHash::Iterator::operator!=(const C4ValueHash::Iterator &other)
{
	return it != other.it;
}
