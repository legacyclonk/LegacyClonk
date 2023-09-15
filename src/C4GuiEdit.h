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
// an edit control to type text in
class Edit : public Control
{
public:
	Edit(const C4Rect &rtBounds, bool fFocusEdit = false);
	~Edit();

private:
	enum CursorOperation { COP_BACK, COP_DELETE, COP_LEFT, COP_RIGHT, COP_HOME, COP_END, };

	bool KeyCursorOp(C4KeyCodeEx key, CursorOperation op);
	bool KeyEnter();
	bool KeyCopy() { Copy(); return true; }
	bool KeyPaste() { Paste(); return true; }
	bool KeyCut() { Cut(); return true; }
	bool KeySelectAll() { SelectAll(); return true; }

	class C4KeyBinding *RegisterCursorOp(CursorOperation op, C4KeyCode key, const char *szName, C4CustomKey::Priority eKeyPrio);

	class C4KeyBinding *pKeyCursorBack, *pKeyCursorDel, *pKeyCursorLeft, *pKeyCursorRight, *pKeyCursorHome, *pKeyCursorEnd,
		*pKeyEnter, *pKeyCopy, *pKeyPaste, *pKeyCut, *pKeySelAll;

protected:
	// context callbacks
	ContextMenu *OnContext(C4GUI::Element *pListItem, int32_t iX, int32_t iY);
	void OnCtxCopy(C4GUI::Element *pThis)   { Copy(); }
	void OnCtxPaste(C4GUI::Element *pThis)  { Paste(); }
	void OnCtxCut(C4GUI::Element *pThis)    { Cut(); }
	void OnCtxClear(C4GUI::Element *pThis)  { DeleteSelection(); }
	void OnCtxSelAll(C4GUI::Element *pThis) { SelectAll(); }

private:
	void Deselect(); // clear selection range

public:
	bool InsertText(const char *szText, bool fUser); // insert text at cursor pos (returns whether all text could be inserted)
	void ClearText(); // remove all the text
	void DeleteSelection(); // deletes the selected text. Adjust cursor position if necessary
	bool SetText(const char *szText, bool fUser) { ClearText(); return InsertText(szText, fUser); }
	void SetPasswordMask(char cNewPasswordMask) { cPasswordMask = cNewPasswordMask; } // mask edit box contents using the given character

private:
	int32_t GetCharPos(int32_t iControlXPos); // get character index of pixel position; always resides within current text length
	void EnsureBufferSize(int32_t iMinBufferSize); // ensure buffer has desired size
	void ScrollCursorInView(); // ensure cursor pos is visible in edit control
	bool DoFinishInput(bool fPasting, bool fPastingMore); // do OnFinishInput callback and process result - returns whether pasting operation should be continued

	bool Copy(); bool Cut(); bool Paste(); // clipboard operations

protected:
	CStdFont *pFont; // font for edit
	char *Text; // edit text
	uint32_t dwBGClr, dwFontClr, dwBorderColor; // drawing colors for edit box
	int32_t iBufferSize; // size of current buffer
	int32_t iCursorPos; // cursor position: char, before which the cursor is located
	int32_t iSelectionStart, iSelectionEnd; // selection range (start may be larger than end)
	int32_t iMaxTextLength; // maximum number of characters to be input here
	uint32_t dwLastInputTime; // time of last input (for cursor flashing)
	int32_t iXScroll; // horizontal scrolling
	char cPasswordMask; // character to be used for masking the contents. 0 for none.

	bool fLeftBtnDown; // flag whether left mouse button is down or not

	virtual bool CharIn(const char *c) override; // input: character key pressed - should return false for none-character-inputs
	virtual void MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, uint32_t dwKeyParam) override; // input: mouse movement or buttons
	virtual void DoDragging(CMouse &rMouse, int32_t iX, int32_t iY, uint32_t dwKeyParam) override; // dragging: allow text selection outside the component
	virtual bool IsFocusOnClick() override { return true; } // edit fields do get focus on click
	virtual void OnGetFocus(bool fByMouse) override; // edit control gets focus
	virtual void OnLooseFocus() override; // edit control looses focus

	virtual void DrawElement(C4FacetEx &cgo) override; // draw edit control

	// called when user presses enter in single-line edit control - closes the current dialog
	virtual InputResult OnFinishInput(bool fPasting, bool fPastingMore) { return IR_CloseDlg; }
	virtual void OnAbortInput() {}
	virtual void OnTextChange() {}

	// get margins from bounds to client rect
	virtual int32_t GetMarginTop() override    { return 2; }
	virtual int32_t GetMarginLeft() override   { return 4; }
	virtual int32_t GetMarginRight() override  { return 4; }
	virtual int32_t GetMarginBottom() override { return 2; }

public:
	const char *GetText() { return Text; }
	void SelectAll(); // select all the text

	static int32_t GetDefaultEditHeight();
	static int32_t GetCustomEditHeight(CStdFont *pUseFont);

	bool GetCurrentWord(char *szTargetBuf, int32_t iMaxTargetBufLen); // get word before cursor pos (for nick completion)

	// layout
	void SetFont(CStdFont *pToFont) { pFont = pToFont; ScrollCursorInView(); }

	void SetColors(uint32_t dwNewBGClr, uint32_t dwNewFontClr, uint32_t dwNewBorderColor)
	{
		dwBGClr = dwNewBGClr; dwFontClr = dwNewFontClr; dwBorderColor = dwNewBorderColor;
	}

	void SetMaxText(int32_t iTo) { iMaxTextLength = iTo; }
};

// an edit doing some callback
template <class CallbackCtrl> class CallbackEdit : public Edit
{
private:
	CallbackCtrl *pCBCtrl;

protected:
	typedef InputResult(CallbackCtrl::*CBFunc)(Edit *, bool, bool);
	typedef void (CallbackCtrl::*CBAbortFunc)();
	CBFunc pCBFunc; CBAbortFunc pCBAbortFunc;

	virtual InputResult OnFinishInput(bool fPasting, bool fPastingMore) override
	{
		if (pCBFunc && pCBCtrl) return (pCBCtrl->*pCBFunc)(this, fPasting, fPastingMore); else return IR_CloseDlg;
	}

	virtual void OnAbortInput() override
	{
		if (pCBAbortFunc && pCBCtrl)(pCBCtrl->*pCBAbortFunc)();
	}

public:
	CallbackEdit(const C4Rect &rtBounds, CallbackCtrl *pCBCtrl, CBFunc pCBFunc, CBAbortFunc pCBAbortFunc = nullptr)
		: Edit(rtBounds), pCBCtrl(pCBCtrl), pCBFunc(pCBFunc), pCBAbortFunc(pCBAbortFunc) {}
};

// an edit control that renames a label - some less decoration; abort on Escape and focus loss
class RenameEdit : public Edit
{
private:
	C4KeyBinding *pKeyAbort; // key bindings
	bool fFinishing; // set during deletion process
	Label *pForLabel; // label that is being renamed
	Control *pPrevFocusCtrl; // previous focus element to be restored after rename

public:
	RenameEdit(Label *pLabel); // construct for label; add element; set focus
	virtual ~RenameEdit();

	void Abort();

private:
	void FinishRename(); // renaming aborted or finished - remove this element and restore label

protected:
	bool KeyAbort() { Abort(); return true; }
	virtual InputResult OnFinishInput(bool fPasting, bool fPastingMore) override; // forward last input to OnOKRename
	virtual void OnLooseFocus() override; // callback when control looses focus: OK input

	virtual void OnCancelRename() {} // renaming was aborted
	virtual RenameResult OnOKRename(const char *szNewName) = 0; // rename performed - return whether name was accepted
};

template <class CallbackDlg, class ParType> class CallbackRenameEdit : public RenameEdit
{
protected:
	typedef void (CallbackDlg::*CBCancelFunc)(ParType);
	typedef RenameResult(CallbackDlg::*CBOKFunc)(ParType, const char *);

	CBCancelFunc pCBCancelFunc; CBOKFunc pCBOKFunc;
	CallbackDlg *pDlg; ParType par;

	virtual void OnCancelRename() override { if (pDlg && pCBCancelFunc)(pDlg->*pCBCancelFunc)(par); }
	virtual RenameResult OnOKRename(const char *szNewName) override { return (pDlg && pCBOKFunc) ? (pDlg->*pCBOKFunc)(par, szNewName) : RR_Accepted; }

public:
	CallbackRenameEdit(Label *pForLabel, CallbackDlg *pDlg, const ParType &par, CBOKFunc pCBOKFunc, CBCancelFunc pCBCancelFunc)
		: RenameEdit(pForLabel), pDlg(pDlg), par(par), pCBOKFunc(pCBOKFunc), pCBCancelFunc(pCBCancelFunc) {}
};

// editbox below descriptive label sharing one window for common tooltip
class LabeledEdit : public C4GUI::Window
{
public:
	LabeledEdit(const C4Rect &rcBounds, const char *szName, bool fMultiline, const char *szPrefText = nullptr, CStdFont *pUseFont = nullptr, uint32_t dwTextClr = C4GUI_CaptionFontClr);

private:
	C4GUI::Edit *pEdit;

public:
	const char *GetText() const { return pEdit->GetText(); }
	C4GUI::Edit *GetEdit() const { return pEdit; }
	static bool GetControlSize(int *piWdt, int *piHgt, const char *szForText, CStdFont *pForFont, bool fMultiline);
};
}
