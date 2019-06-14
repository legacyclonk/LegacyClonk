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

/* Loads all standard graphics from Graphics.c4g */

#pragma once

#include <C4Group.h>
#include <C4GroupSet.h>
#include <C4Surface.h>
#include <C4FacetEx.h>

namespace C4GUI { class Resource; };

class C4GraphicsResource
{
private:
	bool fInitialized;

public:
	C4GraphicsResource();
	~C4GraphicsResource();

protected:
	C4Surface sfcControl;
	int32_t idSfcControl; // id of source group of control surface
	int32_t idPalGrp;     // if of source group of pal file
	// ID of last group in main group set that was already registered into the Files-set
	// used to avoid doubled entries by subsequent calls to RegisterMainGroups
	int32_t idRegisteredMainGroupSetFiles;

public:
	C4GroupSet Files;
	uint8_t GamePalette[256 * 3];
	uint8_t AlphaPalette[256 * 3];
	C4FacetExID fctPlayer;
	C4FacetExID fctFlag;
	C4FacetExID fctCrew;
	C4FacetExID fctScore;
	C4FacetExID fctWealth;
	C4FacetExID fctRank;
	int32_t iNumRanks;
	C4FacetExID fctFire;
	C4FacetExID fctBackground;
	C4Surface sfcLiquidAnimation; int32_t idSfcLiquidAnimation;
	C4FacetExID fctCaptain;
	C4FacetExID fctMouseCursor;
	C4FacetExID fctCursors[3];
	bool fOldStyleCursor; // if set, offsets need to be applied to some cursor facets
	C4FacetExID fctSelectMark;
	C4FacetExID fctOptions;
	C4FacetExID fctMenu;
	C4FacetExID fctUpperBoard;
	C4FacetExID fctLogo;
	C4FacetExID fctConstruction;
	C4FacetExID fctEnergy;
	C4FacetExID fctMagic;
	C4FacetExID fctArrow;
	C4FacetExID fctExit;
	C4FacetExID fctHand;
	C4FacetExID fctGamepad;
	C4FacetExID fctBuild;
	C4FacetExID fctEnergyBars;
	C4Facet fctCursor;
	C4Facet fctDropTarget;
	C4Facet fctInsideSymbol;
	C4Facet fctKeyboard;
	C4Facet fctMouse;
	C4Facet fctCommand;
	C4Facet fctKey;
	C4Facet fctOKCancel;
	C4FacetExID fctCrewClr; // ColorByOwner-surface of fctCrew
	C4FacetExID fctFlagClr; // ColorByOwner-surface of fctFlag
	C4FacetExID fctPlayerClr; // ColorByOwner-surface of fctPlayer
	C4FacetExID fctPlayerGray; // grayed out version of fctPlayer

	// fonts
	CStdFont FontTiny;     // used for logs
	CStdFont FontRegular;  // normal font - just refed from graphics system
	CStdFont FontCaption;  // used for title bars
	CStdFont FontTitle;    // huge font for titles
	CStdFont FontTooltip;  // normal, non-shadowed font (same as BookFont)

public:
	void Default();
	void Clear();
	bool InitFonts(); // init fonts only (early init done by loader screen)
	bool Init(bool fInitGUI);

	bool IsInitialized() { return fInitialized; } // return whether any gfx are loaded (so dlgs can be shown)

	bool RegisterGlobalGraphics(); // register global Graphics.c4g into own group set
	bool RegisterMainGroups();     // register new groups of Game.GroupSet into own group set
	void CloseFiles();             // free group set

	bool ReloadResolutionDependentFiles(); // reload any files that depend on the current resolution

protected:
	bool LoadFile(C4FacetExID &fct, const char *szName, C4GroupSet &rGfxSet, int32_t iWdt = C4FCT_Full, int32_t iHgt = C4FCT_Full, bool fNoWarnIfNotFound = false);
	bool LoadFile(C4Surface &sfc, const char *szName, C4GroupSet &rGfxSet, int32_t &ridCurrSfc);
	bool LoadCursorGfx();
	void ApplyCursorGfx();

	friend class C4GUI::Resource;
	friend class C4StartupGraphics;
};
