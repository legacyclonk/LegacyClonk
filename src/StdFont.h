/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2003, Sven2
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

// text drawing facility for CStdDDraw

#pragma once

#include <Standard.h>
#include <StdMarkup.h>
#include <StdFacet.h>
#include <StdBuf.h>
#include <stdio.h>
#include <map>
#ifdef _WIN32
#include <tchar.h>
#endif

// Font rendering flags
#define STDFONT_CENTERED  0x0001
#define STDFONT_RIGHTALGN 0x0008
#define STDFONT_NOMARKUP  0x0020

#ifndef FW_NORMAL
#define FW_NORMAL 400
#define FW_BOLD   700
#endif

class CMarkup;
class CFacet;
class CStdVectorFont;

class CStdFont
{
public:
	// callback class to allow custom images
	class CustomImages
	{
	protected:
		virtual bool GetFontImage(const char *szImageTag, CFacet &rOutImgFacet) = 0;

		friend class CStdFont;

	public:
		virtual ~CustomImages() {}
	};
	static CStdVectorFont *CreateFont(const StdBuf &Data);
	static CStdVectorFont *CreateFont(const char *szFaceName);
	static void DestroyFont(CStdVectorFont *pFont);

public:
	int id; // used by the engine to keep track of where the font came from

protected:
	uint32_t dwDefFontHeight; // configured font size (in points)
	char szFontName[80 + 1]; // used font name (or surface file name)

	bool fPrerenderedFont; // true for fonts that came from a prerendered bitmap surface - no runtime adding of characters
	CSurface **psfcFontData; // font recource surfaces - additional surfaces created as needed
	int iNumFontSfcs; // number of created font surfaces
	int iSfcSizes; // size for font surfaces
	int iFontZoom; // zoom of font in texture

	bool fUTF8; // if set, UTF8-characters are decoded

	CSurface *sfcCurrent; // current surface font data can be written to at runtime
	int32_t iCurrentSfcX, iCurrentSfcY; // current character rendering position

	int iHSpace; // horizontal space to be added betwen two characters
	int iGfxLineHgt; // height of chaacters; may be larger than line height
	uint32_t dwWeight; // font weight (usually FW_NORMAL or FW_BOLD)
	bool fDoShadow; // if the font is shadowed

	CFacet fctAsciiTexCoords[256 - ' ']; // texture coordinates of ASCII letters
	std::map<uint32_t, CFacet> fctUnicodeMap; // texture coordinates of Unicode letters

	CustomImages *pCustomImages; // callback class for custom images

#if defined(_WIN32) && !defined(HAVE_FREETYPE)
	HDC hDC;
	HBITMAP hbmBitmap;
	uint32_t *pBitmapBits; int iBitmapSize;
	HFONT hFont;
#elif defined(HAVE_FREETYPE)
	CStdVectorFont *pVectorFont; // class assumed to be held externally!
#endif

	bool AddSurface();
	bool CheckRenderedCharSpace(uint32_t iCharWdt, uint32_t iCharHgt);
	bool AddRenderedChar(uint32_t dwChar, CFacet *pfctTarget);

	// get a character at the current string pos and advance pos by that character
	inline uint32_t GetNextCharacter(const char **pszString)
	{
		unsigned char c = **pszString;
		if (!fUTF8 || c < 128) { ++*pszString; return c; }
		else return GetNextUTF8Character(pszString);
	}

	uint32_t GetNextUTF8Character(const char **pszString);

	CFacet &GetCharacterFacet(uint32_t c)
	{
		if (!fUTF8 || c < 128) return fctAsciiTexCoords[c - ' ']; else return GetUnicodeCharacterFacet(c);
	}

	CFacet &GetUnicodeCharacterFacet(uint32_t c);

public:
	int iLineHgt; // height of one line of font (in pixels)
	// draw ine line of text
	void DrawText(CSurface *sfcDest, int iX, int iY, uint32_t dwColor, const char *szText, uint32_t dwFlags, CMarkup &Markup, float fZoom);

	// get text size
	bool GetTextExtent(const char *szText, int32_t &rsx, int32_t &rsy, bool fCheckMarkup = true);
	// get height of a line
	inline int GetLineHeight() { return iLineHgt; }
	// Sometimes, only the width of a text is needed
	int32_t GetTextWidth(const char *szText, bool fCheckMarkup = true) { int32_t x, y; GetTextExtent(szText, x, y, fCheckMarkup); return x; }
	// insert line breaks into a message and return overall height - uses and regards '|' as line breaks
	int BreakMessage(const char *szMsg, int iWdt, class StdStrBuf *pOut, bool fCheckMarkup, float fZoom = 1.0f);
	// get message break and pos after message break - does not regard any manual line breaks!
	int GetMessageBreak(const char *szMsg, const char **ppNewPos, int iBreakWidth, float fZoom = 1.0f);

	CStdFont();
	~CStdFont() { Clear(); }

	// function throws std::runtime_error in case of failure
	// font initialization - writes the surface data
	void Init(CStdVectorFont &VectorFont, uint32_t dwHeight, uint32_t dwFontWeight = FW_NORMAL, const char *szCharset = "", bool fDoShadow = true);

	// font initialization - grabs the given surface data and extracts character sizes from it
	void Init(const char *szFontName, CSurface *psfcFontSfc, int iIndent);

	void Clear(); // clear font

	// query whether font is initialized
	bool IsInitialized() { return !!*szFontName; }

	// query whether font is already initialized with certain data
	bool IsSameAsID(const char *szCFontName, int iCID, int iCIndent)
	{
		return SEqual(szCFontName, szFontName) && iCID == id && iCIndent == -iHSpace;
	}

	bool IsSameAs(const char *szCFontName, uint32_t iCHeight, uint32_t dwCWeight)
	{
		return SEqual(szCFontName, szFontName) && !id && iCHeight == dwDefFontHeight && dwCWeight == dwWeight;
	}

	// set custom image request handler
	void SetCustomImages(CustomImages *pHandler)
	{
		pCustomImages = pHandler;
	}
};

uint8_t GetCharsetCode(const char *strCharset);
const char *GetCharsetCodeName(const char *strCharset);
int32_t GetCharsetCodePage(const char *strCharset);
