/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2005, Sven2
 * Copyright (c) 2017-2022, The LegacyClonk Team and contributors
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

// RTF file parsing functionality

#pragma once

#include "StdBuf.h"

#include <string_view>

class C4RTFFile
{
public:
	struct KeywordTableEntry
	{
		const char *szKeyword;
		int iDefaultParam;
		bool fForceDefaultParam;
		enum { kwdChars, kwdDest, kwdProp, kwdSpec } eType;
		const char *szChars;
		int idx; // index into property table
	};

	struct CharProps {};
	struct ParagraphProps {};
	struct SectionProps {};
	struct DocumentProps {};

	enum DestState { dsNormal, dsSkip, };
	enum ParserState { psNormal, psBinary, psHex, };
	enum SpecialKeyword { specBin, specHex, specSkipDest, };

	// RTF parser state; states may be nested in definition blocks
	struct PropertyState
	{
		PropertyState *pNext; // linked list
		CharProps cp;
		ParagraphProps pp;
		SectionProps sp;
		DocumentProps dp;
		DestState dest;
		ParserState eState;
		char bHex; // used by hex parser
		int iHexBinCnt; // used by hex and binary parser

		PropertyState() : pNext(nullptr), cp(), pp(), sp(), dp(), dest(dsNormal), eState(psNormal), bHex(0), iHexBinCnt(0) {}
	};

	class ParserError
	{
	public:
		StdStrBuf ErrorText;
		ParserError(const char *szErr) { ErrorText.Copy(szErr); }
	};

private:
	StdBuf sRTF; // rtf formatted text

	PropertyState *pState;
	bool fSkipDestIfUnknownKeyword;

public:
	C4RTFFile();
	~C4RTFFile();

private:
	void ClearState();
	void EndGroupAction() {}
	void AssertNoEOF(size_t iPos);
	void ApplyPropChange(int iProp, int iParam) {}
	void ChangeDest(StdStrBuf &sResult, int iDest);
	void SpecialKeyword(StdStrBuf &sResult, int iKeyw, int iParam);
	void TranslateKeyword(StdStrBuf &sResult, const char *szKeyword, int iParam, bool fHasIntParam);
	void ParseKeyword(StdStrBuf &sResult, size_t &iPos);
	void ParseChars(StdStrBuf &sResult, const char *szChars);
	void ParseChar(StdStrBuf &sResult, char c);
	void ParseHexChar(StdStrBuf &sResult, char c);
	void PushState();
	void PopState();

public:
	void Load(const StdBuf &sContents) // load RTF text from file
	{
		sRTF.Copy(sContents);
	}

	StdStrBuf GetPlainText(); // convert to plain text

};

std::string RtfEscape(std::string_view plainText);
