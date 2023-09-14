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

#include "C4Constants.h"
#include "C4Gui.h"

namespace C4GUI
{
// dropdown box to select elements from a list
class ComboBox : public Control
{
public:
	struct ComboMenuCBStruct // struct used as menu callback parameter for dropdown menu
	{
		StdStrBuf sText;
		int32_t id;

		ComboMenuCBStruct() : sText(), id(0) {}
		ComboMenuCBStruct(const char *szText, int32_t id) : sText(szText), id(id) {}
	};

private:
	class C4KeyBinding *pKeyOpenCombo, *pKeyCloseCombo;

private:
	int32_t iOpenMenu; // associated menu (used to flag dropped down)
	ComboBox_FillCB *pFillCallback; // callback used to display the dropdown
	char Text[C4MaxTitle + 1]; // currently selected item
	bool fReadOnly; // merely a label in ReadOnly-mode
	bool fSimple; // combo without border and stuff
	bool fMouseOver; // mouse hovering over this?
	CStdFont *pUseFont; // font used to draw this control
	uint32_t dwFontClr, dwBGClr, dwBorderClr; // colors used to draw this control
	C4Facet *pFctSideArrow; // arrow gfx used to start combo-dropdown

private:
	bool DoDropdown(); // open dropdown menu (context menu)
	bool KeyDropDown() { return DoDropdown(); }
	bool KeyAbortDropDown() { return AbortDropdown(true); }
	bool AbortDropdown(bool fByUser); // abort dropdown menu, if it's open

protected:
	virtual void DrawElement(C4FacetEx &cgo) override; // draw combo box

	virtual void MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, uint32_t dwKeyParam) override; // input: mouse. left-click opens menu
	virtual void MouseEnter(CMouse &rMouse) override; // called when mouse cursor enters element region
	virtual void MouseLeave(CMouse &rMouse) override; // called when mouse cursor leaves element region

	virtual bool IsFocusOnClick() override { return false; } // don't select control on click
	virtual Control *IsFocusElement() override { return fReadOnly ? nullptr : this; } // this control can gain focus if not readonly

	void OnCtxComboSelect(C4GUI::Element *pListItem, const ComboMenuCBStruct &rNewSel);

public:
	ComboBox(const C4Rect &rtBounds);
	~ComboBox();

	void SetComboCB(ComboBox_FillCB *pNewFillCallback);
	static int32_t GetDefaultHeight();
	void SetText(const char *szToText);
	void SetReadOnly(bool fToVal) { if (fReadOnly = fToVal) AbortDropdown(false); }
	void SetSimple(bool fToVal) { fSimple = fToVal; }
	const StdStrBuf GetText() { return StdStrBuf(Text, false); }
	void SetFont(CStdFont *pToFont) { pUseFont = pToFont; }

	void SetColors(uint32_t dwFontClr, uint32_t dwBGClr, uint32_t dwBorderClr)
	{
		this->dwFontClr = dwFontClr; this->dwBGClr = dwBGClr; this->dwBorderClr = dwBorderClr;
	}

	void SetDecoration(C4Facet *pFctSideArrow)
	{
		this->pFctSideArrow = pFctSideArrow;
	}

	friend class ComboBox_FillCB;
};
}
