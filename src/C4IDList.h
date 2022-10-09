/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2021, The LegacyClonk Team and contributors
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

/* A dynamic list of C4IDs */

#pragma once

#include "C4DelegatedIterable.h"
#include "C4ForwardDeclarations.h"
#include "C4Id.h"

#include <vector>

class C4IDList : public C4DelegatedIterable<C4IDList>
{
public:
	struct Entry
	{
		C4ID id;
		int32_t count;

		Entry() : id{C4ID_None}, count{0} {}
		Entry(C4ID id, int32_t count) : id{id}, count{count} {}
		bool operator==(const Entry &other) const { return id == other.id && count == other.count; }
		void CompileFunc(StdCompiler *compiler, bool withValues);
	};

	bool operator==(const C4IDList &other) const;

	// General
	void Clear();
	bool IsClear() const;
	// Access by direct index
	C4ID GetID(size_t index, int32_t *ipCount = nullptr) const;
	int32_t GetCount(size_t index) const;
	bool SetCount(size_t index, int32_t count);

	// Access by ID
	int32_t GetIDCount(C4ID id, int32_t zeroDefVal = 0) const;
	bool SetIDCount(C4ID id, int32_t count, bool addNewID = false);
	void IncreaseIDCount(C4ID id, bool addNewID = true, int32_t increaseBy = 1, bool removeEmpty = false);

	void DecreaseIDCount(C4ID id, bool removeEmpty = true)
	{
		IncreaseIDCount(id, false, -1, removeEmpty);
	}

	int32_t GetNumberOfIDs() const;
	int32_t GetIndex(C4ID id) const;

	// Access by category-sorted index
	C4ID GetID(C4DefList &rDefs, int32_t dwCategory, int32_t index, int32_t *ipCount = nullptr) const;
	int32_t GetNumberOfIDs(C4DefList &rDefs, int32_t dwCategory) const;
	// Aux
	void ConsolidateValids(C4DefList &rDefs, int32_t dwCategory = 0);
	void SortByValue(C4DefList &rDefs);
	// Item operation
	bool DeleteItem(size_t iIndex);
	void Load(C4DefList &defs, int32_t category);
	// Graphics
	void Draw(C4Facet &cgo, int32_t iSelection,
		C4DefList &rDefs, uint32_t dwCategory,
		bool fCounts = true, int32_t iAlign = 0) const;
	// Compiling
	void CompileFunc(StdCompiler *pComp, bool fValues = true);

private:
	std::vector<Entry> content;

public:
	using Iterable = ConstIterableMember<&C4IDList::content>;
};
