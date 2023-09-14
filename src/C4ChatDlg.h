/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2007, Sven2
 * Copyright (c) 2017-2020, The LegacyClonk Team and contributors
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

// IRC client dialog

#pragma once

#include "C4Gui.h"
#include "C4InteractiveThread.h"
#include "C4Network2IRC.h"

// a GUI control to chat in
class C4ChatControl : public C4GUI::Window, private C4InteractiveThread::Callback
{
private:
	class ChatSheet;
	enum SheetType { CS_Server, CS_Channel, CS_Query };

private:
	C4GUI::Tabular *pTabMain, *pTabChats;
	// login controls
	C4GUI::Label *pLblLoginNick, *pLblLoginPass, *pLblLoginRealName, *pLblLoginChannel;
	C4GUI::Edit *pEdtLoginNick, *pEdtLoginPass, *pEdtLoginRealName, *pEdtLoginChannel;
	C4GUI::Button *pBtnLogin;

	StdStrBuf sTitle;
	C4GUI::BaseInputCallback *pTitleChangeBC;

	C4Network2IRCClient *pIRCClient;
	bool fInitialMessagesReceived; // set after initial update call, which fetches all messages. Subsequent calls fetch only unread messages

public:
	C4ChatControl(C4Network2IRCClient *pIRC);
	virtual ~C4ChatControl();

protected:
	virtual void UpdateSize() override;
	C4GUI::InputResult OnLoginDataEnter(C4GUI::Edit *edt, bool fPasting, bool fPastingMore); // advance focus when user presses enter in one of the login edits
	void OnConnectBtn(C4GUI::Control *btn); // callback: connect button pressed

public:
	virtual class C4GUI::Control *GetDefaultControl();
	C4Network2IRCClient *getIRCClient() { return pIRCClient; }

	void SetTitleChangeCB(C4GUI::BaseInputCallback *pNewCB);
	virtual void OnShown(); // callback when shown
	void UpdateShownPage();
	void Update();
	void UpdateTitle();
	bool DlgEnter();
	void OnSec1Timer() { Update(); } // timer proc
	bool ProcessInput(const char *szInput, ChatSheet *pChatSheet); // process chat input - return false if no more pasting is to be done (i.e., on /quit or /part in channel)
	const char *GetTitle() { return sTitle.getData(); }
	void UserQueryQuit();
	ChatSheet *OpenQuery(const char *szForNick, bool fSelect, const char *szIdentFallback);

private:
	static bool IsServiceName(const char *szName);
	ChatSheet *GetActiveChatSheet();
	ChatSheet *GetSheetByTitle(const char *szTitle, C4ChatControl::SheetType eType);
	ChatSheet *GetSheetByIdent(const char *szIdent, C4ChatControl::SheetType eType);
	ChatSheet *GetServerSheet();
	void ClearChatSheets();

	// IRC event hook
	virtual void OnThreadEvent(C4InteractiveEventType eEvent, const std::any &eventData) override { if (std::any_cast<decltype(pIRCClient)>(eventData) == pIRCClient) Update(); }
};

// container dialog for the C4ChatControl
class C4ChatDlg : public C4GUI::Dialog
{
private:
	static C4ChatDlg *pInstance;
	class C4ChatControl *pChatCtrl;
	C4GUI::Button *pBtnClose;

public:
	C4ChatDlg();
	virtual ~C4ChatDlg();

	static C4ChatDlg *ShowChat();
	static void StopChat();
	static bool IsChatActive();
	static bool IsChatEnabled();
	static bool ToggleChat();

protected:
	// default control to be set if unprocessed keyboard input has been detected
	virtual class C4GUI::Control *GetDefaultControl() override;

	// true for dialogs that should span the whole screen
	// not just the mouse-viewport
	virtual bool IsFreePlaceDialog() override { return true; }

	// true for dialogs that receive keyboard input even in shared mode
	virtual bool IsExclusiveDialog() override { return true; }

	// for custom placement procedures; should call SetPos
	virtual bool DoPlacement(C4GUI::Screen *pOnScreen, const C4Rect &rPreferredDlgRect) override;

	virtual void OnClosed(bool fOK) override; // callback when dlg got closed
	virtual void OnShown() override; // callback when shown - should not delete the dialog

	virtual void UpdateSize() override;

	void OnChatTitleChange(const StdStrBuf &sNewTitle);
};
