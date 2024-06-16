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
class DialogWindow;

// a dialog
class Dialog : public Window
{
private:
	enum Fade { eFadeNone = 0, eFadeOut, eFadeIn };

	C4KeyBinding *pKeyAdvanceControl, *pKeyAdvanceControlB, *pKeyHotkey, *pKeyEnter, *pKeyEscape, *pKeyFocusDefControl;

protected:
	WoodenLabel *pTitle; // title bar text
	CallbackButton<Dialog, C4GUI::IconButton> *pCloseBtn;
	Control *pActiveCtrl; // control that has focus
	bool fShow; // if set, the dlg is shown
	bool fOK; // if set, the user pressed OK
	int32_t iFade; // dlg fade (percent)
	Fade eFade; // fading mode
	bool fDelOnClose; // auto-delete when closing
	StdStrBuf TitleString;
	bool fViewportDlg; // set in ctor: if true, dlg is not independent, but drawn ad controlled within viewports
	DialogWindow *pWindow; // window in console mode
	CStdGLCtx *pCtx; // rendering context for OpenGL
	FrameDecoration *pFrameDeco;

	bool CreateConsoleWindow();
	void DestroyConsoleWindow();

	virtual void UpdateSize() override; // called when own size changed - update assigned pWindow

public:
	Dialog(int32_t iWdt, int32_t iHgt, const char *szTitle, bool fViewportDlg);
	virtual ~Dialog();

	virtual void RemoveElement(Element *pChild) override; // clear ptr

	virtual void Draw(C4FacetEx &cgo) override; // render dialog (published)
	virtual void DrawElement(C4FacetEx &cgo) override; // draw dlg bg
	virtual bool IsComponentOutsideClientArea() override { return !!pTitle; } // pTitle lies outside client area

	virtual const char *GetID() { return nullptr; }

	// special handling for viewport dialogs
	virtual void ApplyElementOffset(int32_t &riX, int32_t &riY) override;
	virtual void ApplyInvElementOffset(int32_t &riX, int32_t &riY) override;

	virtual bool IsFocused(Control *pCtrl) override { return pCtrl == pActiveCtrl; }
	void SetFocus(Control *pCtrl, bool fByMouse);
	Control *GetFocus() { return pActiveCtrl; }
	virtual Dialog *GetDlg() override { return this; } // this is the dialog

	virtual bool CharIn(const char *c); // input: character key pressed - should return false for none-character-inputs  (forward to focused control)
	virtual void MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, uint32_t dwKeyParam) override; // input: mouse. forwards to child controls

private:
	bool KeyHotkey(C4KeyCodeEx key);
	bool KeyFocusDefault();

public:
	// default control to be set if unprocessed keyboard input has been detected
	virtual class Control *GetDefaultControl() { return nullptr; }

	// default dlg actions for enter/escape
	virtual bool OnEnter() { UserClose(true); return true; }
	bool KeyEnter() { return OnEnter(); }
	virtual bool OnEscape() { UserClose(false); return true; }
	bool KeyEscape() { return OnEscape(); }

	void AdvanceFocus(bool fBackwards); // change focus to next component
	bool KeyAdvanceFocus(bool fBackwards) { AdvanceFocus(fBackwards); return true; }

	virtual int32_t GetMarginTop() override    { return (pTitle ? pTitle->GetBounds().Hgt : 0) + (pFrameDeco ? pFrameDeco->iBorderTop : Window::GetMarginTop()); }
	virtual int32_t GetMarginLeft() override   { return pFrameDeco ? pFrameDeco->iBorderLeft   : Window::GetMarginLeft(); }
	virtual int32_t GetMarginRight() override  { return pFrameDeco ? pFrameDeco->iBorderRight  : Window::GetMarginRight(); }
	virtual int32_t GetMarginBottom() override { return pFrameDeco ? pFrameDeco->iBorderBottom : Window::GetMarginBottom(); }

	bool IsShown() { return fShow; } // returns whether dlg is on screen (may be invisible)
	bool IsAborted() { return !fShow && !fOK; } // returns whether dialog has been aborted
	bool IsActive(bool fForKeyboard); // return whether dlg has mouse control focus
	bool IsFading() { return eFade != eFadeNone; }

	virtual bool IsFullscreenDialog() { return false; }
	virtual bool HasBackground() { return false; } // true if dlg draws screen background (fullscreen dialogs only)

	// true for dialogs that should span the whole screen
	// not just the mouse-viewport
	virtual bool IsFreePlaceDialog() { return false; }

	// true for dialogs that should be placed at the bottom of the screen (chat)
	virtual bool IsBottomPlacementDialog() { return false; }

	// true for dialogs that receive full keyboard and mouse input even in shared mode
	virtual bool IsExclusiveDialog() { return false; }

	// some dialogs, like menus or chat control, don't really need a mouse
	// so do not enable it for those in shared mode if mouse control is disabled
	virtual bool IsMouseControlled() { return true; }

	// For dialogs associated to a viewport: Return viewport (for placement)
	virtual C4Viewport *GetViewport() { return nullptr; }
	bool IsViewportDialog() { return fViewportDlg; }

	// for custom placement procedures; should call SetPos
	virtual bool DoPlacement(Screen *pOnScreen, const C4Rect &rPreferredDlgRect) { return false; }

	// true for dialogs drawn externally
	virtual bool IsExternalDrawDialog() override { return false; }

	// z-ordering used for dialog placement
	virtual int32_t GetZOrdering() { return C4GUI_Z_DEFAULT; }

	bool Show(Screen *pOnScreen, bool fCB); // show dialog on screen - default to last created screen
	void Close(bool fOK); // close dlg
	bool FadeIn(Screen *pOnScreen); // fade dlg into screen
	void FadeOut(bool fCloseWithOK); // fade out dlg
	bool DoModal(); // execute message loop until dlg is closed (or GUI destructed) - returns whether dlg was OK
	bool Execute(); // execute dialog - does message handling, gfx output and idle proc; return false if dlg got closed or GUI deleted
	void SetDelOnClose(bool fToVal = true) { fDelOnClose = fToVal; } // dialog will delete itself when closed

	void SetTitle(const char *szToTitle, bool fShowCloseButton = true); // change title text; creates or removes title bar if necessary

	void SetFrameDeco(FrameDecoration *pNewDeco) // change border decoration
	{
		if (pFrameDeco) pFrameDeco->Deref();
		if (pFrameDeco = pNewDeco) pNewDeco->Ref();
		UpdateOwnPos(); // margin may have changed; might need to reposition stuff
	}

	void ClearFrameDeco() // clear border decoration; no own pos update!
	{
		if (pFrameDeco) pFrameDeco->Deref(); pFrameDeco = nullptr;
	}

	FrameDecoration *GetFrameDecoration() const { return pFrameDeco; }
	void SetClientSize(int32_t iToWdt, int32_t iToHgt); // resize dialog so its client area has the specified size

	void OnUserClose(C4GUI::Control *btn) // user presses close btn: Usually close dlg with abort
	{
		UserClose(false);
	}

	virtual void UserClose(bool fOK) { Close(fOK); }
	virtual void OnClosed(bool fOK); // callback when dlg got closed
	virtual void OnShown() {}        // callback when shown - should not delete the dialog
	virtual void OnIdle() {}         // idle proc in DoModal

	virtual ContextHandler *GetContextHandler() override // always use own context handler only (no fall-through to screen)
	{
		return pContextHandler;
	}

	friend class Screen;
};

// a dialog covering the whole screen (using UpperBoard-caption)
class FullscreenDialog : public Dialog
{
protected:
	Label *pFullscreenTitle, *pSubTitle; // subtitle to be put in upper-right corner
	int32_t iDlgMarginX, iDlgMarginY; // dialog margin set by screen size
	IconButton *pBtnHelp;

	virtual const char *GetID() override { return nullptr; } // no ID needed, because it's never created as a window

public:
	FullscreenDialog(const char *szTitle, const char *szSubtitle);

	void SetTitle(const char *szToTitle); // change title text; creates or removes title bar if necessary

private:
	void UpdateHelpButtonPos();

protected:
	virtual void DrawElement(C4FacetEx &cgo) override; // draw dlg bg

	// fullscreen dialogs are not closed on Enter
	virtual bool OnEnter() override { return false; }

	virtual bool IsComponentOutsideClientArea() override { return true; }

	virtual bool HasUpperBoard() { return false; } // standard fullscreen dialog: UpperBoard no longer present

	virtual bool IsFullscreenDialog() override { return true; }
	virtual bool DoPlacement(Screen *pOnScreen, const C4Rect &rPreferredDlgRect) override { return true; } // fullscreen dlg already placed

	virtual int32_t GetMarginTop() override;
	virtual int32_t GetMarginLeft() override   { return iDlgMarginX; }
	virtual int32_t GetMarginRight() override  { return iDlgMarginX; }
	virtual int32_t GetMarginBottom() override { return iDlgMarginY; }
	virtual void UpdateOwnPos() override; // called when element bounds were changed externally

	// helper func: draw facet to screen background
	void DrawBackground(C4FacetEx &cgo, C4Facet &rFromFct);
};

// a simple message dialog
class MessageDialog : public Dialog
{
private:
	bool fHasOK;
	bool *piConfigDontShowAgainSetting;
	const int32_t zOrdering;

protected:
	Label *lblText;

public:
	enum Buttons
	{
		btnOK = 1, btnAbort = 2, btnYes = 4, btnNo = 8, btnRetry = 16,
		btnOKAbort = btnOK | btnAbort, btnYesNo = btnYes | btnNo, btnRetryAbort = btnRetry | btnAbort,
	};

	enum DlgSize { dsRegular = C4GUI_MessageDlgWdt, dsMedium = C4GUI_MessageDlgWdtMedium, dsSmall = C4GUI_MessageDlgWdtSmall };

	MessageDialog(const char *szMessage, const char *szCaption, uint32_t dwButtons, Icons icoIcon, DlgSize eSize = dsRegular, bool *piConfigDontShowAgainSetting = nullptr, bool fDefaultNo = false, int32_t zOrdering = C4GUI_Z_INPUT);

protected:
	virtual bool OnEnter() override { if (!fHasOK) return false; Close(true); return true; }

	void OnDontShowAgainCheck(C4GUI::Element *pCheckBox)
	{
		if (piConfigDontShowAgainSetting) *piConfigDontShowAgainSetting = static_cast<C4GUI::CheckBox *>(pCheckBox)->GetChecked();
	}

	virtual const char *GetID() override { return "MessageDialog"; }
	virtual int32_t GetZOrdering() override { return zOrdering; }
};

// a confirmation dialog, which performs a callback after confirmation
class ConfirmationDialog : public MessageDialog
{
private:
	BaseCallbackHandler *pCB;

public:
	ConfirmationDialog(const char *szMessage, const char *szCaption, BaseCallbackHandler *pCB, uint32_t dwButtons = MessageDialog::btnOKAbort, bool fSmall = false, Icons icoIcon = Ico_Confirm);
	~ConfirmationDialog() { if (pCB) pCB->DeRef(); }

protected:
	virtual void OnClosed(bool fOK) override; // callback when dlg got closed
};

// a simple progress dialog
class ProgressDialog : public Dialog
{
protected:
	ProgressBar *pBar; // progress bar component

	virtual const char *GetID() override { return "ProgressDialog"; }

public:
	ProgressDialog(const char *szMessage, const char *szCaption, int32_t iMaxProgress, int32_t iInitialProgress, Icons icoIcon);

	void SetProgress(int32_t iToProgress) { pBar->SetProgress(iToProgress); } // change progress
	virtual bool OnEnter() override { return false; } // don't close on Enter!
};

// a dialog for a one-line text input; contains OK and cancel button
class InputDialog : public Dialog
{
protected:
	Edit *pEdit; // edit for text input
	BaseInputCallback *pCB;
	C4Rect rcEditBounds;
	bool fChatLayout;
	Label *pChatLbl;

	virtual const char *GetID() override { return "InputDialog"; }

public:
	InputDialog(const char *szMessage, const char *szCaption, Icons icoIcon, BaseInputCallback *pCB, bool fChatLayout = false);
	~InputDialog() { delete pCB; }

	virtual void OnClosed(bool fOK) override; // close CB
	void SetMaxText(int32_t iMaxLen);
	void SetInputText(const char *szToText);
	const char *GetInputText();
	void SetCustomEdit(Edit *pCustomEdit);
};

// a dialog showing some information text
class InfoDialog : public Dialog
{
private:
	int32_t iScroll; // current scroll pos; backup for text update
	C4Sec1TimerCallback<InfoDialog> *pSec1Timer; // engine timer hook for text update

protected:
	TextWindow *pTextWin;

	void CreateSubComponents(); // ctor func

	// add text to the info window
	void AddLine(const char *szText);

	void BeginUpdateText(); // backup scrolling and clear text window
	void EndUpdateText();   // restore scroll pos; set last update time

	virtual void UpdateText() {} // function to be overwritten for timed dlgs: Update window text

	virtual void OnSec1Timer(); // idle proc: update text if necessary

public:
	InfoDialog(const char *szCaption, int32_t iLineCount);
	InfoDialog(const char *szCaption, int iLineCount, const StdStrBuf &sText); // init w/o timer
	~InfoDialog();

	friend class C4Sec1TimerCallback<InfoDialog>;
};

// message dialog with a timer
class TimedDialog : public MessageDialog
{
private:
	C4Sec1TimerCallback<TimedDialog> *sec1Timer;
	uint32_t time;

public:
	TimedDialog(uint32_t time, const char *message, const char *caption, uint32_t buttons, Icons icon, DlgSize size = dsRegular, bool *configDontShowAgainSetting = nullptr, bool defaultNo = false, int32_t zOrdering = C4GUI_Z_INPUT);
	virtual ~TimedDialog();
	void OnSec1Timer();

protected:
	virtual const char *GetID() override { return "TimedDialog"; }
	virtual void UpdateText() {}
	void SetText(const char *message);
	uint32_t GetRemainingTime() const { return time; }
};
}
