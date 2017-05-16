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

/* string table: holds all strings used by script engine */

#ifndef C4STRINGTABLE_H

#define C4STRINGTABLE_H

class C4StringTable;

class C4String
{
public:
	C4String(C4StringTable *pTable);
	C4String(StdStrBuf strString, C4StringTable *pTable);
	C4String(const char *strString, C4StringTable *pTable);
	virtual ~C4String();

	// increment/decrement reference count on this string
	void IncRef();
	void DecRef();

	StdCopyStrBuf Data; // string data
	int iRefCnt; // reference count on string (by C4Value)
	bool Hold;  // string stays hold when RefCnt reaches 0 (for in-script strings)

	int iEnumID;

	C4String *Next, *Prev; // double-linked list

	C4StringTable *pTable; // owning table

	void Reg(C4StringTable *pTable);
	void UnReg();

};

class C4StringTable
{
public:
	C4StringTable();
	virtual ~C4StringTable();
	
	void Clear();

	C4String *RegString(const char *strString);
	C4String *FindString(const char *strString);
	C4String *FindString(C4String *pString);
	C4String *FindString(int iEnumID);

	int EnumStrings();

	bool Load(C4Group& ParentGroup);
	bool Save(C4Group& ParentGroup);

	C4String *First, *Last; // string list
};

#endif
