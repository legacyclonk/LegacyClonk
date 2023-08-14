/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
 * Copyright (c) 2017-2023, The LegacyClonk Team and contributors
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

#pragma once

#include "C4Gui.h"
#include "C4Surface.h"

namespace C4GUI
{
// graphical resources
class Resource
{
private:
	static Resource *pRes; // current GUI resources

protected:
	C4Surface sfcCaption, sfcButton, sfcButtonD;
	C4Surface sfcScroll, sfcContext;
	int32_t idSfcCaption, idSfcButton, idSfcButtonD, idSfcScroll, idSfcContext;

public:
	DynBarFacet barCaption, barButton, barButtonD;
	C4FacetExID fctButtonHighlight;
	C4FacetExID fctIcons, fctIconsEx;
	C4FacetExID fctSubmenu;
	C4FacetExID fctCheckbox;
	C4FacetExID fctBigArrows;
	C4FacetExID fctProgressBar;
	C4FacetExID fctSpinBoxArrow;
	ScrollBarFacets sfctScroll;
	C4Facet fctContext;

	CStdFont &CaptionFont; // small, bold font
	CStdFont &TitleFont;   // large, bold font
	CStdFont &TextFont;    // font for normal text
	CStdFont &MiniFont;    // tiny font (logfont)
	CStdFont &TooltipFont; // same as BookFont

public:
	Resource(CStdFont &rCaptionFont, CStdFont &rTitleFont, CStdFont &rTextFont, CStdFont &rMiniFont, CStdFont &rTooltipFont)
	: idSfcCaption(0), idSfcButton(0), idSfcButtonD(0), idSfcScroll(0), idSfcContext(0),
	CaptionFont(rCaptionFont), TitleFont(rTitleFont), TextFont(rTextFont), MiniFont(rMiniFont), TooltipFont(rTooltipFont) {}
	~Resource() { Clear(); }

	bool Load(C4GroupSet &rFromGroup); // load resources
	void Clear(); // clear data

public:
	static Resource *Get() { return pRes; } // get res ptr - only set if successfully loaded
	static void Unload() { delete pRes; } // unload any GUI resources

	CStdFont &GetFontByHeight(int32_t iHgt, float *pfZoom = nullptr); // get optimal font for given control size
};

// shortcut for GUI resource gfx
inline Resource *GetRes() { return Resource::Get(); }
inline bool IsResLoaded() { return Resource::Get() != nullptr; }
}
