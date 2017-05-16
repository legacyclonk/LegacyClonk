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

// an associated list
// used by C4AulScriptEngine, as virtual function table and include/dependency list

#ifndef INC_C4AList
#define INC_C4AList

#define C4AListChunkSize 128 // number of table entries for each chunk

// table entry
struct C4AListEntry
	{
	void *Var, *Val;
	C4AListEntry *next(); // get entry after the given one
	};

// bunch of table entries
struct C4AListChunk
	{
	C4AListEntry Entries[C4AListChunkSize]; // table entries
	void *Stop; // stop entry; should always be NULL
	C4AListChunk *Next; // next chunk
	};

// associative list
class C4AList
	{
	protected:
		C4AListChunk *Table, *CurrC; // first/last table chunk
		int CCount; // number of entries in current chunk
		C4AListEntry *Curr; // next available entry to be used
		void Grow(); // append chunk

	public:
		C4AList(); // constructor
		~C4AList(); // destructor
		void Clear(); // clear the list

		C4AListEntry *push(void *pVar = NULL, void *pVal = NULL); // push var/value pair to end of list
	};

#endif
