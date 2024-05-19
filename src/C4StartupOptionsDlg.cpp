/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2005, Sven2
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

// Startup screen for non-parameterized engine start: Options dialog

#include "C4GuiComboBox.h"
#include "C4GuiEdit.h"
#include "C4GuiResource.h"
#include "C4GuiTabular.h"

#include <C4Include.h>
#include <C4StartupOptionsDlg.h>
#include <C4StartupOptionsAdvancedConfigDialog.h>

#include <C4StartupMainDlg.h>
#include <C4Language.h>
#include <C4GamePadCon.h>
#include <C4Game.h>
#include <C4Log.h>
#include <C4Wrappers.h>

#include <StdGL.h>

class C4StartupOptionsDlg::ScaleEdit : public C4GUI::SpinBox<int32_t>
{
	using Base = C4GUI::SpinBox<int32_t>;
	C4StartupOptionsDlg *pDlg;

public:
	ScaleEdit(C4StartupOptionsDlg *pDlg, const C4Rect &rtBounds, bool fFocusEdit = false);

protected:
	virtual void OnTextChange() override;
	virtual C4GUI::InputResult OnFinishInput(bool fPasting, bool fPastingMore) override;
};

class C4StartupOptionsDlg::EditConfig : public C4GUI::LabeledEdit
{
public:
	EditConfig(const C4Rect &rcBounds, const char *szName, ValidatedStdStrBufBase *psConfigVal, int32_t *piConfigVal, bool fMultiline);

private:
	ValidatedStdStrBufBase *psConfigVal;
	int32_t *piConfigVal;

public:
	void Save2Config(); // control to config
	static bool GetControlSize(int *piWdt, int *piHgt, const char *szForText, bool fMultiline);
	int32_t GetIntVal();
	void SetIntVal(int32_t iToVal);
};

// C4StartupOptionsDlg::SmallButton

void C4StartupOptionsDlg::SmallButton::DrawElement(C4FacetEx &cgo)
{
	// draw the button
	CStdFont &rUseFont = C4Startup::Get()->Graphics.BookFont;
	// get text pos
	int32_t x0 = cgo.TargetX + rcBounds.x, y0 = cgo.TargetY + rcBounds.y, x1 = cgo.TargetX + rcBounds.x + rcBounds.Wdt, y1 = cgo.TargetY + rcBounds.y + rcBounds.Hgt;
	int32_t iTextHgt = rUseFont.GetLineHeight();
	// draw frame
	uint32_t dwClrHigh = C4StartupBtnBorderColor1, dwClrLow = C4StartupBtnBorderColor2;
	if (fDown) std::swap(dwClrHigh, dwClrLow);
	int32_t iIndent = BoundBy<int32_t>((rcBounds.Hgt - iTextHgt) / 3, 2, 5);
	int iDrawQuadTop[8] = { x0, y0, x1, y0, x1 - iIndent, y0 + iIndent, x0, y0 + iIndent };
	int iDrawQuadLeft[8] = { x0, y0, x0 + iIndent, y0, x0 + iIndent, y1 - iIndent, x0, y1 };
	int iDrawQuadRight[8] = { x1, y0, x1, y1, x1 - iIndent, y1, x1 - iIndent, y0 + iIndent };
	int iDrawQuadBottom[8] = { x1, y1, x0, y1, x0 + iIndent, y1 - iIndent, x1, y1 - iIndent };
	lpDDraw->DrawQuadDw(cgo.Surface, iDrawQuadTop, dwClrHigh, dwClrHigh, dwClrHigh, dwClrHigh);
	lpDDraw->DrawQuadDw(cgo.Surface, iDrawQuadLeft, dwClrHigh, dwClrHigh, dwClrHigh, dwClrHigh);
	lpDDraw->DrawQuadDw(cgo.Surface, iDrawQuadRight, dwClrLow, dwClrLow, dwClrLow, dwClrLow);
	lpDDraw->DrawQuadDw(cgo.Surface, iDrawQuadBottom, dwClrLow, dwClrLow, dwClrLow, dwClrLow);
	// draw selection highlight
	int32_t iTxtOff = fDown ? iIndent : 0;
	if (fEnabled) if (HasDrawFocus() || (fMouseOver && IsInActiveDlg(false)))
	{
		lpDDraw->SetBlitMode(C4GFXBLIT_ADDITIVE);
		C4GUI::GetRes()->fctButtonHighlight.DrawX(cgo.Surface, x0 + 5 + iTxtOff, y0 + 3 + iTxtOff, rcBounds.Wdt - 10, rcBounds.Hgt - 6);
		lpDDraw->ResetBlitMode();
	}
	// draw button text
	lpDDraw->TextOut(sText.getData(), rUseFont, 1.0f, cgo.Surface, (x0 + x1) / 2 + iTxtOff, (y0 + y1 - iTextHgt) / 2 + iTxtOff, C4StartupBtnFontClr, ACenter, true);
}

int32_t C4StartupOptionsDlg::SmallButton::GetDefaultButtonHeight()
{
	// button height is used font height plus a small indent
	CStdFont *pUseFont = &(C4Startup::Get()->Graphics.BookFont);
	return pUseFont->GetLineHeight() * 6 / 5 + 6;
}

// C4StartupOptionsDlg::ResChangeConfirmDlg

C4StartupOptionsDlg::ResChangeConfirmDlg::ResChangeConfirmDlg()
	: C4GUI::TimedDialog{12, "", LoadResStr("IDS_MNU_SWITCHRESOLUTION"), C4GUI::MessageDialog::btnYesNo, C4GUI::Ico_None}
{
	UpdateText();
}

void C4StartupOptionsDlg::ResChangeConfirmDlg::UpdateText()
{
	StdStrBuf text;
	C4GUI::GetRes()->TextFont.BreakMessage(
		FormatString(LoadResStr("IDS_MNU_SWITCHRESOLUTION_TEXT"), GetRemainingTime()).getData(),
		GetClientRect().Wdt,
		&text,
		false
	);
	SetText(text.getData());
}

// C4StartupOptionsDlg::ScaleEdit

namespace
{
	constexpr int minScale = 100;
	constexpr int maxScale = 300;
}

C4StartupOptionsDlg::ScaleEdit::ScaleEdit(C4StartupOptionsDlg *pDlg, const C4Rect &rtBounds, bool fFocusEdit) : Base{rtBounds, fFocusEdit, minScale, maxScale}, pDlg{pDlg}
{
	SetColors(C4StartupEditBGColor, C4StartupFontClr, C4StartupEditBorderColor);
}

void C4StartupOptionsDlg::ScaleEdit::OnTextChange()
{
	Base::OnTextChange();
	const auto val = GetValue();
	const auto sliderVal = val - minScale;
	pDlg->pScaleSlider->SetScrollPos(sliderVal);
	pDlg->iNewScale = val;
}

C4GUI::InputResult C4StartupOptionsDlg::ScaleEdit::OnFinishInput(bool fPasting, bool fPastingMore)
{
	Base::OnFinishInput(fPasting, fPastingMore);
	const auto val = GetValue();
	const auto sliderVal = val - minScale;
	pDlg->pScaleSlider->SetScrollPos(sliderVal);
	pDlg->OnScaleSliderChanged(sliderVal);
	pDlg->OnTestScaleBtn(nullptr);
	return C4GUI::IR_Abort;
}

// C4StartupOptionsDlg::KeySelDialog

const char *KeyID2Desc(int32_t iKeyID)
{
	const char *KeyIDStringIDs[C4MaxKey] =
	{
		"IDS_CTL_SELECTLEFT", "IDS_CTL_SELECTTOGGLE", "IDS_CTL_SELECTRIGHT",
		"IDS_CTL_THROW",      "IDS_CTL_UPJUMP",       "IDS_CTL_DIG",
		"IDS_CTL_LEFT",       "IDS_CTL_DOWNSTOP",     "IDS_CTL_RIGHT",
		"IDS_CTL_PLAYERMENU", "IDS_CTL_SPECIAL1",     "IDS_CTL_SPECIAL2"
	};
	if (!Inside<int32_t>(iKeyID, 0, C4MaxKey)) return nullptr;
	return LoadResStr(KeyIDStringIDs[iKeyID]);
}

C4StartupOptionsDlg::KeySelDialog::KeySelDialog(int32_t iKeyID, int32_t iCtrlSet, bool fGamepad)
	: C4GUI::MessageDialog(FormatString(LoadResStr(!fGamepad ? "IDS_MSG_PRESSKEY" : "IDS_MSG_PRESSBTN"),
		KeyID2Desc(iKeyID), iCtrlSet + 1).getData(), LoadResStr("IDS_MSG_DEFINEKEY"),
		C4GUI::MessageDialog::btnAbort, fGamepad ? C4GUI::Ico_Gamepad : C4GUI::Ico_Keyboard, C4GUI::MessageDialog::dsRegular),
	key(KEY_Undefined), fGamepad(fGamepad), iCtrlSet(iCtrlSet)
{
	pKeyListener = new C4KeyBinding(C4KeyCodeEx(KEY_Any, KEYS_None), "DefineKey", KEYSCOPE_Gui, new C4GUI::DlgKeyCBPassKey<C4StartupOptionsDlg::KeySelDialog>(*this, &C4StartupOptionsDlg::KeySelDialog::KeyDown), C4CustomKey::PRIO_PlrControl);
}

C4StartupOptionsDlg::KeySelDialog::~KeySelDialog()
{
	delete pKeyListener;
}

bool C4StartupOptionsDlg::KeySelDialog::KeyDown(C4KeyCodeEx key)
{
	// check if key is valid for this set
	// do not mix gamepad and keyboard keys
	if (Key_IsGamepad(key.Key) != fGamepad) return false;
	// allow selected gamepad only
	if (fGamepad && Key_GetGamepad(key.Key) != iCtrlSet) return false;
	// okay, use it
	this->key = key.Key;
	Close(true);
	return true;
}

// C4StartupOptionsDlg::KeySelButton

// key display arrangement - -1 denotes no key
// every key from 0 to C4MaxKey-1 MUST BE present here, or the engine will crash
const int32_t iKeyPosMaxX = 3, iKeyPosMaxY = 4; // arrange keys in a 4-by-5-array
const int32_t iKeyPosis[iKeyPosMaxY][iKeyPosMaxX] =
{
	{ 0,  1,  2 },
	{ 3,  4,  5 },
	{ 6,  7,  8 },
	{ 9, 10, 11 }
};

void C4StartupOptionsDlg::KeySelButton::DrawElement(C4FacetEx &cgo)
{
	// draw key
	C4Facet cgoDraw(cgo.Surface, rcBounds.x + cgo.TargetX, rcBounds.y + cgo.TargetY, rcBounds.Wdt, rcBounds.Hgt);
	Game.GraphicsResource.fctKey.Draw(cgoDraw, true, fDown);
	int32_t iKeyIndent = cgoDraw.Wdt / 5;
	cgoDraw.X += iKeyIndent; cgoDraw.Wdt -= 2 * iKeyIndent;
	cgoDraw.Y += iKeyIndent * 3 / 4; cgoDraw.Hgt -= 2 * iKeyIndent;
	if (fDown) cgoDraw.Y += iKeyIndent / 2;
	bool fDoHightlight = fHighlight || HasDrawFocus() || (fMouseOver && IsInActiveDlg(false));
	bool fHadBlitMod = false; uint32_t dwOldBlitModClr = 0xffffff;
	if (!fDoHightlight)
	{
		uint32_t dwModClr = 0x7f7f7f;
		if (fHadBlitMod = lpDDraw->GetBlitModulation(dwOldBlitModClr))
			ModulateClr(dwModClr, dwOldBlitModClr);
		lpDDraw->ActivateBlitModulation(dwModClr);
	}
	Game.GraphicsResource.fctCommand.Draw(cgoDraw, true, iKeyID, 0);
	if (!fDoHightlight)
		if (fHadBlitMod)
			lpDDraw->ActivateBlitModulation(dwOldBlitModClr);
		else
			lpDDraw->DeactivateBlitModulation();
	// draw the labels - beside the key
	float fZoom;
	CStdFont &rUseFont = C4Startup::Get()->Graphics.GetBlackFontByHeight(cgoDraw.Hgt / 2 + 5, &fZoom);
	lpDDraw->TextOut(KeyID2Desc(iKeyID), rUseFont, fZoom, cgo.Surface, cgo.TargetX + rcBounds.x + rcBounds.Wdt + 5, cgoDraw.Y - 3, fDoHightlight ? 0xffff0000 : C4StartupFontClr, ALeft, false);
	StdStrBuf strKey; strKey.Copy(C4KeyCodeEx::KeyCode2String(key, true, false));
	lpDDraw->TextOut(strKey.getData(), rUseFont, fZoom, cgo.Surface, cgo.TargetX + rcBounds.x + rcBounds.Wdt + 5, cgoDraw.Y + cgoDraw.Hgt / 2, fDoHightlight ? 0xffff0000 : C4StartupFontClr, ALeft, false);
}

C4StartupOptionsDlg::KeySelButton::KeySelButton(int32_t iKeyID, const C4Rect &rcBounds, char cHotkey)
	: C4GUI::IconButton(C4GUI::Ico_None, rcBounds, cHotkey), iKeyID(iKeyID), key(KEY_Undefined) {}

// C4StartupOptionsDlg::ControlConfigArea

C4StartupOptionsDlg::ControlConfigArea::ControlConfigArea(const C4Rect &rcArea, int32_t iHMargin, int32_t iVMargin, bool fGamepad, C4StartupOptionsDlg *pOptionsDlg)
	: C4GUI::Window(), fGamepad(fGamepad), pGamepadOpener(nullptr), pOptionsDlg(pOptionsDlg), pGUICtrl(nullptr)
{
	CStdFont *pUseFont = &(C4Startup::Get()->Graphics.BookFont);
	SetBounds(rcArea);
	C4GUI::ComponentAligner caArea(rcArea, iHMargin, iVMargin, true);
	// get number of control sets to be configured
	iMaxControlSets = 1; // do not devide by zero
	if (fGamepad && Application.pGamePadControl)
		iMaxControlSets = (std::max)(1, Application.pGamePadControl->GetGamePadCount());
	if (!fGamepad)
		iMaxControlSets = C4MaxKeyboardSet;
	ppKeyControlSetBtns = new C4GUI::IconButton *[iMaxControlSets];
	// top line buttons to select keyboard set or gamepad
	C4Facet fctCtrlPic = fGamepad ? Game.GraphicsResource.fctGamepad : Game.GraphicsResource.fctKeyboard;
	int32_t iCtrlSetWdt = caArea.GetWidth() - caArea.GetHMargin() * 2;
	int32_t iCtrlSetHMargin = 5, iCtrlSetVMargin = 5;
	int32_t iCtrlSetBtnWdt = BoundBy<int32_t>((iCtrlSetWdt - iMaxControlSets * iCtrlSetHMargin * 2) / iMaxControlSets, 5, fctCtrlPic.Wdt);
	int32_t iCtrlSetBtnHgt = fctCtrlPic.GetHeightByWidth(iCtrlSetBtnWdt);
	iCtrlSetHMargin = (iCtrlSetWdt - iCtrlSetBtnWdt * iMaxControlSets) / (iMaxControlSets * 2);
	C4GUI::ComponentAligner caKeyboardSetSel(caArea.GetFromTop(2 * iCtrlSetVMargin + iCtrlSetBtnHgt), iCtrlSetHMargin, iCtrlSetVMargin);
	const char *szCtrlSetHotkeys = "1234567890"; /* 2do */
	uint32_t i;
	for (i = 0; i < static_cast<uint32_t>(iMaxControlSets); ++i)
	{
		char cCtrlSetHotkey = 0;
		if (i <= strlen(szCtrlSetHotkeys)) cCtrlSetHotkey = szCtrlSetHotkeys[i];
		C4GUI::CallbackButton<C4StartupOptionsDlg::ControlConfigArea, C4GUI::IconButton> *pBtn = new C4GUI::CallbackButton<C4StartupOptionsDlg::ControlConfigArea, C4GUI::IconButton>(C4GUI::Ico_None, caKeyboardSetSel.GetFromLeft(iCtrlSetBtnWdt), cCtrlSetHotkey, &C4StartupOptionsDlg::ControlConfigArea::OnCtrlSetBtn, this);
		pBtn->SetFacet(fctCtrlPic);
		fctCtrlPic.X += fctCtrlPic.Wdt;
		AddElement(ppKeyControlSetBtns[i] = pBtn);
		pBtn->SetToolTip(LoadResStr("IDS_MSG_SELECTKEYSET"));
	}
	iSelectedCtrlSet = fGamepad ? 0 : C4P_Control_Keyboard1;
	caArea.ExpandTop(caArea.GetVMargin());
	AddElement(new C4GUI::HorizontalLine(caArea.GetFromTop(2)));
	caArea.ExpandTop(caArea.GetVMargin());
	C4Facet &rfctKey = Game.GraphicsResource.fctKey;
	int32_t iKeyAreaMaxWdt = caArea.GetWidth() - 2 * caArea.GetHMargin(), iKeyAreaMaxHgt = caArea.GetHeight() - 2 * caArea.GetVMargin();
	int32_t iKeyWdt = rfctKey.Wdt * 3 / 2, iKeyHgt = rfctKey.Hgt * 3 / 2;
	int32_t iKeyUseWdt = iKeyWdt + iKeyHgt * 3; // add space for label
	int32_t iKeyMargin = 20;
	int32_t iKeyAreaWdt = (iKeyUseWdt + 2 * iKeyMargin) * iKeyPosMaxX, iKeyAreaHgt = (iKeyHgt + 2 * iKeyMargin) * iKeyPosMaxY;
	if (iKeyAreaWdt > iKeyAreaMaxWdt || iKeyAreaHgt > iKeyAreaMaxHgt)
	{
		// scale down
		float fScaleX = float(iKeyAreaMaxWdt) / float(std::max<int32_t>(iKeyAreaWdt, 1)),
			fScaleY = float(iKeyAreaMaxHgt) / float(std::max<int32_t>(iKeyAreaHgt, 1)), fScale;
		if (fScaleX > fScaleY) fScale = fScaleY; else fScale = fScaleX;
		iKeyMargin = int32_t(fScale * iKeyMargin);
		iKeyWdt = int32_t(fScale * iKeyWdt);
		iKeyUseWdt = int32_t(fScale * iKeyUseWdt);
		iKeyHgt = int32_t(fScale * iKeyHgt);
		iKeyAreaWdt = int32_t(fScale * iKeyAreaWdt);
		iKeyAreaHgt = int32_t(fScale * iKeyAreaHgt);
	}
	C4GUI::ComponentAligner caCtrlKeys(caArea.GetFromTop(iKeyAreaHgt, iKeyAreaWdt), 0, iKeyMargin);
	int32_t iKeyNum;
	for (int iY = 0; iY < iKeyPosMaxY; ++iY)
	{
		C4GUI::ComponentAligner caCtrlKeysLine(caCtrlKeys.GetFromTop(iKeyHgt), iKeyMargin, 0);
		for (int iX = 0; iX < iKeyPosMaxX; ++iX)
		{
			C4Rect rcKey = caCtrlKeysLine.GetFromLeft(iKeyWdt);
			caCtrlKeysLine.ExpandLeft(iKeyWdt - iKeyUseWdt);
			if ((iKeyNum = iKeyPosis[iY][iX]) < 0) continue;
			KeySelButton *pKeyBtn = new C4GUI::CallbackButton<C4StartupOptionsDlg::ControlConfigArea, KeySelButton>(iKeyNum, rcKey, 0 /* no hotkey :( */, &C4StartupOptionsDlg::ControlConfigArea::OnCtrlKeyBtn, this);
			AddElement(KeyControlBtns[iKeyNum] = pKeyBtn);
			pKeyBtn->SetToolTip(KeyID2Desc(iKeyNum));
		}
	}
	// bottom area controls
	caArea.ExpandBottom(-iKeyHgt / 2);
	C4GUI::ComponentAligner caKeyBottomBtns(caArea.GetFromBottom(C4GUI_ButtonHgt), 2, 0);
	// gamepad: Use for GUI
	if (fGamepad)
	{
		int iWdt = 100, iHgt = 20;
		const char *szResetText = LoadResStr("IDS_CTL_GAMEPADFORMENU");
		C4GUI::CheckBox::GetStandardCheckBoxSize(&iWdt, &iHgt, szResetText, pUseFont);
		pGUICtrl = new C4GUI::CheckBox(caKeyBottomBtns.GetFromLeft(iWdt, iHgt), szResetText, !!Config.Controls.GamepadGuiControl);
		pGUICtrl->SetOnChecked(new C4GUI::CallbackHandler<C4StartupOptionsDlg::ControlConfigArea>(this, &C4StartupOptionsDlg::ControlConfigArea::OnGUIGamepadCheckChange));
		pGUICtrl->SetToolTip(LoadResStr("IDS_DESC_GAMEPADFORMENU"));
		pGUICtrl->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
		AddElement(pGUICtrl);
	}
	// reset button
	const char *szBtnText = LoadResStr("IDS_BTN_RESETKEYBOARD");
	int32_t iButtonWidth = 100, iButtonHeight = 20; C4GUI::Button *btn;
	C4GUI::GetRes()->CaptionFont.GetTextExtent(szBtnText, iButtonWidth, iButtonHeight, true);
	C4Rect rcResetBtn = caKeyBottomBtns.GetFromRight(std::min<int32_t>(iButtonWidth + iButtonHeight * 4, caKeyBottomBtns.GetInnerWidth()));
	AddElement(btn = new C4GUI::CallbackButton<C4StartupOptionsDlg::ControlConfigArea, SmallButton>(szBtnText, rcResetBtn, &C4StartupOptionsDlg::ControlConfigArea::OnResetKeysBtn, this));
	btn->SetToolTip(LoadResStr("IDS_MSG_RESETKEYSETS"));

	UpdateCtrlSet();
}

C4StartupOptionsDlg::ControlConfigArea::~ControlConfigArea()
{
	delete[] ppKeyControlSetBtns;
	delete pGamepadOpener;
}

void C4StartupOptionsDlg::ControlConfigArea::OnCtrlSetBtn(C4GUI::Control *btn)
{
	// select Ctrl set of pressed btn
	int i;
	for (i = 0; i < iMaxControlSets; ++i)
		if (btn == ppKeyControlSetBtns[i])
		{
			iSelectedCtrlSet = i;
			break;
		}
	// update shown keys
	UpdateCtrlSet();
}

void C4StartupOptionsDlg::ControlConfigArea::UpdateCtrlSet()
{
	// selected keyboard set btn gets a highlight
	int i;
	for (i = 0; i < iMaxControlSets; ++i)
		ppKeyControlSetBtns[i]->SetHighlight(i == iSelectedCtrlSet);
	// update keys by config
	if (fGamepad)
	{
		//for (i = 0; i < C4MaxKey; ++i)
			//KeyControlBtns[i]->SetKey(Config.Gamepads[iSelectedCtrlSet].Button[i]);
	}
	else
		for (i = 0; i < C4MaxKey; ++i)
			KeyControlBtns[i]->SetKey(Config.Controls.Keyboard[iSelectedCtrlSet][i]);
	// open gamepad
	if (fGamepad && Config.General.GamepadEnabled)
	{
		if (!pGamepadOpener) pGamepadOpener = new C4GamePadOpener(iSelectedCtrlSet);
		else pGamepadOpener->SetGamePad(iSelectedCtrlSet);
	}
	// show/hide gamepad-gui-control checkbox
	if (fGamepad && pGUICtrl)
		pGUICtrl->SetVisibility(iSelectedCtrlSet == 0);
}

void C4StartupOptionsDlg::ControlConfigArea::OnCtrlKeyBtn(C4GUI::Control *btn)
{
	// determine which key has been pressed
	int32_t idKey;
	for (idKey = 0; idKey < C4MaxKey; ++idKey) if (KeyControlBtns[idKey] == btn) break;
	if (idKey == C4MaxKey) return; // can't happen
	// show key selection dialog for it
	KeySelDialog *pDlg = new KeySelDialog(idKey, iSelectedCtrlSet, fGamepad);
	pDlg->SetDelOnClose(false);
	bool fSuccess = GetScreen()->ShowModalDlg(pDlg, false);
	C4KeyCode key = pDlg->GetKeyCode();
	delete pDlg;
	if (!fSuccess) return;
	// key defined: Set it
	KeyControlBtns[idKey]->SetKey(key);
	// and update config
	if (fGamepad)
	{}//Config.Gamepads[iSelectedCtrlSet].Button[idKey] = key;
	else
		Config.Controls.Keyboard[iSelectedCtrlSet][idKey] = key;
}

void C4StartupOptionsDlg::ControlConfigArea::OnResetKeysBtn(C4GUI::Control *btn)
{
	// default keys and axis reset
	if (fGamepad)
	{
		for (int i = 0; i < C4ConfigMaxGamepads; ++i)
			Config.Gamepads[i].Reset();
	}
	else
		Config.Controls.ResetKeys();
	UpdateCtrlSet();
}

void C4StartupOptionsDlg::ControlConfigArea::OnGUIGamepadCheckChange(C4GUI::Element *pCheckBox)
{
	// same as before?
	bool fChecked = static_cast<C4GUI::CheckBox *>(pCheckBox)->GetChecked();
	if (fChecked == !!Config.Controls.GamepadGuiControl) return;
	// reflect change
	Config.Controls.GamepadGuiControl = fChecked;
	Game.pGUI->UpdateGamepadGUIControlEnabled();
	pOptionsDlg->RecreateDialog(false);
}

// C4StartupOptionsDlg::NetworkPortConfig

C4StartupOptionsDlg::NetworkPortConfig::NetworkPortConfig(const C4Rect &rcBounds, const char *szName, int32_t *pConfigValue, int32_t iDefault)
	: C4GUI::Window(), pConfigValue(pConfigValue)
{
	CStdFont *pUseFont = &(C4Startup::Get()->Graphics.BookFont);
	SetBounds(rcBounds);
	C4GUI::ComponentAligner caMain(GetClientRect(), 0, 2, true);
	bool fEnabled = (*pConfigValue != 0);
	C4GUI::Label *pLbl = new C4GUI::Label(szName, caMain.GetFromTop(pUseFont->GetLineHeight()), ALeft, C4StartupFontClr, pUseFont, false);
	AddElement(pLbl);
	C4GUI::ComponentAligner caBottomLine(caMain.GetAll(), 2, 0, false);
	const char *szText = LoadResStr("IDS_CTL_ACTIVE");
	int iWdt = 100, iHgt = 12;
	C4GUI::CheckBox::GetStandardCheckBoxSize(&iWdt, &iHgt, szText, pUseFont);
	pEnableCheck = new C4GUI::CheckBox(caBottomLine.GetFromLeft(iWdt, iHgt), szText, fEnabled);
	pEnableCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pEnableCheck->SetOnChecked(new C4GUI::CallbackHandler<C4StartupOptionsDlg::NetworkPortConfig>(this, &C4StartupOptionsDlg::NetworkPortConfig::OnEnabledCheckChange));
	AddElement(pEnableCheck);
	pPortEdit = new C4GUI::SpinBox<std::int32_t>(caBottomLine.GetAll(), true);
	pPortEdit->SetMinimum(1);
	pPortEdit->SetMaximum(65535);
	pPortEdit->SetColors(C4StartupEditBGColor, C4StartupFontClr, C4StartupEditBorderColor);
	pPortEdit->SetFont(pUseFont);
	pPortEdit->SetValue(fEnabled ? *pConfigValue : iDefault, false);
	pPortEdit->SetMaxText(10); // 65535 is five characters long - but allow some more for easier editing
	pPortEdit->SetVisibility(fEnabled);
	AddElement(pPortEdit);
}

void C4StartupOptionsDlg::NetworkPortConfig::OnEnabledCheckChange(C4GUI::Element *pCheckBox)
{
	pPortEdit->SetVisibility(pEnableCheck->GetChecked());
}

void C4StartupOptionsDlg::NetworkPortConfig::SavePort()
{
	*pConfigValue = GetPort();
}

int32_t C4StartupOptionsDlg::NetworkPortConfig::GetPort()
{
	// controls to config
	if (!pEnableCheck->GetChecked())
		return 0;
	else
		return pPortEdit->GetValue();
}

bool C4StartupOptionsDlg::NetworkPortConfig::GetControlSize(int *piWdt, int *piHgt)
{
	CStdFont *pUseFont = &(C4Startup::Get()->Graphics.BookFont);
	// get size needed for control
	if (piWdt)
	{
		const char *szText = LoadResStr("IDS_CTL_ACTIVE");
		if (!C4GUI::CheckBox::GetStandardCheckBoxSize(piWdt, piHgt, szText, pUseFont)) return false;
		*piWdt *= 2;
	}
	if (piHgt) *piHgt = C4GUI::Edit::GetCustomEditHeight(pUseFont) + pUseFont->GetLineHeight() + 2 * 4;
	return true;
}

// C4StartupOptionsDlg::NetworkServerAddressConfig

C4StartupOptionsDlg::NetworkServerAddressConfig::NetworkServerAddressConfig(const C4Rect &rcBounds, const char *szName, bool *pbConfigEnableValue, char *szConfigAddressValue, int iTabWidth)
	: C4GUI::Window(), pbConfigEnableValue(pbConfigEnableValue), szConfigAddressValue(szConfigAddressValue)
{
	CStdFont *pUseFont = &(C4Startup::Get()->Graphics.BookFont);
	SetBounds(rcBounds);
	C4GUI::ComponentAligner caMain(GetClientRect(), 0, 2, true);
	pEnableCheck = new C4GUI::CheckBox(caMain.GetFromLeft(iTabWidth, pUseFont->GetLineHeight()), szName, *pbConfigEnableValue);
	pEnableCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pEnableCheck->SetOnChecked(new C4GUI::CallbackHandler<C4StartupOptionsDlg::NetworkServerAddressConfig>(this, &C4StartupOptionsDlg::NetworkServerAddressConfig::OnEnabledCheckChange));
	AddElement(pEnableCheck);
	caMain.ExpandLeft(-2);
	pAddressEdit = new C4GUI::Edit(caMain.GetAll(), true);
	pAddressEdit->SetColors(C4StartupEditBGColor, C4StartupFontClr, C4StartupEditBorderColor);
	pAddressEdit->SetFont(pUseFont);
	pAddressEdit->InsertText(szConfigAddressValue, false);
	pAddressEdit->SetMaxText(CFG_MaxString);
	pAddressEdit->SetVisibility(*pbConfigEnableValue);
	AddElement(pAddressEdit);
}

void C4StartupOptionsDlg::NetworkServerAddressConfig::OnEnabledCheckChange(C4GUI::Element *pCheckBox)
{
	// warn about using alternate servers
	if (pEnableCheck->GetChecked())
	{
		GetScreen()->ShowMessage(LoadResStr("IDS_NET_NOOFFICIALLEAGUE"), LoadResStr("IDS_NET_QUERY_MASTERSRV"), C4GUI::Ico_Notify, &Config.Startup.HideMsgNoOfficialLeague);
	}
	// callback when checkbox is ticked
	pAddressEdit->SetVisibility(pEnableCheck->GetChecked());
}

void C4StartupOptionsDlg::NetworkServerAddressConfig::Save2Config()
{
	// controls to config
	*pbConfigEnableValue = pEnableCheck->GetChecked();
	SCopy(pAddressEdit->GetText(), szConfigAddressValue, CFG_MaxString);
}

bool C4StartupOptionsDlg::NetworkServerAddressConfig::GetControlSize(int *piWdt, int *piHgt, int *piTabPos, const char *szForText)
{
	CStdFont *pUseFont = &(C4Startup::Get()->Graphics.BookFont);
	int iWdt = 120;
	C4GUI::CheckBox::GetStandardCheckBoxSize(&iWdt, piHgt, szForText, pUseFont);
	int32_t iEdtWdt = 120, iEdtHgt = 24;
	pUseFont->GetTextExtent("sorgentelefon@treffpunktclonk.net", iEdtWdt, iEdtHgt, false);
	if (piWdt) *piWdt = iWdt + iEdtWdt + 2;
	if (piTabPos) *piTabPos = iWdt + 2;
	if (piHgt) *piHgt = C4GUI::Edit::GetCustomEditHeight(pUseFont) + 2 * 2;
	return true;
}

// C4StartupOptionsDlg::BoolConfig

C4StartupOptionsDlg::BoolConfig::BoolConfig(const C4Rect &rcBounds, const char *szName, bool *pbVal, bool fInvert, bool *pbRestartChangeCfgVal)
	: C4GUI::CheckBox(rcBounds, szName, fInvert != *pbVal), pbVal(pbVal), fInvert(fInvert), pbRestartChangeCfgVal(pbRestartChangeCfgVal)
{
	SetOnChecked(new C4GUI::CallbackHandler<BoolConfig>(this, &BoolConfig::OnCheckChange));
}

void C4StartupOptionsDlg::BoolConfig::OnCheckChange(C4GUI::Element *pCheckBox)
{
	if (pbVal) *pbVal = (GetChecked() != fInvert);
	if (pbRestartChangeCfgVal) GetScreen()->ShowMessage(LoadResStr("IDS_MSG_RESTARTCHANGECFG"), GetText(),
		C4GUI::Ico_Notify, pbRestartChangeCfgVal);
}

// C4StartupOptionsDlg::EditConfig

C4StartupOptionsDlg::EditConfig::EditConfig(const C4Rect &rcBounds, const char *szName, ValidatedStdStrBufBase *psConfigVal, int32_t *piConfigVal, bool fMultiline)
	: C4GUI::LabeledEdit(rcBounds, szName, fMultiline, psConfigVal ? psConfigVal->getData() : nullptr, &(C4Startup::Get()->Graphics.BookFont), C4StartupFontClr), psConfigVal(psConfigVal), piConfigVal(piConfigVal)
{
	GetEdit()->SetColors(C4StartupEditBGColor, C4StartupFontClr, C4StartupEditBorderColor);
	if (piConfigVal) SetIntVal(*piConfigVal);
	GetEdit()->SetMaxText(CFG_MaxString);
}

void C4StartupOptionsDlg::EditConfig::Save2Config()
{
	// controls to config
	if (psConfigVal)
		psConfigVal->CopyValidated(GetEdit()->GetText());
	if (piConfigVal)
		*piConfigVal = GetIntVal();
}

bool C4StartupOptionsDlg::EditConfig::GetControlSize(int *piWdt, int *piHgt, const char *szForText, bool fMultiline)
{
	CStdFont *pUseFont = &(C4Startup::Get()->Graphics.BookFont);
	typedef C4GUI::LabeledEdit BaseEdit;
	return BaseEdit::GetControlSize(piWdt, piHgt, szForText, pUseFont, fMultiline);
}

struct
{
	const char *caption;
	DisplayMode mode;
} static const DisplayModes[] =
{
	{"IDS_MSG_FULLSCREEN", DisplayMode::Fullscreen},
	{"IDS_MSG_WINDOW", DisplayMode::Window}
};

// C4StartupOptionsDlg

C4StartupOptionsDlg::C4StartupOptionsDlg() : C4StartupDlg(LoadResStrNoAmp("IDS_DLG_OPTIONS")), fConfigSaved(false), fCanGoBack(true)
{
	UpdateSize();
	bool fSmall = (GetClientRect().Wdt < 750);
	CStdFont *pUseFont = &(C4Startup::Get()->Graphics.BookFont);

	// key bindings
	C4CustomKey::CodeList keys;
	keys.push_back(C4KeyCodeEx(K_BACK)); keys.push_back(C4KeyCodeEx(K_LEFT));
	pKeyBack = new C4KeyBinding(keys, "StartupOptionsBack", KEYSCOPE_Gui,
		new C4GUI::DlgKeyCB<C4StartupOptionsDlg>(*this, &C4StartupOptionsDlg::KeyBack), C4CustomKey::PRIO_Dlg);
	keys.clear();
	keys.push_back(C4KeyCodeEx(K_F3)); // overloading global toggle with higher priority here, so a new name is required
	pKeyToggleMusic = new C4KeyBinding(keys, "OptionsMusicToggle", KEYSCOPE_Gui,
		new C4GUI::DlgKeyCB<C4StartupOptionsDlg>(*this, &C4StartupOptionsDlg::KeyMusicToggle), C4CustomKey::PRIO_Dlg);

	// screen calculations
	int32_t iButtonWidth, iCaptionFontHgt;
	int32_t iButtonHeight = C4GUI_ButtonHgt;
	C4GUI::GetRes()->CaptionFont.GetTextExtent("<< BACK", iButtonWidth, iCaptionFontHgt, true);
	iButtonWidth *= 3;
	int iIndentX1, iIndentX2, iIndentY1, iIndentY2;
	if (fSmall)
	{
		iIndentX1 = 20; iIndentX2 = 1;
	}
	else
	{
		iIndentX1 = GetClientRect().Wdt / 40;
		iIndentX2 = iIndentX1 / 2;
	}
	if (fSmall)
	{
		iIndentY1 = 1; iIndentY2 = 1;
	}
	else
	{
		iIndentY1 = GetClientRect().Hgt / 200;
		iIndentY2 = std::max<int32_t>(1, iIndentY1 / 2);
	}
	C4GUI::ComponentAligner caMain(GetClientRect(), 0, 0, true);
	C4GUI::ComponentAligner caButtonArea(caMain.GetFromBottom(caMain.GetHeight() / (fSmall ? 20 : 7)), 0, 0);
	C4GUI::ComponentAligner caButtons(caButtonArea.GetCentered(caMain.GetWidth() * 7 / 8, iButtonHeight), 0, 0);
	C4GUI::ComponentAligner caConfigArea(caMain.GetAll(), fSmall ? 0 : (caMain.GetWidth() * 69 / 1730), fSmall ? 0 : (caMain.GetHeight() / 200));

	// back button
	C4GUI::CallbackButton<C4StartupOptionsDlg> *btn;
	AddElement(btn = new C4GUI::CallbackButton<C4StartupOptionsDlg>(LoadResStr("IDS_BTN_BACK"), caButtons.GetFromLeft(iButtonWidth), &C4StartupOptionsDlg::OnBackBtn));
	btn->SetToolTip(LoadResStr("IDS_DLGTIP_BACKMAIN"));

	// main config area tabular
	pOptionsTabular = new C4GUI::Tabular(caConfigArea.GetAll(), C4GUI::Tabular::tbLeft);
	pOptionsTabular->SetGfx(&C4Startup::Get()->Graphics.fctOptionsDlgPaper, &C4Startup::Get()->Graphics.fctOptionsTabClip, &C4Startup::Get()->Graphics.fctOptionsIcons, &C4Startup::Get()->Graphics.BookSmallFont, true);
	AddElement(pOptionsTabular);
	C4GUI::Tabular::Sheet *pSheetGeneral = pOptionsTabular->AddSheet(LoadResStr("IDS_DLG_PROGRAM"), 0);
	C4GUI::Tabular::Sheet *pSheetGraphics = pOptionsTabular->AddSheet(LoadResStr("IDS_DLG_GRAPHICS"), 1);
	C4GUI::Tabular::Sheet *pSheetSound = pOptionsTabular->AddSheet(LoadResStr("IDS_DLG_SOUND"), 2);
	C4GUI::Tabular::Sheet *pSheetKeyboard = pOptionsTabular->AddSheet(LoadResStr("IDS_DLG_KEYBOARD"), 3);
	C4GUI::Tabular::Sheet *pSheetGamepad = pOptionsTabular->AddSheet(LoadResStr("IDS_DLG_GAMEPAD"), 4);
	C4GUI::Tabular::Sheet *pSheetNetwork = pOptionsTabular->AddSheet(LoadResStr("IDS_DLG_NETWORK"), 5);

	C4GUI::CheckBox *pCheck; C4GUI::Label *pLbl;
	int iCheckWdt = 100, iCheckHgt = 20, iEdit2Wdt = 100, iEdit2Hgt = 40;
	BoolConfig::GetStandardCheckBoxSize(&iCheckWdt, &iCheckHgt, "Default text", pUseFont);
	EditConfig::GetControlSize(&iEdit2Wdt, &iEdit2Hgt, "Default text", false);

	// page program
	C4GUI::ComponentAligner caSheetProgram(pSheetGeneral->GetClientRect(), caMain.GetWidth() / 20, caMain.GetHeight() / 20, true);
	// language
	const char *szLangTip = LoadResStr("IDS_MSG_SELECTLANG");
	C4GUI::ComponentAligner caLanguage(caSheetProgram.GetGridCell(0, 1, 0, 8, -1, -1, true, 1, 2), 0, C4GUI_DefDlgSmallIndent, false);
	C4GUI::ComponentAligner caLanguageBox(caLanguage.GetFromTop(C4GUI::ComboBox::GetDefaultHeight()), 0, 0, false);
	StdStrBuf sLangStr; sLangStr.Copy(LoadResStr("IDS_CTL_LANGUAGE")); sLangStr.AppendChar(':');
	int32_t w, q;
	pUseFont->GetTextExtent(sLangStr.getData(), w, q, true);
	pLbl = new C4GUI::Label(sLangStr.getData(), caLanguageBox.GetFromLeft(w + C4GUI_DefDlgSmallIndent), ALeft, C4StartupFontClr, pUseFont, false);
	pLbl->SetToolTip(szLangTip);
	pSheetGeneral->AddElement(pLbl);
	pUseFont->GetTextExtent("XX: Top Secret Language", w, q, true);
	pLangCombo = new C4GUI::ComboBox(caLanguageBox.GetFromLeft((std::min)(w, caLanguageBox.GetWidth())));
	pLangCombo->SetToolTip(szLangTip);
	pLangCombo->SetComboCB(new C4GUI::ComboBox_FillCallback<C4StartupOptionsDlg>(this, &C4StartupOptionsDlg::OnLangComboFill, &C4StartupOptionsDlg::OnLangComboSelChange));
	pLangCombo->SetColors(C4StartupFontClr, C4StartupEditBGColor, C4StartupEditBorderColor);
	pLangCombo->SetFont(pUseFont);
	pLangCombo->SetDecoration(&(C4Startup::Get()->Graphics.fctContext));
	pSheetGeneral->AddElement(pLangCombo);
	pLangInfoLabel = new C4GUI::Label("", caLanguage.GetFromTop(C4GUI::GetRes()->TextFont.GetLineHeight() * 3), ALeft, C4StartupFontClr, pUseFont, false);
	pLangInfoLabel->SetToolTip(szLangTip);
	pSheetGeneral->AddElement(pLangInfoLabel);
	UpdateLanguage();
	// font
	const char *szFontTip = LoadResStr("IDS_DESC_SELECTFONT");
	C4GUI::ComponentAligner caFontBox(caSheetProgram.GetGridCell(0, 1, 2, 9, -1, C4GUI::ComboBox::GetDefaultHeight(), true), 0, 0, false);
	StdStrBuf sFontStr; sFontStr.Copy(LoadResStr("IDS_CTL_FONT")); sFontStr.AppendChar(':');
	pUseFont->GetTextExtent(sFontStr.getData(), w, q, true);
	pLbl = new C4GUI::Label(sFontStr.getData(), caFontBox.GetFromLeft(w + C4GUI_DefDlgSmallIndent), ALeft, C4StartupFontClr, pUseFont, false);
	pLbl->SetToolTip(szFontTip);
	pSheetGeneral->AddElement(pLbl);
	pUseFont->GetTextExtent("Comic Sans MS", w, q, true);
	pFontFaceCombo = new C4GUI::ComboBox(caFontBox.GetFromLeft(std::min<int32_t>(caFontBox.GetInnerWidth() * 3 / 4, w * 3)));
	pFontFaceCombo->SetToolTip(szFontTip);
	pFontFaceCombo->SetComboCB(new C4GUI::ComboBox_FillCallback<C4StartupOptionsDlg>(this, &C4StartupOptionsDlg::OnFontFaceComboFill, &C4StartupOptionsDlg::OnFontComboSelChange));
	pFontFaceCombo->SetColors(C4StartupFontClr, C4StartupEditBGColor, C4StartupEditBorderColor);
	pFontFaceCombo->SetFont(pUseFont);
	pFontFaceCombo->SetDecoration(&(C4Startup::Get()->Graphics.fctContext));
	caFontBox.ExpandLeft(-C4GUI_DefDlgSmallIndent);
	pSheetGeneral->AddElement(pFontFaceCombo);
	pFontSizeCombo = new C4GUI::ComboBox(caFontBox.GetFromLeft(std::min<int32_t>(caFontBox.GetInnerWidth(), w)));
	pFontSizeCombo->SetToolTip(LoadResStr("IDS_DESC_FONTSIZE"));
	pFontSizeCombo->SetComboCB(new C4GUI::ComboBox_FillCallback<C4StartupOptionsDlg>(this, &C4StartupOptionsDlg::OnFontSizeComboFill, &C4StartupOptionsDlg::OnFontComboSelChange));
	pFontSizeCombo->SetColors(C4StartupFontClr, C4StartupEditBGColor, C4StartupEditBorderColor);
	pFontSizeCombo->SetFont(pUseFont);
	pFontSizeCombo->SetDecoration(&(C4Startup::Get()->Graphics.fctContext));
	pSheetGeneral->AddElement(pFontSizeCombo);
	UpdateFontControls();

	// white chat
	C4GUI::ComponentAligner caWhiteChat(caSheetProgram.GetGridCell(0, 1, 3, 9, -1, C4GUI::ComboBox::GetDefaultHeight(), true), 0, 0, false);

	StdStrBuf sWhiteChat(LoadResStr("IDS_MNU_WHITECHAT"), false);
	sWhiteChat.AppendChar(':');
	pUseFont->GetTextExtent(sWhiteChat.getData(), w, q, true);
	pLbl = new C4GUI::Label(sWhiteChat.getData(), caWhiteChat.GetFromLeft(w + C4GUI_DefDlgSmallIndent + C4GUI::ComboBox::GetDefaultHeight()), ALeft, C4StartupFontClr, pUseFont, false);
	pLbl->SetToolTip(LoadResStr("IDS_DESC_WHITECHAT"));
	pSheetGeneral->AddElement(pLbl);
	// - ingame
	const auto szIngame = LoadResStr("IDS_CTL_WHITECHAT_INGAME");
	pUseFont->GetTextExtent(szIngame, w, q, true);
	pCheck = new BoolConfig(caWhiteChat.GetFromLeft(w + C4GUI_DefDlgSmallIndent + 2 * C4GUI::ComboBox::GetDefaultHeight()), szIngame, &Config.General.UseWhiteIngameChat);
	pCheck->SetToolTip(LoadResStr("IDS_DESC_WHITECHAT_INGAME"));
	pCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pSheetGeneral->AddElement(pCheck);
	// - lobby
	const auto szLobby = LoadResStr("IDS_CTL_WHITECHAT_LOBBY");
	pUseFont->GetTextExtent(szLobby, w, q, true);
	pCheck = new BoolConfig(caWhiteChat.GetFromLeft(w + C4GUI_DefDlgSmallIndent + 2 * C4GUI::ComboBox::GetDefaultHeight()), szLobby, &Config.General.UseWhiteLobbyChat);
	pCheck->SetToolTip(LoadResStr("IDS_DESC_WHITECHAT_LOBBY"));
	pCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pSheetGeneral->AddElement(pCheck);

	// timestamps
	pCheck = new BoolConfig(caSheetProgram.GetGridCell(0, 1, 4, 9, -1, iCheckHgt, true), LoadResStr("IDS_CTL_TIMESTAMPS"), &Config.General.ShowLogTimestamps);
	pCheck->SetToolTip(LoadResStr("IDS_DESC_TIMESTAMPS"));
	pCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pSheetGeneral->AddElement(pCheck);

	// preloading
	pCheck = new BoolConfig{caSheetProgram.GetGridCell(0, 1, 5, 9, -1, iCheckHgt, true), LoadResStr("IDS_CTL_PRELOADING"), &Config.General.Preloading};
	pCheck->SetToolTip(LoadResStr("IDS_MSG_PRELOADING_DESC"));
	pCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pSheetGeneral->AddElement(pCheck);

	// fair crew strength
	C4GUI::GroupBox *pGroupFairCrewStrength = new C4GUI::GroupBox(caSheetProgram.GetGridCell(0, 2, 6, 9, -1, pUseFont->GetLineHeight() * 2 + iIndentY2 * 2 + C4GUI_ScrollBarHgt, true, 1, 2));
	pGroupFairCrewStrength->SetTitle(LoadResStr("IDS_CTL_FAIRCREWSTRENGTH"));
	pGroupFairCrewStrength->SetFont(pUseFont);
	pGroupFairCrewStrength->SetColors(C4StartupEditBorderColor, C4StartupFontClr);
	pSheetGeneral->AddElement(pGroupFairCrewStrength);
	C4GUI::ComponentAligner caGroupFairCrewStrength(pGroupFairCrewStrength->GetClientRect(), 1, 0, true);
	StdStrBuf sLabelTxt; sLabelTxt.Copy(LoadResStr("IDS_CTL_FAIRCREWWEAK"));
	w = 20; q = 12; pUseFont->GetTextExtent(sLabelTxt.getData(), w, q, true);
	pGroupFairCrewStrength->AddElement(new C4GUI::Label(sLabelTxt.getData(), caGroupFairCrewStrength.GetFromLeft(w, q), ACenter, C4StartupFontClr, pUseFont, false, false));
	sLabelTxt.Copy(LoadResStr("IDS_CTL_FAIRCREWSTRONG"));
	pUseFont->GetTextExtent(sLabelTxt.getData(), w, q, true);
	pGroupFairCrewStrength->AddElement(new C4GUI::Label(sLabelTxt.getData(), caGroupFairCrewStrength.GetFromRight(w, q), ACenter, C4StartupFontClr, pUseFont, false, false));
	C4GUI::ParCallbackHandler<C4StartupOptionsDlg, int32_t> *pCB = new C4GUI::ParCallbackHandler<C4StartupOptionsDlg, int32_t>(this, &C4StartupOptionsDlg::OnFairCrewStrengthSliderChange);
	C4GUI::ScrollBar *pSlider = new C4GUI::ScrollBar(caGroupFairCrewStrength.GetCentered(caGroupFairCrewStrength.GetInnerWidth(), C4GUI_ScrollBarHgt), true, pCB, 101);
	pSlider->SetDecoration(&C4Startup::Get()->Graphics.sfctBookScroll, false);
	pGroupFairCrewStrength->SetToolTip(LoadResStr("IDS_DESC_FAIRCREWSTRENGTH"));
	pSlider->SetScrollPos(FairCrewStrength2Slider(Config.General.FairCrewStrength));
	pGroupFairCrewStrength->AddElement(pSlider);
	// reset configuration
	const char *szBtnText = LoadResStr("IDS_BTN_RESETCONFIG");
	C4GUI::CallbackButton<C4StartupOptionsDlg, SmallButton> *pSmallBtn;
	C4GUI::GetRes()->CaptionFont.GetTextExtent(szBtnText, iButtonWidth, iButtonHeight, true);
	C4Rect rcResetBtn = caSheetProgram.GetGridCell(1, 2, 8, 9, std::min<int32_t>(iButtonWidth + iButtonHeight * 4, caSheetProgram.GetInnerWidth() * 2 / 5), SmallButton::GetDefaultButtonHeight(), true);
	pSheetGeneral->AddElement(pSmallBtn = new C4GUI::CallbackButton<C4StartupOptionsDlg, SmallButton>(szBtnText, rcResetBtn, &C4StartupOptionsDlg::OnResetConfigBtn, this));
	pSmallBtn->SetToolTip(LoadResStr("IDS_DESC_RESETCONFIG"));

	szBtnText = LoadResStr("IDS_DLG_ADVANCED_SETTINGS");
	C4GUI::GetRes()->CaptionFont.GetTextExtent(szBtnText, iButtonWidth, iButtonHeight, true);
	C4Rect rcAdvancedBtn = caSheetProgram.GetGridCell(0, 2, 8, 9, std::min<int32_t>(iButtonWidth + iButtonHeight * 4, caSheetProgram.GetInnerWidth() * 2 / 5), SmallButton::GetDefaultButtonHeight(), true);
	pSheetGeneral->AddElement(pSmallBtn = new C4GUI::CallbackButton<C4StartupOptionsDlg, SmallButton>(szBtnText, rcAdvancedBtn, &C4StartupOptionsDlg::OnAdvancedConfigBtn, this));
	pSmallBtn->SetToolTip(LoadResStr("IDS_DESC_ADVANCED_SETTINGS"));

	// page graphics
	C4GUI::ComponentAligner caSheetGraphics(pSheetGraphics->GetClientRect(), iIndentX1, iIndentY1, true);

	C4GUI::GroupBox *pGroupDisplaySettings = new C4GUI::GroupBox(caSheetGraphics.GetGridCell(0, 1, 0, 3));
	pGroupDisplaySettings->SetTitle(LoadResStrNoAmp("IDS_CTL_DISPLAY"));
	pGroupDisplaySettings->SetFont(pUseFont);
	pGroupDisplaySettings->SetColors(C4StartupEditBorderColor, C4StartupFontClr);
	pSheetGraphics->AddElement(pGroupDisplaySettings);

	C4GUI::ComponentAligner caDisplaySettings(pGroupDisplaySettings->GetClientRect(), iIndentX1, iIndentY2, true);

	C4GUI::ComponentAligner caDisplayModeRow(caDisplaySettings.GetGridCell(0, 1, 0, 2, -1, C4GUI::ComboBox::GetDefaultHeight(), true), 0, 0, false);

	StdStrBuf sDisplayModeStr; sDisplayModeStr.Copy(LoadResStr("IDS_CTL_DISPLAYMODE")); sDisplayModeStr.AppendChar(':');
	pUseFont->GetTextExtent(sDisplayModeStr.getData(), w, q, true);
	pLbl = new C4GUI::Label(sDisplayModeStr.getData(), caDisplayModeRow.GetFromLeft(w + C4GUI_DefDlgSmallIndent), ALeft, C4StartupFontClr, pUseFont, false);
	pGroupDisplaySettings->AddElement(pLbl);
	pDisplayModeCombo = new C4GUI::ComboBox(caDisplayModeRow.GetAll());
	pDisplayModeCombo->SetComboCB(new C4GUI::ComboBox_FillCallback<C4StartupOptionsDlg>(this, &C4StartupOptionsDlg::OnDisplayModeComboFill, &C4StartupOptionsDlg::OnDisplayModeComboSelChange));
	pDisplayModeCombo->SetColors(C4StartupFontClr, C4StartupEditBGColor, C4StartupEditBorderColor);
	pDisplayModeCombo->SetFont(pUseFont);
	pDisplayModeCombo->SetDecoration(&(C4Startup::Get()->Graphics.fctContext));
	pGroupDisplaySettings->AddElement(pDisplayModeCombo);

	for (const auto &mode : DisplayModes)
	{
		if (mode.mode == Config.Graphics.UseDisplayMode)
		{
			pDisplayModeCombo->SetText(LoadResStr(mode.caption));
		}
	}

	C4GUI::ComponentAligner caScaleRow(caDisplaySettings.GetGridCell(0, 1, 1, 2, -1, iEdit2Hgt, true), 0, 0, false);

	StdStrBuf sScaleStr; sScaleStr.Copy(LoadResStr("IDS_CTL_GRAPHICSSCALE")); sScaleStr.AppendChar(':');
	pUseFont->GetTextExtent(sScaleStr.getData(), w, q, true);
	pLbl = new C4GUI::Label(sScaleStr.getData(), caScaleRow.GetFromLeft(w + C4GUI_DefDlgSmallIndent), ALeft, C4StartupFontClr, pUseFont, false);
	pGroupDisplaySettings->AddElement(pLbl);

	szBtnText = LoadResStr("IDS_BTN_TESTGRAPHICSSCALE");
	C4GUI::CallbackButton<C4StartupOptionsDlg, SmallButton> *pTestScaleBtn;
	C4GUI::GetRes()->CaptionFont.GetTextExtent(szBtnText, w, q, true);
	pGroupDisplaySettings->AddElement(pTestScaleBtn = new C4GUI::CallbackButton<C4StartupOptionsDlg, SmallButton>(szBtnText, caScaleRow.GetFromRight(w), &C4StartupOptionsDlg::OnTestScaleBtn, this));
	caScaleRow.GetFromRight(4 * C4GUI_DefDlgSmallIndent);

	pUseFont->GetTextExtent(" %", w, q, true);
	pLbl = new C4GUI::Label(" %", caScaleRow.GetFromRight(w), ARight, C4StartupFontClr, pUseFont, false);
	pGroupDisplaySettings->AddElement(pLbl);

	pUseFont->GetTextExtent("30000", w, q, true);
	pScaleEdit = new ScaleEdit(this, caScaleRow.GetFromRight(w + C4GUI_DefDlgSmallIndent));
	pScaleEdit->SetFont(pUseFont);
	pScaleEdit->SetToolTip(LoadResStr("IDS_DESC_GRAPHICSSCALE"));
	pGroupDisplaySettings->AddElement(pScaleEdit);

	caScaleRow.GetFromTop((caScaleRow.GetHeight() - C4GUI_ScrollBarHgt) / 2);
	caScaleRow.GetFromBottom(caScaleRow.GetHeight() - C4GUI_ScrollBarHgt);
	auto *pScaleCB = new C4GUI::ParCallbackHandler<C4StartupOptionsDlg, int32_t>(this, &C4StartupOptionsDlg::OnScaleSliderChanged);
	pScaleSlider = new C4GUI::ScrollBar(caScaleRow.GetAll(), true, pScaleCB, maxScale - minScale + 1);

	pScaleSlider->SetDecoration(&C4Startup::Get()->Graphics.sfctBookScroll, false);
	pScaleSlider->SetToolTip(LoadResStr("IDS_DESC_GRAPHICSSCALE"));
	pScaleSlider->SetScrollPos(Config.Graphics.Scale - minScale);
	pGroupDisplaySettings->AddElement(pScaleSlider);

	OnScaleSliderChanged(Config.Graphics.Scale - minScale);
;
	// --subgroup options
	int32_t iNumGfxOptions = 5, iOpt = 0;
	C4GUI::GroupBox *pGroupOptions = new C4GUI::GroupBox(caSheetGraphics.GetGridCell(0, 2, 1, 3));
	pGroupOptions->SetTitle(LoadResStrNoAmp("IDS_DLG_OPTIONS"));
	pGroupOptions->SetFont(pUseFont);
	pGroupOptions->SetColors(C4StartupEditBorderColor, C4StartupFontClr);
	pSheetGraphics->AddElement(pGroupOptions);
	C4GUI::ComponentAligner caGroupOptions(pGroupOptions->GetClientRect(), iIndentX1, iIndentY2, true);
	// add new crew portraits
	pCheck = new BoolConfig(caGroupOptions.GetGridCell(0, 1, iOpt++, iNumGfxOptions, -1, iCheckHgt, true), LoadResStr("IDS_MSG_ADDPORTRAITS"), &Config.Graphics.AddNewCrewPortraits);
	pCheck->SetToolTip(LoadResStr("IDS_MSG_ADDPORTRAITS_DESC"));
	pCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pGroupOptions->AddElement(pCheck);
	// store default portraits in crew
	pCheck = new BoolConfig(caGroupOptions.GetGridCell(0, 1, iOpt++, iNumGfxOptions, -1, iCheckHgt, true), LoadResStr("IDS_MSG_STOREPORTRAITS"), &Config.Graphics.SaveDefaultPortraits);
	pCheck->SetToolTip(LoadResStr("IDS_DESC_STOREPORTRAITS"));
	pCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pGroupOptions->AddElement(pCheck);
	// automatic gfx frame skip
	pCheck = new BoolConfig(caGroupOptions.GetGridCell(0, 1, iOpt++, iNumGfxOptions, -1, iCheckHgt, true), LoadResStr("IDS_MSG_AUTOFRAMESKIP"), &Config.Graphics.AutoFrameSkip);
	pCheck->SetToolTip(LoadResStr("IDS_DESC_AUTOFRAMESKIP"));
	pCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pGroupOptions->AddElement(pCheck);
	// hide folder maps
	pCheck = new BoolConfig(caGroupOptions.GetGridCell(0, 1, iOpt++, iNumGfxOptions, -1, iCheckHgt, true), LoadResStr("IDS_MSG_SHOWFOLDERMAPS"), &Config.Graphics.ShowFolderMaps);
	pCheck->SetToolTip(LoadResStr("IDS_DESC_SHOWFOLDERMAPS"));
	pCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pGroupOptions->AddElement(pCheck);
	// disable gamma
	pCheck = new BoolConfig(caGroupOptions.GetGridCell(0, 1, iOpt++, iNumGfxOptions, -1, iCheckHgt, true), LoadResStr("IDS_CTL_DISABLEGAMMA"), &Config.Graphics.DisableGamma);
	pCheck->SetToolTip(LoadResStr("IDS_MSG_DISABLEGAMMA_DESC"));
	pCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pGroupOptions->AddElement(pCheck);
	// --subgroup effects
	C4GUI::GroupBox *pGroupEffects = new C4GUI::GroupBox(caSheetGraphics.GetGridCell(1, 2, 1, 3));
	pGroupEffects->SetTitle(LoadResStrNoAmp("IDS_CTL_SMOKE"));
	pGroupEffects->SetFont(pUseFont);
	pGroupEffects->SetColors(C4StartupEditBorderColor, C4StartupFontClr);
	pSheetGraphics->AddElement(pGroupEffects);
	C4GUI::ComponentAligner caGroupEffects(pGroupEffects->GetClientRect(), iIndentX1, iIndentY2, true);
	iNumGfxOptions = 2; iOpt = 0;
	// effects level slider
	C4GUI::ComponentAligner caEffectsLevel(caGroupEffects.GetGridCell(0, 1, iOpt++, iNumGfxOptions), 1, 0, false);
	StdStrBuf sEffectsTxt; sEffectsTxt.Copy(LoadResStr("IDS_CTL_SMOKELOW"));
	w = 20; q = 12; pUseFont->GetTextExtent(sEffectsTxt.getData(), w, q, true);
	pGroupEffects->AddElement(new C4GUI::Label(sEffectsTxt.getData(), caEffectsLevel.GetFromLeft(w, q), ACenter, C4StartupFontClr, pUseFont, false, false));
	sEffectsTxt.Copy(LoadResStr("IDS_CTL_SMOKEHI"));
	w = 20; q = 12; pUseFont->GetTextExtent(sEffectsTxt.getData(), w, q, true);
	pGroupEffects->AddElement(new C4GUI::Label(sEffectsTxt.getData(), caEffectsLevel.GetFromRight(w, q), ACenter, C4StartupFontClr, pUseFont, false, false));
	pEffectLevelSlider = new C4GUI::ScrollBar(caEffectsLevel.GetCentered(caEffectsLevel.GetInnerWidth(), C4GUI_ScrollBarHgt), true, new C4GUI::ParCallbackHandler<C4StartupOptionsDlg, int32_t>(this, &C4StartupOptionsDlg::OnEffectsSliderChange), 301);
	pEffectLevelSlider->SetDecoration(&C4Startup::Get()->Graphics.sfctBookScroll, false);
	pEffectLevelSlider->SetToolTip(LoadResStr("IDS_MSG_PARTICLES_DESC"));
	pEffectLevelSlider->SetScrollPos(Config.Graphics.SmokeLevel);
	pGroupEffects->AddElement(pEffectLevelSlider);
	// fire particles
	pCheck = new BoolConfig(caGroupEffects.GetGridCell(0, 1, iOpt++, iNumGfxOptions, -1, iCheckHgt, true), LoadResStr("IDS_MSG_FIREPARTICLES"), &Config.Graphics.FireParticles);
	pCheck->SetToolTip(LoadResStr("IDS_MSG_FIREPARTICLES_DESC"));
	pCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pGroupEffects->AddElement(pCheck);

	// page sound
	C4GUI::ComponentAligner caSheetSound(pSheetSound->GetClientRect(), iIndentX1, iIndentY1, true);
	if (!C4GUI::CheckBox::GetStandardCheckBoxSize(&iCheckWdt, &iCheckHgt, "Lorem ipsum", pUseFont)) { iCheckWdt = 100; iCheckHgt = 20; }
	int32_t iGridWdt = iCheckWdt * 2, iGridHgt = iCheckHgt * 5 / 2;
	// --subgroup menu system sound
	C4GUI::GroupBox *pGroupFESound = new C4GUI::GroupBox(caSheetSound.GetGridCell(0, 2, 0, 5, iGridWdt, iGridHgt, false, 1, 2));
	pGroupFESound->SetTitle(LoadResStrNoAmp("IDS_CTL_FRONTEND"));
	pGroupFESound->SetFont(pUseFont);
	pGroupFESound->SetColors(C4StartupEditBorderColor, C4StartupFontClr);
	pSheetSound->AddElement(pGroupFESound);
	C4GUI::ComponentAligner caGroupFESound(pGroupFESound->GetClientRect(), iIndentX1, iIndentY2, true);
	// menu system music
	pCheck = pFEMusicCheck = new C4GUI::CheckBox(caGroupFESound.GetGridCell(0, 1, 0, 2, -1, iCheckHgt, true), LoadResStr("IDS_CTL_MUSIC"), !!Config.Sound.FEMusic);
	pCheck->SetToolTip(LoadResStr("IDS_DESC_MENUMUSIC"));
	pCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pCheck->SetOnChecked(new C4GUI::CallbackHandler<C4StartupOptionsDlg>(this, &C4StartupOptionsDlg::OnFEMusicCheck));
	pGroupFESound->AddElement(pCheck);
	// menu system sound effects
	pCheck = new BoolConfig(caGroupFESound.GetGridCell(0, 1, 1, 2, -1, iCheckHgt, true), LoadResStr("IDS_CTL_SOUNDFX"), &Config.Sound.FESamples);
	pCheck->SetToolTip(LoadResStr("IDS_DESC_MENUSOUND"));
	pCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pGroupFESound->AddElement(pCheck);
	// --subgroup game sound
	C4GUI::GroupBox *pGroupRXSound = new C4GUI::GroupBox(caSheetSound.GetGridCell(1, 2, 0, 5, iGridWdt, iGridHgt, false, 1, 2));
	pGroupRXSound->SetTitle(LoadResStrNoAmp("IDS_CTL_GAME"));
	pGroupRXSound->SetFont(pUseFont);
	pGroupRXSound->SetColors(C4StartupEditBorderColor, C4StartupFontClr);
	pSheetSound->AddElement(pGroupRXSound);
	C4GUI::ComponentAligner caGroupRXSound(pGroupRXSound->GetClientRect(), iIndentX1, iIndentY2, true);
	// game music
	pCheck = new BoolConfig(caGroupRXSound.GetGridCell(0, 1, 0, 2, -1, iCheckHgt, true), LoadResStr("IDS_CTL_MUSIC"), &Config.Sound.RXMusic);
	pCheck->SetToolTip(LoadResStr("IDS_DESC_GAMEMUSIC"));
	pCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pGroupRXSound->AddElement(pCheck);
	// game sound effects
	pCheck = new BoolConfig(caGroupRXSound.GetGridCell(0, 1, 1, 2, -1, iCheckHgt, true), LoadResStr("IDS_CTL_SOUNDFX"), &Config.Sound.RXSound);
	pCheck->SetToolTip(LoadResStr("IDS_DESC_GAMESOUND"));
	pCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pGroupRXSound->AddElement(pCheck);
	// -- subgroup volume
	C4GUI::GroupBox *pGroupVolume = new C4GUI::GroupBox(caSheetSound.GetGridCell(0, 2, 2, 5, iGridWdt, iGridHgt, false, 2, 3));
	pGroupVolume->SetTitle(LoadResStrNoAmp("IDS_BTN_VOLUME"));
	pGroupVolume->SetFont(pUseFont);
	pGroupVolume->SetColors(C4StartupEditBorderColor, C4StartupFontClr);
	pSheetSound->AddElement(pGroupVolume);
	C4GUI::ComponentAligner caGroupVolume(pGroupVolume->GetClientRect(), iIndentX1, iIndentY2, true);
	// volume sliders
	int32_t i;
	for (i = 0; i < 2; ++i)
	{
		C4GUI::ComponentAligner caVolumeSlider(caGroupVolume.GetGridCell(0, 1, i, 2, -1, pUseFont->GetLineHeight() + iIndentY2 * 2 + C4GUI_ScrollBarHgt, true), 1, 0, false);
		pGroupVolume->AddElement(new C4GUI::Label(FormatString("%s:", LoadResStr(i ? "IDS_CTL_SOUNDFX" : "IDS_CTL_MUSIC")).getData(), caVolumeSlider.GetFromTop(pUseFont->GetLineHeight()), ALeft, C4StartupFontClr, pUseFont, false, false));
		sLabelTxt.Copy(LoadResStr("IDS_CTL_SILENT"));
		w = 20; q = 12; pUseFont->GetTextExtent(sLabelTxt.getData(), w, q, true);
		pGroupVolume->AddElement(new C4GUI::Label(sLabelTxt.getData(), caVolumeSlider.GetFromLeft(w, q), ACenter, C4StartupFontClr, pUseFont, false, false));
		sLabelTxt.Copy(LoadResStr("IDS_CTL_LOUD"));
		pUseFont->GetTextExtent(sLabelTxt.getData(), w, q, true);
		pGroupVolume->AddElement(new C4GUI::Label(sLabelTxt.getData(), caVolumeSlider.GetFromRight(w, q), ACenter, C4StartupFontClr, pUseFont, false, false));
		C4GUI::ParCallbackHandler<C4StartupOptionsDlg, int32_t> *pCB = new C4GUI::ParCallbackHandler<C4StartupOptionsDlg, int32_t>(this, i ? &C4StartupOptionsDlg::OnSoundVolumeSliderChange : &C4StartupOptionsDlg::OnMusicVolumeSliderChange);
		C4GUI::ScrollBar *pSlider = new C4GUI::ScrollBar(caVolumeSlider.GetCentered(caVolumeSlider.GetInnerWidth(), C4GUI_ScrollBarHgt), true, pCB, 101);
		pSlider->SetDecoration(&C4Startup::Get()->Graphics.sfctBookScroll, false);
		pSlider->SetToolTip(i ? LoadResStr("IDS_DESC_VOLUMESOUND") : LoadResStr("IDS_DESC_VOLUMEMUSIC"));
		pSlider->SetScrollPos(i ? Config.Sound.SoundVolume : Config.Sound.MusicVolume);
		pGroupVolume->AddElement(pSlider);
	}

	// page keyboard controls
	pSheetKeyboard->AddElement(new ControlConfigArea(pSheetKeyboard->GetClientRect(), caMain.GetWidth() / 20, caMain.GetHeight() / 40, false, this));

	// page gamepad
	pSheetGamepad->AddElement(new ControlConfigArea(pSheetGamepad->GetClientRect(), caMain.GetWidth() / 20, caMain.GetHeight() / 40, true, this));

	// page network
	C4GUI::ComponentAligner caSheetNetwork(pSheetNetwork->GetClientRect(), caMain.GetWidth() / 20, caMain.GetHeight() / 20, true);
	int iPortCfgWdt = 200, iPortCfgHgt = 48; NetworkPortConfig::GetControlSize(&iPortCfgWdt, &iPortCfgHgt);
	pPortCfgTCP = new NetworkPortConfig(caSheetNetwork.GetGridCell(0, 2, 0, 2, iPortCfgWdt, iPortCfgHgt), LoadResStr("IDS_NET_PORT_TCP"), &(Config.Network.PortTCP), C4NetStdPortTCP);
	pPortCfgUDP = new NetworkPortConfig(caSheetNetwork.GetGridCell(1, 2, 0, 2, iPortCfgWdt, iPortCfgHgt), LoadResStr("IDS_NET_PORT_UDP"), &(Config.Network.PortUDP), C4NetStdPortUDP);
	pPortCfgRef = new NetworkPortConfig(caSheetNetwork.GetGridCell(0, 2, 1, 2, iPortCfgWdt, iPortCfgHgt), LoadResStr("IDS_NET_PORT_REFERENCE"), &(Config.Network.PortRefServer), C4NetStdPortRefServer);
	pPortCfgDsc = new NetworkPortConfig(caSheetNetwork.GetGridCell(1, 2, 1, 2, iPortCfgWdt, iPortCfgHgt), LoadResStr("IDS_NET_PORT_DISCOVERY"), &(Config.Network.PortDiscovery), C4NetStdPortDiscovery);
	pPortCfgTCP->SetToolTip(LoadResStr("IDS_NET_PORT_TCP_DESC"));
	pPortCfgUDP->SetToolTip(LoadResStr("IDS_NET_PORT_UDP_DESC"));
	pPortCfgRef->SetToolTip(LoadResStr("IDS_NET_PORT_REFERENCE_DESC"));
	pPortCfgDsc->SetToolTip(LoadResStr("IDS_NET_PORT_DISCOVERY_DESC"));
	pSheetNetwork->AddElement(pPortCfgTCP);
	pSheetNetwork->AddElement(pPortCfgUDP);
	pSheetNetwork->AddElement(pPortCfgRef);
	pSheetNetwork->AddElement(pPortCfgDsc);
	int iNetHgt0 = pPortCfgDsc->GetBounds().GetBottom();
	caSheetNetwork.ExpandTop(-iNetHgt0);
	int iServerCfgWdt = 120, iServerCfgHgt = 20, iServerCfgWdtMid = 0;
	StdStrBuf sServerText; sServerText.Copy(LoadResStr("IDS_CTL_USEOTHERSERVER"));
	NetworkServerAddressConfig::GetControlSize(&iServerCfgWdt, &iServerCfgHgt, &iServerCfgWdtMid, sServerText.getData());
	pLeagueServerCfg = new NetworkServerAddressConfig(caSheetNetwork.GetFromTop(iServerCfgHgt), sServerText.getData(), &(Config.Network.UseAlternateServer), Config.Network.AlternateServerAddress, iServerCfgWdtMid);
	pLeagueServerCfg->SetToolTip(LoadResStr("IDS_NET_MASTERSRV_DESC"));
	pSheetNetwork->AddElement(pLeagueServerCfg);
	pCheck = new BoolConfig(caSheetNetwork.GetFromTop(pUseFont->GetLineHeight()), LoadResStr("IDS_CTL_AUTOMATICUPDATES"), &Config.Network.AutomaticUpdate, false);
	pCheck->SetToolTip(LoadResStr("IDS_DESC_AUTOMATICUPDATES"));
	pCheck->SetFont(pUseFont, C4StartupFontClr, C4StartupFontClrDisabled);
	pSheetNetwork->AddElement(pCheck);
	const char *szNameCfgText = LoadResStr("IDS_NET_COMPUTERNAME");
	int iNameCfgWdt = 200, iNameCfgHgt = 48; C4StartupOptionsDlg::EditConfig::GetControlSize(&iNameCfgWdt, &iNameCfgHgt, szNameCfgText, false);
	iNameCfgWdt += 5;
	pNetworkNameEdit = new EditConfig(caSheetNetwork.GetGridCell(0, 2, 0, 1, iNameCfgWdt, iNameCfgHgt), szNameCfgText, &Config.Network.LocalName, nullptr, false);
	pNetworkNickEdit = new EditConfig(caSheetNetwork.GetGridCell(1, 2, 0, 1, iNameCfgWdt, iNameCfgHgt), LoadResStr("IDS_NET_USERNAME"), &Config.Network.Nick, nullptr, false);
	pNetworkNameEdit->SetToolTip(LoadResStr("IDS_NET_COMPUTERNAME_DESC"));
	pNetworkNickEdit->SetToolTip(LoadResStr("IDS_NET_USERNAME_DESC"));
	pSheetNetwork->AddElement(pNetworkNameEdit);
	pSheetNetwork->AddElement(pNetworkNickEdit);
	StdStrBuf NickBuf(Config.Network.Nick);
	if (!NickBuf.getLength()) NickBuf.Copy(Config.Network.LocalName);
	pNetworkNickEdit->GetEdit()->SetText(NickBuf.getData(), false);

	// initial focus is on tab selection
	SetFocus(pOptionsTabular, false);
}

C4StartupOptionsDlg::~C4StartupOptionsDlg()
{
	delete pKeyToggleMusic;
	delete pKeyBack;
}

void C4StartupOptionsDlg::OnClosed(bool fOK)
{
	// callback when dlg got closed - save config
	SaveConfig(true, false);
	C4StartupDlg::OnClosed(fOK);
}

int32_t C4StartupOptionsDlg::FairCrewSlider2Strength(int32_t iSliderVal)
{
	// slider linear in rank to value linear in exp
	return int32_t(pow(double(iSliderVal) / 9.5, 1.5) * 1000.0);
}

int32_t C4StartupOptionsDlg::FairCrewStrength2Slider(int32_t iStrengthVal)
{
	// value linear in exp to slider linear in rank
	return int32_t(pow(double(iStrengthVal) / 1000.0, 1.0 / 1.5) * 9.5);
}

void C4StartupOptionsDlg::OnFairCrewStrengthSliderChange(int32_t iNewVal)
{
	// fair crew strength determined by exponential fn
	Config.General.FairCrewStrength = FairCrewSlider2Strength(iNewVal);
}

void C4StartupOptionsDlg::OnResetConfigBtn(C4GUI::Control *btn)
{
	// confirmation
	StdStrBuf sWarningText; sWarningText.Copy(LoadResStr("IDS_MSG_PROMPTRESETCONFIG"));
	sWarningText.AppendChar('|');
	sWarningText.Append(LoadResStr("IDS_MSG_RESTARTCHANGECFG"));
	if (!GetScreen()->ShowMessageModal(sWarningText.getData(), LoadResStr("IDS_BTN_RESETCONFIG"), C4GUI::MessageDialog::btnYesNo, C4GUI::Ico_Notify))
		// user cancelled
		return;
	// reset cfg
	Config.Default();
	Config.fConfigLoaded = true;
	// engine must be restarted now, because some crucial fields such as resolution and used gfx engine do not match their initialization
	Application.Quit();
}

void C4StartupOptionsDlg::OnAdvancedConfigBtn(C4GUI::Control *)
{
	if (C4StartupOptionsAdvancedConfigDialog::ShowModal(Game.pGUI))
	{
		RecreateDialog(false);
	}
}

void C4StartupOptionsDlg::OnScaleSliderChanged(int32_t val)
{
	iNewScale = val + minScale;
	pScaleEdit->SetValue(iNewScale, true);
}

void C4StartupOptionsDlg::OnTestScaleBtn(C4GUI::Control *)
{
	const auto oldScaleInt = Config.Graphics.Scale;
	const auto oldScale = Application.GetScale();
	if (iNewScale == oldScaleInt) return;

	const auto oldResX = Config.Graphics.ResX;
	const auto oldResY = Config.Graphics.ResY;

	Config.Graphics.Scale = iNewScale;
	const auto realResX = static_cast<int32_t>(floorf(oldResX * oldScale));
	const auto realResY = static_cast<int32_t>(floorf(oldResY * oldScale));

	Application.SetResolution(realResX, realResY);
	Application.SetGameFont(Config.General.RXFontName, Config.General.RXFontSize);

	// do it manually and temporarily here, beacuse RecreateDialog would delete this the second time which leads to a crash after this function returns
	FadeOut(true);

	int32_t iPage = pOptionsTabular->GetActiveSheetIndex();
	std::unique_ptr<C4StartupOptionsDlg> pNewDlg{new C4StartupOptionsDlg};
	pNewDlg->FadeIn(Game.pGUI);
	pNewDlg->pOptionsTabular->SelectSheet(iPage, false);
	pNewDlg->fCanGoBack = false;

	ResChangeConfirmDlg *pConfirmDlg = new ResChangeConfirmDlg;

	if (!GetScreen()->ShowModalDlg(pConfirmDlg, true))
	{
		Config.Graphics.Scale = oldScaleInt;

		if (C4GUI::IsGUIValid())
		{
			Application.SetResolution(realResX, realResY);
			Application.SetGameFont(Config.General.RXFontName, Config.General.RXFontSize);
		}
		else
		{
			// make sure config is restored even if the program is closed during the confirmation dialog
			Config.Graphics.ResX = oldResX;
			Config.Graphics.ResY = oldResY;
		}
	}

	RecreateDialog(false);
}

bool C4StartupOptionsDlg::SaveConfig(bool fForce, bool fKeepOpen)
{
	// prevent double save
	if (fConfigSaved) return true;
	// store some config values
	SaveGfxTroubleshoot();
	// save any config fields that are not stored directly; return whether all values are OK
	// check port validity
	if (!fForce)
	{
		StdStrBuf strError(LoadResStr("IDS_ERR_CONFIG"));
		if (pPortCfgTCP->GetPort() > 0 && pPortCfgTCP->GetPort() == pPortCfgRef->GetPort())
		{
			GetScreen()->ShowMessage(LoadResStr("IDS_NET_ERR_PORT_TCPREF"), strError.getData(), C4GUI::Ico_Error);
			return false;
		}
		if (pPortCfgUDP->GetPort() > 0 && pPortCfgUDP->GetPort() == pPortCfgDsc->GetPort())
		{
			GetScreen()->ShowMessage(LoadResStr("IDS_NET_ERR_PORT_UDPDISC"), strError.getData(), C4GUI::Ico_Error);
			return false;
		}
	}
	pPortCfgTCP->SavePort();
	pPortCfgUDP->SavePort();
	pPortCfgRef->SavePort();
	pPortCfgDsc->SavePort();
	pLeagueServerCfg->Save2Config();
	pNetworkNameEdit->Save2Config();
	pNetworkNickEdit->Save2Config();
	// if nick is same as LocalName, don't save in config
	// so LocalName updates will change the nick as well
	if (SEqual(Config.Network.Nick.getData(), Config.Network.LocalName.getData())) Config.Network.Nick.Clear();
	// make sure config is saved, in case the game crashes later on or another instance is started
	Config.Save();
	if (!fKeepOpen) fConfigSaved = true;
	// done; config OK
	return true;
}

void C4StartupOptionsDlg::DoBack()
{
	if (!SaveConfig(false, false)) return;
	// back 2 main
	C4Startup::Get()->SwitchDialog(fCanGoBack ? (C4Startup::SDID_Back) : (C4Startup::SDID_Main));
}

void C4StartupOptionsDlg::UpdateLanguage()
{
	// find currently specified language in language list and display its info
	const C4LanguageInfo *const info{Languages.FindInfo(Config.General.Language)};
	if (info)
	{
		pLangCombo->SetText(FormatString("%s - %s", info->Code, info->Name).getData());
		pLangInfoLabel->SetText(info->Info);
	}
	else
	{
		pLangCombo->SetText(FormatString("unknown (%s)", Config.General.Language).getData());
		pLangInfoLabel->SetText(LoadResStr("IDS_CTL_NOLANGINFO"));
		return; // no need to mess with fallbacks
	}
	// update language fallbacks
	char *szLang = Config.General.LanguageEx;
	SCopy(info->Code, szLang);
	if (*(info->Fallback))
	{
		SAppend(",", szLang);
		Config.General.GetLanguageSequence(info->Fallback, szLang + SLen(szLang));
	}
	// internal fallbacks
	if (!SSearch(Config.General.LanguageEx, "US"))
	{
		if (*szLang) SAppendChar(',', szLang);
		SAppend("US", szLang);
	}
	if (!SSearch(Config.General.LanguageEx, "DE"))
	{
		if (*szLang) SAppendChar(',', szLang);
		SAppend("DE", szLang);
	}
}

void C4StartupOptionsDlg::OnLangComboFill(C4GUI::ComboBox_FillCB *pFiller)
{
	// fill with all possible languages
	for (const auto &info : Languages)
	{
		pFiller->AddEntry(FormatString("%s - %s", info.Code, info.Name).getData(), static_cast<unsigned char>(info.Code[0]) + (static_cast<unsigned char>(info.Code[1]) << 8));
	}
}

bool C4StartupOptionsDlg::OnLangComboSelChange(C4GUI::ComboBox *pForCombo, int32_t idNewSelection)
{
	// set new language by two-character-code
	Config.General.Language[0] = idNewSelection & 0xff;
	Config.General.Language[1] = (idNewSelection & 0xff00) >> 8;
	Config.General.Language[2] = '\0';
	UpdateLanguage();
	Languages.LoadLanguage(Config.General.LanguageEx);
	// recreate everything to reflect language changes
	RecreateDialog(true);
	return true;
}

void C4StartupOptionsDlg::UpdateFontControls()
{
	// display current language and size in comboboxes
	pFontFaceCombo->SetText(Config.General.RXFontName);
	StdStrBuf sSize; sSize.Format("%d", static_cast<int>(Config.General.RXFontSize));
	pFontSizeCombo->SetText(sSize.getData());
}

const char *DefaultFonts[] = { "Arial Unicode MS", "Comic Sans MS", "Endeavour", "Verdana", nullptr };

void C4StartupOptionsDlg::OnFontFaceComboFill(C4GUI::ComboBox_FillCB *pFiller)
{
	// 2do: enumerate Fonts.txt fonts; then enumerate system fonts
	for (int32_t i = 0; DefaultFonts[i]; ++i) pFiller->AddEntry(DefaultFonts[i], i);
}

void C4StartupOptionsDlg::OnFontSizeComboFill(C4GUI::ComboBox_FillCB *pFiller)
{
	// 2do: enumerate possible font sizes by the font here
	// 2do: Hide font sizes that would be too large for the selected resolution
	pFiller->AddEntry("8", 8);
	pFiller->AddEntry("10", 10);
	pFiller->AddEntry("12", 12);
	pFiller->AddEntry("14", 14);
	pFiller->AddEntry("16", 16);
	pFiller->AddEntry("18", 18);
	pFiller->AddEntry("20", 20);
	pFiller->AddEntry("24", 24);
	pFiller->AddEntry("28", 28);
}

bool C4StartupOptionsDlg::OnFontComboSelChange(C4GUI::ComboBox *pForCombo, int32_t idNewSelection)
{
	// set new value
	const char *szNewFontFace = Config.General.RXFontName;
	int32_t iNewFontSize = Config.General.RXFontSize;
	if (pForCombo == pFontFaceCombo)
		szNewFontFace = DefaultFonts[idNewSelection];
	else if (pForCombo == pFontSizeCombo)
		iNewFontSize = idNewSelection;
	else
		// can't happen
		return true;
	// set new fonts
	if (!Application.SetGameFont(szNewFontFace, iNewFontSize))
	{
		GetScreen()->ShowErrorMessage(LoadResStr("IDS_ERR_INITFONTS"));
		return true;
	}
	// recreate everything to reflect font changes
	RecreateDialog(true);
	return true;
}

void C4StartupOptionsDlg::OnDisplayModeComboFill(C4GUI::ComboBox_FillCB *pFiller)
{
	for (const auto &mode : DisplayModes)
	{
		pFiller->AddEntry(LoadResStr(mode.caption), static_cast<int>(mode.mode));
	}
}

bool C4StartupOptionsDlg::OnDisplayModeComboSelChange(C4GUI::ComboBox *pForCombo, int32_t idNewSelection)
{
	Config.Graphics.UseDisplayMode = static_cast<DisplayMode>(idNewSelection);
	Application.SetDisplayMode(Config.Graphics.UseDisplayMode);
	RecreateDialog(true);
	return true;
}

void C4StartupOptionsDlg::RecreateDialog(bool fFade)
{
	// MUST fade for now, or calling function will fail because dialog is deleted immediately
	fFade = true;
	// this actually breaks the possibility to go back :(
	int32_t iPage = pOptionsTabular->GetActiveSheetIndex();
	C4StartupOptionsDlg *pNewDlg = static_cast<C4StartupOptionsDlg *>(C4Startup::Get()->SwitchDialog(C4Startup::SDID_Options, fFade));
	pNewDlg->pOptionsTabular->SelectSheet(iPage, false);
	pNewDlg->fCanGoBack = false;
}

void C4StartupOptionsDlg::SaveGfxTroubleshoot()
{
	lpDDraw->InvalidateDeviceObjects();
	lpDDraw->RestoreDeviceObjects();
}

void C4StartupOptionsDlg::OnEffectsSliderChange(int32_t iNewVal)
{
	Config.Graphics.SmokeLevel = iNewVal;
}

void C4StartupOptionsDlg::OnFEMusicCheck(C4GUI::Element *pCheckBox)
{
	// option change is reflected immediately
	bool fIsOn = static_cast<C4GUI::CheckBox *>(pCheckBox)->GetChecked();
	if (Config.Sound.FEMusic = fIsOn)
	{
		Application.MusicSystem->PlayFrontendMusic();
	}
	else
	{
		Application.MusicSystem->Stop();
	}
}

void C4StartupOptionsDlg::OnMusicVolumeSliderChange(int32_t iNewVal)
{
	// option change is reflected immediately;
	Config.Sound.MusicVolume = iNewVal;
	Application.MusicSystem->UpdateVolume();
}

void C4StartupOptionsDlg::OnSoundVolumeSliderChange(int32_t iNewVal)
{
	// sound system reads this value directly
	Config.Sound.SoundVolume = iNewVal;
	// test sound
	StartSoundEffect("ArrowHit", false, 100, nullptr);
}

bool C4StartupOptionsDlg::KeyMusicToggle()
{
	// do toggle and reflect in checkbox
	pFEMusicCheck->SetChecked(Application.MusicSystem->ToggleOnOff());
	// key processed
	return true;
}

int32_t C4StartupOptionsDlg::EditConfig::GetIntVal()
{
	return atoi(GetEdit()->GetText());
}

void C4StartupOptionsDlg::EditConfig::SetIntVal(int32_t iToVal)
{
	GetEdit()->SetText(FormatString("%d", static_cast<int>(iToVal)).getData(), false);
}
