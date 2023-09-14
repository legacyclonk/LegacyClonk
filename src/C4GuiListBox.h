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
// a vertical list of elements
class ListBox : public Control
{
private:
	class C4KeyBinding *pKeyContext, *pKeyUp, *pKeyDown, *pKeyPageUp, *pKeyPageDown, *pKeyHome, *pKeyEnd, *pKeyActivate, *pKeyLeft, *pKeyRight;

	bool KeyContext();
	bool KeyUp();
	bool KeyDown();
	bool KeyLeft();
	bool KeyRight();
	bool KeyPageUp();
	bool KeyPageDown();
	bool KeyHome();
	bool KeyEnd();
	bool KeyActivate();

protected:
	int32_t iMultiColItemWidth; // if nonzero, the listbox is multicolumn and the column count depends on how many items fit in
	int32_t iColCount; // number of columns (usually 1)
	ScrollWindow *pClientWindow; // client scrolling window
	Element *pSelectedItem; // selected list item
	BaseCallbackHandler *pSelectionChangeHandler, *pSelectionDblClickHandler;
	bool fDrawBackground; // whether darker background is to be drawn
	bool fDrawBorder; // whether 3D frame around box shall be drawn or nay
	bool fSelectionDisabled; // if set, no entries can be selected

	virtual void DrawElement(C4FacetEx &cgo) override; // draw listbox

	virtual bool IsFocused(Control *pCtrl) override
	{
		// subcontrol also counts as focused if the list has focus and the subcontrol is selected
		return Control::IsFocused(pCtrl) || (HasFocus() && pSelectedItem == pCtrl);
	}

	virtual void MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, uint32_t dwKeyParam) override; // input: mouse movement or buttons
	virtual bool IsFocusOnClick() override { return true; } // list boxes do get focus on click
	virtual Control *IsFocusElement() override { return this; } // this control can gain focus
	virtual void OnGetFocus(bool fByMouse) override; // callback when control gains focus - select a list item if none are selected
	virtual bool CharIn(const char *c) override; // character input for direct list element selection

	virtual void AfterElementRemoval() override
	{
		Container::AfterElementRemoval(); UpdateElementPositions();
	} // called by ScrollWindow to parent after an element has been removed

	void UpdateColumnCount();

public:
	ListBox(const C4Rect &rtBounds, int32_t iMultiColItemWidth = 0);
	virtual ~ListBox();

	virtual void RemoveElement(Element *pChild) override; // remove child component
	bool AddElement(Element *pChild, int32_t iIndent = 0); // add element and adjust its pos
	bool InsertElement(Element *pChild, Element *pInsertBefore, int32_t iIndent = 0); // insert element and adjust its pos
	virtual void ElementSizeChanged(Element *pOfElement) override; // called when an element size is changed
	virtual void ElementPosChanged(Element *pOfElement) override; // called when an element position is changed

	int32_t GetItemWidth() { return iMultiColItemWidth ? iMultiColItemWidth : pClientWindow->GetClientRect().Wdt; }

	void SelectionChanged(bool fByUser); // pSelectedItem changed: sound, tooltip, etc.

	void SetSelectionChangeCallbackFn(BaseCallbackHandler *pToHandler) // update selection change handler
	{
		if (pSelectionChangeHandler) pSelectionChangeHandler->DeRef();
		if ((pSelectionChangeHandler = pToHandler)) pToHandler->Ref();
	}

	void SetSelectionDblClickFn(BaseCallbackHandler *pToHandler) // update selection doubleclick handler
	{
		if (pSelectionDblClickHandler) pSelectionDblClickHandler->DeRef();
		if ((pSelectionDblClickHandler = pToHandler)) pSelectionDblClickHandler->Ref();
	}

	void ScrollToBottom() // set scrolling to bottom range
	{
		if (pClientWindow) pClientWindow->ScrollToBottom();
	}

	void ScrollItemInView(Element *pItem); // set scrolling so a specific item is visible
	void FreezeScrolling() { pClientWindow->Freeze(); }
	void UnFreezeScrolling() { pClientWindow->UnFreeze(); }

	// change style
	void SetDecoration(bool fDrawBG, ScrollBarFacets *pToGfx, bool fAutoScroll, bool fDrawBorder = false)
	{
		fDrawBackground = fDrawBG; this->fDrawBorder = fDrawBorder; if (pClientWindow) pClientWindow->SetDecoration(pToGfx, fAutoScroll);
	}

	void SetSelectionDiabled(bool fToVal = true) { fSelectionDisabled = fToVal; }

	// get head and tail list items
	Element *GetFirst() { return pClientWindow ? pClientWindow->GetFirst() : nullptr; }
	Element *GetLast()  { return pClientWindow ? pClientWindow->GetLast()  : nullptr; }

	// get margins from bounds to client rect
	virtual int32_t GetMarginTop() override    { return 3; }
	virtual int32_t GetMarginLeft() override   { return 3; }
	virtual int32_t GetMarginRight() override  { return 3; }
	virtual int32_t GetMarginBottom() override { return 3; }

	Element *GetSelectedItem() { return pSelectedItem; } // get focused listbox item
	bool IsScrollingActive()    { return pClientWindow && pClientWindow->IsScrollingActive(); }
	bool IsScrollingNecessary() { return pClientWindow && pClientWindow->IsScrollingNecessary(); }
	void SelectEntry(Element *pNewSel, bool fByUser);
	void SelectFirstEntry(bool fByUser) { SelectEntry(GetFirst(), fByUser); }
	void SelectNone(bool fByUser) { SelectEntry(nullptr, fByUser); }
	bool IsMultiColumn() const { return iColCount > 1; }
	int32_t ContractToElementHeight(); // make smaller if elements don't use up all of the available height. Return amount by which list box got contracted

	void UpdateElementPositions(); // reposition list items so they are stacked vertically
	void UpdateElementPosition(Element *pOfElement, int32_t iIndent); // update pos for one specific element
	virtual void UpdateSize() override { Control::UpdateSize(); if (pClientWindow) { pClientWindow->UpdateSize(); UpdateElementPositions(); } }

	virtual bool IsSelectedChild(Element *pChild) override { return pChild == pSelectedItem || (pSelectedItem && pSelectedItem->IsParentOf(pChild)); }

	typedef int32_t(*SortFunction)(const Element *pEl1, const Element *pEl2, void *par);
	void SortElements(SortFunction SortFunc, void *par); // sort list items
};
}
