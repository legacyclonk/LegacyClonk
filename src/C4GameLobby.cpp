/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
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

// the ingame-lobby

#include <C4Include.h>
#include <C4GameLobby.h>
#include "C4GameControl.h"

#include "C4FullScreen.h"
#include "C4GuiEdit.h"
#include "C4GuiResource.h"
#include "C4GuiTabular.h"
#include "C4Network2Dialogs.h"
#include "C4GameOptions.h"
#include "C4GameDialogs.h"
#include "C4Wrappers.h"
#include "C4RTF.h"
#include "C4ChatDlg.h"
#include "C4PlayerInfoListBox.h"

#include <format>

namespace C4GameLobby
{

bool UserAbort = false;

// C4PacketCountdown

void C4PacketCountdown::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(iCountdown, "Countdown", 0));
}

std::string C4PacketCountdown::GetCountdownMsg(bool fInitialMsg) const
{
	if (iCountdown < AlmostStartCountdownTime && !fInitialMsg)
	{
		return std::format("{}...", iCountdown);
	}
	else
	{
		return LoadResStr(C4ResStrTableKey::IDS_PRC_COUNTDOWN, iCountdown);
	}
}

// ScenDescs

ScenDesc::ScenDesc(const C4Rect &rcBounds, bool fActive) : C4GUI::Window(), fDescFinished(false), pSec1Timer(nullptr)
{
	// build components
	SetBounds(rcBounds);
	C4GUI::ComponentAligner caMain(GetClientRect(), 0, 0, true);
	AddElement(pDescBox = new C4GUI::TextWindow(caMain.GetAll(), 0, 0, 0, 100, 4096, "", true));
	pDescBox->SetDecoration(false, false, nullptr, true);
	// initial update to set current data
	if (fActive) Activate();
}

void ScenDesc::Update()
{
	// scenario present?
	C4Network2Res *pRes = Game.Parameters.Scenario.getNetRes();
	if (!pRes) return; // something's wrong
	CStdFont &rTitleFont = C4GUI::GetRes()->CaptionFont;
	CStdFont &rTextFont = C4GUI::GetRes()->TextFont;
	pDescBox->ClearText(false);
	if (pRes->isComplete())
	{
		C4Group ScenarioFile;
		if (!ScenarioFile.Open(pRes->getFile()))
		{
			pDescBox->AddTextLine("scenario file load error", &rTextFont, C4GUI_MessageFontClr, false, true);
		}
		else
		{
			StdStrBuf sDesc;
			// load desc
			C4ComponentHost DefDesc;
			if (DefDesc.LoadEx("Desc", ScenarioFile, C4CFN_ScenarioDesc, Config.General.LanguageEx))
			{
				C4RTFFile rtf;
				rtf.Load(StdBuf::MakeRef(DefDesc.GetData(), SLen(DefDesc.GetData())));
				sDesc.Take(rtf.GetPlainText());
			}
			DefDesc.Close();
			if (!!sDesc)
				pDescBox->AddTextLine(sDesc.getData(), &rTextFont, C4GUI_MessageFontClr, false, true, &rTitleFont);
			else
				pDescBox->AddTextLine(Game.Parameters.ScenarioTitle.getData(), &rTitleFont, C4GUI_CaptionFontClr, false, true);
		}
		// okay, done loading. No more updates.
		fDescFinished = true;
		Deactivate();
	}
	else
	{
		pDescBox->AddTextLine(LoadResStr(C4ResStrTableKey::IDS_MSG_SCENARIODESC_LOADING, static_cast<int>(pRes->getPresentPercent())).c_str(),
			&rTextFont, C4GUI_MessageFontClr, false, true);
	}
	pDescBox->UpdateHeight();
}

void ScenDesc::Activate()
{
	// final desc set? no update then
	if (fDescFinished) return;
	// create timer if necessary
	if (!pSec1Timer) pSec1Timer = new C4Sec1TimerCallback<ScenDesc>(this);
	// force an update
	Update();
}

void ScenDesc::Deactivate()
{
	// release timer if set
	if (pSec1Timer)
	{
		pSec1Timer->Release();
		pSec1Timer = nullptr;
	}
}

// MainDlg

MainDlg::MainDlg(bool fHost)
	: C4GUI::FullscreenDialog(!Game.Parameters.ScenarioTitle ?
	LoadResStr(C4ResStrTableKey::IDS_DLG_LOBBY) :
		std::format("{} - {}", Game.Parameters.ScenarioTitle.getData(), LoadResStr(C4ResStrTableKey::IDS_DLG_LOBBY)).c_str(),
		Game.Parameters.ScenarioTitle.getData()),
	pPlayerList(nullptr), pResList(nullptr), pChatBox(nullptr), pRightTabLbl(nullptr), pRightTab(nullptr),
	pEdt(nullptr), btnRun(nullptr), btnPlayers(nullptr), btnResources(nullptr), btnTeams(nullptr), btnChat(nullptr)
{
	// key bindings
	pKeyHistoryUp = new C4KeyBinding(C4KeyCodeEx(K_UP), "LobbyChatHistoryUp", KEYSCOPE_Gui, new C4GUI::DlgKeyCBEx<MainDlg, bool>(*this, true, &MainDlg::KeyHistoryUpDown), C4CustomKey::PRIO_CtrlOverride);
	pKeyHistoryDown = new C4KeyBinding(C4KeyCodeEx(K_DOWN), "LobbyChatHistoryDown", KEYSCOPE_Gui, new C4GUI::DlgKeyCBEx<MainDlg, bool>(*this, false, &MainDlg::KeyHistoryUpDown), C4CustomKey::PRIO_CtrlOverride);
	// timer
	pSec1Timer = new C4Sec1TimerCallback<MainDlg>(this);
	// indents / sizes
	int32_t iIndentX1, iIndentX2, iIndentX3;
	int32_t iIndentY1, iIndentY2, iIndentY3, iIndentY4;
	int32_t iClientListWdt;
	if (GetClientRect().Wdt > 500)
	{
		// normal dlg
		iIndentX1 = 10;   // lower button area
		iIndentX2 = 20;   // center area (chat)
		iIndentX3 = 5;    // client/player list
		iClientListWdt = GetClientRect().Wdt / 3;
	}
	else
	{
		// small dlg
		iIndentX1 = 2;   // lower button area
		iIndentX2 = 2;   // center area (chat)
		iIndentX3 = 1;   // client/player list
		iClientListWdt = GetClientRect().Wdt / 2;
	}
	if (GetClientRect().Hgt > 320)
	{
		// normal dlg
		iIndentY1 = 16;    // lower button area
		iIndentY2 = 20;    // status bar offset
		iIndentY3 = 8;     // center area (chat)
		iIndentY4 = 8;     // client/player list
	}
	else
	{
		// small dlg
		iIndentY1 = 2;     // lower button area
		iIndentY2 = 2;     // status bar offset
		iIndentY3 = 1;     // center area (chat)
		iIndentY4 = 1;     // client/player list
	}
	// set subtitle ToolTip
	if (pSubTitle)
		pSubTitle->SetToolTip(LoadResStr(C4ResStrTableKey::IDS_DLG_SCENARIOTITLE));
	C4GUI::Label *pLbl;
	// main screen components
	C4GUI::ComponentAligner caMain(GetClientRect(), 0, 0, true);
	caMain.GetFromBottom(iIndentY2);
	// lower button-area
	C4GUI::ComponentAligner caBottom(caMain.GetFromBottom(C4GUI_ButtonHgt + iIndentY1 * 2), iIndentX1, iIndentY1);
	// add buttons
	C4GUI::CallbackButton<MainDlg> *btnExit;
	btnExit = new C4GUI::CallbackButton<MainDlg>(LoadResStr(C4ResStrTableKey::IDS_DLG_EXIT), caBottom.GetFromLeft(100), &MainDlg::OnExitBtn);
	if (fHost)
	{
		btnRun = new C4GUI::CallbackButton<MainDlg>(LoadResStr(C4ResStrTableKey::IDS_DLG_GAMEGO), caBottom.GetFromRight(100), &MainDlg::OnRunBtn);
	}

	checkReady = new C4GUI::CheckBox(caBottom.GetFromRight(110), LoadResStr(C4ResStrTableKey::IDS_DLG_READY), false);
	checkReady->SetOnChecked(new C4GUI::CallbackHandler<MainDlg>(this, &MainDlg::OnReadyCheck));

	if (!fHost)
	{
		caBottom.GetFromLeft(10); // for centering the buttons between
	}
	pGameOptionButtons = new C4GameOptionButtons(caBottom.GetCentered(caBottom.GetInnerWidth(), std::min<int32_t>(C4GUI_IconExHgt, caBottom.GetHeight())), true, fHost, true);

	// players / ressources sidebar
	C4GUI::ComponentAligner caRight(caMain.GetFromRight(iClientListWdt), iIndentX3, iIndentY4);
	pRightTabLbl = new C4GUI::WoodenLabel("", caRight.GetFromTop(C4GUI::WoodenLabel::GetDefaultHeight(&(C4GUI::GetRes()->TextFont))), C4GUI_CaptionFontClr, &C4GUI::GetRes()->TextFont, ALeft);
	caRight.ExpandTop(iIndentY4 * 2 + 1); // undo margin, so client list is located directly under label
	pRightTab = new C4GUI::Tabular(caRight.GetAll(), C4GUI::Tabular::tbNone);
	C4GUI::Tabular::Sheet *pPlayerSheet = pRightTab->AddSheet(LoadResStr(C4ResStrTableKey::IDS_DLG_PLAYERS, -1, -1).c_str());
	C4GUI::Tabular::Sheet *pResSheet = pRightTab->AddSheet(LoadResStr(C4ResStrTableKey::IDS_DLG_RESOURCES));
	C4GUI::Tabular::Sheet *pOptionsSheet = pRightTab->AddSheet(LoadResStr(C4ResStrTableKey::IDS_DLG_OPTIONS));
	C4GUI::Tabular::Sheet *pScenarioSheet = pRightTab->AddSheet(LoadResStr(C4ResStrTableKey::IDS_DLG_SCENARIO));
	pPlayerList = new C4PlayerInfoListBox(pPlayerSheet->GetContainedClientRect(), C4PlayerInfoListBox::PILBM_LobbyClientSort);
	pPlayerSheet->AddElement(pPlayerList);

	const C4Rect resSheetBounds{pResSheet->GetContainedClientRect()};
	C4Rect resDlgBounds{resSheetBounds};

	if (!Config.General.Preloading)
	{
		resDlgBounds.Hgt -= C4GUI_ButtonHgt;
	}

	pResList = new C4Network2ResDlg(resDlgBounds, false);
	pResSheet->AddElement(pResList);

	if (!Config.General.Preloading)
	{
		const C4Rect btnPreloadBounds{resSheetBounds.x, resSheetBounds.Hgt - C4GUI_ButtonHgt, resSheetBounds.Wdt, C4GUI_ButtonHgt};
		btnPreload = new C4GUI::CallbackButton<MainDlg>{LoadResStr(C4ResStrTableKey::IDS_DLG_PRELOAD), btnPreloadBounds, &MainDlg::OnBtnPreload, this};
		btnPreload->SetToolTip(LoadResStr(C4ResStrTableKey::IDS_DLGTIP_PRELOAD));
		pResSheet->AddElement(btnPreload);
	}

	pOptionsList = new C4GameOptionsList(pResSheet->GetContainedClientRect(), false, false);
	pOptionsSheet->AddElement(pOptionsList);
	pScenarioInfo = new ScenDesc(pResSheet->GetContainedClientRect(), false);
	pScenarioSheet->AddElement(pScenarioInfo);
	pRightTabLbl->SetContextHandler(new C4GUI::CBContextHandler<C4GameLobby::MainDlg>(this, &MainDlg::OnRightTabContext));
	pRightTabLbl->SetClickFocusControl(pPlayerList);

	bool fHasTeams = Game.Teams.IsMultiTeams();
	bool fHasChat = C4ChatDlg::IsChatActive();
	int32_t iBtnNum = 4 + fHasTeams + fHasChat;
	if (fHasTeams)
		btnTeams = new C4GUI::CallbackButton<MainDlg, C4GUI::IconButton>(C4GUI::Ico_Team, pRightTabLbl->GetToprightCornerRect(16, 16, 4, 4, --iBtnNum), pPlayerSheet->GetHotkey(), &MainDlg::OnTabTeams);
	btnPlayers   = new C4GUI::CallbackButton<MainDlg, C4GUI::IconButton>(C4GUI::Ico_Player,   pRightTabLbl->GetToprightCornerRect(16, 16, 4, 4, --iBtnNum), pPlayerSheet->GetHotkey(),  &MainDlg::OnTabPlayers);
	btnResources = new C4GUI::CallbackButton<MainDlg, C4GUI::IconButton>(C4GUI::Ico_Resource, pRightTabLbl->GetToprightCornerRect(16, 16, 4, 4, --iBtnNum), pResSheet->GetHotkey(),     &MainDlg::OnTabRes);
	btnOptions   = new C4GUI::CallbackButton<MainDlg, C4GUI::IconButton>(C4GUI::Ico_Options,  pRightTabLbl->GetToprightCornerRect(16, 16, 4, 4, --iBtnNum), pOptionsSheet->GetHotkey(), &MainDlg::OnTabOptions);
	btnScenario  = new C4GUI::CallbackButton<MainDlg, C4GUI::IconButton>(C4GUI::Ico_Gfx,      pRightTabLbl->GetToprightCornerRect(16, 16, 4, 4, --iBtnNum), pOptionsSheet->GetHotkey(), &MainDlg::OnTabScenario);
	if (fHasChat)
		btnChat = new C4GUI::CallbackButton<MainDlg, C4GUI::IconButton>(C4GUI::Ico_Ex_Chat, pRightTabLbl->GetToprightCornerRect(16, 16, 4, 4, --iBtnNum), 0 /* 2do*/, &MainDlg::OnBtnChat);

	// update labels and tooltips for player list
	UpdateRightTab();

	// chat area
	C4GUI::ComponentAligner caCenter(caMain.GetAll(), iIndentX2, iIndentY3);
	// chat input box
	C4GUI::ComponentAligner caChat(caCenter.GetFromBottom(C4GUI::Edit::GetDefaultEditHeight()), 0, 0);
	pLbl = new C4GUI::WoodenLabel(LoadResStr(C4ResStrTableKey::IDS_CTL_CHAT), caChat.GetFromLeft(40), C4GUI_CaptionFontClr, &C4GUI::GetRes()->TextFont);
	pEdt = new C4GUI::CallbackEdit<MainDlg>(caChat.GetAll(), this, &MainDlg::OnChatInput);
	pEdt->SetToolTip(LoadResStr(C4ResStrTableKey::IDS_DLGTIP_CHAT)); pLbl->SetToolTip(LoadResStr(C4ResStrTableKey::IDS_DLGTIP_CHAT));
	pLbl->SetClickFocusControl(pEdt);
	// log box
	pChatBox = new C4GUI::TextWindow(caCenter.GetAll(), 0, 0, 0, 100, 4096, "", false, nullptr, 0, true);
	// add components in tab-order
	AddElement(pChatBox);
	AddElement(pLbl); AddElement(pEdt); // chat

	AddElement(pRightTabLbl);
	if (btnTeams) AddElement(btnTeams);
	AddElement(btnPlayers);
	AddElement(btnResources);
	AddElement(btnOptions);
	AddElement(btnScenario);
	if (btnChat) AddElement(btnChat);

	AddElement(pRightTab);
	AddElement(btnExit); btnExit->SetToolTip(LoadResStr(C4ResStrTableKey::IDS_DLGTIP_EXIT));
	AddElement(pGameOptionButtons);
	if (fHost)
	{
		AddElement(btnRun);
		btnRun->SetToolTip(LoadResStr(C4ResStrTableKey::IDS_DLGTIP_GAMEGO));
	}

	AddElement(checkReady);

	// set initial button state
	UpdatePreloadingGUIState(false);

	// set initial focus
	SetFocus(pEdt, false);

	// stuff
	eCountdownState = CDS_None;
	iBackBufferIndex = -1;

	// initial player list update
	UpdatePlayerList();
}

MainDlg::~MainDlg()
{
	pSec1Timer->Release();
	delete pKeyHistoryUp;
	delete pKeyHistoryDown;
}

void MainDlg::OnExitBtn(C4GUI::Control *btn)
{
	// abort dlg
	Close(false);
}

void MainDlg::OnReadyCheck(C4GUI::Element *pCheckBox)
{
	auto *const checkBox = static_cast<C4GUI::CheckBox *>(pCheckBox);
	const bool isOn{checkBox->GetChecked()};

	if (!readyButtonCooldown.TryReset())
	{
		checkBox->SetChecked(!isOn);
		return;
	}

	::Game.Network.Clients.BroadcastMsgToClients(MkC4NetIOPacket(PID_ReadyCheck, C4PacketReadyCheck{Game.Clients.getLocalID(), isOn ? C4PacketReadyCheck::Ready : C4PacketReadyCheck::NotReady}), true);
	C4Client *const local{Game.Clients.getLocal()};
	local->SetLobbyReady(isOn);
	OnClientReadyStateChange(local);
}

void MainDlg::SetCountdownState(CountdownState eToState, int32_t iTimer)
{
	// no change?
	if (eToState == eCountdownState) return;
	// changing away from countdown?
	if (eCountdownState == CDS_Countdown)
	{
		StopSoundEffect("Elevator", C4SoundSystem::GlobalSound);
		if (eToState != CDS_Start) StartSoundEffect("Pshshsh");
	}
	// change to game start?
	if (eToState == CDS_Start)
	{
		// announce it!
		StartSoundEffect("Blast3");
	}
	else if (eToState == CDS_Countdown)
	{
		StartSoundEffect("Fuse");
		StartSoundEffect("Elevator", true);
	}
	if (eToState == CDS_Countdown || eToState == CDS_LongCountdown)
	{
		// game start notify
		Application.NotifyUserIfInactive();
		if (!eCountdownState)
		{
			// host update start button to be abort button
			if (btnRun) btnRun->SetText(LoadResStr(C4ResStrTableKey::IDS_DLG_CANCEL));
		}
	}
	// countdown abort?
	if (!eToState)
	{
		// host update start button to be start button again
		if (btnRun) btnRun->SetText(LoadResStr(C4ResStrTableKey::IDS_DLG_GAMEGO));
		// countdown abort message
		OnLog(LoadResStr(C4ResStrTableKey::IDS_PRC_STARTABORTED), C4GUI_LogFontClr2);
	}
	// set new state
	eCountdownState = eToState;
	// update stuff (makes team sel and fair crew btn available)
	pGameOptionButtons->SetCountdown(IsCountdown());
	UpdatePlayerList();
}

void MainDlg::OnCountdownPacket(const C4PacketCountdown &Pkt)
{
	// determine new countdown state
	int32_t iTimer = 0;
	CountdownState eNewState;
	if (Pkt.IsAbort())
		eNewState = CDS_None;
	else
	{
		iTimer = Pkt.GetCountdown();
		if (!iTimer)
			eNewState = CDS_Start; // game is about to be started (boom)
		else if (iTimer <= AlmostStartCountdownTime)
			eNewState = CDS_Countdown; // eToState
		else
			eNewState = CDS_LongCountdown;
	}
	bool fWasCountdown = !!eCountdownState;
	SetCountdownState(eNewState, iTimer);
	// display countdown (except last, which ends the lobby anyway)
	if (iTimer)
	{
		// first countdown message
		OnLog(Pkt.GetCountdownMsg(!fWasCountdown).c_str(), C4GUI_LogFontClr2);
		StartSoundEffect("Command");
	}
}

bool MainDlg::IsCountdown()
{
	// flag as countdown if countdown running or game is about to start
	// so team choice, etc. will not become available in the last split-second
	return eCountdownState >= CDS_Countdown;
}

bool MainDlg::IsLongCountdown() const
{
	return eCountdownState >= CDS_LongCountdown;
}

void MainDlg::OnClosed(bool fOK)
{
	// lobby aborted by user: remember not to display error log
	if (!fOK)
		C4GameLobby::UserAbort = true;
	// finish countdown if running
	// (may not be finished if status change packet from host is faster than the countdown-initiate)
	if (eCountdownState) SetCountdownState(fOK ? CDS_Start : CDS_None, 0);
}

void MainDlg::OnRunBtn(C4GUI::Control *btn)
{
	// only for host
	if (!Game.Network.isHost()) return;
	// already started? then abort
	if (eCountdownState) { Game.Network.AbortLobbyCountdown(); return; }
	// otherwise start, utilizing correct countdown time
	Start(Config.Lobby.CountdownTime);
}

void MainDlg::Start(int32_t iCountdownTime)
{
	// check league rules
	if (!Game.Parameters.CheckLeagueRulesStart(true))
		return;
	// network savegame resumes: Warn if not all players have been associated
	if (Game.C4S.Head.SaveGame)
		if (Game.PlayerInfos.FindUnassociatedRestoreInfo(Game.RestorePlayerInfos))
		{
			StdStrBuf sMsg; sMsg.Ref(LoadResStr(C4ResStrTableKey::IDS_MSG_NOTALLSAVEGAMEPLAYERSHAVE));
			if (!GetScreen()->ShowMessageModal(sMsg.getData(), LoadResStr(C4ResStrTableKey::IDS_MSG_FREESAVEGAMEPLRS), C4GUI::MessageDialog::btnYesNo, C4GUI::Ico_SavegamePlayer, &Config.Startup.HideMsgPlrNoTakeOver))
				return;
		}
	// validate countdown time
	iCountdownTime = ValidatedCountdownTime(iCountdownTime);
	// either direct start...
	if (!iCountdownTime)
		Game.Network.Start();
	else
		// ...or countdown
		Game.Network.StartLobbyCountdown(iCountdownTime);
}

C4GUI::InputResult MainDlg::OnChatInput(C4GUI::Edit *pEdt, bool fPasting, bool fPastingMore)
{
	// get edit text
	const char *szInputText = pEdt->GetText();
	// no input?
	if (!szInputText || !*szInputText)
	{
		// do some error sound then
		C4GUI::GUISound("Error");
	}
	else
	{
		// store input in backbuffer before processing commands
		// because those might kill the edit field
		Game.MessageInput.StoreBackBuffer(szInputText);
		bool fProcessed = false;
		// CAUTION when implementing special commands (like /quit) here:
		// those must not be executed when text is pasted, because that could crash the GUI system
		// when there are additional lines to paste, but the edit field is destructed by the command
		if (*szInputText == '/')
		{
			// must be 1 char longer than the longest command only. If given commands are longer, they will be truncated, and such a command won't exist anyway
			const int32_t MaxCommandLen = 20;
			char Command[MaxCommandLen + 1];
			const char *szPar = szInputText;
			// parse command until first space
			int32_t iParPos = SCharPos(' ', szInputText);
			if (iParPos < 0)
			{
				// command w/o par
				SCopy(szInputText, Command, MaxCommandLen);
				szPar += SLen(szPar);
			}
			else
			{
				// command with following par
				SCopy(szInputText, Command, (std::min)(MaxCommandLen, iParPos));
				szPar += iParPos + 1;
			}
			fProcessed = true;
			// evaluate lobby-only commands
			if (SEqualNoCase(Command, "/joinplr"))
			{
				// compose path from given filename
				const std::string plrPath{std::format("{}{}", Config.General.PlayerPath, szPar)};
				// player join - check filename
				if (!ItemExists(plrPath.c_str()))
				{
					LobbyError(LoadResStr(C4ResStrTableKey::IDS_MSG_CMD_JOINPLR_NOFILE, plrPath.c_str()).c_str());
				}
				else
					Game.Network.Players.JoinLocalPlayer(plrPath.c_str(), true);
			}
			else if (SEqualNoCase(Command, "/plrclr"))
			{
				// get player name from input text
				int iSepPos = SCharPos(' ', szPar, 0);
				C4PlayerInfo *pNfo = nullptr;
				int32_t idLocalClient = -1;
				if (Game.Network.Clients.GetLocal()) idLocalClient = Game.Network.Clients.GetLocal()->getID();
				if (iSepPos > 0)
				{
					// a player name is given: Parse it
					StdStrBuf sPlrName;
					sPlrName.Copy(szPar, iSepPos);
					szPar += iSepPos + 1; int32_t id = 0;
					while (pNfo = Game.PlayerInfos.GetNextPlayerInfoByID(id))
					{
						id = pNfo->GetID();
						if (WildcardMatch(sPlrName.getData(), pNfo->GetName())) break;
					}
				}
				else
					// no player name: Set local player
					pNfo = Game.PlayerInfos.GetPrimaryInfoByClientID(idLocalClient);
				C4ClientPlayerInfos *pCltNfo = nullptr;
				if (pNfo) pCltNfo = Game.PlayerInfos.GetClientInfoByPlayerID(pNfo->GetID());
				if (!pCltNfo)
				{
					LobbyError(LoadResStr(C4ResStrTableKey::IDS_MSG_CMD_PLRCLR_NOPLAYER));
				}
				else
				{
					// may color of this client be set?
					if (pCltNfo->GetClientID() != idLocalClient && !Game.Network.isHost())
					{
						LobbyError(LoadResStr(C4ResStrTableKey::IDS_MSG_CMD_PLRCLR_NOACCESS));
					}
					else
					{
						// get color to set
						uint32_t dwNewClr;
						if (sscanf(szPar, "%x", &dwNewClr) != 1)
						{
							LobbyError(LoadResStr(C4ResStrTableKey::IDS_MSG_CMD_PLRCLR_USAGE));
						}
						else
						{
							// color validation
							dwNewClr &= 0xffffff;
							if (!dwNewClr) ++dwNewClr;
							// request a color change to this color
							C4ClientPlayerInfos LocalInfoRequest = *pCltNfo;
							C4PlayerInfo *pPlrInfo = LocalInfoRequest.GetPlayerInfoByID(pNfo->GetID());
							assert(pPlrInfo);
							if (pPlrInfo)
							{
								pPlrInfo->SetOriginalColor(dwNewClr); // set this as a new color wish
								Game.Network.Players.RequestPlayerInfoUpdate(LocalInfoRequest);
							}
						}
					}
				}
			}
			else if (SEqualNoCase(Command, "/start"))
			{
				// timeout given?
				int32_t iTimeout = Config.Lobby.CountdownTime;
				if (!Game.Network.isHost())
					LobbyError(LoadResStr(C4ResStrTableKey::IDS_MSG_CMD_HOSTONLY));
				else if (szPar && *szPar && (!sscanf(szPar, "%d", &iTimeout) || iTimeout < 0))
					LobbyError(LoadResStr(C4ResStrTableKey::IDS_MSG_CMD_START_USAGE));
				else
				{
					// abort previous countdown
					if (eCountdownState) Game.Network.AbortLobbyCountdown();
					// start new countdown (aborts previous if necessary)
					Start(iTimeout);
				}
			}
			else if (SEqualNoCase(Command, "/abort"))
			{
				if (!Game.Network.isHost())
					LobbyError(LoadResStr(C4ResStrTableKey::IDS_MSG_CMD_HOSTONLY));
				else if (eCountdownState)
					Game.Network.AbortLobbyCountdown();
				else
					LobbyError(LoadResStr(C4ResStrTableKey::IDS_MSG_CMD_ABORT_NOCOUNTDOWN));
			}
			else if (SEqualNoCase(Command, "/readycheck"))
			{
				if (!Game.Network.isHost())
				{
					LobbyError(LoadResStr(C4ResStrTableKey::IDS_MSG_CMD_HOSTONLY));
				}
				else if (Config.Cooldowns.ReadyCheck.TryReset())
				{
					RequestReadyCheck();
				}
				else
				{
					LobbyError(LoadResStr(C4ResStrTableKey::IDS_MSG_CMD_COOLDOWN, std::to_string(Config.Cooldowns.ReadyCheck.GetRemainingTime().count()).c_str()).c_str());
				}
			}
			else if (SEqualNoCase(Command, "/help"))
			{
				Log(C4ResStrTableKey::IDS_TEXT_COMMANDSAVAILABLEDURINGLO);
				LogNTr("/start [time] - {}", LoadResStr(C4ResStrTableKey::IDS_TEXT_STARTTHEROUNDWITHSPECIFIE));
				LogNTr("/abort - {}", LoadResStr(C4ResStrTableKey::IDS_TEXT_ABORTSTARTCOUNTDOWN));
				LogNTr("/alert - {}", LoadResStr(C4ResStrTableKey::IDS_TEXT_ALERTTHEHOSTIFTHEHOSTISAW));
				LogNTr("/joinplr [filename] - {}", LoadResStr(C4ResStrTableKey::IDS_TEXT_JOINALOCALPLAYERFROMTHESP));
				LogNTr("/kick [client] - {}", LoadResStr(C4ResStrTableKey::IDS_TEXT_KICKTHESPECIFIEDCLIENT));
				LogNTr("/observer [client] - {}", LoadResStr(C4ResStrTableKey::IDS_TEXT_SETTHESPECIFIEDCLIENTTOOB));
				LogNTr("/me [action] - {}", LoadResStr(C4ResStrTableKey::IDS_TEXT_PERFORMANACTIONINYOURNAME));
				LogNTr("/sound [sound] - {}", LoadResStr(C4ResStrTableKey::IDS_TEXT_PLAYASOUNDFROMTHEGLOBALSO));
				LogNTr("/mute [client] - {}", LoadResStr(C4ResStrTableKey::IDS_NET_MUTE_DESC));
				LogNTr("/unmute [client] - {}", LoadResStr(C4ResStrTableKey::IDS_NET_UNMUTE_DESC));
				LogNTr("/team [message] - {}", LoadResStr(C4ResStrTableKey::IDS_MSG_SENDAPRIVATEMESSAGETOYOUR));
				LogNTr("/plrclr [player] [RGB] - {}", LoadResStr(C4ResStrTableKey::IDS_TEXT_CHANGETHECOLOROFTHESPECIF));
				LogNTr("/plrclr [RGB] - {}", LoadResStr(C4ResStrTableKey::IDS_TEXT_CHANGEYOUROWNPLAYERCOLOR));
				LogNTr("/set comment [comment] - {}", LoadResStr(C4ResStrTableKey::IDS_TEXT_SETANEWNETWORKCOMMENT));
				LogNTr("/set password [password] - {}", LoadResStr(C4ResStrTableKey::IDS_TEXT_SETANEWNETWORKPASSWORD));
				LogNTr("/set faircrew [on/off] - {}", LoadResStr(C4ResStrTableKey::IDS_TEXT_ENABLEORDISABLEFAIRCREW));
				LogNTr("/set maxplayer [number] - {}", LoadResStr(C4ResStrTableKey::IDS_TEXT_SETANEWMAXIMUMNUMBEROFPLA));
				LogNTr("/clear - {}", LoadResStr(C4ResStrTableKey::IDS_MSG_CLEARTHEMESSAGEBOARD));
				LogNTr("/readycheck - {}", LoadResStr(C4ResStrTableKey::IDS_MSG_READYCHECK));
			}
			else
			{
				// command not known or not a specific lobby command - forward to messageinput
				fProcessed = false;
			}
		}
		// not processed? Then forward to messageinput, which parses additional commands and sends regular messages
		if (!fProcessed) Game.MessageInput.ProcessInput(szInputText);
	}
	// clear edit field after text has been processed
	pEdt->SelectAll(); pEdt->DeleteSelection();
	// reset backbuffer-index of chat history
	iBackBufferIndex = -1;
	// OK, on we go. Leave edit intact
	return C4GUI::IR_None;
}

void MainDlg::OnClientJoin(C4Client *pNewClient)
{
	// update list
	UpdatePlayerList();
}

void MainDlg::OnClientConnect(C4Client *pClient, C4Network2IOConnection *pConn) {}

void MainDlg::OnClientPart(C4Client *pPartClient)
{
	// update list
	UpdatePlayerList();
}

void MainDlg::HandlePacket(char cStatus, const C4PacketBase *pPacket, C4Network2Client *pClient)
{
	// note that player info update packets are not handled by this function,
	// but by player info list and then forwarded to MainDlg::OnPlayerUpdate
	// this is necessary because there might be changes (e.g. duplicate colors)
	// done by player info list
	// besides, this releases the lobby from doing any host/client-specializations
#define GETPKT(type, name) \
	assert(pPacket); \
	const type &name = static_cast<const type &>(*pPacket);
	switch (cStatus)
	{
	case PID_LobbyCountdown: // initiate or abort countdown
	{
		GETPKT(C4PacketCountdown, Pkt);
		// do countdown
		OnCountdownPacket(Pkt);
	}
	break;
	};
#undef GETPKT
}

bool MainDlg::OnMessage(C4Client *pOfClient, const char *szMessage)
{
	// output message should be prefixed with client already
	if (pChatBox && C4GUI::GetRes())
	{
		StdStrBuf text;

		if (Config.General.ShowLogTimestamps)
		{
			text.Append(GetCurrentTimeStamp());
			text.AppendChar(' ');
		}
		text.Append(szMessage);

		pChatBox->AddTextLine(text.getData(), &C4GUI::GetRes()->TextFont, Game.Network.Players.GetClientChatColor(pOfClient ? pOfClient->getID() : Game.Clients.getLocalID(), true) | C4GUI_MessageFontAlpha, true, true);
		pChatBox->ScrollToBottom();
	}
	// log it
	spdlog::info(szMessage);
	// done, success
	return true;
}

void MainDlg::OnClientSound(C4Client *pOfClient)
{
	// show that someone played a sound
	if (pOfClient && pPlayerList)
	{
		pPlayerList->SetClientSoundIcon(pOfClient->getID());
	}
}

void MainDlg::OnLog(const char *szLogMsg, uint32_t dwClr)
{
	if (pChatBox && C4GUI::GetRes())
	{
		StdStrBuf text;

		if (Config.General.ShowLogTimestamps)
		{
			text.Append(GetCurrentTimeStamp());
			text.AppendChar(' ');
		}
		text.Append(szLogMsg);

		pChatBox->AddTextLine(text.getData(), &C4GUI::GetRes()->TextFont, dwClr, true, true);
		pChatBox->ScrollToBottom();
	}
}

void MainDlg::OnError(const char *szErrMsg)
{
	if (pChatBox && C4GUI::GetRes())
	{
		StartSoundEffect("Error");
		pChatBox->AddTextLine(szErrMsg, &C4GUI::GetRes()->TextFont, C4GUI_ErrorFontClr, true, true);
		pChatBox->ScrollToBottom();
	}
}

void MainDlg::OnSec1Timer()
{
	UpdatePlayerList();
	UpdateResourceProgress();
}

void MainDlg::UpdatePlayerList()
{
	// this updates ping label texts and teams
	if (pPlayerList) pPlayerList->Update();
	UpdatePlayerCountDisplay();
}

void MainDlg::UpdateResourceProgress()
{
	bool isComplete{true};
	std::int32_t resID{-1};
	for (C4Network2Res::Ref res; (res = Game.Network.ResList.getRefNextRes(resID)); ++resID)
	{
		resID = res->getResID();
		if (res->getType() != NRT_Player)
		{
			isComplete &= res->isComplete();
		}
	}

	if (resourcesLoaded != isComplete)
	{
		resourcesLoaded = isComplete;
		UpdatePreloadingGUIState(isComplete);
	}
}

void MainDlg::UpdatePreloadingGUIState(const bool isComplete)
{
	assert(checkReady);
	checkReady->SetEnabled(isComplete);

	if (btnPreload)
	{
		bool active{isComplete && Game.CanPreload()};
		btnPreload->SetEnabled(active);
		btnPreload->SetVisibility(active);
	}

	if (isComplete)
	{
		checkReady->SetToolTip(LoadResStr(C4ResStrTableKey::IDS_DLGTIP_READY));
		checkReady->SetCaption(LoadResStr(C4ResStrTableKey::IDS_DLG_READY));

		if (Config.General.Preloading)
		{
			Preload();
		}
	}
	else
	{
		checkReady->SetToolTip(LoadResStr(C4ResStrTableKey::IDS_DLGTIP_READYNOTAVAILABLE));
		checkReady->SetCaption(LoadResStr(C4ResStrTableKey::IDS_DLG_STILLLOADING));
	}
}

bool MainDlg::Preload()
{
	if (!Game.Preload())
	{
		const char *const message{LoadResStr(C4ResStrTableKey::IDS_ERR_PRELOADING)};

		// Don't use Log here since we want a red message
		OnLog(message, C4GUI_ErrorFontClr);
		spdlog::info(message);

		return false;
	}

	return true;
}

C4GUI::ContextMenu *MainDlg::OnRightTabContext(C4GUI::Element *pLabel, int32_t iX, int32_t iY)
{
	// create context menu
	C4GUI::ContextMenu *pMenu = new C4GUI::ContextMenu();
	// players/ressources
	C4GUI::Tabular::Sheet *pPlayerSheet = pRightTab->GetSheet(0);
	C4GUI::Tabular::Sheet *pResSheet = pRightTab->GetSheet(1);
	C4GUI::Tabular::Sheet *pOptionsSheet = pRightTab->GetSheet(2);
	pMenu->AddItem(pPlayerSheet->GetTitle(), pPlayerSheet->GetToolTip(), C4GUI::Ico_Player,
		new C4GUI::CBMenuHandler<MainDlg>(this, &MainDlg::OnCtxTabPlayers));
	if (Game.Teams.IsMultiTeams())
	{
		StdStrBuf strShowTeamsDesc(LoadResStr(C4ResStrTableKey::IDS_MSG_SHOWTEAMS_DESC));
		pMenu->AddItem(LoadResStr(C4ResStrTableKey::IDS_MSG_SHOWTEAMS), strShowTeamsDesc.getData(), C4GUI::Ico_Team,
			new C4GUI::CBMenuHandler<MainDlg>(this, &MainDlg::OnCtxTabTeams));
	}
	pMenu->AddItem(pResSheet->GetTitle(), pResSheet->GetToolTip(), C4GUI::Ico_Resource,
		new C4GUI::CBMenuHandler<MainDlg>(this, &MainDlg::OnCtxTabRes));
	pMenu->AddItem(pOptionsSheet->GetTitle(), pOptionsSheet->GetToolTip(), C4GUI::Ico_Options,
		new C4GUI::CBMenuHandler<MainDlg>(this, &MainDlg::OnCtxTabOptions));
	// open it
	return pMenu;
}

void MainDlg::OnClientReadyStateChange(C4Client *client)
{
	UpdatePlayerList();

	for (C4Client *clnt = nullptr; (clnt = Game.Clients.getClient(clnt)); )
	{
		// does the client have players?
		if (C4ClientPlayerInfos *const infos{Game.PlayerInfos.GetInfoByClientID(clnt->getID())}; clnt->isHost() || (infos && infos->GetPlayerCount()))
		{
			if (!clnt->isLobbyReady())
			{
				// the client was ready and now isn't? stop the countdown
				if (client->getID() == clnt->getID() && Game.Network.isHost() && IsLongCountdown())
				{
					Game.Network.AbortLobbyCountdown();
				}

				return;
			}
		}
	}

	if (Game.Network.isHost() && !IsLongCountdown())
	{
		Start(Config.Lobby.CountdownTime);
	}
}

void MainDlg::OnClientAddPlayer(const char *szFilename, int32_t idClient)
{
	// check client number
	if (idClient != Game.Clients.getLocalID())
	{
		LobbyError(LoadResStr(C4ResStrTableKey::IDS_ERR_JOINPLR_NOLOCALCLIENT, szFilename, idClient).c_str());
		return;
	}
	// player join - check filename
	if (!ItemExists(szFilename))
	{
		LobbyError(LoadResStr(C4ResStrTableKey::IDS_ERR_JOINPLR_NOFILE, szFilename).c_str());
		return;
	}
	// check countdown state
	if (IsCountdown())
	{
		if (Game.Network.isHost())
			Game.Network.AbortLobbyCountdown();
		else
			return;
	}
	// join!
	Game.Network.Players.JoinLocalPlayer(Config.AtExeRelativePath(szFilename), true);
}

void MainDlg::OnTabPlayers(C4GUI::Control *btn)
{
	if (pPlayerList) pPlayerList->SetMode(C4PlayerInfoListBox::PILBM_LobbyClientSort);
	if (pRightTab)
	{
		pRightTab->SelectSheet(SheetIdx_PlayerList, true);
		UpdateRightTab();
	}
}

void MainDlg::OnTabTeams(C4GUI::Control *btn)
{
	if (pPlayerList) pPlayerList->SetMode(C4PlayerInfoListBox::PILBM_LobbyTeamSort);
	if (pRightTab)
	{
		pRightTab->SelectSheet(SheetIdx_PlayerList, true);
		UpdateRightTab();
	}
}

void MainDlg::OnTabRes(C4GUI::Control *btn)
{
	if (pRightTab)
	{
		pRightTab->SelectSheet(SheetIdx_Res, true);
		UpdateRightTab();
	}
}

void MainDlg::OnTabOptions(C4GUI::Control *btn)
{
	if (pRightTab)
	{
		pRightTab->SelectSheet(SheetIdx_Options, true);
		UpdateRightTab();
	}
}

void MainDlg::OnTabScenario(C4GUI::Control *btn)
{
	if (pRightTab)
	{
		pRightTab->SelectSheet(SheetIdx_Scenario, true);
		UpdateRightTab();
	}
}

void MainDlg::UpdateRightTab()
{
	if (!pRightTabLbl || !pRightTab || !pRightTab->GetActiveSheet() || !pPlayerList) return;

	// update
	if (pRightTab->GetActiveSheetIndex() == SheetIdx_PlayerList) UpdatePlayerList();
	if (pRightTab->GetActiveSheetIndex() == SheetIdx_Res) pResList->Activate(); else pResList->Deactivate();
	if (pRightTab->GetActiveSheetIndex() == SheetIdx_Options) pOptionsList->Activate(); else pOptionsList->Deactivate();
	if (pRightTab->GetActiveSheetIndex() == SheetIdx_Scenario) pScenarioInfo->Activate(); else pScenarioInfo->Deactivate();
	// update selection buttons
	if (btnPlayers) btnPlayers->SetHighlight(pRightTab->GetActiveSheetIndex() == SheetIdx_PlayerList && pPlayerList->GetMode() == C4PlayerInfoListBox::PILBM_LobbyClientSort);
	if (btnResources) btnResources->SetHighlight(pRightTab->GetActiveSheetIndex() == SheetIdx_Res);
	if (btnTeams) btnTeams->SetHighlight(pRightTab->GetActiveSheetIndex() == SheetIdx_PlayerList && pPlayerList->GetMode() == C4PlayerInfoListBox::PILBM_LobbyTeamSort);
	if (btnOptions) btnOptions->SetHighlight(pRightTab->GetActiveSheetIndex() == SheetIdx_Options);
	if (btnScenario) btnScenario->SetHighlight(pRightTab->GetActiveSheetIndex() == SheetIdx_Scenario);

	UpdateRightTabTitle();
}

void MainDlg::UpdateRightTabTitle()
{
	// copy active sheet data to label
	pRightTabLbl->SetText(pRightTab->GetActiveSheet()->GetTitle());
	pRightTabLbl->SetToolTip(pRightTab->GetActiveSheet()->GetToolTip());
}

void MainDlg::OnBtnChat(C4GUI::Control *btn)
{
	// open chat dialog
	C4ChatDlg::ShowChat();
}

void MainDlg::OnBtnPreload(C4GUI::Control *)
{
	if (Preload())
	{
		// disable it for the max. one second delay until C4Network2ResDlg's callback resets it again
		const int32_t buttonHeight{btnPreload->GetHeight()};
		RemoveElement(btnPreload);
		delete btnPreload;
		btnPreload = nullptr;

		if (pResList)
		{
			pResList->GetBounds().Hgt += buttonHeight;
		}
	}
}

bool MainDlg::KeyHistoryUpDown(bool fUp)
{
	// chat input only
	if (!IsFocused(pEdt)) return false;
	pEdt->SelectAll(); pEdt->DeleteSelection();
	const char *szPrevInput = Game.MessageInput.GetBackBuffer(fUp ? (++iBackBufferIndex) : (--iBackBufferIndex));
	if (!szPrevInput || !*szPrevInput)
		iBackBufferIndex = -1;
	else
	{
		pEdt->InsertText(szPrevInput, true);
		pEdt->SelectAll();
	}
	return true;
}

void MainDlg::UpdateFairCrew()
{
	// if the fair crew setting has changed, make sure the buttons reflect this change
	pGameOptionButtons->UpdateFairCrewBtn();
}

void MainDlg::UpdatePassword()
{
	// if the password setting has changed, make sure the buttons reflect this change
	pGameOptionButtons->UpdatePasswordBtn();
}

void MainDlg::UpdatePlayerCountDisplay()
{
	if (pRightTab)
	{
		pRightTab->GetSheet(0)->SetTitle(LoadResStr(C4ResStrTableKey::IDS_DLG_PLAYERS, Game.PlayerInfos.GetActivePlayerCount(true), Game.Parameters.MaxPlayers).c_str());
		if (pRightTab->GetActiveSheetIndex() == SheetIdx_PlayerList)
		{
			UpdateRightTabTitle();
		}
	}
}

int32_t MainDlg::ValidatedCountdownTime(int32_t iTimeout)
{
	// no negative timeouts
	if (iTimeout < 0) iTimeout = 5;
	// in leage mode, there must be at least five seconds timeout
	if (Game.Parameters.isLeague() && iTimeout < 5) iTimeout = 5;
	return iTimeout;
}

void MainDlg::ClearLog()
{
	pChatBox->ClearText(true);
}

void MainDlg::RequestReadyCheck()
{
	if (IsLongCountdown())
	{
		Game.Network.AbortLobbyCountdown();
	}

	for (C4Client *client{nullptr}; (client = Game.Clients.getClient(client)); )
	{
		if (!client->isHost())
		{
			client->SetLobbyReady(false);
		}
	}

	UpdatePlayerList();
	Game.Network.Clients.BroadcastMsgToClients(MkC4NetIOPacket(PID_ReadyCheck, C4PacketReadyCheck{Game.Clients.getLocalID(), C4PacketReadyCheck::Request}));
}

void MainDlg::CheckReady(bool check)
{
	checkReady->SetChecked(check);
	UpdatePlayerList();
}

C4GUI::Control *MainDlg::GetDefaultControl()
{
	return pEdt;
}

void LobbyError(const char *szErrorMsg)
{
	// get lobby
	MainDlg *pLobby = Game.Network.GetLobby();
	if (pLobby) pLobby->OnError(szErrorMsg);
}

/* Countdown */

Countdown::Countdown(int32_t iStartTimer) : iStartTimer(iStartTimer), pSec1Timer(nullptr)
{
	// only on network hosts
	assert(Game.Network.isHost());
	// Init; sends initial countdown packet
	C4PacketCountdown pck(iStartTimer);
	Game.Network.Clients.BroadcastMsgToClients(MkC4NetIOPacket(PID_LobbyCountdown, pck));
	// also process on host
	MainDlg *pLobby = Game.Network.GetLobby();
	if (pLobby)
	{
		pLobby->OnCountdownPacket(pck);
	}
	else
	{
		// no lobby: Message to log for dedicated/console hosts
		LogNTr(pck.GetCountdownMsg());
	}

	// init timer callback
	pSec1Timer = new C4Sec1TimerCallback<Countdown>(this);
}

Countdown::~Countdown()
{
	// release timer
	if (pSec1Timer) pSec1Timer->Release();
}

void Countdown::OnSec1Timer()
{
	// count down
	iStartTimer = std::max<int32_t>(iStartTimer - 1, 0);
	// only send "important" start timer numbers to all clients
	if (iStartTimer <= AlmostStartCountdownTime || // last seconds
		(iStartTimer <= 600 && !(iStartTimer % 10)) || // last minute: 10s interval
		!(iStartTimer % 60)) // otherwise, minute interval
	{
		C4PacketCountdown pck(iStartTimer);
		Game.Network.Clients.BroadcastMsgToClients(MkC4NetIOPacket(PID_LobbyCountdown, pck));
		// also process on host
		MainDlg *pLobby = Game.Network.GetLobby();
		if (pLobby)
			pLobby->OnCountdownPacket(pck);
		else if (iStartTimer)
		{
			// no lobby: Message to log for dedicated/console hosts
			LogNTr(pck.GetCountdownMsg());
		}
	}
	// countdown done
	if (!iStartTimer)
	{
		// Dedicated server: if there are not enough players for this game, abort and quit the application
		if (!Game.Network.GetLobby() && (Game.PlayerInfos.GetPlayerCount() < Game.C4S.GetMinPlayer()))
		{
			Log(C4ResStrTableKey::IDS_MSG_NOTENOUGHPLAYERSFORTHISRO); // it would also be nice to send this message to all clients...
			Application.Quit();
		}
		// Start the game
		else
			Game.Network.Start();
	}
}

void Countdown::Abort()
{
	// host sends packets
	if (!Game.Network.isHost()) return;
	C4PacketCountdown pck(C4PacketCountdown::Abort);
	Game.Network.Clients.BroadcastMsgToClients(MkC4NetIOPacket(PID_LobbyCountdown, pck));
	// also process on host
	MainDlg *pLobby = Game.Network.GetLobby();
	if (pLobby)
	{
		pLobby->OnCountdownPacket(pck);
	}
	else
	{
		// no lobby: Message to log for dedicated/console hosts
		Log(C4ResStrTableKey::IDS_PRC_STARTABORTED);
	}
}

} // end of namespace
