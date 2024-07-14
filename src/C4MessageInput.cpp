/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2005, Sven2
 * Copyright (c) 2017-2021, The LegacyClonk Team and contributors
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

// handles input dialogs, last-message-buffer, MessageBoard-commands

#include "C4GuiEdit.h"
#include "C4GuiResource.h"
#include <C4Include.h>
#include <C4MessageInput.h>

#include <C4Game.h>
#include <C4Object.h>
#include <C4Script.h>
#include <C4Gui.h>
#include <C4Console.h>
#include <C4Application.h>
#include <C4Network2Dialogs.h>
#include <C4Log.h>
#include <C4Player.h>
#include <C4GameLobby.h>

// C4ChatInputDialog

// singleton
C4ChatInputDialog *C4ChatInputDialog::pInstance = nullptr;

// helper func: Determine whether input text is good for a chat-style-layout dialog
bool IsSmallInputQuery(const char *szInputQuery)
{
	if (!szInputQuery) return true;
	int32_t w, h;
	if (SCharCount('|', szInputQuery)) return false;
	if (!C4GUI::GetRes()->TextFont.GetTextExtent(szInputQuery, w, h, true))
		return false; // ???
	return w < C4GUI::GetScreenWdt() / 5;
}

C4ChatInputDialog::C4ChatInputDialog(bool fObjInput, C4Object *pScriptTarget, bool fUppercase, Mode mode, int32_t iPlr, const StdStrBuf &rsInputQuery)
	: C4GUI::InputDialog(fObjInput ? rsInputQuery.getData() : LoadResStrNoAmp(C4ResStrTableKey::IDS_CTL_CHAT).c_str(), nullptr, C4GUI::Ico_None, nullptr, !fObjInput || IsSmallInputQuery(rsInputQuery.getData())),
	fObjInput(fObjInput), fUppercase(fUppercase), pTarget(pScriptTarget), BackIndex(-1), iPlr(iPlr), fProcessed(false)
{
	// singleton-var
	pInstance = this;
	// set custom edit control
	SetCustomEdit(new C4GUI::CallbackEdit<C4ChatInputDialog>(C4Rect(0, 0, 10, 10), this, &C4ChatInputDialog::OnChatInput, &C4ChatInputDialog::OnChatCancel));
	// key bindings
	pKeyHistoryUp = new C4KeyBinding(C4KeyCodeEx(K_UP), "ChatHistoryUp", KEYSCOPE_Gui, new C4GUI::DlgKeyCBEx<C4ChatInputDialog, bool>(*this, true, &C4ChatInputDialog::KeyHistoryUpDown), C4CustomKey::PRIO_CtrlOverride);
	pKeyHistoryDown = new C4KeyBinding(C4KeyCodeEx(K_DOWN), "ChatHistoryDown", KEYSCOPE_Gui, new C4GUI::DlgKeyCBEx<C4ChatInputDialog, bool>(*this, false, &C4ChatInputDialog::KeyHistoryUpDown), C4CustomKey::PRIO_CtrlOverride);
	pKeyAbort = new C4KeyBinding(C4KeyCodeEx(K_F2), "ChatAbort", KEYSCOPE_Gui, new C4GUI::DlgKeyCB<C4GUI::Dialog>(*this, &C4GUI::Dialog::KeyEscape), C4CustomKey::PRIO_CtrlOverride);
	pKeyNickComplete = new C4KeyBinding(C4KeyCodeEx(K_TAB), "ChatNickComplete", KEYSCOPE_Gui, new C4GUI::DlgKeyCB<C4ChatInputDialog>(*this, &C4ChatInputDialog::KeyCompleteNick), C4CustomKey::PRIO_CtrlOverride);
	pKeyPlrControl = new C4KeyBinding(C4KeyCodeEx(KEY_Any, KEYS_Control), "ChatForwardPlrCtrl", KEYSCOPE_Gui, new C4GUI::DlgKeyCBPassKey<C4ChatInputDialog>(*this, &C4ChatInputDialog::KeyPlrControl), C4CustomKey::PRIO_Dlg);
	pKeyGamepadControl = new C4KeyBinding(C4KeyCodeEx(KEY_Any), "ChatForwardGamepadCtrl", KEYSCOPE_Gui, new C4GUI::DlgKeyCBPassKey<C4ChatInputDialog>(*this, &C4ChatInputDialog::KeyGamepadControlDown, &C4ChatInputDialog::KeyGamepadControlUp, &C4ChatInputDialog::KeyGamepadControlPressed), C4CustomKey::PRIO_PlrControl);
	pKeyBackClose = new C4KeyBinding(C4KeyCodeEx(K_BACK), "ChatBackspaceClose", KEYSCOPE_Gui, new C4GUI::DlgKeyCB<C4ChatInputDialog>(*this, &C4ChatInputDialog::KeyBackspaceClose), C4CustomKey::PRIO_CtrlOverride);
	// free when closed...
	SetDelOnClose();

	// initial text
	switch (mode)
	{
	case Allies:
		pEdit->InsertText("/team ", true);
		break;

	case Say:
		pEdit->InsertText("\"", true);
		break;

	case All:
		break;
	}
}

C4ChatInputDialog::~C4ChatInputDialog()
{
	delete pKeyHistoryUp;
	delete pKeyHistoryDown;
	delete pKeyAbort;
	delete pKeyNickComplete;
	delete pKeyPlrControl;
	delete pKeyGamepadControl;
	delete pKeyBackClose;
	if (this == pInstance) pInstance = nullptr;
}

void C4ChatInputDialog::OnChatCancel()
{
	// abort chat: Make sure msg board query is aborted
	fProcessed = true;
	if (fObjInput)
	{
		// check if the target input is still valid
		C4Player *pPlr = Game.Players.Get(iPlr);
		if (!pPlr) return;
		if (pPlr->MarkMessageBoardQueryAnswered(pTarget))
		{
			// there was an associated query - it must be removed on all clients synchronized via queue
			// do this by calling OnMessageBoardAnswer without an answer
			Game.Control.DoInput(CID_MessageBoardAnswer, new C4ControlMessageBoardAnswer(pTarget ? pTarget->Number : 0, iPlr, ""), CDT_Decide);
		}
	}
}

void C4ChatInputDialog::OnClosed(bool fOK)
{
	// make sure chat input is processed, even if closed by other means than Enter on edit
	if (!fProcessed)
		if (fOK)
			OnChatInput(pEdit, false, false); else OnChatCancel();
	else
		OnChatCancel();
	Game.Players.ClearLocalPlayerPressedComs();
	typedef C4GUI::InputDialog BaseDlg;
	BaseDlg::OnClosed(fOK);
}

C4GUI::InputResult C4ChatInputDialog::OnChatInput(C4GUI::Edit *pEdt, bool fPasting, bool fPastingMore)
{
	// no double processing
	if (fProcessed) return C4GUI::IR_CloseDlg;
	// get edit text
	char *szInputText = const_cast<char *>(pEdt->GetText());
	// Store to back buffer
	Game.MessageInput.StoreBackBuffer(szInputText);
	// script queried input?
	if (fObjInput)
	{
		fProcessed = true;
		// check if the target input is still valid
		C4Player *pPlr = Game.Players.Get(iPlr);
		if (!pPlr) return C4GUI::IR_CloseDlg;
		if (!pPlr->MarkMessageBoardQueryAnswered(pTarget))
		{
			// there was no associated query!
			return C4GUI::IR_CloseDlg;
		}
		// then do a script callback, incorporating the input into the answer
		if (fUppercase) SCapitalize(szInputText);
		Game.Control.DoInput(CID_MessageBoardAnswer, new C4ControlMessageBoardAnswer(pTarget ? pTarget->Number : 0, iPlr, szInputText), CDT_Decide);
		return C4GUI::IR_CloseDlg;
	}
	else
		// reroute to message input class
		Game.MessageInput.ProcessInput(szInputText);
	// safety: message board commands may do strange things
	if (!C4GUI::IsGUIValid() || this != pInstance) return C4GUI::IR_Abort;
	// select all text to be removed with next keypress
	// just for pasting mode; usually the dlg will be closed now anyway
	pEdt->SelectAll();
	// avoid dlg close, if more content is to be pasted
	if (fPastingMore) return C4GUI::IR_None;
	fProcessed = true;
	return C4GUI::IR_CloseDlg;
}

bool C4ChatInputDialog::KeyHistoryUpDown(bool fUp)
{
	// browse chat history
	pEdit->SelectAll(); pEdit->DeleteSelection();
	const char *szPrevInput = Game.MessageInput.GetBackBuffer(fUp ? (++BackIndex) : (--BackIndex));
	if (!szPrevInput || !*szPrevInput)
		BackIndex = -1;
	else
	{
		pEdit->InsertText(szPrevInput, true);
		pEdit->SelectAll();
	}
	return true;
}

bool C4ChatInputDialog::KeyPlrControl(C4KeyCodeEx key)
{
	// Control pressed while doing this key: Reroute this key as a player-control
	Game.DoKeyboardInput(uint16_t(key.Key), KEYEV_Down, !!(key.dwShift & KEYS_Alt), false, !!(key.dwShift & KEYS_Shift), key.IsRepeated(), nullptr, true);
	// mark as processed, so it won't get any double processing
	return true;
}

bool C4ChatInputDialog::KeyGamepadControlDown(C4KeyCodeEx key)
{
	// filter gamepad control
	if (!Key_IsGamepad(key.Key)) return false;
	// forward it
	Game.DoKeyboardInput(key.Key, KEYEV_Down, false, false, false, key.IsRepeated(), nullptr, true);
	return true;
}

bool C4ChatInputDialog::KeyGamepadControlUp(C4KeyCodeEx key)
{
	// filter gamepad control
	if (!Key_IsGamepad(key.Key)) return false;
	// forward it
	Game.DoKeyboardInput(key.Key, KEYEV_Up, false, false, false, key.IsRepeated(), nullptr, true);
	return true;
}

bool C4ChatInputDialog::KeyGamepadControlPressed(C4KeyCodeEx key)
{
	// filter gamepad control
	if (!Key_IsGamepad(key.Key)) return false;
	// forward it
	Game.DoKeyboardInput(key.Key, KEYEV_Pressed, false, false, false, key.IsRepeated(), nullptr, true);
	return true;
}

bool C4ChatInputDialog::KeyBackspaceClose()
{
	// close if chat text box is empty (on backspace)
	if (pEdit->GetText() && *pEdit->GetText()) return false;
	Close(false);
	return true;
}

bool C4ChatInputDialog::KeyCompleteNick()
{
	if (!pEdit) return false;
	char IncompleteNick[256 + 1];
	// get current word in edit
	if (!pEdit->GetCurrentWord(IncompleteNick, 256)) return false;
	if (!*IncompleteNick) return false;
	C4Player *plr = Game.Players.First;
	while (plr)
	{
		// Compare name and input
		if (SEqualNoCase(plr->GetName(), IncompleteNick, SLen(IncompleteNick)))
		{
			pEdit->InsertText(plr->GetName() + SLen(IncompleteNick), true);
			return true;
		}
		else
			plr = plr->Next;
	}
	// no match found
	return false;
}

// C4MessageInput

bool C4MessageInput::Init()
{
	// add default commands
	if (Commands.empty())
	{
		AddCommand(SPEED, "SetGameSpeed(%d)");
	}
	return true;
}

void C4MessageInput::Default()
{
	// clear backlog
	for (int32_t cnt = 0; cnt < C4MSGB_BackBufferMax; cnt++) BackBuffer[cnt][0] = 0;
}

void C4MessageInput::Clear()
{
	// close any dialog
	CloseTypeIn();
	Commands.clear();
}

bool C4MessageInput::CloseTypeIn()
{
	// close dialog if present and valid
	C4ChatInputDialog *pDlg = GetTypeIn();
	if (!pDlg || !C4GUI::IsGUIValid()) return false;
	pDlg->Close(false);
	return true;
}

bool C4MessageInput::StartTypeIn(bool fObjInput, C4Object *pObj, bool fUpperCase, C4ChatInputDialog::Mode mode, int32_t iPlr, const StdStrBuf &rsInputQuery)
{
	if (!C4GUI::IsGUIValid()) return false;

	// existing dialog? only close if empty
	if (C4ChatInputDialog *const dialog{GetTypeIn()}; dialog)
	{
		if (dialog->IsEmpty())
		{
			dialog->Close(false);
		}
		else
		{
			return false;
		}
	}

	// start new
	return Game.pGUI->ShowRemoveDlg(new C4ChatInputDialog(fObjInput, pObj, fUpperCase, mode, iPlr, rsInputQuery));
}

bool C4MessageInput::KeyStartTypeIn(C4ChatInputDialog::Mode mode)
{
	// fullscreen only
	if (!Application.isFullScreen) return false;
	// OK, start typing
	return StartTypeIn(false, nullptr, false, mode);
}

bool C4MessageInput::IsTypeIn()
{
	// check GUI and dialog
	return C4GUI::IsGUIValid() && C4ChatInputDialog::IsShown();
}

bool C4MessageInput::ProcessInput(const char *szText)
{
	// helper variables
	C4ControlMessageType eMsgType;
	const char *szMsg = nullptr;
	int32_t iToPlayer = -1;
	std::string tmpString;

	// Starts with '^', "team:" or "/team ": Team message
	if (szText[0] == '^' || SEqual2NoCase(szText, "team:") || SEqual2NoCase(szText, "/team "))
	{
		if (!Game.Teams.IsTeamVisible())
		{
			// team not known; can't send!
			Log(C4ResStrTableKey::IDS_MSG_CANTSENDTEAMMESSAGETEAMSN);
			return false;
		}
		else
		{
			eMsgType = C4CMT_Team;
			szMsg = szText[0] == '^' ? szText + 1 :
				szText[0] == '/' ? szText + 6 : szText + 5;
		}
	}
	// Starts with "/private ": Private message (running game only)
	else if (Game.IsRunning && SEqual2NoCase(szText, "/private "))
	{
		// get target name
		char szTargetPlr[C4MaxName + 1];
		SCopyUntil(szText + 9, szTargetPlr, ' ', C4MaxName);
		// search player
		C4Player *pToPlr = Game.Players.GetByName(szTargetPlr);
		if (!pToPlr) return false;
		// set
		eMsgType = C4CMT_Private;
		iToPlayer = pToPlr->Number;
		szMsg = szText + 10 + SLen(szTargetPlr);
		if (szMsg > szText + SLen(szText)) return false;
	}
	// Starts with "/me ": Me-Message
	else if (SEqual2NoCase(szText, "/me "))
	{
		eMsgType = C4CMT_Me;
		szMsg = szText + 4;
	}
	// Starts with "/sound ": Sound-Message
	else if (SEqual2NoCase(szText, "/sound "))
	{
		eMsgType = C4CMT_Sound;
		szMsg = szText + 7;
	}
	// Starts with "/alert": Taskbar flash (message optional)
	else if (SEqual2NoCase(szText, "/alert ") || SEqualNoCase(szText, "/alert"))
	{
		eMsgType = C4CMT_Alert;
		szMsg = szText + 6;
		if (*szMsg) ++szMsg;
	}
	// Starts with '"': Let the clonk say it
	else if (Game.IsRunning && szText[0] == '"')
	{
		eMsgType = C4CMT_Say;
		// Append '"', if neccessary
		tmpString = szText;
		if (tmpString.back() != '"') tmpString.push_back('"');
		szMsg = tmpString.c_str();
	}
	// Starts with '/': Command
	else if (szText[0] == '/')
		return ProcessCommand(szText);
	// Regular message
	else
	{
		eMsgType = C4CMT_Normal;
		szMsg = szText;
	}

	// message?
	if (szMsg)
	{
		char szMessage[C4MaxMessage + 1];
		// go over whitespaces, check empty message
		while (IsWhiteSpace(*szMsg)) szMsg++;
		if (!*szMsg)
		{
			if (eMsgType != C4CMT_Alert) return true;
			*szMessage = '\0';
		}
		else
		{
			// trim right
			const char *szEnd = szMsg + SLen(szMsg) - 1;
			while (IsWhiteSpace(*szEnd) && szEnd >= szMsg) szEnd--;
			// Say: Strip quotation marks in cinematic film mode
			if (Game.C4S.Head.Film == C4SFilm_Cinematic)
			{
				if (eMsgType == C4CMT_Say) { ++szMsg; szEnd--; }
			}
			// get message
			SCopy(szMsg, szMessage, std::min<unsigned long>(C4MaxMessage, szEnd - szMsg + 1));
		}
		// get sending player (if any)
		C4Player *pPlr = Game.IsRunning ? Game.Players.GetLocalByIndex(0) : nullptr;
		// send
		Game.Control.DoInput(CID_Message,
			new C4ControlMessage(eMsgType, szMessage, pPlr ? pPlr->Number : -1, iToPlayer),
			CDT_Private);
	}

	return true;
}

bool C4MessageInput::ProcessCommand(const char *szCommand)
{
	C4GameLobby::MainDlg *pLobby = Game.Network.GetLobby();
	// command
	char szCmdName[C4MaxName + 1];
	SCopyUntil(szCommand + 1, szCmdName, ' ', C4MaxName);
	// parameter
	const char *pCmdPar = SSearch(szCommand, " ");
	if (!pCmdPar) pCmdPar = "";

	// dev-scripts
	if (SEqual(szCmdName, "help"))
	{
		Log(C4ResStrTableKey::IDS_TEXT_COMMANDSAVAILABLEDURINGGA);
		LogNTr("/private [player] [message] - {}", LoadResStr(C4ResStrTableKey::IDS_MSG_SENDAPRIVATEMESSAGETOTHES));
		LogNTr("/team [message] - {}", LoadResStr(C4ResStrTableKey::IDS_MSG_SENDAPRIVATEMESSAGETOYOUR));
		LogNTr("/me [action] - {}", LoadResStr(C4ResStrTableKey::IDS_TEXT_PERFORMANACTIONINYOURNAME));
		LogNTr("/sound [sound] - {}", LoadResStr(C4ResStrTableKey::IDS_TEXT_PLAYASOUNDFROMTHEGLOBALSO));
		LogNTr("/mute [client] - {}", LoadResStr(C4ResStrTableKey::IDS_TEXT_MUTESOUNDCOMMANDSBYTHESPE));
		LogNTr("/unmute [client] - {}", LoadResStr(C4ResStrTableKey::IDS_TEXT_UNMUTESOUNDCOMMANDSBYTHESP));
		LogNTr("/kick [client] - {}", LoadResStr(C4ResStrTableKey::IDS_TEXT_KICKTHESPECIFIEDCLIENT));
		LogNTr("/observer [client] - {}", LoadResStr(C4ResStrTableKey::IDS_TEXT_SETTHESPECIFIEDCLIENTTOOB));
		LogNTr("/fast [x] - {}", LoadResStr(C4ResStrTableKey::IDS_TEXT_SETTOFASTMODESKIPPINGXFRA));
		LogNTr("/slow - {}", LoadResStr(C4ResStrTableKey::IDS_TEXT_SETTONORMALSPEEDMODE));
		LogNTr("/chart - {}", LoadResStr(C4ResStrTableKey::IDS_TEXT_DISPLAYNETWORKSTATISTICS));
		LogNTr("/nodebug - {}", LoadResStr(C4ResStrTableKey::IDS_TEXT_PREVENTDEBUGMODEINTHISROU));
		LogNTr("/set comment [comment] - {}", LoadResStr(C4ResStrTableKey::IDS_TEXT_SETANEWNETWORKCOMMENT));
		LogNTr("/set password [password] - {}", LoadResStr(C4ResStrTableKey::IDS_TEXT_SETANEWNETWORKPASSWORD));
		LogNTr("/set faircrew [on/off] - {}", LoadResStr(C4ResStrTableKey::IDS_TEXT_ENABLEORDISABLEFAIRCREW));
		LogNTr("/set maxplayer [4] - {}", LoadResStr(C4ResStrTableKey::IDS_TEXT_SETANEWMAXIMUMNUMBEROFPLA));
		LogNTr("/script [script] - {}", LoadResStr(C4ResStrTableKey::IDS_TEXT_EXECUTEASCRIPTCOMMAND));
		LogNTr("/clear - {}", LoadResStr(C4ResStrTableKey::IDS_MSG_CLEARTHEMESSAGEBOARD));
		return true;
	}
	// dev-scripts
	if (SEqual(szCmdName, "script"))
	{
		if (!Game.IsRunning) return false;
		if (!Game.DebugMode) return false;
		if (Game.Network.isEnabled() && !Game.Network.isHost()) return false;

		Game.Control.DoInput(CID_Script, new C4ControlScript(pCmdPar, C4ControlScript::SCOPE_Console, Config.Developer.ConsoleScriptStrictness), CDT_Decide);
		return true;
	}
	// set runtimte properties
	if (SEqual(szCmdName, "set"))
	{
		if (SEqual2(pCmdPar, "maxplayer "))
		{
			if (Game.Control.isCtrlHost())
			{
				if (atoi(pCmdPar + 10) == 0 && !SEqual(pCmdPar + 10, "0"))
				{
					LogNTr("Syntax: /set maxplayer count");
					return false;
				}
				Game.Control.DoInput(CID_Set,
					new C4ControlSet(C4CVT_MaxPlayer, atoi(pCmdPar + 10)),
					CDT_Decide);
				return true;
			}
		}
		if (SEqual2(pCmdPar, "comment ") || SEqual(pCmdPar, "comment"))
		{
			if (!Game.Network.isEnabled() || !Game.Network.isHost()) return false;
			// Set in configuration, update reference
			Config.Network.Comment.CopyValidated(pCmdPar[7] ? (pCmdPar + 8) : "");
			Game.Network.InvalidateReference();
			Log(C4ResStrTableKey::IDS_NET_COMMENTCHANGED);
			return true;
		}
		if (SEqual2(pCmdPar, "password ") || SEqual(pCmdPar, "password"))
		{
			if (!Game.Network.isEnabled() || !Game.Network.isHost()) return false;
			Game.Network.SetPassword(pCmdPar[8] ? (pCmdPar + 9) : nullptr);
			if (pLobby) pLobby->UpdatePassword();
			return true;
		}
		if (SEqual2(pCmdPar, "faircrew "))
		{
			if (!Game.Control.isCtrlHost() || Game.Parameters.isLeague()) return false;
			C4ControlSet *pSet = nullptr;
			if (SEqual(pCmdPar + 9, "on"))
				pSet = new C4ControlSet(C4CVT_FairCrew, Config.General.FairCrewStrength);
			else if (SEqual(pCmdPar + 9, "off"))
				pSet = new C4ControlSet(C4CVT_FairCrew, -1);
			else if (isdigit(static_cast<unsigned char>(pCmdPar[9])))
				pSet = new C4ControlSet(C4CVT_FairCrew, atoi(pCmdPar + 9));
			else
				return false;
			Game.Control.DoInput(CID_Set, pSet, CDT_Decide);
			return true;
		}
		// unknown property
		return false;
	}
	// get szen from network folder - not in lobby; use res tab there
	if (SEqual(szCmdName, "netgetscen"))
	{
		if (Game.Network.isEnabled() && !Game.Network.isHost() && !pLobby)
		{
			const C4Network2ResCore *pResCoreScen = Game.Parameters.Scenario.getResCore();
			if (pResCoreScen)
			{
				C4Network2Res::Ref pScenario = Game.Network.ResList.getRefRes(pResCoreScen->getID());
				if (pScenario)
					if (C4Group_CopyItem(pScenario->getFile(), Config.AtExePath(GetFilename(Game.ScenarioFilename))))
					{
						Log(C4ResStrTableKey::IDS_MSG_CMD_NETGETSCEN_SAVED, Config.AtExePath(GetFilename(Game.ScenarioFilename)));
						return true;
					}
			}
		}
		return false;
	}
	// clear message board
	if (SEqual(szCmdName, "clear"))
	{
		// lobby
		if (pLobby)
		{
			pLobby->ClearLog();
		}
		// fullscreen
		else if (Game.GraphicsSystem.MessageBoard.Active)
			Game.GraphicsSystem.MessageBoard.ClearLog();
		else
		{
			// EM mode
			Console.ClearLog();
		}
		return true;
	}
	// kick client
	if (SEqual(szCmdName, "kick"))
	{
		if (Game.Network.isEnabled() && Game.Network.isHost())
		{
			// find client
			C4Client *pClient = Game.Clients.getClientByName(pCmdPar);
			if (!pClient)
			{
				Log(C4ResStrTableKey::IDS_MSG_CMD_NOCLIENT, pCmdPar);
				return false;
			}
			// league: Kick needs voting
			if (Game.Parameters.isLeague() && Game.Players.GetAtClient(pClient->getID()))
				Game.Network.Vote(VT_Kick, true, pClient->getID());
			else
				// add control
				Game.Clients.CtrlRemove(pClient, LoadResStr(C4ResStrTableKey::IDS_MSG_KICKFROMMSGBOARD));
		}
		return true;
	}
	// set fast mode
	if (SEqual(szCmdName, "fast"))
	{
		if (!Game.IsRunning) return false;
		if (Game.Parameters.isLeague())
		{
			Log(C4ResStrTableKey::IDS_LOG_COMMANDNOTALLOWEDINLEAGUE);
			return false;
		}
		int32_t iFS;
		if ((iFS = atoi(pCmdPar)) == 0) return false;
		// set frameskip and fullspeed flag
		Game.FrameSkip = BoundBy<int32_t>(iFS, 1, 500);
		Game.FullSpeed = true;
		// start calculation immediatly
		Application.NextTick(false);
		return true;
	}
	// reset fast mode
	if (SEqual(szCmdName, "slow"))
	{
		if (!Game.IsRunning) return false;
		Game.FullSpeed = false;
		Game.FrameSkip = 1;
		return true;
	}

	if (SEqual(szCmdName, "nodebug"))
	{
		if (!Game.IsRunning) return false;
		Game.Control.DoInput(CID_Set, new C4ControlSet(C4CVT_DisableDebug, 0), CDT_Decide);
		return true;
	}

	if (SEqual(szCmdName, "msgboard"))
	{
		if (!Game.IsRunning) return false;
		// get line cnt
		int32_t iLineCnt = BoundBy(atoi(pCmdPar), 0, 20);
		if (iLineCnt == 0)
			Game.GraphicsSystem.MessageBoard.ChangeMode(2);
		else if (iLineCnt == 1)
			Game.GraphicsSystem.MessageBoard.ChangeMode(0);
		else
		{
			Game.GraphicsSystem.MessageBoard.iLines = iLineCnt;
			Game.GraphicsSystem.MessageBoard.ChangeMode(1);
		}
		return true;
	}

	// kick/activate/deactivate/observer
	if (SEqual(szCmdName, "activate") || SEqual(szCmdName, "deactivate") || SEqual(szCmdName, "observer"))
	{
		if (!Game.Network.isEnabled() || !Game.Network.isHost())
		{
			Log(C4ResStrTableKey::IDS_MSG_CMD_HOSTONLY); return false;
		}
		// search for client
		C4Client *pClient = Game.Clients.getClientByName(pCmdPar);
		if (!pClient)
		{
			Log(C4ResStrTableKey::IDS_MSG_CMD_NOCLIENT, pCmdPar);
			return false;
		}
		// what to do?
		C4ControlClientUpdate *pCtrl = nullptr;
		if (szCmdName[0] == 'a') // activate
			pCtrl = new C4ControlClientUpdate(pClient->getID(), CUT_Activate, true);
		else if (szCmdName[0] == 'd' && !Game.Parameters.isLeague()) // deactivate
			pCtrl = new C4ControlClientUpdate(pClient->getID(), CUT_Activate, false);
		else if (szCmdName[0] == 'o' && !Game.Parameters.isLeague()) // observer
			pCtrl = new C4ControlClientUpdate(pClient->getID(), CUT_SetObserver);
		// perform it
		if (pCtrl)
			Game.Control.DoInput(CID_ClientUpdate, pCtrl, CDT_Sync);
		else
			Log(C4ResStrTableKey::IDS_LOG_COMMANDNOTALLOWEDINLEAGUE);
		return true;
	}

	// control mode
	if (SEqual(szCmdName, "centralctrl") || SEqual(szCmdName, "decentralctrl") || SEqual(szCmdName, "asyncctrl"))
	{
		if (!Game.Network.isEnabled() || !Game.Network.isHost())
		{
			Log(C4ResStrTableKey::IDS_MSG_CMD_HOSTONLY); return false;
		}
		if (Game.Parameters.isLeague() && *szCmdName == 'a')
		{
			Log(C4ResStrTableKey::IDS_LOG_COMMANDNOTALLOWEDINLEAGUE); return false;
		}
		Game.Network.SetCtrlMode(*szCmdName == 'c' ? CNM_Central : *szCmdName == 'd' ? CNM_Decentral : CNM_Async);
		return true;
	}

	// mute
	if (SEqual(szCmdName, "mute"))
	{
		if (auto *client = Game.Clients.getClientByName(pCmdPar); client)
			client->SetMuted(true);
		return true;
	}

	// unmute
	if (SEqual(szCmdName, "unmute"))
	{
		if (auto *client = Game.Clients.getClientByName(pCmdPar); client)
			client->SetMuted(false);
		return true;
	}

	// show chart
	if (Game.IsRunning) if (SEqual(szCmdName, "chart"))
		return Game.ToggleChart();

	// custom command
	if (Game.IsRunning && GetCommand(szCmdName))
	{
		const auto *const pLocalPlr = Game.Players.GetLocalByIndex(0);
		const std::int32_t localPlr = pLocalPlr ? pLocalPlr->Number : NO_OWNER;
		// add custom command call
		Game.Control.DoInput(CID_CustomCommand, new C4ControlCustomCommand(localPlr, szCmdName, pCmdPar), CDT_Decide);
		// ok
		return true;
	}

	// unknown command
	const std::string error{LoadResStr(C4ResStrTableKey::IDS_ERR_UNKNOWNCMD, szCmdName)};
	if (pLobby) pLobby->OnError(error.c_str()); else LogNTr(error);
	return false;
}

void C4MessageInput::AddCommand(const std::string &strCommand, const std::string &strScript, C4MessageBoardCommand::Restriction eRestriction)
{
	if (GetCommand(strCommand)) return;
	// create entry
	Commands[strCommand] = {strScript, eRestriction};
}

C4MessageBoardCommand *C4MessageInput::GetCommand(const std::string &strName)
{
	auto command = Commands.find(strName);
	if (command != Commands.end()) return &command->second;
	return nullptr;
}

void C4MessageInput::RemoveCommand(const std::string &command)
{
	Commands.erase(command);
}

void C4MessageInput::ClearPointers(C4Object *pObj)
{
	// target object loose? stop input
	C4ChatInputDialog *pDlg = GetTypeIn();
	if (pDlg && pDlg->GetScriptTargetObject() == pObj) CloseTypeIn();
}

void C4MessageInput::AbortMsgBoardQuery(C4Object *pObj, int32_t iPlr)
{
	// close typein if it is used for the given parameters
	C4ChatInputDialog *pDlg = GetTypeIn();
	if (pDlg && pDlg->IsScriptQueried() && pDlg->GetScriptTargetObject() == pObj && pDlg->GetScriptTargetPlayer() == iPlr) CloseTypeIn();
}

void C4MessageInput::StoreBackBuffer(const char *szMessage)
{
	if (!szMessage || !szMessage[0]) return;
	int32_t i, cnt;
	// Check: Remove doubled buffer
	for (i = 0; i < C4MSGB_BackBufferMax - 1; ++i)
		if (SEqual(BackBuffer[i], szMessage))
			break;
	// Move up buffers
	for (cnt = i; cnt > 0; cnt--) SCopy(BackBuffer[cnt - 1], BackBuffer[cnt]);
	// Add message
	SCopy(szMessage, BackBuffer[0], C4MaxMessage);
}

const char *C4MessageInput::GetBackBuffer(int32_t iIndex)
{
	if (!Inside<int32_t>(iIndex, 0, C4MSGB_BackBufferMax - 1)) return nullptr;
	return BackBuffer[iIndex];
}

void C4MessageBoardCommand::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(script);
	pComp->Separator(StdCompiler::SEP_SEP);

	constexpr StdEnumEntry<Restriction> restrictions[] =
	{
		{"Escaped", C4MSGCMDR_Escaped},
		{"Plain", C4MSGCMDR_Plain},
		{"Identifier", C4MSGCMDR_Identifier}
	};
	pComp->Value(mkEnumAdaptT<int>(restriction, restrictions));
}

bool C4MessageBoardCommand::operator==(const C4MessageBoardCommand &other) const
{
	return script == other.script && restriction == other.restriction;
}

void C4MessageBoardQuery::CompileFunc(StdCompiler *pComp)
{
	// note that this CompileFunc does not save the fAnswered-flag, so pending message board queries will be re-asked when resuming SaveGames
	pComp->Separator(StdCompiler::SEP_START); // '('
	// callback object number
	pComp->Value(pCallbackObj); pComp->Separator();
	// input query string
	pComp->Value(sInputQuery); pComp->Separator();
	// options
	pComp->Value(fIsUppercase);
	// list end
	pComp->Separator(StdCompiler::SEP_END); // ')'
}

bool C4ChatInputDialog::IsEmpty() const
{
	return !*pEdit->GetText();
}
