/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2017-2022, The LegacyClonk Team and contributors
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

#include <C4Include.h>
#include <C4ValueList.h>
#include <stdexcept>

#include <C4Aul.h>
#include <C4FindObject.h>

#include <format>
#include <compare>
#include <ranges>

C4ValueList::C4ValueList(const std::int32_t size)
{
	SetSize(size);
}

C4ValueList::C4ValueList(const C4ValueList &other)
	: values{other.values}
{
}

C4ValueList &C4ValueList::operator=(const C4ValueList &other)
{
	values.resize(other.values.size());

	for (std::size_t i{0}; i < values.size(); ++i)
	{
		values[i].Set(other.values[i]);
	}

	return *this;
}

C4Value &C4ValueList::GetItem(std::int32_t index)
{
	if (index < 0) index = 0;

	if (index >= GetSize() && index < MaxSize)
	{
		SetSize(index + 1);
	}

	if (index >= GetSize())
	{
		throw C4AulExecError(nullptr, "out of memory");
	}

	return values[index];
}

void C4ValueList::SetSize(const std::int32_t size)
{
	const auto cmp = size <=> GetSize();

	if (cmp == std::strong_ordering::equal || (cmp == std::strong_ordering::greater && size > MaxSize))
	{
		return;
	}

	else if (cmp == std::strong_ordering::less)
	{
		values.resize(size);
		return;
	}

	std::vector<C4Value> newValues(size);

	for (std::size_t i{0}; i < values.size(); ++i)
	{
		values[i].Move(&newValues[i]);
	}

	values = std::move(newValues);
}

void C4ValueList::Reset()
{
	values.clear();
}

void C4ValueList::DenumeratePointers(C4Section *const section)
{
	for (auto &value : values)
	{
		value.DenumeratePointer(section);
	}
}

void C4ValueList::CompileFunc(class StdCompiler *pComp)
{
	// FIXME: this should be one of C4ValueInt or (u)intptr_t (or C4ID), but which one?
	int32_t size{GetSize()};
	// Size. Reset if not found.
	try
	{
		pComp->Value(size);
	}
	catch (const StdCompiler::NotFoundException &)
	{
		Reset(); return;
	}
	// Separator
	if (!pComp->Separator(StdCompiler::SEP_SEP2))
	{
		assert(pComp->isCompiler());
		// No ';'-separator? So it's a really old value list format, or empty
		pComp->Separator(StdCompiler::SEP_SEP);
		this->SetSize(C4MaxVariable);
		// First variable was misinterpreted as size
		values[0] = C4Value{C4V_Data{size}, C4V_Any};
		// Read remaining data
		pComp->Value(mkArrayAdaptS(values.data() + 1, C4MaxVariable - 1, C4Value()));
	}
	else
	{
		if (pComp->isCompiler())
		{
			// Allocate
			this->SetSize(size);
			// Values
			pComp->Value(mkArrayAdaptS(values.data(), size, C4Value()));
		}
		else
		{
			pComp->Value(mkArrayAdaptS(values.data(), size));
		}
	}
}

C4ValueArray::C4ValueArray()
	: C4ValueList() {}

C4ValueArray::C4ValueArray(int32_t inSize)
	: C4ValueList(inSize) {}

C4ValueArray::C4ValueArray(const C4ValueArray &Array2)
	: C4ValueList(Array2) {}

C4ValueArray::~C4ValueArray() {}

C4ValueArray *C4ValueArray::SetLength(int32_t size)
{
	if (GetRefCount() > 1)
	{
		C4ValueArray *pNew = static_cast<C4ValueArray *>((new C4ValueArray(size))->IncRef());
		for (std::int32_t i = 0; i < (std::min)(size, GetSize()); i++)
			pNew->values[i].Set(values[i]);
		DecRef();
		return pNew;
	}
	else
	{
		SetSize(size);
		return this;
	}
}

bool C4ValueArray::hasIndex(const C4Value &index) const
{
	C4Value copyIndex = index;
	if (!copyIndex.ConvertTo(C4V_Int)) throw std::runtime_error{std::format("array access: can not convert \"{}\" to int", GetC4VName(index.GetType()))};
	return copyIndex._getInt() < GetSize();
}

C4Value &C4ValueArray::operator[](const C4Value &index)
{
	C4Value copyIndex = index;
	if (!copyIndex.ConvertTo(C4V_Int)) throw std::runtime_error{std::format("array access: can not convert \"{}\" to int", GetC4VName(index.GetType()))};
	return (*this)[copyIndex._getInt()];
}
