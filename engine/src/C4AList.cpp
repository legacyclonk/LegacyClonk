/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
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

// an associative list

#include <C4Include.h>
#include <C4AList.h>

/* associative list */

C4AList::C4AList()
	{
	// init list with no chunks
	// other values will be inited upon first Grow()
	Table = NULL;
	}

C4AList::~C4AList()
	{
	// clear list
	Clear();
	}

void C4AList::Clear()
	{
	// free chunks
	C4AListChunk *c = Table;
	while (c)
		{
		C4AListChunk *c2 = c->Next;
		delete c;
		c = c2;
		}
	// reset values
	Table = NULL;
	}


void C4AList::Grow()
	{
	// create table if not existant
	if (!Table)
		{
		Table = CurrC = new C4AListChunk;
		}
	else
		{
		// otherwise, add new chunk to end of table
		CurrC->Next = new C4AListChunk;
		CurrC = CurrC->Next;
		}
	// init new chunk
	ZeroMemory(CurrC, sizeof(C4AListChunk));
	// reset current chunk pos
	Curr = &CurrC->Entries[0];
	CCount = 0;
	}

C4AListEntry *C4AList::push(void *pVar, void *pVal)
	{
	// init/grow list if necessary
	if (!Table || (CCount == C4AListChunkSize)) Grow();
	// push to end of list
	C4AListEntry *Entry = Curr;
	Entry->Var = pVar; Entry->Val = pVal;
	// select next entry
	CCount++; Curr++;
	// done, hand back entry
	return Entry;
	}

void *C4AList::get(void *pVar, C4AListEntry *pOff)
	{
	// list empty?
	if (!Table) return NULL;
	// start at beginning of table, if no offset assigned
	if (!pOff) pOff = &Table->Entries[0];
	// get from offset
	return pOff->get(pVar);
	}

void *C4AListEntry::get(void *pVar)
	{
	// search entries; beginning at this
	C4AListEntry *pOff = this;
	while (1)
		{
		// entry is invalid/stop-entry?
		if (!pOff->Var)
			{
			// check if it's just the end of a chunk
			if (!(pOff = (C4AListEntry *) pOff->Val))
				// so it's a stop entry or the end of the list; break here
				break;
			// if it was, check next chunk
			if (!pOff->Var) break;
			}
		// so we've got a valid entry; check if it's the correct one
		if (pOff->Var == pVar) return pOff->Val;
		// it wasn't; check next
		pOff++;
		}
	// nothing found
	return NULL;

	}

C4AListEntry *C4AListEntry::next()
	{
	// search entries; beginning at this
	C4AListEntry *pOff = this + 1;
	// entry is valid? fine
	if (pOff->Var) return pOff;
	// check if it's just the end of a chunk
	if (!(pOff = (C4AListEntry *) pOff->Val))
		// it's a stop entry or the end of the list; return failure
		return NULL;
	// return beginning of next chunk, if valid
	if (pOff->Var) return pOff;
	// otherwise, fail
	return NULL;
	}

C4AListEntry *C4AListEntry::nextVal()
	{
	// get entries beginning at next one
	C4AListEntry *e = this;
	while ((e = e->next()))
		{
		// return if Val != NULL
		if (e->Val) return e;
		}
	// nothing found
	return NULL;
	}

C4AListEntry *C4AListEntry::firstVal()
	{
	// get entries beginning at next one
	C4AListEntry *e = this;
	while (e)
		{
		// return if Val != NULL
		if (e->Val) return e;
		//next
		e = e->next();
		}
	// nothing found
	return NULL;
	}
