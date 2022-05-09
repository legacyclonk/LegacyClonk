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

/* Main class to initialize configuration and execute the game */

#include <C4Include.h>
#include <C4Application.h>
#include <C4Console.h>
#include <C4FullScreen.h>
#include <C4GameLobby.h>
#include <C4GamePadCon.h>
#include <C4Language.h>
#include <C4Log.h>
#include <C4Startup.h>
#include <C4Version.h>

#ifdef _WIN32
#include <C4UpdateDlg.h>

#include "StdStringEncodingConverter.h"
#include <StdRegistry.h>
#endif

#include <cassert>
#include <stdexcept>

constexpr unsigned int defaultGameTickDelay = 16;

C4Sec1TimerCallbackBase::C4Sec1TimerCallbackBase() : pNext(nullptr), iRefs(2)
{
	// register into engine callback stack
	Application.AddSec1Timer(this);
}

C4Application::C4Application() :
	isFullScreen(true), UseStartupDialog(true), launchEditor(false), restartAtEnd(false),
	DDraw(nullptr), AppState(C4AS_None),
	iLastGameTick(0), iGameTickDelay(defaultGameTickDelay), iExtraGameTickDelay(0), pGamePadControl(nullptr),
	CheckForUpdates(false) {}

C4Application::~C4Application()
{
	// clear gamepad
	delete pGamePadControl;
	// Close log
	CloseLog();
	// Launch editor
	if (launchEditor)
	{
#ifdef _WIN32
		char strCommandLine[_MAX_PATH + 1]; SCopy(Config.AtExePath(C4CFN_Editor), strCommandLine);
		STARTUPINFO StartupInfo{};
		StartupInfo.cb = sizeof(StartupInfo);
		PROCESS_INFORMATION ProcessInfo{};
		CreateProcess(nullptr, strCommandLine, nullptr, nullptr, TRUE, 0, nullptr, nullptr, &StartupInfo, &ProcessInfo);
#endif
	}
}

void C4Application::DoInit()
{
	assert(AppState == C4AS_None);
	// Config overwrite by parameter
	StdStrBuf sConfigFilename;
	char szParameter[_MAX_PATH + 1];
	for (int32_t iPar = 0; SGetParameter(GetCommandLine(), iPar, szParameter, _MAX_PATH); iPar++)
		if (SEqual2NoCase(szParameter, "/config:"))
			sConfigFilename.Copy(szParameter + 8);
	// Config check
	Config.Init();
	Config.Load(true, sConfigFilename.getData());
	Config.Save();
	// sometimes, the configuration can become corrupted due to loading errors or w/e
	// check this and reset defaults if necessary
	if (Config.IsCorrupted())
	{
		if (sConfigFilename)
		{
			// custom config corrupted: Fail
			throw StartupException{"Warning: Custom configuration corrupted - program abort!"};
		}
		else
		{
			// default config corrupted: Restore default
			Log("Warning: Configuration corrupted - restoring default!\n");
			Config.Default();
			Config.Save();
			Config.Load();
		}
	}
	MMTimer = Config.General.MMTimer;
	// Init C4Group
	C4Group_SetMaker(Config.General.Name);
	C4Group_SetProcessCallback(&ProcessCallback);
	C4Group_SetTempPath(Config.General.TempPath);
	C4Group_SetSortList(C4CFN_FLS);

	// Open log
	OpenLog();

	// init system group
	if (!SystemGroup.Open(C4CFN_System))
	{
		// Error opening system group - no LogFatal, because it needs language table.
		// This will *not* use the FatalErrors stack, but this will cause the game
		// to instantly halt, anyway.
		throw StartupException{"Error opening system group file (System.c4g)!"};
	}

	// Language override by parameter
	const char *pLanguage;
	if (pLanguage = SSearchNoCase(GetCommandLine(), "/Language:"))
		SCopyUntil(pLanguage, Config.General.LanguageEx, ' ', CFG_MaxString);

	// Init external language packs
	Languages.Init();
	// Load language string table
	if (!Languages.LoadLanguage(Config.General.LanguageEx))
		// No language table was loaded - bad luck...
		if (!IsResStrTableLoaded())
			Log("WARNING: No language string table loaded!");

	// Parse command line
	Game.ParseCommandLine(GetCommandLine());

#ifdef WIN32
	// Windows: handle incoming updates directly, even before starting up the gui
	//          because updates will be applied in the console anyway.
	if (Application.IncomingUpdate)
		if (C4UpdateDlg::ApplyUpdate(Application.IncomingUpdate.getData(), false, nullptr))
			return;
#endif

	// activate
	Active = true;

	// Init carrier window
	if (isFullScreen)
	{
		if (!(pWindow = FullScreen.Init(this)))
		{
			Clear(); return;
		}
		pWindow->SetSize(static_cast<int32_t>(Config.Graphics.ResX * GetScale()), static_cast<int32_t>(Config.Graphics.ResY * GetScale()));
		SetDisplayMode(Config.Graphics.UseDisplayMode);

#ifdef _WIN32
		if (Config.Graphics.UseDisplayMode == DisplayMode::Window)
		{
			if (Config.Graphics.Maximized) pWindow->Maximize();
			else pWindow->SetPosition(Config.Graphics.PositionX, Config.Graphics.PositionY);
		}
#endif
	}
	else
	{
		if (!(pWindow = Console.Init(this)))
		{
			Clear(); return;
		}
	}

	// init timers (needs window)
	if (!InitTimer())
	{
		LogFatal(LoadResStr("IDS_ERR_TIMER"));
		Clear(); throw StartupException{GetFatalError()};
	}

	// Engine header message
	Log(C4ENGINEINFOLONG);
	LogF("Version: %s %s", C4VERSION, C4_OS);

	// Initialize OpenGL
	DDraw = DDrawInit(this, Config.Graphics.Engine);
	if (!DDraw) { LogFatal(LoadResStr("IDS_ERR_DDRAW")); Clear(); throw StartupException{GetFatalError()}; }

#if defined(_WIN32) && !defined(USE_CONSOLE)
	// Register clonk file classes - notice: this will only work if we have administrator rights
	SetFileClasses();
#endif

	// Initialize gamepad
	if (!pGamePadControl && Config.General.GamepadEnabled)
		pGamePadControl = new C4GamePadControl();

	AppState = C4AS_PreInit;
}

bool C4Application::PreInit()
{
	SetGameTickDelay(defaultGameTickDelay);

	if (!Game.PreInit()) return false;

	// startup dialog: Only use if no next mission has been provided
	bool fDoUseStartupDialog = UseStartupDialog && !*Game.ScenarioFilename;

	// init loader: default spec
	if (fDoUseStartupDialog)
	{
		if (!Game.GraphicsSystem.InitLoaderScreen(C4CFN_StartupBackgroundMain))
		{
			LogFatal(LoadResStr("IDS_PRC_ERRLOADER")); return false;
		}
	}

	Game.SetInitProgress(fDoUseStartupDialog ? 10.0f : 1.0f);

#ifdef ENABLE_SOUND
	try
	{
		if (!AudioSystem)
		{
			AudioSystem.reset(C4AudioSystem::NewInstance(Config.Sound.MaxChannels));
		}
	}
	catch (const std::runtime_error &e)
	{
		Log(e.what());
		Log(LoadResStr("IDS_PRC_NOAUDIO"));
	}
#endif

	// Music
	MusicSystem.emplace();

	Game.SetInitProgress(fDoUseStartupDialog ? 20.0f : 2.0f);

	// Sound
	SoundSystem.emplace();

	// Toasts
	if (!ToastSystem)
	{
		ToastSystem.reset(C4ToastSystem::NewInstance());
	}

	Game.SetInitProgress(fDoUseStartupDialog ? 30.0f : 3.0f);

	AppState = fDoUseStartupDialog ? C4AS_Startup : C4AS_StartGame;

	return true;
}

bool C4Application::ProcessCallback(const char *szMessage, int iProcess)
{
	Console.Out(szMessage);
	return true;
}

void C4Application::Clear()
{
	Game.Clear();
	NextMission.Clear();
	// close system group (System.c4g)
	SystemGroup.Close();
	// Close timers
	sec1TimerCallbacks.clear();
	// Log
	if (IsResStrTableLoaded()) // Avoid (double and undefined) message on (second?) shutdown...
		Log(LoadResStr("IDS_PRC_DEINIT"));
	// Clear external language packs and string table
	Languages.Clear();
	Languages.ClearLanguage();
	// gamepad clear
	delete pGamePadControl; pGamePadControl = nullptr;
	// music system clear
	SoundSystem.reset();
	MusicSystem.reset();
	AudioSystem.reset();
	ToastSystem.reset();
	// Clear direct draw (late, because it's needed for e.g. Log)
	delete DDraw; DDraw = nullptr;
	// Close window
	FullScreen.Clear();
	Console.Clear();
	// The very final stuff
	CStdApp::Clear();
}

bool C4Application::OpenGame()
{
	if (isFullScreen)
	{
		// Open game
		return Game.Init();
	}
	else
	{
		// Execute command line
		if (Game.ScenarioFilename[0] || Game.DirectJoinAddress[0])
			return Console.OpenGame(szCmdLine);
	}
	// done; success
	return true;
}

void C4Application::Quit()
{
	// Clear definitions passed by frontend for this round
	Config.General.Definitions[0] = 0;
#ifdef _WIN32
	if (pWindow)
	{
		// store if window is maximized and where it is positioned
		WINDOWPLACEMENT placement;
		GetWindowPlacement(pWindow->hWindow, &placement);
		Config.Graphics.Maximized = placement.showCmd == SW_SHOWMAXIMIZED;
		Config.Graphics.PositionX = placement.rcNormalPosition.left;
		Config.Graphics.PositionY = placement.rcNormalPosition.top;
	}
#endif
	// Save config if there was no loading error
	if (Config.fConfigLoaded) Config.Save();
	// quit app
	CStdApp::Quit();
	AppState = C4AS_Quit;
}

void C4Application::QuitGame()
{
	// reinit desired? Do restart
	if (UseStartupDialog || NextMission)
	{
		// backup last start params
		bool fWasNetworkActive = Game.NetworkActive;
		StdStrBuf password;
		if (fWasNetworkActive) password.Copy(Game.Network.GetPassword());
		// the rest isn't changed by Clear()
		decltype(Game.DefinitionFilenames) defs{Game.DefinitionFilenames};
		// stop game
		Game.Clear();
		Game.Default();
		AppState = C4AS_PreInit;
		// if a next mission is desired, set to start it
		if (NextMission)
		{
			SCopy(NextMission.getData(), Game.ScenarioFilename, _MAX_PATH);
			SReplaceChar(Game.ScenarioFilename, '\\', DirSep[0]); // linux/mac: make sure we are using forward slashes
			Game.fLobby = Game.NetworkActive = fWasNetworkActive;
			if (fWasNetworkActive) Game.Network.SetPassword(password.getData());
			Game.DefinitionFilenames = defs;
			Game.FixedDefinitions = true;
			Game.fObserve = false;
			NextMission.Clear();
		}
	}
	else
	{
		Quit();
	}
}

void C4Application::Execute()
{
	CStdApp::Execute();
	// Recursive execution check
	static int32_t iRecursionCount = 0;
	++iRecursionCount;
	// Exec depending on game state
	switch (AppState)
	{
	case C4AS_Quit:
		// Do nothing, HandleMessage will return HR_Failure soon
		break;
	case C4AS_PreInit:
		if (!PreInit()) Quit();
		break;
	case C4AS_Startup:
		if (pWindow)
		{
			pWindow->SetProgress(100);
		}

#ifdef USE_CONSOLE
		// Console engines just stay in this state until aborted or new commands arrive on stdin
#else
		AppState = C4AS_Game;
		// if no scenario or direct join has been specified, get game startup parameters by startup dialog
		Game.Parameters.ScenarioTitle.CopyValidated(LoadResStr("IDS_PRC_INITIALIZE"));
		if (!C4Startup::Execute()) { Quit(); --iRecursionCount; return; }
		AppState = C4AS_StartGame;
#endif
		break;
	case C4AS_StartGame:
		// immediate progress to next state; OpenGame will enter HandleMessage-loops in startup and lobby!
		AppState = C4AS_Game;
		// first-time game initialization
		if (!OpenGame())
		{
			// set error flag (unless this was a lobby user abort)
			if (!C4GameLobby::UserAbort)
				Game.fQuitWithError = true;
			// no start: Regular QuitGame; this may reset the engine to startup mode if desired
			QuitGame();
		}
		break;
	case C4AS_Game:
	{
		uint32_t iThisGameTick = timeGetTime();
		// Game (do additional timing check)
		if (Game.IsRunning && iRecursionCount <= 1) if (Game.GameGo || !iExtraGameTickDelay || (iThisGameTick > iLastGameTick + iExtraGameTickDelay))
		{
			// Execute
			Game.Execute();
			// Save back time
			iLastGameTick = iThisGameTick;
		}
		// Graphics
		if (!Game.DoSkipFrame)
		{
			uint32_t iPreGfxTime = timeGetTime();
			// Fullscreen mode
			if (isFullScreen)
				FullScreen.Execute();
			// Console mode
			else
				Console.Execute();
			// Automatic frame skip if graphics are slowing down the game (skip max. every 2nd frame)
			Game.DoSkipFrame = Game.Parameters.AutoFrameSkip && ((iPreGfxTime + iGameTickDelay) < timeGetTime());
		}
		else
			Game.DoSkipFrame = false;
		// Sound
		SoundSystem->Execute();
		// Gamepad
		if (pGamePadControl) pGamePadControl->Execute();
		break;
	}
	case C4AS_None:
		assert(!"Unhandled switch case");
	}

	--iRecursionCount;
}

void C4Application::OnNetworkEvents()
{
	InteractiveThread.ProcessEvents();
}

void C4Application::DoSec1Timers()
{
	for (auto it = sec1TimerCallbacks.begin(); it != sec1TimerCallbacks.end(); )
	{
		if ((*it)->IsReleased())
		{
			it = sec1TimerCallbacks.erase(it);
		}
		else
		{
			(*it++)->OnSec1Timer();
		}
	}
}

void C4Application::SetGameTickDelay(int iDelay)
{
	// Remember delay
	iGameTickDelay = iDelay;
	// Smaller than minimum refresh delay?
	if (iDelay < Config.Graphics.MaxRefreshDelay)
	{
		// Set critical timer
		ResetTimer(iDelay);
		// No additional breaking needed
		iExtraGameTickDelay = 0;
	}
	else
	{
		// Do some magic to get as near as possible to the requested delay
		int iGraphDelay = (std::max)(1, iDelay);
		iGraphDelay /= (iGraphDelay + Config.Graphics.MaxRefreshDelay - 1) / Config.Graphics.MaxRefreshDelay;
		// Set critical timer
		ResetTimer(iGraphDelay);
		// Slow down game tick
		iExtraGameTickDelay = iDelay - iGraphDelay / 2;
	}
}

bool C4Application::SetResolution(int32_t iNewResX, int32_t iNewResY)
{
	const auto scale = GetScale();
	iNewResX = static_cast<int32_t>(ceilf(iNewResX / scale));
	iNewResY = static_cast<int32_t>(ceilf(iNewResY / scale));

	if (iNewResX != Config.Graphics.ResX || iNewResY != Config.Graphics.ResY)
	{
		Config.Graphics.ResX = iNewResX;
		Config.Graphics.ResY = iNewResY;

		// ask graphics system to change it
		if (lpDDraw) lpDDraw->OnResolutionChanged();
		// notify game
		Game.OnResolutionChanged();
	}
	return true;
}

bool C4Application::SetGameFont(const char *szFontFace, int32_t iFontSize)
{
#ifndef USE_CONSOLE
	// safety
	if (!szFontFace || !*szFontFace || iFontSize < 1 || SLen(szFontFace) >= static_cast<int>(sizeof(Config.General.RXFontName))) return false;
	// first, check if the selected font can be created at all
	// check regular font only - there's no reason why the other fonts couldn't be created
	CStdFont TestFont;
	if (!Game.FontLoader.InitFont(TestFont, szFontFace, C4FontLoader::C4FT_Main, iFontSize, &Game.GraphicsResource.Files))
		return false;
	// OK; reinit all fonts
	StdStrBuf sOldFont; sOldFont.Copy(Config.General.RXFontName);
	int32_t iOldFontSize = Config.General.RXFontSize;
	SCopy(szFontFace, Config.General.RXFontName);
	Config.General.RXFontSize = iFontSize;
	if (!Game.GraphicsResource.InitFonts() || (C4Startup::Get() && !C4Startup::Get()->Graphics.InitFonts()))
	{
		// failed :o
		// shouldn't happen. Better restore config.
		SCopy(sOldFont.getData(), Config.General.RXFontName);
		Config.General.RXFontSize = iOldFontSize;
		return false;
	}
#endif
	// save changes
	return true;
}

void C4Application::AddSec1Timer(C4Sec1TimerCallbackBase *callback)
{
	sec1TimerCallbacks.emplace_back(callback);
}

void C4Application::OnCommand(const char *szCmd)
{
	// Find parameters
	const char *szPar = strchr(szCmd, ' ');
	while (szPar && *szPar == ' ') szPar++;

	if (SEqual(szCmd, "/quit"))
	{
		Quit();
		return;
	}

	switch (AppState)
	{
	case C4AS_Startup:
		// Open a new game
		if (SEqual2(szCmd, "/open "))
		{
			// Try to start the game with given parameters
			AppState = C4AS_Game;
			Game.ParseCommandLine(szPar);
			UseStartupDialog = true;
			if (!Game.Init())
				AppState = C4AS_Startup;
		}

		break;

	case C4AS_Game:
		// Clear running game
		if (SEqual(szCmd, "/close"))
		{
			Game.Clear();
			Game.Default();
			AppState = C4AS_PreInit;
			UseStartupDialog = true;
			return;
		}

		// Lobby commands
		if (Game.Network.isLobbyActive())
		{
			if (SEqual2(szCmd, "/start"))
			{
				// timeout given?
				int32_t iTimeout = Config.Lobby.CountdownTime;
				if (!Game.Network.isHost())
					Log(LoadResStr("IDS_MSG_CMD_HOSTONLY"));
				else if (szPar && (!sscanf(szPar, "%d", &iTimeout) || iTimeout < 0))
					Log(LoadResStr("IDS_MSG_CMD_START_USAGE"));
				else
					// start new countdown (aborts previous if necessary)
					Game.Network.StartLobbyCountdown(iTimeout);
				break;
			}
		}

		// Normal commands
		Game.MessageInput.ProcessInput(szCmd);
		break;

	case C4AS_None: case C4AS_PreInit: case C4AS_StartGame: case C4AS_Quit:
		assert(!"Unhandled switch case");
	}
}

void C4Application::Activate()
{
#ifdef WIN32
	// Activate the application to regain focus if it has been lost during loading.
	// As this is officially not possible any more in new versions of Windows
	// (BringWindowTopTop alone won't have any effect if the calling process is
	// not in the foreground itself), we are using an ugly OS hack.
	DWORD nForeThread = GetWindowThreadProcessId(GetForegroundWindow(), 0);
	DWORD nAppThread = GetCurrentThreadId();
	if (nForeThread != nAppThread)
	{
		AttachThreadInput(nForeThread, nAppThread, TRUE);
		BringWindowToTop(FullScreen.hWindow);
		ShowWindow(FullScreen.hWindow, SW_SHOW);
		AttachThreadInput(nForeThread, nAppThread, FALSE);
	}
	else
	{
		BringWindowToTop(FullScreen.hWindow);
		ShowWindow(FullScreen.hWindow, SW_SHOW);
	}
#endif
}

void C4Application::SetNextMission(const char *szMissionFilename)
{
	// set next mission if any is desired
	if (szMissionFilename)
		NextMission.Copy(szMissionFilename);
	else
		NextMission.Clear();
}

#if defined(_WIN32) && !defined(USE_CONSOLE)
namespace
{
	constexpr auto C4FileClassContentType = L"application/vnd.clonk.c4group";

	bool SetProtocol(std::wstring_view protocol, std::wstring_view command, std::wstring_view module)
	{
		if (!SetRegClassesRoot(protocol, L"", L"URL: Protocol")) return false;
		if (!SetRegClassesRoot(protocol, L"URL Protocol", L"")) return false;

		if (!SetRegClassesRoot(std::wstring{command} + L"\\shell\\open\\command", L"", std::wstring{command} + module.data())) return false;

		if (!SetRegClassesRoot(std::wstring{protocol} + L"\\DefaultIcon", L"", std::wstring{module} + L",1")) return false;

		return true;
	}

	std::wstring Escape(std::wstring_view str)
	{
		std::wstring result;
		result.reserve(str.size());

		for (const auto &c : str)
		{
			if (c == L'"')
			{
				result += L'\\';
			}

			result += c;
		}

		return result;
	}

	template<typename... Args>
	std::wstring WithArguments(Args... args)
	{
		std::wstring result;
		(result.append(L"\"").append(args).append(L"\""), ...);
		return result;
	}

	template<typename... Args>
	std::wstring WithCommands(std::wstring_view enginePath, Args... args)
	{
		std::wstring result{L"\""};
		result.append(Escape(enginePath));
		result.append(L"\"");

		result.append(WithArguments(args...));

		result.append(L" \"%1\"");
		return result;
	}

	struct ShellCommand
	{
		std::wstring_view Name;
		std::wstring Label;
		std::wstring Arguments;
		std::wstring IconPath;

		bool Apply(std::wstring_view enginePath, const bool makeDefault = false) const
		{
			return SetRegShell(L"Clonk4.Scenario", Name.data(), Label.data(), WithCommands(enginePath, Arguments).c_str(), IconPath, makeDefault);
		}
	};

	static std::wstring LoadResStrW(const char *const name)
	{
		return StdStringEncodingConverter{}.WinAcpToUtf16(LoadResStrNoAmp(name));
	}
}

bool C4Application::SetFileClasses()
{
	std::array<wchar_t, _MAX_PATH + 1> enginePathBuffer;
	GetModuleFileNameW(nullptr, enginePathBuffer.data(), _MAX_PATH);

	std::wstring enginePath{enginePathBuffer.data()};
	std::wstring defaultShellCommand{StdStringEncodingConverter{}.WinAcpToUtf16(Config.General.DefaultShellCommand.c_str())};
	const std::wstring scenarioIconPath{CreateIconPath(enginePath, 1)};

	if (!SetRegFileClass(L"Clonk4.Scenario", L"c4s", L"Clonk 4 Scenario", scenarioIconPath, C4FileClassContentType)) return false;
	if (!SetRegFileClass(L"Clonk4.Group", L"c4g", L"Clonk 4 Group", CreateIconPath(enginePath, 2), C4FileClassContentType)) return false;
	if (!SetRegFileClass(L"Clonk4.Folder", L"c4f", L"Clonk 4 Folder", CreateIconPath(enginePath, 3), C4FileClassContentType)) return false;
	if (!SetRegFileClass(L"Clonk4.Player", L"c4p", L"Clonk 4 Player", CreateIconPath(enginePath, 4), C4FileClassContentType)) return false;
	if (!SetRegFileClass(L"Clonk4.Definition", L"c4d", L"Clonk 4 Object Definition", CreateIconPath(enginePath, 6), C4FileClassContentType)) return false;
	if (!SetRegFileClass(L"Clonk4.Object", L"c4i", L"Clonk 4 Object Info", CreateIconPath(enginePath, 7), C4FileClassContentType)) return false;
	if (!SetRegFileClass(L"Clonk4.Material", L"c4m", L"Clonk 4 Material", CreateIconPath(enginePath, 8), L"text/plain")) return false;
	if (!SetRegFileClass(L"Clonk4.Binary", L"c4b", L"Clonk 4 Binary", CreateIconPath(enginePath, 9), L"application/octet-stream")) return false;
	if (!SetRegFileClass(L"Clonk4.Video", L"c4v", L"Clonk 4 Video", CreateIconPath(enginePath, 10), L"video/avi")) return false;
	if (!SetRegFileClass(L"Clonk4.Weblink", L"c4l", L"Clonk 4 Weblink", CreateIconPath(enginePath, 11), C4FileClassContentType)) return false;
	if (!SetRegFileClass(L"Clonk4.Update", L"c4u", L"Clonk 4 Update", CreateIconPath(enginePath, 13), C4FileClassContentType)) return false;

	if (!SetProtocol(L"clonk", L"%s %1", enginePath)) return false;

	// c4u application: send to engine
	if (!SetRegShell(L"Clonk4.Update", L"Update", L"Update", WithCommands(enginePath), CreateIconPath(enginePath, 13), true)) return false;

	const std::array<ShellCommand, 3> shellCommands
	{{
		{L"Open", LoadResStrW("IDS_DLG_STARTGAME"), L"", scenarioIconPath},
		{L"OpenConsole", LoadResStrW("IDS_SHELL_OPENCONSOLE"), WithArguments(L"/console", L"/network"), scenarioIconPath},
		{L"OpenLobby", LoadResStrW("IDS_DLG_NETSTART"), WithArguments(L"/lobby", L"/network"), scenarioIconPath}
	}};

	for (const auto &command : shellCommands)
	{
		if (!command.Apply(enginePath, command.Name == defaultShellCommand))
		{
			return false;
		}
	}

	// kill old App Paths registration
	DeleteRegistryKey(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\Clonk.exe");

	return true;
}
#endif
