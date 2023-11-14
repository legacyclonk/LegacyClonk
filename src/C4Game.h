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

/* Main class to run the game */

#pragma once

#include <C4Def.h>
#include "C4Section.h"
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
#include <C4NetworkRestartInfos.h>
#include "C4FileMonitor.h"

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
	std::vector<std::unique_ptr<C4Section>> Sections;
	C4Group ScenarioFile;
	C4Scenario C4S;
	std::string Loader;
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
	C4ComponentHost Info;
	C4ComponentHost Title;
	C4ComponentHost Names;
	C4ComponentHost GameText;
	C4AulScriptEngine ScriptEngine;
	C4GameScriptHost Script;
	C4LangStringTable MainSysLangStringTable, ScenarioLangStringTable, ScenarioSysLangStringTable;
	C4LoadedParticleList Particles;
	C4PlayerList Players;
	StdStrBuf PlayerNames;
	C4GameControl Control;
	C4Control &Input; // shortcut

	C4GroupSet GroupSet;
	C4Group *pParentGroup;
	C4Extra Extra;
	C4GUI::Screen *pGUI;
	C4ScenarioSection *pScenarioSections, *pCurrentScenarioSection;
	C4Effect *pGlobalEffects;
#ifndef USE_CONSOLE
	// We don't need fonts when we don't have graphics
	C4FontLoader FontLoader;
#endif
	C4Scoreboard Scoreboard;
	class C4Network2Stats *pNetworkStatistics; // may be nullptr if no statistics are recorded
	class C4KeyboardInput &KeyboardInput;
	char CurrentScenarioSection[C4MaxName + 1];
	char ScenarioFilename[_MAX_PATH + 1];
	char PlayerFilenames[20 * _MAX_PATH + 1];
	std::vector<std::string> DefinitionFilenames;
	bool FixedDefinitions;
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
	StdStrBuf RecordDumpFile;
	StdStrBuf RecordStream;
	bool TempScenarioFile;
	bool fPreinited; // set after PreInit has been called; unset by Clear and Default
	int32_t FrameCounter;
	int32_t iTick2, iTick3, iTick5, iTick10, iTick35, iTick255, iTick500, iTick1000;
	bool TimeGo;
	int32_t Time;
	int32_t StartTime;
	int32_t InitProgress; int32_t LastInitProgress;
	int32_t ObjectEnumerationIndex;
	int32_t Rules;
	bool GameGo;
	bool FullSpeed;
	int32_t FrameSkip; bool DoSkipFrame;
	uint32_t FoWColor; // FoW-color; may contain transparency
	bool fResortAnyObject; // if set, object list will be checked for unsorted objects next frame
	bool IsRunning; // (NoSave) if set, the game is running; if not, just the startup message board is painted
	bool PointersDenumerated; // (NoSave) set after object pointers have been denumerated
	bool fQuitWithError; // if set, game shut down irregularly
	bool IsMusicEnabled;
	int32_t iMusicLevel; // scenario-defined music level
	// current play list
	StdStrBuf PlayList;
	bool DebugMode;
	// next mission to be played after this one
	StdStrBuf NextMission, NextMissionText, NextMissionDesc;
	C4NetworkRestartInfos::Infos RestartRestoreInfos;

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
	void InitLogger();
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
	void ReloadFile(const char *path);
	bool ReloadDef(C4ID id, uint32_t reloadWhat = C4D_Load_RX);
	bool ReloadParticle(const char *szName);
	// Object functions
	void ClearPointers(C4Object *cobj);
	void UpdateScriptPointers();
	void UpdateMaterialScriptPointers();
	C4Object *ObjectPointer(std::int32_t number);
	C4Object *SafeObjectPointer(std::int32_t number);
	std::int32_t ObjectNumber(C4Object *obj);

	auto GetAllObjects()
	{
		return Sections
				| std::views::transform([](const auto &section) { return C4LinkedListIterator<&C4ObjectLink::Next>{section->Objects.First}; })
				| std::views::join
				| std::views::transform(&C4ObjectLink::Obj);
	}

	auto GetAllObjectsWithStatus()
	{
		return GetAllObjects() | std::views::filter(&C4Object::Status);
	}

	std::int32_t GetSectionIndex(C4Section &section) const noexcept
	{
		for (std::size_t i{0}; i < Sections.size(); ++i)
		{
			if (&section == Sections[i].get())
			{
				return static_cast<std::int32_t>(i);
			}
		}

		return -1;
	}

	C4Section *GetSectionByIndex(const std::int32_t index) noexcept
	{
		if (Inside(index, 0, static_cast<std::int32_t>(Sections.size() - 1)))
		{
			return Sections[index].get();
		}

		return nullptr;
	}

	void AssignPlrViewRange()
	{
		std::ranges::for_each(Sections, &C4ObjectList::AssignPlrViewRange, &C4Section::Objects);
	}

	template<typename Func, typename T = std::invoke_result_t<Func, C4GameObjects &>>
	decltype(auto) FindFirstInAllObjects(Func &&func, T defaultValue = {})
	{
		for (const auto &section : Sections)
		{
			if (decltype(auto) value = std::invoke(std::forward<Func>(func), section->Objects); value)
			{
				return value;
			}
		}

		return defaultValue;
	}

	void ResetAudibility()
	{
		std::ranges::for_each(Sections, &C4ObjectList::ResetAudibility, &C4Section::Objects);
	}

	void ValidateOwners()
	{
		std::ranges::for_each(Sections, &C4ObjectList::ValidateOwners, &C4Section::Objects);
	}

	void SortByCategory()
	{
		std::ranges::for_each(Sections, &C4ObjectList::SortByCategory, &C4Section::Objects);
	}

	bool LoadScenarioSection(const char *szSection, uint32_t dwFlags);

	bool DrawTextSpecImage(C4FacetExSurface &fctTarget, const char *szSpec, uint32_t dwClr = 0xff);
	bool SpeedUp();
	bool SlowDown();
	bool InitKeyboard(); // register main keyboard input functions

protected:
	bool InitSystem();
	void InitValueOverloads();
	void InitGoals();
	void InitRules();
	void UpdateRules();
	void CloseScenario();
	void Ticks();
	std::vector<std::string> FoldersWithLocalsDefs(std::string path);
	bool DefinitionFilenamesFromSaveGame();
	bool LoadScenarioComponents();
	void LoadScenarioScripts();

public:
	bool SaveGameTitle(C4Group &hGroup);
	bool Preload();
	bool CanPreload() const;

protected:
	bool InitGame(C4Group &hGroup, bool fLoadSky);
	bool InitGameFirstPart();
	bool InitGameSecondPart(C4Group &hGroup, bool fLoadSky, bool preloading);
	bool InitGameFinal();
	bool InitNetworkFromAddress(const char *szAddress);
	bool InitNetworkFromReference(const C4Network2Reference &Reference);
	bool InitNetworkHost();
	bool InitControl();
	bool InitScriptEngine();
	void LinkScriptEngine();
	bool InitPlayers();
	bool OpenScenario();
	bool InitDefs();
	bool EnumerateMaterials();
	bool GameOverCheck();
	bool Compile(const char *szSource);
	bool Decompile(std::string &buf, bool fSaveSection, bool fSaveExact);

public:
	void CompileFunc(StdCompiler *pComp, CompileSettings comp);
	bool SaveData(C4Group &hGroup, bool fSaveSection, bool fInitial, bool fSaveExact);

protected:
	bool CompileRuntimeData(C4ComponentHost &rGameData);


	bool ToggleDebugMode(); // dbg modeon/off if allowed

public:
	bool ToggleChart(); // chart dlg on/off
	void SetMusicLevel(int32_t iToLvl); // change game music volume; multiplied by config volume for real volume
	bool ToggleMusic(); // music on / off
	bool ToggleSound(); // sound on / off
	void AddDirectoryForMonitoring(const char *directory);

protected:
	enum class PreloadLevel
	{
		None,
		Scenario,
		Basic,
		LandscapeObjects
	};
	std::thread PreloadThread;
	PreloadLevel PreloadStatus;
	CStdCSecEx PreloadMutex;
	bool LandscapeLoaded;
	std::unique_ptr<C4FileMonitor> FileMonitor;
};

const int32_t C4RULE_StructuresNeedEnergy      = 1,
              C4RULE_ConstructionNeedsMaterial = 2,
              C4RULE_FlagRemoveable            = 4,
              C4RULE_StructuresSnowIn          = 8,
              C4RULE_TeamHombase               = 16;

extern C4Game Game;

// a global wrapper
inline std::string GetKeyboardInputName(const char *szKeyName, bool fShort = false, int32_t iIndex = 0)
{
	return Game.KeyboardInput.GetKeyCodeNameByKeyName(szKeyName, fShort, iIndex);
}
