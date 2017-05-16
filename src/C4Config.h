/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Game configuration as stored in registry */

#pragma once

#include "C4Constants.h"
#include "C4InputValidation.h"

#define C4CFG_Company "RedWolf Design"
#define C4CFG_Product "Clonk Rage"

#define C4CFG_LeagueServer   "www.clonk.de:84/league/server"
#define C4CFG_FallbackServer "www.clonk.de:80/league/server"

#define C4CFG_UpdateServer "www.clonkx.de/rage/update" // for download files, replace 'update' with the below

#define C4CFG_UpdateEngine  "cr_%d_%s.c4u"
#define C4CFG_UpdateObjects "cr_%d%d%d%d_%d_%s.c4u"
#define C4CFG_UpdateMajor   "cr_%d%d%d%d_%s.c4u"

class C4ConfigGeneral
{
public:
	enum { ConfigResetSafetyVal = 42 };

	char Name[CFG_MaxString + 1];
	char Language[CFG_MaxString + 1]; // entered by user in frontend options (may contain comma separated list or long language descriptions)
	char LanguageEx[CFG_MaxString + 1]; // full fallback list composed by frontend options (condensed comma separated list)
	char LanguageCharset[CFG_MaxString + 1];
	char Definitions[CFG_MaxString + 1];
	char Participants[CFG_MaxString + 1];
	int32_t AlwaysDebug; // if set: turns on debugmode whenever engine is started
	char RXFontName[CFG_MaxString + 1];
	int32_t RXFontSize;
	char PlayerPath[CFG_MaxString + 1];
	char DefinitionPath[CFG_MaxString + 1];
	char UserPath[CFG_MaxString + 1]; // absolute path; environment variables are stored and only expanded upon evaluation
	StdStrBuf SaveGameFolder;
	StdStrBuf SaveDemoFolder;
	StdStrBuf ScreenshotFolder;
	char MissionAccess[CFG_MaxString + 1];
	int32_t FPS;
	int32_t Record;
	int32_t DefRec;
	int32_t MMTimer;    // use multimedia-timers
	int32_t FairCrew;   // don't use permanent crew physicals
	int32_t FairCrewStrength; // strength of clonks in fair crew mode
	int32_t MouseAScroll; // auto scroll strength
	int32_t ScrollSmooth; // view movement smoothing
	int32_t ConfigResetSafety; // safety value: If this value is screwed, the config got currupted and must be reset
	// Determined at run-time
	char ExePath[CFG_MaxString + 1];
	char TempPath[CFG_MaxString + 1];
	char LogPath[CFG_MaxString + 1];
	char ScreenshotPath[CFG_MaxString + 1];
	bool GamepadEnabled;
	bool FirstStart;
	bool fUTF8;
	bool UserPortraitsWritten; // set when default portraits have been copied to the UserPath (this is only done once)

public:
	static int GetLanguageSequence(const char *strSource, char *strTarget);
	void DefaultLanguage();
	BOOL CreateSaveFolder(const char *strDirectory, const char *strLanguageTitle);
	void DeterminePaths(BOOL forceWorkingDirectory);
	void CompileFunc(StdCompiler *pComp);
};

class C4ConfigDeveloper
{
public:
	int32_t AutoFileReload;
	void CompileFunc(StdCompiler *pComp);
};

class C4ConfigGraphics
{
public:
	int32_t SplitscreenDividers;
	int32_t ShowPlayerInfoAlways;
	int32_t ShowCommands;
	int32_t ShowCommandKeys;
	int32_t ShowPortraits;
	int32_t AddNewCrewPortraits;
	int32_t SaveDefaultPortraits;
	int32_t ShowStartupMessages;
	int32_t VerboseObjectLoading;
	int32_t ColorAnimation;
	int32_t SmokeLevel;
	int32_t VideoModule;
	int32_t UpperBoard;
	int32_t ShowClock;
	int32_t ResX, ResY;
	int32_t ShowAllResolutions;
	int32_t ShowCrewNames; // show player name above clonks?
	int32_t ShowCrewCNames; // show clonk names above clonks?
	int32_t BitDepth; // used bit depth for newgfx
	int32_t NewGfxCfg;   // some configuration settings for newgfx
	int32_t NewGfxCfgGL; // some configuration settings for newgfx (OpenGL)
	int32_t MsgBoard;
	int32_t PXSGfx; // show PXS-graphics (instead of sole pixels)
	int32_t Engine; // 0: D3D; 1: OpenGL;
	int32_t BlitOff;     // blit offset (percent)
	int32_t BlitOffGL;   // blit offset (percent) (OpenGL)
	int32_t TexIndent;   // blit offset (per mille)
	int32_t TexIndentGL; // blit offset (per mille) (OpenGL)
	int32_t Gamma1, Gamma2, Gamma3; // gamma ramps
	int32_t Currency; // default wealth symbolseb
	int32_t RenderInactiveEM; // draw vieports even if inactive in CPEM
	int32_t DisableGamma;
	int32_t Monitor; // monitor index to play on
	int32_t FireParticles; // draw extended fire particles if enabled (defualt on)
	int32_t MaxRefreshDelay; // minimum time after which graphics should be refreshed (ms)
	int32_t AutoFrameSkip; // if true, gfx frames are skipped when they would slow down the game
	void CompileFunc(StdCompiler *pComp);
};

class C4ConfigSound
{
public:
	int32_t RXSound;
	int32_t RXMusic;
	int32_t FEMusic;
	int32_t FESamples;
	int32_t FMMode;
	int32_t Verbose; // show music files names
	int32_t MusicVolume;
	int32_t SoundVolume;
	void CompileFunc(StdCompiler *pComp);
};

class C4ConfigNetwork
{
public:
	int32_t ControlRate;
	int32_t Lobby;
	int32_t NoRuntimeJoin;
	int32_t MaxResSearchRecursion;
	char WorkPath[CFG_MaxString + 1];
	ValidatedStdCopyStrBuf<C4InVal::VAL_Comment> Comment;
	int32_t MasterServerSignUp;
	int32_t MasterReferencePeriod;
	int32_t LeagueServerSignUp;
	int32_t UseAlternateServer;
	int32_t PortTCP, PortUDP, PortDiscovery, PortRefServer;
	int32_t ControlMode;
	ValidatedStdCopyStrBuf<C4InVal::VAL_NameNoEmpty> LocalName;
	ValidatedStdCopyStrBuf<C4InVal::VAL_NameAllowEmpty> Nick;
	int32_t MaxLoadFileSize;
	char LastPassword[CFG_MaxString + 1];
	char ServerAddress[CFG_MaxString + 1];
	char AlternateServerAddress[CFG_MaxString + 1];
	char UpdateServerAddress[CFG_MaxString + 1];
	char PuncherAddress[CFG_MaxString + 1];
	int32_t AutomaticUpdate;
	int32_t LastUpdateTime;
	int32_t AsyncMaxWait;

public:
	void CompileFunc(StdCompiler *pComp);
	const char *GetLeagueServerAddress();
};

class C4ConfigStartup
{
public:
	// config for do-not-show-this-msg-again-messages
	int32_t HideMsgGfxEngineChange;
	int32_t HideMsgGfxBitDepthChange;
	int32_t HideMsgMMTimerChange;
	int32_t HideMsgStartDedicated;
	int32_t HideMsgPlrTakeOver;
	int32_t HideMsgPlrNoTakeOver;
	int32_t HideMsgNoOfficialLeague;
	int32_t HideMsgIRCDangerous;
	int32_t NoSplash;
	int32_t AlphabeticalSorting; // if set, Folder.txt-sorting is ignored in scenario selection
	int32_t LastPortraitFolderIdx;
	void CompileFunc(StdCompiler *pComp);
};

class C4ConfigLobby
{
public:
	int32_t CountdownTime;
	int32_t AllowPlayerSave; // whether save-to-disk function is enabled for player ressources
	void CompileFunc(StdCompiler *pComp);
};

class C4ConfigIRC
{
public:
	char Server[CFG_MaxString + 1];
	char Nick[CFG_MaxString + 1];
	char RealName[CFG_MaxString + 1];
	char Channel[CFG_MaxString + 1];
	void CompileFunc(StdCompiler *pComp);
	int32_t AllowAllChannels;
};

const int C4ConfigMaxGamepads = 4;

class C4ConfigGamepad
{
public:
	int32_t Button[C4MaxKey];
	uint32_t AxisMin[6], AxisMax[6];
	bool AxisCalibrated[6];
	void CompileFunc(StdCompiler *pComp, bool fButtonsOnly = false);
	void Reset(); // reset all buttons and axis calibration to default
};

class C4ConfigControls
{
public:
	int32_t GamepadGuiControl;
	int32_t MouseAScroll; // auto scroll strength
	int32_t Keyboard[C4MaxKeyboardSet][C4MaxKey];
	void CompileFunc(StdCompiler *pComp, bool fKeysOnly = false);
	void ResetKeys(); // reset all keys to default
};

class C4Config
{
public:
	C4Config();
	~C4Config();

public:
	C4ConfigGeneral   General;
	C4ConfigDeveloper Developer;
	C4ConfigGraphics  Graphics;
	C4ConfigSound     Sound;
	C4ConfigNetwork   Network;
	C4ConfigLobby     Lobby;
	C4ConfigIRC       IRC;
	C4ConfigGamepad   Gamepads[C4ConfigMaxGamepads];
	C4ConfigControls  Controls;
	C4ConfigStartup   Startup;
	bool fConfigLoaded; // true if config has been successfully loaded
	StdStrBuf ConfigFilename; // set for configs loaded from a nondefault config file

public:
	const char *GetSubkeyPath(const char *strSubkey);
	void Default();
	BOOL Save();
	BOOL Load(BOOL forceWorkingDirectory = TRUE, const char *szConfigFile = NULL);
	BOOL Init();
	const char *AtExePath(const char *szFilename);
	const char *AtTempPath(const char *szFilename);
	const char *AtNetworkPath(const char *szFilename);
	const char *AtExeRelativePath(const char *szFilename);
	const char *AtScreenshotPath(const char *szFilename);
	const char *AtUserPath(const char *szFilename); // this one will expand environment variables on-the-fly
	void ForceRelativePath(StdStrBuf *sFilename); // try AtExeRelativePath; force GetC4Filename if not possible
	void CompileFunc(StdCompiler *pComp);
	bool IsCorrupted() { return (General.ConfigResetSafety != C4ConfigGeneral::ConfigResetSafetyVal) || !Graphics.ResX; }

protected:
	void ExpandEnvironmentVariables(char *strPath, int iMaxLen);
};

extern C4Config Config;
