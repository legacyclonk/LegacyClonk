/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2004, Sven2
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

// engine font loading

#pragma once

#include <vector>

class C4Group;
class C4GroupSet;
class C4Config;
class CStdFont;

// font definition to be read
class C4FontDef
{
public:
	StdStrBuf Name;        // font name
	int32_t iSize;         // average font height of base font
	StdStrBuf LogFont;     // very small font used for log messages
	StdStrBuf SmallFont;   // pretty small font used in tiny dialogs
	StdStrBuf Font;        // base font used for anything
	StdStrBuf CaptionFont; // caption font used in GUI
	StdStrBuf TitleFont;   // font used to draw the loader caption

	C4FontDef() : iSize(0) {}
	void CompileFunc(StdCompiler *pComp);
};

// holder class for loaded ttf fonts
class C4VectorFont
{
protected:
	StdStrBuf Name;
	StdBuf Data;
	CStdVectorFont *pFont;
	char FileName[_MAX_PATH + 1]; // file name of temprarily extracted file
	bool fIsTempFile; // if set, the file resides at the temp path and is to be deleted

public:
	C4VectorFont *pNext; // next font

	C4VectorFont() : pFont(nullptr), fIsTempFile(false), pNext(nullptr) { *FileName = 0; }
	~C4VectorFont(); // dtor - releases font and deletes temp file

	bool Init(C4Group &hGrp, const char *szFilename, C4Config &rCfg); // load font from group
	bool Init(const char *szFacename, int32_t iSize, uint32_t dwWeight, const char *szCharSet); // load system font specified by face name

	friend class C4FontLoader;
};

// font loader
class C4FontLoader
{
protected:
	std::vector<C4FontDef> FontDefs; // array of loaded font definitions
	C4VectorFont *pVectorFonts; // vector fonts loaded and extracted to temp store

public:
	// enum of different fonts used in the clonk engine
	enum FontType { C4FT_Log, C4FT_MainSmall, C4FT_Main, C4FT_Caption, C4FT_Title };

public:
	C4FontLoader() : pVectorFonts(nullptr) {}
	~C4FontLoader() { Clear(); }

	void Clear(); // clear loaded fonts
	int32_t LoadDefs(C4Group &hGroup, C4Config &rCfg); // load font definitions from group file; return number of loaded font defs
	void AddVectorFont(C4VectorFont *pAddFont); // adds a new font to the list

#ifdef C4ENGINE
	bool InitFont(CStdFont &rFont, C4VectorFont *pFont, int32_t iSize, uint32_t dwWeight, bool fDoShadow);
	// init a font class of the given type
	// iSize is always the size of the normal font, which is adjusted for larger (title) and smaller (log) font types
	bool InitFont(CStdFont &rFont, const char *szFontName, FontType eType, int32_t iSize, C4GroupSet *pGfxGroups, bool fDoShadow = true);
#endif
};
