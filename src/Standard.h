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

/* All kinds of valuable helpers */

#pragma once

// Integer dataypes
#include <stdint.h>

#include "C4Breakpoint.h"

#include <string.h>
#include <string>

#ifdef _WIN32
#include "C4Windows.h"
#else

#define INFINITE 0xFFFFFFFF

unsigned long timeGetTime(void);

inline int stricmp(const char *s1, const char *s2)
{
	return strcasecmp(s1, s2);
}

#define GetRValue(rgb) (static_cast<unsigned char>(rgb))
#define GetGValue(rgb) (static_cast<unsigned char>((static_cast<unsigned short>(rgb)) >> 8))
#define GetBValue(rgb) (static_cast<unsigned char>((rgb) >> 16))
constexpr uint32_t RGB(uint8_t r, uint8_t g, uint8_t b) { return r | (g << 8) | (b << 16); }
#endif // _WIN32

// These functions have to be provided by the application.
bool Log(const char *szMessage);
bool LogSilent(const char *szMessage);

#include <memory.h>
#include <math.h>

#include <algorithm>
#include <concepts>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

constexpr auto SizeMax = std::numeric_limits<size_t>::max();

// Color triplets
#define C4RGB(r, g, b) (((static_cast<uint32_t>(r) & 0xff) << 16) | ((static_cast<uint32_t>(g) & 0xff) << 8) | ((b) & 0xff))

// Small helpers
template <class T> inline T Abs(T val) { return val > 0 ? val : -val; }
template <class T> inline constexpr bool Inside(T ival, T lbound, T rbound) { return ival >= lbound && ival <= rbound; }
template <class T> inline T BoundBy(T bval, T lbound, T rbound) { return bval < lbound ? lbound : bval > rbound ? rbound : bval; }
template <class T> inline int Sign(T val) { return val < 0 ? -1 : val > 0 ? 1 : 0; }
template <class T> inline void Toggle(T &v) { v = !v; }

template<std::integral To, std::integral From>
To checked_cast(From from)
{
	if constexpr (std::is_signed_v<From>)
	{
		if constexpr (std::is_unsigned_v<To>)
		{
			if (from < 0)
			{
				throw std::runtime_error{"Conversion of negative value to unsigned type requested"};
			}
		}
		else if constexpr (std::numeric_limits<From>::min() < std::numeric_limits<To>::min())
		{
			if (from < std::numeric_limits<To>::min())
			{
				throw std::runtime_error{"Conversion of value requested that is smaller than the target-type minimum"};
			}
		}
	}

	if constexpr (std::numeric_limits<From>::max() > std::numeric_limits<To>::max())
	{
		if (std::cmp_greater(from, std::numeric_limits<To>::max()))
		{
			throw std::runtime_error{"Conversion of value requested that is bigger than the target-type maximum"};
		}
	}

	return static_cast<To>(from);
}

inline int DWordAligned(int val)
{
	if (val % 4) { val >>= 2; val <<= 2; val += 4; }
	return val;
}

int32_t Distance(int32_t iX1, int32_t iY1, int32_t iX2, int32_t iY2);
int Angle(int iX1, int iY1, int iX2, int iY2);
int Pow(int base, int exponent);

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

#ifdef _WIN32
#define DirSep "\\"
#else
#define DirSep "/"
#endif

// sprintf wrapper

#include <stdio.h>
#include <stdarg.h>

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

const char *GetCurrentTimeStamp(bool enableMarkupColor = true);
