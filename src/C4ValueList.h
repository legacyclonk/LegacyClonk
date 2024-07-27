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

// Stellt eine einfache dynamische Integer-Liste bereit.

#pragma once

#include "C4Value.h"
#include "C4ValueStandardRefCountedContainer.h"

#include <span>
#include <vector>

class C4ValueList
{
public:
	enum { MaxSize = 1000000, }; // ye shalt not create arrays larger than that!

	C4ValueList() = default;
	C4ValueList(std::int32_t size);
	C4ValueList(const C4ValueList &other);

	template<typename T>
	C4ValueList(const std::span<T> data)
	{
		values.reserve(data.size());

		for (const auto value : data)
		{
			values.emplace_back(value);
		}
	}

	C4ValueList &operator=(const C4ValueList &ValueList2);

protected:
	std::vector<C4Value> values;

public:
	std::int32_t GetSize() const { return static_cast<std::int32_t>(values.size()); }

	const C4Value &GetItem(const std::int32_t index) const { return Inside(index, 0, GetSize() - 1) ? values[index] : C4VNull; }
	C4Value &GetItem(std::int32_t index);

	C4Value operator[](const std::int32_t index) const { return GetItem(index); }
	C4Value &operator[](const std::int32_t index) { return GetItem(index); }

	void Reset();
	void SetSize(std::int32_t size); // (enlarge only!)

	void DenumeratePointers(C4Section *section = nullptr);

	// comparison
	bool operator==(const C4ValueList &other) const = default;

	// Compilation
	void CompileFunc(class StdCompiler *pComp);

private:
	friend class C4SortObject;
};

// value list with reference count, used for arrays
class C4ValueArray : public C4ValueList, public C4ValueStandardRefCountedContainer<C4ValueArray>
{
public:
	C4ValueArray();
	C4ValueArray(std::int32_t size);

	template<typename T>
	C4ValueArray(const std::span<T> values) : C4ValueList{values} {}

	~C4ValueArray();

	// Change length, return self or new copy if necessary
	C4ValueArray *SetLength(std::int32_t size);
	virtual bool hasIndex(const C4Value &index) const override;
	virtual C4Value &operator[](const C4Value &index) override;
	using C4ValueList::operator[];

	virtual void CompileFunc(StdCompiler *pComp) override
	{
		C4ValueList::CompileFunc(pComp);
	}

	virtual void DenumeratePointers(C4Section *const section = nullptr) override
	{
		C4ValueList::DenumeratePointers(section);
	}

private:
	// Only for IncRef/AddElementRef
	friend C4ValueStandardRefCountedContainer<C4ValueArray>;
	C4ValueArray(const C4ValueArray &Array2);
};
