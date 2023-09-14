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

namespace C4GUI
{
// tabbing panel
class Tabular : public Control
{
public:
	// sheet covering the client area of a tabular
	class Sheet : public Window
	{
	protected:
		StdStrBuf sTitle; // sheet label
		int32_t icoTitle; // sheet label icon
		char cHotkey;
		uint32_t dwCaptionClr; // caption color - default if 0
		bool fHasCloseButton, fCloseButtonHighlighted;
		bool fTitleMarkup;

		Sheet(const char *szTitle, const C4Rect &rcBounds, int32_t icoTitle = Ico_None, bool fHasCloseButton = false, bool fTitleMarkup = true); // expands hotkey markup in title

		void DrawCaption(C4FacetEx &cgo, int32_t x, int32_t y, int32_t iMaxWdt, bool fLarge, bool fActive, bool fFocus, C4FacetEx *pfctClip, C4FacetEx *pfctIcon, CStdFont *pUseFont);
		void GetCaptionSize(int32_t *piWdt, int32_t *piHgt, bool fLarge, bool fActive, C4FacetEx *pfctClip, C4FacetEx *pfctIcon, CStdFont *pUseFont);
		virtual void OnShown(bool fByUser) {} // calklback from tabular after sheet has been made visible
		void SetCloseButtonHighlight(bool fToVal) { fCloseButtonHighlighted = fToVal; }
		bool IsPosOnCloseButton(int32_t x, int32_t y, int32_t iCaptWdt, int32_t iCaptHgt, bool fLarge);
		bool IsActiveSheet();

	public:
		const char *GetTitle() { return sTitle.getData(); }
		char GetHotkey() { return cHotkey; }
		void SetTitle(const char *szNewTitle);
		void SetCaptionColor(uint32_t dwNewClr = 0) { dwCaptionClr = dwNewClr; }
		virtual void UserClose() { delete this; } // user pressed close button
		bool HasCloseButton() const { return fHasCloseButton; }

		friend class Tabular;
	};

	enum TabPosition
	{
		tbNone = 0, // no tabs
		tbTop,      // tabs on top
		tbLeft,     // tabs to the left
	};

private:
	Sheet *pActiveSheet; // currently selected sheet
	TabPosition eTabPos; // whither tabs shalt be shown or nay, en where art thy shown?
	int32_t iMaxTabWidth; // maximum tab length; used when tabs are positioned left and do not have gfx
	int32_t iSheetSpacing, iSheetOff; // distances of sheet captions
	int32_t iCaptionLengthTotal, iCaptionScrollPos; // scrolling in captions (top only)
	bool fScrollingLeft, fScrollingRight, fScrollingLeftDown, fScrollingRightDown; // scrolling in captions (top only)
	time_t tLastScrollTime; // set when fScrollingLeftDown or fScrollingRightDown are true: Time for next scrolling if mouse is held down
	int iSheetMargin;
	bool fDrawSelf; // if border and bg shall be drawn

	C4FacetEx *pfctBack, *pfctClip, *pfctIcons; // set for tabulars that have custom gfx
	CStdFont *pSheetCaptionFont; // font to be used for caption drawing; nullptr if default GUI font is to be used

	C4KeyBinding *pKeySelUp, *pKeySelDown, *pKeySelUp2, *pKeySelDown2, *pKeyCloseTab; // key bindings

	void SelectionChanged(bool fByUser); // pActiveSheet changed: sound, tooltip, etc.
	void SheetsChanged(); // update iMaxTabWidth by new set of sheet labels
	void UpdateScrolling();
	void DoCaptionScroll(int32_t iDir);

private:
	bool HasGfx() { return pfctBack && pfctClip && pfctIcons; } // whether the control uses custom graphics

protected:
	bool KeySelUp(); // keyboard callback: Select previous sheet
	bool KeySelDown(); // keyboard callback: Select next sheet
	bool KeyCloseTab(); // keyboard callback: Close current sheet if possible

	virtual void DrawElement(C4FacetEx &cgo) override;
	virtual void MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, uint32_t dwKeyParam) override;
	void MouseLeaveCaptionArea();
	virtual void MouseLeave(CMouse &rMouse) override;
	virtual void OnGetFocus(bool fByMouse) override;

	virtual Control *IsFocusElement() override { return eTabPos ? this : nullptr; } // this control can gain focus only if tabs are enabled only
	virtual bool IsFocusOnClick() override { return false; } // but never get focus on single mouse click, because this would de-focus any contained controls!

	int32_t GetTopSize() { return (eTabPos == tbTop) ? 20 : 0; } // vertical size of tab selection bar
	int32_t GetLeftSize() { return (eTabPos == tbLeft) ? (HasGfx() ? GetLeftClipSize(pfctClip) : 20 + iMaxTabWidth) : 0; } // horizontal size of tab selection bar
	bool HasLargeCaptions() { return eTabPos == tbLeft; }

	virtual int32_t GetMarginTop() override    { return iSheetMargin + GetTopSize()  + (HasGfx() ? (rcBounds.Hgt - GetTopSize())  * 30 / 483 : 0); }
	virtual int32_t GetMarginLeft() override   { return iSheetMargin + GetLeftSize() + (HasGfx() ? (rcBounds.Wdt - GetLeftSize()) * 13 / 628 : 0); }
	virtual int32_t GetMarginRight() override  { return iSheetMargin + (HasGfx() ? (rcBounds.Wdt - GetLeftSize()) * 30 / 628 : 0); }
	virtual int32_t GetMarginBottom() override { return iSheetMargin + (HasGfx() ? (rcBounds.Hgt - GetTopSize())  * 32 / 483 : 0); }

	virtual void UpdateSize() override;

	virtual bool IsSelectedChild(Element *pChild) override { return pChild == pActiveSheet || (pActiveSheet && pActiveSheet->IsParentOf(pChild)); }

public:
	Tabular(const C4Rect &rtBounds, TabPosition eTabPos);
	~Tabular();

	virtual void RemoveElement(Element *pChild) override; // clear ptr
	Sheet *AddSheet(const char *szTitle, int32_t icoTitle = Ico_None);
	void AddCustomSheet(Sheet *pAddSheet);
	void ClearSheets(); // del all sheets
	void SelectSheet(int32_t iIndex, bool fByUser);
	void SelectSheet(Sheet *pSelSheet, bool fByUser);

	Sheet *GetSheet(int32_t iIndex) { return static_cast<Sheet *>(GetElementByIndex(iIndex)); }
	Sheet *GetActiveSheet() { return pActiveSheet; }
	int32_t GetActiveSheetIndex();
	int32_t GetSheetCount() { return GetElementCount(); }

	void SetGfx(C4FacetEx *pafctBack, C4FacetEx *pafctClip, C4FacetEx *pafctIcons, CStdFont *paSheetCaptionFont, bool fResizeByAspect);
	static int32_t GetLeftClipSize(C4FacetEx *pfctForClip) { return pfctForClip->Wdt * 95 / 120; } // left clip area size by gfx
	void SetSheetMargin(int32_t iMargin) { iSheetMargin = iMargin; UpdateOwnPos(); }
	void SetDrawDecoration(bool fToVal) { fDrawSelf = fToVal; }

	friend class Sheet;
};
}
