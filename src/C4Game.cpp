/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2016, The OpenClonk Team and contributors
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

/* Main class to run the game */

#include <C4Include.h>
#include <C4Game.h>
#include <C4Version.h>
#include <C4Network2Reference.h>
#include <C4FileMonitor.h>

#include <C4GameSave.h>
#include <C4Record.h>
#include <C4Application.h>
#include <C4Object.h>
#include <C4ObjectInfo.h>
#include <C4Random.h>
#include <C4ObjectCom.h>
#include <C4SurfaceFile.h>
#include <C4FullScreen.h>
#include <C4Startup.h>
#include <C4Viewport.h>
#include <C4Command.h>
#include <C4Stat.h>
#include <C4PlayerInfo.h>
#include <C4LoaderScreen.h>
#include <C4Network2Dialogs.h>
#include <C4Console.h>
#include <C4Network2Stats.h>
#include <C4Log.h>
#include <C4Wrappers.h>
#include <C4Player.h>
#include <C4GameOverDlg.h>
#include <C4ObjectMenu.h>
#include <C4GameLobby.h>
#include <C4ChatDlg.h>
#include "C4Thread.h"

#include <StdFile.h>
#include <StdGL.h>

#include <iterator>
#include <sstream>
#include <utility>

constexpr unsigned int defaultIngameGameTickDelay = 28;

C4Game::C4Game()
	: Input(Control.Input), KeyboardInput(C4KeyboardInput_Init()), StartupLogPos(0), QuitLogPos(0), fQuitWithError(false), fPreinited(false),
	Teams(Parameters.Teams),
	PlayerInfos(Parameters.PlayerInfos),
	RestorePlayerInfos(Parameters.RestorePlayerInfos),
	Clients(Parameters.Clients)
{
	Default();
}

C4Game::~C4Game()
{
	// make sure no startup gfx remain loaded
	C4Startup::Unload();
}

bool C4Game::InitDefs()
{
	int32_t iDefs = 0;
	Log(LoadResStr("IDS_PRC_INITDEFS"));
	int iDefResCount = 0;
	for ([[maybe_unused]] const auto &def : Parameters.GameRes.iterRes(NRT_Definitions))
		++iDefResCount;
	int i = 0;
	// Load specified defs
	for (const auto &def : Parameters.GameRes.iterRes(NRT_Definitions))
	{
		int iMinProgress = 10 + (25 * i) / iDefResCount;
		int iMaxProgress = 10 + (25 * (i + 1)) / iDefResCount;
		++i;
		iDefs += Defs.Load(def.getFile(), C4D_Load_RX, Config.General.LanguageEx, &*Application.SoundSystem, true, iMinProgress, iMaxProgress);

		// Def load failure
		if (Defs.LoadFailure) return false;
	}

	// Load for scenario file - ignore sys group here, because it has been loaded already
	iDefs += Defs.Load(ScenarioFile, C4D_Load_RX, Config.General.LanguageEx, &*Application.SoundSystem, true, true, 35, 40, false);

	// Absolutely no defs: we don't like that
	if (!iDefs) { LogFatal(LoadResStr("IDS_PRC_NODEFS")); return false; }

	// Check def engine version (should be done immediately on def load)
	iDefs = Defs.CheckEngineVersion(C4XVER1, C4XVER2, C4XVER3, C4XVER4, C4XVERBUILD);
	if (iDefs > 0) { LogF(LoadResStr("IDS_PRC_DEFSINVC4X"), iDefs); }

	// sort before CheckRequireDef for better id-lookup performance
	Defs.SortByID();

	// Check for unmet requirements
	Defs.CheckRequireDef();

	// get default particles
	Particles.SetDefParticles();

	// Done
	return true;
}

bool C4Game::OpenScenario()
{
	// Scenario from record stream
	if (RecordStream.getSize())
	{
		StdStrBuf RecordFile;
		if (!C4Playback::StreamToRecord(RecordStream.getData(), &RecordFile))
		{
			LogFatal("[!] Could not process record stream data!"); return false;
		}
		SCopy(RecordFile.getData(), ScenarioFilename, _MAX_PATH);
	}

	// Scenario filename check & log
	if (!ScenarioFilename[0]) { LogFatal(LoadResStr("IDS_PRC_NOC4S")); return false; }
	LogF(LoadResStr("IDS_PRC_LOADC4S"), ScenarioFilename);

	// get parent folder, if it's c4f
	pParentGroup = GroupSet.RegisterParentFolders(ScenarioFilename);

	// open scenario
	if (pParentGroup)
	{
		// open from parent group
		if (!ScenarioFile.OpenAsChild(pParentGroup, GetFilename(ScenarioFilename)))
		{
			LogF("%s: %s", LoadResStr("IDS_PRC_FILENOTFOUND"), ScenarioFilename); return false;
		}
	}
	else
		// open directly
		if (!ScenarioFile.Open(ScenarioFilename))
		{
			LogF("%s: %s", LoadResStr("IDS_PRC_FILENOTFOUND"), ScenarioFilename); return false;
		}

	// add scenario to group
	GroupSet.RegisterGroup(ScenarioFile, false, C4GSPrio_Scenario, C4GSCnt_Scenario);

	// Read scenario core
	if (!C4S.Load(ScenarioFile))
	{
		LogFatal(LoadResStr("IDS_PRC_FILEINVALID")); return false;
	}

	// Check minimum engine version
	if (CompareVersion(C4S.Head.C4XVer[0], C4S.Head.C4XVer[1], C4S.Head.C4XVer[2], C4S.Head.C4XVer[3], C4S.Head.C4XVer[4]) > 0)
	{
		LogFatal(FormatString(LoadResStr("IDS_PRC_NOREQC4X"), C4S.Head.C4XVer[0], C4S.Head.C4XVer[1], C4S.Head.C4XVer[2], C4S.Head.C4XVer[3], C4S.Head.C4XVer[4]).getData());
		return false;
	}

	// Add scenario origin to group set
	if (C4S.Head.Origin.getLength() && !ItemIdentical(C4S.Head.Origin.getData(), ScenarioFilename))
		GroupSet.RegisterParentFolders(C4S.Head.Origin.getData());

	// Scenario definition preset
	if (!FixedDefinitions)
	{
		const std::vector<std::string> &defs = C4S.Definitions.GetModules();
		if (!defs.empty()) DefinitionFilenames = defs;

		if (DefinitionFilenames.empty())
		{
			Log(LoadResStr("IDS_PRC_LOCALONLY"));
		}
		else
		{
			Log(LoadResStr("IDS_PRC_SCEOWNDEFS"));
		}
	}

	// add any custom definition path
	if (*Config.General.DefinitionPath)
	{
		StdStrBuf sDefPath; sDefPath.Copy(Config.General.DefinitionPath);
		char *szDefPath = sDefPath.GrabPointer(); TruncateBackslash(szDefPath); sDefPath.Take(szDefPath);
		if (DirectoryExists(sDefPath.getData()))
		{
			std::transform(DefinitionFilenames.begin(), DefinitionFilenames.end(), std::inserter(DefinitionFilenames, DefinitionFilenames.begin()), [](const std::string &def)
			{
				return std::string{Config.General.DefinitionPath} + def;
			});
		}
	}

	// Scan folder local definitions
	std::vector<std::string> localDefs = FoldersWithLocalsDefs(ScenarioFilename);

	DefinitionFilenames.insert(DefinitionFilenames.end(), localDefs.begin(), localDefs.end());

	// Check mission access
	if (C4S.Head.MissionAccess[0])
		if (!SIsModule(Config.General.MissionAccess, C4S.Head.MissionAccess))
		{
			LogFatal(LoadResStr("IDS_PRC_NOMISSIONACCESS")); return false;
		}

	// Game (runtime data)
	GameText.Load(C4CFN_Game, ScenarioFile, C4CFN_Game);

	// SaveGame definition preset override (not needed with new scenarios that
	// have def specs in scenario core, keep for downward compatibility)
	if (C4S.Head.SaveGame) DefinitionFilenamesFromSaveGame();

	// String tables
	ScenarioLangStringTable.LoadEx("StringTbl", ScenarioFile, C4CFN_ScriptStringTbl, Config.General.LanguageEx);

	// Load parameters (not as network client, because then team info has already been sent by host)
	if (!Network.isEnabled() || Network.isHost())
	{
		if (!Parameters.Load(ScenarioFile, &C4S, GameText.GetData(), &ScenarioLangStringTable, DefinitionFilenames))
		{
			LogFatal(LoadResStr("IDS_ERR_LOAD_PARAMETERS"));
			return false;
		}

		if (C4S.Head.SaveGame)
		{
			// make sure that at least all players from the savegame can join
			const auto restoreCount = Parameters.RestorePlayerInfos.GetPlayerCount();
			if (Parameters.MaxPlayers < restoreCount)
			{
				Parameters.MaxPlayers = restoreCount;
			}
		}
	}

	// Title
	Title.LoadEx(LoadResStr("IDS_CNS_TITLE"), ScenarioFile, C4CFN_Title, Config.General.LanguageEx);
	if (!Title.GetLanguageString(Config.General.LanguageEx, Parameters.ScenarioTitle))
		Parameters.ScenarioTitle.CopyValidated(C4S.Head.Title);

	// Load Strings (since kept objects aren't denumerated in sect-load, no problems should occur...)
	if (ScenarioFile.FindEntry(C4CFN_Strings))
		if (!ScriptEngine.Strings.Load(ScenarioFile))
		{
			LogFatal(LoadResStr("IDS_ERR_STRINGS")); return false;
		}
	SetInitProgress(4);

	// Compile runtime data
	if (!CompileRuntimeData(GameText))
	{
		LogFatal(LoadResStr("IDS_ERR_LOAD_RUNTIMEDATA")); return false;
	}

	// If scenario is a directory: Watch for changes
	if (!ScenarioFile.IsPacked())
	{
		AddDirectoryForMonitoring(ScenarioFile.GetFullName().getData());
	}

	PreloadStatus = PreloadLevel::Scenario;

	return true;
}

void C4Game::CloseScenario()
{
	// safe scenario file name
	char szSzenarioFile[_MAX_PATH + 1];
	SCopy(ScenarioFile.GetFullName().getData(), szSzenarioFile, _MAX_PATH);
	// close scenario
	ScenarioFile.Close();
	GroupSet.CloseFolders();
	pParentGroup = nullptr;
	// remove if temporary
	if (TempScenarioFile)
	{
		EraseItem(szSzenarioFile);
		TempScenarioFile = false;
	}
	// clear scenario section
	// this removes any temp files, which may yet need to be used by any future features
	// so better don't do this too early (like, in C4Game::Clear)
	if (pScenarioSections) { delete pScenarioSections; pScenarioSections = pCurrentScenarioSection = nullptr; }
}

bool C4Game::PreInit()
{
	// System
	if (!InitSystem())
	{
		return false;
	}

	// Startup message board
	if (Application.isFullScreen)
	{
		C4Facet cgo; cgo.Set(Application.DDraw->lpBack, 0, 0, Config.Graphics.ResX, Config.Graphics.ResY);
		GraphicsSystem.MessageBoard.Init(cgo, true);
	}

	Log(FANPROJECTTEXT);
	Log(TRADEMARKTEXT);

	// gfx resource file preinit (global files only)
	Log(LoadResStr("IDS_PRC_GFXRES"));
	if (!GraphicsResource.Init())
		// Error was already logged
		return false;

	// Graphics system (required for GUI)
	if (!GraphicsSystem.Init())
	{
		LogFatal(LoadResStr("IDS_ERR_NOGFXSYS")); return false;
	}

	// load GUI
	if (!pGUI)
		pGUI = new C4GUI::Screen(0, 0, Config.Graphics.ResX, Config.Graphics.ResY);

	fPreinited = true;

	return true;
}

bool C4Game::Init()
{
	IsRunning = false;

	InitProgress = 0; LastInitProgress = 0;
	SetInitProgress(0);

	// start log pos (used by startup)
	StartupLogPos = GetLogPos();
	fQuitWithError = false;
	C4GameLobby::UserAbort = false;

	// Store a start time that identifies this game on this host
	StartTime = static_cast<int32_t>(time(nullptr));

	// Get PlayerFilenames from Config, if ParseCommandLine did not fill some in
	// Must be done here, because InitGame calls PlayerInfos.InitLocal
	if (!*PlayerFilenames)
		SCopy(Config.General.Participants, PlayerFilenames, (std::min)(sizeof(PlayerFilenames), sizeof(Config.General.Participants)) - 1);

	// Join a game?
	if (pJoinReference || *DirectJoinAddress)
	{
		if (!GraphicsSystem.pLoaderScreen)
		{
			// init extra; needed for loader screen
			Log(LoadResStr("IDS_PRC_INITEXTRA"));
			Extra.Init();

			// init loader
			if (Application.isFullScreen && !GraphicsSystem.InitLoaderScreen(C4S.Head.Loader))
			{
				LogFatal(LoadResStr("IDS_PRC_ERRLOADER")); return false;
			}
		}

		SetInitProgress(5);

		// Initialize network
		if (pJoinReference)
		{
			// By reference
			bool fSuccess = InitNetworkFromReference(*pJoinReference);
			delete pJoinReference; pJoinReference = nullptr;
			if (!fSuccess)
				return false;
		}
		else
		{
			// By address
			if (!InitNetworkFromAddress(DirectJoinAddress))
				return false;
		}

		// check wether console mode is allowed
		if (!Application.isFullScreen && !Parameters.AllowDebug)
		{
			LogFatal(LoadResStr("IDS_TEXT_JOININCONSOLEMODENOTALLOW")); return false;
		}

		// do lobby (if desired)
		if (Network.isLobbyActive())
			if (!Network.DoLobby())
				return false;
	}

	// Local game or host?
	else
	{
		// check whether console mode is allowed
		if (!Application.isFullScreen && (NetworkActive && Config.Network.LeagueServerSignUp))
		{
			LogFatal("[!] League games in developer mode not allowed!"); return false;
		}

		// Open scenario
		if (!OpenScenario())
		{
			return false;
		}

		// init extra; needed for loader screen
		Log(LoadResStr("IDS_PRC_INITEXTRA"));
		Extra.Init();

		// init loader
		if (Application.isFullScreen && !GraphicsSystem.InitLoaderScreen(C4S.Head.Loader))
		{
			LogFatal(LoadResStr("IDS_PRC_ERRLOADER")); return false;
		}

		// Init network
		if (!InitNetworkHost()) return false;
		SetInitProgress(7);
	}

	Application.SetGameTickDelay(defaultIngameGameTickDelay);
	// now free all startup gfx to make room for game gfx
	C4Startup::Unload();

	// Init debugmode
	DebugMode = !Application.isFullScreen;
	if (Config.General.AlwaysDebug)
		DebugMode = true;
	if (!Parameters.AllowDebug)
		DebugMode = false;

	// Init game
	if (!InitGame(ScenarioFile, nullptr, true)) return false;

	// Network final init
	if (Network.isEnabled())
	{
		if (!Network.FinalInit())
		{
			LogFatal(LoadResStr("IDS_ERR_NETWORKFINALINIT"));
			return false;
		}
	}
	// non-net may have to synchronize now to keep in sync with replays
	// also needs to synchronize to update transfer zones
	else
	{
		// - would kill DebugRec-sync for runtime debugrec starts
		C4DebugRecOff DBGRECOFF(!!C4S.Head.SaveGame);
		SyncClearance();
		Synchronize(false);
	}

	// Init players
	if (!InitPlayers()) return false;
	SetInitProgress(98);

	// Final init
	if (!InitGameFinal()) return false;
	SetInitProgress(99);

	// Color palette
	if (Application.isFullScreen) Application.DDraw->WipeSurface(Application.DDraw->lpPrimary);
	GraphicsSystem.SetPalette();
	GraphicsSystem.ApplyGamma();

	// Message board and upper board
	if (Application.isFullScreen)
	{
		InitFullscreenComponents(true);
	}

	// Default fullscreen menu, in case any old surfaces are left (extra safety)
	FullScreen.CloseMenu();

	// start statistics (always for now. Make this a config?)
	pNetworkStatistics = new C4Network2Stats();

	// clear loader screen
	if (GraphicsSystem.pLoaderScreen)
	{
		delete GraphicsSystem.pLoaderScreen;
		GraphicsSystem.pLoaderScreen = nullptr;
	}

	// game running now!
	IsRunning = true;

	// Start message
	Log(LoadResStr(C4S.Head.NetworkGame ? "IDS_PRC_JOIN" : C4S.Head.SaveGame ? "IDS_PRC_RESUME" : "IDS_PRC_START"));

	// set non-exclusive GUI
	if (pGUI)
	{
		pGUI->SetExclusive(false);
	}

	// after GUI is made non-exclusive, recheck the scoreboard
	Scoreboard.DoDlgShow(0, false);
	SetInitProgress(100);

	// and redraw background
	GraphicsSystem.InvalidateBg();

	return true;
}

void C4Game::Clear()
{
	// join the thread first as it will mess with the cleared state otherwise
	if (PreloadThread.joinable())
	{
		PreloadThread.join();
	}

	FileMonitor.reset();

	if (Application.MusicSystem)
	{
		// fade out music
		Application.MusicSystem->Stop(2000);
	}
	// game no longer running
	IsRunning = false;
	PointersDenumerated = false;

	C4ST_SHOWSTAT

	// Evaluation
	if (GameOver)
	{
		if (!Evaluated) Evaluate();
	}

	// stop statistics
	delete pNetworkStatistics; pNetworkStatistics = nullptr;
	C4AulProfiler::Abort();

	// exit gui
	delete pGUI; pGUI = nullptr;

	// next mission (shoud have been transferred to C4Application now if next mission was desired)
	NextMission.Clear(); NextMissionText.Clear(); NextMissionDesc.Clear();

	Network.Clear();
	Control.Clear();

	// Clear
	Scoreboard.Clear();
	MouseControl.Clear();
	Players.Clear();
	Parameters.Clear();
	RoundResults.Clear();
	C4S.Clear();
	Weather.Clear();
	GraphicsSystem.Clear();
	DeleteObjects(true);
	Defs.Clear();
	Landscape.Clear();
	PXS.Clear();
	delete pGlobalEffects; pGlobalEffects = nullptr;
	Particles.Clear();
	Material.Clear();
	TextureMap.Clear(); // texture map *MUST* be cleared after the materials, because of the patterns!
	GraphicsResource.Clear();
	Messages.Clear();
	MessageInput.Clear();
	Info.Clear();
	Title.Clear();
	Script.Clear();
	Names.Clear();
	GameText.Clear();
	RecordDumpFile.Clear();
	RecordStream.Clear();

	PathFinder.Clear();
	TransferZones.Clear();
#ifndef USE_CONSOLE
	FontLoader.Clear();
#endif

	ScriptEngine.Clear();
	MainSysLangStringTable.Clear();
	ScenarioLangStringTable.Clear();
	ScenarioSysLangStringTable.Clear();
	CloseScenario();
	GroupSet.Clear();
	KeyboardInput.Clear();

	if (Application.MusicSystem)
	{
		SetMusicLevel(100);
	}

	PlayList.Clear();

	// global fullscreen class is not cleared, because it holds the carrier window
	// but the menu must be cleared (maybe move Fullscreen.Menu somewhere else?)
	FullScreen.CloseMenu();

	// Message
	// avoid double message by not printing it if no restbl is loaded
	// this would log an "[Undefined]" only, anyway
	// (could abort the whole clear-procedure here, btw?)
	if (IsResStrTableLoaded()) Log(LoadResStr("IDS_CNS_GAMECLOSED"));

	// clear game starting parameters
	DefinitionFilenames.clear();
	*DirectJoinAddress = *ScenarioFilename = *PlayerFilenames = 0;

	// join reference
	delete pJoinReference; pJoinReference = nullptr;

	// okay, game cleared now. Remember log section
	QuitLogPos = GetLogPos();

	fPreinited = false;
}

bool C4Game::GameOverCheck()
{
	int32_t cnt;
	bool fDoGameOver = false;

	// Only every 35 ticks
	if (Tick35) return false;

	// do not GameOver in replay
	if (Control.isReplay()) return false;

	// All players eliminated: game over
	if (!Players.GetCountNotEliminated())
		fDoGameOver = true;

	// Cooperative game over (obsolete with new game goal objects, kept for
	// downward compatibility with CreateObjects,ClearObjects,ClearMaterial settings)
	C4ID c_id;
	int32_t count, mat;
	bool condition_valid, condition_true;
	bool game_over_valid = false, game_over = true;
	// CreateObjects
	condition_valid = false;
	condition_true = true;
	for (cnt = 0; (c_id = C4S.Game.CreateObjects.GetID(cnt, &count)); cnt++)
		if (count > 0)
		{
			condition_valid = true;
			// Count objects, fullsize only
			C4ObjectLink *cLnk;
			int32_t iCount = 0;
			for (cLnk = Objects.First; cLnk; cLnk = cLnk->Next)
				if (cLnk->Obj->Status)
					if (cLnk->Obj->Def->id == c_id)
						if (cLnk->Obj->GetCon() >= FullCon)
							iCount++;
			if (iCount < count) condition_true = false;
		}
	if (condition_valid)
	{
		game_over_valid = true; if (!condition_true) game_over = false;
	}
	// ClearObjects
	condition_valid = false;
	condition_true = true;
	for (cnt = 0; (c_id = C4S.Game.ClearObjects.GetID(cnt, &count)); cnt++)
	{
		condition_valid = true;
		// Count objects, if category living, live only
		C4ObjectLink *cLnk;
		C4Def *cdef = C4Id2Def(c_id);
		bool alive_only = false;
		if (cdef && (cdef->Category & C4D_Living)) alive_only = true;
		int32_t iCount = 0;
		for (cLnk = Objects.First; cLnk; cLnk = cLnk->Next)
			if (cLnk->Obj->Status)
				if (cLnk->Obj->Def->id == c_id)
					if (!alive_only || cLnk->Obj->GetAlive())
						iCount++;
		if (iCount > count) condition_true = false;
	}
	if (condition_valid)
	{
		game_over_valid = true; if (!condition_true) game_over = false;
	}
	// ClearMaterial
	condition_valid = false;
	condition_true = true;
	for (cnt = 0; cnt < C4MaxNameList; cnt++)
		if (C4S.Game.ClearMaterial.Name[cnt][0])
			if (MatValid(mat = Material.Get(C4S.Game.ClearMaterial.Name[cnt])))
			{
				condition_valid = true;
				if (Landscape.EffectiveMatCount[mat] > static_cast<uint32_t>(C4S.Game.ClearMaterial.Count[cnt]))
					condition_true = false;
			}
	if (condition_valid)
	{
		game_over_valid = true; if (!condition_true) game_over = false;
	}

	// Evaluate game over
	if (game_over_valid)
		if (game_over)
			fDoGameOver = true;

	// Message
	if (fDoGameOver) DoGameOver();

	return GameOver;
}

int32_t iLastControlSize = 0;
extern int32_t iPacketDelay;

C4ST_NEW(ControlRcvStat,  "C4Game::Execute ReceiveControl")
C4ST_NEW(ControlStat,     "C4Game::Execute ExecuteControl")
C4ST_NEW(ExecObjectsStat, "C4Game::Execute ExecObjects")
C4ST_NEW(GEStats,         "C4Game::Execute pGlobalEffects->Execute")
C4ST_NEW(PXSStat,         "C4Game::Execute PXS.Execute")
C4ST_NEW(PartStat,        "C4Game::Execute Particles.Execute")
C4ST_NEW(MassMoverStat,   "C4Game::Execute MassMover.Execute")
C4ST_NEW(WeatherStat,     "C4Game::Execute Weather.Execute")
C4ST_NEW(PlayersStat,     "C4Game::Execute Players.Execute")
C4ST_NEW(LandscapeStat,   "C4Game::Execute Landscape.Execute")
C4ST_NEW(MusicSystemStat, "C4Game::Execute MusicSystem.Execute")
C4ST_NEW(MessagesStat,    "C4Game::Execute Messages.Execute")
C4ST_NEW(ScriptStat,      "C4Game::Execute Script.Execute")

#define EXEC_S(Expressions, Stat) \
	{ C4ST_START(Stat) Expressions C4ST_STOP(Stat) }

#ifdef DEBUGREC
#define EXEC_S_DR(Expressions, Stat, DebugRecName) { AddDbgRec(RCT_Block, DebugRecName, 6); EXEC_S(Expressions, Stat) }
#define EXEC_DR(Expressions, DebugRecName) { AddDbgRec(RCT_Block, DebugRecName, 6); Expressions }
#else
#define EXEC_S_DR(Expressions, Stat, DebugRecName) EXEC_S(Expressions, Stat)
#define EXEC_DR(Expressions, DebugRecName) Expressions
#endif

bool C4Game::Execute() // Returns true if the game is over
{
	// Let's go
	GameGo = true;

	// Network
	Network.Execute();

	// Prepare control
	bool fControl;
	EXEC_S(fControl = Control.Prepare();, ControlStat)
	if (!fControl) return false; // not ready yet: wait

	// Halt
	if (HaltCount) return false;

#ifdef DEBUGREC
	Landscape.DoRelights();
#endif

	// Execute the control
	Control.Execute();
	if (!IsRunning) return false;

	// Ticks
	EXEC_DR(Ticks();, "Ticks")

#ifdef DEBUGREC
	// debugrec
	AddDbgRec(RCT_DbgFrame, &FrameCounter, sizeof(int32_t));
#endif

	// Game

	EXEC_S(ExecObjects();, ExecObjectsStat)
	if (pGlobalEffects)
		EXEC_S_DR(pGlobalEffects->Execute(nullptr);, GEStats, "GEEx\0");
	EXEC_S_DR(PXS.Execute();,                      PXSStat,         "PXSEx")
	EXEC_S_DR(Particles.GlobalParticles.Exec();,   PartStat,        "ParEx")
	EXEC_S_DR(MassMover.Execute();,                MassMoverStat,   "MMvEx")
	EXEC_S_DR(Weather.Execute();,                  WeatherStat,     "WtrEx")
	EXEC_S_DR(Landscape.Execute();,                LandscapeStat,   "LdsEx")
	EXEC_S_DR(Players.Execute();,                  PlayersStat,     "PlrEx")
	// FIXME: C4Application::Execute should do this, but what about the stats?
	EXEC_S_DR(Application.MusicSystem->Execute();, MusicSystemStat, "Music")
	EXEC_S_DR(Messages.Execute();,                 MessagesStat,    "MsgEx")
	EXEC_S_DR(Script.Execute();,                   ScriptStat,      "Scrpt")

	EXEC_DR(MouseControl.Execute();, "Input")

	EXEC_DR(UpdateRules();
	GameOverCheck();, "Misc\0")

	Control.DoSyncCheck();

	// Evaluation; Game over dlg
	if (GameOver)
	{
		if (!Evaluated) Evaluate();
		if (!GameOverDlgShown) ShowGameOverDlg();
	}

	// show stat each 1000 ticks
	if (!(FrameCounter % 1000))
	{
		C4ST_SHOWPARTSTAT
		C4ST_RESETPART
	}

#ifdef DEBUGREC
	AddDbgRec(RCT_Block, "eGame", 6);

	Landscape.DoRelights();
#endif

	return true;
}

void C4Game::InitFullscreenComponents(bool fRunning)
{
	if (!Application.DDraw) return;

	if (fRunning)
	{
		// running game: Message board upper board and viewports
		C4Facet cgo2;
		cgo2.Set(Application.DDraw->lpBack, 0, 0, Config.Graphics.ResX, C4UpperBoard::Height());

		C4Facet cgo;
		cgo.Set(Application.DDraw->lpBack, 0, Config.Graphics.ResY - GraphicsResource.FontRegular.GetLineHeight(),
			Config.Graphics.ResX, GraphicsResource.FontRegular.GetLineHeight());
		GraphicsSystem.UpperBoard.Init(cgo2, cgo);
		GraphicsSystem.MessageBoard.Init(cgo, false);

		GraphicsSystem.RecalculateViewports();
	}
	else
	{
		// startup game: Just fullscreen message board
		C4Facet cgo; cgo.Set(Application.DDraw->lpBack, 0, 0, Config.Graphics.ResX, Config.Graphics.ResY);
		GraphicsSystem.MessageBoard.Init(cgo, true);
	}
}

bool C4Game::InitMaterialTexture()
{
	// Clear old data
	TextureMap.Clear();
	Material.Clear();

	// Check for scenario local materials
	bool fHaveScenMaterials = ScenarioFile.FindEntry(C4CFN_Material);

	// Load all materials
	auto matRes = Parameters.GameRes.iterRes(NRT_Material);
	bool fFirst = true, fOverloadMaterials = true, fOverloadTextures = true;
	long tex_count = 0, mat_count = 0;
	while (fOverloadMaterials || fOverloadTextures)
	{
		// Are there any scenario local materials that need to be looked at firs?
		C4Group Mats;
		if (fHaveScenMaterials)
		{
			if (!Mats.OpenAsChild(&ScenarioFile, C4CFN_Material))
			{
				LogFatal(FormatString(LoadResStr("IDS_ERR_SCENARIOMATERIALS"), Mats.GetError()).getData());
				return false;
			}
			// Once only
			fHaveScenMaterials = false;
		}
		else
		{
			if (matRes == matRes.end()) break;
			// Find next external material source
			if (!Mats.Open(matRes->getFile()))
			{
				LogFatal(FormatString(LoadResStr("IDS_ERR_EXTERNALMATERIALS"), matRes->getFile(), Mats.GetError()).getData());
				return false;
			}
			++matRes;
		}

		// First material file? Load texture map.
		bool fNewOverloadMaterials = false, fNewOverloadTextures = false;
		if (fFirst)
		{
			long tme_count = TextureMap.LoadMap(Mats, C4CFN_TexMap, &fNewOverloadMaterials, &fNewOverloadTextures);
			LogF(LoadResStr("IDS_PRC_TEXMAPENTRIES"), tme_count);
			// Only once
			fFirst = false;
		}
		else
		{
			// Check overload-flags only
			if (!C4TextureMap::LoadFlags(Mats, C4CFN_TexMap, &fNewOverloadMaterials, &fNewOverloadTextures))
				fOverloadMaterials = fOverloadTextures = false;
		}

		// Load textures
		if (fOverloadTextures)
		{
			int iTexs = TextureMap.LoadTextures(Mats);
			// Automatically continue search if no texture was found
			if (!iTexs) fNewOverloadTextures = true;
			tex_count += iTexs;
		}

		// Load materials
		if (fOverloadMaterials)
		{
			int iMats = Material.Load(Mats);
			// Automatically continue search if no material was found
			if (!iMats) fNewOverloadMaterials = true;
			mat_count += iMats;
		}

		// Set flags
		fOverloadTextures = fNewOverloadTextures;
		fOverloadMaterials = fNewOverloadMaterials;
	}

	// Logs
	LogF(LoadResStr("IDS_PRC_TEXTURES"), tex_count);
	LogF(LoadResStr("IDS_PRC_MATERIALS"), mat_count);

	// Load material enumeration
	if (!Material.LoadEnumeration(ScenarioFile))
	{
		LogFatal(LoadResStr("IDS_PRC_NOMATENUM")); return false;
	}

	// Initialize texture map
	TextureMap.Init();

	// Cross map mats (after texture init, because Material-Texture-combinations are used)
	Material.CrossMapMaterials();

	// Enumerate materials
	if (!EnumerateMaterials()) return false;

	// get material script funcs
	Material.UpdateScriptPointers();

	return true;
}

void C4Game::ClearObjectPtrs(C4Object *pObj)
{
	// May not call Objects.ClearPointers() because that would
	// remove pObj from primary list and pObj is to be kept
	// until CheckObjectRemoval().
	C4Object *cObj; C4ObjectLink *clnk;
	for (clnk = Objects.First; clnk && (cObj = clnk->Obj); clnk = clnk->Next)
		cObj->ClearPointers(pObj);
	// check in inactive objects as well
	for (clnk = Objects.InactiveObjects.First; clnk && (cObj = clnk->Obj); clnk = clnk->Next)
		cObj->ClearPointers(pObj);
	Application.SoundSystem->ClearPointers(pObj);
}

void C4Game::ClearPointers(C4Object *pObj)
{
	BackObjects.ClearPointers(pObj);
	ForeObjects.ClearPointers(pObj);
	Messages.ClearPointers(pObj);
	ClearObjectPtrs(pObj);
	Players.ClearPointers(pObj);
	GraphicsSystem.ClearPointers(pObj);
	MessageInput.ClearPointers(pObj);
	Console.ClearPointers(pObj);
	MouseControl.ClearPointers(pObj);
	TransferZones.ClearPointers(pObj);
	if (pGlobalEffects)
		pGlobalEffects->ClearPointers(pObj);
}

bool C4Game::TogglePause()
{
	// pause toggling disabled during round evaluation
	if (C4GameOverDlg::IsShown()) return false;
	// otherwise, toggle
	if (IsPaused()) return Unpause(); else return Pause();
}

bool C4Game::Pause()
{
	// already paused?
	if (IsPaused()) return false;
	// pause by net?
	if (Network.isEnabled())
	{
		// league? Vote...
		if (Parameters.isLeague() && !Evaluated)
		{
			Network.Vote(VT_Pause, true, true);
			return false;
		}
		// host only
		if (!Network.isHost()) return true;
		Network.Pause();
	}
	else
	{
		// pause game directly
		HaltCount = true;
	}
	Console.UpdateHaltCtrls(IsPaused());
	return true;
}

bool C4Game::Unpause()
{
	// already paused?
	if (!IsPaused()) return false;
	// pause by net?
	if (Network.isEnabled())
	{
		// league? Vote...
		if (Parameters.isLeague() && !Evaluated)
		{
			Network.Vote(VT_Pause, true, false);
			return false;
		}
		// host only
		if (!Network.isHost()) return true;
		Network.Start();
	}
	else
	{
		// unpause game directly
		HaltCount = false;
	}
	Console.UpdateHaltCtrls(IsPaused());
	return true;
}

bool C4Game::IsPaused()
{
	// pause state defined either by network or by game halt count
	if (Network.isEnabled())
		return !Network.isRunning();
	return !!HaltCount;
}

C4Object *C4Game::NewObject(C4Def *pDef, C4Object *pCreator,
	int32_t iOwner, C4ObjectInfo *pInfo,
	int32_t iX, int32_t iY, int32_t iR,
	C4Fixed xdir, C4Fixed ydir, C4Fixed rdir,
	int32_t iCon, int32_t iController)
{
	// Safety
	if (!pDef) return nullptr;
#ifdef DEBUGREC
	C4RCCreateObj rc;
	rc.id = pDef->id;
	rc.oei = ObjectEnumerationIndex + 1;
	rc.x = iX; rc.y = iY; rc.ownr = iOwner;
	AddDbgRec(RCT_CrObj, &rc, sizeof(rc));
#endif
	// Create object
	auto obj = std::make_unique<C4Object>();
	C4Object *const objPtr{obj.get()};

	// Initialize object
	obj->Init(pDef, pCreator, iOwner, pInfo, iX, iY, iR, xdir, ydir, rdir, iController);
	// Enumerate object
	obj->Number = ++ObjectEnumerationIndex;
	// Add to object list
	if (Objects.Add(obj.get()))
	{
		obj.release();
	}
	else
	{
		return nullptr;
	}

	// From now on, object is ready to be used in scripts!
	// Construction callback
	C4AulParSet pars(C4VObj(pCreator));
	objPtr->Call(PSF_Construction, pars);
	// AssignRemoval called? (Con 0)
	if (!objPtr->Status) { return nullptr; }
	// Do initial con
	objPtr->DoCon(iCon, true);
	// AssignRemoval called? (Con 0)
	if (!objPtr->Status) { return nullptr; }
	// Success
	return objPtr;
}

void C4Game::DeleteObjects(bool fDeleteInactive)
{
	// del any objects
	Objects.DeleteObjects();
	BackObjects.Clear();
	ForeObjects.Clear();
	if (fDeleteInactive) Objects.InactiveObjects.DeleteObjects();
	// reset resort flag
	fResortAnyObject = false;
}

C4Object *C4Game::CreateObject(C4ID id, C4Object *pCreator, int32_t iOwner,
	int32_t x, int32_t y, int32_t r,
	C4Fixed xdir, C4Fixed ydir, C4Fixed rdir, int32_t iController)
{
	C4Def *pDef;
	// Get pDef
	if (!(pDef = C4Id2Def(id))) return nullptr;
	// Create object
	return NewObject(pDef, pCreator,
		iOwner, nullptr,
		x, y, r,
		xdir, ydir, rdir,
		FullCon, iController);
}

C4Object *C4Game::CreateInfoObject(C4ObjectInfo *cinf, int32_t iOwner,
	int32_t tx, int32_t ty)
{
	C4Def *def;
	// Valid check
	if (!cinf) return nullptr;
	// Get def
	if (!(def = C4Id2Def(cinf->id))) return nullptr;
	// Create object
	return NewObject(def, nullptr,
		iOwner, cinf,
		tx, ty, 0,
		Fix0, Fix0, Fix0,
		FullCon, NO_OWNER);
}

C4Object *C4Game::CreateObjectConstruction(C4ID id,
	C4Object *pCreator,
	int32_t iOwner,
	int32_t iX, int32_t iBY,
	int32_t iCon,
	bool fTerrain)
{
	C4Def *pDef;
	C4Object *pObj;

	// Get def
	if (!(pDef = C4Id2Def(id))) return nullptr;

	int32_t dx, dy, dwdt, dhgt;
	dwdt = pDef->Shape.Wdt; dhgt = pDef->Shape.Hgt;
	dx = iX - dwdt / 2; dy = iBY - dhgt;

	// Terrain & Basement
	if (fTerrain)
	{
		// Clear site background (ignored for ultra-large structures)
		if (dwdt * dhgt < 12000)
			Landscape.DigFreeRect(dx, dy, dwdt, dhgt);
		// Raise Terrain
		Landscape.RaiseTerrain(dx, dy + dhgt, dwdt);
		// Basement
		if (pDef->Basement)
		{
			const int32_t BasementStrength = 8;
			// Border basement
			if (pDef->Basement > 1)
			{
				Landscape.DrawMaterialRect(MGranite, dx, dy + dhgt, std::min<int32_t>(pDef->Basement, dwdt), BasementStrength);
				Landscape.DrawMaterialRect(MGranite, dx + dwdt - std::min<int32_t>(pDef->Basement, dwdt), dy + dhgt, std::min<int32_t>(pDef->Basement, dwdt), BasementStrength);
			}
			// Normal basement
			else
				Landscape.DrawMaterialRect(MGranite, dx, dy + dhgt, dwdt, BasementStrength);
		}
	}

	// Create object
	if (!(pObj = NewObject(pDef,
		pCreator,
		iOwner, nullptr,
		iX, iBY, 0,
		Fix0, Fix0, Fix0,
		iCon, pCreator ? pCreator->Controller : NO_OWNER))) return nullptr;

	return pObj;
}

void C4Game::BlastObjects(int32_t tx, int32_t ty, int32_t level, C4Object *inobj, int32_t iCausedBy, C4Object *pByObj)
{
	C4Object *cObj; C4ObjectLink *clnk;

	// layer check: Blast in same layer only
	if (pByObj) pByObj = pByObj->pLayer;

	// Contained blast
	if (inobj)
	{
		inobj->Blast(level, iCausedBy);
		for (clnk = Objects.First; clnk && (cObj = clnk->Obj); clnk = clnk->Next)
			if (cObj->Status) if (cObj->Contained == inobj) if (cObj->pLayer == pByObj)
				cObj->Blast(level, iCausedBy);
	}

	// Uncontained blast local outside objects
	else
	{
		for (clnk = Objects.First; clnk && (cObj = clnk->Obj); clnk = clnk->Next)
			if (cObj->Status) if (!cObj->Contained) if (cObj->pLayer == pByObj)
			{
				// Direct hit (5 pixel range to all sides)
				if (Inside<int32_t>(ty - (cObj->y + cObj->Shape.y), -5, cObj->Shape.Hgt - 1 + 10))
					if (Inside<int32_t>(tx - (cObj->x + cObj->Shape.x), -5, cObj->Shape.Wdt - 1 + 10))
						cObj->Blast(level, iCausedBy);
				// Shock wave hit (if in level range, living, object and vehicle only. No structures/StatickBack, as this would mess up castles, elevators, etc.!)
				if (cObj->Category & (C4D_Living | C4D_Object | C4D_Vehicle))
					if (!cObj->Def->NoHorizontalMove)
						if (Abs(ty - cObj->y) <= level)
							if (Abs(tx - cObj->x) <= level)
							{
								// vehicles and floating objects only if grab+pushable (no throne, no tower entrances...)
								if (cObj->Def->Grab != 1)
								{
									if (cObj->Category & C4D_Vehicle)
										continue;
									if (cObj->Action.Act >= 0)
										if (cObj->Def->ActMap[cObj->Action.Act].Procedure == DFA_FLOAT)
											continue;
								}
								if (cObj->Category & C4D_Living)
								{
									// living takes additional dmg from blasts
									cObj->DoEnergy(-level / 2, false, C4FxCall_EngBlast, iCausedBy);
									cObj->DoDamage(level / 2, iCausedBy, C4FxCall_DmgBlast);
								}

								// force argument evaluation order
								const auto p2 = itofix(-level + Abs(ty - cObj->y)) / BoundBy<int32_t>(cObj->Mass / 10, 4, (cObj->Category & C4D_Living) ? 8 : 20);
								const auto p1 = itofix(Sign(cObj->x - tx + Rnd3()) * (level - Abs(tx - cObj->x))) / BoundBy<int32_t>(cObj->Mass / 10, 4, (cObj->Category & C4D_Living) ? 8 : 20);
								cObj->Fling(p1, p2, true, iCausedBy);
							}
			}
	}
}

void C4Game::ShakeObjects(int32_t tx, int32_t ty, int32_t range, int32_t iCausedBy)
{
	C4Object *cObj; C4ObjectLink *clnk;

	for (clnk = Objects.First; clnk && (cObj = clnk->Obj); clnk = clnk->Next)
		if (cObj->Status) if (!cObj->Contained)
			if (cObj->Category & C4D_Living)
				if (Abs(ty - cObj->y) <= range)
					if (Abs(tx - cObj->x) <= range)
						if (!Random(3))
							if (cObj->Action.t_attach)
								if (!MatVehicle(cObj->Shape.AttachMat))
								{
									cObj->Fling(itofix(Rnd3()), Fix0, false, iCausedBy);
								}
}

C4Object *C4Game::OverlapObject(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt, int32_t category)
{
	C4Object *cObj; C4ObjectLink *clnk;
	C4Rect rect1, rect2;
	rect1.x = tx; rect1.y = ty; rect1.Wdt = wdt; rect1.Hgt = hgt;
	C4LArea Area(&Objects.Sectors, tx, ty, wdt, hgt); C4LSector *pSector;
	for (C4ObjectList *pObjs = Area.FirstObjectShapes(&pSector); pSector; pObjs = Area.NextObjectShapes(pObjs, &pSector))
		for (clnk = pObjs->First; clnk && (cObj = clnk->Obj); clnk = clnk->Next)
			if (cObj->Status) if (!cObj->Contained)
				if (cObj->Category & category & C4D_SortLimit)
				{
					rect2 = cObj->Shape; rect2.x += cObj->x; rect2.y += cObj->y;
					if (rect1.Overlap(rect2)) return cObj;
				}
	return nullptr;
}

C4Object *C4Game::FindObject(C4ID id,
	int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt,
	uint32_t ocf,
	const char *szAction, C4Object *pActionTarget,
	C4Object *pExclude,
	C4Object *pContainer,
	int32_t iOwner,
	C4Object *pFindNext)
{
	C4Object *pClosest = nullptr;
	int32_t iClosest = 0, iDistance, iFartherThan = -1;
	C4Object *cObj;
	C4ObjectLink *cLnk;
	C4Def *pDef;
	C4Object *pFindNextCpy = pFindNext;

	// check the easy cases first
	if (id != C4ID_None)
	{
		if (!(pDef = C4Id2Def(id))) return nullptr; // no valid def
		if (!pDef->Count) return nullptr; // no instances at all
	}

	// Finding next closest: find closest but further away than last closest
	if (pFindNext && (iWdt == -1) && (iHgt == -1))
	{
		iFartherThan = (pFindNext->x - iX) * (pFindNext->x - iX) + (pFindNext->y - iY) * (pFindNext->y - iY);
		pFindNext = nullptr;
	}

	bool bFindActIdle = SEqual(szAction, "Idle") || SEqual(szAction, "ActIdle");

	// Scan all objects
	for (cLnk = Objects.First; cLnk && (cObj = cLnk->Obj); cLnk = cLnk->Next)
	{
		// Not skipping to find next
		if (!pFindNext)
			// Status
			if (cObj->Status)
				// ID
				if ((id == C4ID_None) || (cObj->Def->id == id))
					// OCF (match any specified)
					if (cObj->OCF & ocf)
						// Exclude
						if (cObj != pExclude)
							// Action
							if (!szAction || !szAction[0] || (bFindActIdle && cObj->Action.Act <= ActIdle) || ((cObj->Action.Act > ActIdle) && SEqual(szAction, cObj->Def->ActMap[cObj->Action.Act].Name)))
								// ActionTarget
								if (!pActionTarget || ((cObj->Action.Act > ActIdle) && ((cObj->Action.Target == pActionTarget) || (cObj->Action.Target2 == pActionTarget))))
									// Container
									if (!pContainer || (cObj->Contained == pContainer) || ((reinterpret_cast<std::intptr_t>(pContainer) == NO_CONTAINER) && !cObj->Contained) || ((reinterpret_cast<std::intptr_t>(pContainer) == ANY_CONTAINER) && cObj->Contained))
										// Owner
										if ((iOwner == ANY_OWNER) || (cObj->Owner == iOwner))
											// Area
										{
											// Full range
											if ((iX == 0) && (iY == 0) && (iWdt == 0) && (iHgt == 0))
												return cObj;
											// Point
											if ((iWdt == 0) && (iHgt == 0))
											{
												if (Inside<int32_t>(iX - (cObj->x + cObj->Shape.x), 0, cObj->Shape.Wdt - 1))
													if (Inside<int32_t>(iY - (cObj->y + cObj->Shape.y), 0, cObj->Shape.Hgt - 1))
														return cObj;
												continue;
											}
											// Closest
											if ((iWdt == -1) && (iHgt == -1))
											{
												iDistance = (cObj->x - iX) * (cObj->x - iX) + (cObj->y - iY) * (cObj->y - iY);
												// same distance?
												if ((iDistance == iFartherThan) && !pFindNextCpy)
													return cObj;
												// nearer than/first closest?
												if (!pClosest || (iDistance < iClosest))
													if (iDistance > iFartherThan)
													{
														pClosest = cObj; iClosest = iDistance;
													}
											}
											// Range
											else if (Inside<int32_t>(cObj->x - iX, 0, iWdt - 1) && Inside<int32_t>(cObj->y - iY, 0, iHgt - 1))
												return cObj;
										}

		// Find next mark reached
		if (cObj == pFindNextCpy) pFindNext = pFindNextCpy = nullptr;
	}

	return pClosest;
}

C4Object *C4Game::FindVisObject(int32_t tx, int32_t ty, int32_t iPlr, const C4Facet &fctViewport,
	int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt,
	uint32_t ocf,
	C4Object *pExclude,
	int32_t iOwner,
	C4Object *pFindNext)
{
	// FIXME: Use C4FindObject here for optimization
	C4Object *cObj; C4ObjectLink *cLnk; C4ObjectList *pLst = &ForeObjects;

	// scan all object lists separately
	while (pLst)
	{
		// Scan all objects in list
		for (cLnk = Objects.First; cLnk && (cObj = cLnk->Obj); cLnk = cLnk->Next)
		{
			// Not skipping to find next
			if (!pFindNext)
				// Status
				if (cObj->Status == C4OS_NORMAL)
					// exclude fore/back-objects from main list
					if ((pLst != &Objects) || (!(cObj->Category & C4D_BackgroundOrForeground)))
						// exclude MouseIgnore-objects
						if (~cObj->Category & C4D_MouseIgnore)
							// OCF (match any specified)
							if (cObj->OCF & ocf)
								// Exclude
								if (cObj != pExclude)
									// Container
									if (!cObj->Contained)
										// Owner
										if ((iOwner == ANY_OWNER) || (cObj->Owner == iOwner))
											// Visibility
											if (!cObj->Visibility || cObj->IsVisible(iPlr, false))
												// Area
											{
												// Layer check: Layered objects are invisible to players whose cursor is in another layer
												if (ValidPlr(iPlr))
												{
													C4Object *pCursor = Players.Get(iPlr)->Cursor;
													if (!pCursor || (pCursor->pLayer != cObj->pLayer)) continue;
												}
												// Full range
												if ((iX == 0) && (iY == 0) && (iWdt == 0) && (iHgt == 0))
													return cObj;
												// get object position
												int32_t iObjX, iObjY; cObj->GetViewPos(iObjX, iObjY, tx, ty, fctViewport);
												// Point search
												if ((iWdt == 0) && (iHgt == 0))
												{
													if (Inside<int32_t>(iX - (iObjX + cObj->Shape.x), 0, cObj->Shape.Wdt - 1))
														if (Inside<int32_t>(iY - (iObjY + cObj->Shape.y - cObj->addtop()), 0, cObj->Shape.Hgt + cObj->addtop() - 1))
															return cObj;
													continue;
												}
												// Range
												if (Inside<int32_t>(iObjX - iX, 0, iWdt - 1) && Inside<int32_t>(iObjY - iY, 0, iHgt - 1))
												{
													return cObj;
												}
											}

			// Find next mark reached
			if (cObj == pFindNext) pFindNext = nullptr;
		}
		// next list
		if (pLst == &ForeObjects) pLst = &Objects;
		else if (pLst == &Objects) pLst = &BackObjects;
		else pLst = nullptr;
	}

	// none found
	return nullptr;
}

int32_t C4Game::ObjectCount(C4ID id,
	int32_t x, int32_t y, int32_t wdt, int32_t hgt,
	uint32_t ocf,
	const char *szAction, C4Object *pActionTarget,
	C4Object *pExclude,
	C4Object *pContainer,
	int32_t iOwner)
{
	int32_t iResult = 0; C4Def *pDef;
	// check the easy cases first
	if (id != C4ID_None)
	{
		if (!(pDef = C4Id2Def(id))) return 0; // no valid def
		if (!pDef->Count) return 0; // no instances at all
		if (!x && !y && !wdt && !hgt && ocf == OCF_All && !szAction && !pActionTarget && !pExclude && !pContainer && (iOwner == ANY_OWNER))
			// plain id-search: return known count
			return pDef->Count;
	}
	C4Object *cObj; C4ObjectLink *clnk;
	bool bFindActIdle = SEqual(szAction, "Idle") || SEqual(szAction, "ActIdle");
	for (clnk = Objects.First; clnk && (cObj = clnk->Obj); clnk = clnk->Next)
		// Status
		if (cObj->Status)
			// ID
			if ((id == C4ID_None) || (cObj->Def->id == id))
				// OCF
				if (cObj->OCF & ocf)
					// Exclude
					if (cObj != pExclude)
						// Action
						if (!szAction || !szAction[0] || (bFindActIdle && cObj->Action.Act <= ActIdle) || ((cObj->Action.Act > ActIdle) && SEqual(szAction, cObj->Def->ActMap[cObj->Action.Act].Name)))
							// ActionTarget
							if (!pActionTarget || ((cObj->Action.Act > ActIdle) && ((cObj->Action.Target == pActionTarget) || (cObj->Action.Target2 == pActionTarget))))
								// Container
								if (!pContainer || (cObj->Contained == pContainer) || ((reinterpret_cast<std::intptr_t>(pContainer) == NO_CONTAINER) && !cObj->Contained) || ((reinterpret_cast<std::intptr_t>(pContainer) == ANY_CONTAINER) && cObj->Contained))
									// Owner
									if ((iOwner == ANY_OWNER) || (cObj->Owner == iOwner))
										// Area
									{
										// Full range
										if ((x == 0) && (y == 0) && (wdt == 0) && (hgt == 0))
										{
											iResult++; continue;
										}
										// Point
										if ((wdt == 0) && (hgt == 0))
										{
											if (Inside<int32_t>(x - (cObj->x + cObj->Shape.x), 0, cObj->Shape.Wdt - 1))
												if (Inside<int32_t>(y - (cObj->y + cObj->Shape.y), 0, cObj->Shape.Hgt - 1))
												{
													iResult++; continue;
												}
											continue;
										}
										// Range
										if (Inside<int32_t>(cObj->x - x, 0, wdt - 1) && Inside<int32_t>(cObj->y - y, 0, hgt - 1))
										{
											iResult++; continue;
										}
									}

	return iResult;
}

// Deletes removal-assigned data from list.
// Pointer clearance is done by AssignRemoval.

void C4Game::ObjectRemovalCheck() // Every Tick255 by ExecObjects
{
	C4Object *cObj; C4ObjectLink *clnk, *next;
	for (clnk = Objects.First; clnk && (cObj = clnk->Obj); clnk = next)
	{
		next = clnk->Next;
		if (!cObj->Status && (cObj->RemovalDelay == 0))
		{
			Objects.Remove(cObj);
			delete cObj;
		}
	}
}

void C4Game::ExecObjects() // Every Tick1 by Execute
{
#ifdef DEBUGREC
	AddDbgRec(RCT_Block, "ObjEx", 6);
#endif

	// Execute objects - reverse order to ensure
	C4Object *cObj; C4ObjectLink *clnk;
	for (clnk = Objects.Last; clnk && (cObj = clnk->Obj); clnk = clnk->Prev)
		if (cObj->Status)
			// Execute object
			cObj->Execute();
		else
			// Status reset: process removal delay
			if (cObj->RemovalDelay > 0) cObj->RemovalDelay--;

#ifdef DEBUGREC
	AddDbgRec(RCT_Block, "ObjCC", 6);
#endif

	// Cross check objects
	Objects.CrossCheck();

#ifdef DEBUGREC
	AddDbgRec(RCT_Block, "ObjRs", 6);
#endif

	// Resort
	if (fResortAnyObject)
	{
		fResortAnyObject = false;
		Objects.ResortUnsorted();
	}
	if (Objects.ResortProc) Objects.ExecuteResorts();

#ifdef DEBUGREC
	AddDbgRec(RCT_Block, "ObjRm", 6);
#endif

	// Removal
	if (!Tick255) ObjectRemovalCheck();
}

bool C4Game::CreateViewport(int32_t iPlayer, bool fSilent)
{
	return GraphicsSystem.CreateViewport(iPlayer, fSilent);
}

C4ID DefFileGetID(const char *szFilename)
{
	C4Group hDef;
	C4DefCore DefCore;
	if (!hDef.Open(szFilename)) return C4ID_None;
	if (!DefCore.Load(hDef)) { hDef.Close(); return C4ID_None; }
	hDef.Close();
	return DefCore.id;
}

bool C4Game::DropFile(const char *szFilename, int32_t iX, int32_t iY)
{
	C4ID c_id; C4Def *cdef;
	// Drop def to create object
	if (SEqualNoCase(GetExtension(szFilename), "c4d"))
	{
		// Get id from file
		if (c_id = DefFileGetID(szFilename))
			// Get loaded def or try to load def from file
			if ((cdef = C4Id2Def(c_id))
				|| (Defs.Load(szFilename, C4D_Load_RX, Config.General.LanguageEx, &*Application.SoundSystem) && (cdef = C4Id2Def(c_id))))
			{
				return DropDef(c_id, iX, iY);
			}
		// Failure
		Console.Out(FormatString(LoadResStr("IDS_CNS_DROPNODEF"), GetFilename(szFilename)).getData());
		return false;
	}
	return false;
}

bool C4Game::DropDef(C4ID id, int32_t iX, int32_t iY)
{
	// def exists?
	if (C4Id2Def(id))
	{
		Control.DoInput(CID_EMDropDef, new C4ControlEMDropDef(id, iX, iY), CDT_Decide);
		return true;
	}
	else
	{
		// Failure
		Console.Out(FormatString(LoadResStr("IDS_CNS_DROPNODEF"), C4IdText(id)).getData());
	}
	return false;
}

bool C4Game::EnumerateMaterials()
{
	// Check material number
	if (Material.Num > C4MaxMaterial)
	{
		LogFatal(LoadResStr("IDS_PRC_TOOMANYMATS")); return false;
	}

	// Get hardcoded system material indices
	MVehic = Material.Get("Vehicle"); MCVehic = Mat2PixColDefault(MVehic);
	MTunnel = Material.Get("Tunnel");
	MWater = Material.Get("Water");
	MSnow = Material.Get("Snow");
	MGranite = Material.Get("Granite");

	const std::string_view material{C4S.Landscape.Material};

	if (const std::size_t pos = material.find('-'); pos != std::string_view::npos)
	{
		if (CompareVersion(C4S.Head.C4XVer[0], C4S.Head.C4XVer[1], C4S.Head.C4XVer[2], C4S.Head.C4XVer[3], C4S.Head.C4XVer[4], 4, 9, 10, 15, 359) == -1)
		{
			if (!C4S.Landscape.InEarth.IsClear() || !C4S.Animals.EarthNest.IsClear())
			{
				DebugLogF("Scenario.txt: Material=%s specifies a texture, which breaks InEarth and Nest before [359]. Version=4,9,10,15,359 or higher enables the fixed behavior.", C4S.Landscape.Material);
			}

			MEarth = MNone;
		}
		else
		{
			MEarth = Material.Get(std::string{material.substr(0, pos)}.c_str());
		}
	}
	else
	{
		MEarth = Material.Get(C4S.Landscape.Material);
	}

	if ((MVehic == MNone) || (MTunnel == MNone))
	{
		LogFatal(LoadResStr("IDS_PRC_NOSYSMATS")); return false;
	}
	// mapping to landscape palette will occur when landscape has been created
	// set the pal
	GraphicsSystem.SetPalette();

	return true;
}

void C4Game::CastObjects(C4ID id, C4Object *pCreator, int32_t num, int32_t level, int32_t tx, int32_t ty, int32_t iOwner, int32_t iController)
{
	int32_t cnt;
	for (cnt = 0; cnt < num; cnt++)
	{
		// force argument evaluation order
		const auto r4 = itofix(Random(3) + 1);
		const auto r3 = FIXED10(Random(2 * level + 1) - level);
		const auto r2 = FIXED10(Random(2 * level + 1) - level);
		const auto r1 = Random(360);
		CreateObject(id, pCreator, iOwner, tx, ty, r1, r2, r3, r4, iController);
	}
}

void C4Game::BlastCastObjects(C4ID id, C4Object *pCreator, int32_t num, int32_t tx, int32_t ty, int32_t iController)
{
	int32_t cnt;
	for (cnt = 0; cnt < num; cnt++)
	{
		// force argument evaluation order
		const auto r4 = itofix(Random(3) + 1);
		const auto r3 = FIXED10(Random(61) - 40);
		const auto r2 = FIXED10(Random(61) - 30);
		const auto r1 = Random(360);
		CreateObject(id, pCreator, NO_OWNER, tx, ty, r1, r2, r3, r4, iController);
	}
}

void C4Game::Sec1Timer()
{
	// updates the game clock
	if (TimeGo) { Time++; TimeGo = false; }
	FPS = cFPS; cFPS = 0;
}

void C4Game::Default()
{
	PointersDenumerated = false;
	IsRunning = false;
	FrameCounter = 0;
	GameOver = GameOverDlgShown = false;
	ScenarioFilename[0] = 0;
	PlayerFilenames[0] = 0;
	DefinitionFilenames.clear();
	FixedDefinitions = false;
	DirectJoinAddress[0] = 0;
	pJoinReference = nullptr;
	Parameters.ScenarioTitle.Ref("Loading...");
	HaltCount = 0;
	Evaluated = false;
	Verbose = false;
	TimeGo = false;
	Time = 0;
	StartTime = 0;
	InitProgress = 0; LastInitProgress = 0;
	FPS = cFPS = 0;
	fScriptCreatedObjects = false;
	fLobby = fObserve = false;
	iLobbyTimeout = 0;
	iTick2 = iTick3 = iTick5 = iTick10 = iTick35 = iTick255 = iTick500 = iTick1000 = 0;
	ObjectEnumerationIndex = 0;
	FullSpeed = false;
	FrameSkip = 1; DoSkipFrame = false;
	PreloadStatus = PreloadLevel::None;
	Defs.Clear();
	Material.Default();
	Objects.Default();
	BackObjects.Default();
	ForeObjects.Default();
	Players.Default();
	Weather.Default();
	Landscape.Default();
	TextureMap.Default();
	Rank.Default();
	MassMover.Default();
	PXS.Default();
	GraphicsSystem.Default();
	C4S.Default();
	Messages.Clear();
	MessageInput.Default();
	Info.Default();
	Title.Default();
	Names.Default();
	GameText.Default();
	MainSysLangStringTable.Default();
	ScenarioLangStringTable.Default();
	ScenarioSysLangStringTable.Default();
	Script.Default();
	GraphicsResource.Default();
	Control.Default();
	MouseControl.Default();
	PathFinder.Default();
	TransferZones.Default();
	GroupSet.Default();
	pParentGroup = nullptr;
	pGUI = nullptr;
	pScenarioSections = pCurrentScenarioSection = nullptr;
	*CurrentScenarioSection = 0;
	pGlobalEffects = nullptr;
	fResortAnyObject = false;
	pNetworkStatistics = nullptr;
	IsMusicEnabled = false;
	iMusicLevel = 100;
	PlayList.Clear();
}

void C4Game::Evaluate()
{
	// League game?
	bool fLeague = Network.isEnabled() && Network.isHost() && Parameters.isLeague();

	// Stop record
	StdStrBuf RecordName; uint8_t RecordSHA[StdSha1::DigestLength];
	if (Control.isRecord())
		Control.StopRecord(&RecordName, fLeague ? RecordSHA : nullptr);

	// Send league result
	if (fLeague)
		Network.LeagueGameEvaluate(RecordName.getData(), RecordSHA);

	// Players
	// saving local players only, because remote players will probably not rejoin after evaluation anyway)
	Players.Evaluate();
	Players.Save(true);

	// Round results
	RoundResults.EvaluateGame();

	// Set game flag
	Log(LoadResStr("IDS_PRC_EVALUATED"));
	Evaluated = true;
}

void C4Game::DrawCursors(C4FacetEx &cgo, int32_t iPlayer)
{
	const auto scale = Application.GetScale();
	const auto inverseScale = 1 / scale;
	C4DrawTransform transform;
	transform.SetMoveScale(0.f, 0.f, inverseScale, inverseScale);
	// Draw cursor mark arrow & cursor object name
	C4Object *cursor;
	C4Facet &fctCursor = GraphicsResource.fctCursor;
	for (C4Player *pPlr = Players.First; pPlr; pPlr = pPlr->Next)
		if (pPlr->Number == iPlayer || iPlayer == NO_OWNER)
			if (pPlr->CursorFlash || pPlr->SelectFlash)
				if (pPlr->Cursor)
				{
					cursor = pPlr->Cursor;
					if (Inside<int32_t>(cursor->x - fctCursor.Wdt / 2 - cgo.TargetX, 1 - fctCursor.Wdt, cgo.Wdt) && Inside<int32_t>(cursor->y - cursor->Def->Shape.Hgt / 2 - fctCursor.Hgt - cgo.TargetY, 1 - fctCursor.Hgt, cgo.Hgt))
					{
						int32_t cox = cgo.X + cursor->x - cgo.TargetX;
						int32_t coy = cgo.Y + cursor->y - cgo.TargetY;
						int32_t cphase = (cursor->Contained ? 1 : 0);
						fctCursor.DrawT(cgo.Surface, static_cast<int>(cox * scale) - fctCursor.Wdt / 2, static_cast<int>(coy * scale) - cursor->Def->Shape.Hgt / 2 - fctCursor.Hgt, cphase, 0, &transform, true);
						if (cursor->Info)
						{
							std::string text{cursor->GetName()};
							int32_t texthgt = GraphicsResource.FontRegular.GetLineHeight();
							if (cursor->Info->Rank > 0)
							{
								text = FormatString("%s|%s", cursor->Info->sRankName.getData(), cursor->GetName()).getData();
								texthgt += texthgt;
							}

							Application.DDraw->TextOut(text.c_str(), GraphicsResource.FontRegular, 1.0, cgo.Surface,
								cox,
								coy - cursor->Def->Shape.Hgt / 2 - static_cast<int>(fctCursor.Hgt / scale) - 2 - texthgt,
								0xffff0000, ACenter);
						}
					}
				}
}

void C4Game::Ticks()
{
	// Frames
	FrameCounter++; GameGo = FullSpeed;
	// Ticks
	if (++iTick2 == 2)       iTick2 = 0;
	if (++iTick3 == 3)       iTick3 = 0;
	if (++iTick5 == 5)       iTick5 = 0;
	if (++iTick10 == 10)     iTick10 = 0;
	if (++iTick35 == 35)     iTick35 = 0;
	if (++iTick255 == 255)   iTick255 = 0;
	if (++iTick500 == 500)   iTick500 = 0;
	if (++iTick1000 == 1000) iTick1000 = 0;
	// FPS / time
	cFPS++; TimeGo = true;
	// Frame skip
	if (FrameCounter % FrameSkip) DoSkipFrame = true;
	// Control
	Control.Ticks();
	// Full speed
	if (GameGo) Application.NextTick(false); // short-circuit the timer
	// statistics
	if (pNetworkStatistics) pNetworkStatistics->ExecuteFrame();
}

bool C4Game::Compile(const char *szSource)
{
	if (!szSource) return true;
	// C4Game is not defaulted on compilation.
	// Loading of runtime data overrides only certain values.
	// Doesn't compile players; those will be done later
	CompileSettings Settings(false, false, true);
	if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(
		mkParAdapt(*this, Settings),
		StdStrBuf(szSource, false),
		C4CFN_Game))
		return false;
	return true;
}

void C4Game::CompileFunc(StdCompiler *pComp, CompileSettings comp)
{
	if (!comp.fScenarioSection && comp.fExact)
	{
		const auto name = pComp->Name("Game");
		pComp->Value(mkNamingAdapt(Time,                                    "Time",                   0));
		pComp->Value(mkNamingAdapt(FrameCounter,                            "Frame",                  0));
		pComp->Value(mkNamingAdapt(Control.ControlTick,                     "ControlTick",            0));
		pComp->Value(mkNamingAdapt(Control.SyncRate,                        "SyncRate",               C4SyncCheckRate));
		pComp->Value(mkNamingAdapt(iTick2,                                  "Tick2",                  0));
		pComp->Value(mkNamingAdapt(iTick3,                                  "Tick3",                  0));
		pComp->Value(mkNamingAdapt(iTick5,                                  "Tick5",                  0));
		pComp->Value(mkNamingAdapt(iTick10,                                 "Tick10",                 0));
		pComp->Value(mkNamingAdapt(iTick35,                                 "Tick35",                 0));
		pComp->Value(mkNamingAdapt(iTick255,                                "Tick255",                0));
		pComp->Value(mkNamingAdapt(iTick500,                                "Tick500",                0));
		pComp->Value(mkNamingAdapt(iTick1000,                               "Tick1000",               0));
		pComp->Value(mkNamingAdapt(ObjectEnumerationIndex,                  "ObjectEnumerationIndex", 0));
		pComp->Value(mkNamingAdapt(Rules,                                   "Rules",                  0));
		pComp->Value(mkNamingAdapt(PlayList,                                "PlayList",               ""));
		pComp->Value(mkNamingAdapt(mkStringAdaptMA(CurrentScenarioSection), "CurrentScenarioSection", ""));
		pComp->Value(mkNamingAdapt(fResortAnyObject,                        "ResortAnyObj",           false));
		pComp->Value(mkNamingAdapt(IsMusicEnabled,                          "MusicEnabled",           false));
		pComp->Value(mkNamingAdapt(iMusicLevel,                             "MusicLevel",             100));
		pComp->Value(mkNamingAdapt(NextMission,                             "NextMission",            StdStrBuf()));
		pComp->Value(mkNamingAdapt(NextMissionText,                         "NextMissionText",        StdStrBuf()));
		pComp->Value(mkNamingAdapt(NextMissionDesc,                         "NextMissionDesc",        StdStrBuf()));
		pComp->Value(mkNamingAdapt(mkSTLMapAdapt(MessageInput.Commands),    "MessageBoardCommands",   std::unordered_map<std::string, C4MessageBoardCommand>{}));
	}

	pComp->Value(mkNamingAdapt(mkInsertAdapt(Script, ScriptEngine), "Script"));

	if (comp.fExact)
	{
		pComp->Value(mkNamingAdapt(Weather,       "Weather"));
		pComp->Value(mkNamingAdapt(Landscape,     "Landscape"));
		pComp->Value(mkNamingAdapt(Landscape.Sky, "Sky"));
	}

	pComp->Value(mkNamingAdapt(mkNamingPtrAdapt(pGlobalEffects, "GlobalEffects"), "Effects"));

	// scoreboard compiles into main level [Scoreboard]
	if (!comp.fScenarioSection && comp.fExact)
		pComp->Value(mkNamingAdapt(Scoreboard, "Scoreboard"));
	if (comp.fPlayers)
	{
		assert(pComp->isDecompiler());
		// player parsing: Parse all players
		// This doesn't create any players, but just parses existing by their ID
		// Primary player ininitialization (also setting ID) is done by player info list
		// Won't work this way for binary mode!
		for (C4Player *pPlr = Players.First; pPlr; pPlr = pPlr->Next)
			pComp->Value(mkNamingAdapt(*pPlr, FormatString("Player%d", pPlr->ID).getData()));
	}
}

void SetClientPrefix(char *szFilename, const char *szClient);

bool C4Game::Decompile(StdStrBuf &rBuf, bool fSaveSection, bool fSaveExact)
{
	// Decompile (without players for scenario sections)
	rBuf.Take(DecompileToBuf<StdCompilerINIWrite>(mkParAdapt(*this, CompileSettings(fSaveSection, !fSaveSection && fSaveExact, fSaveExact))));
	return true;
}

bool C4Game::Preload()
{
	if (CanPreload())
	{
#ifndef USE_CONSOLE
		std::unique_ptr<CStdGLCtx> context{lpDDraw->CreateContext(Application.pWindow, &Application)};
		if (!context)
		{
			return false;
		}

		context->Deselect();
		pGL->GetMainCtx().Select();
		PreloadThread = C4Thread::Create({"PreloadThread"}, [](std::unique_ptr<CStdGLCtx> preloadContext)
		{
			preloadContext->Select(false, true);
			CStdLock lock{&Game.PreloadMutex};
			Game.InitGameFirstPart() && Game.InitGameSecondPart(Game.ScenarioFile, nullptr, true, true);
			preloadContext->Finish();
			preloadContext->Deselect(true);
		},
		std::move(context));
#else
		PreloadThread = C4Thread::Create({"PreloadThread"}, []
		{
			CStdLock lock{&Game.PreloadMutex};
			Game.InitGameFirstPart() && Game.InitGameSecondPart(Game.ScenarioFile, nullptr, true, true);
		});
#endif

		return true;
	}

	return false;
}

bool C4Game::CanPreload() const
{
	return PreloadStatus <= PreloadLevel::Scenario && !PreloadThread.joinable();
}

bool C4Game::CompileRuntimeData(C4ComponentHost &rGameData)
{
	// Compile
	if (!Compile(rGameData.GetData())) return false;
	// Music System: Set play list
	Application.MusicSystem->SetPlayList(PlayList.getData());
	// Success
	return true;
}

bool C4Game::SaveData(C4Group &hGroup, bool fSaveSection, bool fInitial, bool fSaveExact)
{
	// Enumerate pointers & strings
	if (PointersDenumerated)
	{
		Players.EnumeratePointers();
		ScriptEngine.Strings.EnumStrings();
		if (pGlobalEffects) pGlobalEffects->EnumeratePointers();
	}

	// Decompile
	StdStrBuf Buf;
	if (!Decompile(Buf, fSaveSection, fSaveExact))
		return false;

	// Denumerate pointers, if game is in denumerated state
	if (PointersDenumerated)
	{
		ScriptEngine.DenumerateVariablePointers();
		Players.DenumeratePointers();
		if (pGlobalEffects) pGlobalEffects->DenumeratePointers();
	}

	// Initial?
	if (fInitial && GameText.GetData())
	{
		// HACK: Reinsert player sections, if any.
		const char *pPlayerSections = strstr(GameText.GetData(), "[Player");
		if (pPlayerSections)
		{
			Buf.Append("\r\n\r\n");
			Buf.Append(pPlayerSections);
		}
	}

	// Empty? All default; just remove from group then
	if (!Buf.getLength())
	{
		hGroup.Delete(C4CFN_Game);
		return true;
	}

	// Save
	return hGroup.Add(C4CFN_Game, Buf, false, true);
}

bool C4Game::SaveGameTitle(C4Group &hGroup)
{
	// Game not running
	if (!FrameCounter)
	{
		char *bpBytes; size_t iSize;
		if (ScenarioFile.LoadEntry(C4CFN_ScenarioTitle, &bpBytes, &iSize))
			hGroup.Add(C4CFN_ScenarioTitle, bpBytes, iSize, false, true);
		if (ScenarioFile.LoadEntry(C4CFN_ScenarioTitlePNG, &bpBytes, &iSize))
			hGroup.Add(C4CFN_ScenarioTitlePNG, bpBytes, iSize, false, true);
	}

	// Fullscreen screenshot
	else if (Application.isFullScreen && Application.Active)
	{
		constexpr std::int32_t surfaceWidth{200};
		constexpr std::int32_t surfaceHeight{150};

		const auto surface = std::make_unique<C4Surface>(surfaceWidth, surfaceHeight);

		// Fullscreen
		Application.DDraw->Blit(Application.DDraw->lpBack,
			0.0f, 0.0f, float(Application.DDraw->lpBack->Wdt), float(Application.DDraw->lpBack->Hgt),
			surface.get(), 0, 0, surfaceWidth, surfaceHeight);

		if (!surface->SavePNG(Config.AtTempPath(C4CFN_TempTitle), false, true, false))
		{
			return false;
		}

		if (!hGroup.Move(Config.AtTempPath(C4CFN_TempTitle), C4CFN_ScenarioTitlePNG))
		{
			return false;
		}
	}

	return true;
}

bool C4Game::DoKeyboardInput(C4KeyCode vk_code, C4KeyEventType eEventType, bool fAlt, bool fCtrl, bool fShift, bool fRepeated, class C4GUI::Dialog *pForDialog, bool fPlrCtrlOnly)
{
#ifdef USE_X11
	static std::map<C4KeyCode, bool> PressedKeys;
	// Keyrepeats are send as down, down, ..., down, up, where all downs are not distinguishable from the first.
	if (eEventType == KEYEV_Down)
	{
		if (PressedKeys[vk_code]) fRepeated = true;
		else PressedKeys[vk_code] = true;
	}
	else if (eEventType == KEYEV_Up)
	{
		PressedKeys[vk_code] = false;
	}
#endif
	// compose key
	C4KeyCodeEx Key(vk_code, C4KeyShiftState(fAlt * KEYS_Alt + fCtrl * KEYS_Control + fShift * KEYS_Shift), fRepeated);
	// compose keyboard scope
	uint32_t InScope = 0;
	if (fPlrCtrlOnly)
		InScope = KEYSCOPE_Control;
	else
	{
		if (IsRunning) InScope = KEYSCOPE_Generic;
		// if GUI has keyfocus, this overrides regular controls
		if (pGUI && (pGUI->HasKeyboardFocus() || pForDialog))
		{
			InScope |= KEYSCOPE_Gui;
			// control to console mode dialog: Make current keyboard target the active dlg,
			// so it can process input
			if (pForDialog) pGUI->ActivateDialog(pForDialog);
			// any keystroke in GUI resets tooltip times
			pGUI->KeyAny();
		}
		else
		{
			if (Application.isFullScreen)
			{
				if (FullScreen.pMenu && FullScreen.pMenu->IsActive()) // fullscreen menu
					InScope |= KEYSCOPE_FullSMenu;
				else if (C4S.Head.Replay && C4S.Head.Film) // film view only
					InScope |= KEYSCOPE_FilmView;
				else if (GraphicsSystem.GetViewport(NO_OWNER)) // NO_OWNER-viewport-controls
					InScope |= KEYSCOPE_FreeView;
				else
				{
					// regular player viewport controls
					InScope |= KEYSCOPE_FullSView;
					// player controls disabled during round over dlg
					if (!C4GameOverDlg::IsShown()) InScope |= KEYSCOPE_Control;
				}
			}
			else
				// regular player viewport controls
				InScope |= KEYSCOPE_Control;
		}
		// fullscreen/console (in running game)
		if (IsRunning)
		{
			if (FullScreen.Active) InScope |= KEYSCOPE_Fullscreen;
			if (Console.Active) InScope |= KEYSCOPE_Console;
		}
	}
	// okay; do input
	if (KeyboardInput.DoInput(Key, eEventType, InScope))
		return true;

	// unprocessed key
	return false;
}

bool C4Game::CanQuickSave()
{
	if (Network.isEnabled())
	{
		// Network hosts only
		if (!Network.isHost())
		{
			Log(LoadResStr("IDS_GAME_NOCLIENTSAVE")); return false;
		}

		// Not for league games
		if (Parameters.isLeague() && !Control.isReplay())
		{
			Log("[!] It's not allowed to save running league games"); return false;
		}
	}

	return true;
}

bool C4Game::QuickSave(const char *strFilename, const char *strTitle, bool fForceSave)
{
	// Check
	if (!fForceSave) if (!CanQuickSave()) return false;

	// Set working directory (should already be in exe path anyway - just to make sure...?)
	SetWorkingDirectory(Config.General.ExePath);

	// Create savegame folder
	if (!Config.General.CreateSaveFolder(Config.General.SaveGameFolder.getData(), LoadResStr("IDS_GAME_SAVEGAMESTITLE")))
	{
		Log(LoadResStr("IDS_GAME_FAILSAVEGAME")); return false;
	}

	// Create savegame subfolder(s)
	char strSaveFolder[_MAX_PATH + 1];
	for (int i = 0; i < SCharCount(DirectorySeparator, strFilename); i++)
	{
		SCopy(Config.General.SaveGameFolder.getData(), strSaveFolder); AppendBackslash(strSaveFolder);
		SCopyUntil(strFilename, strSaveFolder + SLen(strSaveFolder), DirectorySeparator, _MAX_PATH, i);
		if (!Config.General.CreateSaveFolder(strSaveFolder, strTitle))
		{
			Log(LoadResStr("IDS_GAME_FAILSAVEGAME")); return false;
		}
	}

	// Compose savegame filename
	StdStrBuf strSavePath;
	strSavePath.Format("%s%c%s", Config.General.SaveGameFolder.getData(), DirectorySeparator, strFilename);

	// Must not be the scenario file that is currently open
	if (ItemIdentical(ScenarioFilename, strSavePath.getData()))
	{
		StartSoundEffect("Error");
		GameMsgGlobal(LoadResStr("IDS_GAME_NOSAVEONCURR"), FRed);
		Log(LoadResStr("IDS_GAME_FAILSAVEGAME"));
		return false;
	}

	// Wait message
	Log(LoadResStr("IDS_HOLD_SAVINGGAME"));
	GraphicsSystem.MessageBoard.EnsureLastMessage();

	// Save to target scenario file
	C4GameSave *pGameSave;
	pGameSave = new C4GameSaveSavegame();
	if (!pGameSave->Save(strSavePath.getData()))
	{
		Log(LoadResStr("IDS_GAME_FAILSAVEGAME")); delete pGameSave; return false;
	}
	delete pGameSave;

	// Success
	Log(LoadResStr("IDS_CNS_GAMESAVED"));
	return true;
}

bool LandscapeFree(int32_t x, int32_t y)
{
	if (!Inside<int32_t>(x, 0, GBackWdt - 1) || !Inside<int32_t>(y, 0, GBackHgt - 1)) return false;
	return !DensitySolid(GBackDensity(x, y));
}

void C4Game::ReloadFile(const char *const path)
{
	if (Network.isEnabled()) return;

	const char *const relativePath{Config.AtExeRelativePath(path)};

	if (C4Def *const def{Defs.GetByPath(relativePath)}; def)
	{
		ReloadDef(def->id);
	}
	else
	{
		ScriptEngine.ReloadScript(relativePath, &Defs);
	}
}

bool C4Game::ReloadDef(C4ID id, uint32_t reloadWhat)
{
	bool fSucc;
	// not in network
	if (Network.isEnabled()) return false;
	// syncronize (close menus with dead surfaces, etc.)
	// no need to sync back player files, though
	Synchronize(false);
	// reload def
	C4ObjectLink *clnk;
	C4Def *pDef = Defs.ID2Def(id);
	if (!pDef) return false;
	// Message
	LogF("Reloading %s from %s", C4IdText(pDef->id), GetFilename(pDef->Filename));
	// Reload def
	if (Defs.Reload(pDef, reloadWhat, Config.General.LanguageEx, &*Application.SoundSystem))
	{
		// Success, update all concerned object faces
		// may have been done by graphics-update already - but not for objects using graphics of another def
		// better update everything :)
		for (clnk = Objects.First; clnk && clnk->Obj; clnk = clnk->Next)
		{
			if (clnk->Obj->id == id)
				clnk->Obj->UpdateFace(true);
		}
		fSucc = true;
	}
	else
	{
		// Failure, remove all objects of this type
		for (clnk = Objects.First; clnk && clnk->Obj; clnk = clnk->Next)
			if (clnk->Obj->id == id)
				clnk->Obj->AssignRemoval();
		// safety: If a removed def is being profiled, profiling must stop
		C4AulProfiler::Abort();
		// Kill def
		Defs.Remove(pDef);
		// Log
		Log("Reloading failure. All objects of this type removed.");
		fSucc = false;
	}
	// update game messages
	Messages.UpdateDef(id);
	// done
	return fSucc;
}

bool C4Game::ReloadParticle(const char *szName)
{
	// not in network
	if (Network.isEnabled()) return false;
	// safety
	if (!szName) return false;
	// get particle def
	C4ParticleDef *pDef = Particles.GetDef(szName);
	if (!pDef) return false;
	// verbose
	LogF("Reloading particle %s from %s", pDef->Name.getData(), GetFilename(pDef->Filename.getData()));
	// reload it
	if (!pDef->Reload())
	{
		// safer: remove all particles
		ParticleSystem.ClearParticles();
		// clear def
		delete pDef;
		// log
		Log("Reloading failure. All particles removed.");
		// failure
		return false;
	}
	// success
	return true;
}

bool C4Game::InitGame(C4Group &hGroup, C4ScenarioSection *section, bool fLoadSky)
{
	const CStdLock lock{&PreloadMutex};
	{
		if (!section)
		{
			RestartRestoreInfos.Clear();

			C4PlayerInfo *info;
			for (int32_t i = 0; info = PlayerInfos.GetPlayerInfoByIndex(i); ++i)
			{
				if (!info->IsRemoved() && !info->IsInvisible())
				{
					RestartRestoreInfos.Players.emplace(std::string{info->GetName()}, C4NetworkRestartInfos::Player{std::string{info->GetName()}, info->GetType(), info->GetColor(), info->GetTeam()});
				}
			}

			// file monitor
			if (Config.Developer.AutoFileReload && !Application.isFullScreen && !FileMonitor)
			{
				try
				{
					FileMonitor = std::make_unique<C4FileMonitor>(std::bind(&C4Game::ReloadFile, this, std::placeholders::_1));
				}
				catch (const std::runtime_error &e)
				{
					LogF(LoadResStr("IDS_ERR_FILEMONITOR"), e.what());
				}
			}

			CStdLock lock{&PreloadMutex};
			if (!InitGameFirstPart()) return false;

			// join local players for regular games
			// should be done before record/replay is initialized, so the players are stored in PlayerInfos.txt
			// for local savegame resumes, players are joined into PlayerInfos and later associated in InitPlayers
			if (!Network.isEnabled())
			{
				PlayerInfos.InitLocal();
			}

			// for replays, make sure teams are assigned correctly
			if (C4S.Head.Replay)
			{
				PlayerInfos.RecheckAutoGeneratedTeams(); // checks that all teams used in playerinfos exist
				Teams.RecheckPlayers();                  // syncs player list of teams with teams set in PlayerInfos
			}

			for (const auto &def : Parameters.GameRes.iterRes(NRT_Definitions))
			{
				auto group = std::make_unique<C4Group>();
				if (!group->Open(def.getFile()))
				{
					LogFatal(FormatString(LoadResStr("IDS_ERR_OPENRES"), def.getFile(), group->GetError()).getData());
					return false;
				}

				GroupSet.RegisterGroup(*group.release(), true, C4GSPrio_Definitions, C4GSCnt_DefinitionRoot, true);
			}

			// Graphics and fonts (may reinit main font, too)
			// Call it here for overloads by C4GroupSet (definitions, Extra.c4g, scenario, folders etc.)
			Log(LoadResStr("IDS_PRC_GFXRES"));
			if (!GraphicsResource.Init())
			{
				return false;
			}

			SetInitProgress(10);
		}

		// determine startup player count
		if (!FrameCounter) Parameters.StartupPlayerCount = PlayerInfos.GetStartupCount();

		// The Landscape is the last long chunk of loading time, so it's a good place to start the music fadeout
		if (!section) Application.MusicSystem->Stop(2000);

		if (!InitGameSecondPart(hGroup, section, fLoadSky, false))
		{
			return false;
		}
	}

	if (!section)
	{
		// set up control (inits Record/Replay)
		if (!InitControl())
		{
			LogFatal("IDS_ERR_INITCONTROL");
			return false;
		}
	}

	// Load round results
	if (!section)
		if (hGroup.FindEntry(C4CFN_RoundResults))
		{
			if (!RoundResults.Load(hGroup, C4CFN_RoundResults))
			{
				LogFatal(LoadResStr("IDS_ERR_ERRORLOADINGROUNDRESULTS")); return false;
			}
		}
		else
		{
			RoundResults.Init();
		}

	// Denumerate game data pointers
	if (!section) ScriptEngine.DenumerateVariablePointers();
	if (!section && pGlobalEffects) pGlobalEffects->DenumeratePointers();

	// Check object enumeration
	if (!CheckObjectEnumeration())
	{
		LogFatal(LoadResStr("IDS_ERR_CHECKOBJECTENUMERATION"));
		return false;
	}

	// Okay; everything in denumerated state from now on
	PointersDenumerated = true;

	for (const auto &def : Defs)
	{
		def->Script.Call(PSF_InitializeDef, {section ? C4VString(section->GetName()) : C4VNull});
	}

	// Environment
	if (!C4S.Head.NoInitialize && LandscapeLoaded)
	{
		Log(LoadResStr("IDS_PRC_ENVIRONMENT"));
		InitVegetation();
		InitInEarth();
		InitAnimals();
		InitEnvironment();
		InitRules();
		InitGoals();
		Landscape.PostInitMap();
	}
	SetInitProgress(94);

	// Weather
	if (LandscapeLoaded) Weather.Init(!C4S.Head.SaveGame);
	SetInitProgress(95);

	// FoW-color
	FoWColor = C4S.Game.FoWColor;

	// goal objects exist, but no GOAL? create it
	if (!C4S.Head.SaveGame)
		if (Objects.ObjectsInt().ObjectCount(C4ID_None, C4D_Goal))
			if (!Objects.FindInternal(C4Id("GOAL")))
				CreateObject(C4Id("GOAL"), nullptr);
	SetInitProgress(96);

	// close any gfx groups, because they are no longer needed (after sky is initialized)
	GraphicsResource.CloseFiles();

	if (!section)
	{
		// Music
		Application.MusicSystem->PlayScenarioMusic(ScenarioFile);
		SetMusicLevel(iMusicLevel);
		SetInitProgress(97);
	}
	return true;
}

bool C4Game::InitGameFirstPart()
{
	if (PreloadStatus >= PreloadLevel::Basic)
	{
		return true;
	}

	if (PreloadStatus == PreloadLevel::None && NetworkActive && !Network.isHost())
	{
		// get scenario
		SetInitProgress(6);
		char scenario[_MAX_PATH + 1];

		if (!Network.RetrieveScenario(scenario))
		{
			LogFatal(LoadResStr("IDS_ERR_RETRIEVESCENARIO"));
			return false;
		}

		// open new scenario
		SCopy(scenario, ScenarioFilename, _MAX_PATH);
		if (!OpenScenario()) return false;
		TempScenarioFile = true;

		// get everything else
		if (!Parameters.GameRes.RetrieveFiles())
		{
			LogFatal(LoadResStr("IDS_ERR_RETRIEVEFILES"));
			return false;
		}

		// Check network game data scenario type (safety)
		if (!C4S.Head.NetworkGame)
		{
			LogFatal(LoadResStr("IDS_NET_NONETGAME")); return false;
		}

		SetInitProgress(7);
	}

	// system scripts
	if (!InitScriptEngine())
	{
		LogFatal(LoadResStr("IDS_ERR_INITSCRIPTENGINE"));
		return false;
	}

	SetInitProgress(8);

	// Scenario components;
	if (!LoadScenarioComponents())
	{
		return false;
	}

	// Definitions
	if (!InitDefs())
	{
		LogFatal(LoadResStr("IDS_ERR_INITDEFS"));
		return false;
	}

	SetInitProgress(40);

	// Scenario scripts (and local System.c4g)
	// After defs to get overloading priority
	LoadScenarioScripts();

	SetInitProgress(56);

	// Link scripts
	LinkScriptEngine();

	SetInitProgress(57);

	// Materials
	if (!InitMaterialTexture())
	{
		return false;
	}
	SetInitProgress(58);

	// Colorize defs by material
	Defs.ColorizeByMaterial(Material, GBM);
	SetInitProgress(60);

	PreloadStatus = PreloadLevel::Basic;

	return true;
}

bool C4Game::InitGameSecondPart(C4Group &hGroup, C4ScenarioSection *section, bool fLoadSky, bool preloading)
{
	if (!section)
	{
		if (PreloadStatus >= PreloadLevel::LandscapeObjects || (C4S.Landscape.MapPlayerExtend && preloading))
		{
			return true;
		}

		FixRandom(Parameters.RandomSeed);
	}

	// Landscape
	Log(LoadResStr("IDS_PRC_LANDSCAPE"));
	LandscapeLoaded = false;
	if (!Landscape.Init(hGroup, section, fLoadSky, LandscapeLoaded, !!C4S.Head.SaveGame))
	{
		LogFatal(LoadResStr("IDS_ERR_GBACK")); return false;
	}
	SetInitProgress(88);
	// the savegame flag is set if runtime data is present, in which case this is to be used
	// except for scenario sections
	if (LandscapeLoaded && (!C4S.Head.SaveGame || section))
		Landscape.ScenarioInit();
	SetInitProgress(89);
	// Init main object list
	Objects.Init(Landscape.Width, Landscape.Height);

	// Pathfinder
	if (!section) PathFinder.Init(&LandscapeFree, &TransferZones);
	SetInitProgress(90);

	// PXS
	if (hGroup.FindEntry(C4CFN_PXS))
	{
		if (!PXS.Load(hGroup))
		{
			LogFatal(LoadResStr("IDS_ERR_PXS")); return false;
		}
	}
	else if (LandscapeLoaded)
	{
		PXS.Clear();
		PXS.Default();
	}
	SetInitProgress(91);

	// MassMover
	if (hGroup.FindEntry(C4CFN_MassMover))
	{
		if (!MassMover.Load(hGroup))
		{
			LogFatal(LoadResStr("IDS_ERR_MOVER")); return false;
		}
	}
	else if (LandscapeLoaded)
	{
		// clear and set mass-mover to default values
		MassMover.Clear();
		MassMover.Default();
	}

	SetInitProgress(92);

	// definition value overloads
	if (!section) InitValueOverloads();

	Objects.Clear(!section);

	// Load objects
	int32_t iObjects = Objects.Load(hGroup, section);
	if (iObjects) { LogF(LoadResStr("IDS_PRC_OBJECTSLOADED"), iObjects); }
	SetInitProgress(93);

	if (!section)
	{
		PreloadStatus = PreloadLevel::LandscapeObjects;
	}

	return true;
}

bool C4Game::InitGameFinal()
{
	// Validate object owners & assign loaded info objects
	Objects.ValidateOwners();
	Objects.AssignInfo();
	Objects.AssignPlrViewRange(); // update FoW-repellers

	// Script constructor call
	int32_t iObjCount = Objects.ObjectCount();
	if (!C4S.Head.SaveGame) Script.Call(PSF_Initialize);
	if (Objects.ObjectCount() != iObjCount) fScriptCreatedObjects = true;

	// Player final init
	C4Player *pPlr;
	for (pPlr = Players.First; pPlr; pPlr = pPlr->Next)
		pPlr->FinalInit(!C4S.Head.SaveGame);

	// Create viewports
	for (pPlr = Players.First; pPlr; pPlr = pPlr->Next)
		if (pPlr->LocalControl)
			CreateViewport(pPlr->Number);
	// Check fullscreen viewports
	FullScreen.ViewportCheck();
	// update halt state
	Console.UpdateHaltCtrls(!!HaltCount);

	// Host: players without connected clients: remove via control queue
	if (Network.isEnabled() && Network.isHost())
		for (int32_t cnt = 0; cnt < Players.GetCount(); cnt++)
			if (Players.GetByIndex(cnt)->AtClient < 0)
				Players.Remove(Players.GetByIndex(cnt), true, false);

	// It should be safe now to reload stuff
	if (FileMonitor)
	{
		FileMonitor->StartMonitoring();
	}
	return true;
}

bool C4Game::InitScriptEngine()
{
	// engine functions
	InitFunctionMap(&ScriptEngine);

	// system functions: check if system group is open
	if (!Application.OpenSystemGroup())
	{
		LogFatal(LoadResStr("IDS_ERR_INVALIDSYSGRP")); return false;
	}
	C4Group &File = Application.SystemGroup;

	// Load string table
	MainSysLangStringTable.LoadEx("StringTbl", File, C4CFN_ScriptStringTbl, Config.General.LanguageEx);

	// get scripts
	char fn[_MAX_FNAME + 1] = { 0 };
	File.ResetSearch();
	while (File.FindNextEntry(C4CFN_ScriptFiles, fn, nullptr, nullptr, !!fn[0]))
	{
		// host will be destroyed by script engine, so drop the references
		C4ScriptHost *scr = new C4ScriptHost();
		scr->Reg2List(&ScriptEngine, &ScriptEngine);
		scr->Load(nullptr, File, fn, Config.General.LanguageEx, nullptr, &MainSysLangStringTable);
	}

	// load standard clonk names
	Names.Load(LoadResStr("IDS_CNS_NAMES"), File, C4CFN_Names);

	return true;
}

void C4Game::LinkScriptEngine()
{
	// Link script engine (resolve includes/appends, generate code)
	ScriptEngine.Link(&Defs);

	// Set name list for globals
	ScriptEngine.GlobalNamed.SetNameList(&ScriptEngine.GlobalNamedNames);
}

bool C4Game::InitPlayers()
{
	int32_t iPlrCnt = 0;

	if (C4S.Head.NetworkRuntimeJoin)
	{
		// Load players to restore from scenario
		C4PlayerInfoList LocalRestorePlayerInfos;
		LocalRestorePlayerInfos.Load(ScenarioFile, C4CFN_SavePlayerInfos, &ScenarioLangStringTable);
		// -- runtime join player restore
		// all restore functions will be executed on RestorePlayerInfos, because the main playerinfos may be more up-to-date
		// extract all players to temp store and update filenames to point there
		if (!LocalRestorePlayerInfos.RecreatePlayerFiles())
		{
			LogFatal(LoadResStr("IDS_ERR_NOPLRFILERECR")); return false;
		}
		// recreate the files
		if (!LocalRestorePlayerInfos.RecreatePlayers())
		{
			LogFatal(LoadResStr("IDS_ERR_NOPLRNETRECR")); return false;
		}
	}
	else if (RestorePlayerInfos.GetActivePlayerCount(true))
	{
		// -- savegame player restore
		// for savegames or regular scenarios with restore infos, the player info list should have been loaded from the savegame
		// or got restored from game text in OpenScenario()
		// merge restore player info into main player info list now
		// -for single-host games, this will move all infos
		// -for network games, it will merge according to savegame association done in the lobby
		// for all savegames, script players get restored by adding one new script player for earch savegame script player to the host
		if (!PlayerInfos.RestoreSavegameInfos(RestorePlayerInfos))
		{
			LogFatal(LoadResStr("IDS_ERR_NOPLRSAVEINFORECR")); return false;
		}
		RestorePlayerInfos.Clear();
		// try to associate local filenames (non-net+replay) or ressources (net) with all player infos
		if (!PlayerInfos.RecreatePlayerFiles())
		{
			LogFatal(LoadResStr("IDS_ERR_NOPLRFILERECR")); return false;
		}
		// recreate players by joining all players whose joined-flag is already set
		if (!PlayerInfos.RecreatePlayers())
		{
			LogFatal(LoadResStr("IDS_ERR_NOPLRSAVERECR")); return false;
		}
	}

	// any regular non-net non-replay game: Do the normal control queue join
	// this includes additional player joins in savegames
	if (!Network.isEnabled() && !Control.NoInput())
		if (!PlayerInfos.LocalJoinUnjoinedPlayersInQueue())
		{
			// error joining local players - either join was done earlier somehow,
			// or the player count check will soon end this round
		}

	// non-replay player joins will be done by player info list when go tick is reached
	// this is handled by C4Network2Players and needs no further treatment here
	// set iPlrCnt for player count check in host/single games
	iPlrCnt = PlayerInfos.GetJoinIssuedPlayerCount();

	// Check valid participating player numbers (host/single only)
	if (!Network.isEnabled() || (Network.isHost() && !fLobby))
	{
#ifndef USE_CONSOLE
		// No players in fullscreen
		if (iPlrCnt == 0)
			if (Application.isFullScreen && !Control.NoInput())
			{
				LogFatal(LoadResStr("IDS_CNS_NOFULLSCREENPLRS"));
				return false;
			}
#endif
		// Too many players
		if (iPlrCnt > Parameters.MaxPlayers)
		{
			if (Application.isFullScreen)
			{
				LogFatal(FormatString(LoadResStr("IDS_PRC_TOOMANYPLRS"), Parameters.MaxPlayers).getData());
				return false;
			}
			else
			{
				Console.Message(FormatString(LoadResStr("IDS_PRC_TOOMANYPLRS"), Parameters.MaxPlayers).getData());
			}
		}
	}
	// Console and no real players: halt
	if (Console.Active)
		if (!fLobby)
			if (!(PlayerInfos.GetActivePlayerCount(false) - PlayerInfos.GetActiveScriptPlayerCount(true, false)))
				++HaltCount;
	return true;
}

bool C4Game::InitControl()
{
	// Replay?
	if (C4S.Head.Replay)
	{
		// no joins
		PlayerFilenames[0] = 0;
		// start playback
		if (!Control.InitReplay(ScenarioFile))
			return false;
	}
	else if (Network.isEnabled())
	{
		// initialize
		if (!Control.InitNetwork(Clients.getLocal()))
			return false;
		// league?
		if (Parameters.isLeague())
		{
			// enforce league rules
			if (Network.isHost())
				if (!Parameters.CheckLeagueRulesStart(true))
					return false;
		}
	}
	// Otherwise: local game
	else
	{
		// init
		if (!Control.InitLocal(Clients.getLocal()))
			return false;
	}

	// record?
	if (!C4S.Head.Replay && (Config.General.Record || Parameters.isLeague()))
		if (!Control.StartRecord(true, Parameters.doStreaming()))
		{
			// Special: If this happens for a league host, the game must not start.
			if (Network.isEnabled() && Network.isHost() && Parameters.isLeague())
			{
				LogFatal(LoadResStr("IDS_ERR_NORECORD"));
				return false;
			}
			else
			{
				Log(LoadResStr("IDS_ERR_NORECORD"));
			}
		}

	return true;
}

int32_t ListExpandValids(C4IDList &rlist,
	C4ID *idlist, int32_t maxidlist)
{
	int32_t cnt, cnt2, ccount, cpos;
	for (cpos = 0, cnt = 0; rlist.GetID(cnt); cnt++)
		if (C4Id2Def(rlist.GetID(cnt, &ccount)))
			for (cnt2 = 0; cnt2 < ccount; cnt2++)
				if (cpos < maxidlist)
				{
					idlist[cpos] = rlist.GetID(cnt); cpos++;
				}
	return cpos;
}

bool C4Game::PlaceInEarth(C4ID id)
{
	int32_t cnt, tx, ty;
	for (cnt = 0; cnt < 35; cnt++) // cheap trys
	{
		tx = Random(GBackWdt); ty = Random(GBackHgt);
		if (GBackMat(tx, ty) == MEarth)
			if (CreateObject(id, nullptr, NO_OWNER, tx, ty, Random(360)))
				return true;
	}
	return false;
}

C4Object *C4Game::PlaceVegetation(C4ID id, int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, int32_t iGrowth)
{
	int32_t cnt, iTx, iTy, iMaterial;

	// Get definition
	C4Def *pDef;
	if (!(pDef = C4Id2Def(id))) return nullptr;

	// No growth specified: full or random growth
	if (iGrowth <= 0)
	{
		iGrowth = FullCon;
		if (pDef->Growth) if (!Random(3)) iGrowth = Random(FullCon) + 1;
	}

	// Place by placement type
	switch (pDef->Placement)
	{
	// Surface soil
	case C4D_Place_Surface:
		for (cnt = 0; cnt < 20; cnt++)
		{
			// Random hit within target area
			iTx = iX + Random(iWdt); iTy = iY + Random(iHgt);
			// Above IFT
			while ((iTy > 0) && GBackIFT(iTx, iTy)) iTy--;
			// Above semi solid
			if (!AboveSemiSolid(iTx, iTy) || !Inside<int32_t>(iTy, 50, GBackHgt - 50))
				continue;
			// Free above
			if (GBackSemiSolid(iTx, iTy - pDef->Shape.Hgt) || GBackSemiSolid(iTx, iTy - pDef->Shape.Hgt / 2))
				continue;
			// Free upleft and upright
			if (GBackSemiSolid(iTx - pDef->Shape.Wdt / 2, iTy - pDef->Shape.Hgt * 2 / 3) || GBackSemiSolid(iTx + pDef->Shape.Wdt / 2, iTy - pDef->Shape.Hgt * 2 / 3))
				continue;
			// Soil check
			iTy += 3; // two pix into ground
			iMaterial = GBackMat(iTx, iTy);
			if (iMaterial != MNone) if (Material.Map[iMaterial].Soil)
			{
				if (!pDef->Growth) iGrowth = FullCon;
				iTy += 5;
				return CreateObjectConstruction(id, nullptr, NO_OWNER, iTx, iTy, iGrowth);
			}
		}
		break;

	// Underwater
	case C4D_Place_Liquid:
		// Random range
		iTx = iX + Random(iWdt); iTy = iY + Random(iHgt);
		// Find liquid
		if (!FindSurfaceLiquid(iTx, iTy, pDef->Shape.Wdt, pDef->Shape.Hgt))
			if (!FindLiquid(iTx, iTy, pDef->Shape.Wdt, pDef->Shape.Hgt))
				return nullptr;
		// Liquid bottom
		if (!SemiAboveSolid(iTx, iTy)) return nullptr;
		iTy += 3;
		// Create object
		return CreateObjectConstruction(id, nullptr, NO_OWNER, iTx, iTy, iGrowth);
	}

	// Undefined placement type
	return nullptr;
}

C4Object *C4Game::PlaceAnimal(C4ID idAnimal)
{
	C4Def *pDef = C4Id2Def(idAnimal);
	if (!pDef) return nullptr;
	int32_t iX, iY;
	// Placement
	switch (pDef->Placement)
	{
	// Running free
	case C4D_Place_Surface:
		iX = Random(GBackWdt); iY = Random(GBackHgt);
		if (!FindSolidGround(iX, iY, pDef->Shape.Wdt)) return nullptr;
		break;
	// In liquid
	case C4D_Place_Liquid:
		iX = Random(GBackWdt); iY = Random(GBackHgt);
		if (!FindSurfaceLiquid(iX, iY, pDef->Shape.Wdt, pDef->Shape.Hgt))
			if (!FindLiquid(iX, iY, pDef->Shape.Wdt, pDef->Shape.Hgt))
				return nullptr;
		iY += pDef->Shape.Hgt / 2;
		break;
	// Floating in air
	case C4D_Place_Air:
		iX = Random(GBackWdt);
		for (iY = 0; (iY < GBackHgt) && !GBackSemiSolid(iX, iY); iY++);
		if (iY <= 0) return nullptr;
		iY = Random(iY);
		break;
	default:
		return nullptr;
	}
	// Create object
	return CreateObject(idAnimal, nullptr, NO_OWNER, iX, iY);
}

void C4Game::InitInEarth()
{
	const int32_t maxvid = 100;
	int32_t cnt, vidnum;
	C4ID vidlist[maxvid];
	// Amount
	int32_t amt = (GBackWdt * GBackHgt / 5000) * C4S.Landscape.InEarthLevel.Evaluate() / 100;
	// List all valid IDs from C4S
	vidnum = ListExpandValids(C4S.Landscape.InEarth, vidlist, maxvid);
	// Place
	if (vidnum > 0)
		for (cnt = 0; cnt < amt; cnt++)
			PlaceInEarth(vidlist[Random(vidnum)]);
}

void C4Game::InitVegetation()
{
	const int32_t maxvid = 100;
	int32_t cnt, vidnum;
	C4ID vidlist[maxvid];
	// Amount
	int32_t amt = (GBackWdt / 50) * C4S.Landscape.VegLevel.Evaluate() / 100;
	// Get percentage vidlist from C4S
	vidnum = ListExpandValids(C4S.Landscape.Vegetation, vidlist, maxvid);
	// Place vegetation
	if (vidnum > 0)
		for (cnt = 0; cnt < amt; cnt++)
			PlaceVegetation(vidlist[Random(vidnum)], 0, 0, GBackWdt, GBackHgt, -1);
}

void C4Game::InitAnimals()
{
	int32_t cnt, cnt2;
	C4ID idAnimal; int32_t iCount;
	// Place animals
	for (cnt = 0; (idAnimal = C4S.Animals.FreeLife.GetID(cnt, &iCount)); cnt++)
		for (cnt2 = 0; cnt2 < iCount; cnt2++)
			PlaceAnimal(idAnimal);
	// Place nests
	for (cnt = 0; (idAnimal = C4S.Animals.EarthNest.GetID(cnt, &iCount)); cnt++)
		for (cnt2 = 0; cnt2 < iCount; cnt2++)
			PlaceInEarth(idAnimal);
}

void C4Game::ParseCommandLine(const char *szCmdLine)
{
	Log("Command line: "); Log(szCmdLine);

	// Definitions by registry config
	DefinitionFilenames.clear();
	std::istringstream stream(Config.General.Definitions);
	for (std::string s; std::getline(stream, s, ';');)
	{
		DefinitionFilenames.push_back(s);
	}
	*PlayerFilenames = 0;
	NetworkActive = false;

	// Scan additional parameters from command line
	char szParameter[_MAX_PATH + 1];
	for (int32_t iPar = 0; SGetParameter(szCmdLine, iPar, szParameter, _MAX_PATH); iPar++)
	{
		// Scenario file
		if (SEqualNoCase(GetExtension(szParameter), "c4s"))
		{
			SCopy(szParameter, ScenarioFilename, _MAX_PATH); continue;
		}
		if (SEqualNoCase(GetFilename(szParameter), "scenario.txt"))
		{
			SCopy(szParameter, ScenarioFilename, _MAX_PATH);
			if (GetFilename(ScenarioFilename) != ScenarioFilename) *(GetFilename(ScenarioFilename) - 1) = 0;
			continue;
		}
		// Player file
		if (SEqualNoCase(GetExtension(szParameter), "c4p"))
		{
			SAddModule(PlayerFilenames, szParameter);
			continue;
		}
		// Definition file
		if (SEqualNoCase(GetExtension(szParameter), "c4d"))
		{
			DefinitionFilenames.push_back(szParameter);
			continue;
		}
		// Update file
		if (SEqualNoCase(GetExtension(szParameter), "c4u"))
		{
			Application.IncomingUpdate.Copy(szParameter);
			continue;
		}
		// record stream
		if (SEqualNoCase(GetExtension(szParameter), "c4r"))
		{
			RecordStream.Copy(szParameter);
		}
		// Fair Crew
		if (SEqualNoCase(szParameter, "/ncrw") || SEqualNoCase(szParameter, "/faircrew"))
			Config.General.FairCrew = true;
		// Trained Crew (Player Crew)
		if (SEqualNoCase(szParameter, "/ucrw") || SEqualNoCase(szParameter, "/trainedcrew"))
			Config.General.FairCrew = false;
		// record dump
		if (SEqual2NoCase(szParameter, "/recdump:"))
			RecordDumpFile.Copy(szParameter + 9);
		// record stream
		if (SEqual2NoCase(szParameter, "/stream:"))
			RecordStream.Copy(szParameter + 8);
		// startup start screen
		if (SEqual2NoCase(szParameter, "/startup:"))
			C4Startup::SetStartScreen(szParameter + 9);
		// Network
		if (SEqualNoCase(szParameter, "/network"))
			NetworkActive = true;
		if (SEqualNoCase(szParameter, "/nonetwork"))
			NetworkActive = false;
		// Signup
		if (SEqualNoCase(szParameter, "/signup"))
		{
			NetworkActive = true;
			Config.Network.MasterServerSignUp = true;
		}
		if (SEqualNoCase(szParameter, "/nosignup"))
			Config.Network.MasterServerSignUp = Config.Network.LeagueServerSignUp = false;
		// League
		if (SEqualNoCase(szParameter, "/league"))
		{
			NetworkActive = true;
			Config.Network.MasterServerSignUp = Config.Network.LeagueServerSignUp = true;
		}
		if (SEqualNoCase(szParameter, "/noleague"))
			Config.Network.LeagueServerSignUp = false;
		// Lobby
		if (SEqual2NoCase(szParameter, "/lobby"))
		{
			NetworkActive = true; fLobby = true;
			// lobby timeout specified? (e.g. /lobby:120)
			if (szParameter[6] == ':')
			{
				iLobbyTimeout = atoi(szParameter + 7);
				if (iLobbyTimeout < 0) iLobbyTimeout = 0;
			}
		}
		// Observe
		if (SEqualNoCase(szParameter, "/observe"))
		{
			NetworkActive = true; fObserve = true;
		}
		// Enable runtime join
		if (SEqualNoCase(szParameter, "/runtimejoin"))
			Config.Network.NoRuntimeJoin = false;
		// Disable runtime join
		if (SEqualNoCase(szParameter, "/noruntimejoin"))
			Config.Network.NoRuntimeJoin = true;
		// Check for update
		if (SEqualNoCase(szParameter, "/update"))
			Application.CheckForUpdates = true;
		// Direct join
		if (SEqual2NoCase(szParameter, "/join:"))
		{
			NetworkActive = true;
			SCopy(szParameter + 6, DirectJoinAddress, _MAX_PATH);
			continue;
		}
		// Direct join by URL
		if (SEqual2NoCase(szParameter, "clonk:"))
		{
			// Store address
			SCopy(szParameter + 6, DirectJoinAddress, _MAX_PATH);
			SClearFrontBack(DirectJoinAddress, '/');
			// Special case: if the target address is "update" then this is used for update initiation by url
			if (SEqualNoCase(DirectJoinAddress, "update"))
			{
				Application.CheckForUpdates = true;
				DirectJoinAddress[0] = 0;
				continue;
			}
			// Self-enable network
			NetworkActive = true;
			continue;
		}
		// port overrides
		if (SEqual2NoCase(szParameter, "/tcpport:"))
			Config.Network.PortTCP = atoi(szParameter + 9);
		if (SEqual2NoCase(szParameter, "/udpport:"))
			Config.Network.PortUDP = atoi(szParameter + 9);
		// network game password
		if (SEqual2NoCase(szParameter, "/pass:"))
			Network.SetPassword(szParameter + 6);
		// network game comment
		if (SEqual2NoCase(szParameter, "/comment:"))
			Config.Network.Comment.CopyValidated(szParameter + 9);
#ifndef NDEBUG
		// debug configs
		if (SEqualNoCase(szParameter, "/host"))
		{
			NetworkActive = true;
			fLobby = true;
			Config.Network.PortTCP = 11112;
			Config.Network.PortUDP = 11113;
			Config.Network.MasterServerSignUp = Config.Network.LeagueServerSignUp = false;
		}
		if (SEqual2NoCase(szParameter, "/client:"))
		{
			NetworkActive = true;
			SCopy("localhost", DirectJoinAddress, _MAX_PATH);
			fLobby = true;
			Config.Network.PortTCP = 11112 + 2 * (atoi(szParameter + 8) + 1);
			Config.Network.PortUDP = 11113 + 2 * (atoi(szParameter + 8) + 1);
		}
#endif
	}

	// Check for fullscreen switch in command line
	if (SSearchNoCase(szCmdLine, "/console"))
		Application.isFullScreen = false;

	// verbose
	if (SSearchNoCase(szCmdLine, "/verbose"))
		Verbose = true;

	// startup dialog required?
	Application.UseStartupDialog = Application.isFullScreen && !*DirectJoinAddress && !*ScenarioFilename && !RecordStream.getSize();
}

bool C4Game::LoadScenarioComponents()
{
	// Info
	Info.Load(LoadResStr("IDS_CNS_INFO"), ScenarioFile, C4CFN_Info);
	// Overload clonk names from scenario file
	if (ScenarioFile.EntryCount(C4CFN_Names))
		Names.Load(LoadResStr("IDS_CNS_NAMES"), ScenarioFile, C4CFN_Names);
	// scenario sections
	char fn[_MAX_FNAME + 1] = { 0 };
	ScenarioFile.ResetSearch(); *fn = 0;
	while (ScenarioFile.FindNextEntry(C4CFN_ScenarioSections, fn, nullptr, nullptr, !!*fn))
	{
		// get section name
		char SctName[_MAX_FNAME + 1];
		int32_t iWildcardPos = SCharPos('*', C4CFN_ScenarioSections);
		SCopy(fn + iWildcardPos, SctName, _MAX_FNAME);
		RemoveExtension(SctName);
		if (SLen(SctName) > C4MaxName || !*SctName)
		{
			DebugLog("invalid section name");
			LogFatal(FormatString(LoadResStr("IDS_ERR_SCENSECTION"), fn).getData()); return false;
		}
		// load this section into temp store
		C4ScenarioSection *pSection = new C4ScenarioSection(SctName);
		if (!pSection->ScenarioLoad(fn))
		{
			LogFatal(FormatString(LoadResStr("IDS_ERR_SCENSECTION"), fn).getData()); return false;
		}
	}

	// Success
	return true;
}

void C4Game::LoadScenarioScripts()
{
	// Script
	Script.Reg2List(&ScriptEngine, &ScriptEngine);
	Script.Load(LoadResStr("IDS_CNS_SCRIPT"), ScenarioFile, C4CFN_Script, Config.General.LanguageEx, nullptr, &ScenarioLangStringTable);
	// additional system scripts?
	C4Group SysGroup;
	char fn[_MAX_FNAME + 1] = { 0 };
	if (SysGroup.OpenAsChild(&ScenarioFile, C4CFN_System))
	{
		ScenarioSysLangStringTable.LoadEx("StringTbl", SysGroup, C4CFN_ScriptStringTbl, Config.General.LanguageEx);
		// load all scripts in there
		SysGroup.ResetSearch();
		while (SysGroup.FindNextEntry(C4CFN_ScriptFiles, fn, nullptr, nullptr, !!fn[0]))
		{
			// host will be destroyed by script engine, so drop the references
			C4ScriptHost *scr = new C4ScriptHost();
			scr->Reg2List(&ScriptEngine, &ScriptEngine);
			scr->Load(nullptr, SysGroup, fn, Config.General.LanguageEx, nullptr, &ScenarioSysLangStringTable);
		}
		// if it's a physical group: watch out for changes
		if (!SysGroup.IsPacked())
		{
			AddDirectoryForMonitoring(SysGroup.GetFullName().getData());
		}
		SysGroup.Close();
	}
}

bool C4Game::InitKeyboard()
{
	C4CustomKey::CodeList Keys;

	// clear previous
	KeyboardInput.Clear();

	// globals
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_F3),               "MusicToggle",  C4KeyScope(KEYSCOPE_Generic | KEYSCOPE_Gui),    new C4KeyCB  <C4Game>                (*this,                    &C4Game::ToggleMusic)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_F3, KEYS_Control), "SoundToggle",  C4KeyScope(KEYSCOPE_Generic | KEYSCOPE_Gui),    new C4KeyCB  <C4Game>                (*this,                    &C4Game::ToggleSound)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_F9),               "Screenshot",   C4KeyScope(KEYSCOPE_Fullscreen | KEYSCOPE_Gui), new C4KeyCBEx<C4GraphicsSystem, bool>(GraphicsSystem, false,    &C4GraphicsSystem::SaveScreenshot)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_F9, KEYS_Control), "ScreenshotEx",            KEYSCOPE_Fullscreen,                 new C4KeyCBEx<C4GraphicsSystem, bool>(GraphicsSystem, true,     &C4GraphicsSystem::SaveScreenshot)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(KEY_C, KEYS_Alt),    "ToggleChat",   C4KeyScope(KEYSCOPE_Generic | KEYSCOPE_Gui),    new C4KeyCB  <C4Game>                (*this,                    &C4Game::ToggleChat)));

	// main ingame
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_F1), "ToggleShowHelp",         KEYSCOPE_Generic, new C4KeyCB<C4GraphicsSystem>(GraphicsSystem, &C4GraphicsSystem::ToggleShowHelp)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_F4), "NetClientListDlgToggle", KEYSCOPE_Generic, new C4KeyCB<C4Network2>      (Network,        &C4Network2::ToggleClientListDlg)));

	// messageboard
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_UP, KEYS_Shift),   "MsgBoardScrollUp",   KEYSCOPE_Fullscreen, new C4KeyCB<C4MessageBoard>(GraphicsSystem.MessageBoard, &C4MessageBoard::ControlScrollUp)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_DOWN, KEYS_Shift), "MsgBoardScrollDown", KEYSCOPE_Fullscreen, new C4KeyCB<C4MessageBoard>(GraphicsSystem.MessageBoard, &C4MessageBoard::ControlScrollDown)));

	// debug mode & debug displays
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_F5, KEYS_Control), "DbgModeToggle",          KEYSCOPE_Generic, new C4KeyCB<C4Game>          (*this,          &C4Game::ToggleDebugMode)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_F6, KEYS_Control), "DbgShowVtxToggle",       KEYSCOPE_Generic, new C4KeyCB<C4GraphicsSystem>(GraphicsSystem, &C4GraphicsSystem::ToggleShowVertices)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_F7, KEYS_Control), "DbgShowActionToggle",    KEYSCOPE_Generic, new C4KeyCB<C4GraphicsSystem>(GraphicsSystem, &C4GraphicsSystem::ToggleShowAction)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_F8, KEYS_Control), "DbgShowSolidMaskToggle", KEYSCOPE_Generic, new C4KeyCB<C4GraphicsSystem>(GraphicsSystem, &C4GraphicsSystem::ToggleShowSolidMask)));

	// playback speed - improve...
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_ADD,      KEYS_Shift), "GameSpeedUp",  KEYSCOPE_Generic, new C4KeyCB<C4Game>(*this, &C4Game::SpeedUp)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_SUBTRACT, KEYS_Shift), "GameSlowDown", KEYSCOPE_Generic, new C4KeyCB<C4Game>(*this, &C4Game::SlowDown)));

	// fullscreen menu
	Keys.clear(); Keys.push_back(C4KeyCodeEx(K_LEFT));
	if (Config.Controls.GamepadGuiControl) Keys.push_back(C4KeyCodeEx(KEY_Gamepad(0, KEY_JOY_Left)));
	KeyboardInput.RegisterKey(new C4CustomKey(Keys,                  "FullscreenMenuLeft",   KEYSCOPE_FullSMenu, new C4KeyCBEx<C4FullScreen, uint8_t>(FullScreen, COM_MenuLeft,  &C4FullScreen::MenuKeyControl)));
	Keys.clear(); Keys.push_back(C4KeyCodeEx(K_RIGHT));
	if (Config.Controls.GamepadGuiControl) Keys.push_back(C4KeyCodeEx(KEY_Gamepad(0, KEY_JOY_Right)));
	KeyboardInput.RegisterKey(new C4CustomKey(Keys,                  "FullscreenMenuRight",  KEYSCOPE_FullSMenu, new C4KeyCBEx<C4FullScreen, uint8_t>(FullScreen, COM_MenuRight, &C4FullScreen::MenuKeyControl)));
	Keys.clear(); Keys.push_back(C4KeyCodeEx(K_UP));
	if (Config.Controls.GamepadGuiControl) Keys.push_back(C4KeyCodeEx(KEY_Gamepad(0, KEY_JOY_Up)));
	KeyboardInput.RegisterKey(new C4CustomKey(Keys,                  "FullscreenMenuUp",     KEYSCOPE_FullSMenu, new C4KeyCBEx<C4FullScreen, uint8_t>(FullScreen, COM_MenuUp,    &C4FullScreen::MenuKeyControl)));
	Keys.clear(); Keys.push_back(C4KeyCodeEx(K_DOWN));
	if (Config.Controls.GamepadGuiControl) Keys.push_back(C4KeyCodeEx(KEY_Gamepad(0, KEY_JOY_Down)));
	KeyboardInput.RegisterKey(new C4CustomKey(Keys,                  "FullscreenMenuDown",   KEYSCOPE_FullSMenu, new C4KeyCBEx<C4FullScreen, uint8_t>(FullScreen, COM_MenuDown,  &C4FullScreen::MenuKeyControl)));
	Keys.clear(); Keys.push_back(C4KeyCodeEx(K_SPACE)); Keys.push_back(C4KeyCodeEx(K_RETURN));
	if (Config.Controls.GamepadGuiControl) Keys.push_back(C4KeyCodeEx(KEY_Gamepad(0, KEY_JOY_AnyLowButton)));
	KeyboardInput.RegisterKey(new C4CustomKey(Keys,                  "FullscreenMenuOK",     KEYSCOPE_FullSMenu, new C4KeyCBEx<C4FullScreen, uint8_t>(FullScreen, COM_MenuEnter, &C4FullScreen::MenuKeyControl))); // name used by PlrControlKeyName
	Keys.clear(); Keys.push_back(C4KeyCodeEx(K_ESCAPE));
	if (Config.Controls.GamepadGuiControl) Keys.push_back(C4KeyCodeEx(KEY_Gamepad(0, KEY_JOY_AnyHighButton)));
	KeyboardInput.RegisterKey(new C4CustomKey(Keys,                  "FullscreenMenuCancel", KEYSCOPE_FullSMenu, new C4KeyCBEx<C4FullScreen, uint8_t>(FullScreen, COM_MenuClose, &C4FullScreen::MenuKeyControl))); // name used by PlrControlKeyName
	Keys.clear(); Keys.push_back(C4KeyCodeEx(K_SPACE));
	if (Config.Controls.GamepadGuiControl) Keys.push_back(C4KeyCodeEx(KEY_Gamepad(0, KEY_JOY_AnyButton)));
	KeyboardInput.RegisterKey(new C4CustomKey(Keys,                  "FullscreenMenuOpen",   KEYSCOPE_FreeView,  new C4KeyCB  <C4FullScreen>      (FullScreen,                &C4FullScreen::ActivateMenuMain))); // name used by C4MainMenu!
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_RIGHT),  "FilmNextPlayer",       KEYSCOPE_FilmView,  new C4KeyCB  <C4GraphicsSystem>  (GraphicsSystem,            &C4GraphicsSystem::ViewportNextPlayer)));

	// chat
	Keys.clear();
	Keys.push_back(C4KeyCodeEx(K_RETURN));
	Keys.push_back(C4KeyCodeEx(K_F2)); // alternate chat key, if RETURN is blocked by player control
	KeyboardInput.RegisterKey(new C4CustomKey(Keys,                                "ChatOpen",        KEYSCOPE_Generic, new C4KeyCBEx<C4MessageInput, C4ChatInputDialog::Mode>(MessageInput, C4ChatInputDialog::All,    &C4MessageInput::KeyStartTypeIn)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_RETURN, KEYS_Shift),   "ChatOpen2Allies", KEYSCOPE_Generic, new C4KeyCBEx<C4MessageInput, C4ChatInputDialog::Mode>(MessageInput, C4ChatInputDialog::Allies, &C4MessageInput::KeyStartTypeIn)));
	KeyboardInput.RegisterKey(new C4CustomKey{C4KeyCodeEx{K_RETURN, KEYS_Alt},     "ChatOpen2Say",    KEYSCOPE_Generic, new C4KeyCBEx<C4MessageInput, C4ChatInputDialog::Mode>{MessageInput, C4ChatInputDialog::Say,    &C4MessageInput::KeyStartTypeIn}});

	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_LEFT),   "FreeViewScrollLeft",    KEYSCOPE_FreeView,   new C4KeyCBEx<C4GraphicsSystem, C4Vec2D>(GraphicsSystem, C4Vec2D(-5, 0), &C4GraphicsSystem::FreeScroll)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_RIGHT),  "FreeViewScrollRight",   KEYSCOPE_FreeView,   new C4KeyCBEx<C4GraphicsSystem, C4Vec2D>(GraphicsSystem, C4Vec2D(+5, 0), &C4GraphicsSystem::FreeScroll)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_UP),     "FreeViewScrollUp",      KEYSCOPE_FreeView,   new C4KeyCBEx<C4GraphicsSystem, C4Vec2D>(GraphicsSystem, C4Vec2D(0, -5), &C4GraphicsSystem::FreeScroll)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_DOWN),   "FreeViewScrollDown",    KEYSCOPE_FreeView,   new C4KeyCBEx<C4GraphicsSystem, C4Vec2D>(GraphicsSystem, C4Vec2D(0, +5), &C4GraphicsSystem::FreeScroll)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_TAB),    "ScoreboardToggle",      KEYSCOPE_Generic,    new C4KeyCB  <C4Scoreboard>             (Scoreboard,                     &C4Scoreboard::KeyUserShow)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_ESCAPE), "GameAbort",             KEYSCOPE_Fullscreen, new C4KeyCB  <C4FullScreen>             (FullScreen,                     &C4FullScreen::ShowAbortDlg)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_PAUSE),  "FullscreenPauseToggle", KEYSCOPE_Fullscreen, new C4KeyCB  <C4Game>                   (Game,                           &C4Game::TogglePause)));

	// console keys
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_PAUSE),             "ConsolePauseToggle",   KEYSCOPE_Console, new C4KeyCB  <C4Console>          (Console,              &C4Console::TogglePause)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_SPACE),             "EditCursorModeToggle", KEYSCOPE_Console, new C4KeyCB  <C4EditCursor>       (Console.EditCursor,   &C4EditCursor::ToggleMode)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_ADD),               "ToolsDlgGradeUp",      KEYSCOPE_Console, new C4KeyCBEx<C4ToolsDlg, int32_t>(Console.ToolsDlg, +5, &C4ToolsDlg::ChangeGrade)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_SUBTRACT),          "ToolsDlgGradeDown",    KEYSCOPE_Console, new C4KeyCBEx<C4ToolsDlg, int32_t>(Console.ToolsDlg, -5, &C4ToolsDlg::ChangeGrade)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(KEY_M, KEYS_Control), "ToolsDlgPopMaterial",  KEYSCOPE_Console, new C4KeyCB  <C4ToolsDlg>         (Console.ToolsDlg,     &C4ToolsDlg::PopMaterial)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(KEY_T, KEYS_Control), "ToolsDlgPopTextures",  KEYSCOPE_Console, new C4KeyCB  <C4ToolsDlg>         (Console.ToolsDlg,     &C4ToolsDlg::PopTextures)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(KEY_I, KEYS_Control), "ToolsDlgIFTToggle",    KEYSCOPE_Console, new C4KeyCB  <C4ToolsDlg>         (Console.ToolsDlg,     &C4ToolsDlg::ToggleIFT)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(KEY_W, KEYS_Control), "ToolsDlgToolToggle",   KEYSCOPE_Console, new C4KeyCB  <C4ToolsDlg>         (Console.ToolsDlg,     &C4ToolsDlg::ToggleTool)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_DELETE),            "EditCursorDelete",     KEYSCOPE_Console, new C4KeyCB  <C4EditCursor>       (Console.EditCursor,   &C4EditCursor::Delete)));

	// no default keys assigned
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(KEY_Default), "ChartToggle", C4KeyScope(KEYSCOPE_Generic | KEYSCOPE_Gui), new C4KeyCB  <C4Game>                (*this,          &C4Game::ToggleChart)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(KEY_Default), "NetObsNextPlayer",       KEYSCOPE_FreeView,                new C4KeyCB  <C4GraphicsSystem>      (GraphicsSystem, &C4GraphicsSystem::ViewportNextPlayer)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(KEY_Default), "CtrlRateDown",           KEYSCOPE_Generic,                 new C4KeyCBEx<C4GameControl, int32_t>(Control, -1,    &C4GameControl::KeyAdjustControlRate)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(KEY_Default), "CtrlRateUp",             KEYSCOPE_Generic,                 new C4KeyCBEx<C4GameControl, int32_t>(Control, +1,    &C4GameControl::KeyAdjustControlRate)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(KEY_Default), "NetAllowJoinToggle",     KEYSCOPE_Generic,                 new C4KeyCB  <C4Network2>            (Network,        &C4Network2::ToggleAllowJoin)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(KEY_Default), "NetStatsToggle",         KEYSCOPE_Generic,                 new C4KeyCB  <C4GraphicsSystem>      (GraphicsSystem, &C4GraphicsSystem::ToggleShowNetStatus)));

	// Map player keyboard controls
	int32_t iKdbSet, iCtrl;
	StdStrBuf sPlrCtrlName;
	for (iKdbSet = C4P_Control_Keyboard1; iKdbSet <= C4P_Control_Keyboard4; iKdbSet++)
		for (iCtrl = 0; iCtrl < C4MaxKey; iCtrl++)
		{
			sPlrCtrlName.Format("Kbd%dKey%d", iKdbSet - C4P_Control_Keyboard1 + 1, iCtrl + 1);
			KeyboardInput.RegisterKey(new C4CustomKey(
				C4KeyCodeEx(Config.Controls.Keyboard[iKdbSet][iCtrl]),
				sPlrCtrlName.getData(), KEYSCOPE_Control,
				new C4KeyCBExPassKey<C4Game, C4KeySetCtrl>(*this, C4KeySetCtrl(iKdbSet, iCtrl), &C4Game::LocalControlKey, &C4Game::LocalControlKeyUp),
				C4CustomKey::PRIO_PlrControl));
		}

	// Map player gamepad controls
	for (int32_t iGamepad = 0; iGamepad < C4ConfigMaxGamepads; iGamepad++)
	{
		int32_t iControlSet = iGamepad + C4P_Control_GamePad1;
		C4ConfigGamepad &cfg = Config.Gamepads[iGamepad];

		std::map<C4KeyCodeEx, uint32_t> keyToCommand;
		for (int iButton = 0; iButton < C4MaxGamePadButtons; iButton++) {
			C4KeyCodeEx keyCode = C4KeyCodeEx(KEY_Gamepad(iGamepad, KEY_JOY_Button(iButton)));
			keyToCommand.insert_or_assign(keyCode, cfg.ButtonMenuCommand[iButton]); // Ensure that buttons with no normal command and only a menu-command are also registered
			keyToCommand.insert_or_assign(keyCode, cfg.ButtonCommand[iButton]); // Normal command overrides menu command, and is converted back to menu command by C4Menu::ConvertCom
		}
		for (int iAxis = 0; iAxis < C4MaxGamePadAxis; iAxis++) {
			keyToCommand.insert_or_assign(C4KeyCodeEx(KEY_Gamepad(iGamepad, KEY_JOY_Axis(iAxis, true))), cfg.AxisMaxCommand[iAxis]);
			keyToCommand.insert_or_assign(C4KeyCodeEx(KEY_Gamepad(iGamepad, KEY_JOY_Axis(iAxis, false))), cfg.AxisMinCommand[iAxis]);
		}

		for (auto& [keyCodeEx, iCommand] : keyToCommand)
		{
			if (iCommand == COM_None) continue;
			sPlrCtrlName = C4KeyCodeEx::KeyCode2String(keyCodeEx.Key, false, false);
			KeyboardInput.RegisterKey(new C4CustomKey(
				keyCodeEx,
				sPlrCtrlName.getData(), KEYSCOPE_Control,
				new C4KeyCBExPassKey(*this, C4KeySetCtrl(iControlSet, iCommand), &C4Game::LocalControlGamepad, &C4Game::LocalControlGamepadUp),
				C4CustomKey::PRIO_PlrControl));
		}
	}

	// load any custom keysboard overloads
	KeyboardInput.LoadCustomConfig();

	// done, success
	return true;
}

bool C4Game::InitSystem()
{
	// Timer flags
	GameGo = false;
	// set gamma
	GraphicsSystem.SetGamma(Config.Graphics.Gamma1, Config.Graphics.Gamma2, Config.Graphics.Gamma3, C4GRI_USER);
	// open graphics group now for font-init
	if (!GraphicsResource.RegisterGlobalGraphics()) return false;
	// load font list
#ifndef USE_CONSOLE
	if (!FontLoader.LoadDefs(Application.SystemGroup, Config))
	{
		LogFatal(LoadResStr("IDS_ERR_FONTDEFS")); return false;
	}
#endif
	// init extra root group
	// this loads font definitions in this group as well
	// the function may return false, if no extra group is present - that is OK
	if (Extra.InitGroup())
		// add any Graphics.c4g-files in Extra.c4g-root
		GraphicsResource.RegisterMainGroups();
	// init main system font
	// This is preliminary, because it's not unlikely to be overloaded after scenario opening and Extra.c4g-initialization.
	// But postponing initialization until then would mean a black screen for quite some time of the initialization progress.
	// Peter wouldn't like this...
#ifndef USE_CONSOLE
	if (!FontLoader.InitFont(GraphicsResource.FontRegular, Config.General.RXFontName, C4FontLoader::C4FT_Main, Config.General.RXFontSize, &GraphicsResource.Files))
		return false;
#endif
	// init message input (default commands)
	MessageInput.Init();
	// init keyboard input (default keys, plus overloads)
	if (!InitKeyboard())
	{
		LogFatal(LoadResStr("IDS_ERR_NOKEYBOARD")); return false;
	}
	// Rank system
	Rank.Init(Config.GetSubkeyPath("ClonkRanks"), LoadResStr("IDS_GAME_DEFRANKS"), 1000);
	// done, success
	return true;
}

C4Player *C4Game::JoinPlayer(const char *szFilename, int32_t iAtClient, const char *szAtClientName, C4PlayerInfo *pInfo)
{
	assert(pInfo);
	C4Player *pPlr;
	// Join
	if (!(pPlr = Players.Join(szFilename, true, iAtClient, szAtClientName, pInfo))) return nullptr;
	// Player final init
	pPlr->FinalInit(true);
	// Create player viewport
	if (pPlr->LocalControl) CreateViewport(pPlr->Number);
	// Check fullscreen viewports
	FullScreen.ViewportCheck();
	// Update menus
	Console.UpdateMenus();
	// Append player name to list of session player names (no duplicates)
	if (!SIsModule(PlayerNames.getData(), pPlr->GetName()))
	{
		if (PlayerNames) PlayerNames += ";";
		PlayerNames += pPlr->GetName();
	}
	// Success
	return pPlr;
}

void C4Game::FixRandom(int32_t iSeed)
{
	FixedRandom(iSeed);
	Randomize3();
}

bool C4Game::LocalControlKey(C4KeyCodeEx key, C4KeySetCtrl Ctrl)
{
	// keyboard callback: Perform local player control
	C4Player *pPlr;
	if (pPlr = Players.GetLocalByKbdSet(Ctrl.iKeySet))
	{
		// Swallow a event generated from Keyrepeat for AutoStopControl
		if (pPlr->ControlStyle)
		{
			if (key.IsRepeated())
				return true;
		}
		LocalPlayerControl(pPlr->Number, Control2Com(Ctrl.iCtrl, false));
		return true;
	}
	// not processed - must return false here, so unused keyboard control sets do not block used ones
	return false;
}

bool C4Game::LocalControlKeyUp(C4KeyCodeEx key, C4KeySetCtrl Ctrl)
{
	// Direct callback for released key in AutoStopControl-mode (ignore repeated)
	if (key.IsRepeated())
		return true;
	C4Player *pPlr;
	if ((pPlr = Players.GetLocalByKbdSet(Ctrl.iKeySet)) && pPlr->ControlStyle)
	{
		int iCom = Control2Com(Ctrl.iCtrl, true);
		if (iCom != COM_None) LocalPlayerControl(pPlr->Number, iCom);
		return true;
	}
	// not processed - must return false here, so unused keyboard control sets do not block used ones
	return false;
}

bool C4Game::LocalControlGamepad(C4KeyCodeEx key, C4KeySetCtrl Ctrl)
{
	// keyboard callback: Perform local player control
	C4Player* pPlr;
	if (pPlr = Players.GetLocalByKbdSet(Ctrl.iKeySet))
	{
		int32_t iCommand = Ctrl.iCtrl;
		bool cursorMenuActive = pPlr->Cursor && pPlr->Cursor->Menu && pPlr->Cursor->Menu->IsActive();
		if (Key_IsGamepadButton(key.Key) && (pPlr->Menu.IsActive() || cursorMenuActive)) {
			int32_t iGamepad = Ctrl.iKeySet - C4P_Control_GamePad1;
			int32_t iButton = Key_GetGamepadButtonIndex(key.Key);
			iCommand = Config.Gamepads[iGamepad].ButtonMenuCommand[iButton];
		}
		else { // No menu active, send normal command
			// Swallow a event generated from Keyrepeat for AutoStopControl
			if (pPlr->ControlStyle)
			{
				if (key.IsRepeated())
					return true;
			}
		}
		LocalPlayerControl(pPlr->Number, iCommand);
		return true;
	}
	// not processed - must return false here, so unused keyboard control sets do not block used ones
	return false;
}

bool C4Game::LocalControlGamepadUp(C4KeyCodeEx key, C4KeySetCtrl Ctrl)
{
	// Direct callback for released key in AutoStopControl-mode (ignore repeated)
	if (key.IsRepeated())
		return true;
	C4Player* pPlr;
	if ((pPlr = Players.GetLocalByKbdSet(Ctrl.iKeySet)) && pPlr->ControlStyle)
	{
		// Normalize COM, so that Release events are only generate for "standard" buttons, not e.g. COM_MenuEnter
		int32_t iCom = Control2Com(Com2Control(Ctrl.iCtrl), true);
		if (iCom != COM_None) {
			LocalPlayerControl(pPlr->Number, iCom);
		}
		return true;
	}
	// not processed - must return false here, so unused keyboard control sets do not block used ones
	return false;
}

void C4Game::LocalPlayerControl(int32_t iPlayer, int32_t iCom)
{
	C4Player *pPlr = Players.Get(iPlayer); if (!pPlr) return;
	int32_t iData = 0;
	// Menu button com
	if (iCom == COM_PlayerMenu)
	{
		// Player menu open: close
		if (pPlr->Menu.IsActive())
			pPlr->Menu.Close(false);
		// Menu closed: open main menu
		else
			pPlr->ActivateMenuMain();
		return;
	}
	// Local player menu active: convert menu com and control local
	if (pPlr->Menu.ConvertCom(iCom, iData, true))
	{
		pPlr->Menu.Control(iCom, iData);
		return;
	}
	// Pre-queue asynchronous menu conversions
	if (pPlr->Cursor && pPlr->Cursor->Menu)
		pPlr->Cursor->Menu->ConvertCom(iCom, iData, true);
	// Not for eliminated (checked again in DirectCom, but make sure no control is generated for eliminated players!)
	if (pPlr->Eliminated) return;
	// Player control: add to control queue
	Input.Add(CID_PlrControl, new C4ControlPlayerControl(iPlayer, iCom, iData));
}

bool C4Game::DefinitionFilenamesFromSaveGame()
{
	std::string source(GameText.GetData());

	// Use loaded game text component
	if (source.size())
	{
		// Search def file name section
		size_t pos = source.find("[DefinitionFiles]");
		if (pos != std::string::npos)
		{
			DefinitionFilenames.clear();
			std::istringstream stream(source.substr(pos));
			std::string line;
			bool found = false;
			while (std::getline(stream, line))
			{
				size_t p = line.find("Definition");
				if (p == 0 && (p = line.find('=', p) != std::string::npos))
				{
					found = true;
					DefinitionFilenames.push_back(line.substr(p));
				}
				else if (found)
				{
					break;
				}
			}
			return found;
		}
	}
	return false;
}

bool C4Game::DoGameOver()
{
	// Duplication safety
	if (GameOver) return false;
	// Flag, log, call
	GameOver = true;
	Log(LoadResStr("IDS_PRC_GAMEOVER"));
	Script.GRBroadcast(PSF_OnGameOver);
	// Flag all surviving players as winners
	for (C4Player *pPlayer = Players.First; pPlayer; pPlayer = pPlayer->Next)
		if (!pPlayer->Eliminated)
			pPlayer->EvaluateLeague(false, true);
	return true;
}

void C4Game::ShowGameOverDlg()
{
	// safety
	if (GameOverDlgShown) return;
	// flag, show
	GameOverDlgShown = true;
#ifdef USE_CONSOLE
	// wait for streaming to finish
	if (Network.isEnabled() && Network.isStreaming())
	{
		LogF("Network: Sending %d bytes pending for stream...", Network.getPendingStreamData());
		while (Network.isStreaming())
			if (!Application.HandleMessage(100, false))
				break;
	}
	// console engine quits here directly
	Application.QuitGame();
#else
	if (pGUI && Application.isFullScreen)
	{
		C4GameOverDlg *pDlg = new C4GameOverDlg();
		pDlg->SetDelOnClose();
		if (!pDlg->Show(pGUI, true)) { delete pDlg; Application.QuitGame(); }
	}
#endif
}

void C4Game::SyncClearance()
{
	PXS.SyncClearance();
	Objects.SyncClearance();
}

void C4Game::Synchronize(bool fSavePlayerFiles)
{
	// Log
	LogSilentF("Network: Synchronization (Frame %i) [PlrSave: %d]", FrameCounter, fSavePlayerFiles);
	// callback to control (to start record)
	Control.OnGameSynchronizing();
	// Fix random
	FixRandom(Parameters.RandomSeed);
	// Synchronize members
	Defs.Synchronize();
	Landscape.Synchronize();
	MassMover.Synchronize();
	PXS.Synchronize();
	Objects.Synchronize();
	// synchronize local player files if desired
	// this will reset any InActionTimes!
	// (not in replay mode)
	if (fSavePlayerFiles && !Control.isReplay()) Players.SynchronizeLocalFiles();
	// callback to network
	if (Network.isEnabled()) Network.OnGameSynchronized();
	// TransferZone synchronization: Must do this after dynamic creation to avoid synchronization loss
	// if UpdateTransferZone-callbacks do sync-relevant changes
	TransferZones.Synchronize();
}

C4Object *C4Game::FindBase(int32_t iPlayer, int32_t iIndex)
{
	C4Object *cObj; C4ObjectLink *clnk;
	for (clnk = Objects.First; clnk && (cObj = clnk->Obj); clnk = clnk->Next)
		// Status
		if (cObj->Status)
			// Base
			if (cObj->Base == iPlayer)
				// Index
				if (iIndex == 0) return cObj;
				else iIndex--;
	// Not found
	return nullptr;
}

C4Object *C4Game::FindFriendlyBase(int32_t iPlayer, int32_t iIndex)
{
	C4Object *cObj; C4ObjectLink *clnk;
	for (clnk = Objects.First; clnk && (cObj = clnk->Obj); clnk = clnk->Next)
		// Status
		if (cObj->Status)
			// Base
			if (ValidPlr(cObj->Base))
				// friendly Base
				if (!Hostile(cObj->Base, iPlayer))
					// Index
					if (iIndex == 0) return cObj;
					else iIndex--;
	// Not found
	return nullptr;
}

C4Object *C4Game::FindObjectByCommand(int32_t iCommand, C4Object *pTarget, C4Value iTx, int32_t iTy, C4Object *pTarget2, C4Object *pFindNext)
{
	C4Object *cObj; C4ObjectLink *clnk;
	for (clnk = Objects.First; clnk && (cObj = clnk->Obj); clnk = clnk->Next)
	{
		// find next
		if (pFindNext) { if (cObj == pFindNext) pFindNext = nullptr; continue; }
		// Status
		if (cObj->Status)
			// Check commands
			for (C4Command *pCommand = cObj->Command; pCommand; pCommand = pCommand->Next)
				// Command
				if (pCommand->Command == iCommand)
					// Target
					if (!pTarget || (pCommand->Target == pTarget))
						// Position
						if ((!iTx && !iTy) || ((pCommand->Tx == iTx) && (pCommand->Ty == iTy)))
							// Target2
							if (!pTarget2 || (pCommand->Target2 == pTarget2))
								// Found
								return cObj;
	}
	// Not found
	return nullptr;
}

bool C4Game::InitNetworkFromAddress(const char *szAddress)
{
	StdStrBuf strRefQueryFailed(LoadResStr("IDS_NET_REFQUERY_FAILED"));
	// Query reference
	C4Network2RefClient RefClient;
	if (!RefClient.Init() ||
		!RefClient.SetServer(szAddress) ||
		!RefClient.QueryReferences())
	{
		LogFatal(FormatString(strRefQueryFailed.getData(), RefClient.GetError()).getData()); return false;
	}
	// We have to wait for the answer
	StdStrBuf Message = FormatString(LoadResStr("IDS_NET_REFQUERY_QUERYMSG"), szAddress);
	Log(Message.getData());
	// Set up wait dialog
	C4GUI::MessageDialog *pDlg = nullptr;
	if (pGUI && !Console.Active)
	{
		// create & show
		pDlg = new C4GUI::MessageDialog(Message.getData(), LoadResStr("IDS_NET_REFQUERY_QUERYTITLE"),
			C4GUI::MessageDialog::btnAbort, C4GUI::Ico_NetWait, C4GUI::MessageDialog::dsMedium);
		if (!pDlg || !pDlg->Show(pGUI, true)) return false;
	}
	// Wait for response
	while (RefClient.isBusy())
	{
		// Execute GUI
		if (Application.HandleMessage(100) == HR_Failure ||
			(pDlg && pDlg->IsAborted()))
		{
			if (pGUI) delete pDlg;
			return false;
		}
		// Check if reference is received
		if (!RefClient.Execute(0))
			break;
	}
	// Close dialog
	if (pGUI) delete pDlg;
	// Error?
	if (!RefClient.isSuccess())
	{
		LogFatal(FormatString(strRefQueryFailed.getData(), RefClient.GetError()).getData()); return false;
	}
	// Get references
	C4Network2Reference **ppRefs = nullptr; int32_t iRefCount;
	if (!RefClient.GetReferences(ppRefs, iRefCount) || iRefCount <= 0)
	{
		LogFatal(FormatString(strRefQueryFailed.getData(), LoadResStr("IDS_NET_REFQUERY_NOREF")).getData()); return false;
	}
	// Connect to first reference
	bool fSuccess = InitNetworkFromReference(*ppRefs[0]);
	// Remove references
	for (int i = 0; i < iRefCount; i++)
		delete ppRefs[i];
	delete[] ppRefs;
	return fSuccess;
}

bool C4Game::InitNetworkFromReference(const C4Network2Reference &Reference)
{
	// Find host data
	C4Client *pHostData = Reference.Parameters.Clients.getClientByID(C4ClientIDHost);
	if (!pHostData) { LogFatal(LoadResStr("IDS_NET_INVALIDREF")); return false; }
	// Save scenario title
	Parameters.ScenarioTitle.CopyValidated(Reference.getTitle());
	// Log
	LogF(LoadResStr("IDS_NET_JOINGAMEBY"), pHostData->getName());
	// Init clients
	Clients.Init();
	// Connect
	if (Network.InitClient(Reference, false) != C4Network2::IR_Success)
	{
		LogFatal(FormatString(LoadResStr("IDS_NET_NOHOSTCON"), pHostData->getName()).getData());
		return false;
	}
	// init control
	if (!Control.InitNetwork(Clients.getLocal())) return false;
	// init local player info list
	Network.Players.Init();
	return true;
}

bool C4Game::InitNetworkHost()
{
	// Network not active?
	if (!NetworkActive)
	{
		// Clear client list
		if (!C4S.Head.Replay)
			Clients.Init();
		return true;
	}
	// network not active?
	if (C4S.Head.NetworkGame)
	{
		LogFatal(LoadResStr("IDS_NET_NODIRECTSTART")); Clients.Init();
	}
	// replay?
	if (C4S.Head.Replay)
	{
		LogFatal(LoadResStr("IDS_PRC_NONETREPLAY")); return true;
	}
	// clear client list
	Clients.Init();
	// init network as host
	if (!Network.InitHost(fLobby)) return false;
	// init control
	if (!Control.InitNetwork(Clients.getLocal())) return false;
	// init local player info list
	Network.Players.Init();
	// allow connect
	Network.AllowJoin(true);
	// do lobby (if desired)
	if (fLobby)
	{
		if (!Network.DoLobby()) return false;
	}
	else
	{
		// otherwise: start manually
		if (!Network.Start()) return false;
	}
	// ok
	return true;
}

bool C4Game::CheckObjectEnumeration()
{
	// Check valid & maximum number & duplicate numbers
	int32_t iMax = 0;
	C4Object *cObj; C4ObjectLink *clnk;
	C4Object *cObj2; C4ObjectLink *clnk2;
	clnk = Objects.First; if (!clnk) clnk = Objects.InactiveObjects.First;
	while (clnk)
	{
		// Invalid number
		cObj = clnk->Obj;
		if (cObj->Number < 1)
		{
			LogF("Invalid object enumeration number (%d) of object %s (x=%d)", cObj->Number, C4IdText(cObj->id), cObj->x);
			return false;
		}
		// Max
		if (cObj->Number > iMax) iMax = cObj->Number;
		// Duplicate
		for (clnk2 = Objects.First; clnk2 && (cObj2 = clnk2->Obj); clnk2 = clnk2->Next)
			if (cObj2 != cObj)
				if (cObj->Number == cObj2->Number)
				{
					LogF("Duplicate object enumeration number %d (%s and %s)", cObj2->Number, cObj->GetName(), cObj2->GetName());
					return false;
				}
		for (clnk2 = Objects.InactiveObjects.First; clnk2 && (cObj2 = clnk2->Obj); clnk2 = clnk2->Next)
			if (cObj2 != cObj)
				if (cObj->Number == cObj2->Number)
				{
					LogF("Duplicate object enumeration number %d (%s and %s(i))", cObj2->Number, cObj->GetName(), cObj2->GetName());
					return false;
				}
		// next
		if (!clnk->Next)
			if (clnk == Objects.Last) clnk = Objects.InactiveObjects.First; else clnk = nullptr;
		else
			clnk = clnk->Next;
	}
	// Adjust enumeration index
	if (iMax > ObjectEnumerationIndex) ObjectEnumerationIndex = iMax;
	// Done
	return true;
}

std::vector<std::string> C4Game::FoldersWithLocalsDefs(std::string path)
{
	std::vector<std::string> defs;

	// Get relative path
	path = Config.AtExeRelativePath(path.c_str());

	// Scan path for folder names
	std::string folderName;
	C4Group group;
	for (size_t pos = path.find(DirectorySeparator); pos != std::string::npos; pos = path.find(DirectorySeparator, pos + 1))
	{
		// Get folder name
		folderName = path.substr(0, pos);
		// Open folder
		if (SEqualNoCase(GetExtension(folderName.c_str()), "c4f"))
		{
			if (group.Open(folderName.c_str()))
			{
				// Check for contained defs
				// do not, however, add them to the group set:
				// parent folders are added by OpenScenario already!
				int32_t contents;
				if ((contents = GroupSet.CheckGroupContents(group, C4GSCnt_Definitions)))
				{
					defs.push_back(folderName);
				}
				// Close folder
				group.Close();
			}
		}
	}

	return defs;
}

void C4Game::InitValueOverloads()
{
	C4ID idOvrl; C4Def *pDef;
	// set new values
	for (int32_t cnt = 0; idOvrl = C4S.Game.Realism.ValueOverloads.GetID(cnt); cnt++)
		if (pDef = Defs.ID2Def(idOvrl))
			pDef->Value = C4S.Game.Realism.ValueOverloads.GetIDCount(idOvrl);
}

void C4Game::InitEnvironment()
{
	// Place environment objects
	int32_t cnt, cnt2;
	C4ID idType; int32_t iCount;
	for (cnt = 0; (idType = C4S.Environment.Objects.GetID(cnt, &iCount)); cnt++)
		for (cnt2 = 0; cnt2 < iCount; cnt2++)
			CreateObject(idType, nullptr);
}

void C4Game::InitRules()
{
	// Place rule objects
	int32_t cnt, cnt2;
	C4ID idType; int32_t iCount;
	for (cnt = 0; (idType = Parameters.Rules.GetID(cnt, &iCount)); cnt++)
		for (cnt2 = 0; cnt2 < std::max<int32_t>(iCount, 1); cnt2++)
			CreateObject(idType, nullptr);
	// Update rule flags
	UpdateRules();
}

void C4Game::InitGoals()
{
	// Place goal objects
	int32_t cnt, cnt2;
	C4ID idType; int32_t iCount;
	for (cnt = 0; (idType = Parameters.Goals.GetID(cnt, &iCount)); cnt++)
		for (cnt2 = 0; cnt2 < iCount; cnt2++)
			CreateObject(idType, nullptr);
}

void C4Game::UpdateRules()
{
	if (Tick255 && FrameCounter > 1) return;
	Rules = 0;
	if (ObjectCount(C4ID_Energy))       Rules |= C4RULE_StructuresNeedEnergy;
	if (ObjectCount(C4ID_CnMaterial))   Rules |= C4RULE_ConstructionNeedsMaterial;
	if (ObjectCount(C4ID_FlagRemvbl))   Rules |= C4RULE_FlagRemoveable;
	if (ObjectCount(C4Id("STSN")))      Rules |= C4RULE_StructuresSnowIn;
	if (ObjectCount(C4ID_TeamHomebase)) Rules |= C4RULE_TeamHombase;
}

void C4Game::SetInitProgress(float fToProgress)
{
	// set new progress
	InitProgress = int32_t(fToProgress);
	// if progress is more than one percent, display it
	if (InitProgress > LastInitProgress)
	{
		LastInitProgress = InitProgress;
		if (Application.IsMainThread())
		{
			GraphicsSystem.MessageBoard.LogNotify();
		}

		if (Application.pWindow)
		{
			Application.pWindow->SetProgress(InitProgress);
		}
	}
}

void C4Game::OnResolutionChanged()
{
	// update anything that's dependent on screen resolution
	if (pGUI)
		pGUI->SetBounds(C4Rect(0, 0, Config.Graphics.ResX, Config.Graphics.ResY));
	if (FullScreen.Active)
		InitFullscreenComponents(!!IsRunning);
	// note that this may fail if the gfx groups are closed already (runtime resolution change)
	// doesn't matter; old gfx are kept in this case
	if (GraphicsResource.IsInitialized())
	{
		GraphicsResource.ReloadResolutionDependentFiles();
	}
}

bool C4Game::LoadScenarioSection(const char *szSection, uint32_t dwFlags)
{
	// note on scenario section saving:
	// if a scenario section overwrites a value that had used the default values in the main scenario section,
	// returning to the main section with an unsaved landscape (and thus an unsaved scenario core),
	// would leave those values in the altered state of the previous section
	// scenario designers should regard this and always define any values, that are defined in subsections as well
	C4Group hGroup, *pGrp;

	// if current section was the loaded section (maybe main, but need not for resumed savegames)
	if (!pCurrentScenarioSection)
	{
		pCurrentScenarioSection = new C4ScenarioSection(CurrentScenarioSection);
		if (!*CurrentScenarioSection) SCopy(C4ScenSect_Main, CurrentScenarioSection, C4MaxName);
	}

	// find section to load
	C4ScenarioSection *pLoadSect = pScenarioSections;
	while (pLoadSect) if (SEqualNoCase(pLoadSect->GetName(), szSection)) break; else pLoadSect = pLoadSect->pNext;
	if (!pLoadSect)
	{
		DebugLogF("LoadScenarioSection: scenario section %s not found!", szSection);
		return false;
	}

	// save current section state
	if (pLoadSect != pCurrentScenarioSection && dwFlags & (C4S_SAVE_LANDSCAPE | C4S_SAVE_OBJECTS))
	{
		// ensure that the section file does point to temp store
		if (!pCurrentScenarioSection->EnsureTempStore(!(dwFlags & C4S_SAVE_LANDSCAPE), !(dwFlags & C4S_SAVE_OBJECTS)))
		{
			DebugLogF("LoadScenarioSection(%s): could not extract section files of current section %s", szSection, pCurrentScenarioSection->GetName());
			return false;
		}
		// open current group
		if (!(pGrp = pCurrentScenarioSection->GetGroupfile(hGroup)))
		{
			DebugLog("LoadScenarioSection: error opening current group file");
			return false;
		}
		// store landscape, if desired (w/o material enumeration - that's assumed to stay consistent during the game)
		if (dwFlags & C4S_SAVE_LANDSCAPE)
		{
			// storing the landscape implies storing the scenario core
			// otherwise, the ExactLandscape-flag would be lost
			// maybe imply exact landscapes by the existance of Landscape.png-files?
			C4Scenario rC4S = C4S;
			rC4S.SetExactLandscape();
			if (!rC4S.Save(*pGrp, true))
			{
				DebugLog("LoadScenarioSection: Error saving C4S");
				return false;
			}
			// landscape
			{
				C4DebugRecOff DBGRECOFF;
				Objects.RemoveSolidMasks();
				if (!Landscape.Save(*pGrp))
				{
					DebugLog("LoadScenarioSection: Error saving Landscape");
					return false;
				}
				Objects.PutSolidMasks();
			}
			// PXS
			if (!PXS.Save(*pGrp))
			{
				DebugLog("LoadScenarioSection: Error saving PXS");
				return false;
			}
			// MassMover (create copy, may not modify running data)
			C4MassMoverSet MassMoverSet;
			MassMoverSet.Copy(MassMover);
			if (!MassMoverSet.Save(*pGrp))
			{
				DebugLog("LoadScenarioSection: Error saving MassMover");
				return false;
			}
		}
		// store objects
		if (dwFlags & C4S_SAVE_OBJECTS)
		{
			// strings; those will have to be merged when reloaded
			if (!ScriptEngine.Strings.Save(*pGrp))
			{
				DebugLog("LoadScenarioSection: Error saving strings");
				return false;
			}
			// objects: do not save info objects or inactive objects
			if (!Objects.Save(*pGrp, false, false))
			{
				DebugLog("LoadScenarioSection: Error saving objects");
				return false;
			}
		}
		// close current group
		if (hGroup.IsOpen()) hGroup.Close();
		// mark modified
		pCurrentScenarioSection->fModified = true;
	}
	// open section group
	if (!(pGrp = pLoadSect->GetGroupfile(hGroup)))
	{
		DebugLog("LoadScenarioSection: error opening group file");
		return false;
	}
	// remove all objects (except inactive)
	// do correct removal calls, because this will stop fire sounds, etc.
	C4ObjectLink *clnk;
	for (clnk = Objects.First; clnk; clnk = clnk->Next) clnk->Obj->AssignRemoval();
	for (clnk = Objects.First; clnk; clnk = clnk->Next)
		if (clnk->Obj->Status)
		{
			DebugLogF("LoadScenarioSection: WARNING: Object %d created in destruction process!", static_cast<int>(clnk->Obj->Number));
			ClearPointers(clnk->Obj);
			// clnk->Obj->AssignRemoval(); - this could create additional objects in endless recursion...
		}
	DeleteObjects(false);
	// remove global effects
	if (pGlobalEffects) if (~dwFlags | C4S_KEEP_EFFECTS)
	{
		pGlobalEffects->ClearAll(nullptr, C4FxCall_RemoveClear);
		// scenario section call might have been done from a global effect
		// rely on dead effect removal for actually removing the effects; do not clear the array here!
	}
	// del particles as well
	Particles.ClearParticles();
	// clear transfer zones
	TransferZones.Clear();
	// backup old sky
	char szOldSky[C4MaxDefString + 1];
	SCopy(C4S.Landscape.SkyDef, szOldSky, C4MaxDefString);
	// overload scenario values (fails if no scenario core is present; that's OK)
	C4S.Load(*pGrp, true);
	// determine whether a new sky has to be loaded
	bool fLoadNewSky = !SEqualNoCase(szOldSky, C4S.Landscape.SkyDef) || pGrp->FindEntry(C4CFN_Sky ".*");
	// re-init game in new section
	if (!InitGame(*pGrp, pLoadSect, fLoadNewSky))
	{
		DebugLog("LoadScenarioSection: Error reiniting game");
		return false;
	}
	// set new current section
	pCurrentScenarioSection = pLoadSect;
	SCopy(pCurrentScenarioSection->GetName(), CurrentScenarioSection);
	// resize viewports
	GraphicsSystem.RecalculateViewports();

	for (auto plr = Players.First; plr; plr = plr->Next)
	{
		plr->ApplyForcedControl();
	}

	// done, success
	return true;
}

bool C4Game::ToggleDebugMode()
{
	// debug mode not allowed
	if (!Parameters.AllowDebug && !DebugMode) { GraphicsSystem.FlashMessage(LoadResStr("IDS_MSG_DEBUGMODENOTALLOWED")); return false; }
	Toggle(DebugMode);
	if (!DebugMode) GraphicsSystem.DeactivateDebugOutput();
	GraphicsSystem.FlashMessageOnOff(LoadResStr("IDS_CTL_DEBUGMODE"), DebugMode);
	return true;
}

bool C4Game::ToggleChart()
{
	C4ChartDialog::Toggle();
	return true;
}

void C4Game::Abort(bool fApproved)
{
	// league needs approval
	if (Network.isEnabled() && Parameters.isLeague() && !fApproved)
	{
		if (Control.isCtrlHost() && !GameOver)
		{
			Network.Vote(VT_Cancel);
			return;
		}
		if (!Control.isCtrlHost() && !GameOver && Players.GetLocalByIndex(0))
		{
			Network.Vote(VT_Kick, true, Control.ClientID());
			return;
		}
	}
	// hard-abort: eval league and quit
	// manually evaluate league
	Players.RemoveLocal(true, true);
	Players.RemoveAtRemoteClient(true, true);
	// normal quit
	Application.QuitGame();
}

bool C4Game::DrawTextSpecImage(C4FacetExSurface &fctTarget, const char *szSpec, uint32_t dwClr)
{
	// safety
	if (!szSpec) return false;
	// regular ID? -> Draw def
	if (LooksLikeID(szSpec))
	{
		C4Def *pDef = C4Id2Def(C4Id(szSpec));
		if (!pDef) return false;
		pDef->Picture2Facet(fctTarget, dwClr);
		return true;
	}
	// C4ID:Index?
	if (SLen(szSpec) > 5 && szSpec[4] == ':')
	{
		char idbuf[5]; SCopy(szSpec, idbuf, 4);
		if (LooksLikeID(idbuf))
		{
			int iIndex = -1;
			if (sscanf(szSpec + 5, "%d", &iIndex) == 1) if (iIndex >= 0)
			{
				C4Def *pDef = C4Id2Def(C4Id(idbuf));
				if (!pDef) return false;
				pDef->Picture2Facet(fctTarget, dwClr, iIndex);
				return true;
			}
		}
	}
	// portrait spec?
	if (SEqual2(szSpec, "Portrait:"))
	{
		szSpec += 9;
		C4ID idPortrait;
		const char *szPortraitName = C4Portrait::EvaluatePortraitString(szSpec, idPortrait, C4ID_None, &dwClr);
		if (idPortrait == C4ID_None) return false;
		C4Def *pPortraitDef = Defs.ID2Def(idPortrait);
		if (!pPortraitDef || !pPortraitDef->Portraits) return false;
		C4DefGraphics *pDefPortraitGfx = pPortraitDef->Portraits->Get(szPortraitName);
		if (!pDefPortraitGfx) return false;
		C4PortraitGraphics *pPortraitGfx = pDefPortraitGfx->IsPortrait();
		if (!pPortraitGfx) return false;
		C4Surface *sfcPortrait = pPortraitGfx->GetBitmap(dwClr);
		if (!sfcPortrait) return false;
		fctTarget.Set(sfcPortrait, 0, 0, sfcPortrait->Wdt, sfcPortrait->Hgt);
		return true;
	}
	else if (SEqual2(szSpec, "Ico:Locked"))
	{
		static_cast<C4Facet &>(fctTarget) = C4GUI::Icon::GetIconFacet(C4GUI::Ico_Ex_LockedFrontal);
		return true;
	}
	else if (SEqual2(szSpec, "Ico:League"))
	{
		static_cast<C4Facet &>(fctTarget) = C4GUI::Icon::GetIconFacet(C4GUI::Ico_Ex_League);
		return true;
	}
	else if (SEqual2(szSpec, "Ico:GameRunning"))
	{
		static_cast<C4Facet &>(fctTarget) = C4GUI::Icon::GetIconFacet(C4GUI::Ico_GameRunning);
		return true;
	}
	else if (SEqual2(szSpec, "Ico:Lobby"))
	{
		static_cast<C4Facet &>(fctTarget) = C4GUI::Icon::GetIconFacet(C4GUI::Ico_Lobby);
		return true;
	}
	else if (SEqual2(szSpec, "Ico:RuntimeJoin"))
	{
		static_cast<C4Facet &>(fctTarget) = C4GUI::Icon::GetIconFacet(C4GUI::Ico_RuntimeJoin);
		return true;
	}
	else if (SEqual2(szSpec, "Ico:FairCrew"))
	{
		static_cast<C4Facet &>(fctTarget) = C4GUI::Icon::GetIconFacet(C4GUI::Ico_Ex_FairCrew);
		return true;
	}
	else if (SEqual2(szSpec, "Ico:Settlement"))
	{
		static_cast<C4Facet &>(fctTarget) = GraphicsResource.fctScore;
		return true;
	}
	// unknown spec
	return false;
}

bool C4Game::SpeedUp()
{
	// As these functions work stepwise, there's the old maximum speed of 50.
	// Use /fast to set to even higher speeds.
	FrameSkip = BoundBy<int32_t>(FrameSkip + 1, 1, 50);
	FullSpeed = true;
	GraphicsSystem.FlashMessage(FormatString(LoadResStr("IDS_MSG_SPEED"), FrameSkip).getData());
	return true;
}

bool C4Game::SlowDown()
{
	FrameSkip = BoundBy<int32_t>(FrameSkip - 1, 1, 50);
	if (FrameSkip == 1)
		FullSpeed = false;
	GraphicsSystem.FlashMessage(FormatString(LoadResStr("IDS_MSG_SPEED"), FrameSkip).getData());
	return true;
}

void C4Game::SetMusicLevel(int32_t iToLvl)
{
	// change game music volume; multiplied by config volume for real volume
	iMusicLevel = BoundBy<int32_t>(iToLvl, 0, 100);
	Application.MusicSystem->UpdateVolume();
}

bool C4Game::ToggleMusic()
{
	Application.MusicSystem->ToggleOnOff(!IsRunning);
	return true;
}

bool C4Game::ToggleSound()
{
	Application.SoundSystem->ToggleOnOff();
	return true;
}

void C4Game::AddDirectoryForMonitoring(const char *const directory)
{
	if (FileMonitor)
	{
		FileMonitor->AddDirectory(directory);
	}
}

bool C4Game::ToggleChat()
{
	return C4ChatDlg::ToggleChat();
}
