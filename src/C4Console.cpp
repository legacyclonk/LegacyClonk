/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2024, The LegacyClonk Team and contributors
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

#include "imgui.h"
#include "ImGuiTextselect.hpp"

#include "res/DeveloperModeIcons.h"


C4Console::C4Console()
{
	Active = false;
	Editing = true;
	fGameOpen = false;

	logTextSelect = new TextSelect{std::bind(&C4Console::GetLineAtIdx, this, std::placeholders::_1), std::bind(&C4Console::GetNumLines, this)};
}

C4Console::~C4Console()
{
	delete logTextSelect;
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
	if (imGui)
	{
		imGui->Select();
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

bool C4Console::Init(CStdApp *app, const char *title, const C4Rect &bounds, CStdWindow *parent, std::uint32_t additionalFlags, std::int32_t minWidth, std::int32_t minHeight)
{
	// Active
	Active = true;
	// Editing (enable even if network)
	Editing = true;
	// Create dialog window
	return CStdWindow::Init(app, title, bounds, parent, additionalFlags, minWidth, minHeight);
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
	if(lpDDraw)
	{
		toolCursorImage = lpDDraw->LoadPNGFromMemory(developerModeCursorImage, developerModeCursorImageLength);
		toolMouseImage = lpDDraw->LoadPNGFromMemory(developerModeMouseImage, developerModeMouseImageLength);
		toolBrushImage = lpDDraw->LoadPNGFromMemory(developerModeBrushImage, developerModeBrushImageLength);
		playImage = lpDDraw->LoadPNGFromMemory(developerModePlayImage, developerModePlayImageLength);
		haltImage = lpDDraw->LoadPNGFromMemory(developerModeHaltImage, developerModeHaltImageLength);
	}

	InitImGui();
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
	if (logBuffer.size() > logBufferSize)
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
	bool fOkay{true};

	ImGui::SetMouseCursor(ImGuiMouseCursor_Wait);

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

	ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

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
	{
		if (!Game.C4S.Head.SaveGame)
		{
			Message(LoadResStr(C4ResStrTableKey::IDS_CNS_NOGAMEOVERSCEN));
			return;
		}
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

	if(C4Console *consoleContext = reinterpret_cast<C4Console*>(userdata))
	{
		std::string Filename = *filelist;
		consoleContext->FileSaveMainThread(Filename, false);
	}
}

void C4Console::FileSaveMainThread(std::string filename, bool fSaveGame)
{
	Application.InteractiveThread.ExecuteInMainThread([this, filename, fSaveGame] mutable
	{
		std::replace(filename.begin(), filename.end(), '\\', '/');
		if(!filename.ends_with(".c4s"))
		{
			filename.append(".c4s");
		}
		bool fOkay = true;
		// Close current scenario file
		if (!Game.ScenarioFile.Close()) fOkay = false;
		// Copy current scenario file to target
		if (!C4Group_CopyItem(Game.ScenarioFilename, filename.c_str())) fOkay = false;
		// Open new scenario file
		SCopy(filename.c_str(), Game.ScenarioFilename);

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

	if(C4Console *consoleContext = reinterpret_cast<C4Console*>(userdata))
	{
		while (*filelist)
		{
			// Compose command line
			std::string commandLine{'"'};
			commandLine.append(*filelist);
			std::replace(commandLine.begin(), commandLine.end(), '\\', '/');
			commandLine.append("\" ");

			Application.InteractiveThread.ExecuteInMainThread([consoleContext, CmdLine = std::move(commandLine)]
			{
				consoleContext->OpenGame(CmdLine.c_str());
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
				Message(std::format("File \"{}\" does not exist", filename).c_str(), false);
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
		std::string filepath{*filelist};
		std::replace(filepath.begin(), filepath.end(), '\\', '/');
		if (Game.Network.isEnabled())
		{
			Game.Network.Players.JoinLocalPlayer(filepath.c_str(), true);
		}
		else
		{
			Game.Players.CtrlJoinLocalNoNetwork(filepath.c_str(), Game.Clients.getLocalID(), Game.Clients.getLocalName());
		}
		filelist++;
	}
}

void C4Console::PlayerJoin()
{
	static const SDL_DialogFileFilter PlayerFilter[] = {
		{ "Clonk 4 Player",  "c4p" }
	};
	SDL_ShowOpenFileDialog(JoinPlayerCallback, nullptr, sdlWindow, PlayerFilter, 1, Config.General.ExePath, true);
}

void C4Console::SetCaption(const char *szCaption)
{
	if (!Active) return;
#ifdef _WIN32
	// Sorry, the window caption needs to be constant so
	// the window can be found using FindWindow
	SetTitle(C4ENGINECAPTION); // TODO: Still the case?
#else
	SetTitle(szCaption);
#endif
}

void C4Console::Execute()
{
	EditCursor.Execute();
	PropertyDlg.Execute();

	Game.GraphicsSystem.Execute();
	Draw();
}

int C4Console::TextEditCallbackStub(ImGuiInputTextCallbackData *data)
{
	C4Console *console{reinterpret_cast<C4Console*>(data->UserData)};
	return console->TextEditCallback(data);
}

// TODO: Reference the implementation from ImGui Console demo that uses input history and text completion.
int C4Console::TextEditCallback(ImGuiInputTextCallbackData *data)
{
    return 0;
}

void C4Console::Draw()
{
	imGui->Select();
	lpDDraw->FillBG();
	imGui->NewFrame();

	const bool lobbyActive{Game.Network.isLobbyActive()};
	const bool controlsDisabled{!fGameOpen};
	const bool controlsDisabledIfNotInLobby{!lobbyActive && controlsDisabled};

	bool showAbout{false};

	ImGui::SetNextWindowPos({0, 0});
	std::int32_t x, y;
	SDL_GetWindowSizeInPixels(sdlWindow, &x, &y);
	ImGui::SetNextWindowSize({float(x), float(y)});
	ImGui::Begin("##Console", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar);

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
			if (ImGui::MenuItem(LoadResStr(C4ResStrTableKey::IDS_MNU_SCRIPT)))
			{
				EditScript();
			}
			if (ImGui::MenuItem(LoadResStr(C4ResStrTableKey::IDS_MNU_TITLE)))
			{
				EditTitle();
			}
			if (ImGui::MenuItem(LoadResStr(C4ResStrTableKey::IDS_MNU_INFO)))
			{
				EditInfo();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(LoadResStr(C4ResStrTableKey::IDS_MNU_PLAYER)))
		{
			if (ImGui::MenuItem(LoadResStr(C4ResStrTableKey::IDS_MNU_JOIN)))
			{
				PlayerJoin();
			}

			ImGui::BeginDisabled(!Editing || (Game.Network.isEnabled() && !Game.Network.isHost()));

			const char *plrQuitNet{LoadResStrV(C4ResStrTableKey::IDS_CNS_PLRQUITNET)};
			const char *plrQuit{LoadResStrV(C4ResStrTableKey::IDS_CNS_PLRQUIT)};
			StdStrBuf resString{Game.Network.isEnabled() ? plrQuitNet : plrQuit};
			resString.Replace("%s", "{}");
			for (C4Player *player{Game.Players.First}; player; player = player->Next)
			{
				std::string playerName{player->GetName()};
				std::string playerClient{player->AtClientName};
				if (ImGui::MenuItem(std::vformat(resString.getData(), std::make_format_args(playerName, playerClient)).c_str()))
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
				StdStrBuf newPlayerViewportFormat{LoadResStrV(C4ResStrTableKey::IDS_CNS_NEWPLRVIEWPORT)};
				newPlayerViewportFormat.Replace("%s", "{}");
				std::string playerName{player->GetName()};
				if (ImGui::MenuItem(std::vformat(newPlayerViewportFormat.getData(), std::make_format_args(playerName)).c_str()))
				{
					Game.CreateViewport(player->Number);
				}
			}

			ImGui::EndMenu();
		}
		ImGui::EndDisabled();

		if (Game.Network.isEnabled() && Game.Network.isHost())
		{
			if (ImGui::BeginMenu(LoadResStr(C4ResStrTableKey::IDS_MNU_NET)))
			{
				constexpr auto removeClientMenuEntry = [](C4Client *const client, const char *const label)
				{
					if (ImGui::MenuItem(label))
					{
						Game.Clients.CtrlRemove(client, LoadResStr(C4ResStrTableKey::IDS_MSG_KICKBYMENU));
					}
				};

				for (C4Network2Client *client{Game.Network.Clients.GetNextClient(nullptr)}; client; client = Game.Network.Clients.GetNextClient(client))
				{
					StdStrBuf clientFormat;
					if (client->isHost())
					{
						clientFormat = LoadResStrV(C4ResStrTableKey::IDS_MNU_NETHOST);
					}
					else if(client->isActivated())
					{
						clientFormat = LoadResStrV(C4ResStrTableKey::IDS_MNU_NETCLIENT);
					}
					else
					{
						clientFormat = LoadResStrV(C4ResStrTableKey::IDS_MNU_NETCLIENTDE);
					}
					clientFormat.Replace("%s", "{}");
					clientFormat.Replace("%i", "{}");

					const std::string clientName{client->getName()};
					const std::string clientId{std::to_string(client->getID())};
					removeClientMenuEntry(client->getClient(), std::vformat(clientFormat.getData(), std::make_format_args( clientName, clientId)).c_str());
				}

				ImGui::EndMenu();
			}
		}

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

	ImGui::BeginChild("##log", ImVec2(0.0f, y - 150.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoMove);
	ImGuiListClipper logClipper;
	const ImVec4 warningColor{ImVec4(0.94f, 0.7f, 0.11f, 1.0f)};
	const ImVec4 errorColor{ImVec4(0.97f, 0.23f, 0.18f, 1.0f)};
	logClipper.Begin(logBuffer.size()); // TODO: maybe use second parameter to set custom height
	while (logClipper.Step())
	{
		for (std::int32_t lineNo = logClipper.DisplayStart; lineNo < logClipper.DisplayEnd; lineNo++)
		{
			// TODO: Find a way to get log type (warning, error) directly instead of comparing strings.
			if(logBuffer[lineNo].contains("WARNING"))
			{
				ImGui::TextColored(warningColor, "%s", logBuffer[lineNo].c_str());
			}
			else if(logBuffer[lineNo].contains("ERROR"))
			{
				ImGui::TextColored(errorColor, "%s", logBuffer[lineNo].c_str());
			}
			else
			{
				ImGui::Text("%s", logBuffer[lineNo].c_str());
			}
		}
	}
	if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
	{
		ImGui::SetScrollHereY(1.0f);
	}

	// TODO: Improve selection precision. There seems to be a small offset between the cursor and the actual string character.
	logTextSelect->update();

	if (ImGui::BeginPopupContextWindow())
	{
		ImGui::BeginDisabled(!logTextSelect->hasSelection());
		if (ImGui::MenuItem("Copy", "Ctrl+C"))
		{
			logTextSelect->copy();
		}
		ImGui::EndDisabled();

		if (ImGui::MenuItem("Select all", "Ctrl+A"))
		{
			logTextSelect->selectAll();
		}

		if (ImGui::MenuItem("Clear selection"))
		{
			logTextSelect->clearSelection();
		}
		ImGui::EndPopup();
	}

	ImGui::EndChild();


	// Command-line
	float commandLineWidth{ImGui::GetContentRegionAvail().x};
	ImGui::SetNextItemWidth(commandLineWidth - 30);
	bool reclaimFocus{false};
	static char inputBuf[512];
	ImGuiInputTextFlags inputTextFlags{ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_ElideLeft};
	if (ImGui::InputText("", inputBuf, IM_COUNTOF(inputBuf), inputTextFlags, &TextEditCallbackStub, (void*)this))
	{
		StdStrBuf s{&inputBuf[0]};
		s.TrimSpaces();
		if (s[0])
		{
			In(s.getData());
		}
		strcpy(inputBuf, "");
		reclaimFocus = true;
	}

	// Auto-focus on window apparition
	ImGui::SetItemDefaultFocus();
	if (reclaimFocus)
	{
		ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
	}

	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::BeginDisabled(controlsDisabled);

	const auto addFunctionEntry = [this](C4AulFunc *const func)
	{
		if (ImGui::Selectable(std::string{func->Name}.append("()").c_str(), selectedFunction == func))
		{
			selectedFunction = func;
			SAppend(func->Name, inputBuf);
			SAppend("()", inputBuf);
		}
	};

	ImGui::SameLine();

	if (ImGui::BeginCombo("##funcselector", selectedFunction ? selectedFunction->Name : nullptr, ImGuiComboFlags_NoPreview))
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

	ImGui::EndDisabled();
	ImGui::Spacing();
	ImGui::BeginDisabled(controlsDisabledIfNotInLobby || !halt);
	if (ImGui::ImageButton("#Play", {playImage}, {16, 16}))
	{
		DoPlay();
	}
	ImGui::EndDisabled();

	ImGui::SameLine();

	ImGui::BeginDisabled(controlsDisabledIfNotInLobby || halt);
	if (ImGui::ImageButton("#Pause", {haltImage}, {16, 16}))
	{
		DoHalt();
	}
	ImGui::EndDisabled();

	ImGui::SameLine(0, 20);

	auto CreateSelectedButton = [this] (ConsoleMode thisMode, std::uint32_t imageId, bool isDisabled)
	{
		const std::uint32_t imageSize{16};
		ImGui::BeginDisabled(isDisabled);
		ImGui::PushID(imageId);
		if(ImGui::ImageButton("", {imageId}, {imageSize,imageSize}))
		{
			EditCursor.SetMode(thisMode);
		}

		if(thisMode == EditCursor.GetMode())
		{
			ImGui::GetWindowDrawList()->AddRect(
			ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
			IM_COL32(30, 200, 247, 255),
			2.0f, 0, 3.0f);
		}

		ImGui::PopID();
		ImGui::EndDisabled();
	};

	CreateSelectedButton(ConsoleMode::Play, toolCursorImage, controlsDisabled);
	ImGui::SameLine();
	CreateSelectedButton(ConsoleMode::Edit, toolMouseImage, controlsDisabled || !Editing);
	ImGui::SameLine();
	CreateSelectedButton(ConsoleMode::Draw, toolBrushImage, controlsDisabled || !Editing);

	ImGui::PopStyleVar();
	ImGui::Spacing();
	ImGui::Separator();

	ImGuiOldColumnFlags columnFlags{ImGuiOldColumnFlags_NoResize};
	ImGui::BeginColumns("##meta", 4, columnFlags);
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

	imGui->Render();

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
