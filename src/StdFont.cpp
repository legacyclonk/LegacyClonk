/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2003, Sven2
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

/* freetype support by JanL and Guenther */
// text drawing facility for CStdDDraw

#include "C4Config.h"
#include <Standard.h>
#include <StdBuf.h>
#include "StdFont.h"
#include <StdDDraw2.h>
#include <C4Surface.h>
#include <StdMarkup.h>

#include <cmath>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <cstdio>

#include <tchar.h>
#else
#define _T(x) x
#endif // _WIN32

#ifdef HAVE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#endif // HAVE_FREETYPE

#ifdef HAVE_ICONV
#include <iconv.h>
#endif // HAVE_ICONV

/* Initialization */

#ifdef HAVE_FREETYPE
class CStdVectorFont
{
	FT_Library library;
	FT_Face face;

public:
	CStdVectorFont(const char *filepathname)
	{
		// Initialize Freetype
		if (FT_Init_FreeType(&library))
			throw std::runtime_error("Cannot init Freetype");
		// Load the font
		FT_Error e;
		if (e = FT_New_Face(library, filepathname, 0, &face))
			throw std::runtime_error(std::format("Cannot load {}: {}", filepathname, e));
	}

	CStdVectorFont(const StdBuf &Data)
	{
		// Initialize Freetype
		if (FT_Init_FreeType(&library))
			throw std::runtime_error("Cannot init Freetype");
		// Load the font
		FT_Error e;
		if (e = FT_New_Memory_Face(library, static_cast<const FT_Byte *>(Data.getData()), Data.getSize(), 0, &face))
			throw std::runtime_error(std::format("Cannot load font: {}", e));
	}

	~CStdVectorFont()
	{
		FT_Done_Face(face);
		FT_Done_FreeType(library);
	}

	operator FT_Face() { return face; }
	FT_Face operator->() { return face; }
};

CStdVectorFont *CStdFont::CreateFont(const char *szFaceName)
{
	return new CStdVectorFont(szFaceName);
}

CStdVectorFont *CStdFont::CreateFont(const StdBuf &Data)
{
	return new CStdVectorFont(Data);
}

void CStdFont::DestroyFont(CStdVectorFont *pFont)
{
	delete pFont;
}

#else

CStdVectorFont *CStdFont::CreateFont(const StdBuf &Data)
{
	return 0;
}

CStdVectorFont *CStdFont::CreateFont(const char *szFaceName)
{
	return 0;
}

void CStdFont::DestroyFont(CStdVectorFont *pFont) {}

#endif

CStdFont::CStdFont()
{
	// set default values
	psfcFontData = nullptr;
	sfcCurrent = nullptr;
	iNumFontSfcs = 0;
	iSfcSizes = 64;
	dwDefFontHeight = iLineHgt = 10;
	iFontZoom = 1; // default: no internal font zooming - likely no antialiasing either...
	iHSpace = -1;
	iGfxLineHgt = iLineHgt + 1;
	dwWeight = FW_NORMAL;
	fDoShadow = false;
	fUTF8 = false;
	// font not yet initialized
	*szFontName = 0;
	id = 0;
	pCustomImages = nullptr;
	fPrerenderedFont = false;
#ifdef HAVE_FREETYPE
	pVectorFont = nullptr;
#endif
}

bool CStdFont::AddSurface()
{
	// add new surface as render target; copy old ones
	C4Surface **pNewSfcs = new C4Surface *[iNumFontSfcs + 1];
	if (iNumFontSfcs) std::memcpy(pNewSfcs, psfcFontData, iNumFontSfcs * sizeof(C4Surface *));
	delete[] psfcFontData;
	psfcFontData = pNewSfcs;
	C4Surface *sfcNew = psfcFontData[iNumFontSfcs] = new C4Surface();
	++iNumFontSfcs;
	if (iSfcSizes) if (!sfcNew->Create(iSfcSizes, iSfcSizes)) return false;
	if (sfcCurrent)
	{
		sfcCurrent->Unlock();
	}
	sfcCurrent = sfcNew;
	sfcCurrent->Lock();
	iCurrentSfcX = iCurrentSfcY = 0;
	return true;
}

bool CStdFont::CheckRenderedCharSpace(uint32_t iCharWdt, uint32_t iCharHgt)
{
	// need to do a line break?
	if (iCurrentSfcX + iCharWdt >= static_cast<uint32_t>(iSfcSizes)) if (iCurrentSfcX)
	{
		iCurrentSfcX = 0;
		iCurrentSfcY += iCharHgt;
		if (iCurrentSfcY + iCharHgt >= static_cast<uint32_t>(iSfcSizes))
		{
			// surface is full: Next one
			if (!AddSurface()) return false;
		}
	}
	// OK draw it there
	return true;
}

bool CStdFont::AddRenderedChar(uint32_t dwChar, C4Facet *pfctTarget)
{
	int shadowSize = fDoShadow ? static_cast<int>(std::round(scale)) : 0;

#ifdef HAVE_FREETYPE
	// Freetype character rendering
	FT_Set_Pixel_Sizes(*pVectorFont, dwDefFontHeight, dwDefFontHeight);
	int32_t iBoldness = dwWeight - 400; // zero is normal; 300 is bold
	if (iBoldness)
	{
		iBoldness = (1 << 16) + (iBoldness << 16) / 400;
		FT_Matrix mat;
		mat.xx = iBoldness; mat.xy = mat.yx = 0; mat.yy = 1 << 16;
		// .*(100 + iBoldness/3)/100
		FT_Set_Transform(*pVectorFont, &mat, nullptr);
	}
	else
	{
		FT_Set_Transform(*pVectorFont, nullptr, nullptr);
	}
	// Render
	if (FT_Load_Char(*pVectorFont, dwChar, FT_LOAD_RENDER | FT_LOAD_NO_HINTING))
	{
		// although the character was not drawn, assume it's not in the font and won't be needed
		// so return success here
		return true;
	}
	// Make a shortcut to the glyph
	FT_GlyphSlot slot = (*pVectorFont)->glyph;
	if (slot->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY)
	{
		// although the character was drawn in a strange way, assume it's not in the font and won't be needed
		// so return success here
		return true;
	}
	// linebreak/ new surface check
	int width = std::max<int>(slot->advance.x / 64, (std::max)(slot->bitmap_left, 0) + slot->bitmap.width) + shadowSize;
	if (!CheckRenderedCharSpace(width, iGfxLineHgt)) return false;
	// offset from the top
	int at_y = iCurrentSfcY + dwDefFontHeight * (*pVectorFont)->ascender / (*pVectorFont)->units_per_EM - slot->bitmap_top;
	int at_x = iCurrentSfcX + (std::max)(slot->bitmap_left, 0);
	// Copy to the surface
	for (unsigned int y = 0; y < slot->bitmap.rows + shadowSize; ++y)
	{
		for (unsigned int x = 0; x < slot->bitmap.width + shadowSize; ++x)
		{
			unsigned char bAlpha, bAlphaShadow;
			if (x < slot->bitmap.width && y < slot->bitmap.rows)
				bAlpha = 255 - slot->bitmap.buffer[slot->bitmap.width * y + x];
			else
				bAlpha = 255;
			// Make a shadow from the upper-left pixel, and blur with the eight neighbors
			uint32_t dwPixVal = 0u;
			bAlphaShadow = 255;
			if (fDoShadow && x >= shadowSize && y >= shadowSize)
			{
				int iShadow = 0;
				if (x < slot->bitmap.width && y < slot->bitmap.rows) iShadow += slot->bitmap.buffer[(x - (shadowSize - 1)) + slot->bitmap.width * (y - (shadowSize - 1))];
				if (x > shadowSize && y < slot->bitmap.rows) iShadow += slot->bitmap.buffer[(x - (shadowSize + 1)) + slot->bitmap.width * (y - (shadowSize - 1))];
				if (x > (shadowSize - 1) && y < slot->bitmap.rows) iShadow += slot->bitmap.buffer[(x - shadowSize) + slot->bitmap.width * (y - (shadowSize - 1))];
				if (x < slot->bitmap.width && y > shadowSize) iShadow += slot->bitmap.buffer[(x - (shadowSize - 1)) + slot->bitmap.width * (y - (shadowSize + 1))];
				if (x > shadowSize && y > shadowSize) iShadow += slot->bitmap.buffer[(x - (shadowSize + 1)) + slot->bitmap.width * (y - (shadowSize + 1))];
				if (x > (shadowSize - 1) && y > shadowSize) iShadow += slot->bitmap.buffer[(x - shadowSize) + slot->bitmap.width * (y - (shadowSize + 1))];
				if (x < slot->bitmap.width && y > (shadowSize - 1)) iShadow += slot->bitmap.buffer[(x - (shadowSize - 1)) + slot->bitmap.width * (y - shadowSize)];
				if (x > shadowSize && y > (shadowSize - 1)) iShadow += slot->bitmap.buffer[(x - (shadowSize + 1)) + slot->bitmap.width * (y - shadowSize)];
				if (x > (shadowSize - 1) && y > (shadowSize - 1)) iShadow += slot->bitmap.buffer[(x - shadowSize) + slot->bitmap.width * (y - shadowSize)] * 8;
				bAlphaShadow -= iShadow / 16;
				// because blitting on a black pixel reduces luminosity as compared to shadowless font,
				// assume luminosity as if blitting shadowless font on a 50% gray background
				unsigned char cBack = (255 - bAlpha);
				dwPixVal = RGB(cBack / 2, cBack / 2, cBack / 2);
			}
			dwPixVal += bAlphaShadow << 24;
			BltAlpha(dwPixVal, bAlpha << 24 | 0xffffff);
			sfcCurrent->SetPixDw(at_x + x, at_y + y, dwPixVal);
		}
	}
	// Save the position of the glyph for the rendering code
	pfctTarget->Set(sfcCurrent, iCurrentSfcX, iCurrentSfcY, width, iGfxLineHgt);

#endif // end of freetype rendering

	// advance texture position
	iCurrentSfcX += pfctTarget->Wdt;
	return true;
}

uint32_t CStdFont::GetNextUTF8Character(const char **pszString)
{
	// assume the current character is UTF8 already (i.e., highest bit set)
	const char *szString = *pszString;
	unsigned char c = *szString++;
	uint32_t dwResult = '?';
	assert(c > 127);
	if (c > 191 && c < 224)
	{
		unsigned char c2 = *szString++;
		if ((c2 & 192) != 128) { *pszString = szString; return '?'; }
		dwResult = ((c & 31) << 6) | (c2 & 63); // two char code
	}
	else if (c >= 224 && c <= 239)
	{
		unsigned char c2 = *szString++;
		if ((c2 & 192) != 128) { *pszString = szString; return '?'; }
		unsigned char c3 = *szString++;
		if ((c3 & 192) != 128) { *pszString = szString; return '?'; }
		dwResult = ((c & 15) << 12) | ((c2 & 63) << 6) | (c3 & 63); // three char code
	}
	else if (c >= 240 && c <= 247)
	{
		unsigned char c2 = *szString++;
		if ((c2 & 192) != 128) { *pszString = szString; return '?'; }
		unsigned char c3 = *szString++;
		if ((c3 & 192) != 128) { *pszString = szString; return '?'; }
		unsigned char c4 = *szString++;
		if ((c4 & 192) != 128) { *pszString = szString; return '?'; }
		dwResult = ((c & 7) << 18) | ((c2 & 63) << 12) | ((c3 & 63) << 6) | (c4 & 63); // four char code
	}
	*pszString = szString;
	return dwResult;
}

C4Facet &CStdFont::GetUnicodeCharacterFacet(uint32_t c)
{
	// find/add facet in map
	C4Facet &rFacet = fctUnicodeMap[c];
	// create character on the fly if necessary and possible
	if (!rFacet.Surface && !fPrerenderedFont)
	{
		sfcCurrent->Lock();
		AddRenderedChar(c, &rFacet);
		sfcCurrent->Unlock();
	}
	// rendering might have failed, in which case rFacet remains empty. Should be OK; char won't be printed then
	return rFacet;
}

void CStdFont::Init(CStdVectorFont &VectorFont, uint32_t dwHeight, uint32_t dwFontWeight, const char *szCharset, bool fDoShadow, float scale)
{
	const auto realHeight = dwHeight;
	// clear previous
	Clear();
	this->scale = scale;
	dwHeight = static_cast<uint32_t>(dwHeight * scale);
	// set values
	iHSpace = fDoShadow ? -1 : 0; // horizontal shadow
	dwWeight = dwFontWeight;
	this->fDoShadow = fDoShadow;
	if (SEqual(szCharset, "UTF-8")) fUTF8 = true;
	// determine needed texture size
	if (dwHeight * iFontZoom > 40)
		iSfcSizes = 512;
	else if (dwDefFontHeight * iFontZoom > 20)
		iSfcSizes = 256;
	else
		iSfcSizes = 128;
	dwDefFontHeight = dwHeight;
	// create surface
	if (!AddSurface())
	{
		Clear();
		throw std::runtime_error(std::string("Cannot create surface (") + szFontName + ")");
	}

#ifdef HAVE_FREETYPE
	// Store vector font - assumed to be held externally!
	pVectorFont = &VectorFont;
	// Get size
	// FIXME: use bbox or dynamically determined line heights here
	iLineHgt = (VectorFont->ascender - VectorFont->descender) * dwHeight / VectorFont->units_per_EM;
	iGfxLineHgt = iLineHgt + fDoShadow; // vertical shadow

#else

	throw std::runtime_error("You have a engine without Truetype support.");

#endif // HAVE_FREETYPE

	// loop through all ANSI/ASCII printable characters and prepare them
	// in case of UTF8, unicode characters will be created on the fly and extended ASCII characters (128-255) are not needed
	// now render all characters!

#if defined(HAVE_ICONV)
	// Initialize iconv
	struct iconv_t_wrapper
	{
		iconv_t i;
		iconv_t_wrapper(const char *to, const char *from)
		{
			i = iconv_open(to, from);
			if (i == iconv_t(-1))
				throw std::runtime_error(std::string("Cannot open iconv (") + to + ", " + from + ")");
		}
		~iconv_t_wrapper() { iconv_close(i); }
		operator iconv_t() { return i; }
	};
#ifdef __BIG_ENDIAN__
	iconv_t_wrapper iconv_handle("UCS-4BE", C4Config::GetCharsetCodeName(szCharset));
#else
	iconv_t_wrapper iconv_handle("UCS-4LE", C4Config::GetCharsetCodeName(szCharset));
#endif
#elif defined(_WIN32)
	int32_t iCodePage = C4Config::GetCharsetCodePage(szCharset);
#endif
	int cMax = fUTF8 ? 127 : 255;
	for (int c = ' '; c <= cMax; ++c)
	{
		uint32_t dwChar = c;
#if defined(HAVE_ICONV)
		// convert from whatever legacy encoding in use to unicode
		if (!fUTF8)
		{
			// Convert to unicode
			char chr = dwChar;
			char *in = &chr;
			char *out = reinterpret_cast<char *>(&dwChar);
			size_t insize = 1;
			size_t outsize = 4;
			iconv(iconv_handle, const_cast<ICONV_CONST char * *>(&in), &insize, &out, &outsize);
		}
#elif defined(_WIN32)
		// convert using Win32 API
		if (!fUTF8 && c >= 128)
		{
			char cc[2] = { static_cast<char>(c), '\0' };
			wchar_t outbuf[4];
			if (MultiByteToWideChar(iCodePage, 0, cc, -1, outbuf, 4)) // 2do: Convert using proper codepage
			{
				// now convert from UTF-16 to UCS-4
				if (((outbuf[0] & 0xfc00) == 0xd800) && ((outbuf[1] & 0xfc00) == 0xdc00))
				{
					dwChar = 0x10000 + (((outbuf[0] & 0x3ff) << 10) | (outbuf[1] & 0x3ff));
				}
				else
					dwChar = outbuf[0];
			}
			else
			{
				// conversion error. Shouldn't be fatal; just pretend it were a Unicode character
			}
		}
#else
		// no conversion available? Just break for non-iso8859-1.
#endif // defined HAVE_ICONV
		if (!AddRenderedChar(dwChar, &(fctAsciiTexCoords[c - ' '])))
		{
			sfcCurrent->Unlock();
			Clear();
			throw std::runtime_error(std::string("Cannot render characters for Font (") + szFontName + ")");
		}
	}

	sfcCurrent->Unlock();

	// adjust line height
	iLineHgt /= iFontZoom;
	this->scale = static_cast<float>(dwHeight) / realHeight;

	fPrerenderedFont = false;
	if (0) for (int i = 0; i < iNumFontSfcs; ++i)
	{
		const std::string pngFilename{std::format("{}{}{}_{}.png", +szFontName, dwHeight, fDoShadow ? "_shadow" : "", i)};
		psfcFontData[i]->SavePNG(pngFilename.c_str(), true, false, false);
	}
}

const uint32_t FontDelimeterColor        = 0xff0000,
               FontDelimiterColorLB      = 0x00ff00,
               FontDelimeterColorIndent1 = 0xffff00,
               FontDelimeterColorIndent2 = 0xff00ff;

// perform color matching in 16 bit
inline bool ColorMatch(uint32_t dwClr1, uint32_t dwClr2)
{
	return ClrDw2W(dwClr1) == ClrDw2W(dwClr2);
}

void CStdFont::Init(const char *szFontName, C4Surface *psfcFontSfc, int iIndent)
{
	// clear previous
	Clear();
	// grab surface
	iSfcSizes = 0;
	if (!AddSurface()) { Clear(); throw std::runtime_error(std::string("Error creating surface for ") + szFontName); }
	*sfcCurrent = std::move(*psfcFontSfc);
	// extract character positions from image data
	if (!sfcCurrent->Hgt)
	{
		Clear();
		throw std::runtime_error(std::string("Error loading ") + szFontName);
	}
	// get line height
	iGfxLineHgt = 1;
	while (iGfxLineHgt < sfcCurrent->Hgt)
	{
		uint32_t dwPix = sfcCurrent->GetPixDw(0, iGfxLineHgt, false);
		if (ColorMatch(dwPix, FontDelimeterColor) || ColorMatch(dwPix, FontDelimiterColorLB) ||
			ColorMatch(dwPix, FontDelimeterColorIndent1) || ColorMatch(dwPix, FontDelimeterColorIndent2))
			break;
		++iGfxLineHgt;
	}
	// set font height and width indent
	dwDefFontHeight = iLineHgt = iGfxLineHgt - iIndent;
	iHSpace = -iIndent;
	// determine character sizes
	int iX = 0, iY = 0;
	for (int c = ' '; c < 256; ++c)
	{
		// save character pos
		fctAsciiTexCoords[c - ' '].X = iX; // left
		fctAsciiTexCoords[c - ' '].Y = iY; // top
		bool IsLB = false;
		// get horizontal extent
		while (iX < sfcCurrent->Wdt)
		{
			uint32_t dwPix = sfcCurrent->GetPixDw(iX, iY, false);
			if (ColorMatch(dwPix, FontDelimeterColor) || ColorMatch(dwPix, FontDelimeterColorIndent1) || ColorMatch(dwPix, FontDelimeterColorIndent2))
				break;
			if (ColorMatch(dwPix, FontDelimiterColorLB)) { IsLB = true; break; }
			++iX;
		}
		// remove vertical line
		if (iX < sfcCurrent->Wdt)
			for (int y = 0; y < iGfxLineHgt; ++y)
				sfcCurrent->SetPixDw(iX, iY + y, 0xffffffff);
		// save char size
		fctAsciiTexCoords[c - ' '].Wdt = iX - fctAsciiTexCoords[c - ' '].X;
		fctAsciiTexCoords[c - ' '].Hgt = iGfxLineHgt;
		// next line?
		if (++iX >= sfcCurrent->Wdt || IsLB)
		{
			iY += iGfxLineHgt;
			iX = 0;
			// remove horizontal line
			if (iY < sfcCurrent->Hgt)
				for (int x = 0; x < sfcCurrent->Wdt; ++x)
					sfcCurrent->SetPixDw(x, iY, 0xffffffff);
			// skip empty line
			++iY;
			// end reached?
			if (iY + iGfxLineHgt > sfcCurrent->Hgt)
			{
				// all filled
				break;
			}
		}
	}
	// release texture data
	sfcCurrent->Unlock();
	// adjust line height
	iLineHgt /= iFontZoom;
	// set name
	SCopy(szFontName, this->szFontName);
	// mark prerendered
	fPrerenderedFont = true;
}

void CStdFont::Clear()
{
#ifdef HAVE_FREETYPE
	pVectorFont = nullptr;
#endif
	// clear font sfcs
	if (psfcFontData)
	{
		while (iNumFontSfcs--) delete psfcFontData[iNumFontSfcs];
		delete[] psfcFontData;
		psfcFontData = nullptr;
	}
	sfcCurrent = nullptr;
	iNumFontSfcs = 0;
	for (int c = ' '; c < 256; ++c) fctAsciiTexCoords[c - ' '].Default();
	fctUnicodeMap.clear();
	// set default values
	dwDefFontHeight = iLineHgt = 10;
	iFontZoom = 1; // default: no internal font zooming - likely no antialiasing either...
	iHSpace = -1;
	iGfxLineHgt = iLineHgt + 1;
	dwWeight = FW_NORMAL;
	fDoShadow = false;
	fPrerenderedFont = false;
	fUTF8 = false;
	// font not yet initialized
	*szFontName = 0;
	id = 0;
}

/* Text size measurement */

bool CStdFont::GetTextExtent(const char *szText, int32_t &rsx, int32_t &rsy, bool fCheckMarkup, bool ignoreScale)
{
	float realScale = 1.f;
	if (!ignoreScale)
	{
		realScale = scale;
	}
	// safety
	if (!szText) return false;
	// keep track of each row's size
	int lineStepHeight = static_cast<int>(std::ceil(iLineHgt / realScale));
	float iRowWdt = 0, iWdt = 0;
	int iHgt = lineStepHeight;
	// ignore any markup
	CMarkup MarkupChecker(false);
	// go through all text
	while (*szText)
	{
		// ignore markup
		if (fCheckMarkup) MarkupChecker.SkipTags(&szText);
		// get current char
		uint32_t c = GetNextCharacter(&szText);
		// done? (must check here, because markup-skip may have led to text end)
		if (!c) break;
		// line break?
		if (c == _T('\n') || (fCheckMarkup && c == _T('|'))) { iRowWdt = 0; iHgt += lineStepHeight; continue; }
		// ignore system characters
		if (c < _T(' ')) continue;
		// image?
		int iImgLgt;
		if (fCheckMarkup && c == '{' && szText[0] == '{' && szText[1] != '{' && (iImgLgt = SCharPos('}', szText + 1)) > 0 && szText[iImgLgt + 2] == '}')
		{
			char imgbuf[101];
			SCopy(szText + 1, imgbuf, (std::min)(iImgLgt, 100));
			C4Facet fct;
			// image renderer initialized?
			if (pCustomImages)
				// try to get an image then
				pCustomImages->GetFontImage(imgbuf, fct);
			if (fct.Hgt)
			{
				// image found: adjust aspect by font height and calc appropriate width
				iRowWdt += (fct.Wdt * iGfxLineHgt) / realScale / fct.Hgt;
			}
			else
			{
				// image renderer not hooked or ID not found, or surface not present: just ignore it
				// printing it out wouldn't look better...
			}
			// skip image tag
			szText += iImgLgt + 3;
		}
		else
		{
			// regular char
			// look up character width in texture coordinates table
			iRowWdt += GetCharacterFacet(c).Wdt / realScale / iFontZoom;
		}
		// apply horizontal indent for all but last char
		if (*szText) iRowWdt += iHSpace;
		// adjust max row size
		if (iRowWdt > iWdt) iWdt = iRowWdt;
	}
	// store output
	rsx = static_cast<int>(iWdt); rsy = iHgt;
	// done, success
	return true;
}

int CStdFont::BreakMessage(const char *szMsg, int iWdt, StdStrBuf *pOut, bool fCheckMarkup, float fZoom, size_t maxLines)
{
	// safety
	if (!szMsg || !pOut) return 0;
	pOut->Clear();
	uint32_t c;
	const char *szPos = szMsg, // current parse position in the text
		*szLastBreakPos = szMsg, // points to the char after at (whitespace) or after ('-') which text can be broken
		*szLastEmergenyBreakPos, // same, but at last char in case no suitable linebreak could be found
		*szLastPos;              // last position until which buffer has been transferred to output
	int iLastBreakOutLen, iLastEmergencyBreakOutLen; // size of output string at break positions
	float iX = 0, // current text width at parse pos
		iXBreak = 0, // text width as it was at last break pos
		iXEmergencyBreak; // same, but at last char in case no suitable linebreak could be found
	int iHgt = GetLineHeight(); // total height of output text
	bool fIsFirstLineChar = true;
	// ignore any markup
	CMarkup MarkupChecker(false);
	// go through all text
	while (*(szLastPos = szPos))
	{
		// ignore markup
		if (fCheckMarkup) MarkupChecker.SkipTags(&szPos);
		// get current char
		c = GetNextCharacter(&szPos);
		// done? (must check here, because markup-skip may have led to text end)
		if (!c) break;
		// manual break?
		float iCharWdt = 0;
		if (c != '\n' && (!fCheckMarkup || c != '|'))
		{
			// image?
			int iImgLgt;
			if (fCheckMarkup && c == '{' && szPos[0] == '{' && szPos[1] != '{' && (iImgLgt = SCharPos('}', szPos + 1)) > 0 && szPos[iImgLgt + 2] == '}')
			{
				char imgbuf[101];
				SCopy(szPos + 1, imgbuf, (std::min)(iImgLgt, 100));
				C4Facet fct;
				// image renderer initialized?
				if (pCustomImages)
					// try to get an image then
					pCustomImages->GetFontImage(imgbuf, fct);
				if (fct.Hgt)
				{
					// image found: adjust aspect by font height and calc appropriate width
					iCharWdt = (fct.Wdt * iGfxLineHgt) / scale / fct.Hgt;
				}
				else
				{
					// image renderer not hooked or ID not found, or surface not present: just ignore it
					// printing it out wouldn't look better...
					iCharWdt = 0;
				}
				// skip image tag
				szPos += iImgLgt + 3;
			}
			else
			{
				// regular char
				// look up character width in texture coordinates table
				if (c >= ' ')
					iCharWdt = fZoom * GetCharacterFacet(c).Wdt / iFontZoom / scale + iHSpace;
				else
					iCharWdt = 0; // OMFG ctrl char
			}
			// add chars to output
			pOut->Append(szLastPos, szPos - szLastPos);
			// add to line; always add one char at minimum
			if ((iX += iCharWdt) <= iWdt || fIsFirstLineChar)
			{
				// check whether linebreak possibility shall be marked here
				// 2do: What about unicode-spaces?
				if (c < 256) if (std::isspace(static_cast<unsigned char>(c)) || c == '-')
				{
					szLastBreakPos = szPos;
					iLastBreakOutLen = pOut->getLength();
					// space: Break directly at space if it isn't the first char here
					// first char spaces must remain, in case the output area is just one char width
					if (c != '-' && !fIsFirstLineChar) --szLastBreakPos; // because c<256, the character length can be safely assumed to be 1 here
					iXBreak = iX;
				}
				// always mark emergency break after char that fitted the line
				szLastEmergenyBreakPos = szPos;
				iXEmergencyBreak = iX;
				iLastEmergencyBreakOutLen = pOut->getLength();
				// line OK; continue filling it
				fIsFirstLineChar = false;
				continue;
			}
			// line must be broken now
			// check if a linebreak is possible directly here, because it's a space
			// only check for space and not for other breakable characters (such as '-'), because the break would happen after those characters instead of at them
			if (c < 128 && std::isspace(static_cast<unsigned char>(c)))
			{
				szLastBreakPos = szPos - 1;
				iLastBreakOutLen = pOut->getLength();
				iXBreak = iX;
			}
			// if there was no linebreak, do it at emergency pos
			else if (szLastBreakPos == szMsg)
			{
				szLastBreakPos = szLastEmergenyBreakPos;
				iLastBreakOutLen = iLastEmergencyBreakOutLen;
				iXBreak = iXEmergencyBreak;
			}
			StdStrBuf tempPart;
			// insert linebreak at linebreak pos
			// was it a space? Then just overwrite space with a linebreak
			if (static_cast<uint8_t>(*szLastBreakPos) < 128 && std::isspace(static_cast<unsigned char>(*szLastBreakPos)))
			{
				*pOut->getMPtr(iLastBreakOutLen - 1) = '\n';
				if (fCheckMarkup)
				{
					tempPart.Copy(pOut->getMPtr(iLastBreakOutLen));
					pOut->SetLength(iLastBreakOutLen - 1);
				}
			}
			else
			{
				// otherwise, insert line break
				pOut->InsertChar('\n', iLastBreakOutLen);
				if (fCheckMarkup)
				{
					tempPart.Copy(pOut->getMPtr(iLastBreakOutLen) + 1);
					pOut->SetLength(iLastBreakOutLen);
				}
			}
			if (fCheckMarkup)
			{
				CMarkup markup(false);
				const char *data = pOut->getData();
				const char *lastLine = (std::max)(data + pOut->getSize() - 3, data);
				while (lastLine > data && *lastLine != '\n') --lastLine;
				while (*lastLine)
				{
					while (*lastLine == '<' && markup.Read(&lastLine));
					if (*lastLine) ++lastLine;
				}
				pOut->Append(markup.ToCloseMarkup().c_str());
				pOut->AppendChar('\n');
				pOut->Append(markup.ToMarkup().c_str());
				pOut->Append(tempPart);
			}
			// calc next line usage
			iX -= iXBreak;
		}
		else
		{
			// a static linebreak: Everything's well; this just resets the line width
			iX = 0;
			// add to output
			pOut->Append(szLastPos, szPos - szLastPos);
		}
		// forced or manual line break: set new line beginning to char after line break
		szLastBreakPos = szMsg = szPos;
		// manual line break or line width overflow: add char to next line
		iHgt += GetLineHeight();
		fIsFirstLineChar = true;

		if (maxLines == 1)
		{
			pOut->Append(szPos);
			return iHgt;
		}
		else if (maxLines != 0) --maxLines;
	}
	// transfer final data to buffer (any missing markup)
	pOut->Append(szLastPos, szPos - szLastPos);
	// return text height
	return iHgt;
}

/* Text drawing */

void CStdFont::DrawText(C4Surface *sfcDest, int iX, int iY, uint32_t dwColor, const char *szText, uint32_t dwFlags, CMarkup &Markup, float fZoom)
{
	float x = static_cast<float>(iX), y = static_cast<float>(iY);
	CBltTransform bt, *pbt = nullptr;
	// set blit color
	dwColor = InvertRGBAAlpha(dwColor);
	uint32_t dwOldModClr;
	bool fWasModulated = lpDDraw->GetBlitModulation(dwOldModClr);
	if (fWasModulated) ModulateClr(dwColor, dwOldModClr);
	// get alpha fade percentage
	uint32_t dwAlphaMod = BoundBy<int>(((static_cast<int>(dwColor >> 0x18) - 0x50) * 0xff) / 0xaf, 0, 255) << 0x18 | 0xffffff;
	// adjust text starting position (horizontal only)
	if (dwFlags & STDFONT_CENTERED)
	{
		// centered
		int32_t sx, sy;
		GetTextExtent(szText, sx, sy, !(dwFlags & STDFONT_NOMARKUP));
		x -= fZoom * (sx / 2);
	}
	else if (dwFlags & STDFONT_RIGHTALGN)
	{
		// right-aligned
		int32_t sx, sy;
		GetTextExtent(szText, sx, sy, !(dwFlags & STDFONT_NOMARKUP));
		x -= fZoom * sx;
	}
	// apply texture zoom
	fZoom /= scale;
	fZoom /= iFontZoom;
	// set start markup transformation
	if (!Markup.Clean()) pbt = &bt;
	// output text
	uint32_t c;
	C4Facet fctFromBlt; // source facet
	while (c = GetNextCharacter(&szText))
	{
		// ignore system characters
		if (c < _T(' ')) continue;
		// apply markup
		if (c == '<' && (~dwFlags & STDFONT_NOMARKUP))
		{
			// get tag
			if (Markup.Read(&--szText))
			{
				// mark transform to be done
				// (done only if tag was found, so most normal blits don't init a trasnformation matrix)
				pbt = &bt;
				// skip the tag
				continue;
			}
			// invalid tag: render it as text
			++szText;
		}
		float w2, h2; // dst width/height
		// custom image?
		int iImgLgt;
		if (c == '{' && szText[0] == '{' && szText[1] != '{' && (iImgLgt = SCharPos('}', szText + 1)) > 0 && szText[iImgLgt + 2] == '}' && !(dwFlags & STDFONT_NOMARKUP))
		{
			fctFromBlt.Default();
			char imgbuf[101];
			SCopy(szText + 1, imgbuf, (std::min)(iImgLgt, 100));
			szText += iImgLgt + 3;
			// image renderer initialized?
			if (pCustomImages)
				// try to get an image then
				pCustomImages->GetFontImage(imgbuf, fctFromBlt);
			if (fctFromBlt.Surface && fctFromBlt.Hgt)
			{
				// image found: adjust aspect by font height and calc appropriate width
				w2 = (fctFromBlt.Wdt * iGfxLineHgt) * fZoom / fctFromBlt.Hgt;
				h2 = iGfxLineHgt * fZoom;
			}
			else
			{
				// image renderer not hooked or ID not found, or surface not present: just ignore it
				// printing it out wouldn't look better...
				continue;
			}
			// normal: not modulated, unless done by transform or alpha fadeout
			if ((dwColor >> 0x18) <= 0x50)
				lpDDraw->DeactivateBlitModulation();
			else
				lpDDraw->ActivateBlitModulation((dwColor & 0xff000000) | 0xffffff);
		}
		else
		{
			// regular char
			// get texture coordinates
			fctFromBlt = GetCharacterFacet(c);
			w2 = fctFromBlt.Wdt * fZoom; h2 = fctFromBlt.Hgt * fZoom;
			lpDDraw->ActivateBlitModulation(dwColor);
		}
		// do color/markup
		if (pbt)
		{
			// reset data to be transformed by markup
			uint32_t dwBlitClr = dwColor;
			bt.Set(1, 0, 0, 0, 1, 0, 0, 0, 1);
			// apply markup
			Markup.Apply(bt, dwBlitClr);
			if (dwBlitClr != dwColor) ModulateClrA(dwBlitClr, dwAlphaMod);
			lpDDraw->ActivateBlitModulation(dwBlitClr);
			// move transformation center to center of letter
			float fOffX = w2 / 2 + x;
			float fOffY = h2 / 2 + y;
			bt.mat[2] += fOffX - fOffX * bt.mat[0] - fOffY * bt.mat[1];
			bt.mat[5] += fOffY - fOffX * bt.mat[3] - fOffY * bt.mat[4];
		}
		// blit character or image
		lpDDraw->Blit(fctFromBlt.Surface, float(fctFromBlt.X), float(fctFromBlt.Y), float(fctFromBlt.Wdt), float(fctFromBlt.Hgt),
			sfcDest, x, y, w2, h2,
			true, pbt, true);
		// advance pos and skip character indent
		x += w2 + iHSpace;
	}
	// reset blit modulation
	if (fWasModulated)
		lpDDraw->ActivateBlitModulation(dwOldModClr);
	else
		lpDDraw->DeactivateBlitModulation();
}

int CStdFont::GetLineHeight() const
{
	return static_cast<int>(iLineHgt / scale);
}
