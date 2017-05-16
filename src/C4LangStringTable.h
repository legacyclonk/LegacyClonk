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

// Loads StringTbl* and replaces $..$-strings by localized versions

#ifndef INC_C4LangStringTable
#define INC_C4LangStringTable

#include "C4ComponentHost.h"

class C4LangStringTable : public C4ComponentHost
{
public:
	// do replacement in buffer
	// if any replacement is done, the buffer will be realloced
	void ReplaceStrings(StdStrBuf &rBuf);
	void ReplaceStrings(const StdStrBuf &rBuf, StdStrBuf &rTarget, const char *szParentFilePath = NULL);
};

#endif // INC_C4LangStringTable
