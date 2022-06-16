/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
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

/* Handles engine execution in developer mode */

#include <C4Include.h>
#include <C4Console.h>
#include <C4Application.h>

#include <C4GameSave.h>
#include <C4UserMessages.h>
#include <C4Version.h>
#include <C4Language.h>
#include <C4Log.h>
#include <C4Player.h>

#include <StdFile.h>
#include <StdGL.h>
#include <StdRegistry.h>

#ifdef _WIN32
#include "C4Windows.h"
#include <commdlg.h>
#endif

#include "imgui.h"

C4Console::C4Console()
{
	Active = false;
	Editing = true;
	fGameOpen = false;
}

#ifdef _WIN32

LRESULT CALLBACK ConsoleWinProc(HWND wnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (Console.ImGui && Console.ImGui->HandleMessage(wnd, Msg, wParam, lParam))
	{
		return 0;
	}

	switch (Msg)
	{
	case WM_ACTIVATEAPP:
		Application.Active = wParam != 0;
		return TRUE;

	case WM_TIMER:
		if (wParam == SEC1_TIMER) { Console.Sec1Timer(); }
		return TRUE;

	case WM_DESTROY:
		StoreWindowPosition(wnd, "Main", Config.GetSubkeyPath("Console"), false);
		Application.Quit();
		return TRUE;

	case WM_CLOSE:
		Console.Close();
		return TRUE;

	case WM_COMMAND:
		// Evaluate command
		switch (LOWORD(wParam))
		{
		case IDOK:
			// IDC_COMBOINPUT to Console.In()
			char buffer[16000];
			GetDlgItemText(wnd, IDC_COMBOINPUT, buffer, 16000);
			if (buffer[0])
				Console.In(buffer);
			return TRUE;

		default:
			return DefWindowProc(wnd, Msg, wParam, lParam);
		};

	case WM_COPYDATA:
	{
		COPYDATASTRUCT *pcds = reinterpret_cast<COPYDATASTRUCT *>(lParam);
		if (pcds->dwData == WM_USER_RELOADFILE)
		{
			// get path, ensure proper termination
			const char *szPath = reinterpret_cast<const char *>(pcds->lpData);
			if (szPath[pcds->cbData - 1]) break;
			// reload
			Game.ReloadFile(szPath);
		}
		return FALSE;
	}
	}

	return DefWindowProc(wnd, Msg, wParam, lParam);
}

#elif defined(USE_X11) && !WITH_DEVELOPER_MODE

void C4Console::HandleMessage(XEvent &e)
{
	// Parent handling
	C4ConsoleBase::HandleMessage(e);

	switch (e.type)
	{
	case FocusIn:
		Application.Active = true;
		break;
	case FocusOut:
		Application.Active = false;
		break;
	}
}

#endif // _WIN32/USE_X11

CStdWindow *C4Console::Init(CStdApp *pApp)
{
	// Active
	Active = true;
	// Editing (enable even if network)
	Editing = true;
	// Create dialog window
#ifdef _WIN32
	WNDCLASSEX WndClass{};
	WndClass.cbSize = sizeof(WNDCLASSEX);
	WndClass.style = CS_DBLCLKS;
	WndClass.lpfnWndProc = ConsoleWinProc;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hInstance = pApp->hInstance;
	WndClass.hCursor = nullptr;
	WndClass.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
	WndClass.lpszMenuName = nullptr;
	WndClass.lpszClassName = "C4Console";
	WndClass.hIcon = nullptr; //LoadIcon(pApp->hInstance, MAKEINTRESOURCE(IDI_00_C4X));
	WndClass.hIconSm = nullptr;//LoadIcon(pApp->hInstance, MAKEINTRESOURCE(IDI_00_C4X));
	assert(RegisterClassEx(&WndClass));
	if (!(hWindow = CreateWindowEx(
		0,
		"C4Console",
		STD_PRODUCT,
		WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_VISIBLE),
		CW_USEDEFAULT, CW_USEDEFAULT, 0, 0,
		nullptr, nullptr, pApp->hInstance, nullptr)))
	{
		char *lpMsgBuf;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			GetLastError(),
			0,
			(LPTSTR)&lpMsgBuf,
			0,
			nullptr);
		LogF("Error creating dialog window: %s", lpMsgBuf);
		// Free the buffer.
		LocalFree(lpMsgBuf);
		return nullptr;
	}

	// Restore window position
	RestoreWindowPosition(hWindow, "Main", Config.GetSubkeyPath("Console"));
	// Set text
	SetCaption(LoadResStr("IDS_CNS_CONSOLE"));

	// Hide title bar
	SetWindowLong(hWindow, GWL_STYLE, GetWindowLong(hWindow, GWL_STYLE) & ~(WS_CAPTION | WS_THICKFRAME));
	SetWindowLong(hWindow, GWL_EXSTYLE, GetWindowLong(hWindow, GWL_EXSTYLE) & ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));

	// Load bitmaps
	/*hbmMouse   = (HBITMAP)LoadBitmap(pApp->hInstance, MAKEINTRESOURCE(IDB_MOUSE));
	hbmMouse2  = (HBITMAP)LoadBitmap(pApp->hInstance, MAKEINTRESOURCE(IDB_MOUSE2));
	hbmCursor  = (HBITMAP)LoadBitmap(pApp->hInstance, MAKEINTRESOURCE(IDB_CURSOR));
	hbmCursor2 = (HBITMAP)LoadBitmap(pApp->hInstance, MAKEINTRESOURCE(IDB_CURSOR2));
	hbmBrush   = (HBITMAP)LoadBitmap(pApp->hInstance, MAKEINTRESOURCE(IDB_BRUSH));
	hbmBrush2  = (HBITMAP)LoadBitmap(pApp->hInstance, MAKEINTRESOURCE(IDB_BRUSH2));
	hbmPlay    = (HBITMAP)LoadBitmap(pApp->hInstance, MAKEINTRESOURCE(IDB_PLAY));
	hbmPlay2   = (HBITMAP)LoadBitmap(pApp->hInstance, MAKEINTRESOURCE(IDB_PLAY2));
	hbmHalt    = (HBITMAP)LoadBitmap(pApp->hInstance, MAKEINTRESOURCE(IDB_HALT));
	hbmHalt2   = (HBITMAP)LoadBitmap(pApp->hInstance, MAKEINTRESOURCE(IDB_HALT2));*/
#endif
	// Success
	return this;
}

void C4Console::InitGUI()
{
	ImGui.emplace(hWindow);
	Draw();
	ShowWindow(hWindow, SW_SHOWNORMAL);
}

void C4Console::In(const char *szText)
{
	if (!Active || !szText) return;
	// begins with '/'? then it's a command
	if (*szText == '/')
	{
		Game.MessageInput.ProcessCommand(szText);
		// done
		return;
	}
	// begins with '#'? then it's a message. Route cia ProcessInput to allow #/sound
	if (*szText == '#')
	{
		Game.MessageInput.ProcessInput(szText + 1);
		return;
	}
	// editing enabled?
	if (!EditCursor.EditingOK()) return;
	// pass through network queue
	Game.Control.DoInput(CID_Script, new C4ControlScript(szText, C4ControlScript::SCOPE_Console), CDT_Decide);
}

void C4Console::Out(const char *szText)
{
	if (!Active) return;
	if (!szText || !*szText) return;

	logBuffer.emplace_back(szText);
	if (logBuffer.size() > LogBufferSize)
	{
		logBuffer.pop_front();
	}
}

void C4Console::ClearLog()
{
	logBuffer.clear();
}

// Someone defines Status as int....
#ifdef Status
#undef Status
#endif

void C4Console::DoPlay()
{
	Game.Unpause();
}

void C4Console::DoHalt()
{
	Game.Pause();
}

void C4Console::UpdateHaltCtrls(bool fHalt)
{
	if (Active)
	{
		halt = fHalt;
	}
}

void C4Console::SaveGame(bool fSaveGame)
{
	// Network hosts only
	if (Game.Network.isEnabled() && !Game.Network.isHost())
	{
		Message(LoadResStr("IDS_GAME_NOCLIENTSAVE")); return;
	}

	// Can't save to child groups
	if (Game.ScenarioFile.GetMother())
	{
		Message(FormatString(LoadResStr("IDS_CNS_NOCHILDSAVE"), GetFilename(Game.ScenarioFile.GetName())).getData());
		return;
	}

	// Save game to open scenario file
	bool fOkay = true;
#ifdef _WIN32
	SetCursor(LoadCursor(0, IDC_WAIT));
#elif WITH_DEVELOPER_MODE
	// Seems not to work. Don't know why...
	gdk_window_set_cursor(window->window, cursorWait);
#endif

	C4GameSave *pGameSave;
	if (fSaveGame)
		pGameSave = new C4GameSaveSavegame();
	else
		pGameSave = new C4GameSaveScenario(!Console.Active || Game.Landscape.Mode == C4LSC_Exact, false);
	if (!pGameSave->Save(Game.ScenarioFile, false))
	{
		Out("Game::Save failed"); fOkay = false;
	}
	delete pGameSave;

	// Close and reopen scenario file to fix file changes
	if (!Game.ScenarioFile.Close())
	{
		Out("ScenarioFile::Close failed"); fOkay = false;
	}
	if (!Game.ScenarioFile.Open(Game.ScenarioFilename))
	{
		Out("ScenarioFile::Open failed"); fOkay = false;
	}

#ifdef _WIN32
	SetCursor(LoadCursor(0, IDC_ARROW));
#elif WITH_DEVELOPER_MODE
	gdk_window_set_cursor(window->window, nullptr);
#endif

	// Initialize/script notification
	if (Game.fScriptCreatedObjects)
		if (!fSaveGame)
		{
			Message((std::string{LoadResStr("IDS_CNS_SCRIPTCREATEDOBJECTS")} + LoadResStr("IDS_CNS_WARNDOUBLE")).c_str());
			Game.fScriptCreatedObjects = false;
		}

	// Status report
	if (!fOkay) Message(LoadResStr("IDS_CNS_SAVERROR"));
	else Out(LoadResStr(fSaveGame ? "IDS_CNS_GAMESAVED" : "IDS_CNS_SCENARIOSAVED"));
}

void C4Console::FileSave(bool fSaveGame)
{
	// Don't quicksave games over scenarios
	if (fSaveGame)
		if (!Game.C4S.Head.SaveGame)
		{
			Message(LoadResStr("IDS_CNS_NOGAMEOVERSCEN"));
			return;
		}
	// Save game
	SaveGame(fSaveGame);
}

void C4Console::FileSaveAs(bool fSaveGame)
{
	// Do save-as dialog
	char filename[512 + 1];
	SCopy(Game.ScenarioFile.GetName(), filename);
	if (!FileSelect(filename, 512,
		"Clonk 4 Scenario\0*.c4s\0\0",
		OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
		true)) return;
	DefaultExtension(filename, "c4s");
	bool fOkay = true;
	// Close current scenario file
	if (!Game.ScenarioFile.Close()) fOkay = false;
	// Copy current scenario file to target
	if (!C4Group_CopyItem(Game.ScenarioFilename, filename)) fOkay = false;
	// Open new scenario file
	SCopy(filename, Game.ScenarioFilename);

	SetCaption(GetFilename(Game.ScenarioFilename));
	if (!Game.ScenarioFile.Open(Game.ScenarioFilename)) fOkay = false;
	// Failure message
	if (!fOkay)
	{
		Message(FormatString(LoadResStr("IDS_CNS_SAVEASERROR"), Game.ScenarioFilename).getData());
		return;
	}
	// Save game
	SaveGame(fSaveGame);
}

void C4Console::Message(const char *const message)
{
	if (Active)
	{
		popupMessage = message;
	}
}

void C4Console::FileOpen()
{
	// Get scenario file name
	char c4sfile[512 + 1] = "";
	if (!FileSelect(c4sfile, 512,
		"Clonk 4 Scenario\0*.c4s;*.c4f;Scenario.txt\0\0",
		OFN_HIDEREADONLY | OFN_FILEMUSTEXIST
	)) return;
	// Compose command line
	std::string commandLine{'"'};
	commandLine.append(c4sfile);
	commandLine.append("\" ");

	// Open game
	Application.InteractiveThread.ExecuteInMainThread([this, commandLine = std::move(commandLine)]
	{
		OpenGame(commandLine.c_str());
	});
}

void C4Console::FileOpenWPlrs()
{
	// Get scenario file name
	char c4sfile[512 + 1] = "";
	if (!FileSelect(c4sfile, 512,
		"Clonk 4 Scenario\0*.c4s;*.c4f\0\0",
		OFN_HIDEREADONLY | OFN_FILEMUSTEXIST
	)) return;
	// Get player file name(s)
	char c4pfile[4096 + 1] = "";
	if (!FileSelect(c4pfile, 4096,
		"Clonk 4 Player\0*.c4p\0\0",
		OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT | OFN_EXPLORER
	)) return;

	// Compose command line
	// Compose command line
	std::string commandLine{'"'};
	commandLine.append(c4sfile);
	commandLine.append("\" ");
	char cmdline[6000] = "";
	SAppend("\"", cmdline, 5999); SAppend(c4sfile, cmdline, 5999); SAppend("\" ", cmdline, 5999);
	if (DirectoryExists(c4pfile)) // Multiplayer
	{
		const char *cptr = c4pfile + SLen(c4pfile) + 1;
		while (*cptr)
		{
			commandLine.append("\"");
			commandLine.append(c4pfile);
			commandLine.append(DirSep);
			commandLine.append(cptr);
			commandLine.append("\" ");
		}
	}
	else // Single player
	{
		commandLine.append("\"");
		commandLine.append(c4pfile);
		commandLine.append("\" ");
	}

	// Open game
	Application.InteractiveThread.ExecuteInMainThread([this, commandLine = std::move(commandLine)]
	{
		OpenGame(commandLine.c_str());
	});
}

void C4Console::FileClose()
{
	CloseGame();
}

#ifdef _WIN32
bool C4Console::FileSelect(char *sFilename, int iSize, const char *szFilter, DWORD    dwFlags, bool fSave)
#else
bool C4Console::FileSelect(char *sFilename, int iSize, const char *szFilter, uint32_t dwFlags, bool fSave)
#endif
{
#ifdef _WIN32
	OPENFILENAME ofn{};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWindow;
	ofn.lpstrFilter = szFilter;
	ofn.lpstrFile = sFilename;
	ofn.nMaxFile = iSize;
	ofn.Flags = dwFlags;

	bool fResult;
	if (fSave)
		fResult = GetSaveFileName(&ofn);
	else
		fResult = GetOpenFileName(&ofn);

	// Reset working directory to exe path as Windows file dialog might have changed it
	SetCurrentDirectory(Config.General.ExePath);
	return fResult;
#elif WITH_DEVELOPER_MODE
	GtkWidget *dialog = gtk_file_chooser_dialog_new(fSave ? "Save file..." : "Load file...", GTK_WINDOW(window), fSave ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, fSave ? GTK_STOCK_SAVE : GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, nullptr);

	// TODO: Set dialog modal?

	if (g_path_is_absolute(sFilename))
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), sFilename);
	else if (fSave)
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), sFilename);

	// Install file filter
	while (*szFilter)
	{
		char pattern[16 + 1];

		GtkFileFilter *filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, szFilter);
		szFilter += SLen(szFilter) + 1;

		while (true)
		{
			SCopyUntil(szFilter, pattern, ';', 16);

			int len = SLen(pattern);
			char last_c = szFilter[len];

			szFilter += (len + 1);

			// Got not all of the filter, try again.
			if (last_c != ';' && last_c != '\0')
				continue;

			gtk_file_filter_add_pattern(filter, pattern);
			if (last_c == '\0') break;
		}

		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	}

	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), (dwFlags & OFN_ALLOWMULTISELECT) != 0);
	gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), true);

	int response;
	while (true)
	{
		response = gtk_dialog_run(GTK_DIALOG(dialog));
		if (response == GTK_RESPONSE_CANCEL || response == GTK_RESPONSE_DELETE_EVENT) break;

		bool error = false;

		// Check for OFN_FILEMUSTEXIST
		if ((dwFlags & OFN_ALLOWMULTISELECT) == 0)
		{
			char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

			if ((dwFlags & OFN_FILEMUSTEXIST) && !g_file_test(filename, G_FILE_TEST_IS_REGULAR))
			{
				Message(FormatString("File \"%s\" does not exist", filename).getData(), false);
				error = true;
			}

			g_free(filename);
		}

		if (!error) break;
	}

	if (response != GTK_RESPONSE_ACCEPT)
	{
		gtk_widget_destroy(dialog);
		return false;
	}

	// Build result string
	if ((dwFlags & OFN_ALLOWMULTISELECT) == 0)
	{
		// Just the file name without multiselect
		char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		SCopy(filename, sFilename, iSize);
		g_free(filename);
	}
	else
	{
		// Otherwise its the folder followed by the file names,
		// separated by '\0'-bytes
		char *folder = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
		int len = SLen(folder);

		if (iSize > 0) SCopy(folder, sFilename, (std::min)(len + 1, iSize));
		iSize -= (len + 1); sFilename += (len + 1);
		g_free(folder);

		GSList *files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
		for (GSList *item = files; item != nullptr; item = item->next)
		{
			const char *file = static_cast<const char *>(item->data);
			char *basefile = g_path_get_basename(file);

			int len = SLen(basefile);
			if (iSize > 0) SCopy(basefile, sFilename, (std::min)(len + 1, iSize));
			iSize -= (len + 1); sFilename += (len + 1);

			g_free(basefile);
			g_free(item->data);
		}

		// End of list
		*sFilename = '\0';
		g_slist_free(files);
	}

	gtk_widget_destroy(dialog);
	return true;
#endif // WITH_DEVELOPER_MODE / _WIN32
	return 0;
}

void C4Console::FileRecord()
{
	// only in running mode
	if (!Game.IsRunning || !Game.Control.IsRuntimeRecordPossible()) return;
	// start record!
	Game.Control.RequestRuntimeRecord();
}

void C4Console::ClearPointers(C4Object *pObj)
{
	EditCursor.ClearPointers(pObj);
	PropertyDlg.ClearPointers(pObj);
}

void C4Console::Default()
{
	EditCursor.Default();
	PropertyDlg.Default();
	ToolsDlg.Default();
	halt = false;
	selectedFunction = nullptr;
	popupMessage.clear();
}

void C4Console::Clear()
{
	C4ConsoleBase::Clear();

	EditCursor.Clear();
	PropertyDlg.Clear();
	ToolsDlg.Clear();
	logBuffer.clear();
#ifndef _WIN32
	Application.Quit();
#endif
}

void C4Console::Close()
{
	Application.Quit();
}

void C4Console::FileQuit()
{
	Close();
}

#define C4COPYRIGHT_YEAR    "2008" // might make this dynamic some time...
#define C4COPYRIGHT_COMPANY "RedWolf Design GmbH"

void C4Console::ViewportNew()
{
	Game.CreateViewport(NO_OWNER);
}

void C4Console::EditTitle()
{
	if (Game.Network.isEnabled()) return;
	Game.Title.Open();
}

void C4Console::EditScript()
{
	if (Game.Network.isEnabled()) return;
	Game.Script.Open();
	Game.ScriptEngine.ReLink(&Game.Defs);
}

void C4Console::EditInfo()
{
	if (Game.Network.isEnabled()) return;
	Game.Info.Open();
}

void C4Console::EditObjects()
{
	ObjectListDlg.Open();
}

void C4Console::PlayerJoin()
{
	// Get player file name(s)
	char c4pfile[4096 + 1] = "";
	if (!FileSelect(c4pfile, 4096,
		"Clonk 4 Player\0*.c4p\0\0",
		OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT | OFN_EXPLORER
	)) return;

	// Compose player file list
	char c4plist[6000] = "";
	// Multiple players
	if (DirectoryExists(c4pfile))
	{
		const char *cptr = c4pfile + SLen(c4pfile) + 1;
		while (*cptr)
		{
			SNewSegment(c4plist);
			SAppend(c4pfile, c4plist); SAppend(DirSep, c4plist); SAppend(cptr, c4plist);
			cptr += SLen(cptr) + 1;
		}
	}
	// Single player
	else
	{
		SAppend(c4pfile, c4plist);
	}

	// Join players (via network/ctrl queue)
	char szPlayerFilename[_MAX_PATH + 1];
	for (int iPar = 0; SCopySegment(c4plist, iPar, szPlayerFilename, ';', _MAX_PATH); iPar++)
		if (szPlayerFilename[0])
			if (Game.Network.isEnabled())
				Game.Network.Players.JoinLocalPlayer(szPlayerFilename, true);
			else
				Game.Players.CtrlJoinLocalNoNetwork(szPlayerFilename, Game.Clients.getLocalID(), Game.Clients.getLocalName());
}

void C4Console::SetCaption(const char *szCaption)
{
	if (!Active) return;
#ifdef _WIN32
	// Sorry, the window caption needs to be constant so
	// the window can be found using FindWindow
	SetTitle(C4ENGINECAPTION);
#else
	SetTitle(szCaption);
#endif
}

void C4Console::Execute()
{
	EditCursor.Execute();
#ifdef _WIN32
	PropertyDlg.Execute();
#endif
	ObjectListDlg.Execute();

	Draw();

	Game.GraphicsSystem.Execute();
}

void C4Console::Draw()
{
	SetSize(400, 400);
	lpDDraw->FillBG();
	ImGui->NewFrame();

	const bool lobbyActive{Game.Network.isLobbyActive()};
	const bool controlsDisabled{!fGameOpen};
	const bool controlsDisabledIfNotInLobby{!lobbyActive && controlsDisabled};

	bool showAbout{false};

	ImGui::SetNextWindowPos({0, 0});
	ImGui::SetNextWindowSize({400, 400});
	ImGui::Begin("Konsolenmodus", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar);

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu(LoadResStr("IDS_MNU_FILE")))
		{
			if (ImGui::MenuItem(LoadResStr("IDS_MNU_OPEN"))) FileOpen();
			if (ImGui::MenuItem(LoadResStr("IDS_MNU_OPENWPLRS"))) FileOpenWPlrs();
			ImGui::Separator();

			ImGui::BeginDisabled(controlsDisabled || !Game.Players.GetCount());
			if (ImGui::MenuItem(LoadResStr("IDS_MNU_SAVEGAME"))) FileSave(true);
			if (ImGui::MenuItem(LoadResStr("IDS_MNU_SAVEGAMEAS"))) FileSaveAs(true);
			ImGui::EndDisabled();

			ImGui::Separator();

			ImGui::BeginDisabled(!Game.IsRunning || !Game.Control.IsRuntimeRecordPossible());
			if (ImGui::MenuItem(LoadResStr("IDS_MNU_RECORD"))) FileRecord();
			ImGui::EndDisabled();

			ImGui::Separator();

			ImGui::BeginDisabled(controlsDisabled);
			if (ImGui::MenuItem(LoadResStr("IDS_MNU_SAVESCENARIO"))) FileSave(false);
			if (ImGui::MenuItem(LoadResStr("IDS_MNU_SAVESCENARIOAS"))) FileSaveAs(false);
			ImGui::EndDisabled();

			ImGui::Separator();

			ImGui::BeginDisabled(controlsDisabled);
			if (ImGui::MenuItem(LoadResStr("IDS_MNU_CLOSE"))) FileClose();
			ImGui::EndDisabled();

			ImGui::EndMenu();
		}

		ImGui::BeginDisabled(controlsDisabled || !Editing);

		if (ImGui::BeginMenu(LoadResStr("IDS_MNU_COMPONENTS")))
		{
			if (ImGui::MenuItem(LoadResStr("IDS_MNU_SCRIPT"))) EditScript();
			if (ImGui::MenuItem(LoadResStr("IDS_MNU_TITLE"))) EditTitle();
			if (ImGui::MenuItem(LoadResStr("IDS_MNU_INFO"))) EditInfo();
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(LoadResStr("IDS_MNU_PLAYER")))
		{
			if (ImGui::MenuItem(LoadResStr("IDS_MNU_JOIN"))) PlayerJoin();

			ImGui::BeginDisabled(!Editing || (Game.Network.isEnabled() && !Game.Network.isHost()));

			const char *resString{LoadResStr(Game.Network.isEnabled() ? "IDS_CNS_PLRQUITNET" : "IDS_CNS_PLRQUIT")};

			for (C4Player *player{Game.Players.First}; player; player = player->Next)
			{
				if (ImGui::MenuItem(FormatString(resString, player->GetName(), player->AtClientName).getData()))
				{
					Game.Control.Input.Add(CID_EliminatePlayer, new C4ControlEliminatePlayer{player->Number});
				}
			}

			ImGui::EndDisabled();

			ImGui::EndMenu();
		}

		ImGui::EndDisabled();

		ImGui::BeginDisabled(controlsDisabled);
		if (ImGui::BeginMenu(LoadResStr("IDS_MNU_VIEWPORT")))
		{
			if (ImGui::MenuItem(LoadResStr("IDS_MNU_NEW"))) ViewportNew();

			for (C4Player *player{Game.Players.First}; player; player = player->Next)
			{
				if (ImGui::MenuItem(FormatString(LoadResStr("IDS_CNS_NEWPLRVIEWPORT"), player->GetName()).getData())) Game.CreateViewport(player->Number);
			}

			ImGui::EndMenu();
		}
		ImGui::EndDisabled();

		ImGui::BeginDisabled(!Game.Network.isHost() || !Game.Network.isEnabled());

		if (ImGui::BeginMenu(LoadResStr("IDS_MNU_NET")))
		{
			constexpr auto removeClientMenuEntry = [](C4Client *const client, const char *const label)
			{
				if (ImGui::MenuItem(label))
				{
					Game.Clients.CtrlRemove(client, LoadResStr("IDS_MSG_KICKBYMENU"));
				}
			};

			removeClientMenuEntry(Game.Clients.getLocal(), LoadResStr("IDS_MNU_NETHOST"));

			for (C4Network2Client *client{Game.Network.Clients.GetNextClient(nullptr)}; client; client = Game.Network.Clients.GetNextClient(client))
			{
				removeClientMenuEntry(client->getClient(), FormatString(LoadResStr(client->isActivated() ? "IDS_MNU_NETCLIENT" : "IDS_MNU_NETCLIENTDE"), client->getName(), client->getID()).getData());
			}
		}

		ImGui::EndDisabled();

		if (ImGui::BeginMenu("?"))
		{
			if (ImGui::MenuItem(LoadResStr("IDS_MENU_ABOUT")))
			{
				showAbout = true;
			}

			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	if (ImGui::BeginTable("table", 2))
	{
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		for (const auto &line : logBuffer)
		{
			ImGui::TextUnformatted(line.c_str(), line.c_str() + line.size());
		}

		ImGui::TableNextColumn();

		const ConsoleMode mode{EditCursor.GetMode()};
		ImGui::BeginDisabled(controlsDisabled);
		if (ImGui::RadioButton("Play", mode == ConsoleMode::Play)) EditCursor.SetMode(ConsoleMode::Play);
		ImGui::EndDisabled();

		ImGui::BeginDisabled(controlsDisabled || !Editing);
		if (ImGui::RadioButton("Edit", mode == ConsoleMode::Edit)) EditCursor.SetMode(ConsoleMode::Edit);
		if (ImGui::RadioButton("Draw", mode == ConsoleMode::Draw)) EditCursor.SetMode(ConsoleMode::Draw);
		ImGui::EndDisabled();

		ImGui::EndTable();
	}

	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::BeginDisabled(controlsDisabled);

	const auto addFunctionEntry = [this](C4AulFunc *const func)
	{
		if (ImGui::Selectable(std::string{func->Name}.append("()").c_str(), selectedFunction == func))
		{
			selectedFunction = func;
		}
	};

	if (ImGui::BeginCombo("##maininput", selectedFunction ? selectedFunction->Name : nullptr))
	{
		// Add global and standard functions
		for (C4AulFunc *func{Game.ScriptEngine.GetFirstFunc()}; func; func = Game.ScriptEngine.GetNextFunc(func))
		{
			if (func->GetPublic())
			{
				addFunctionEntry(func);
			}
		}

		// Add scenario script functions
		if (Game.Script.GetSFunc(0))
		{
			ImGui::Separator();
		}

		C4AulFunc *func;
		for (std::int32_t i{0}; (func = Game.Script.GetSFunc(i)); ++i)
		{
			addFunctionEntry(func);
		}

		ImGui::EndCombo();
	}

	ImGui::SameLine();
	ImGui::Button("OK");

	ImGui::EndDisabled();

	ImGui::Text("Frame: %d Script: %d", Game.FrameCounter, Game.Script.Counter);
	ImGui::SameLine();

	ImGui::BeginDisabled(controlsDisabledIfNotInLobby || !halt);
	if (ImGui::Button("Play")) DoPlay();
	ImGui::EndDisabled();

	ImGui::SameLine();

	ImGui::BeginDisabled(controlsDisabledIfNotInLobby || halt);
	if (ImGui::Button("Pause")) DoHalt();
	ImGui::EndDisabled();

	ImGui::SameLine();

	ImGui::Text("%02d:%02d:%02d (%i FPS)", Game.Time / 3600, (Game.Time % 3600) / 60, Game.Time % 60, Game.FPS);

	const StdStrBuf cursorText{EditCursor.GetStatusBarText()};
	ImGui::Text("Cursor: %s", cursorText.isNull() ? "" : cursorText.getData());

	ImGui::PopStyleVar();

	if (showAbout)
	{
		ImGui::OpenPopup("HelpAbout");
	}

	if (ImGui::BeginPopup("HelpAbout"))
	{
		static constinit const char AboutMessage[] {C4ENGINECAPTION " " C4VERSION "\n\nCopyright(c) " C4COPYRIGHT_YEAR " " C4COPYRIGHT_COMPANY};
		ImGui::TextUnformatted(AboutMessage, AboutMessage + std::size(AboutMessage) - 1);

		ImGui::EndPopup();
	}

	if (!popupMessage.empty())
	{
		ImGui::OpenPopup("Message");
	}

	if (ImGui::BeginPopup("Message"))
	{
		ImGui::TextUnformatted(popupMessage.c_str(), popupMessage.c_str() + popupMessage.size());
		popupMessage.clear();
		ImGui::EndPopup();
	}

	ImGui::End();

	ImGui->Render();

	lpDDraw->PageFlip();
}

bool C4Console::OpenGame(const char *szCmdLine)
{
	bool fGameWasOpen = fGameOpen;
	// Close any old game
	CloseGame();

	// Default game dependent members
	Default();
	SetCaption(GetFilename(Game.ScenarioFile.GetName()));
	// Init game dependent members
	if (!EditCursor.Init()) return false;
	// Default game - only if open before, because we do not want to default out the GUI
	if (fGameWasOpen) Game.Default();

	// Pretend to be called with a new commandline
	if (szCmdLine)
		Game.ParseCommandLine(szCmdLine);

	// PreInit is required because GUI has been deleted
	if (!Game.PreInit()) { Game.Clear(); return false; }

	// Init game
	if (!Game.Init())
	{
		Game.Clear();
		return false;
	}

	// Console updates
	fGameOpen = true;
	if (Game.Control.NoInput())
	{
		Editing = false;
	}

	return true;
}

void C4Console::CloseGame()
{
	if (!fGameOpen) return;
	Game.Clear();
	Game.GameOver = false; // No leftover values when exiting on closed game
	Game.GameOverDlgShown = false;
	fGameOpen = false;
	SetCaption(LoadResStr("IDS_CNS_CONSOLE"));
}

bool C4Console::TogglePause()
{
	return Game.TogglePause();
}
