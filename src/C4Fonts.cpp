/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
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

// engine font loading

#include <C4Include.h>
#include "C4Application.h"
#include <C4Fonts.h>

#include <C4Language.h>
#include <C4Config.h>
#include <C4Components.h>
#include <C4Log.h>
#include <C4Surface.h>
#include <C4Wrappers.h>
#include "StdFont.h"

#include <stdexcept>

void C4FontDef::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(toC4CStrBuf(Name),        "Name",        ""));
	pComp->Value(mkNamingAdapt(iSize,                    "Size",        1));
	pComp->Value(mkNamingAdapt(toC4CStrBuf(LogFont),     "LogFont",     ""));
	pComp->Value(mkNamingAdapt(toC4CStrBuf(SmallFont),   "SmallFont",   ""));
	pComp->Value(mkNamingAdapt(toC4CStrBuf(Font),        "Font",        ""));
	pComp->Value(mkNamingAdapt(toC4CStrBuf(CaptionFont), "CaptionFont", ""));
	pComp->Value(mkNamingAdapt(toC4CStrBuf(TitleFont),   "TitleFont",   ""));
}

C4VectorFont::~C4VectorFont()
{
	CStdFont::DestroyFont(pFont);
	// temp file loaded?
	if (*FileName)
	{
		// release font
		if (fIsTempFile) EraseFile(FileName);
	}
}

bool C4VectorFont::Init(C4Group &hGrp, const char *szFilename, C4Config &rCfg)
{
	// name by file
	Name.Copy(GetFilenameOnly(szFilename));
	if (!hGrp.LoadEntry(szFilename, Data)) return false;
	// success
	return true;
}

bool C4VectorFont::Init(const char *szFacename, int32_t iSize, uint32_t dwWeight, const char *szCharSet)
{
	// name by face
	Name.Copy(szFacename);
#if defined(_WIN32) && defined(HAVE_FREETYPE)
	// Win32 using freetype: Load TrueType-data from WinGDI into Data-buffer to be used by FreeType
	bool fSuccess = false;
	HDC hDC = ::CreateCompatibleDC(nullptr);
	if (hDC)
	{
		HFONT hFont = ::CreateFontA(iSize, 0, 0, 0, dwWeight, FALSE,
			FALSE, FALSE, C4Config::GetCharsetCode(szCharSet), OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS, 5,
			VARIABLE_PITCH, szFacename);
		if (hFont)
		{
			SelectObject(hDC, hFont);
			uint32_t dwTTFSize = ::GetFontData(hDC, 0, 0, nullptr, 0);
			if (dwTTFSize && dwTTFSize != GDI_ERROR)
			{
				Data.SetSize(dwTTFSize);
				uint32_t dwRealTTFSize = ::GetFontData(hDC, 0, 0, Data.getMData(), dwTTFSize);
				if (dwRealTTFSize == dwTTFSize)
				{
					fSuccess = true;
				}
				else
					Data.Clear();
			}
			DeleteObject(hFont);
		}
		DeleteDC(hDC);
	}
	if (!fSuccess) return false;
#else
	// otherwise, just assume that font can be created by face name
#endif
	// success
	return true;
}

void C4FontLoader::Clear()
{
	// clear loaded defs
	FontDefs.clear();
	// delete loaded vector fonts
	C4VectorFont *pVecFont, *pNextVecFont = pVectorFonts;
	while (pVecFont = pNextVecFont)
	{
		pNextVecFont = pVecFont->pNext;
		delete pVecFont;
	}
	pVectorFonts = nullptr;
}

void C4FontLoader::AddVectorFont(C4VectorFont *pNewFont)
{
	// add as last to chain
	C4VectorFont **ppFont = &pVectorFonts;
	while (*ppFont) ppFont = &((*ppFont)->pNext);
	*ppFont = pNewFont;
}

size_t C4FontLoader::LoadDefs(C4Group &hGroup, C4Config &rCfg)
{
	// load vector fonts
	char fn[_MAX_PATH + 1], fnDef[32]; int32_t i = 0;
	while (SCopySegment(C4CFN_FontFiles, i++, fnDef, '|', 31))
	{
		hGroup.ResetSearch();
		while (hGroup.FindNextEntry(fnDef, fn))
		{
			C4VectorFont *pVecFon = new C4VectorFont();
			if (pVecFon->Init(hGroup, fn, rCfg))
			{
				pVecFon->pNext = pVectorFonts;
				pVectorFonts = pVecFon;
			}
			else delete pVecFon;
		}
	}
	// load definition file
	StdStrBuf DefFileContent;
	if (!hGroup.LoadEntryString(C4CFN_FontDefs, DefFileContent)) return 0;
	std::vector<C4FontDef> NewFontDefs;
	if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(
		mkNamingAdapt(mkSTLContainerAdapt(NewFontDefs), "Font"),
		DefFileContent,
		C4CFN_FontDefs))
		return 0;
	// Copy the new FontDefs into the list
	FontDefs.insert(FontDefs.end(), NewFontDefs.begin(), NewFontDefs.end());
	// done
	return NewFontDefs.size();
}

bool C4FontLoader::InitFont(CStdFont &rFont, C4VectorFont *pFont, int32_t iSize, uint32_t dwWeight, bool fDoShadow)
{
	if (!pFont->pFont)
	{
#ifdef HAVE_FREETYPE
		// Creation from binary font data
		if (!pFont->Data.isNull()) pFont->pFont = CStdFont::CreateFont(pFont->Data);
		// creation from filename
		if (!pFont->pFont) pFont->pFont = CStdFont::CreateFont(pFont->FileName);
#endif
		// WinGDI: Try creation from font face name only
		if (!pFont->pFont) pFont->pFont = CStdFont::CreateFont(pFont->Name.getData());
		if (!pFont->pFont) return false; // this font can't be used
	}
	rFont.Init(*(pFont->pFont), iSize, dwWeight, Config.General.LanguageCharset, fDoShadow, Application.GetScale()); // throws exception on error
	return true;
}

bool C4FontLoader::InitFont(CStdFont &rFont, const char *szFontName, FontType eType, int32_t iSize, C4GroupSet *pGfxGroups, bool fDoShadow)
{
	// safety
	if (!szFontName || !*szFontName)
	{
		LogFatal(C4ResStrTableKey::IDS_ERR_INITFONTS);
		return false;
	}
	// get font to load
	// pFontDefs may be nullptr if no fonts are loaded; but then iFontDefCount is zero as well
	// the function must not be aborted, because a standard windows font may be loaded
	std::vector<C4FontDef>::iterator pFontDefC = FontDefs.begin(), pFontDef = FontDefs.end();
	while (pFontDefC != FontDefs.end())
	{
		// check font
		if (pFontDefC->Name == szFontName)
		{
			int32_t iSizeDiff = Abs(pFontDefC->iSize - iSize);
			// better match than last font?
			if (pFontDef == FontDefs.end() || Abs(pFontDef->iSize - iSize) >= iSizeDiff)
				pFontDef = pFontDefC;
		}
		// check next one
		++pFontDefC;
	}
	// if def has not been found, use the def as font name
	// determine font def string
	const char *szFontString = szFontName;
	// special: Fonts without shadow are always newly rendered
	if (!fDoShadow) { pFontDef = FontDefs.end(); }
	if (pFontDef != FontDefs.end()) switch (eType)
	{
	case C4FT_Log:       szFontString = pFontDef->LogFont.getData(); break;
	case C4FT_MainSmall: szFontString = pFontDef->SmallFont.getData(); break;
	case C4FT_Main:      szFontString = pFontDef->Font.getData(); break;
	case C4FT_Caption:   szFontString = pFontDef->CaptionFont.getData(); break;
	case C4FT_Title:     szFontString = pFontDef->TitleFont.getData(); break;
	default: LogFatal(C4ResStrTableKey::IDS_ERR_INITFONTS); return false; // invalid call
	}
	// font not assigned?
	if (!*szFontString)
	{
		// invalid call or spec
		LogFatal(C4ResStrTableKey::IDS_ERR_INITFONTS); return false;
	}
	// get font name
	char FontFaceName[C4MaxName + 1], FontParam[C4MaxName + 1];
	SCopyUntil(szFontString, FontFaceName, ',', C4MaxName);
	// is it an image file?
	const char *szExt = GetExtension(FontFaceName);
	if (SEqualNoCase(szExt, "png") || SEqualNoCase(szExt, "bmp"))
	{
		// image file name: load bitmap font from image file
		// if no graphics group is given, do not load yet
		if (!pGfxGroups)
		{
			LogFatal(C4ResStrTableKey::IDS_ERR_INITFONTS);
			return false;
		}
		// indent given?
		int32_t iIndent = 0;
		if (SCopySegment(szFontString, 1, FontParam, ',', C4MaxName))
			sscanf(FontParam, "%i", &iIndent);
		// load font face from gfx group
		int32_t iGrpId;
		C4Group *pGrp = pGfxGroups->FindEntry(FontFaceName, nullptr, &iGrpId);
		if (!pGrp)
		{
			LogFatal(C4ResStrTableKey::IDS_ERR_INITFONTS);
			return false;
		}
		// check if it's already loaded from that group with that parameters
		if (!rFont.IsSameAsID(FontFaceName, iGrpId, iIndent))
		{
			// it's not; so (re-)load it now!
			if (rFont.IsInitialized())
			{
				// reloading
				rFont.Clear();
				Log(C4ResStrTableKey::IDS_PRC_UPDATEFONT, FontFaceName, iIndent, 0);
			}
			C4Surface sfc;
			if (!sfc.Load(*pGrp, FontFaceName))
			{
				LogFatal(C4ResStrTableKey::IDS_ERR_INITFONTS);
				return false;
			}
			// init font from face
			try
			{
				rFont.Init(GetFilenameOnly(FontFaceName), &sfc, iIndent);
			}
			catch (std::runtime_error &e)
			{
				LogFatalNTr(e.what());
				LogFatal(C4ResStrTableKey::IDS_ERR_INITFONTS);
				return false;
			}
			rFont.id = iGrpId;
		}
	}
	else
	{
		int32_t iDefFontSize; uint32_t dwDefWeight = FW_NORMAL;
		switch (eType)
		{
		case C4FT_Log:       iDefFontSize = iSize * 12 / 14; break;
		case C4FT_MainSmall: iDefFontSize = iSize * 13 / 14; break;
		case C4FT_Main:      iDefFontSize = iSize; break;
		case C4FT_Caption:   iDefFontSize = iSize * 16 / 14; break;
		case C4FT_Title:     iDefFontSize = iSize * 22 / 14; break;
		default: LogFatal(C4ResStrTableKey::IDS_ERR_INITFONTS); return false; // invalid call
		}
		// regular font name: let WinGDI or Freetype draw a font with the given parameters
		// font size given?
		if (SCopySegment(szFontString, 1, FontParam, ',', C4MaxName))
			sscanf(FontParam, "%i", &iDefFontSize);
		// font weight given?
		if (SCopySegment(szFontString, 2, FontParam, ',', C4MaxName))
			sscanf(FontParam, "%i", &dwDefWeight);
		// check if it's already loaded from that group with that parameters
		if (!rFont.IsSameAs(FontFaceName, iDefFontSize, dwDefWeight))
		{
			// it's not; so (re-)load it now!
			if (rFont.IsInitialized())
			{
				// reloading
				rFont.Clear();
				Log(C4ResStrTableKey::IDS_PRC_UPDATEFONT, FontFaceName, iDefFontSize, dwDefWeight);
			}
			// init with given font name
			try
			{
				// check if one of the internally listed fonts should be used
				C4VectorFont *pFont = pVectorFonts;
				while (pFont)
				{
					if (SEqual(pFont->Name.getData(), FontFaceName))
					{
						if (InitFont(rFont, pFont, iDefFontSize, dwDefWeight, fDoShadow)) break;
					}
					pFont = pFont->pNext;
				}
				// no internal font matching? Then create one using the given face/filename (using a system font)
				if (!pFont)
				{
					pFont = new C4VectorFont();
					if (pFont->Init(FontFaceName, iDefFontSize, dwDefWeight, Config.General.LanguageCharset))
					{
						AddVectorFont(pFont);
						if (!InitFont(rFont, pFont, iDefFontSize, dwDefWeight, fDoShadow))
							throw std::runtime_error(std::format("Error initializing font {}", FontFaceName));
					}
					else
					{
						delete pFont;
						// no match for font face found
						throw std::runtime_error(std::format("Font face {} undefined", FontFaceName));
					}
				}
			}
			catch (std::runtime_error &e)
			{
				LogFatalNTr(e.what());
				LogFatal(C4ResStrTableKey::IDS_ERR_INITFONTS);
				return false;
			}
			rFont.id = 0;
		}
	}
	// done, success
	return true;
}
