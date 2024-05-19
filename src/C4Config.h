/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
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

/* Game configuration as stored in registry */

#pragma once

#include "C4Constants.h"
#include "C4Cooldown.h"
#include "C4InputValidation.h"
#include "StdConfig.h"

#ifdef C4ENGINE
#include "C4AulScriptStrict.h"
#include "StdWindow.h"
#endif

#define C4CFG_Company "LegacyClonk Team"
#define C4CFG_Product "LegacyClonk"

#define C4CFG_LeagueServer   "league.clonkspot.org:80"
#define C4CFG_FallbackServer "league.clonkspot.org:80"
#define C4CFG_OfficialLeagueServer "league.clonkspot.org"

#define C4CFG_UpdateServer "update.clonkspot.org/lc/update" // for download files, replace 'update' with the below

#define C4CFG_UpdateEngine  "lc_%d_%s.c4u"
#define C4CFG_UpdateObjects "lc_%d%d%d%d_%d_%s.c4u"
#define C4CFG_UpdateMajor   "lc_%d%d%d%d_%s.c4u"

class C4ConfigGeneral
{
public:
	enum { ConfigResetSafetyVal = 42 };

	uint32_t Version; // config version number for compatibility changes
	char Name[CFG_MaxString + 1];
	char Language[CFG_MaxString + 1]; // entered by user in frontend options (may contain comma separated list or long language descriptions)
	char LanguageEx[CFG_MaxString + 1]; // full fallback list composed by frontend options (condensed comma separated list)
	char LanguageCharset[CFG_MaxString + 1];
	char Definitions[CFG_MaxString + 1];
	char Participants[CFG_MaxString + 1];
	bool AlwaysDebug; // if set: turns on debugmode whenever engine is started
	bool AllowScriptingInReplays; // allow /script in replays (scripts can cause desyncs)
	char RXFontName[CFG_MaxString + 1];
	int32_t RXFontSize;
	char PlayerPath[CFG_MaxString + 1];
	char DefinitionPath[CFG_MaxString + 1];
	char UserPath[CFG_MaxString + 1]; // absolute path; environment variables are stored and only expanded upon evaluation
	StdStrBuf SaveGameFolder;
	StdStrBuf SaveDemoFolder;
	StdStrBuf ScreenshotFolder;
	char MissionAccess[CFG_MaxString + 1];
	bool FPS;
	bool Record;
	bool FairCrew;   // don't use permanent crew physicals
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
	bool UseWhiteIngameChat;
	bool UseWhiteLobbyChat;
	bool ShowLogTimestamps;
	bool Preloading;

public:
	static int GetLanguageSequence(const char *strSource, char *strTarget);
	void DefaultLanguage();
	bool CreateSaveFolder(const char *strDirectory, const char *strLanguageTitle);
	void DeterminePaths(bool forceWorkingDirectory);
	void CompileFunc(StdCompiler *pComp);
};

#ifdef C4ENGINE

class C4ConfigDeveloper
{
public:
	struct ConsoleScriptStrictnessWrapper
	{
		C4AulScriptStrict Strictness;

		void CompileFunc(StdCompiler *comp);

		operator C4AulScriptStrict() const noexcept { return Strictness == MaxStrictSentinel ? C4AulScriptStrict::MAXSTRICT : Strictness; }

		static constexpr auto MaxStrictSentinel = static_cast<C4AulScriptStrict>(255);
	};

public:
	bool AutoFileReload;
	ConsoleScriptStrictnessWrapper ConsoleScriptStrictness;

	void CompileFunc(StdCompiler *pComp);
};

class C4ConfigGraphics
{
public:
	enum RenderInactive : uint32_t
	{
		Fullscreen = 1 << 0,
		Console = 1 << 1
	};

	int32_t SplitscreenDividers;
	bool ShowPlayerHUDAlways;
	bool ShowCommands;
	bool ShowCommandKeys;
	bool ShowPortraits;
	bool AddNewCrewPortraits;
	bool SaveDefaultPortraits;
	int32_t VerboseObjectLoading;
	bool ColorAnimation;
	int32_t SmokeLevel;
	int32_t UpperBoard;
	bool ShowClock;
	int32_t ResX, ResY;
	int32_t Scale;
	bool ShowCrewNames; // show player name above clonks?
	bool ShowCrewCNames; // show clonk names above clonks?
	bool NoAlphaAdd; // always modulate alpha values instead of assing them (->no custom modulated alpha)
	bool PointFiltering; // don't use linear filtering, because some crappy graphic cards can't handle it...
	bool NoBoxFades; // map all DrawBoxFade-calls to DrawBoxDw
	uint32_t AllowedBlitModes; // bit mask for allowed blitting modes
	bool NoAcceleration; // whether direct rendering is used (X11)
	bool Shader; // whether to use pixelshaders
	bool MsgBoard;
	bool PXSGfx; // show PXS-graphics (instead of sole pixels)
	int32_t Engine; // 0: OpenGL; 3: disabled graphics
	std::int32_t BlitOffset;   // blit offset (percent) (OpenGL)
	std::int32_t TexIndent; // blit offset (per mille) (OpenGL)
	int32_t Gamma1, Gamma2, Gamma3; // gamma ramps
	uint32_t RenderInactive; // draw viewports even if inactive. 1 = Fullscreen, 2 = EM, 1 | 2 = both
	bool DisableGamma;
	int32_t Monitor; // monitor index to play on
	bool FireParticles; // draw extended fire particles if enabled (defualt on)
	int32_t MaxRefreshDelay; // minimum time after which graphics should be refreshed (ms)
	bool AutoFrameSkip; // if true, gfx frames are skipped when they would slow down the game
	int32_t CacheTexturesInRAM; // -1 for disabled; otherwise after CacheTexturesInRAM times of Locking, Unlock(true) keeps the texture in RAM
	DisplayMode UseDisplayMode;
#ifdef _WIN32
	bool Maximized;
	int PositionX;
	int PositionY;
#endif
	bool ShowFolderMaps; // if true, folder maps are shown

	void CompileFunc(StdCompiler *pComp);
};

class C4ConfigSound
{
public:
	bool RXSound;
	bool RXMusic;
	bool FEMusic;
	bool FESamples;
	int32_t MusicVolume;
	int32_t SoundVolume;
	int32_t MaxChannels;
	bool PreferLinearResampling;
	bool MuteSoundCommand; // whether to mute /sound by default
	void CompileFunc(StdCompiler *pComp);
};

class C4ConfigNetwork
{
public:
	int32_t ControlRate;
	bool NoRuntimeJoin;
	int32_t MaxResSearchRecursion;
	char WorkPath[CFG_MaxString + 1];
	ValidatedStdStrBuf<C4InVal::VAL_Comment> Comment;
	bool MasterServerSignUp;
	int32_t MasterReferencePeriod;
	bool LeagueServerSignUp;
	bool UseAlternateServer;
	int32_t PortTCP, PortUDP, PortDiscovery, PortRefServer;
	int32_t ControlMode;
	ValidatedStdStrBuf<C4InVal::VAL_NameNoEmpty> LocalName;
	ValidatedStdStrBuf<C4InVal::VAL_NameAllowEmpty> Nick;
	int32_t MaxLoadFileSize;
	char LastPassword[CFG_MaxString + 1];
	char ServerAddress[CFG_MaxString + 1];
	char AlternateServerAddress[CFG_MaxString + 1];
	char UpdateServerAddress[CFG_MaxString + 1];
	char PuncherAddress[CFG_MaxString + 1];

	StdStrBuf LeagueAccount;
	StdStrBuf LeaguePassword;
	bool LeagueAutoLogin;

	bool AutomaticUpdate;
	uint64_t LastUpdateTime;
	int32_t AsyncMaxWait;

public:
	void CompileFunc(StdCompiler *pComp);
	const char *GetLeagueServerAddress();
};

class C4ConfigStartup
{
public:
	// config for do-not-show-this-msg-again-messages
	bool HideMsgStartDedicated;
	bool HideMsgPlrTakeOver;
	bool HideMsgPlrNoTakeOver;
	bool HideMsgNoOfficialLeague;
	bool HideMsgIRCDangerous;
	bool AlphabeticalSorting; // if set, Folder.txt-sorting is ignored in scenario selection
	int32_t LastPortraitFolderIdx;
	void CompileFunc(StdCompiler *pComp);
};

class C4ConfigLobby
{
public:
	int32_t CountdownTime;
	bool AllowPlayerSave; // whether save-to-disk function is enabled for player ressources
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
};

const int C4ConfigMaxGamepads = 4;

class C4ConfigGamepad
{
public:
	int32_t ButtonCommand[C4MaxGamePadButtons];
	int32_t ButtonMenuCommand[C4MaxGamePadButtons];
	uint32_t AxisMinCommand[C4MaxGamePadAxis], AxisMaxCommand[C4MaxGamePadAxis];
	void CompileFunc(StdCompiler *pComp, bool fButtonsOnly = false);
	void Reset(); // reset all buttons and axis calibration to default
	bool HasExplicitJumpButton();
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

class C4ConfigCooldowns
{
public:
	C4CooldownSeconds SoundCommand;
	C4CooldownSeconds ReadyCheck;
	void CompileFunc(StdCompiler *comp);
};

class C4ConfigToasts
{
public:
	bool ReadyCheck;
	void CompileFunc(StdCompiler *comp);
};

#endif

class C4Config
{
public:
	C4Config();
	~C4Config();

public:
	C4ConfigGeneral   General;
#ifdef C4ENGINE
	C4ConfigDeveloper Developer;
	C4ConfigGraphics  Graphics;
	C4ConfigSound     Sound;
	C4ConfigNetwork   Network;
	C4ConfigLobby     Lobby;
	C4ConfigIRC       IRC;
	C4ConfigGamepad   Gamepads[C4ConfigMaxGamepads];
	C4ConfigControls  Controls;
	C4ConfigStartup   Startup;
	C4ConfigCooldowns Cooldowns;
	C4ConfigToasts    Toasts;
#endif
	bool fConfigLoaded; // true if config has been successfully loaded
	StdStrBuf ConfigFilename; // set for configs loaded from a nondefault config file

public:
	const char *GetSubkeyPath(const char *strSubkey);
	void Default();
	bool Save();
	bool Load(bool forceWorkingDirectory = true, const char *szConfigFile = nullptr);
	bool Init();
	const char *AtExePath(const char *szFilename);
	const char *AtTempPath(const char *szFilename);
#ifdef C4ENGINE
	const char *AtNetworkPath(const char *szFilename);
#endif
	const char *AtExeRelativePath(const char *szFilename);
	const char *AtScreenshotPath(const char *szFilename);
	const char *AtUserPath(const char *szFilename); // this one will expand environment variables on-the-fly
	void ForceRelativePath(StdStrBuf *sFilename); // try AtExeRelativePath; force GetC4Filename if not possible
	void CompileFunc(StdCompiler *pComp);
	bool IsCorrupted()
	{
		return (General.ConfigResetSafety != C4ConfigGeneral::ConfigResetSafetyVal)
#ifdef C4ENGINE
		|| !Graphics.ResX
#endif
		;
	}

protected:
	void ExpandEnvironmentVariables(char *strPath, int iMaxLen);

#ifdef C4ENGINE
private:
	void AdaptToCurrentVersion();
#endif
};

extern C4Config Config;
