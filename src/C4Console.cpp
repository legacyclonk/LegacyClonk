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
#include "C4TextEncoding.h"

#include <StdFile.h>

#ifdef _WIN32
#include "StdRegistry.h"
#include "StdStringEncodingConverter.h"
#include "res/engine_resource.h"
#endif

#ifdef _WIN32
#include "C4Windows.h"
#include <commdlg.h>
#endif

#include "imgui/imgui.h"
#include "imgui/textselect.hpp"


C4Console::C4Console()
{
	Active = false;
	Editing = true;
	fGameOpen = false;

	LogTextSelect = new TextSelect{std::bind(&C4Console::GetLineAtIdx, this, std::placeholders::_1), std::bind(&C4Console::GetNumLines, this)};
}

C4Console::~C4Console()
{
	delete LogTextSelect;
}

std::string_view C4Console::GetLineAtIdx(std::size_t idx)
{
	return logBuffer[idx];
}

std::size_t C4Console::GetNumLines()
{
	return logBuffer.size();
}

void C4Console::HandleMessage(SDL_Event& sdl_event)
{
	if (ImGui)
	{
		ImGui->Select();
		ImGui_ImplSDL3_ProcessEvent(&sdl_event);
	}

	for (const std::unique_ptr<C4Viewport>& Viewport : Game.GraphicsSystem.GetViewports())
	{
		if(!Viewport)
		{
			continue;
		}
		if(C4ViewportWindow* ViewportWindow = Viewport->GetViewportWindow())
		{
			ViewportWindow->HandleMessage(sdl_event);
		}
	}
}

#if 0 //def _WIN32

LRESULT CALLBACK ConsoleWinProc(HWND wnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{

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

bool C4Console::Init(CStdApp *app, const char *title, const C4Rect &bounds, CStdWindow *parent, std::uint32_t AdditionalFlags, std::int32_t MinWidth, std::int32_t MinHeight)
{
	// Active
	Active = true;
	// Editing (enable even if network)
	Editing = true;
	// Create dialog window
	return CStdWindow::Init(app, title, bounds, parent, AdditionalFlags, MinWidth, MinHeight);
#if 0 //def _WIN32
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
	return true;
}

void C4Console::InitGUI()
{
	ImGui.emplace(sdlWindow);
	Draw();
	SDL_ShowWindow(sdlWindow);
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
	// begins with '#'? then it's a message. Route via ProcessInput to allow #/sound
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
		logBuffer.erase(logBuffer.begin());
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
		Message(LoadResStr(C4ResStrTableKey::IDS_GAME_NOCLIENTSAVE)); return;
	}

	// Can't save to child groups
	if (Game.ScenarioFile.GetMother())
	{
		Message(LoadResStr(C4ResStrTableKey::IDS_CNS_NOCHILDSAVE, GetFilename(Game.ScenarioFile.GetName())).c_str());
		return;
	}

	// Save game to open scenario file
	bool fOkay = true;

#if 0 //def _WIN32
	SetCursor(LoadCursor(0, IDC_WAIT));
//#elif WITH_DEVELOPER_MODE
	// Seems not to work. Don't know why...
//	gdk_window_set_cursor(window->window, cursorWait);
#endif
	// TODO
	//SDL_SetCursor()

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

	// TODO: Use sdl cursor
//#ifdef _WIN32
//	SetCursor(LoadCursor(0, IDC_ARROW));
//#elif WITH_DEVELOPER_MODE
//	gdk_window_set_cursor(window->window, nullptr);
//#endif

	// Initialize/script notification
	if (Game.fScriptCreatedObjects)
		if (!fSaveGame)
		{
			Message((std::string{LoadResStr(C4ResStrTableKey::IDS_CNS_SCRIPTCREATEDOBJECTS)} + LoadResStr(C4ResStrTableKey::IDS_CNS_WARNDOUBLE)).c_str());
			Game.fScriptCreatedObjects = false;
		}

	// Status report
	if (!fOkay) Message(LoadResStr(C4ResStrTableKey::IDS_CNS_SAVERROR));
	else Out(LoadResStrChoice(fSaveGame, C4ResStrTableKey::IDS_CNS_GAMESAVED, C4ResStrTableKey::IDS_CNS_SCENARIOSAVED));


}

void C4Console::FileSave(bool fSaveGame)
{
	// Don't quicksave games over scenarios
	if (fSaveGame)
		if (!Game.C4S.Head.SaveGame)
		{
			Message(LoadResStr(C4ResStrTableKey::IDS_CNS_NOGAMEOVERSCEN));
			return;
		}
	// Save game
	SaveGame(fSaveGame);
}

void SDLCALL C4Console::FileSaveGameAsCallback(void* userdata, const char* const* filelist, int filter)
{
	if (!filelist)
	{
		SDL_Log("An error occured: %s", SDL_GetError());
		return;
	}
	else if (!*filelist)
	{
		// Dialog canceled.
		return;
	}

	if(!userdata)
	{
		return;
	}

	if(C4Console* ConsoleContext = reinterpret_cast<C4Console*>(userdata))
	{
		std::string Filename = *filelist;
		ConsoleContext->FileSaveMainThread(Filename, true);
	}
}

// TODO: Combine with FileSaveGameAsCallback method. Userdata is already used by console this pointer,
// and using a struct is a bit cumbersome.
void SDLCALL C4Console::FileSaveScenarioAsCallback(void* userdata, const char* const* filelist, int filter)
{
	if (!filelist)
	{
		SDL_Log("An error occured: %s", SDL_GetError());
		return;
	}
	else if (!*filelist)
	{
		// Dialog canceled.
		return;
	}

	if(!userdata)
	{
		return;
	}

	if(C4Console* ConsoleContext = reinterpret_cast<C4Console*>(userdata))
	{
		std::string Filename = *filelist;
		ConsoleContext->FileSaveMainThread(Filename, false);
	}
}

void C4Console::FileSaveMainThread(std::string Filename, bool fSaveGame)
{
	Application.InteractiveThread.ExecuteInMainThread([this, Filename, fSaveGame] mutable
	{
		std::replace(Filename.begin(), Filename.end(), '\\', '/');
		if(!Filename.ends_with(".c4s"))
		{
			Filename.append(".c4s");
		}
		bool fOkay = true;
		// Close current scenario file
		if (!Game.ScenarioFile.Close()) fOkay = false;
		// Copy current scenario file to target
		if (!C4Group_CopyItem(Game.ScenarioFilename, Filename.c_str())) fOkay = false;
		// Open new scenario file
		SCopy(Filename.c_str(), Game.ScenarioFilename);

		//SetCaption(GetFilename(Game.ScenarioFilename)); // TODO: needed?
		if (!Game.ScenarioFile.Open(Game.ScenarioFilename)) fOkay = false;
		// Failure message
		if (!fOkay)
		{
			Message(LoadResStr(C4ResStrTableKey::IDS_CNS_SAVEASERROR, Game.ScenarioFilename).c_str());
			return;
		}
		// Save game
		SaveGame(fSaveGame);
	});
}

void C4Console::FileSaveAs(bool fSaveGame)
{
	static const SDL_DialogFileFilter PlayerFilter[] = {
		{ "Clonk 4 Scenario",  "c4s" }
	};
	SDL_ShowSaveFileDialog(fSaveGame ? FileSaveGameAsCallback : FileSaveScenarioAsCallback, this, sdlWindow, PlayerFilter, 1, Config.General.ExePath);
}

void C4Console::Message(const char *const message)
{
	if (Active)
	{
		popupMessage = message;
	}
}

void SDLCALL C4Console::FileOpenCallback(void* userdata, const char* const* filelist, int filter)
{
	if (!filelist)
	{
		SDL_Log("An error occured: %s", SDL_GetError());
		return;
	}
	else if (!*filelist)
	{
		// Dialog canceled.
		return;
	}

	if(!userdata)
	{
		return;
	}

	if(C4Console* ConsoleContext = reinterpret_cast<C4Console*>(userdata))
	{
		while (*filelist)
		{
			// Compose command line
			std::string commandLine{'"'};
			commandLine.append(*filelist);
			std::replace(commandLine.begin(), commandLine.end(), '\\', '/');
			commandLine.append("\" ");

			Application.InteractiveThread.ExecuteInMainThread([ConsoleContext, CmdLine = std::move(commandLine)]
			{
				ConsoleContext->OpenGame(CmdLine.c_str());
			});
			break;
		}
	}
}

void C4Console::FileOpen()
{
	static const SDL_DialogFileFilter ScenarioFilter[] = {
		{ "Clonk 4 Scenario",  "c4s;c4f;Scenario.txt"}
	};
	SDL_ShowOpenFileDialog(FileOpenCallback, this, sdlWindow, ScenarioFilter, 1, Config.General.ExePath, false);
}

void C4Console::FileOpenWPlrs()
{
	static const SDL_DialogFileFilter ScenarioFilter[] = {
		{ "Clonk 4 Scenario",  "c4s;c4f;Scenario.txt"}
	};
	SDL_ShowOpenFileDialog(FileOpenCallback, this, sdlWindow, ScenarioFilter, 1, Config.General.ExePath, false);

	// TODO: Construct chain with file open callbacks or remove option. Since it can be done via two user commands.
	return;
	// Get scenario file name
	char c4sfile[512 + 1] = "";
	/*
	if (!FileSelect(c4sfile, 512,
		"Clonk 4 Scenario\0*.c4s;*.c4f\0\0",
		OFN_HIDEREADONLY | OFN_FILEMUSTEXIST
	)) return;
	// Get player file name(s)
	*/
	char c4pfile[4096 + 1] = "";
/*
	if (!FileSelect(c4pfile, 4096,
		"Clonk 4 Player\0*.c4p\0\0",
		OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT | OFN_EXPLORER
	)) return;
*/
	// Compose command line
	std::string commandLine{'"'};
	commandLine.append(c4sfile);
	commandLine.append("\" ");
	char cmdline[6000] = "";
	SAppend("\"", cmdline, 5999); SAppend(c4sfile, cmdline, 5999); SAppend("\" ", cmdline, 5999);
	if (DirectoryExists(c4pfile)) // Multiple players
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
	else // One player
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

// TODO: Obsolete. Remove.
#ifdef _WIN32
bool C4Console::FileSelect(char *sFilename, int iSize, const char *szFilter, DWORD    dwFlags, bool fSave)
#else
bool C4Console::FileSelect(char *sFilename, int iSize, const char *szFilter, uint32_t dwFlags, bool fSave)
#endif
{

	return true;

#if 0 //def _WIN32
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
	// TODO: Open component host dialog
	//Game.Title.Open();
}

void C4Console::EditScript()
{
	if (Game.Network.isEnabled()) return;
	// TODO: Imgui open script dialog
	//Game.Script.Open();
	Game.ScriptEngine.ReLink(&Game.Defs);
}

void C4Console::EditInfo()
{
	if (Game.Network.isEnabled()) return;
	// TODO: Imgui open info dialog
	//Game.Info.Open();
}

void C4Console::EditObjects()
{
	ObjectListDlg.Open();
}

static void SDLCALL JoinPlayerCallback(void* userdata, const char* const* filelist, int filter)
{
	if (!filelist)
	{
		SDL_Log("An error occured: %s", SDL_GetError());
		return;
	}
	else if (!*filelist)
	{
		// Dialog canceled.
		return;
	}

	while (*filelist)
	{
		SDL_Log("Full path to selected file: '%s'", *filelist);
		if (Game.Network.isEnabled())
		{
			Game.Network.Players.JoinLocalPlayer(*filelist, true);
		}
		else
		{
			Game.Players.CtrlJoinLocalNoNetwork(*filelist, Game.Clients.getLocalID(), Game.Clients.getLocalName());
		}
		filelist++;
	}
}

void C4Console::PlayerJoin()
{
	static const SDL_DialogFileFilter PlayerFilter[] = {
		{ "Clonk 4 Player",  "c4p" }
	};
	SDL_ShowOpenFileDialog(JoinPlayerCallback, nullptr, sdlWindow, PlayerFilter, 1, nullptr, true);
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
	PropertyDlg.Execute();
	//ObjectListDlg.Execute();

	Game.GraphicsSystem.Execute();
	Draw();

}

void C4Console::Draw()
{
	ImGui->Select();
	lpDDraw->FillBG();
	ImGui->NewFrame();

	const bool lobbyActive{Game.Network.isLobbyActive()};
	const bool controlsDisabled{!fGameOpen};
	const bool controlsDisabledIfNotInLobby{!lobbyActive && controlsDisabled};

	bool showAbout{false};

	ImGui::SetNextWindowPos({0, 0});
	std::int32_t x,y;
	SDL_GetWindowSizeInPixels(sdlWindow, &x, &y);
	ImGui::SetNextWindowSize({float(x), float(y)});
	ImGui::Begin("Konsolenmodus", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar);

	// TODO: Add tooltips to menu options. Ideally localized.
	ImGui::PushStyleVarY(ImGuiStyleVar_FramePadding, 5.0f);
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu(LoadResStr(C4ResStrTableKey::IDS_MNU_FILE)))
		{
			if (ImGui::MenuItem(LoadResStr(C4ResStrTableKey::IDS_MNU_OPEN))) FileOpen();
			// TODO: Disabled for now until it is properly implemented again. The only thing missing is a chain of file dialogs with their callbacks.
			//if (ImGui::MenuItem(LoadResStr(C4ResStrTableKey::IDS_MNU_OPENWPLRS))) FileOpenWPlrs();
			ImGui::Separator();

			ImGui::BeginDisabled(controlsDisabled || !Game.Players.GetCount());
			if (ImGui::MenuItem(LoadResStr(C4ResStrTableKey::IDS_MNU_SAVEGAME))) FileSave(true);
			if (ImGui::MenuItem(LoadResStr(C4ResStrTableKey::IDS_MNU_SAVEGAMEAS))) FileSaveAs(true);
			ImGui::EndDisabled();

			ImGui::Separator();

			ImGui::BeginDisabled(!Game.IsRunning || !Game.Control.IsRuntimeRecordPossible());
			if (ImGui::MenuItem(LoadResStr(C4ResStrTableKey::IDS_MNU_RECORD))) FileRecord();
			ImGui::EndDisabled();

			ImGui::Separator();

			ImGui::BeginDisabled(controlsDisabled);
			if (ImGui::MenuItem(LoadResStr(C4ResStrTableKey::IDS_MNU_SAVESCENARIO))) FileSave(false);
			if (ImGui::MenuItem(LoadResStr(C4ResStrTableKey::IDS_MNU_SAVESCENARIOAS))) FileSaveAs(false);
			ImGui::EndDisabled();

			ImGui::Separator();

			ImGui::BeginDisabled(controlsDisabled);
			if (ImGui::MenuItem(LoadResStr(C4ResStrTableKey::IDS_MNU_CLOSE))) FileClose();
			ImGui::EndDisabled();

			ImGui::EndMenu();
		}


		ImGui::BeginDisabled(controlsDisabled || !Editing);

		if (ImGui::BeginMenu(LoadResStr(C4ResStrTableKey::IDS_MNU_COMPONENTS)))
		{
			if (ImGui::MenuItem(LoadResStr(C4ResStrTableKey::IDS_MNU_SCRIPT))) EditScript();
			if (ImGui::MenuItem(LoadResStr(C4ResStrTableKey::IDS_MNU_TITLE))) EditTitle();
			if (ImGui::MenuItem(LoadResStr(C4ResStrTableKey::IDS_MNU_INFO))) EditInfo();
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(LoadResStr(C4ResStrTableKey::IDS_MNU_PLAYER)))
		{
			if (ImGui::MenuItem(LoadResStr(C4ResStrTableKey::IDS_MNU_JOIN)))
			{
				PlayerJoin();
			}

			ImGui::BeginDisabled(!Editing || (Game.Network.isEnabled() && !Game.Network.isHost()));

			const char *PLRQUITNET {LoadResStrV(C4ResStrTableKey::IDS_CNS_PLRQUITNET)};
			const char *PLRQUIT {LoadResStrV(C4ResStrTableKey::IDS_CNS_PLRQUIT)};
			const char *resString {Game.Network.isEnabled() ? PLRQUITNET : PLRQUIT};

			for (C4Player *player{Game.Players.First}; player; player = player->Next)
			{
				if (ImGui::MenuItem(std::format("{} {} {}",resString, player->GetName(), player->AtClientName).c_str()))
				{
					Game.Control.Input.Add(CID_EliminatePlayer, new C4ControlEliminatePlayer{player->Number});
				}
			}

			ImGui::EndDisabled();

			ImGui::EndMenu();
		}

		ImGui::EndDisabled();

		ImGui::BeginDisabled(controlsDisabled);
		if (ImGui::BeginMenu(LoadResStr(C4ResStrTableKey::IDS_MNU_VIEWPORT)))
		{
			if (ImGui::MenuItem(LoadResStr(C4ResStrTableKey::IDS_MNU_NEW)))
			{
				ViewportNew();
			}

			for (C4Player *player{Game.Players.First}; player; player = player->Next)
			{
				const std::string text = LoadResStrV(C4ResStrTableKey::IDS_CNS_NEWPLRVIEWPORT);
				if (ImGui::MenuItem(std::format("{}{}",text.c_str(), player->GetName()).c_str())) Game.CreateViewport(player->Number);
			}

			ImGui::EndMenu();
		}
		ImGui::EndDisabled();

		ImGui::BeginDisabled(!Game.Network.isHost() || !Game.Network.isEnabled());

		if (ImGui::BeginMenu(LoadResStr(C4ResStrTableKey::IDS_MNU_NET)))
		{
			constexpr auto removeClientMenuEntry = [](C4Client *const client, const char *const label)
			{
				if (ImGui::MenuItem(label))
				{
					Game.Clients.CtrlRemove(client, LoadResStr(C4ResStrTableKey::IDS_MSG_KICKBYMENU));
				}
			};

			removeClientMenuEntry(Game.Clients.getLocal(), LoadResStrV(C4ResStrTableKey::IDS_MNU_NETHOST));

			for (C4Network2Client *client{Game.Network.Clients.GetNextClient(nullptr)}; client; client = Game.Network.Clients.GetNextClient(client))
			{
				removeClientMenuEntry(client->getClient(), std::format("{} {} {}", LoadResStrV(client->isActivated() ? C4ResStrTableKey::IDS_MNU_NETCLIENT : C4ResStrTableKey::IDS_MNU_NETCLIENTDE), client->getName(), client->getID()).c_str());
			}
		}

		ImGui::EndDisabled();

		if (ImGui::BeginMenu("?"))
		{
			if (ImGui::MenuItem(LoadResStrV(C4ResStrTableKey::IDS_MENU_ABOUT)))
			{
				showAbout = true;
			}

			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	ImGui::PopStyleVar(1);

	ImGui::BeginChild("##log", ImVec2(0.0f, y-150.0), ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoMove);
	ImGuiListClipper LogClipper;
	static ImVec4 WarningColor = ImVec4(0.94f, 0.7f, 0.11f, 1.0f);
	static ImVec4 ErrorColor = ImVec4(0.97f, 0.23f, 0.18f, 1.0f);
	LogClipper.Begin(logBuffer.size()); // TODO: maybe use second parameter to set custom height
	while (LogClipper.Step())
	{
		for (int line_no = LogClipper.DisplayStart; line_no < LogClipper.DisplayEnd; line_no++)
		{
			// TODO: Find a way to get log type (warning, error) directly instead of comparing strings.
			if(logBuffer[line_no].starts_with("WARNING"))
			{
				ImGui::TextColored(WarningColor, logBuffer[line_no].c_str());
			}
			else if(logBuffer[line_no].starts_with("ERROR") || logBuffer[line_no].starts_with("FATAL ERROR"))
			{
				ImGui::TextColored(ErrorColor, logBuffer[line_no].c_str());
			}
			else
			{
				ImGui::DebugTextUnformattedWithLocateItem(logBuffer[line_no].c_str(), logBuffer[line_no].c_str() + logBuffer[line_no].length()-1);
			}
		}
	}
	if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
		ImGui::SetScrollHereY(1.0f);

	// TODO: Improve selection precision. There seems to be a small offset between the cursor and the actual string character.
	LogTextSelect->update();

	if (ImGui::BeginPopupContextWindow()) {
		ImGui::BeginDisabled(!LogTextSelect->hasSelection());
		if (ImGui::MenuItem("Copy", "Ctrl+C")) {
			LogTextSelect->copy();
		}
		ImGui::EndDisabled();

		if (ImGui::MenuItem("Select all", "Ctrl+A")) {
			LogTextSelect->selectAll();
		}

		if (ImGui::MenuItem("Clear selection")) {
			LogTextSelect->clearSelection();
		}
		ImGui::EndPopup();
	}

	ImGui::EndChild();


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
	ImGui::Spacing();

	ImGui::BeginDisabled(controlsDisabledIfNotInLobby || !halt);
	if (ImGui::Button("Start"))
	{
		DoPlay();
	}
	ImGui::EndDisabled();

	ImGui::SameLine();

	ImGui::BeginDisabled(controlsDisabledIfNotInLobby || halt);
	if (ImGui::Button("Pause"))
	{
		DoHalt();
	}
	ImGui::EndDisabled();

	ImGui::SameLine(0, 20);

	const ConsoleMode mode{EditCursor.GetMode()};
	ImGui::BeginDisabled(controlsDisabled);
	if (ImGui::RadioButton("Play", mode == ConsoleMode::Play))
	{
		EditCursor.SetMode(ConsoleMode::Play);
	}
	ImGui::EndDisabled();
	ImGui::SameLine();

	ImGui::BeginDisabled(controlsDisabled || !Editing);
	if (ImGui::RadioButton("Edit", mode == ConsoleMode::Edit))
	{
		EditCursor.SetMode(ConsoleMode::Edit);
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("Draw", mode == ConsoleMode::Draw))
	{
		EditCursor.SetMode(ConsoleMode::Draw);
	}
	ImGui::EndDisabled();

	ImGui::PopStyleVar();
	ImGui::Spacing();
	ImGui::Separator();

	ImGuiOldColumnFlags ColumnFlags = ImGuiOldColumnFlags_NoResize;
	ImGui::BeginColumns("##meta", 4, ColumnFlags);
	ImGui::Text("Frame: %d", Game.FrameCounter);
	ImGui::NextColumn();
	ImGui::Text("Script: %d", Game.Script.Counter);
	ImGui::NextColumn();
	ImGui::Text("%02d:%02d:%02d", Game.Time / 3600, (Game.Time % 3600) / 60, Game.Time % 60);
	ImGui::NextColumn();
	ImGui::Text("%i FPS", Game.FPS);
	ImGui::SetColumnWidth(0, 150);
	ImGui::EndColumns();

	ImGui::Spacing();

	const StdStrBuf cursorText{EditCursor.GetStatusBarText()};
	if (!cursorText.isNull())
	{
		ImGui::Text("Cursor: %s", cursorText.isNull() ? "" : cursorText.getData());
	}

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
	SetCaption(LoadResStr(C4ResStrTableKey::IDS_CNS_CONSOLE));
}

bool C4Console::TogglePause()
{
	return Game.TogglePause();
}
