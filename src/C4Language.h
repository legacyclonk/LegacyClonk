/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2020, The LegacyClonk Team and contributors
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

/* Language module controlling external language packs */

#pragma once

#include "C4DelegatedIterable.h"
#include <C4Group.h>
#include <C4GroupSet.h>
#include "C4ResStrTable.h"

#include <array>
#include <string>
#include <vector>

#ifdef HAVE_ICONV
#include <iconv.h>
#endif

class C4Language;

class C4LanguageInfo
{
	friend class C4Language;

public:
	std::array<char, 2> Code;
	std::string Name;
	std::string Info;
	std::string Fallback;
	std::string Charset;

protected:
	C4LanguageInfo *Next;
};

class C4Language : public C4DelegatedIterable<C4Language>
{
public:
	C4Language();
	~C4Language();

protected:
	C4Group PackDirectory;
	C4GroupSet Packs;
	C4GroupSet PackGroups;
	std::vector<C4LanguageInfo> Infos;
	char PackGroupLocation[_MAX_FNAME + 1];

public:
	using Iterable = ConstIterableMember<&C4Language::Infos>;

public:
	void ClearLanguage();
	// Initialization
	bool Init();
	void Clear();
	// Handling of external language packs
	C4GroupSet &GetPackGroups(const char *strRelativePath);
	// Handling of language info loaded from string tables
	const C4LanguageInfo *FindInfo(const char *code);
	// Loading of actual resource string table
	bool LoadLanguage(const char *strLanguages);
	// Encoding conversion functions
	static StdStrBuf IconvClonk(const char *string);
	static StdStrBuf IconvSystem(const char *string);
	static StdStrBuf IconvUtf8(const char *string);

protected:
	// Handling of language info loaded from string tables
	void InitInfos();
	void LoadInfos(C4Group &hGroup);
	// Loading of actual resource string table
	bool InitStringTable(const char *strCode);
	bool LoadStringTable(C4Group &hGroup, const char *strCode);
#ifdef HAVE_ICONV
	static iconv_t local_to_host;
	static iconv_t host_to_local;
	static iconv_t local_to_utf_8;
	static StdStrBuf Iconv(const char *string, iconv_t cd);
#endif
};

extern C4Language Languages;

static inline StdStrBuf LoadResStrUtf8(const C4ResStrTableKeyFormat<> ident)
{
	return Languages.IconvUtf8(LoadResStr(ident));
}

static inline StdStrBuf LoadResStrUtf8Choice(const bool condition, const C4ResStrTableKeyFormat<> ifTrue, const C4ResStrTableKeyFormat<> ifFalse)
{
	return Languages.IconvUtf8(LoadResStrV(condition ? ifTrue.Id : ifFalse.Id));
}
