/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
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

/* At static list of C4IDs */

#include <C4Include.h>
#include <C4IDList.h>

#include <C4Def.h>
#include <C4Application.h>
#include <C4Game.h>

#include <algorithm>

void C4IDList::Clear()
{
	content.clear();
}

bool C4IDList::IsClear() const
{
	return content.empty();
}

C4ID C4IDList::GetID(size_t index, int32_t *ipCount) const
{
	if (!Inside<int32_t>(index, 0, GetNumberOfIDs() - 1)) return C4ID_None;
	const auto [id, count] = content[index];
	// get chunk to query
	if (ipCount) *ipCount = count;
	return id;
}

int32_t C4IDList::GetCount(size_t index) const
{
	if (!Inside<int32_t>(index, 0, GetNumberOfIDs() - 1)) return 0;
	return content[index].count;
}

bool C4IDList::SetCount(size_t index, int32_t count)
{
	if (!Inside<int32_t>(index, 0, GetNumberOfIDs() - 1)) return false;
	content[index].count = count;
	return true;
}

static auto findId(C4ID id, std::vector<C4IDList::Entry> &content)
{
	return std::find_if(content.begin(), content.end(), [id](const C4IDList::Entry &entry)
	{
		return entry.id == id;
	});
}

static auto findId(C4ID id, const std::vector<C4IDList::Entry> &content)
{
	return std::find_if(content.cbegin(), content.cend(), [id](const C4IDList::Entry &entry)
	{
		return entry.id == id;
	});
}

int32_t C4IDList::GetIDCount(C4ID id, int32_t zeroDefVal) const
{
	const auto it = findId(id, content);
	if (it == content.cend()) return 0;

	if (it->count == 0) return zeroDefVal;
	return it->count;
}

bool C4IDList::SetIDCount(C4ID id, int32_t count, bool addNewID)
{
	const auto it = findId(id, content);
	if (it == content.cend())
	{
		if (addNewID)
		{
			content.emplace_back(id, count);
			return true;
		}
		else
		{
			return false;
		}
	}

	it->count = count;
	return true;
}

int32_t C4IDList::GetNumberOfIDs() const
{
	return content.size();
}

int32_t C4IDList::GetIndex(C4ID id) const
{
	int32_t i = 0;
	for (const auto &it : content)
	{
		if (it.id == id) return i;
		++i;
	}
	return -1;
}

void C4IDList::IncreaseIDCount(C4ID id, bool addNewID, int32_t increaseBy, bool removeEmpty)
{
	const auto it = findId(id, content);
	if (it == content.cend())
	{
		if (addNewID)
		{
			content.emplace_back(id, increaseBy);
		}
	}
	else
	{
		it->count += increaseBy;
		if (removeEmpty && it->count == 0)
		{
			content.erase(it);
		}
	}
}

// Access by category-sorted index
#ifdef C4ENGINE

C4ID C4IDList::GetID(C4DefList &defs, int32_t category, int32_t index, int32_t *ipCount) const
{
	int32_t cindex = -1;
	for (const auto &it : content)
	{
		const auto def = defs.ID2Def(it.id);
		if (!def) continue;

		if (category == C4D_All || def->Category & category)
		{
			++cindex;
			if (cindex == index)
			{
				if (ipCount) *ipCount = it.count;
				return it.id;
			}
		}
	}

	return C4ID_None;
}

int32_t C4IDList::GetNumberOfIDs(C4DefList &defs, int32_t category) const
{
	return std::count_if(content.cbegin(), content.cend(), [&defs, category](const Entry &entry)
	{
		if (const auto def = defs.ID2Def(entry.id))
			return category == C4D_All || def->Category & category;
		return false;
	});
}

#endif

// Removes all empty id gaps from the list.

bool C4IDList::ConsolidateValids(C4DefList &defs, int32_t category)
{
	bool removedSome = false;
	content.erase(std::remove_if(content.begin(), content.end(), [&removedSome, &defs, category](const auto &entry)
	{
		const auto def = defs.ID2Def(entry.id);
		return !def || (category && !(def->Category & category));
	}), content.end());
	return removedSome;
}

void C4IDList::SortByValue(C4DefList &defs)
{
	std::stable_sort(content.begin(), content.end(), [&defs](const Entry &a, const Entry &b)
	{
		const auto defA = defs.ID2Def(a.id);
		const auto defB = defs.ID2Def(b.id);
		// FIXME: Should call GetValue here
		return defA && defB && defA->Value < defB->Value;
	});
}

void C4IDList::Load(C4DefList &defs, int32_t category)
{
	Clear();
	// add all IDs of def list
	int32_t count = 0;
	while (const auto def = defs.GetDef(count++, category))
	{
		content.emplace_back(def->id, 0);
	}
}

void C4IDList::Draw(C4Facet &cgo, int32_t selection,
	C4DefList &defs, uint32_t category,
	bool counts, int32_t align) const
{
#ifdef C4ENGINE
	int32_t sections = cgo.GetSectionCount();
	int32_t idnum = GetNumberOfIDs(defs, category);
	int32_t firstid = BoundBy<int32_t>(selection - sections / 2, 0, std::max<int32_t>(idnum - sections, 0));
	int32_t idcount;
	C4ID id;
	C4Facet cgo2;
	char buf[10];
	for (int32_t cnt = 0; (cnt < sections) && (id = GetID(defs, category, firstid + cnt, &idcount)); ++cnt)
	{
		cgo2 = cgo.TruncateSection(align);
		defs.Draw(id, cgo2, (firstid + cnt == selection), 0);
		sprintf(buf, "%dx", idcount);
		if (counts) Application.DDraw->TextOut(buf, Game.GraphicsResource.FontRegular, 1.0, cgo2.Surface, cgo2.X + cgo2.Wdt - 1, cgo2.Y + cgo2.Hgt - 1 - Game.GraphicsResource.FontRegular.GetLineHeight(), CStdDDraw::DEFAULT_MESSAGE_COLOR, ARight);
	}
#endif
}

bool C4IDList::DeleteItem(size_t index)
{
	if (!Inside<int32_t>(index, 0, GetNumberOfIDs() - 1)) return false;

	content.erase(std::next(content.cbegin(), index));
	return true;
}

void C4IDList::Entry::CompileFunc(StdCompiler *compiler, bool withValues)
{
	compiler->Value(mkC4IDAdapt(id));

	if (compiler->isCompiler() && !LooksLikeID(id))
	{
		// this breaks the loop in StdSTLContainerAdapt::CompileFunc
		compiler->excNotFound("Invalid ID");
	}

	if (withValues)
	{
		if (compiler->Separator(StdCompiler::SEP_SET))
			compiler->Value(mkDefaultAdapt(count, 0));
	}
}

void C4IDList::CompileFunc(StdCompiler *pComp, bool withValues)
{
	auto stlAdapter = mkSTLContainerAdapt(content, StdCompiler::SEP_SEP2);
	pComp->Value(mkParAdapt(stlAdapter, withValues));
}

bool C4IDList::operator==(const C4IDList &other) const
{
	return content == other.content;
}
