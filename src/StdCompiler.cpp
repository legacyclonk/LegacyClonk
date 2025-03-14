/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2017, The OpenClonk Team and contributors
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

#include "StdCompiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <algorithm>
#include <cinttypes>
#include <cstring>
#include <format>
#include <utility>

StdCompiler::NameGuard::NameGuard(NameGuard &&other) noexcept
	: compiler{std::exchange(other.compiler, nullptr)}, foundName{other.foundName} {}

StdCompiler::NameGuard &StdCompiler::NameGuard::operator=(NameGuard &&other) noexcept
{
	compiler = std::exchange(other.compiler, nullptr);
	foundName = other.foundName;
	return *this;
}

StdCompiler::NameGuard::~NameGuard()
{
	End();
}

void StdCompiler::NameGuard::End()
{
	if (compiler)
	{
		compiler->NameEnd();
		compiler = nullptr;
	}
}

void StdCompiler::NameGuard::Abort()
{
	if (compiler)
	{
		compiler->NameEnd(true);
		compiler = nullptr;
	}
}

void StdCompiler::NameGuard::Disarm() noexcept
{
	compiler = nullptr;
}

// *** StdCompiler


char StdCompiler::SeparatorToChar(Sep eSep)
{
	switch (eSep)
	{
	case SEP_SEP: return ',';
	case SEP_SEP2: return ';';
	case SEP_SET: return '=';
	case SEP_PART: return '.';
	case SEP_PART2: return ':';
	case SEP_PLUS: return '+';
	case SEP_START: return '(';
	case SEP_END: return ')';
	case SEP_START2: return '[';
	case SEP_END2: return ']';
	case SEP_VLINE: return '|';
	case SEP_DOLLAR: return '$';
	}
	assert(false);
	return ' ';
}

bool StdCompiler::IsIdentifierChar(char c) noexcept
{
	return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-';
}

bool StdCompiler::IsIdentifier(std::string_view str)
{
	return std::all_of(begin(str), end(str), StdCompiler::IsIdentifierChar);
}

// *** StdCompilerBinWrite

void StdCompilerBinWrite::QWord    (int64_t  &rInt)   { WriteValue(rInt); }
void StdCompilerBinWrite::QWord    (uint64_t  &rInt)  { WriteValue(rInt); }
void StdCompilerBinWrite::DWord    (int32_t  &rInt)   { WriteValue(rInt); }
void StdCompilerBinWrite::DWord    (uint32_t &rInt)   { WriteValue(rInt); }
void StdCompilerBinWrite::Word     (int16_t  &rShort) { WriteValue(rShort); }
void StdCompilerBinWrite::Word     (uint16_t &rShort) { WriteValue(rShort); }
void StdCompilerBinWrite::Byte     (int8_t   &rByte)  { WriteValue(rByte); }
void StdCompilerBinWrite::Byte     (uint8_t  &rByte)  { WriteValue(rByte); }
void StdCompilerBinWrite::Boolean  (bool     &rBool)  { WriteValue(rBool); }
void StdCompilerBinWrite::Character(char     &rChar)  { WriteValue(rChar); }

void StdCompilerBinWrite::String(char *szString, size_t iMaxLength, RawCompileType eType)
{
	WriteData(szString, strlen(szString) + 1);
}

void StdCompilerBinWrite::String(std::string &str, RawCompileType type)
{
	WriteData(str.c_str(), str.size() + 1);
}

template <class T>
void StdCompilerBinWrite::WriteValue(const T &rValue)
{
	// Copy data
	if (fSecondPass)
		*Buf.getMPtr<T>(iPos) = rValue;
	iPos += sizeof(rValue);
}

void StdCompilerBinWrite::WriteData(const void *pData, size_t iSize)
{
	// Copy data
	if (fSecondPass)
		Buf.Write(pData, iSize, iPos);
	iPos += iSize;
}

void StdCompilerBinWrite::Raw(void *pData, size_t iSize, RawCompileType eType)
{
	// Copy data
	if (fSecondPass)
		Buf.Write(pData, iSize, iPos);
	iPos += iSize;
}

void StdCompilerBinWrite::Begin()
{
	fSecondPass = false; iPos = 0;
}

void StdCompilerBinWrite::BeginSecond()
{
	Buf.New(iPos);
	fSecondPass = true; iPos = 0;
}

// *** StdCompilerBinRead

void StdCompilerBinRead::QWord(int64_t &rInt) { ReadValue(rInt); }
void StdCompilerBinRead::QWord(uint64_t &rInt) { ReadValue(rInt); }
void StdCompilerBinRead::DWord(int32_t &rInt) { ReadValue(rInt); }
void StdCompilerBinRead::DWord(uint32_t &rInt) { ReadValue(rInt); }
void StdCompilerBinRead::Word(int16_t &rShort) { ReadValue(rShort); }
void StdCompilerBinRead::Word(uint16_t &rShort) { ReadValue(rShort); }
void StdCompilerBinRead::Byte(int8_t &rByte) { ReadValue(rByte); }
void StdCompilerBinRead::Byte(uint8_t &rByte) { ReadValue(rByte); }
void StdCompilerBinRead::Boolean(bool &rBool) { ReadValue(rBool); }
void StdCompilerBinRead::Character(char &rChar) { ReadValue(rChar); }

void StdCompilerBinRead::String(char *szString, size_t iMaxLength, RawCompileType eType)
{
	// At least one byte data needed
	if (iPos >= Buf.getSize())
	{
		excEOF(); return;
	}
	// Copy until no data left
	char *pPos = szString;
	while (*pPos++ = *Buf.getPtr<char>(iPos++))
		if (iPos >= Buf.getSize())
		{
			excEOF(); return;
		}
		else if (pPos > szString + iMaxLength)
		{
			excCorrupt("string too long"); return;
		}
}

void StdCompilerBinRead::String(std::string &str, RawCompileType type)
{
	// At least one byte data needed
	if (iPos >= Buf.getSize())
	{
		excEOF(); return;
	}
	const auto iStart = iPos;
	// Search string end
	while (*Buf.getPtr<char>(iPos++))
		if (iPos >= Buf.getSize())
		{
			excEOF(); return;
		}
	// Copy data
	str.assign(Buf.getPtr<char>(iStart), Buf.getPtr<char>(iPos - 1));
}

void StdCompilerBinRead::Raw(void *pData, size_t iSize, RawCompileType eType)
{
	if (iPos + iSize > Buf.getSize())
	{
		excEOF(); return;
	}
	// Copy data
	memcpy(pData, Buf.getPtr(iPos), iSize);
	iPos += iSize;
}

std::string StdCompilerBinRead::getPosition() const
{
	return std::format("byte {}", iPos);
}

template <class T>
inline void StdCompilerBinRead::ReadValue(T &rValue)
{
	// Don't read beyond end of buffer
	if (iPos + sizeof(T) > Buf.getSize())
	{
		excEOF(); return;
	}
	// Copy
	rValue = *Buf.getPtr<T>(iPos);
	iPos += sizeof(T);
}

void StdCompilerBinRead::Begin()
{
	iPos = 0;
}

// *** StdCompilerINIWrite

StdCompiler::NameGuard StdCompilerINIWrite::Name(const char *szName)
{
	// Sub-Namesections exist, so it's a section. Write name if not already done so.
	if (fPutName) PutName(true);
	// Push struct
	Naming *pnNaming = new Naming;
	pnNaming->Name.Copy(szName);
	pnNaming->Parent = pNaming;
	pNaming = pnNaming;
	iDepth++;
	// Done
	fPutName = true; fInSection = false;
	return {this, true};
}

void StdCompilerINIWrite::NameEnd(bool fBreak)
{
	// Append newline
	if (!fPutName && !fInSection)
		buf += "\r\n";
	fPutName = false;
	// Note this makes it impossible to distinguish an empty name section from
	// a non-existing name section.

	// Pop
	assert(iDepth);
	Naming *poNaming = pNaming;
	pNaming = poNaming->Parent;
	delete poNaming;
	iDepth--;
	// We're inside a section now
	fInSection = true;
}

bool StdCompilerINIWrite::Separator(Sep eSep)
{
	if (fInSection)
	{
		// Re-put section name
		PutName(true);
	}
	else
	{
		PrepareForValue();
		buf += SeparatorToChar(eSep);
	}
	return true;
}

void StdCompilerINIWrite::QWord(int64_t &rInt)
{
	PrepareForValue();
	buf += std::format("{}", rInt);
}

void StdCompilerINIWrite::QWord(uint64_t &rInt)
{
	PrepareForValue();
	buf += std::format("{}", rInt);
}

void StdCompilerINIWrite::DWord(int32_t &rInt)
{
	PrepareForValue();
	buf += std::format("{}", rInt);
}

void StdCompilerINIWrite::DWord(uint32_t &rInt)
{
	PrepareForValue();
	buf += std::format("{}", rInt);
}

void StdCompilerINIWrite::Word(int16_t &rInt)
{
	PrepareForValue();
	buf += std::format("{}", rInt);
}

void StdCompilerINIWrite::Word(uint16_t &rInt)
{
	PrepareForValue();
	buf += std::format("{}", rInt);
}

void StdCompilerINIWrite::Byte(int8_t &rByte)
{
	PrepareForValue();
	buf += std::format("{:d}", rByte);
}

void StdCompilerINIWrite::Byte(uint8_t &rInt)
{
	PrepareForValue();
	buf += std::format("{:d}", rInt);
}

void StdCompilerINIWrite::Boolean(bool &rBool)
{
	PrepareForValue();
	buf += rBool ? "true" : "false";
}

void StdCompilerINIWrite::Character(char &rChar)
{
	PrepareForValue();
	buf += rChar;
}

void StdCompilerINIWrite::String(char *szString, size_t iMaxLength, RawCompileType eType)
{
	StringN(szString, iMaxLength, eType);
}

void StdCompilerINIWrite::StringN(const char *szString, size_t iMaxLength, RawCompileType eType)
{
	PrepareForValue();
	switch (eType)
	{
	case RCT_Escaped:
		WriteEscaped(szString, szString + strlen(szString));
		break;
	case RCT_All:
	case RCT_Idtf:
	case RCT_IdtfAllowEmpty:
	case RCT_ID:
		buf += szString;
	}
}

void StdCompilerINIWrite::String(std::string &str, RawCompileType type)
{
	StringN(str.c_str(), str.size(), type);
}

void StdCompilerINIWrite::Raw(void *pData, size_t iSize, RawCompileType eType)
{
	switch (eType)
	{
	case RCT_Escaped:
		WriteEscaped(reinterpret_cast<char *>(pData), reinterpret_cast<char *>(pData) + iSize);
		break;
	case RCT_All:
	case RCT_Idtf:
	case RCT_IdtfAllowEmpty:
	case RCT_ID:
		buf.append(reinterpret_cast<char *>(pData), iSize);
	}
}

void StdCompilerINIWrite::Begin()
{
	pNaming = nullptr;
	fPutName = false;
	iDepth = 0;
	fInSection = false;
	buf.clear();
}

void StdCompilerINIWrite::End()
{
	// Ensure all namings were closed properly
	assert(!iDepth);
}

void StdCompilerINIWrite::PrepareForValue()
{
	// Put name (value-type), if not already done so
	if (fPutName) PutName(false);
	// No data allowed inside of sections
	assert(!fInSection);
	// No values allowed on top-level - must be contained in at least one section
	assert(iDepth > 1);
}

void StdCompilerINIWrite::WriteEscaped(const char *szString, const char *pEnd)
{
	buf += '"';
	// Try to write chunks as huge as possible of "normal" chars.
	// Note this excludes '\0', so the standard Append() can be used.
	const char *pStart, *pPos; pStart = pPos = szString;
	bool fLastNumEscape = false; // catch "\1""1", which must become "\1\61"
	for (; pPos < pEnd; pPos++)
		if (!isprint(static_cast<unsigned char>(*pPos)) || *pPos == '\\' || *pPos == '"' || (fLastNumEscape && isdigit(static_cast<unsigned char>(*pPos))))
		{
			// Write everything up to this point
			if (pPos - pStart) buf.append(pStart, pPos - pStart);
			// Escape
			fLastNumEscape = false;
			switch (*pPos)
			{
			case '\a': buf += "\\a"; break;
			case '\b': buf += "\\b"; break;
			case '\f': buf += "\\f"; break;
			case '\n': buf += "\\n"; break;
			case '\r': buf += "\\r"; break;
			case '\t': buf += "\\t"; break;
			case '\v': buf += "\\v"; break;
			case '\"': buf += "\\\""; break;
			case '\\': buf += "\\\\"; break;
			default:
				buf += std::format("\\{:o}", *reinterpret_cast<const unsigned char *>(pPos));
				fLastNumEscape = true;
			}
			// Set pointer
			pStart = pPos + 1;
		}
		else
			fLastNumEscape = false;
	// Write the rest
	if (pEnd - pStart) buf.append(pStart, pEnd - pStart);
	buf += '"';
}

void StdCompilerINIWrite::WriteIndent(bool fSection)
{
	// Do not indent level 1 (level 0 values aren't allowed - see above)
	int iIndent = iDepth - 1;
	// Sections are indented more, even though they belong to this level
	if (!fSection) iIndent--;
	// Do indention
	if (iIndent <= 0) return;
	buf.append(iIndent * 2, ' ');
}

void StdCompilerINIWrite::PutName(bool fSection)
{
	if (fSection && !buf.empty())
		buf += "\r\n";
	WriteIndent(fSection);
	// Put name
	if (fSection)
		buf += std::format("[{}]\r\n", pNaming->Name.getData());
	else
		buf += std::format("{}=", pNaming->Name.getData());
	// Set flag
	fPutName = false;
}

// *** StdCompilerINIRead

StdCompilerINIRead::StdCompilerINIRead()
	: pNameRoot(nullptr), iDepth(0), iRealDepth(0) {}

StdCompilerINIRead::~StdCompilerINIRead()
{
	FreeNameTree();
}

// Naming
StdCompiler::NameGuard StdCompilerINIRead::Name(const char *szName)
{
	// Increase depth
	iDepth++;
	// Parent category virtual?
	if (iDepth - 1 > iRealDepth)
		return {this, false};
	// Name must be alphanumerical and non-empty (force it)
	if (!isalpha(static_cast<unsigned char>(*szName)))
	{
		assert(false); return {this, false};
	}
	for (const char *p = szName + 1; *p; p++)
		// C4Update needs Name**...
		if (!isalnum(static_cast<unsigned char>(*p)) && *p != ' ' && *p != '_' && *p != '*')
		{
			assert(false); return {this, false};
		}
	// Search name
	NameNode *pNode;
	for (pNode = pName->FirstChild; pNode; pNode = pNode->NextChild)
		if (pNode->Pos && pNode->Name == szName)
			break;
	// Not found?
	if (!pNode)
	{
		NotFoundName = szName;
		return {this, false};
	}
	// Save tree position, indicate success
	pName = pNode;
	pPos = pName->Pos;
	pReenter = nullptr;
	iRealDepth++;
	return {this, true};
}

void StdCompilerINIRead::NameEnd(bool fBreak)
{
	assert(iDepth > 0);
	if (iRealDepth == iDepth)
	{
		// Remove childs
		for (NameNode *pNode = pName->FirstChild, *pNext; pNode; pNode = pNext)
		{
			// Report unused entries
			if (pNode->Pos && !fBreak)
				Warn("Unexpected {} \"{}\"!", pNode->Section ? "section" : "value", pNode->Name.getData());
			// delete node
			pNext = pNode->NextChild;
			delete pNode;
		}
		// Remove name so it won't be found again
		NameNode *pParent = pName->Parent;
		(pName->PrevChild ? pName->PrevChild->NextChild : pParent->FirstChild) = pName->NextChild;
		(pName->NextChild ? pName->NextChild->PrevChild : pParent->LastChild)  = pName->PrevChild;
		delete pName;
		// Go up
		pName = pParent;
		iRealDepth--;
	}
	// Decrease depth
	iDepth--;
	// This is the middle of nowhere
	pPos = nullptr; pReenter = nullptr;
}

bool StdCompilerINIRead::FollowName(const char *szName)
{
	// Current naming virtual?
	if (iDepth > iRealDepth)
		return false;
	// Next section must be the one
	if (!pName->NextChild || pName->NextChild->Name != szName)
	{
		// End current naming
		NameEnd();
		// Go into virtual naming
		iDepth++;
		return false;
	}
	// End current naming
	NameEnd();
	// Start new one
	Name(szName).Disarm();
	// Done
	return true;
}

// Separators
bool StdCompilerINIRead::Separator(Sep eSep)
{
	if (iDepth > iRealDepth) return false;
	// In section?
	if (pName->Section)
	{
		// Store current name, search another section with the same name
		StdStrBuf CurrName;
		CurrName.Take(pName->Name);
		NameEnd();
		auto guard = Name(CurrName.getData());
		guard.Disarm();
		return static_cast<bool>(guard);
	}
	// Position saved back from separator mismatch?
	if (pReenter) { pPos = pReenter; pReenter = nullptr; }
	// Nothing to read?
	if (!pPos) return false;
	// Read (while skipping over whitespace)
	SkipWhitespace();
	// Separator mismatch? Let all read attempts fail until the correct separator is found or the naming ends.
	if (*pPos != SeparatorToChar(eSep)) { pReenter = pPos; pPos = nullptr; return false; }
	// Go over separator, success
	pPos++;
	return true;
}

void StdCompilerINIRead::NoSeparator()
{
	// Position saved back from separator mismatch?
	if (pReenter) { pPos = pReenter; pReenter = nullptr; }
}

int StdCompilerINIRead::NameCount(const char *szName)
{
	// not in virtual naming
	if (iDepth > iRealDepth || !pName) return 0;
	// count within current name
	int iCount = 0;
	NameNode *pNode;
	for (pNode = pName->FirstChild; pNode; pNode = pNode->NextChild)
		// if no name is given, all valid subsections are counted
		if (pNode->Pos && (!szName || pNode->Name == szName))
			++iCount;
	return iCount;
}

// Various data readers
void StdCompilerINIRead::QWord(int64_t &rInt)
{
	rInt = ReadNum(strtoll);
}

void StdCompilerINIRead::QWord(uint64_t &rInt)
{
	rInt = ReadNum(strtoull);
}

void StdCompilerINIRead::DWord(int32_t &rInt)
{
	rInt = ReadNum(strtol);
}

void StdCompilerINIRead::DWord(uint32_t &rInt)
{
	rInt = ReadNum(strtoul);
}

void StdCompilerINIRead::Word(int16_t &rShort)
{
	const int MIN = -(1 << 15), MAX = (1 << 15) - 1;
	int iNum = ReadNum(strtol);
	if (iNum < MIN || iNum > MAX)
		Warn("number out of range ({} to {}): {} ", MIN, MAX, iNum);
	rShort = BoundBy(iNum, MIN, MAX);
}

void StdCompilerINIRead::Word(uint16_t &rShort)
{
	const unsigned int MIN = 0, MAX = (1 << 16) - 1;
	unsigned int iNum = ReadNum(strtoul);
	if (iNum > MAX)
		Warn("number out of range ({} to {}): {} ", MIN, MAX, iNum);
	rShort = BoundBy(iNum, MIN, MAX);
}

void StdCompilerINIRead::Byte(int8_t &rByte)
{
	const int MIN = -(1 << 7), MAX = (1 << 7) - 1;
	int iNum = ReadNum(strtol);
	if (iNum < MIN || iNum > MAX)
		Warn("number out of range ({} to {}): {} ", MIN, MAX, iNum);
	rByte = BoundBy(iNum, MIN, MAX);
}

void StdCompilerINIRead::Byte(uint8_t &rByte)
{
	const unsigned int MIN = 0, MAX = (1 << 8) - 1;
	unsigned int iNum = ReadNum(strtoul);
	if (iNum > MAX)
		Warn("number out of range ({} to {}): {} ", MIN, MAX, iNum);
	rByte = BoundBy(iNum, MIN, MAX);
}

void StdCompilerINIRead::Boolean(bool &rBool)
{
	if (!pPos) { notFound("Boolean"); return; }
	if (*pPos == '1' && !isdigit(static_cast<unsigned char>(*(pPos + 1))))
	{
		rBool = true; pPos++;
	}
	else if (*pPos == '0' && !isdigit(static_cast<unsigned char>(*(pPos + 1))))
	{
		rBool = false; pPos++;
	}
	else if (SEqual2(pPos, "true"))
	{
		rBool = true; pPos += 4;
	}
	else if (SEqual2(pPos, "false"))
	{
		rBool = false; pPos += 5;
	}
	else
	{
		notFound("Boolean"); return;
	}
}

void StdCompilerINIRead::Character(char &rChar)
{
	if (!pPos || !isalpha(static_cast<unsigned char>(*pPos)))
	{
		notFound("Character"); return;
	}
	rChar = *pPos++;
}

void StdCompilerINIRead::String(char *szString, size_t iMaxLength, RawCompileType eType)
{
	// Read data
	StdBuf Buf = ReadString(iMaxLength, eType, true);
	// Copy
	SCopy(Buf.getPtr<char>(), szString, iMaxLength);
}

void StdCompilerINIRead::String(std::string &str, RawCompileType type)
{
	// For Backwards compatibility: Escaped strings default to normal strings if no escaped string is given
	if (type == RCT_Escaped && pPos && *pPos != '"') type = RCT_All;
	// Get length
	size_t iLength = GetStringLength(type);
	// Read data
	StdBuf Buf = ReadString(iLength, type, true);
	str = Buf.getPtr<char>();
}

void StdCompilerINIRead::Raw(void *pData, size_t iSize, RawCompileType eType)
{
	// Read data
	StdBuf Buf = ReadString(iSize, eType, false);
	// Correct size?
	if (Buf.getSize() != iSize)
		Warn("got {} bytes raw data, but {} bytes expected!", Buf.getSize(), iSize);
	// Copy
	std::memmove(pData, Buf.getData(), iSize);
}

std::string StdCompilerINIRead::getPosition() const
{
	if (pPos)
		return std::format("line {}", SGetLine(Buf.getData(), pPos));
	else if (iDepth == iRealDepth)
	{
		if (pName->Section)
		{
			return std::format("section \"{}\", after line {}", pName->Name.getData(), SGetLine(Buf.getData(), pName->Pos));
		}
		else
		{
			return std::format("value \"{}\", line {}", pName->Name.getData(), SGetLine(Buf.getData(), pName->Pos));
		}
	}
	else if (iRealDepth)
		return std::format("missing value/section \"{}\" inside section \"{}\" (line {})", NotFoundName.getData(), pName->Name.getData(), SGetLine(Buf.getData(), pName->Pos));
	else
		return std::format("missing value/section \"{}\"", NotFoundName.getData());
}

void StdCompilerINIRead::Begin()
{
	// Already running? This may happen if someone confuses Compile with Value.
	assert(!iDepth && !iRealDepth && !pNameRoot);
	// Create tree
	CreateNameTree();
	// Start must be inside a section
	iDepth = iRealDepth = 0;
	pPos = nullptr; pReenter = nullptr;
}

void StdCompilerINIRead::End()
{
	assert(!iDepth && !iRealDepth);
	FreeNameTree();
}

void StdCompilerINIRead::CreateNameTree()
{
	FreeNameTree();
	// Create root node
	pName = pNameRoot = new NameNode();
	// No input? Stop
	if (!Buf) return;
	// Start scanning
	pPos = Buf.getPtr(0);
	while (*pPos)
	{
		// Go over whitespace
		int iIndent = 0;
		while (*pPos == ' ' || *pPos == '\t')
		{
			pPos++; iIndent++;
		}
		// Name/Section?
		bool fSection = *pPos == '[' && isalpha(static_cast<unsigned char>(*(pPos + 1)));
		if (fSection || isalpha(static_cast<unsigned char>(*pPos)))
		{
			// Treat values as if they had more indention
			// (so they become children of sections on the same level)
			if (!fSection) iIndent++; else pPos++;
			// Go up in tree structure if there is less indention
			while (pName->Parent && pName->Indent >= iIndent)
				pName = pName->Parent;
			// Copy name
			StdStrBuf Name;
			while (isalnum(static_cast<unsigned char>(*pPos)) || *pPos == ' ' || *pPos == '_')
				Name.AppendChar(*pPos++);
			while (*pPos == ' ' || *pPos == '\t') pPos++;
			if (*pPos != (fSection ? ']' : '='))
			{
				// Warn, ignore
				if (isprint(static_cast<unsigned char>(*pPos)))
				{
					Warn("Unexpected character ('{}'): %s ignored", unsigned(*pPos), fSection ? "section" : "value");
				}
				else
				{
					Warn("Unexpected character ('{:#02x}'): %s ignored", unsigned(*pPos), fSection ? "section" : "value");
				}
			}
			else
			{
				pPos++;
				// Create new node
				NameNode *pPrev = pName->LastChild;
				pName =
					pName->LastChild =
					(pName->LastChild ? pName->LastChild->NextChild : pName->FirstChild) =
					new NameNode(pName);
				pName->PrevChild = pPrev;
				pName->Name.Take(Name);
				pName->Pos = pPos;
				pName->Indent = iIndent;
				pName->Section = fSection;
				// Values don't have children (even if the indention looks like it)
				if (!fSection)
					pName = pName->Parent;
			}
		}
		// Skip line
		while (*pPos && (*pPos != '\n' && *pPos != '\r'))
			pPos++;
		while (*pPos == '\n' || *pPos == '\r')
			pPos++;
	}
	// Set pointer back
	pName = pNameRoot;
}

void StdCompilerINIRead::FreeNameTree()
{
	// free all nodes
	FreeNameNode(pNameRoot);
	pName = pNameRoot = nullptr;
}

void StdCompilerINIRead::FreeNameNode(NameNode *pDelNode)
{
	NameNode *pNode = pDelNode;
	while (pNode)
	{
		if (pNode->FirstChild)
			pNode = pNode->FirstChild;
		else
		{
			NameNode *pDelete = pNode;
			if (pDelete == pDelNode) { delete pDelete; break; }
			if (pNode->NextChild)
				pNode = pNode->NextChild;
			else
			{
				pNode = pNode->Parent;
				if (pNode) pNode->FirstChild = nullptr;
			}
			delete pDelete;
		}
	}
}

void StdCompilerINIRead::SkipWhitespace()
{
	while (*pPos == ' ' || *pPos == '\t')
		pPos++;
}

size_t StdCompilerINIRead::GetStringLength(RawCompileType eRawType)
{
	// Excpect valid position
	if (!pPos)
	{
		notFound("String"); return 0;
	}
	// Skip whitespace
	SkipWhitespace();
	// Save position
	const char *pStart = pPos;
	// Escaped? Go over '"'
	if (eRawType == RCT_Escaped && *pPos++ != '"')
	{
		notFound("Escaped string"); return 0;
	}
	// Search end of string
	size_t iLength = 0;
	while (!TestStringEnd(eRawType))
	{
		// Read a character (we're just counting atm)
		if (eRawType == RCT_Escaped)
			ReadEscapedChar();
		else
			pPos++;
		// Count it
		iLength++;
	}
	// Reset position, return the length
	pPos = pStart;
	return iLength;
}

StdBuf StdCompilerINIRead::ReadString(size_t iLength, RawCompileType eRawType, bool fAppendNull)
{
	// Excpect valid position
	if (!pPos)
	{
		notFound("String"); return StdBuf();
	}
	// Skip whitespace
	SkipWhitespace();
	// Escaped? Go over '"'
	if (eRawType == RCT_Escaped && *pPos++ != '"')
	{
		notFound("Escaped string"); return StdBuf();
	}
	// Create buffer
	StdBuf OutBuf; OutBuf.New(iLength + (fAppendNull ? sizeof('\0') : 0));
	// Read
	char *pOut = OutBuf.getMPtr<char>();
	while (iLength && !TestStringEnd(eRawType))
	{
		// Read a character
		if (eRawType == RCT_Escaped)
			*pOut++ = ReadEscapedChar();
		else
			*pOut++ = *pPos++;
		// Count it
		iLength--;
	}
	// Escaped: Go over '"'
	if (eRawType == RCT_Escaped)
	{
		while (*pPos != '"')
		{
			if (!*pPos || *pPos == '\n' || *pPos == '\r')
			{
				Warn("string not terminated!");
				pPos--;
				break;
			}
			pPos++;
		}
		pPos++;
	}
	// Nothing read? Identifiers need to be non-empty
	if (pOut == OutBuf.getData() && (eRawType == RCT_Idtf || eRawType == RCT_ID))
	{
		notFound("String"); return StdBuf();
	}
	// Append null
	if (fAppendNull)
		*pOut = '\0';
	// Shrink, if less characters were read
	OutBuf.Shrink(iLength);
	// Done
	return OutBuf;
}

bool StdCompilerINIRead::TestStringEnd(RawCompileType eType)
{
	switch (eType)
	{
	case RCT_Escaped: return *pPos == '"' || !*pPos || *pPos == '\n' || *pPos == '\r';
	case RCT_All: return !*pPos || *pPos == '\n' || *pPos == '\r';
	// '-' is needed for Layers in Scenario.txt (C4NameList) and other Material-Texture combinations
	case RCT_Idtf: case RCT_IdtfAllowEmpty: case RCT_ID: return !IsIdentifierChar(*pPos);
	}
	// unreachable
	return true;
}

char StdCompilerINIRead::ReadEscapedChar()
{
	// Catch some no-noes like \0, \n etc.
	if (*pPos >= 0 && iscntrl(static_cast<unsigned char>(*pPos)))
	{
		Warn("Nonprintable character found in string: {:02x}", static_cast<unsigned char>(*pPos));
		return *pPos++;
	}
	// Not escaped? Just return it
	if (*pPos != '\\') return *pPos++;
	// What type of escape?
	switch (*++pPos)
	{
	case 'a': pPos++; return '\a';
	case 'b': pPos++; return '\b';
	case 'f': pPos++; return '\f';
	case 'n': pPos++; return '\n';
	case 'r': pPos++; return '\r';
	case 't': pPos++; return '\t';
	case 'v': pPos++; return '\v';
	case '\'': pPos++; return '\'';
	case '"': pPos++; return '"';
	case '\\': pPos++; return '\\';
	case '?': pPos++; return '?';
	case 'x':
		// Treat '\x' as 'x' - damn special cases
		if (!isxdigit(static_cast<unsigned char>(*++pPos)))
			return 'x';
		else
		{
			// Read everything that looks like it might be hexadecimal - MSVC does it this way, so do not sue me.
			int iCode = 0;
			do
			{
				iCode = iCode * 16 + (isdigit(static_cast<unsigned char>(*pPos)) ? *pPos - '0' : *pPos - 'a' + 10); pPos++;
			} while (isxdigit(static_cast<unsigned char>(*pPos)));
			// Done. Don't bother to check the range (we aren't doing anything mission-critical here, are we?)
			return char(iCode);
		}
	default:
		// Not octal? Let it pass through.
		if (!isdigit(static_cast<unsigned char>(*pPos)) || *pPos >= '8')
			return *pPos++;
		else
		{
			// Read it the octal way.
			int iCode = 0;
			do
			{
				iCode = iCode * 8 + (*pPos - '0'); pPos++;
			} while (isdigit(static_cast<unsigned char>(*pPos)) && *pPos < '8');
			// Done. See above.
			return char(iCode);
		}
	}
	// unreachable
	assert(false);
}

void StdCompilerINIRead::notFound(const char *szWhat)
{
	excNotFound("{} expected", szWhat);
}
