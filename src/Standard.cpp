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

/* All kinds of valuable helpers */

#include <Standard.h>
#if defined(HAVE_PTHREAD) && defined(C4ENGINE)
// c4group is single-threaded
#include <StdSync.h>
#endif

#ifdef _WIN32
#include <shellapi.h>
#else
#include <ctype.h>
#endif

#include <time.h>

#include <sys/timeb.h>
#include <cassert>
#include <cctype>
#include <cstring>

// Basics

int32_t Distance(int32_t iX1, int32_t iY1, int32_t iX2, int32_t iY2)
{
	int64_t dx = int64_t(iX1) - iX2, dy = int64_t(iY1) - iY2;
	int64_t d2 = dx * dx + dy * dy;
	if (d2 < 0) return -1;
	int32_t dist = int32_t(sqrt(double(d2)));
	if (int64_t(dist) * dist < d2) ++dist;
	if (int64_t(dist) * dist > d2) --dist;
	return dist;
}

int Angle(int iX1, int iY1, int iX2, int iY2)
{
	int iAngle = (int)(180.0 * atan2f(float(Abs(iY1 - iY2)), float(Abs(iX1 - iX2))) / M_PI);
	if (iX2 > iX1)
	{
		if (iY2 < iY1) iAngle = 90 - iAngle; else iAngle = 90 + iAngle;
	}
	else
	{
		if (iY2 < iY1) iAngle = 270 + iAngle; else iAngle = 270 - iAngle;
	}
	return iAngle;
}

/* Fast integer exponentiation */
int Pow(int base, int exponent)
{
	if (exponent < 0) return 0;

	int result = 1;

	if (exponent & 1) result = base;
	exponent >>= 1;

	while (exponent)
	{
		base *= base;
		if (exponent & 1) result *= base;
		exponent >>= 1;
	}

	return result;
}

bool ForLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
	bool(*fnCallback)(int32_t, int32_t, int32_t), int32_t iPar,
	int32_t *lastx, int32_t *lasty)
{
	int d, dx, dy, aincr, bincr, xincr, yincr, x, y;
	if (Abs(x2 - x1) < Abs(y2 - y1))
	{
		if (y1 > y2) { std::swap(x1, x2); std::swap(y1, y2); }
		xincr = (x2 > x1) ? 1 : -1;
		dy = y2 - y1; dx = Abs(x2 - x1);
		d = 2 * dx - dy; aincr = 2 * (dx - dy); bincr = 2 * dx; x = x1; y = y1;
		if (!fnCallback(x, y, iPar))
		{
			if (lastx) *lastx = x; if (lasty) *lasty = y;
			return false;
		}
		for (y = y1 + 1; y <= y2; ++y)
		{
			if (d >= 0) { x += xincr; d += aincr; }
			else d += bincr;
			if (!fnCallback(x, y, iPar))
			{
				if (lastx) *lastx = x; if (lasty) *lasty = y;
				return false;
			}
		}
	}
	else
	{
		if (x1 > x2) { std::swap(x1, x2); std::swap(y1, y2); }
		yincr = (y2 > y1) ? 1 : -1;
		dx = x2 - x1; dy = Abs(y2 - y1);
		d = 2 * dy - dx; aincr = 2 * (dy - dx); bincr = 2 * dy; x = x1; y = y1;
		if (!fnCallback(x, y, iPar))
		{
			if (lastx) *lastx = x; if (lasty) *lasty = y;
			return false;
		}
		for (x = x1 + 1; x <= x2; ++x)
		{
			if (d >= 0) { y += yincr; d += aincr; }
			else d += bincr;
			if (!fnCallback(x, y, iPar))
			{
				if (lastx) *lastx = x; if (lasty) *lasty = y;
				return false;
			}
		}
	}
	return true;
}

// Characters

char CharCapital(char cChar)
{
	if (Inside(cChar, 'a', 'z')) return (cChar - 32);
	if (cChar == '\xe4' /*ä*/) return '\xc4' /*Ä*/;
	if (cChar == '\xf6' /*ö*/) return '\xd6' /*Ö*/;
	if (cChar == '\xfc' /*ü*/) return '\xdc' /*Ü*/;
	return cChar;
}

bool IsIdentifier(char cChar)
{
	if (Inside(cChar, 'A', 'Z')) return true;
	if (Inside(cChar, 'a', 'z')) return true;
	if (Inside(cChar, '0', '9')) return true;
	if (cChar == '_') return true;
	if (cChar == '~') return true;
	if (cChar == '+') return true;
	if (cChar == '-') return true;
	return false;
}

bool IsWhiteSpace(char cChar)
{
	if (cChar == ' ') return true;
	if (cChar == 0x09) return true; // Tab
	if (cChar == 0x0D) return true; // Line feed
	if (cChar == 0x0A) return true; // Line feed
	return false;
}

// Strings

int SLen(const char *sptr)
{
	int slen = 0;
	if (!sptr) return 0;
	while (*sptr) { slen++; sptr++; }
	return slen;
}

void SCopyL(const char *szSource, char *sTarget, int iMaxL)
{
	if (szSource == sTarget) return;
	if (!sTarget) return; *sTarget = 0; if (!szSource) return;
	while (*szSource && (iMaxL > 0))
	{
		*sTarget = *szSource; iMaxL--; szSource++; sTarget++;
	}
	*sTarget = 0;
}

void SCopy(const char *szSource, char *sTarget, int iMaxL)
{
	if (szSource == sTarget) return;
	if (iMaxL == -1)
	{
		if (!sTarget) return; *sTarget = 0; if (!szSource) return;
		strcpy(sTarget, szSource);
	}
	else
		SCopyL(szSource, sTarget, iMaxL);
}

void SCopyUntil(const char *szSource, char *sTarget, char cUntil, int iMaxL, int iIndex)
{
	if (szSource == sTarget) return;
	if (!sTarget) return; *sTarget = 0; if (!szSource) return;
	while (*szSource && ((*szSource != cUntil) || (iIndex > 0)) && (iMaxL != 0))
	{
		*sTarget = *szSource; if (*szSource == cUntil) iIndex--; szSource++; sTarget++; iMaxL--;
	}
	*sTarget = 0;
}

void SCopyUntil(const char *szSource, char *sTarget, const char *sUntil, size_t iMaxL)
{
	size_t n = (std::min)(strcspn(szSource, sUntil), iMaxL - 1);
	strncpy(sTarget, szSource, n);
	sTarget[n] = 0;
}

bool SEqual(const char *szStr1, const char *szStr2)
{
	if (!szStr1 || !szStr2) return false;
	return !strcmp(szStr1, szStr2);
}

// Beginning of string 1 needs to match string 2.

bool SEqual2(const char *szStr1, const char *szStr2)
{
	if (!szStr1 || !szStr2) return false;
	while (*szStr1 && *szStr2)
		if (*szStr1++ != *szStr2++) return false;
	if (*szStr2) return false; // Str1 is shorter
	return true;
}

bool SEqualNoCase(const std::string &str1, const std::string &str2, int32_t length)
{
	if (str1.size() != str2.size())
	{
		return false;
	}

	switch (length)
	{
	case -1:
		length = static_cast<int32_t>(str1.size());
		break;
	case 0:
		return true;
	default:
		length = std::min(length, static_cast<int32_t>(str1.size()));
		break;
	}

	return std::equal(std::begin(str1), std::begin(str1) + length, std::begin(str2), [](const char &a, const char &b)
	{
		return tolower(a) == tolower(b);
	});
}

bool SEqualNoCase(const char *szStr1, const char *szStr2, int32_t iLen)
{
	if (!szStr1 || !szStr2) return false;
	if (iLen == 0) return true;
	while (*szStr1 && *szStr2)
	{
		if (CharCapital(*szStr1++) != CharCapital(*szStr2++)) return false;
		if (iLen > 0) { iLen--; if (iLen == 0) return true; }
	}
	if (*szStr1 || *szStr2) return false;
	return true;
}

bool SEqual2NoCase(const std::string &str1, const std::string &str2, int32_t length)
{
	assert(str1.size() <= str2.size());
	return SEqualNoCase(str1, str2.substr(0, std::min(static_cast<size_t>(length), str1.size())), length);
}

bool SEqual2NoCase(const char *szStr1, const char *szStr2, int iLen)
{
	if (!szStr1 || !szStr2) return false;
	if (iLen == 0) return true;
	while (*szStr1 && *szStr2)
	{
		if (CharCapital(*szStr1++) != CharCapital(*szStr2++)) return false;
		if (iLen > 0) { iLen--; if (iLen == 0) return true; }
	}
	if (*szStr2) return false; // Str1 is shorter
	return true;
}

int SCharPos(char cTarget, const char *szInStr, int iIndex)
{
	const char *cpos;
	int ccpos;
	if (!szInStr) return -1;
	for (cpos = szInStr, ccpos = 0; *cpos; cpos++, ccpos++)
		if (*cpos == cTarget)
			if (iIndex == 0) return ccpos;
			else iIndex--;
			return -1;
}

int SCharLastPos(char cTarget, const char *szInStr)
{
	const char *cpos;
	int ccpos, lcpos;
	if (!szInStr) return -1;
	for (cpos = szInStr, ccpos = 0, lcpos = -1; *cpos; cpos++, ccpos++)
		if (*cpos == cTarget) lcpos = ccpos;
	return lcpos;
}

void SAppend(const char *szSource, char *szTarget, int iMaxL)
{
	if (iMaxL == -1)
		SCopy(szSource, szTarget + SLen(szTarget));
	else
		SCopy(szSource, szTarget + SLen(szTarget), iMaxL - SLen(szTarget));
}

void SAppendChar(char cChar, char *szStr)
{
	if (!szStr) return;
	char *cPos;
	for (cPos = szStr; *cPos; cPos++);
	*cPos = cChar; *(cPos + 1) = 0;
}

bool SCopySegment(const char *szString, int iSegment, char *sTarget,
	char cSeparator, int iMaxL, bool fSkipWhitespace)
{
	// Advance to indexed segment
	while (iSegment > 0)
	{
		if (SCharPos(cSeparator, szString) == -1)
		{
			sTarget[0] = 0; return false;
		}
		szString += SCharPos(cSeparator, szString) + 1;
		iSegment--;
	}
	// Advance whitespace
	if (fSkipWhitespace)
		szString = SAdvanceSpace(szString);
	// Copy segment contents
	SCopyUntil(szString, sTarget, cSeparator, iMaxL);
	return true;
}

bool SCopySegmentEx(const char *szString, int iSegment, char *sTarget,
	char cSep1, char cSep2, int iMaxL, bool fSkipWhitespace)
{
	// Advance to indexed segment
	while (iSegment > 0)
	{
		// use the separator that's closer
		int iPos1 = SCharPos(cSep1, szString), iPos2 = SCharPos(cSep2, szString);
		if (iPos1 == -1)
			if (iPos2 == -1)
			{
				sTarget[0] = 0; return false;
			}
			else
				iPos1 = iPos2;
		else if (iPos2 != -1 && iPos2 < iPos1)
			iPos1 = iPos2;
		szString += iPos1 + 1;
		iSegment--;
	}
	// Advance whitespace
	if (fSkipWhitespace)
		szString = SAdvanceSpace(szString);
	// Copy segment contents; use separator that's closer
	int iPos1 = SCharPos(cSep1, szString), iPos2 = SCharPos(cSep2, szString);
	if (iPos2 != -1 && (iPos2 < iPos1 || iPos1 == -1)) cSep1 = cSep2;
	SCopyUntil(szString, sTarget, cSep1, iMaxL);
	return true;
}

int SCharCount(char cTarget, const char *szInStr, const char *cpUntil)
{
	int iResult = 0;
	// Scan string
	while (*szInStr)
	{
		// End position reached (end character is not included)
		if (szInStr == cpUntil) return iResult;
		// Character found
		if (*szInStr == cTarget) iResult++;
		// Advance
		szInStr++;
	}
	// Done
	return iResult;
}

int SCharCountEx(const char *szString, const char *szCharList)
{
	int iResult = 0;
	while (*szCharList)
	{
		iResult += SCharCount(*szCharList, szString);
		szCharList++;
	}
	return iResult;
}

void SReplaceChar(char *str, char fc, char tc)
{
	while (str && *str)
	{
		if (*str == fc) *str = tc; str++;
	}
}

void SCapitalize(char *str)
{
	while (str && *str)
	{
		*str = CharCapital(*str);
		str++;
	}
}

const char *SSearch(const char *szString, const char *szIndex)
{
	const char *cscr;
	int indexlen, match = 0;
	if (!szString || !szIndex) return nullptr;
	indexlen = SLen(szIndex);
	for (cscr = szString; cscr && *cscr; cscr++)
	{
		if (*cscr == szIndex[match]) match++;
		else match = 0;
		if (match >= indexlen) return cscr + 1;
	}
	return nullptr;
}

const char *SSearchNoCase(const char *szString, const char *szIndex)
{
	const char *cscr;
	int indexlen, match = 0;
	if (!szString || !szIndex) return nullptr;
	indexlen = SLen(szIndex);
	for (cscr = szString; cscr && *cscr; cscr++)
	{
		if (CharCapital(*cscr) == CharCapital(szIndex[match])) match++;
		else match = 0;
		if (match >= indexlen) return cscr + 1;
	}
	return nullptr;
}

void SWordWrap(char *szText, char cSpace, char cSepa, int iMaxLine)
{
	if (!szText) return;
	// Scan string
	char *cPos, *cpLastSpace = nullptr;
	int iLineRun = 0;
	for (cPos = szText; *cPos; cPos++)
	{
		// Store last space
		if (*cPos == cSpace) cpLastSpace = cPos;
		// Separator encountered: reset line run
		if (*cPos == cSepa) iLineRun = 0;
		// Need a break, insert at last space
		if (iLineRun >= iMaxLine)
			if (cpLastSpace)
			{
				*cpLastSpace = cSepa; iLineRun = cPos - cpLastSpace;
			}
		// Line run
		iLineRun++;
	}
}

const char *SAdvanceSpace(const char *szSPos)
{
	if (!szSPos) return nullptr;
	while (IsWhiteSpace(*szSPos)) szSPos++;
	return szSPos;
}

const char *SAdvancePast(const char *szSPos, char cPast)
{
	if (!szSPos) return nullptr;
	while (*szSPos)
	{
		if (*szSPos == cPast) { szSPos++; break; }
		szSPos++;
	}
	return szSPos;
}

void SCopyIdentifier(const char *szSource, char *sTarget, int iMaxL)
{
	if (!szSource || !sTarget) return;
	while (IsIdentifier(*szSource))
	{
		if (iMaxL == 1) { *sTarget++ = *szSource++; break; }
		iMaxL--;
		*sTarget++ = *szSource++;
	}
	*sTarget = 0;
}

int SClearFrontBack(char *szString, char cClear)
{
	int cleared = 0;
	char *cpos;
	if (!szString) return 0;
	for (cpos = szString; *cpos && (*cpos == cClear); cpos++, cleared++);
	// strcpy is undefined when used on overlapping strings...
	if (cpos != szString) memmove(szString, cpos, SLen(cpos) + 1);
	for (cpos = szString + SLen(szString) - 1; (cpos > szString) && (*cpos == cClear); cpos--, cleared++)
		*cpos = 0x00;
	return cleared;
}

void SNewSegment(char *szStr, const char *szSepa)
{
	if (szStr[0]) SAppend(szSepa, szStr);
}

int SGetLine(const char *szText, const char *cpPosition)
{
	if (!szText || !cpPosition) return 0;
	int iLines = 0;
	while (*szText && (szText < cpPosition))
	{
		if (*szText == 0x0A) iLines++;
		szText++;
	}
	return iLines;
}

int SLineGetCharacters(const char *szText, const char *cpPosition)
{
	if (!szText || !cpPosition) return 0;
	int iChars = 0;
	while (*szText && (szText < cpPosition))
	{
		if (*szText == 0x0A) iChars = 0;
		iChars++;
		szText++;
	}
	return iChars;
}

void SInsert(char *szString, const char *szInsert, int iPosition, int iMaxLen)
{
	// Safety
	if (!szString || !szInsert || !szInsert[0]) return;
	size_t insertlen = strlen(szInsert);
	if (iMaxLen >= 0 && strlen(szString) + insertlen > (size_t)iMaxLen) return;
	// Move up string remainder
	memmove(szString + iPosition + insertlen, szString + iPosition, SLen(szString + iPosition) + 1);
	// Copy insertion
	std::memmove(szString + iPosition, szInsert, SLen(szInsert));
}

void SDelete(char *szString, int iLen, int iPosition)
{
	// Safety
	if (!szString) return;
	// Move down string remainder
	std::memmove(szString + iPosition, szString + iPosition + iLen, SLen(szString + iPosition + iLen) + 1);
}

bool SCopyEnclosed(const char *szSource, char cOpen, char cClose, char *sTarget, int iSize)
{
	int iPos, iLen;
	if (!szSource || !sTarget) return false;
	if ((iPos = SCharPos(cOpen, szSource)) < 0) return false;
	if ((iLen = SCharPos(cClose, szSource + iPos + 1)) < 0) return false;
	SCopy(szSource + iPos + 1, sTarget, (std::min)(iLen, iSize));
	return true;
}

bool SGetModule(const char *szList, int iIndex, char *sTarget, int iSize)
{
	if (!szList || !sTarget) return false;
	if (!SCopySegment(szList, iIndex, sTarget, ';', iSize)) return false;
	SClearFrontBack(sTarget);
	return true;
}

bool SIsModule(const char *szList, const char *szString, int *ipIndex, bool fCaseSensitive)
{
	char szModule[1024 + 1];
	// Compare all modules
	for (int iMod = 0; SGetModule(szList, iMod, szModule, 1024); iMod++)
		if (fCaseSensitive ? SEqual(szString, szModule) : SEqualNoCase(szString, szModule))
		{
			// Provide index if desired
			if (ipIndex) *ipIndex = iMod;
			// Found
			return true;
		}
	// Not found
	return false;
}

bool SAddModule(char *szList, const char *szModule, bool fCaseSensitive)
{
	// Safety / no empties
	if (!szList || !szModule || !szModule[0]) return false;
	// Already a module?
	if (SIsModule(szList, szModule, nullptr, fCaseSensitive)) return false;
	// New segment, add string
	SNewSegment(szList);
	SAppend(szModule, szList);
	// Success
	return true;
}

bool SAddModules(char *szList, const char *szModules, bool fCaseSensitive)
{
	// Safety / no empties
	if (!szList || !szModules || !szModules[0]) return false;
	// Add modules
	char szModule[1024 + 1]; // limited
	for (int cnt = 0; SGetModule(szModules, cnt, szModule, 1024); cnt++)
		SAddModule(szList, szModule, fCaseSensitive);
	// Success
	return true;
}

bool SRemoveModule(char *szList, const char *szModule, bool fCaseSensitive)
{
	int iMod, iPos, iLen;
	// Not a module
	if (!SIsModule(szList, szModule, &iMod, fCaseSensitive)) return false;
	// Get module start
	iPos = 0;
	if (iMod > 0) iPos = SCharPos(';', szList, iMod - 1) + 1;
	// Get module length
	iLen = SCharPos(';', szList + iPos);
	if (iLen < 0) iLen = SLen(szList); else iLen += 1;
	// Delete module
	SDelete(szList, iLen, iPos);
	// Success
	return true;
}

bool SRemoveModules(char *szList, const char *szModules, bool fCaseSensitive)
{
	// Safety / no empties
	if (!szList || !szModules || !szModules[0]) return false;
	// Remove modules
	char szModule[1024 + 1]; // limited
	for (int cnt = 0; SGetModule(szModules, cnt, szModule, 1024); cnt++)
		SRemoveModule(szList, szModule, fCaseSensitive);
	// Success
	return true;
}

int SModuleCount(const char *szList)
{
	if (!szList) return 0;
	int iCount = 0;
	bool fNewModule = true;
	while (*szList)
	{
		switch (*szList)
		{
		case ' ': break;
		case ';': fNewModule = true; break;
		default: if (fNewModule) iCount++; fNewModule = false; break;
		}
		szList++;
	}
	return iCount;
}

bool SWildcardMatchEx(const char *szString, const char *szWildcard)
{
	// safety
	if (!szString || !szWildcard) return false;
	// match char-wise
	const char *pWild = szWildcard, *pPos = szString;
	const char *pLWild = nullptr, *pLPos = nullptr; // backtracking
	while (*pWild || pLWild)
		// string wildcard?
		if (*pWild == '*')
		{
			pLWild = ++pWild; pLPos = pPos;
		}
		// nothing left to match?
		else if (!*pPos)
			break;
		// equal or one-character-wildcard? proceed
		else if (*pWild == '?' || *pWild == *pPos)
		{
			pWild++; pPos++;
		}
		// backtrack possible?
		else if (pLPos)
		{
			pWild = pLWild; pPos = ++pLPos;
		}
		// match failed
		else
			return false;
	// match complete if both strings are fully matched
	return !*pWild && !*pPos;
}

const char *SGetParameter(const char *strCommandLine, int iParameter, char *strTarget, int iSize, bool *pWasQuoted)
{
	// Safeties
	if (iParameter < 0) return nullptr;
	// Parse command line which may contain spaced or quoted parameters
	static char strParameter[2048 + 1];
	const char *c = strCommandLine;
	bool fQuoted;
	while (c && *c)
	{
		// Quoted parameter
		if (fQuoted = (*c == '"'))
		{
			SCopyUntil(++c, strParameter, '"', 2048);
			c += SLen(strParameter);
			if (*c == '"') c++;
		}
		// Spaced parameter
		else
		{
			bool fWrongQuote = (SCharPos('"', c) > -1) && (SCharPos('"', c) < SCharPos(' ', c));
			SCopyUntil(c, strParameter, fWrongQuote ? '"' : ' ', 2048);
			c += (std::max)(SLen(strParameter), 1);
		}
		// Process (non-empty) parameter
		if (strParameter[0])
		{
			// Success
			if (iParameter == 0)
			{
				if (strTarget) SCopy(strParameter, strTarget, iSize);
				if (pWasQuoted) *pWasQuoted = fQuoted;
				return strParameter;
			}
			// Continue
			else
				iParameter--;
		}
	}
	// Not found
	return nullptr;
}

bool IsSafeFormatString(const char *szFmt)
{
	for (const char *pPos = szFmt; *pPos; pPos++)
		if (*pPos == '%')
		{
			pPos++;
			if (*pPos == '-') pPos++;
			if (*pPos == '+') pPos++;
			if (*pPos == '0') pPos++;
			if (*pPos == ' ') pPos++;
			if (*pPos == '#') pPos++;
			while (isdigit(*pPos)) pPos++;
			if (*pPos == '.') pPos++;
			while (isdigit(*pPos)) pPos++;
			if (*pPos == 'h') pPos++;
			if (*pPos == 'l') pPos++;
			if (*pPos == 'L') pPos++;
			if (*pPos == 'z') pPos++;
			switch (tolower(*pPos))
			{
			case 'c': case 'd': case 'i': case 'o': case 'u': case 'x': case 'e': case 'f': case 'g': case 's': case '%': case 'p':
				break;
			default:
				return false;
			}
		}
	return true;
}

// Blitting

void StdBlit(uint8_t *bypSource, int iSourcePitch, int iSrcBufHgt,
	int iSrcX, int iSrcY, int iSrcWdt, int iSrcHgt,
	uint8_t *bypTarget, int iTargetPitch, int iTrgBufHgt,
	int iTrgX, int iTrgY, int iTrgWdt, int iTrgHgt,
	int iBytesPerPixel, bool fFlip)
{
	if (!bypSource || !bypTarget) return;
	if (!iTrgWdt || !iTrgHgt) return;
	int xcnt, ycnt, zcnt, sline, tline, fy;
	for (ycnt = 0; ycnt < iTrgHgt; ycnt++)
	{
		fy = iSrcHgt * ycnt / iTrgHgt;
		if (iSrcBufHgt > 0) sline = (iSrcBufHgt - 1 - iSrcY - fy) * iSourcePitch;
		else sline = (iSrcY + fy) * iSourcePitch;
		if (iTrgBufHgt > 0) tline = (iTrgBufHgt - 1 - iTrgY - ycnt) * iTargetPitch;
		else tline = (iTrgY + ycnt) * iTargetPitch;
		if (!fFlip)
		{
			for (xcnt = 0; xcnt < iTrgWdt; xcnt++)
				for (zcnt = 0; zcnt < iBytesPerPixel; zcnt++)
					bypTarget[tline + (iTrgX + xcnt) * iBytesPerPixel + zcnt]
					= bypSource[sline + (iSrcX + iSrcWdt * xcnt / iTrgWdt) * iBytesPerPixel + zcnt];
		}
		else
		{
			for (xcnt = 0; xcnt < iTrgWdt; xcnt++)
				for (zcnt = 0; zcnt < iBytesPerPixel; zcnt++)
					bypTarget[tline + (iTrgX + iTrgWdt - 1 - xcnt) * iBytesPerPixel + zcnt]
					= bypSource[sline + (iSrcX + iSrcWdt * xcnt / iTrgWdt) * iBytesPerPixel + zcnt];
		}
	}
}

// Global variables used by StdRandom

/* extern */ int RandomCount = 0;
/* extern */ unsigned int RandomHold = 0;

/* Some part of the Winapi */

#ifndef _WIN32

#include <sys/time.h>

unsigned long timeGetTime(void)
{
	static time_t sec_offset;
	timeval tv;
	gettimeofday(&tv, 0);
	if (!sec_offset) sec_offset = tv.tv_sec;
	return (tv.tv_sec - sec_offset) * 1000 + tv.tv_usec / 1000;
}

#endif

const char *GetCurrentTimeStamp(bool enableMarkupColor)
{
	static char buf[25];

	time_t timenow;
	time(&timenow);

	strftime(buf, sizeof(buf), enableMarkupColor ? "<c 909090>[%H:%M:%S]</c>" : "[%H:%M:%S]", localtime(&timenow));

	return buf;
}
