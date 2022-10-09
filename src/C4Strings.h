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

#pragma once

#include "C4Breakpoint.h"

#include <cstddef>
#include <cstdio>
#include <limits>

#ifdef _WIN32
#include "C4Windows.h"
#else
#include <strings.h>
#endif

#ifndef _WIN32
inline int stricmp(const char *s1, const char *s2)
{
	return strcasecmp(s1, s2);
}
#endif

constexpr auto SizeMax = std::numeric_limits<size_t>::max();

char CharCapital(char cChar);
bool IsIdentifier(char cChar);
bool IsWhiteSpace(char cChar);

size_t SLen(const char *sptr);

bool SEqual(const char *szStr1, const char *szStr2);
bool SEqual2(const char *szStr1, const char *szStr2);

bool SEqualNoCase(const char *szStr1, const char *szStr2, size_t iLen = SizeMax);
bool SEqual2NoCase(const char *szStr1, const char *szStr2, size_t iLen = SizeMax);

void SCopy(const char *szSource, char *sTarget, size_t iMaxL = SizeMax);
void SCopyUntil(const char *szSource, char *sTarget, char cUntil, size_t iMaxL = SizeMax, size_t iIndex = 0);
void SCopyUntil(const char *szSource, char *sTarget, const char *sUntil, size_t iMaxL);
void SCopyIdentifier(const char *szSource, char *sTarget, size_t iMaxL = SizeMax);
bool SCopySegment(const char *fstr, size_t segn, char *tstr, char sepa = ';', size_t iMaxL = SizeMax, bool fSkipWhitespace = false);
bool SCopySegmentEx(const char *fstr, size_t segn, char *tstr, char sepa1, char sepa2, size_t iMaxL = SizeMax, bool fSkipWhitespace = false);
bool SCopyEnclosed(const char *szSource, char cOpen, char cClose, char *sTarget, size_t iSize);

void SAppend(const char *szSource, char *szTarget, size_t iMaxL = SizeMax);
void SAppendChar(char cChar, char *szStr);

void SInsert(char *szString, const char *szInsert, size_t iPosition = 0, size_t iMaxL = SizeMax);
void SDelete(char *szString, size_t iLen, size_t iPosition = 0);

int SCharPos(char cTarget, const char *szInStr, size_t iIndex = 0);
int SCharLastPos(char cTarget, const char *szInStr);
int SCharCount(char cTarget, const char *szInStr, const char *cpUntil = nullptr);
int SCharCountEx(const char *szString, const char *szCharList);

void SReplaceChar(char *str, char fc, char tc);

const char *SSearch(const char *szString, const char *szIndex);
const char *SSearchNoCase(const char *szString, const char *szIndex);

const char *SAdvanceSpace(const char *szSPos);
const char *SAdvancePast(const char *szSPos, char cPast);

bool SGetModule(const char *szList, size_t iIndex, char *sTarget, size_t iSize = SizeMax);
bool SIsModule(const char *szList, const char *szString, size_t *ipIndex = nullptr, bool fCaseSensitive = false);
bool SAddModule(char *szList, const char *szModule, bool fCaseSensitive = false);
bool SAddModules(char *szList, const char *szModules, bool fCaseSensitive = false);
bool SRemoveModule(char *szList, const char *szModule, bool fCaseSensitive = false);
bool SRemoveModules(char *szList, const char *szModules, bool fCaseSensitive = false);
int SModuleCount(const char *szList);

const char *SGetParameter(const char *strCommandLine, size_t iParameter, char *strTarget = nullptr, size_t iSize = SizeMax, bool *pWasQuoted = nullptr);

void SNewSegment(char *szStr, const char *szSepa = ";");
void SCapitalize(char *szString);
void SWordWrap(char *szText, char cSpace, char cSepa, size_t iMaxLine);
int SClearFrontBack(char *szString, char cClear = ' ');

int SGetLine(const char *szText, const char *cpPosition);
int SLineGetCharacters(const char *szText, const char *cpPosition);

// case sensitive wildcard match with some extra functionality
// can match strings like  "*Cl?nk*vour" to "Clonk Endeavour"
bool SWildcardMatchEx(const char *szString, const char *szWildcard);

#define LineFeed "\x00D\x00A"
#define EndOfFile "\x020"

// sprintf wrapper

// format string security
bool IsSafeFormatString(const char *szFmt);

// secure sprintf
template <std::size_t N, typename... Args>
inline int ssprintf(char (&str)[N], const char *fmt, Args... args)
{
	// Check parameters
	if (!IsSafeFormatString(fmt))
	{
		BREAKPOINT_HERE;
		fmt = "<UNSAFE FORMAT STRING>";
	}
	// Build string
	int m = snprintf(str, N, fmt, args...);
	if (m >= N) { m = N - 1; str[m] = 0; }
	return m;
}
