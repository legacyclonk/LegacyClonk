/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2005, Sven2
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

// main game dialogs (abort game dlg, observer dlg)

#include <C4Include.h>
#include <C4GameDialogs.h>

#include <C4Viewport.h>
#include <C4Network2Dialogs.h>
#include <C4Game.h>
#include <C4Player.h>

bool C4AbortGameDialog::is_shown = false;

// C4GameAbortDlg

C4AbortGameDialog::C4AbortGameDialog()
	: C4GUI::Dialog((Game.Control.isCtrlHost() || (Game.C4S.Head.Film == 2)) ? 400 : C4GUI::MessageDialog::dsSmall, 100, LoadResStr("IDS_DLG_ABORT"), false), fGameHalted(false)
{
	const auto showRestart = (Game.Control.isCtrlHost() || (Game.C4S.Head.Film == 2));

	is_shown = true; // assume dlg will be shown, soon

	CStdFont &rUseFont = C4GUI::GetRes()->TextFont;
	// get positions
	C4GUI::ComponentAligner caMain(GetClientRect(), C4GUI_DefDlgIndent, C4GUI_DefDlgIndent, true);
	// place icon
	C4Rect rcIcon = caMain.GetFromLeft(C4GUI_IconWdt);
	rcIcon.Hgt = C4GUI_IconHgt;
	AddElement(new C4GUI::Icon(rcIcon, C4GUI::Ico_Exit));

	// centered text dialog: waste some icon space on the right to balance dialog
	caMain.GetFromRight(C4GUI_IconWdt);

	StdStrBuf sMsgBroken;
	int iMsgHeight = rUseFont.BreakMessage(LoadResStr("IDS_HOLD_ABORT"), caMain.GetInnerWidth(), &sMsgBroken, true);
	C4GUI::Label *pLblMessage = new C4GUI::Label("", caMain.GetFromTop(iMsgHeight), ACenter, C4GUI_MessageFontClr, &rUseFont, false);
	pLblMessage->SetText(sMsgBroken.getData(), false);
	AddElement(pLblMessage);

	// place button(s)
	C4GUI::ComponentAligner caButtonArea(caMain.GetFromTop(C4GUI_ButtonAreaHgt), 0, 0);
	int32_t iButtonCount = showRestart ? 3 : 2;
	C4Rect rcBtn = caButtonArea.GetCentered(iButtonCount * C4GUI_DefButton2Wdt + (iButtonCount - 1) * C4GUI_DefButton2HSpace, C4GUI_ButtonHgt);
	rcBtn.Wdt = C4GUI_DefButton2Wdt;

	auto yesBtn = new C4GUI::CallbackButton<C4AbortGameDialog>(LoadResStr("IDS_DLG_YES"), rcBtn, &C4AbortGameDialog::OnYesBtn, this);
	AddElement(yesBtn);
	rcBtn.x += C4GUI_DefButton2Wdt + C4GUI_DefButton2HSpace;

	if (showRestart)
	{
		AddElement(new C4GUI::CallbackButton<C4AbortGameDialog>(LoadResStr("IDS_BTN_RESTART"), rcBtn, &C4AbortGameDialog::OnRestartBtn, this));
		rcBtn.x += C4GUI_DefButton2Wdt + C4GUI_DefButton2HSpace;
	}

	AddElement(new C4GUI::CallbackButton<C4AbortGameDialog>(LoadResStr("IDS_DLG_NO"), rcBtn, &C4AbortGameDialog::OnNoBtn, this));

	SetFocus(yesBtn, false);
	// resize to actually needed size
	SetClientSize(GetClientRect().Wdt, GetClientRect().Hgt - caMain.GetHeight());
}

C4AbortGameDialog::~C4AbortGameDialog()
{
	is_shown = false;
}

void C4AbortGameDialog::OnShown()
{
	if (!Game.Network.isEnabled())
	{
		fGameHalted = true;
		Game.HaltCount++;
	}
}

void C4AbortGameDialog::OnClosed(bool fOK)
{
	if (fGameHalted)
		Game.HaltCount--;
	bool restart = fRestart;
	// inherited
	typedef C4GUI::Dialog Base;
	Base::OnClosed(fOK);

	if (!restart)
	{
		Game.RestartRestoreInfos.Clear();
	}

	// abort
	if (fOK)
	{
		if (restart)
		{
			Application.SetNextMission(Game.ScenarioFilename);
		}

		Game.Abort();
	}
}

void C4AbortGameDialog::OnYesBtn(C4GUI::Control *)
{
	Close(true);
}

void C4AbortGameDialog::OnNoBtn(C4GUI::Control *)
{
	Close(false);
}

void C4AbortGameDialog::OnRestartBtn(C4GUI::Control *)
{
	fRestart = true;
	Close(true);
}
