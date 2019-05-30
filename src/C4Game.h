/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
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

/* Main class to run the game */

#pragma once

#ifdef C4ENGINE

#include <C4Def.h>
#include <C4Texture.h>
#include <C4RankSystem.h>
#include <C4GraphicsSystem.h>
#include <C4GraphicsResource.h>
#include <C4GameMessage.h>
#include <C4MouseControl.h>
#include <C4MessageInput.h>
#include <C4Weather.h>
#include <C4Material.h>
#include <C4GameObjects.h>
#include <C4Landscape.h>
#include <C4Scenario.h>
#include <C4MassMover.h>
#include <C4PXS.h>
#include <C4PlayerList.h>
#include <C4Teams.h>
#include <C4PlayerInfo.h>
#include <C4Control.h>
#include <C4PathFinder.h>
#include <C4ComponentHost.h>
#include <C4ScriptHost.h>
#include <C4Particles.h>
#include <C4GroupSet.h>
#include <C4Extra.h>
#include <C4GameControl.h>
#include <C4Effects.h>
#include <C4Fonts.h>
#include "C4LangStringTable.h"
#include "C4Scoreboard.h"
#include <C4Network2.h>
#include <C4Scenario.h>
#include <C4Client.h>
#include <C4Network2Reference.h>
#include <C4RoundResults.h>

class C4Game
{
private:
	// used as StdCompiler-parameter
	struct CompileSettings
	{
		bool fScenarioSection;
		bool fPlayers;
		bool fExact;

		CompileSettings(bool fScenarioSection, bool fPlayers, bool fExact)
			: fScenarioSection(fScenarioSection), fPlayers(fPlayers), fExact(fExact) {}
	};

	// struct of keyboard set and indexed control key
	struct C4KeySetCtrl
	{
		int32_t iKeySet, iCtrl;

		C4KeySetCtrl(int32_t iKeySet, int32_t iCtrl) : iKeySet(iKeySet), iCtrl(iCtrl) {}
	};

public:
	C4Game();
	~C4Game();

public:
	C4DefList Defs;
	C4TextureMap TextureMap;
	C4RankSystem Rank;
	C4GraphicsSystem GraphicsSystem;
	C4MessageInput MessageInput;
	C4GraphicsResource GraphicsResource;
	C4Network2 Network;
	C4ClientList &Clients; // Shortcut
	C4GameParameters Parameters;
	C4TeamList &Teams; // Shortcut
	C4PlayerInfoList &PlayerInfos; // Shortcut
	C4PlayerInfoList &RestorePlayerInfos; // Shortcut
	C4RoundResults RoundResults;
	C4GameMessageList Messages;
	C4MouseControl MouseControl;
	C4Weather Weather;
	C4MaterialMap Material;
	C4GameObjects Objects;
	C4ObjectList BackObjects; // objects in background (C4D_Background)
	C4ObjectList ForeObjects; // objects in foreground (C4D_Foreground)
	C4Landscape Landscape;
	C4Scenario C4S;
	C4ComponentHost Info;
	C4ComponentHost Title;
	C4ComponentHost Names;
	C4ComponentHost GameText;
	C4AulScriptEngine ScriptEngine;
	C4GameScriptHost Script;
	C4LangStringTable MainSysLangStringTable, ScenarioLangStringTable, ScenarioSysLangStringTable;
	C4MassMoverSet MassMover;
	C4PXSSystem PXS;
	C4ParticleSystem Particles;
	C4PlayerList Players;
	StdStrBuf PlayerNames;
	C4GameControl Control;
	C4Control &Input; // shortcut

	C4PathFinder PathFinder;
	C4TransferZones TransferZones;
	C4Group ScenarioFile;
	C4GroupSet GroupSet;
	C4Group *pParentGroup;
	C4Extra Extra;
	C4GUIScreen *pGUI;
	C4ScenarioSection *pScenarioSections, *pCurrentScenarioSection;
	C4Effect *pGlobalEffects;
#ifndef USE_CONSOLE
	// We don't need fonts when we don't have graphics
	C4FontLoader FontLoader;
#endif
	C4Scoreboard Scoreboard;
	class C4Network2Stats *pNetworkStatistics; // may be nullptr if no statistics are recorded
	class C4KeyboardInput &KeyboardInput;
	class C4FileMonitor *pFileMonitor;
	char CurrentScenarioSection[C4MaxName + 1];
	char ScenarioFilename[_MAX_PATH + 1];
	StdCopyStrBuf ScenarioTitle;
	char PlayerFilenames[20 * _MAX_PATH + 1];
	char DefinitionFilenames[20 * _MAX_PATH + 1];
	char DirectJoinAddress[_MAX_PATH + 1];
	class C4Network2Reference *pJoinReference;
	int32_t FPS, cFPS;
	int32_t HaltCount;
	bool GameOver;
	bool Evaluated;
	bool GameOverDlgShown;
	bool fScriptCreatedObjects;
	bool fLobby;
	int32_t iLobbyTimeout;
	bool fObserve;
	bool NetworkActive;
	bool Record;
	bool Verbose; // default false; set to true only by command line
	StdStrBuf RecordDumpFile;
	StdStrBuf RecordStream;
	bool TempScenarioFile;
	bool fPreinited; // set after PreInit has been called; unset by Clear and Default
	int32_t FrameCounter;
	int32_t iTick2, iTick3, iTick5, iTick10, iTick35, iTick255, iTick500, iTick1000;
	bool TimeGo;
	int32_t Time;
	int32_t StartTime;
	int32_t InitProgress; int32_t LastInitProgress; int32_t LastInitProgressShowTime;
	int32_t ObjectEnumerationIndex;
	int32_t Rules;
	bool GameGo;
	bool FullSpeed;
	int32_t FrameSkip; bool DoSkipFrame;
	uint32_t FoWColor; // FoW-color; may contain transparency
	bool fResortAnyObject; // if set, object list will be checked for unsorted objects next frame
	bool IsRunning; // (NoSave) if set, the game is running; if not, just the startup message board is painted
	bool PointersDenumerated; // (NoSave) set after object pointers have been denumerated
	size_t StartupLogPos, QuitLogPos; // current log positions when game was last started and cleared
	bool fQuitWithError; // if set, game shut down irregularly
	int32_t iMusicLevel; // scenario-defined music level
	// current play list
	StdCopyStrBuf PlayList;
	bool DebugMode;
	// next mission to be played after this one
	StdCopyStrBuf NextMission, NextMissionText, NextMissionDesc;

public:
	// Init and execution
	void Default();
	void Clear();
	void Abort(bool fApproved = false); // hard-quit on Esc+Y (/J/O)
	void Evaluate();
	void ShowGameOverDlg();
	void Sec1Timer();
	bool DoKeyboardInput(C4KeyCode vk_code, C4KeyEventType eEventType, bool fAlt, bool fCtrl, bool fShift, bool fRepeated, class C4GUI::Dialog *pForDialog = nullptr, bool fPlrCtrlOnly = false);
	void DrawCursors(C4FacetEx &cgo, int32_t iPlayer);
	bool LocalControlKey(C4KeyCodeEx key, C4KeySetCtrl Ctrl);
	bool LocalControlKeyUp(C4KeyCodeEx key, C4KeySetCtrl Ctrl);
	void LocalPlayerControl(int32_t iPlayer, int32_t iCom);
	void FixRandom(int32_t iSeed);
	bool Init();
	bool PreInit();
	void ParseCommandLine(const char *szCmdLine);
	bool Execute();
	class C4Player *JoinPlayer(const char *szFilename, int32_t iAtClient, const char *szAtClientName, C4PlayerInfo *pInfo);
	bool DoGameOver();
	bool CanQuickSave();
	bool QuickSave(const char *strFilename, const char *strTitle, bool fForceSave = false);
	void SetInitProgress(float fToProgress);
	void OnResolutionChanged(); // update anything that's dependent on screen resolution
	void InitFullscreenComponents(bool fRunning);
	bool ToggleChat();
	// Pause
	bool TogglePause();
	bool Pause();
	bool Unpause();
	bool IsPaused();
	// Network
	void Synchronize(bool fSavePlayerFiles);
	void SyncClearance();
	// Editing
	bool DropFile(const char *szFilename, int32_t iX, int32_t iY);
	bool CreateViewport(int32_t iPlayer, bool fSilent = false);
	bool DropDef(C4ID id, int32_t iX, int32_t iY);
	bool ReloadFile(const char *szPath);
	bool ReloadDef(C4ID id);
	bool ReloadParticle(const char *szName);
	// Object functions
	void ClearPointers(C4Object *cobj);
	C4Object *CreateObject(C4ID type, C4Object *pCreator, int32_t owner = NO_OWNER,
		int32_t x = 50, int32_t y = 50, int32_t r = 0,
		FIXED xdir = Fix0, FIXED ydir = Fix0, FIXED rdir = Fix0, int32_t iController = NO_OWNER);
	C4Object *CreateObjectConstruction(C4ID type,
		C4Object *pCreator,
		int32_t owner,
		int32_t ctx = 0, int32_t bty = 0,
		int32_t con = 1, bool terrain = false);
	C4Object *CreateInfoObject(C4ObjectInfo *cinf, int32_t owner,
		int32_t tx = 50, int32_t ty = 50);
	void BlastObjects(int32_t tx, int32_t ty, int32_t level, C4Object *inobj, int32_t iCausedBy, C4Object *pByObj);
	void ShakeObjects(int32_t tx, int32_t ry, int32_t range, int32_t iCausedBy);
	C4Object *OverlapObject(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt,
		int32_t category);
	C4Object *FindObject(C4ID id,
		int32_t iX = 0, int32_t iY = 0, int32_t iWdt = 0, int32_t iHgt = 0,
		uint32_t ocf = OCF_All,
		const char *szAction = nullptr, C4Object *pActionTarget = nullptr,
		C4Object *pExclude = nullptr,
		C4Object *pContainer = nullptr,
		int32_t iOwner = ANY_OWNER,
		C4Object *pFindNext = nullptr);
	C4Object *FindVisObject( // find object in view at pos, regarding parallaxity and visibility (but not distance)
		int32_t tx, int32_t ty, int32_t iPlr, const C4Facet &fctViewport,
		int32_t iX = 0, int32_t iY = 0, int32_t iWdt = 0, int32_t iHgt = 0,
		uint32_t ocf = OCF_All,
		C4Object *pExclude = nullptr,
		int32_t iOwner = ANY_OWNER,
		C4Object *pFindNext = nullptr);
	int32_t ObjectCount(C4ID id,
		int32_t x = 0, int32_t y = 0, int32_t wdt = 0, int32_t hgt = 0,
		uint32_t ocf = OCF_All,
		const char *szAction = nullptr, C4Object *pActionTarget = nullptr,
		C4Object *pExclude = nullptr,
		C4Object *pContainer = nullptr,
		int32_t iOwner = ANY_OWNER);
	C4Object *FindBase(int32_t iPlayer, int32_t iIndex);
	C4Object *FindFriendlyBase(int32_t iPlayer, int32_t iIndex);
	C4Object *FindObjectByCommand(int32_t iCommand, C4Object *pTarget = nullptr, C4Value iTx = C4VNull, int32_t iTy = 0, C4Object *pTarget2 = nullptr, C4Object *pFindNext = nullptr);
	void CastObjects(C4ID id, C4Object *pCreator, int32_t num, int32_t level, int32_t tx, int32_t ty, int32_t iOwner = NO_OWNER, int32_t iController = NO_OWNER);
	void BlastCastObjects(C4ID id, C4Object *pCreator, int32_t num, int32_t tx, int32_t ty, int32_t iController = NO_OWNER);
	C4Object *PlaceVegetation(C4ID id, int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, int32_t iGrowth);
	C4Object *PlaceAnimal(C4ID idAnimal);

	bool LoadScenarioSection(const char *szSection, uint32_t dwFlags);

	bool DrawTextSpecImage(C4FacetExSurface &fctTarget, const char *szSpec, uint32_t dwClr = 0xff);
	bool SpeedUp();
	bool SlowDown();
	bool InitKeyboard(); // register main keyboard input functions

protected:
	bool InitSystem();
	void InitInEarth();
	void InitVegetation();
	void InitAnimals();
	void InitGoals();
	void InitRules();
	void InitValueOverloads();
	void InitEnvironment();
	void UpdateRules();
	void CloseScenario();
	void DeleteObjects(bool fDeleteInactive);
	void ExecObjects();
	void Ticks();
	const char *FoldersWithLocalsDefs(const char *szPath);
	bool CheckObjectEnumeration();
	bool DefinitionFilenamesFromSaveGame();
	bool LoadScenarioComponents();
	bool LoadScenarioScripts();

public:
	bool SaveGameTitle(C4Group &hGroup);

protected:
	bool InitGame(C4Group &hGroup, C4ScenarioSection *section, bool fLoadSky);
	bool InitGameFinal();
	bool InitNetworkFromAddress(const char *szAddress);
	bool InitNetworkFromReference(const C4Network2Reference &Reference);
	bool InitNetworkHost();
	bool InitControl();
	bool InitScriptEngine();
	bool LinkScriptEngine();
	bool InitPlayers();
	bool OpenScenario();
	bool InitDefs();
	bool InitMaterialTexture();
	bool EnumerateMaterials();
	bool GameOverCheck();
	bool PlaceInEarth(C4ID id);
	bool Compile(const char *szSource);
	bool Decompile(StdStrBuf &rBuf, bool fSaveSection, bool fSaveExact);

public:
	void CompileFunc(StdCompiler *pComp, CompileSettings comp);
	bool SaveData(C4Group &hGroup, bool fSaveSection, bool fInitial, bool fSaveExact);

protected:
	bool CompileRuntimeData(C4ComponentHost &rGameData);

	// Object function internals
	C4Object *NewObject(C4Def *ndef, C4Object *pCreator,
		int32_t owner, C4ObjectInfo *info,
		int32_t tx, int32_t ty, int32_t tr,
		FIXED xdir, FIXED ydir, FIXED rdir,
		int32_t con, int32_t iController);
	void ClearObjectPtrs(C4Object *tptr);
	void ObjectRemovalCheck();

	bool ToggleDebugMode(); // dbg modeon/off if allowed

public:
	bool ToggleChart(); // chart dlg on/off
	void SetMusicLevel(int32_t iToLvl); // change game music volume; multiplied by config volume for real volume
};

const int32_t C4RULE_StructuresNeedEnergy      = 1,
              C4RULE_ConstructionNeedsMaterial = 2,
              C4RULE_FlagRemoveable            = 4,
              C4RULE_StructuresSnowIn          = 8,
              C4RULE_TeamHombase               = 16;

extern char OSTR[500];

extern C4Game Game;

// a global wrapper
inline StdStrBuf GetKeyboardInputName(const char *szKeyName, bool fShort = false, int32_t iIndex = 0)
{
	return Game.KeyboardInput.GetKeyCodeNameByKeyName(szKeyName, fShort, iIndex);
}

#endif // C4ENGINE
