/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2008, Sven2
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

// game over dialog showing winners and losers

#include <C4Include.h>
#include <C4GameOverDlg.h>

#include <C4Game.h>
#include <C4FullScreen.h>
#include <C4Player.h>
#include <C4PlayerInfo.h>
#include <C4PlayerInfoListBox.h>

// C4GoalDisplay

C4GoalDisplay::GoalPicture::GoalPicture(const C4Rect &rcBounds, C4ID idGoal, bool fFulfilled)
	: idGoal(idGoal), fFulfilled(fFulfilled), C4GUI::Window()
{
	// bounds
	SetBounds(rcBounds);
	// can't get specialized desc from object at the moment because of potential script callbacks!
	StdStrBuf strGoalName, strGoalDesc;
	// just get desc from def
	C4Def *pGoalDef = Game.Defs.ID2Def(idGoal);
	if (pGoalDef)
	{
		strGoalName.Copy(pGoalDef->GetName());
		strGoalDesc.Copy(pGoalDef->GetDesc());
	}
	// get tooltip
	StdStrBuf sToolTip;
	if (fFulfilled)
		sToolTip.Format(LoadResStr("IDS_DESC_GOALFULFILLED"), strGoalName.getData(), strGoalDesc.getData());
	else
		sToolTip.Format(LoadResStr("IDS_DESC_GOALNOTFULFILLED"), strGoalName.getData(), strGoalDesc.getData());
	SetToolTip(sToolTip.getData());
	// create buffered picture of goal definition
	C4Def *pDrawDef = Game.Defs.ID2Def(idGoal);
	if (pDrawDef)
	{
		Picture.Create(C4PictureSize, C4PictureSize);
		// get an object instance to draw (optional; may be zero)
		C4Object *pGoalObj = Game.Objects.FindInternal(idGoal);
		// draw goal def!
		pDrawDef->Draw(Picture, false, 0, pGoalObj);
	}
	// unfulfilled goal: grey out picture
	if (!fFulfilled)
		Picture.Grayscale(30);
}

void C4GoalDisplay::GoalPicture::DrawElement(C4FacetEx &cgo)
{
	// draw area
	C4Facet cgoDraw;
	cgoDraw.Set(cgo.Surface, cgo.X + rcBounds.x + cgo.TargetX, cgo.Y + rcBounds.y + cgo.TargetY, rcBounds.Wdt, rcBounds.Hgt);
	// draw buffered picture
	Picture.Draw(cgoDraw);
	// draw star symbol if fulfilled
	if (fFulfilled)
	{
		cgoDraw.Set(cgoDraw.Surface, cgoDraw.X + cgoDraw.Wdt * 1 / 2, cgoDraw.Y + cgoDraw.Hgt * 1 / 2, cgoDraw.Wdt / 2, cgoDraw.Hgt / 2);
		C4GUI::Icon::GetIconFacet(C4GUI::Ico_Star).Draw(cgoDraw);
	}
}

void C4GoalDisplay::SetGoals(const C4IDList &rAllGoals, const C4IDList &rFulfilledGoals, int32_t iGoalSymbolHeight)
{
	// clear previous
	ClearChildren();
	// determine goal display area by goal count
	int32_t iGoalSymbolMargin = C4GUI_DefDlgSmallIndent;
	int32_t iGoalSymbolAreaHeight = 2 * iGoalSymbolMargin + iGoalSymbolHeight;
	int32_t iGoalAreaWdt = GetClientRect().Wdt;
	int32_t iGoalsPerRow = std::max<int32_t>(1, iGoalAreaWdt / iGoalSymbolAreaHeight);
	int32_t iGoalCount = rAllGoals.GetNumberOfIDs();
	int32_t iRowCount = (iGoalCount - 1) / iGoalsPerRow + 1;
	C4Rect rcNewBounds = GetBounds();
	rcNewBounds.Hgt = GetMarginTop() + GetMarginBottom() + iRowCount * iGoalSymbolAreaHeight;
	SetBounds(rcNewBounds);
	C4GUI::ComponentAligner caAll(GetClientRect(), 0, 0, true);
	// place goal symbols in this area
	int32_t iGoal = 0;
	for (int32_t iRow = 0; iRow < iRowCount; ++iRow)
	{
		int32_t iColCount = std::min<int32_t>(iGoalCount - iGoal, iGoalsPerRow);
		C4GUI::ComponentAligner caGoalArea(caAll.GetFromTop(iGoalSymbolAreaHeight, iColCount * iGoalSymbolAreaHeight), iGoalSymbolMargin, iGoalSymbolMargin, false);
		for (int32_t iCol = 0; iCol < iColCount; ++iCol, ++iGoal)
		{
			C4ID idGoal = rAllGoals.GetID(iGoal);
			bool fFulfilled = !!rFulfilledGoals.GetIDCount(idGoal, 1);
			AddElement(new GoalPicture(caGoalArea.GetGridCell(iCol, iColCount, iRow, iRowCount), idGoal, fFulfilled));
		}
	}
}

// C4GameOverDlg

bool C4GameOverDlg::is_shown = false;

C4GameOverDlg::C4GameOverDlg() : C4GUI::Dialog((C4GUI::GetScreenWdt() < 800) ? (C4GUI::GetScreenWdt() - 10) : std::min<int32_t>(C4GUI::GetScreenWdt() - 150, 800),
	(C4GUI::GetScreenHgt() < 600) ? (C4GUI::GetScreenHgt() - 10) : std::min<int32_t>(C4GUI::GetScreenHgt() - 150, 600),
	LoadResStr("IDS_TEXT_EVALUATION"),
	false), pNetResultLabel(nullptr), fIsNetDone(false)
{
	is_shown = true; // assume dlg will be shown, soon

	pBtnRestart = nullptr;
	pBtnNextMission = nullptr;

	bool hideRestart = false;
	size_t buttonCount = 2;
	if (Game.Control.isCtrlHost() || (Game.C4S.Head.Film == 2))
	{
		++buttonCount;
		if (Game.NextMission)
		{
			if (C4GUI::GetScreenWdt() < 1280)
			{
				hideRestart = true;
			}
			else
			{
				++buttonCount;
			}
		}
		SetBounds(C4Rect(0, 0, (C4GUI::GetScreenWdt() < 1280) ? (C4GUI::GetScreenWdt() - 10) : std::min<int32_t>(C4GUI::GetScreenWdt() - 150, 1280), (C4GUI::GetScreenHgt() < 720) ? (C4GUI::GetScreenHgt() - 10) : std::min<int32_t>(C4GUI::GetScreenHgt() - 150, 720)));
	}

	UpdateOwnPos();

	// indents / sizes
	int32_t iDefBtnHeight = 32;
	int32_t iIndentX1 = 10;
	int32_t iIndentY1 = 6, iIndentY2 = 0;
	// main screen components
	C4GUI::ComponentAligner caMain(GetClientRect(), 0, iIndentY1, true);
	int32_t iMainTextWidth = caMain.GetWidth() - 6 * iIndentX1;
	caMain.GetFromBottom(iIndentY2);
	// lower button-area
	C4GUI::ComponentAligner caBottom(caMain.GetFromBottom(iDefBtnHeight + iIndentY1 * 2), iIndentX1, 0);
	int32_t iBottomButtonSize = caBottom.GetInnerWidth();
	iBottomButtonSize = std::min<int32_t>(iBottomButtonSize / 2 - 2 * iIndentX1, C4GUI::GetRes()->CaptionFont.GetTextWidth("Quit it, baby! And some.") * 13 / 10);
	// goal display
	const C4IDList &rGoals = Game.RoundResults.GetGoals();
	const C4IDList &rFulfilledGoals = Game.RoundResults.GetFulfilledGoals();
	if (rGoals.GetNumberOfIDs())
	{
		C4GoalDisplay *pGoalDisplay = new C4GoalDisplay(caMain.GetFromTop(C4GUI_IconExHgt));
		pGoalDisplay->SetGoals(rGoals, rFulfilledGoals, C4GUI_IconExHgt);
		AddElement(pGoalDisplay);
		// goal display may have resized itself; adjust component aligner
		caMain.ExpandTop(C4GUI_IconExHgt - pGoalDisplay->GetBounds().Hgt);
	}
	// league/network result, present or pending
	fIsNetDone = false;
	bool fHasNetResult = Game.RoundResults.HasNetResult();
	const char *szNetResult = nullptr;
	if (Game.Parameters.isLeague() || fHasNetResult)
	{
		if (fHasNetResult)
			szNetResult = Game.RoundResults.GetNetResultString();
		else
			szNetResult = LoadResStr("IDS_TEXT_LEAGUEWAITINGFOREVALUATIO");
		pNetResultLabel = new C4GUI::Label("", caMain.GetFromTop(C4GUI::GetRes()->TextFont.GetLineHeight() * 2, iMainTextWidth), ACenter, C4GUI_Caption2FontClr, nullptr, false, false, true);
		AddElement(pNetResultLabel);
		// only add label - contents and fIsNetDone will be set in next update
	}
	else
	{
		// otherwise, network is always done
		fIsNetDone = true;
	}
	// extra evaluation string area
	const char *szCustomEvaluationStrings = Game.RoundResults.GetCustomEvaluationStrings();
	if (szCustomEvaluationStrings && *szCustomEvaluationStrings)
	{
		int32_t iMaxHgt = caMain.GetInnerHeight() / 3; // max 1/3rd of height for extra data
		C4GUI::MultilineLabel *pCustomStrings = new C4GUI::MultilineLabel(caMain.GetFromTop(0 /* resized later*/, iMainTextWidth), 0, 0, "    ", true, true);
		pCustomStrings->AddLine(szCustomEvaluationStrings, &C4GUI::GetRes()->TextFont, C4GUI_MessageFontClr, true, false, nullptr);
		C4Rect rcCustomStringBounds = pCustomStrings->GetBounds();
		if (rcCustomStringBounds.Hgt > iMaxHgt)
		{
			// Buffer too large: Use a scrollbox instead
			delete pCustomStrings;
			rcCustomStringBounds.Hgt = iMaxHgt;
			C4GUI::TextWindow *pCustomStringsWin = new C4GUI::TextWindow(rcCustomStringBounds, 0, 0, 0, 0, 0, "    ", true, nullptr, 0, true);
			pCustomStringsWin->SetDecoration(false, false, nullptr, false);
			pCustomStringsWin->AddTextLine(szCustomEvaluationStrings, &C4GUI::GetRes()->TextFont, C4GUI_MessageFontClr, true, false, nullptr);
			caMain.ExpandTop(-iMaxHgt);
			AddElement(pCustomStringsWin);
		}
		else
		{
			// buffer size OK: Reserve required space
			caMain.ExpandTop(-rcCustomStringBounds.Hgt);
			AddElement(pCustomStrings);
		}
	}
	// player list area
	C4GUI::ComponentAligner caPlayerArea(caMain.GetAll(), iIndentX1, 0);
	iPlrListCount = 1; bool fSepTeamLists = false;
	if (Game.Teams.GetTeamCount() == 2 && !Game.Teams.IsAutoGenerateTeams())
	{
		// exactly two predefined teams: Use two player list boxes; one for each team
		iPlrListCount = 2;
		fSepTeamLists = true;
	}
	ppPlayerLists = new C4PlayerInfoListBox *[iPlrListCount];
	for (int32_t i = 0; i < iPlrListCount; ++i)
	{
		ppPlayerLists[i] = new C4PlayerInfoListBox(caPlayerArea.GetGridCell(i, iPlrListCount, 0, 1), C4PlayerInfoListBox::PILBM_Evaluation, fSepTeamLists ? Game.Teams.GetTeamByIndex(i)->GetID() : 0);
		ppPlayerLists[i]->SetSelectionDiabled(true);
		ppPlayerLists[i]->SetDecoration(false, nullptr, true, false);
		AddElement(ppPlayerLists[i]);
	}

	// add buttons
	pBtnExit = new C4GUI::CallbackButton<C4GameOverDlg>(LoadResStr("IDS_BTN_ENDROUND"), caBottom.GetGridCell(0, buttonCount, 0, 1, iBottomButtonSize, -1, true), &C4GameOverDlg::OnExitBtn);
	pBtnExit->SetToolTip(LoadResStr("IDS_DESC_ENDTHEROUND"));
	AddElement(pBtnExit);
	pBtnContinue = new C4GUI::CallbackButton<C4GameOverDlg>(LoadResStr("IDS_BTN_CONTINUEGAME"), caBottom.GetGridCell(1, buttonCount, 0, 1, iBottomButtonSize, -1, true), &C4GameOverDlg::OnContinueBtn);
	pBtnContinue->SetToolTip(LoadResStr("IDS_DESC_CONTINUETHEROUNDWITHNOFUR"));
	AddElement(pBtnContinue);

	// not available for regular replay and network clients, obviously
	// it is available for films though, so you can create cinematics for adventures
	if (Game.Control.isCtrlHost() || (Game.C4S.Head.Film == 2))
	{
		if (!hideRestart)
		{
			pBtnRestart = new C4GUI::CallbackButton<C4GameOverDlg>(LoadResStr("IDS_BTN_RESTART"), caBottom.GetGridCell(2, buttonCount, 0, 1, iBottomButtonSize, -1, true), &C4GameOverDlg::OnRestartBtn);
			pBtnRestart->SetToolTip(LoadResStr("IDS_DESC_RESTART"));
			AddElement(pBtnRestart);
		}

		// convert continue button to "next mission" button if available
		if (Game.NextMission)
		{
			pBtnNextMission = new C4GUI::CallbackButton<C4GameOverDlg>(Game.NextMissionText.getData(), caBottom.GetGridCell(3 - hideRestart, buttonCount, 0, 1, iBottomButtonSize, -1, true), &C4GameOverDlg::OnNextMissionBtn);
			pBtnNextMission->SetToolTip(Game.NextMissionDesc.getData());
			AddElement(pBtnNextMission);
		}
	}
	// updates
	pSec1Timer = new C4Sec1TimerCallback<C4GameOverDlg>(this);
	Update();
	// initial focus on quit button if visible, so space/enter/low gamepad buttons quit
	fIsQuitBtnVisible = fIsNetDone || !Game.Network.isHost();
	if (fIsQuitBtnVisible) SetFocus(pBtnExit, false);
}

C4GameOverDlg::~C4GameOverDlg()
{
	pSec1Timer->Release();
	delete ppPlayerLists;
	is_shown = false;
}

void C4GameOverDlg::Update()
{
	for (int32_t i = 0; i < iPlrListCount; ++i) ppPlayerLists[i]->Update();
	if (pNetResultLabel)
	{
		SetNetResult(Game.RoundResults.GetNetResultString(), Game.RoundResults.GetNetResult(), Game.Network.getPendingStreamData(), Game.Network.isStreaming());
	}
	// exit/continue button only visible for host if league streaming finished
	bool fBtnsVisible = fIsNetDone || !Game.Network.isHost();
	if (fBtnsVisible != fIsQuitBtnVisible)
	{
		fIsQuitBtnVisible = fBtnsVisible;
		pBtnExit->SetVisibility(fBtnsVisible);
		pBtnContinue->SetVisibility(fBtnsVisible);
		if (fIsQuitBtnVisible) SetFocus(pBtnExit, false);
	}
}

void C4GameOverDlg::SetNetResult(const char *szResultString, C4RoundResults::NetResult eResultType, size_t iPendingStreamingData, bool fIsStreaming)
{
	// add info about pending streaming data
	StdStrBuf sResult(szResultString);
	if (fIsStreaming)
	{
		sResult.AppendChar('|');
		sResult.AppendFormat("[!]Transmitting record to league server... (%d kb remaining)", int(iPendingStreamingData / 1024));
	}
	// message linebreak into box
	StdStrBuf sBrokenResult;
	C4GUI::GetRes()->TextFont.BreakMessage(sResult.getData(), pNetResultLabel->GetBounds().Wdt, &sBrokenResult, true);
	pNetResultLabel->SetText(sBrokenResult.getData(), false);
	// all done?
	if (eResultType != C4RoundResults::NR_None && !fIsStreaming)
	{
		// a final result is determined and all streaming data has been transmitted
		fIsNetDone = true;
	}
	// network error?
	if (eResultType == C4RoundResults::NR_NetError)
	{
		// disconnected. Do not show winners/losers
		for (int32_t i = 0; i < iPlrListCount; ++i) ppPlayerLists[i]->SetMode(C4PlayerInfoListBox::PILBM_EvaluationNoWinners);
	}
}

void C4GameOverDlg::OnExitBtn(C4GUI::Control *btn)
{
	// callback: exit button pressed.
	Close(false);
}

void C4GameOverDlg::OnContinueBtn(C4GUI::Control *btn)
{
	// callback: continue button pressed
	Close(true);
}

void C4GameOverDlg::OnRestartBtn(C4GUI::Control *btn)
{
	// callback: restart button pressed
	nextMissionMode = Restart;
	Close(true);
}

void C4GameOverDlg::OnNextMissionBtn(C4GUI::Control *btn)
{
	// callback: next mission button pressed
	nextMissionMode = NextMission;
	Close(true);
}

void C4GameOverDlg::OnShown()
{
	// close some other dialogs
	Game.Scoreboard.HideDlg();
	FullScreen.CloseMenu();
	for (C4Player *plr = Game.Players.First; plr; plr = plr->Next)
		plr->CloseMenu();
	// pause game when round results dlg is shown
	Game.Pause();
}

void C4GameOverDlg::OnClosed(bool fOK)
{
	typedef C4GUI::Dialog BaseClass;
	auto _nextMissionMode = nextMissionMode;
	BaseClass::OnClosed(fOK); // deletes this!
	// continue round
	if (fOK)
	{
		if (_nextMissionMode != None)
		{
			// switch to next mission if next mission button is pressed
			Application.SetNextMission(_nextMissionMode == NextMission ? Game.NextMission.getData() : Game.ScenarioFilename);
			Application.QuitGame();
		}
		else
		{
			// unpause game when continue is pressed
			Game.Unpause();
		}
	}
	// end round
	else
	{
		Application.QuitGame();
	}
}
