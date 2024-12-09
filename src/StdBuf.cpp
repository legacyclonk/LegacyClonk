/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
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

#include <Standard.h>
#include <StdBuf.h>
#include <StdCompiler.h>
#include <StdAdaptors.h>
#include <StdFile.h>

#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <ios>

#ifdef _WIN32
#include <io.h>
#else
#include <cstdlib>

#define O_BINARY 0
#define O_SEQUENTIAL 0
#include <unistd.h>
#endif
#include <fcntl.h>
#include <sys/stat.h>


// *** StdBuf

bool StdBuf::LoadFromFile(const char *szFile) try
{
	std::ifstream file{szFile, std::ios::binary};
	// Create buf
	New(FileSize(szFile));
	return file && file.read(static_cast<char *>(getMData()), getSize());
}
catch (const std::ios_base::failure &)
{
	return false;
}

bool StdBuf::SaveToFile(const char *szFile) const try
{
	std::ofstream file{szFile, std::ios::binary | std::ios::trunc};
	return file && file.write(static_cast<const char *>(getData()), getSize());
}
catch (const std::ios_base::failure &)
{
	return false;
}

bool StdStrBuf::LoadFromFile(const char *szFile) try
{
	std::ifstream file{szFile, std::ios::binary};
	// Create buf
	SetLength(FileSize(szFile));
	return file && file.read(getMData(), getLength());
}
catch (const std::ios_base::failure &)
{
	return false;
}

bool StdStrBuf::SaveToFile(const char *szFile) const try
{
	std::ofstream file{szFile, std::ios::binary | std::ios::trunc};
	return file && file.write(getData(), getLength());
}
catch (const std::ios_base::failure &)
{
	return false;
}

void StdBuf::CompileFunc(StdCompiler *pComp, int iType)
{
	// Size (guess it is a small value most of the time - if it's big, an extra byte won't hurt anyway)
	auto tmp = static_cast<uint32_t>(iSize); pComp->Value(mkIntPackAdapt(tmp)); iSize = tmp;
	pComp->Separator(StdCompiler::SEP_PART2);
	// Read/write data
	if (pComp->isCompiler())
	{
		New(iSize);
		pComp->Raw(getMData(), iSize, StdCompiler::RawCompileType(iType));
	}
	else
	{
		pComp->Raw(const_cast<void *>(getData()), iSize, StdCompiler::RawCompileType(iType));
	}
}

// *** StdStringBuf

void StdStrBuf::CompileFunc(StdCompiler *pComp, int iRawType)
{
	if (pComp->isCompiler())
	{
		std::string data;
		pComp->String(data, StdCompiler::RawCompileType(iRawType));
		Copy(data.c_str(), data.size());
	}
	else
	{
		// pData is only read anyway, since it is a decompiler
		const char *data{getData()};
		if (!data)
		{
			data = "";
		}
		pComp->String(data, getLength(), StdCompiler::RawCompileType(iRawType));
	}
}

// replace all occurences of one string with another. Return number of replacements.
int StdStrBuf::Replace(const char *szOld, const char *szNew, size_t iStartSearch)
{
	if (!getPtr(0) || !szOld) return 0;
	if (!szNew) szNew = "";
	int cnt = 0;
	size_t iOldLen = std::strlen(szOld), iNewLen = std::strlen(szNew);
	if (iOldLen != iNewLen)
	{
		// count number of occurences to calculate new string length
		size_t iResultLen = getLength();
		const char *szPos = getPtr(iStartSearch);
		while (szPos = SSearch(szPos, szOld))
		{
			iResultLen += iNewLen - iOldLen;
			++cnt;
		}
		if (!cnt) return 0;
		// now construct new string by replacement
		StdStrBuf sResult;
		sResult.New(iResultLen + 1);
		const char *szRPos = getPtr(0), *szRNextPos;
		char *szWrite = sResult.getMPtr(0);
		if (iStartSearch)
		{
			std::memcpy(szWrite, szRPos, iStartSearch * sizeof(char));
			szRPos += iStartSearch;
			szWrite += iStartSearch;
		}
		while (szRNextPos = SSearch(szRPos, szOld))
		{
			std::memcpy(szWrite, szRPos, (szRNextPos - szRPos - iOldLen) * sizeof(char));
			szWrite += (szRNextPos - szRPos - iOldLen);
			std::memcpy(szWrite, szNew, iNewLen * sizeof(char));
			szWrite += iNewLen;
			szRPos = szRNextPos;
		}
		strcpy(szWrite, szRPos);
		Take(sResult);
	}
	else
	{
		// replace directly in this string
		char *szRPos = getMPtr(iStartSearch);
		while (szRPos = const_cast<char *>(SSearch(szRPos, szOld)))
		{
			std::memcpy(szRPos - iOldLen, szNew, iOldLen * sizeof(char));
			++cnt;
		}
	}
	return cnt;
}

int StdStrBuf::ReplaceChar(char cOld, char cNew, size_t iStartSearch)
{
	if (isNull()) return 0;
	char *szPos = getMPtr(0);
	if (!cOld) return 0;
	if (!cNew) cNew = '_';
	int cnt = 0;
	while (szPos = std::strchr(szPos, cOld))
	{
		*szPos++ = cNew;
		++cnt;
	}
	return cnt;
}

void StdStrBuf::ReplaceEnd(size_t iPos, const char *szNewEnd)
{
	size_t iLen = getLength();
	assert(iPos <= iLen); if (iPos > iLen) return;
	size_t iEndLen = std::strlen(szNewEnd);
	if (iLen - iPos != iEndLen) SetLength(iPos + iEndLen);
	std::memcpy(getMPtr(iPos), szNewEnd, iEndLen * sizeof(char));
}

bool StdStrBuf::ValidateChars(const char *szInitialChars, const char *szMidChars)
{
	// only given chars may be in string
	for (size_t i = 0; i < getLength(); ++i)
		if (!std::strchr(i ? szMidChars : szInitialChars, getData()[i]))
			return false;
	return true;
}

bool StdStrBuf::GetSection(size_t idx, StdStrBuf *psOutSection, char cSeparator) const
{
	assert(psOutSection);
	psOutSection->Clear();
	const char *szStr = getData(), *szSepPos;
	if (!szStr) return false; // invalid argument
	while ((szSepPos = std::strchr(szStr, cSeparator)) && idx) { szStr = szSepPos + 1; --idx; }
	if (idx) return false; // indexed section not found
	// fill output buffer with section, if not empty
	if (!szSepPos) szSepPos = getData() + getLength();
	if (szSepPos != szStr) psOutSection->Copy(szStr, szSepPos - szStr);
	// return true even if section is empty, because the section obviously exists
	// (to enable loops like while (buf.GetSection(i++, &sect)) if (sect) ...)
	return true;
}

void StdStrBuf::EnsureUnicode()
{
	bool valid = true;
	int need_continuation_bytes = 0;
	// Check wether valid UTF-8
	for (size_t i = 0; i < getSize(); ++i)
	{
		unsigned char c = *getPtr(i);
		// remaining of a code point started before
		if (need_continuation_bytes)
		{
			--need_continuation_bytes;
			// (10000000-10111111)
			if (0x80 <= c && c <= 0xBF)
				continue;
			else
			{
				valid = false;
				break;
			}
		}
		// ASCII
		if (c < 0x80)
			continue;
		// Two byte sequence (11000010-11011111)
		// Note: 1100000x is an invalid overlong sequence
		if (0xC2 <= c && c <= 0xDF)
		{
			need_continuation_bytes = 1;
			continue;
		}
		// Three byte sequence (11100000-11101111)
		if (0xE0 <= c && c <= 0xEF)
		{
			need_continuation_bytes = 2;
			continue;
			// FIXME: could check for UTF-16 surrogates from a broken utf-16->utf-8 converter here
		}
		// Four byte sequence (11110000-11110100)
		if (0xF0 <= c && c <= 0xF4)
		{
			need_continuation_bytes = 3;
			continue;
		}
		valid = false;
		break;
	}
	if (need_continuation_bytes)
		valid = false;
	// assume that it's windows-1252 and convert to utf-8
	if (!valid)
	{
		size_t j = 0;
		StdStrBuf buf;
		buf.Grow(getLength());
		// totally unfounded statistic: most texts have less than 20 umlauts.
		enum { GROWSIZE = 20 };
		for (size_t i = 0; i < getSize(); ++i)
		{
			unsigned char c = *getPtr(i);
			if (c < 0x80)
			{
				if (j >= buf.getLength())
					buf.Grow(GROWSIZE);
				*buf.getMPtr(j++) = c;
				continue;
			}
			if (0xA0 <= c)
			{
				if (j + 1 >= buf.getLength())
					buf.Grow(GROWSIZE);
				*buf.getMPtr(j++) = (0xC0 | c >> 6);
				*buf.getMPtr(j++) = (0x80 | c & 0x3F);
				continue;
			}
			// Extra windows-1252-characters
			buf.SetLength(j);
			// Let's hope that no editor mangles these UTF-8 strings...
			static const char *extra_chars[] =
			{
				"€", "?", "‚", "ƒ", "„", "…", "†", "‡", "ˆ", "‰", "Š", "‹", "Œ", "?", "Ž", "?",
				"?", "‘", "’", "“", "”", "•", "–", "—", "˜", "™", "š", "›", "œ", "?", "ž", "Ÿ"
			};
			buf.Append(extra_chars[c - 0x80]);
			j += std::strlen(extra_chars[c - 0x80]);
		}
		buf.SetLength(j);
		Take(buf);
	}
}

bool StdStrBuf::TrimSpaces()
{
	// get left trim
	size_t iSpaceLeftCount = 0, iLength = getLength();
	if (!iLength) return false;
	const char *szStr = getData();
	while (iSpaceLeftCount < iLength)
		if (std::isspace(static_cast<unsigned char>(szStr[iSpaceLeftCount])))
			++iSpaceLeftCount;
		else
			break;
	// only spaces? Clear!
	if (iSpaceLeftCount == iLength)
	{
		Clear();
		return true;
	}
	// get right trim
	size_t iSpaceRightCount = 0;
	while (std::isspace(static_cast<unsigned char>(szStr[iLength - 1 - iSpaceRightCount]))) ++iSpaceRightCount;
	// anything to trim?
	if (!iSpaceLeftCount && !iSpaceRightCount) return false;
	// only right trim? Can do this by shortening
	if (!iSpaceLeftCount)
	{
		SetLength(iLength - iSpaceRightCount);
		return true;
	}
	// left trim involved - move text and shorten
	std::memmove(getMPtr(0), szStr + iSpaceLeftCount, iLength - iSpaceLeftCount - iSpaceRightCount);
	SetLength(iLength - iSpaceLeftCount - iSpaceRightCount);
	return true;
}
