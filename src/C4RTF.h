/* by Sven2, 2005 */
// RTF file parsing functionality

#pragma once

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

		PropertyState() : pNext(NULL), cp(), pp(), sp(), dp(), dest(dsNormal), eState(psNormal), bHex(0), iHexBinCnt(0) {}
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
