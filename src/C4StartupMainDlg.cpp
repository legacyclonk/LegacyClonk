/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2005, Sven2
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

// Startup screen for non-parameterized engine start (stub)

#include <C4Include.h>
#include <C4StartupMainDlg.h>
#include <C4UpdateDlg.h>
#include <C4Version.h>

#include <C4StartupNetDlg.h>
#include <C4StartupScenSelDlg.h>
#include <C4StartupOptionsDlg.h>
#include <C4StartupAboutDlg.h>
#include <C4StartupPlrSelDlg.h>
#include <C4Startup.h>
#include <C4Game.h>
#include <C4Log.h>
#include <C4Language.h>

C4StartupMainDlg::C4StartupMainDlg() : C4StartupDlg(nullptr) // create w/o title; it is drawn in custom draw proc
{
	fFirstShown = true;
	// screen calculations
	int iButtonPadding = 2;
	int iButtonHeight = C4GUI_BigButtonHgt;
	C4GUI::ComponentAligner caMain(rcBounds, 0, 0, true);
	C4GUI::ComponentAligner caRightPanel(caMain.GetFromRight(rcBounds.Wdt * 2 / 5), rcBounds.Wdt / 26, 40 + rcBounds.Hgt / 8);
	C4GUI::ComponentAligner caButtons(caRightPanel.GetAll(), 0, iButtonPadding);
	// main menu buttons
	C4GUI::CallbackButton<C4StartupMainDlg> *btn;
	AddElement(btn = new C4GUI::CallbackButton<C4StartupMainDlg>(LoadResStr("IDS_BTN_LOCALGAME"), caButtons.GetFromTop(iButtonHeight), &C4StartupMainDlg::OnStartBtn));
	btn->SetToolTip(LoadResStr("IDS_DLGTIP_STARTGAME"));
	btn->SetCustomGraphics(&C4Startup::Get()->Graphics.barMainButtons, &C4Startup::Get()->Graphics.barMainButtonsDown);
	pStartButton = btn;
	AddElement(btn = new C4GUI::CallbackButton<C4StartupMainDlg>(LoadResStr("IDS_BTN_NETWORKGAME"), caButtons.GetFromTop(iButtonHeight), &C4StartupMainDlg::OnNetJoinBtn));
	btn->SetToolTip(LoadResStr("IDS_DLGTIP_NETWORKGAME"));
	btn->SetCustomGraphics(&C4Startup::Get()->Graphics.barMainButtons, &C4Startup::Get()->Graphics.barMainButtonsDown);
	AddElement(btn = new C4GUI::CallbackButton<C4StartupMainDlg>(LoadResStr("IDS_DLG_PLAYERSELECTION"), caButtons.GetFromTop(iButtonHeight), &C4StartupMainDlg::OnPlayerSelectionBtn));
	btn->SetToolTip(LoadResStr("IDS_DLGTIP_PLAYERSELECTION"));
	btn->SetCustomGraphics(&C4Startup::Get()->Graphics.barMainButtons, &C4Startup::Get()->Graphics.barMainButtonsDown);
	AddElement(btn = new C4GUI::CallbackButton<C4StartupMainDlg>(LoadResStr("IDS_DLG_OPTIONS"), caButtons.GetFromTop(iButtonHeight), &C4StartupMainDlg::OnOptionsBtn));
	btn->SetToolTip(LoadResStr("IDS_DLGTIP_OPTIONS"));
	btn->SetCustomGraphics(&C4Startup::Get()->Graphics.barMainButtons, &C4Startup::Get()->Graphics.barMainButtonsDown);
	AddElement(btn = new C4GUI::CallbackButton<C4StartupMainDlg>(LoadResStr("IDS_DLG_ABOUT"), caButtons.GetFromTop(iButtonHeight), &C4StartupMainDlg::OnAboutBtn));
	btn->SetToolTip(LoadResStr("IDS_DLGTIP_ABOUT"));
	btn->SetCustomGraphics(&C4Startup::Get()->Graphics.barMainButtons, &C4Startup::Get()->Graphics.barMainButtonsDown);
	AddElement(btn = new C4GUI::CallbackButton<C4StartupMainDlg>(LoadResStr("IDS_DLG_EXIT"), caButtons.GetFromTop(iButtonHeight), &C4StartupMainDlg::OnExitBtn));
	btn->SetToolTip(LoadResStr("IDS_DLGTIP_EXIT"));
	btn->SetCustomGraphics(&C4Startup::Get()->Graphics.barMainButtons, &C4Startup::Get()->Graphics.barMainButtonsDown);
	// list of selected players
	AddElement(pParticipantsLbl = new C4GUI::Label("test", GetClientRect().Wdt * 39 / 40, GetClientRect().Hgt * 9 / 10, ARight, 0xffffffff, &C4GUI::GetRes()->TitleFont, false));
	pParticipantsLbl->SetToolTip(LoadResStr("IDS_DLGTIP_SELECTEDPLAYERS"));

	CStdFont &trademarkFont = C4GUI::GetRes()->MiniFont;
	AddElement(new C4GUI::Label(FANPROJECTTEXT "   " TRADEMARKTEXT,
		GetClientRect().Wdt, GetClientRect().Hgt - trademarkFont.GetLineHeight() / 2, ARight, 0xffffffff, &trademarkFont));

	// player selection shortcut - to be made optional
	UpdateParticipants();
	pParticipantsLbl->SetContextHandler(new C4GUI::CBContextHandler<C4StartupMainDlg>(this, &C4StartupMainDlg::OnPlayerSelContext));
	// key bindings
	C4CustomKey::CodeList keys;
	keys.push_back(C4KeyCodeEx(K_DOWN)); keys.push_back(C4KeyCodeEx(K_RIGHT));
	if (Config.Controls.GamepadGuiControl)
	{
		keys.push_back(C4KeyCodeEx(KEY_Gamepad(0, KEY_JOY_Down))); // right will be done by Dialog already
	}
	pKeyDown = new C4KeyBinding(keys, "StartupMainCtrlNext", KEYSCOPE_Gui,
		new C4GUI::DlgKeyCBEx<C4StartupMainDlg, bool>(*this, false, &C4StartupMainDlg::KeyAdvanceFocus), C4CustomKey::PRIO_CtrlOverride);
	keys.clear(); keys.push_back(C4KeyCodeEx(K_UP)); keys.push_back(C4KeyCodeEx(K_LEFT));
	if (Config.Controls.GamepadGuiControl)
	{
		keys.push_back(C4KeyCodeEx(KEY_Gamepad(0, KEY_JOY_Up))); // left will be done by Dialog already
	}
	pKeyUp = new C4KeyBinding(keys, "StartupMainCtrlPrev", KEYSCOPE_Gui,
		new C4GUI::DlgKeyCBEx<C4StartupMainDlg, bool>(*this, true, &C4StartupMainDlg::KeyAdvanceFocus), C4CustomKey::PRIO_CtrlOverride);
	keys.clear(); keys.push_back(C4KeyCodeEx(K_RETURN));
	pKeyEnter = new C4KeyBinding(keys, "StartupMainOK", KEYSCOPE_Gui,
		new C4GUI::DlgKeyCB<C4StartupMainDlg>(*this, &C4StartupMainDlg::KeyEnterDown, &C4StartupMainDlg::KeyEnterUp), C4CustomKey::PRIO_CtrlOverride);
	keys.clear(); keys.push_back(C4KeyCodeEx(K_F6));
	pKeyEditor = new C4KeyBinding(keys, "StartupMainEditor", KEYSCOPE_Gui,
		new C4GUI::DlgKeyCB<C4StartupMainDlg>(*this, &C4StartupMainDlg::SwitchToEditor), C4CustomKey::PRIO_CtrlOverride);
}

C4StartupMainDlg::~C4StartupMainDlg()
{
	delete pKeyEnter;
	delete pKeyUp;
	delete pKeyDown;
	delete pKeyEditor;
}

void C4StartupMainDlg::DrawElement(C4FacetEx &cgo)
{
	// inherited
	typedef C4GUI::FullscreenDialog Base;
	Base::DrawElement(cgo);
	// draw logo
	C4FacetEx &fctLogo = Game.GraphicsResource.fctLogo;
	float fLogoZoom = 0.4f;
	fctLogo.DrawX(cgo.Surface, rcBounds.Wdt * 30 / 31 - int32_t(fLogoZoom * fctLogo.Wdt), rcBounds.Hgt / 21 - 5, int32_t(fLogoZoom * fctLogo.Wdt), int32_t(fLogoZoom * fctLogo.Hgt));
	// draw version info
	StdStrBuf sVer;
	sVer.Format(LoadResStr("IDS_DLG_VERSION"), C4VERSION);
	lpDDraw->TextOut(sVer.getData(), C4GUI::GetRes()->TextFont, 1.0f, cgo.Surface, rcBounds.Wdt * 39 / 40, rcBounds.Hgt / 18 + int32_t(fLogoZoom * fctLogo.Hgt), 0xffffffff, ARight, true);
}

C4GUI::ContextMenu *C4StartupMainDlg::OnPlayerSelContext(C4GUI::Element *pBtn, int32_t iX, int32_t iY)
{
	// preliminary player selection via simple context menu
	C4GUI::ContextMenu *pCtx = new C4GUI::ContextMenu();
	pCtx->AddItem("Add", "Add participant", C4GUI::Ico_None, nullptr, new C4GUI::CBContextHandler<C4StartupMainDlg>(this, &C4StartupMainDlg::OnPlayerSelContextAdd));
	pCtx->AddItem("Remove", "Remove participant", C4GUI::Ico_None, nullptr, new C4GUI::CBContextHandler<C4StartupMainDlg>(this, &C4StartupMainDlg::OnPlayerSelContextRemove));
	return pCtx;
}

C4GUI::ContextMenu *C4StartupMainDlg::OnPlayerSelContextAdd(C4GUI::Element *pBtn, int32_t iX, int32_t iY)
{
	auto contextMenu = std::make_unique<C4GUI::ContextMenu>();
	for (const auto &pathInfo : Reloc)
	{
		for (const auto &item : fs::directory_iterator{pathInfo.Path})
		{
			const std::string pathString{(pathInfo.Path / item.path()).string()};
			const std::string fileName{item.path().filename().string()};
			if (fileName[0] == '.')
			{
				continue;
			}

			if (!WildcardMatch(C4CFN_PlayerFiles, fileName.c_str()))
			{
				continue;
			}

			if (!SIsModule(Config.General.Participants, pathString.c_str()))
			{
				contextMenu->AddItem(C4Language::IconvClonk(item.path().filename().replace_extension().string().c_str()).getData(), "Let this player join in next game", C4GUI::Ico_Player,
					new C4GUI::CBMenuHandlerEx<C4StartupMainDlg, StdStrBuf>{this, &C4StartupMainDlg::OnPlayerSelContextAddPlr, StdStrBuf{pathString.c_str()}}, nullptr);
			}
		}
	}

	return contextMenu.release();
}

C4GUI::ContextMenu *C4StartupMainDlg::OnPlayerSelContextRemove(C4GUI::Element *pBtn, int32_t iX, int32_t iY)
{
	C4GUI::ContextMenu *pCtx = new C4GUI::ContextMenu();
	char szPlayer[1024 + 1];
	for (int i = 0; SCopySegment(Config.General.Participants, i, szPlayer, ';', 1024, true); i++)
		if (*szPlayer)
			pCtx->AddItem(GetFilenameOnly(szPlayer), "Remove this player from participation list", C4GUI::Ico_Player, new C4GUI::CBMenuHandlerEx<C4StartupMainDlg, int>(this, &C4StartupMainDlg::OnPlayerSelContextRemovePlr, i), nullptr);
	return pCtx;
}

void C4StartupMainDlg::OnPlayerSelContextAddPlr(C4GUI::Element *pTarget, const StdStrBuf &rsFilename)
{
	SAddModule(Config.General.Participants, rsFilename.getData());
	UpdateParticipants();
}

void C4StartupMainDlg::OnPlayerSelContextRemovePlr(C4GUI::Element *pTarget, const int &iIndex)
{
	char szPlayer[1024 + 1];
	if (SCopySegment(Config.General.Participants, iIndex, szPlayer, ';', 1024, true))
		SRemoveModule(Config.General.Participants, szPlayer, false);
	UpdateParticipants();
}

void C4StartupMainDlg::UpdateParticipants()
{
	// First validate all participants (files must exist)
	StdStrBuf strPlayers, strPlayer; strPlayer.SetLength(1024 + 1);
	strPlayers.Copy(Config.General.Participants); *Config.General.Participants = 0;
	for (int i = 0; SCopySegment(strPlayers.getData(), i, strPlayer.getMData(), ';', 1024, true); i++)
	{
		const char *szPlayer = strPlayer.getData();
		if (!szPlayer || !*szPlayer) continue;
		if (!FileExists(szPlayer)) continue;
		if (!SEqualNoCase(GetExtension(szPlayer), "c4p")) continue; // additional sanity check to clear strange exe-path-only entries in player list?
		SAddModule(Config.General.Participants, szPlayer);
	}
	// Draw selected players - we are currently displaying the players stored in Config.General.Participants.
	// Existence of the player files is not validated and player filenames are displayed directly
	// (names are not loaded from the player core).
	strPlayers = LoadResStr("IDS_DESC_PLRS");
	if (!Config.General.Participants[0])
		strPlayers.Append(LoadResStr("IDS_DLG_NOPLAYERSSELECTED"));
	else
		for (int i = 0; SCopySegment(Config.General.Participants, i, strPlayer.getMData(), ';', 1024, true); i++)
		{
			if (i > 0) strPlayers.Append(", ");
			strPlayers.Append(C4Language::IconvClonk(GetFilenameOnly(strPlayer.getData())).getData());
		}
	pParticipantsLbl->SetText(strPlayers.getData());
}

void C4StartupMainDlg::OnClosed(bool fOK)
{
	// if dlg got aborted (by user), quit startup
	// if it got closed with OK, the user pressed one of the buttons and dialog switching has been done already
	if (!fOK) C4Startup::Get()->Exit();
}

void C4StartupMainDlg::OnStartBtn(C4GUI::Control *btn)
{
	// advance to scenario selection screen
	C4Startup::Get()->SwitchDialog(C4Startup::SDID_ScenSel);
}

void C4StartupMainDlg::OnPlayerSelectionBtn(C4GUI::Control *btn)
{
	// advance to player selection screen
	C4Startup::Get()->SwitchDialog(C4Startup::SDID_PlrSel);
}

void C4StartupMainDlg::OnNetJoinBtn(C4GUI::Control *btn)
{
	// advanced net join and host dlg!
	C4Startup::Get()->SwitchDialog(C4Startup::SDID_NetJoin);
}

void C4StartupMainDlg::OnOptionsBtn(C4GUI::Control *btn)
{
	// advance to options screen
	C4Startup::Get()->SwitchDialog(C4Startup::SDID_Options);
}

void C4StartupMainDlg::OnAboutBtn(C4GUI::Control *btn)
{
	// advance to about screen
	C4Startup::Get()->SwitchDialog(C4Startup::SDID_About);
}

void C4StartupMainDlg::OnExitBtn(C4GUI::Control *btn)
{
	// callback: exit button pressed
	C4Startup::Get()->Exit();
}

bool C4StartupMainDlg::KeyEnterDown()
{
	// just execute selected button: Re-Send as space
	return Game.DoKeyboardInput(K_SPACE, KEYEV_Down, false, false, false, false, this);
}

bool C4StartupMainDlg::KeyEnterUp()
{
	// just execute selected button: Re-Send as space
	return Game.DoKeyboardInput(K_SPACE, KEYEV_Up, false, false, false, false, this);
}

void C4StartupMainDlg::OnShown()
{
	// Incoming update
	if (Application.IncomingUpdate)
	{
		C4UpdateDlg::ApplyUpdate(Application.IncomingUpdate.getData(), false, GetScreen());
		Application.IncomingUpdate.Clear();
	}
	// Manual update by command line or url
	if (Application.CheckForUpdates)
	{
		C4UpdateDlg::CheckForUpdates(GetScreen(), false);
		Application.CheckForUpdates = false;
	}
	// Automatic update
	else
	{
		if (Config.Network.AutomaticUpdate)
			C4UpdateDlg::CheckForUpdates(GetScreen(), true);
	}

	// first start evaluation
	if (Config.General.FirstStart)
	{
		Config.General.FirstStart = false;
	}

	// first thing that's needed is a new player, if there's none - independent of first start
	bool hasPlayer{false};
	for (const auto &pathInfo : Reloc)
	{
		for (const auto &item : fs::directory_iterator{pathInfo.Path})
		{
			const std::string fileName{item.path().filename().string()};
			if (fileName[0] == '.')
			{
				continue;
			}

			if (WildcardMatch(C4CFN_PlayerFiles, fileName.c_str()))
			{
				hasPlayer = true;
				break;
			}
		}
	}

	if (!hasPlayer)
	{
		// no player created yet: Create one
		C4GUI::Dialog *pDlg;
		GetScreen()->ShowModalDlg(pDlg = new C4StartupPlrPropertiesDlg(nullptr, nullptr), true);
	}
	// make sure participants are updated after switching back from player selection
	UpdateParticipants();

	// First show
	if (fFirstShown)
	{
		// Activate the application (trying to prevent flickering half-focus in win32...)
		Application.Activate();
		// Set the focus to the start button (we might still not have the focus after the update-check sometimes... :/)
		SetFocus(pStartButton, false);
	}
	fFirstShown = false;
}

bool C4StartupMainDlg::SwitchToEditor()
{
#ifdef _WIN32
	// No editor executable available
	if (!FileExists(Config.AtExePath(C4CFN_Editor))) return false;
	// Flag editor launch
	Application.launchEditor = true;
	// Quit
	C4Startup::Get()->Exit();
#endif

	return true;
}
