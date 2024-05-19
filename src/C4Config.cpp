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

#include <C4Config.h>

#include <C4UpperBoard.h>
#include <C4Log.h>
#include "C4Version.h"
#ifdef C4ENGINE
#include <C4Application.h>
#include <C4Network2.h>
#include <C4Language.h>
#include <StdResStr2.h>
#endif

#include <StdFile.h>

#ifdef _WIN32
#include "StdRegistry.h"
#elif defined(__linux__)
#include <clocale>
#endif

bool isGermanSystem()
{
#ifdef _WIN32
	if (PRIMARYLANGID(GetUserDefaultLangID()) == LANG_GERMAN) return true;
#elif defined(__APPLE__) and defined(C4ENGINE)
	extern bool isGerman();
	if (isGerman()) return true;
#elif defined(__linux__)
	if (strstr(std::setlocale(LC_MESSAGES, nullptr), "de")) return true;
#endif
	return false;
}

void C4ConfigGeneral::CompileFunc(StdCompiler *pComp)
{
	// For those without the ability to intuitively guess what the falses and trues mean:
	// its mkNamingAdapt(field, name, default, fPrefillDefault, fStoreDefault)
	// where fStoreDefault writes out the value to the config even if it's the same as the default.
#define s mkStringAdaptM
	// Version got introduced in 348, so any config without it is assumed to be created by 347
	pComp->Value(mkNamingAdapt(Version,            "Version",         347));
	pComp->Value(mkNamingAdapt(s(Name),            "Name",            ""));
	pComp->Value(mkNamingAdapt(s(Language),        "Language",        "", false, true));
	pComp->Value(mkNamingAdapt(s(LanguageEx),      "LanguageEx",      "", false, true));
	pComp->Value(mkNamingAdapt(s(LanguageCharset), "LanguageCharset", "", false, true));
	fUTF8 = SEqual(LanguageCharset, "UTF-8");
	pComp->Value(mkNamingAdapt(s(Definitions),     "Definitions",    ""));
	pComp->Value(mkNamingAdapt(s(Participants),    "Participants",   ""));
	pComp->Value(mkNamingAdapt(s(LogPath),         "LogPath",        "",  false, true));
	pComp->Value(mkNamingAdapt(s(PlayerPath),      "PlayerPath",     "",  false, true));
	pComp->Value(mkNamingAdapt(s(DefinitionPath),  "DefinitionPath", "",  false, true));
#ifdef _WIN32
	pComp->Value(mkNamingAdapt(s(UserPath), "UserPath", "%APPDATA%\\LegacyClonk",                        false, true));
#elif defined(__linux__)
	pComp->Value(mkNamingAdapt(s(UserPath), "UserPath", "$HOME/.legacyclonk",                            false, true));
#elif defined(__APPLE__)
	pComp->Value(mkNamingAdapt(s(UserPath), "UserPath", "$HOME/Library/Application Support/LegacyClonk", false, true));
#endif
	pComp->Value(mkNamingAdapt(SaveGameFolder, "SaveGameFolder", "Savegames.c4f", false, true));
	pComp->Value(mkNamingAdapt(SaveDemoFolder, "SaveDemoFolder", "Records.c4f",   false, true));
#ifdef C4ENGINE
	pComp->Value(mkNamingAdapt(s(MissionAccess), "MissionAccess", "", false, true));
#endif
	pComp->Value(mkNamingAdapt(FPS,                     "FPS",                     false,         false, true));
	pComp->Value(mkNamingAdapt(Record,                  "Record",                  false,         false, true));
	pComp->Value(mkNamingAdapt(ScreenshotFolder,        "ScreenshotFolder",        "Screenshots", false, true));
	pComp->Value(mkNamingAdapt(FairCrew,                "NoCrew",                  false,         false, true));
	pComp->Value(mkNamingAdapt(FairCrewStrength,        "DefCrewStrength",         1000,          false, true));
	pComp->Value(mkNamingAdapt(ScrollSmooth,            "ScrollSmooth",            4));
	pComp->Value(mkNamingAdapt(AlwaysDebug,             "DebugMode",               false,         false, true));
	pComp->Value(mkNamingAdapt(AllowScriptingInReplays, "AllowScriptingInReplays", false));

	pComp->Value(mkNamingAdapt(s(RXFontName),        "FontName",             "Endeavour", false, true));
	pComp->Value(mkNamingAdapt(RXFontSize,           "FontSize",             14,          false, true));
	pComp->Value(mkNamingAdapt(GamepadEnabled,       "GamepadEnabled",       true));
	pComp->Value(mkNamingAdapt(FirstStart,           "FirstStart",           true));
	pComp->Value(mkNamingAdapt(UserPortraitsWritten, "UserPortraitsWritten", false));
	pComp->Value(mkNamingAdapt(ConfigResetSafety,    "ConfigResetSafety",    static_cast<int32_t>(ConfigResetSafetyVal)));
	pComp->Value(mkNamingAdapt(UseWhiteIngameChat,   "UseWhiteIngameChat",   false, false, true));
	pComp->Value(mkNamingAdapt(UseWhiteLobbyChat,    "UseWhiteLobbyChat",    false, false, true));
	pComp->Value(mkNamingAdapt(ShowLogTimestamps,    "ShowLogTimestamps",    false, false, true));

#ifdef __APPLE__
	pComp->Value(mkNamingAdapt(Preloading,           "Preloading",           false));
#else
	pComp->Value(mkNamingAdapt(Preloading,           "Preloading",           true));
#endif
}

#ifdef C4ENGINE

void C4ConfigDeveloper::ConsoleScriptStrictnessWrapper::CompileFunc(StdCompiler *const comp)
{
	StdEnumEntry<C4AulScriptStrict> ConsoleScriptStrictnessValues[] =
	{
		{"NonStrict", C4AulScriptStrict::NONSTRICT},
		{"Strict1", C4AulScriptStrict::STRICT1},
		{"Strict2", C4AulScriptStrict::STRICT2},
		{"Strict3", C4AulScriptStrict::STRICT3},
		{"MaxStrict", MaxStrictSentinel}
	};

	comp->Value(mkEnumAdaptT<C4AulScriptStrict>(Strictness, ConsoleScriptStrictnessValues));

	if (comp->isCompiler() && Strictness != MaxStrictSentinel)
	{
		Strictness = static_cast<C4AulScriptStrict>(std::clamp(std::to_underlying(Strictness), std::to_underlying(C4AulScriptStrict::NONSTRICT), std::to_underlying(C4AulScriptStrict::MAXSTRICT)));
	}
}

void C4ConfigDeveloper::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(AutoFileReload, "AutoFileReload", true, false, true));
	pComp->Value(mkNamingAdapt(ConsoleScriptStrictness, "ConsoleScriptStrictness", ConsoleScriptStrictnessWrapper{ConsoleScriptStrictnessWrapper::MaxStrictSentinel}));
}

void C4ConfigGraphics::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(ResX,                 "ResolutionX",          800,   false, true));
	pComp->Value(mkNamingAdapt(ResY,                 "ResolutionY",          600,   false, true));
	pComp->Value(mkNamingAdapt(Scale,                "Scale",                100,   false, true));
	pComp->Default("ShowAllResolutions");
	pComp->Value(mkNamingAdapt(SplitscreenDividers,  "SplitscreenDividers",  1));
	pComp->Value(mkNamingAdapt(ShowPlayerHUDAlways,  "ShowPlayerHUDAlways",  true));
	pComp->Value(mkNamingAdapt(ShowPortraits,        "ShowPortraits",        true,  false, true));
	pComp->Value(mkNamingAdapt(AddNewCrewPortraits,  "AddNewCrewPortraits",  true,  false, true));
	pComp->Value(mkNamingAdapt(SaveDefaultPortraits, "SaveDefaultPortraits", true,  false, true));
	pComp->Value(mkNamingAdapt(ShowCommands,         "ShowCommands",         true,  false, true));
	pComp->Value(mkNamingAdapt(ShowCommandKeys,      "ShowCommandKeys",      true,  false, true));
	pComp->Value(mkNamingAdapt(ColorAnimation,       "ColorAnimation",       false, false, true));
	pComp->Value(mkNamingAdapt(SmokeLevel,           "SmokeLevel",           200,   false, true));
	pComp->Value(mkNamingAdapt(VerboseObjectLoading, "VerboseObjectLoading", 0,     false, true));

	StdEnumEntry<int32_t> UpperBoardDisplayModes[] =
	{
		{"Hide", C4UpperBoard::Hide},
		{"Full", C4UpperBoard::Full},
		{"Small", C4UpperBoard::Small},
		{"Mini", C4UpperBoard::Mini}
	};
	pComp->Value(mkNamingAdapt(mkEnumAdaptT<int32_t>(UpperBoard, UpperBoardDisplayModes), "UpperBoard", C4UpperBoard::Full, false, true));

	pComp->Value(mkNamingAdapt(ShowClock,            "ShowClock",            false, false, true));
	pComp->Value(mkNamingAdapt(ShowCrewNames,        "ShowCrewNames",        true,  false, true));
	pComp->Value(mkNamingAdapt(ShowCrewCNames,       "ShowCrewCNames",       true,  false, true));
	pComp->Value(mkNamingAdapt(MsgBoard,             "MsgBoard",             true,  false, true));
	pComp->Value(mkNamingAdapt(PXSGfx,               "PXSGfx",               true));
	pComp->Value(mkNamingAdapt(Engine,               "Engine",               GFXENGN_OPENGL, false, true));
	pComp->Value(mkNamingAdapt(NoAlphaAdd,           "NoAlphaAdd",           false));
	pComp->Value(mkNamingAdapt(PointFiltering,       "PointFiltering",       false));
	pComp->Value(mkNamingAdapt(NoBoxFades,           "NoBoxFades",           false));
	pComp->Value(mkNamingAdapt(NoAcceleration,       "NoAcceleration",       false));
	pComp->Value(mkNamingAdapt(TexIndent,            "TexIndent",            0));
	pComp->Value(mkNamingAdapt(BlitOffset,           "BlitOffset",           0));
	pComp->Value(mkNamingAdapt(AllowedBlitModes,     "AllowedBlitModes",     C4GFXBLIT_ALL));
	pComp->Value(mkNamingAdapt(Gamma1,               "Gamma1",               0));
	pComp->Value(mkNamingAdapt(Gamma2,               "Gamma2",               0x808080));
	pComp->Value(mkNamingAdapt(Gamma3,               "Gamma3",               0xffffff));
	pComp->Default("Currency");
	pComp->Value(mkNamingAdapt(RenderInactive,       "RenderInactive",       Console));
	pComp->Value(mkNamingAdapt(DisableGamma,         "DisableGamma",         false, false, true));
	pComp->Value(mkNamingAdapt(Monitor,              "Monitor",              0)); // 0 = D3DADAPTER_DEFAULT
	pComp->Value(mkNamingAdapt(FireParticles,        "FireParticles",        true,  false, true));
	pComp->Value(mkNamingAdapt(MaxRefreshDelay,      "MaxRefreshDelay",      30));
	pComp->Value(mkNamingAdapt(Shader,               "Shader",               false, false, true));
	pComp->Value(mkNamingAdapt(AutoFrameSkip,        "AutoFrameSkip",        true,  false, true));
	pComp->Value(mkNamingAdapt(CacheTexturesInRAM,   "CacheTexturesInRAM",   100));

	StdEnumEntry<DisplayMode> DisplayModes[] =
	{
		{"Fullscreen", DisplayMode::Fullscreen},
		{"Window", DisplayMode::Window}
	};
	pComp->Value(mkNamingAdapt(mkEnumAdaptT<int>(UseDisplayMode, DisplayModes), "DisplayMode", DisplayMode::Fullscreen, false, true));

#ifdef _WIN32
	pComp->Value(mkNamingAdapt(Maximized,   "Maximized",   false, false, true));
	pComp->Value(mkNamingAdapt(PositionX,   "PositionX",   0,     false, true));
	pComp->Value(mkNamingAdapt(PositionY,   "PositionY",   0,     false, true));
#endif

	pComp->Value(mkNamingAdapt(ShowFolderMaps, "ShowFolderMaps", true));
}

void C4ConfigSound::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(RXSound,     "Sound",       true,  false, true));
	pComp->Value(mkNamingAdapt(RXMusic,     "Music",       true,  false, true));
	pComp->Value(mkNamingAdapt(FEMusic,     "MenuMusic",   true,  false, true));
	pComp->Value(mkNamingAdapt(FESamples,   "MenuSound",   true,  false, true));
	pComp->Default("Verbose");
	pComp->Value(mkNamingAdapt(MusicVolume, "MusicVolume", 100, false, true));
	pComp->Value(mkNamingAdapt(SoundVolume, "SoundVolume", 100, false, true));
	pComp->Value(mkNamingAdapt(MaxChannels, "MaxChannels", C4AudioSystem::MaxChannels));
	pComp->Value(mkNamingAdapt(PreferLinearResampling, "PreferLinearResampling", false));

	if (pComp->isCompiler())
	{
		MaxChannels = std::clamp(MaxChannels, 1, C4AudioSystem::MaxChannels);
	}

	pComp->Value(mkNamingAdapt(MuteSoundCommand, "MuteSoundCommand", false, false, true));
}

void C4ConfigNetwork::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(ControlRate,           "ControlRate",           2,         false, true));
	pComp->Value(mkNamingAdapt(s(WorkPath),           "WorkPath",              "Network", false, true));
	pComp->Value(mkNamingAdapt(NoRuntimeJoin,         "NoRuntimeJoin",         true,      false, true));
	pComp->Value(mkNamingAdapt(MaxResSearchRecursion, "MaxResSearchRecursion", 1,         false, true));
	pComp->Value(mkNamingAdapt(Comment,               "Comment",               "",        false, true));

	pComp->Value(mkNamingAdapt(PortTCP,       "PortTCP",       C4NetStdPortTCP,       false, true));
	pComp->Value(mkNamingAdapt(PortUDP,       "PortUDP",       C4NetStdPortUDP,       false, true));
	pComp->Value(mkNamingAdapt(PortDiscovery, "PortDiscovery", C4NetStdPortDiscovery, false, true));
	pComp->Value(mkNamingAdapt(PortRefServer, "PortRefServer", C4NetStdPortRefServer, false, true));

	pComp->Value(mkNamingAdapt(ControlMode,        "ControlMode",        0,              false, true));
	pComp->Value(mkNamingAdapt(LocalName,          "LocalName",          "Unknown",      false, true));
	pComp->Value(mkNamingAdapt(Nick,               "Nick",               "",             false, true));
	pComp->Value(mkNamingAdapt(MaxLoadFileSize,    "MaxLoadFileSize", 100 * 1024 * 1024, false, true));

	pComp->Value(mkNamingAdapt(MasterServerSignUp,        "MasterServerSignUp",     true,   false, true));
	pComp->Value(mkNamingAdapt(MasterReferencePeriod,     "MasterReferencePeriod",  120,    false, true));
	pComp->Value(mkNamingAdapt(LeagueServerSignUp,        "LeagueServerSignUp",     false,  false, true));
	pComp->Value(mkNamingAdapt(s(ServerAddress),          "ServerAddress",          C4CFG_LeagueServer, false, true));
	pComp->Value(mkNamingAdapt(UseAlternateServer,        "UseAlternateServer",     false,  false, true));
	pComp->Value(mkNamingAdapt(s(AlternateServerAddress), "AlternateServerAddress", C4CFG_LeagueServer, false, true));
	pComp->Value(mkNamingAdapt(s(UpdateServerAddress),    "UpdateServerAddress",    C4CFG_UpdateServer));
	pComp->Value(mkNamingAdapt(s(LastPassword),           "LastPassword",           "Wipf", false, true));
	pComp->Value(mkNamingAdapt(AutomaticUpdate,           "EnableAutomaticUpdate",  true));
	pComp->Value(mkNamingAdapt(LastUpdateTime,            "LastUpdateTime",         0,    false, true));
	pComp->Value(mkNamingAdapt(AsyncMaxWait,              "AsyncMaxWait",           2,    false, true));

	constexpr auto defaultPuncherServer = "netpuncher.openclonk.org:11115";
	pComp->Value(mkNamingAdapt(s(PuncherAddress), "PuncherAddress", defaultPuncherServer, false, true));

	if (pComp->isCompiler() && SEqual(PuncherAddress, "clonk.de:11115")) strncpy(PuncherAddress, defaultPuncherServer, CFG_MaxString);

	pComp->Value(mkNamingAdapt(LeagueAccount,     "LeagueNick",      "",               false, false));
	pComp->Value(mkNamingAdapt(LeagueAutoLogin,   "LeagueAutoLogin", true,             false, false));
}

void C4ConfigLobby::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(AllowPlayerSave, "AllowPlayerSave", false, false, true));
	pComp->Value(mkNamingAdapt(CountdownTime,   "CountdownTime",   5,     false, true));
}

void C4ConfigIRC::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(s(Server),        "Server2",          "irc.euirc.net", false, true));
	pComp->Value(mkNamingAdapt(s(Nick),          "Nick",             "",              false, true));
	pComp->Value(mkNamingAdapt(s(RealName),      "RealName",         "",              false, true));
	pComp->Value(mkNamingAdapt(s(Channel),       "Channel",          "#clonken,#legacyclonk", false, true));
}

void C4ConfigGamepad::CompileFunc(StdCompiler *pComp, bool fButtonsOnly)
{
	// Left stick
	pComp->Value(mkNamingAdapt(AxisMinCommand[XINPUT_AXIS_LS_X], "Axis0MinCommand", COM_Left));
	pComp->Value(mkNamingAdapt(AxisMaxCommand[XINPUT_AXIS_LS_X], "Axis0MaxCommand", COM_Right));
	pComp->Value(mkNamingAdapt(AxisMinCommand[XINPUT_AXIS_LS_Y], "Axis1MinCommand", COM_Down));
	pComp->Value(mkNamingAdapt(AxisMaxCommand[XINPUT_AXIS_LS_Y], "Axis1MaxCommand", COM_Up));
	// Right stick
	pComp->Value(mkNamingAdapt(AxisMinCommand[XINPUT_AXIS_RS_X], "Axis2MinCommand", COM_Left));
	pComp->Value(mkNamingAdapt(AxisMaxCommand[XINPUT_AXIS_RS_X], "Axis2MaxCommand", COM_Right));
	pComp->Value(mkNamingAdapt(AxisMinCommand[XINPUT_AXIS_RS_Y], "Axis3MinCommand", COM_Up));
	pComp->Value(mkNamingAdapt(AxisMaxCommand[XINPUT_AXIS_RS_Y], "Axis3MaxCommand", COM_Down));
	// Triggers
	pComp->Value(mkNamingAdapt(AxisMinCommand[XINPUT_AXIS_LT], "Axis4MinCommand", COM_None));
	pComp->Value(mkNamingAdapt(AxisMaxCommand[XINPUT_AXIS_LT], "Axis4MaxCommand", COM_CursorLeft));
	pComp->Value(mkNamingAdapt(AxisMinCommand[XINPUT_AXIS_RT], "Axis5MinCommand", COM_None));
	pComp->Value(mkNamingAdapt(AxisMaxCommand[XINPUT_AXIS_RT], "Axis5MaxCommand", COM_CursorRight));

	// Primary function for each button, used for normal control
	pComp->Value(mkNamingAdapt(ButtonCommand[XINPUT_BUTTON_UP], "ButtonCommand0", COM_Up));
	pComp->Value(mkNamingAdapt(ButtonCommand[XINPUT_BUTTON_DOWN], "ButtonCommand1", COM_Down));
	pComp->Value(mkNamingAdapt(ButtonCommand[XINPUT_BUTTON_LEFT], "ButtonCommand2", COM_Left));
	pComp->Value(mkNamingAdapt(ButtonCommand[XINPUT_BUTTON_RIGHT], "ButtonCommand3", COM_Right));
	pComp->Value(mkNamingAdapt(ButtonCommand[XINPUT_BUTTON_START], "ButtonCommand4", COM_PlayerMenu));
	pComp->Value(mkNamingAdapt(ButtonCommand[XINPUT_BUTTON_BACK], "ButtonCommand5", COM_Special));
	pComp->Value(mkNamingAdapt(ButtonCommand[XINPUT_BUTTON_LS], "ButtonCommand6", COM_None));
	pComp->Value(mkNamingAdapt(ButtonCommand[XINPUT_BUTTON_RS], "ButtonCommand7", COM_None));
	pComp->Value(mkNamingAdapt(ButtonCommand[XINPUT_BUTTON_LB], "ButtonCommand8", COM_CursorToggle));
	pComp->Value(mkNamingAdapt(ButtonCommand[XINPUT_BUTTON_RB], "ButtonCommand9", COM_CursorToggle));
	pComp->Value(mkNamingAdapt(ButtonCommand[10], "ButtonCommand10", COM_None));
	pComp->Value(mkNamingAdapt(ButtonCommand[11], "ButtonCommand11", COM_None));
	pComp->Value(mkNamingAdapt(ButtonCommand[XINPUT_BUTTON_A], "ButtonCommand12", COM_Jump));
	pComp->Value(mkNamingAdapt(ButtonCommand[XINPUT_BUTTON_B], "ButtonCommand13", COM_Dig));
	pComp->Value(mkNamingAdapt(ButtonCommand[XINPUT_BUTTON_X], "ButtonCommand14", COM_Throw));
	pComp->Value(mkNamingAdapt(ButtonCommand[XINPUT_BUTTON_Y], "ButtonCommand15", COM_Special2));

	// Possible secondary function for each button when in a menu
	pComp->Value(mkNamingAdapt(ButtonMenuCommand[XINPUT_BUTTON_UP], "ButtonMenuCommand0", COM_MenuUp));
	pComp->Value(mkNamingAdapt(ButtonMenuCommand[XINPUT_BUTTON_DOWN], "ButtonMenuCommand1", COM_MenuDown));
	pComp->Value(mkNamingAdapt(ButtonMenuCommand[XINPUT_BUTTON_LEFT], "ButtonMenuCommand2", COM_MenuLeft));
	pComp->Value(mkNamingAdapt(ButtonMenuCommand[XINPUT_BUTTON_RIGHT], "ButtonMenuCommand3", COM_MenuRight));
	pComp->Value(mkNamingAdapt(ButtonMenuCommand[XINPUT_BUTTON_START], "ButtonMenuCommand4", COM_MenuClose));
	pComp->Value(mkNamingAdapt(ButtonMenuCommand[XINPUT_BUTTON_BACK], "ButtonMenuCommand5", COM_MenuClose));
	pComp->Value(mkNamingAdapt(ButtonMenuCommand[XINPUT_BUTTON_LS], "ButtonMenuCommand6", COM_None));
	pComp->Value(mkNamingAdapt(ButtonMenuCommand[XINPUT_BUTTON_RS], "ButtonMenuCommand7", COM_None));
	pComp->Value(mkNamingAdapt(ButtonMenuCommand[XINPUT_BUTTON_LB], "ButtonMenuCommand8", COM_CursorToggle));
	pComp->Value(mkNamingAdapt(ButtonMenuCommand[XINPUT_BUTTON_RB], "ButtonMenuCommand9", COM_CursorToggle));
	pComp->Value(mkNamingAdapt(ButtonMenuCommand[10], "ButtonMenuCommand10", COM_None));
	pComp->Value(mkNamingAdapt(ButtonMenuCommand[11], "ButtonMenuCommand11", COM_None));
	pComp->Value(mkNamingAdapt(ButtonMenuCommand[XINPUT_BUTTON_A], "ButtonMenuCommand12", COM_MenuEnter));
	pComp->Value(mkNamingAdapt(ButtonMenuCommand[XINPUT_BUTTON_B], "ButtonMenuCommand13", COM_MenuClose));
	pComp->Value(mkNamingAdapt(ButtonMenuCommand[XINPUT_BUTTON_X], "ButtonMenuCommand14", COM_Throw));
	pComp->Value(mkNamingAdapt(ButtonMenuCommand[XINPUT_BUTTON_Y], "ButtonMenuCommand15", COM_MenuEnterAll));
}

bool C4ConfigGamepad::HasExplicitJumpButton() {
	for (auto& command : this->AxisMaxCommand) if (command == COM_Jump) return true;
	for (auto& command : this->AxisMinCommand) if (command == COM_Jump) return true;
	for (auto& command : this->ButtonCommand) if (command == COM_Jump) return true;
	return false;
}

void C4ConfigGamepad::Reset()
{
	// loads an empty config for the gamepad config
	StdCompilerNull Comp; Comp.Compile(mkParAdapt(*this, false));
}

void C4ConfigControls::CompileFunc(StdCompiler *pComp, bool fKeysOnly)
{
#ifndef USE_CONSOLE
#ifdef _WIN32
#define KEY(win, x, sdl) win
#elif defined(USE_X11)
#define KEY(win, x, sdl) x
#else
#define KEY(win, x, sdl) sdl
#endif

	bool fGer = isGermanSystem();

	pComp->Value(mkNamingAdapt(Keyboard[0][ 0], "Kbd1Key1",  KEY('Q', XK_q, SDL_SCANCODE_Q)));
	pComp->Value(mkNamingAdapt(Keyboard[0][ 1], "Kbd1Key2",  KEY('W', XK_w, SDL_SCANCODE_W)));
	pComp->Value(mkNamingAdapt(Keyboard[0][ 2], "Kbd1Key3",  KEY('E', XK_e, SDL_SCANCODE_E)));
	pComp->Value(mkNamingAdapt(Keyboard[0][ 3], "Kbd1Key4",  KEY('A', XK_a, SDL_SCANCODE_A)));
	pComp->Value(mkNamingAdapt(Keyboard[0][ 4], "Kbd1Key5",  KEY('S', XK_s, SDL_SCANCODE_S)));
	pComp->Value(mkNamingAdapt(Keyboard[0][ 5], "Kbd1Key6",  KEY('D', XK_d, SDL_SCANCODE_D)));
	pComp->Value(mkNamingAdapt(Keyboard[0][ 6], "Kbd1Key7",  fGer ? KEY('Y', XK_y,    SDL_SCANCODE_Z)    : KEY('Z', XK_z, SDL_SCANCODE_Z)));
	pComp->Value(mkNamingAdapt(Keyboard[0][ 7], "Kbd1Key8",  KEY('X', XK_x, SDL_SCANCODE_X)));
	pComp->Value(mkNamingAdapt(Keyboard[0][ 8], "Kbd1Key9",  KEY('C', XK_c, SDL_SCANCODE_C)));
	pComp->Value(mkNamingAdapt(Keyboard[0][ 9], "Kbd1Key10", fGer ? KEY(226, XK_less, SDL_SCANCODE_NONUSBACKSLASH) : KEY('R', XK_r, SDL_SCANCODE_R)));
	pComp->Value(mkNamingAdapt(Keyboard[0][10], "Kbd1Key11", KEY('V', XK_v, SDL_SCANCODE_V)));
	pComp->Value(mkNamingAdapt(Keyboard[0][11], "Kbd1Key12", KEY('F', XK_f, SDL_SCANCODE_F)));

	pComp->Value(mkNamingAdapt(Keyboard[1][ 0], "Kbd2Key1",  KEY(103, XK_KP_Home,      SDL_SCANCODE_KP_7)));
	pComp->Value(mkNamingAdapt(Keyboard[1][ 1], "Kbd2Key2",  KEY(104, XK_KP_Up,        SDL_SCANCODE_KP_8)));
	pComp->Value(mkNamingAdapt(Keyboard[1][ 2], "Kbd2Key3",  KEY(105, XK_KP_Page_Up,   SDL_SCANCODE_KP_9)));
	pComp->Value(mkNamingAdapt(Keyboard[1][ 3], "Kbd2Key4",  KEY(100, XK_KP_Left,      SDL_SCANCODE_KP_4)));
	pComp->Value(mkNamingAdapt(Keyboard[1][ 4], "Kbd2Key5",  KEY(101, XK_KP_Begin,     SDL_SCANCODE_KP_5)));
	pComp->Value(mkNamingAdapt(Keyboard[1][ 5], "Kbd2Key6",  KEY(102, XK_KP_Right,     SDL_SCANCODE_KP_6)));
	pComp->Value(mkNamingAdapt(Keyboard[1][ 6], "Kbd2Key7",  KEY( 97, XK_KP_End,       SDL_SCANCODE_KP_1)));
	pComp->Value(mkNamingAdapt(Keyboard[1][ 7], "Kbd2Key8",  KEY( 98, XK_KP_Down,      SDL_SCANCODE_KP_2)));
	pComp->Value(mkNamingAdapt(Keyboard[1][ 8], "Kbd2Key9",  KEY( 99, XK_KP_Page_Down, SDL_SCANCODE_KP_3)));
	pComp->Value(mkNamingAdapt(Keyboard[1][ 9], "Kbd2Key10", KEY( 96, XK_KP_Insert,    SDL_SCANCODE_KP_0)));
	pComp->Value(mkNamingAdapt(Keyboard[1][10], "Kbd2Key11", KEY(110, XK_KP_Delete,    SDL_SCANCODE_KP_PERIOD)));
	pComp->Value(mkNamingAdapt(Keyboard[1][11], "Kbd2Key12", KEY(107, XK_KP_Add,       SDL_SCANCODE_KP_PLUS)));

	pComp->Value(mkNamingAdapt(Keyboard[2][ 0], "Kbd3Key1",  KEY('I', XK_i,          SDL_SCANCODE_I)));
	pComp->Value(mkNamingAdapt(Keyboard[2][ 1], "Kbd3Key2",  KEY('O', XK_o,          SDL_SCANCODE_O)));
	pComp->Value(mkNamingAdapt(Keyboard[2][ 2], "Kbd3Key3",  KEY('P', XK_p,          SDL_SCANCODE_P)));
	pComp->Value(mkNamingAdapt(Keyboard[2][ 3], "Kbd3Key4",  KEY('K', XK_k,          SDL_SCANCODE_K)));
	pComp->Value(mkNamingAdapt(Keyboard[2][ 4], "Kbd3Key5",  KEY('L', XK_l,          SDL_SCANCODE_L)));
	pComp->Value(mkNamingAdapt(Keyboard[2][ 5], "Kbd3Key6",  fGer ? KEY(192, XK_odiaeresis, SDL_SCANCODE_SEMICOLON) : KEY(0xBA, XK_semicolon, SDL_SCANCODE_SEMICOLON)));
	pComp->Value(mkNamingAdapt(Keyboard[2][ 6], "Kbd3Key7",  KEY(188, XK_comma,      SDL_SCANCODE_COMMA)));
	pComp->Value(mkNamingAdapt(Keyboard[2][ 7], "Kbd3Key8",  KEY(190, XK_period,     SDL_SCANCODE_PERIOD)));
	pComp->Value(mkNamingAdapt(Keyboard[2][ 8], "Kbd3Key9",  fGer ? KEY(189, XK_minus,      SDL_SCANCODE_SLASH)   : KEY(0xBF, XK_slash,     SDL_SCANCODE_SLASH)));
	pComp->Value(mkNamingAdapt(Keyboard[2][ 9], "Kbd3Key10", KEY('M', XK_m,          SDL_SCANCODE_M)));
	pComp->Value(mkNamingAdapt(Keyboard[2][10], "Kbd3Key11", KEY(222, XK_adiaeresis, SDL_SCANCODE_APOSTROPHE)));
	pComp->Value(mkNamingAdapt(Keyboard[2][11], "Kbd3Key12", KEY(186, XK_udiaeresis, SDL_SCANCODE_LEFTBRACKET)));

	pComp->Value(mkNamingAdapt(Keyboard[3][ 0], "Kbd4Key1",  KEY(VK_INSERT, XK_Insert,    SDL_SCANCODE_INSERT)));
	pComp->Value(mkNamingAdapt(Keyboard[3][ 1], "Kbd4Key2",  KEY(VK_HOME,   XK_Home,      SDL_SCANCODE_HOME)));
	pComp->Value(mkNamingAdapt(Keyboard[3][ 2], "Kbd4Key3",  KEY(VK_PRIOR,  XK_Page_Up,   SDL_SCANCODE_PAGEUP)));
	pComp->Value(mkNamingAdapt(Keyboard[3][ 3], "Kbd4Key4",  KEY(VK_DELETE, XK_Delete,    SDL_SCANCODE_DELETE)));
	pComp->Value(mkNamingAdapt(Keyboard[3][ 4], "Kbd4Key5",  KEY(VK_UP,     XK_Up,        SDL_SCANCODE_UP)));
	pComp->Value(mkNamingAdapt(Keyboard[3][ 5], "Kbd4Key6",  KEY(VK_NEXT,   XK_Page_Down, SDL_SCANCODE_PAGEDOWN)));
	pComp->Value(mkNamingAdapt(Keyboard[3][ 6], "Kbd4Key7",  KEY(VK_LEFT,   XK_Left,      SDL_SCANCODE_LEFT)));
	pComp->Value(mkNamingAdapt(Keyboard[3][ 7], "Kbd4Key8",  KEY(VK_DOWN,   XK_Down,      SDL_SCANCODE_DOWN)));
	pComp->Value(mkNamingAdapt(Keyboard[3][ 8], "Kbd4Key9",  KEY(VK_RIGHT,  XK_Right,     SDL_SCANCODE_RIGHT)));
	pComp->Value(mkNamingAdapt(Keyboard[3][ 9], "Kbd4Key10", KEY(VK_END,    XK_End,       SDL_SCANCODE_END)));
	pComp->Value(mkNamingAdapt(Keyboard[3][10], "Kbd4Key11", KEY(VK_RETURN, XK_Return,    SDL_SCANCODE_RETURN)));
	pComp->Value(mkNamingAdapt(Keyboard[3][11], "Kbd4Key12", KEY(VK_BACK,   XK_BackSpace, SDL_SCANCODE_BACKSPACE)));

	if (fKeysOnly) return;

	pComp->Value(mkNamingAdapt(MouseAScroll,      "MouseAutoScroll",   0));
	pComp->Value(mkNamingAdapt(GamepadGuiControl, "GamepadGuiControl", 0, false, true));

#undef KEY
#undef s
#endif // USE_CONSOLE
}

void C4ConfigCooldowns::CompileFunc(StdCompiler *comp)
{
	using namespace std::chrono_literals;

	comp->Value(mkNamingAdapt(SoundCommand, "SoundCommand", 0s));
	comp->Value(mkNamingAdapt(mkParAdapt(ReadyCheck, 5s), "ReadyCheck", 10s));
}

void C4ConfigToasts::CompileFunc(StdCompiler *comp)
{
	comp->Value(mkNamingAdapt(ReadyCheck, "ReadyCheck", true));
}
#endif

C4Config::C4Config()
{
	Default();
}

C4Config::~C4Config()
{
	fConfigLoaded = false;
}

void C4Config::Default()
{
	// force default values
	StdCompilerNull Comp; Comp.Compile(*this);
	fConfigLoaded = false;
}

bool C4Config::Load(bool forceWorkingDirectory, const char *szConfigFile)
{
	try
	{
#ifdef _WIN32
		// Windows: Default load from registry, if no explicit config file is specified
		if (!szConfigFile)
		{
			StdCompilerConfigRead CfgRead(HKEY_CURRENT_USER, "Software\\" C4CFG_Company "\\" C4CFG_Product);
			CfgRead.Compile(*this);
		}
		else
#endif
		{
			// Nonwindows or explicit config file: Determine filename to load config from
			StdStrBuf filename;
			if (szConfigFile)
			{
				// Config filename is specified
				filename.Ref(szConfigFile);
				// make sure we're at the correct path to load it
				if (forceWorkingDirectory) General.DeterminePaths(true);
			}
			else
			{
				// Config filename from home
				StdStrBuf home(getenv("HOME"), false);
				if (home) { home += "/"; }
				filename.Copy(home);
#ifdef __APPLE__
				filename += "Library/Preferences/legacyclonk.config";
#else
				filename += ".legacyclonk/config";
#endif
			}

			// Load config file into buf
			StdStrBuf buf;
			buf.LoadFromFile(filename.getData());

			if (buf.isNull())
			{
				// Config file not present?
#ifdef __linux__
				if (!szConfigFile)
				{
					StdStrBuf filename(getenv("HOME"), false);
					if (filename) { filename += "/"; }
					filename += ".legacyclonk";
					CreateDirectory(filename.getData());
				}
#endif
				// Buggy StdCompiler crashes when compiling a Null-StdStrBuf
				buf.Ref(" ");
			}

			// Read config from buffer
			StdCompilerINIRead IniRead;
			IniRead.setInput(buf);
			IniRead.Compile(*this);
		}
	}
	catch ([[maybe_unused]] const StdCompiler::Exception &e)
	{
		// Configuration file syntax error?
#ifdef C4ENGINE
		LogF("Error loading configuration: %s"/*restbl not yet loaded*/, e.what());
#endif
		return false;
	}

	// Config postinit
	General.DeterminePaths(forceWorkingDirectory);
#ifdef C4ENGINE
	AdaptToCurrentVersion();
#ifdef _WIN32
	bool fWinSock = AcquireWinSock();
#endif
	if (SEqual(Network.LocalName.getData(), "Unknown"))
	{
		char LocalName[25 + 1]; *LocalName = 0;
		gethostname(LocalName, 25);
		if (*LocalName) Network.LocalName.Copy(LocalName);
	}
#ifdef _WIN32
	if (fWinSock) ReleaseWinSock();
#endif
#endif
	General.DefaultLanguage();
#ifndef USE_CONSOLE
	if (Graphics.Engine != GFXENGN_NOGFX) Graphics.Engine = GFXENGN_OPENGL;
#endif
	// Warning against invalid ports
#ifdef C4ENGINE
	for (const auto &port :
		{
			&Config.Network.PortTCP,
			&Config.Network.PortUDP,
			&Config.Network.PortDiscovery,
			&Config.Network.PortRefServer
		}
	)
	{
		if (*port < 0 || *port > 65535) *port = 0;
	}
	if (Config.Network.PortTCP > 0 && Config.Network.PortTCP == Config.Network.PortRefServer)
	{
		Log("Warning: Network TCP port and reference server port both set to same value - increasing reference server port!");
		++Config.Network.PortRefServer;
		if (Config.Network.PortRefServer >= 65536) Config.Network.PortRefServer = C4NetStdPortRefServer;
	}
	if (Config.Network.PortUDP > 0 && Config.Network.PortUDP == Config.Network.PortDiscovery)
	{
		Log("Warning: Network UDP port and LAN game discovery port both set to same value - increasing discovery port!");
		++Config.Network.PortDiscovery;
		if (Config.Network.PortDiscovery >= 65536) Config.Network.PortDiscovery = C4NetStdPortDiscovery;
	}
#endif
	fConfigLoaded = true;
	if (szConfigFile) ConfigFilename.Copy(szConfigFile); else ConfigFilename.Clear();
	return true;
}

bool C4Config::Save()
{
	try
	{
#ifdef _WIN32
		if (!ConfigFilename.getLength())
		{
			// Windows: Default save to registry, if it wasn't loaded from file
			StdCompilerConfigWrite CfgWrite(HKEY_CURRENT_USER, "Software\\" C4CFG_Company "\\" C4CFG_Product);
			CfgWrite.Decompile(*this);
		}
		else
#endif
		{
			StdStrBuf filename;
			if (ConfigFilename.getLength())
			{
				filename.Ref(ConfigFilename);
			}
			else
			{
				filename.Copy(getenv("HOME"));
				if (filename) { filename += "/"; }
#ifdef __APPLE__
				filename += "Library/Preferences/legacyclonk.config";
#else
				filename += ".legacyclonk/config";
#endif
			}
			StdCompilerINIWrite IniWrite;
			IniWrite.Decompile(*this);
			IniWrite.getOutput().SaveToFile(filename.getData());
		}
	}
	catch ([[maybe_unused]] const StdCompiler::Exception &e)
	{
#ifdef C4ENGINE
		LogF(LoadResStr("IDS_ERR_CONFSAVE"), e.what());
#endif
		return false;
	}
	return true;
}

void C4ConfigGeneral::DeterminePaths(bool forceWorkingDirectory)
{
#ifdef _WIN32
	// Exe path
	if (GetModuleFileName(nullptr, ExePath, CFG_MaxString))
	{
		TruncatePath(ExePath); AppendBackslash(ExePath);
	}
	// Temp path
	GetTempPath(CFG_MaxString, TempPath);
	if (TempPath[0]) AppendBackslash(TempPath);
#elif defined(__linux__)
#ifdef C4ENGINE
	GetParentPath(Application.Location, ExePath);
#else
	ExePath[0] = '.'; ExePath[1] = 0;
#endif
	AppendBackslash(ExePath);
	const char *t = getenv("TMPDIR");
	if (t)
	{
		SCopy(t, TempPath, sizeof(TempPath) - 2);
		AppendBackslash(TempPath);
	}
	else
		SCopy("/tmp/", TempPath);
#else
	// Mac: Just use the working directory as ExePath.
	SCopy(GetWorkingDirectory(), ExePath);
	AppendBackslash(ExePath);
	SCopy("/tmp/", TempPath);
#endif
	// Force working directory to exe path if desired
	if (forceWorkingDirectory)
		SetWorkingDirectory(ExePath);
	// Log path
	SCopy(ExePath, LogPath);
	if (LogPath[0]) AppendBackslash(LogPath);
	else SCopy(ExePath, LogPath);
	// Screenshot path
	SCopy(ExePath, ScreenshotPath, CFG_MaxString - 1);
	if (ScreenshotFolder.getLength() + SLen(ScreenshotPath) + 1 <= CFG_MaxString)
	{
		SAppend(ScreenshotFolder.getData(), ScreenshotPath);
		AppendBackslash(ScreenshotPath);
	}
	// Player path
	if (PlayerPath[0]) AppendBackslash(PlayerPath);
#ifdef C4ENGINE
	// Create user path if it doesn't already exist
	if (!DirectoryExists(Config.AtUserPath("")))
		CreateDirectory(Config.AtUserPath(""), nullptr); // currently no error handling here; also: no recursive directory creation
#endif
}

char AtPathFilename[_MAX_PATH + 1];

const char *C4Config::AtExePath(const char *szFilename)
{
	SCopy(General.ExePath, AtPathFilename, _MAX_PATH);
	SAppend(szFilename, AtPathFilename, _MAX_PATH);
	return AtPathFilename;
}

const char *C4Config::AtUserPath(const char *szFilename)
{
	SCopy(General.UserPath, AtPathFilename, _MAX_PATH);
	ExpandEnvironmentVariables(AtPathFilename, _MAX_PATH);
	AppendBackslash(AtPathFilename);
	SAppend(szFilename, AtPathFilename, _MAX_PATH);
	return AtPathFilename;
}

const char *C4Config::AtTempPath(const char *szFilename)
{
	SCopy(General.TempPath, AtPathFilename, _MAX_PATH);
	SAppend(szFilename, AtPathFilename, _MAX_PATH);
	return AtPathFilename;
}

#ifdef C4ENGINE

const char *C4Config::AtNetworkPath(const char *szFilename)
{
	SCopy(Network.WorkPath, AtPathFilename, _MAX_PATH);
	SAppend(szFilename, AtPathFilename, _MAX_PATH);
	return AtPathFilename;
}

#endif

const char *C4Config::AtScreenshotPath(const char *szFilename)
{
	SCopy(General.ScreenshotPath, AtPathFilename, _MAX_PATH);
	if (const auto len = SLen(AtPathFilename); len > 0)
		if (AtPathFilename[len - 1] == DirectorySeparator)
			AtPathFilename[len - 1] = '\0';
	if (!DirectoryExists(AtPathFilename) && !CreateDirectory(AtPathFilename, nullptr))
	{
		SCopy(General.ExePath, General.ScreenshotPath, CFG_MaxString - 1);
		SCopy(General.ScreenshotPath, AtPathFilename, _MAX_PATH);
	}
	else
		AppendBackslash(AtPathFilename);
	SAppend(szFilename, AtPathFilename, _MAX_PATH);
	return AtPathFilename;
}

#ifdef C4ENGINE

bool C4ConfigGeneral::CreateSaveFolder(const char *strDirectory, const char *strLanguageTitle)
{
	// Create directory if needed
	if (!DirectoryExists(strDirectory))
		if (!CreateDirectory(strDirectory, nullptr))
			return false;
	// Create title component if needed
	char lang[3]; SCopy(Config.General.Language, lang, 2);
	StdStrBuf strTitleFile; strTitleFile.Format("%s%c%s", strDirectory, DirectorySeparator, C4CFN_WriteTitle);
	StdStrBuf strTitleData; strTitleData.Format("%s:%s", lang, strLanguageTitle);
	CStdFile hFile;
	if (!FileExists(strTitleFile.getData()))
		if (!hFile.Create(strTitleFile.getData()) || !hFile.WriteString(strTitleData.getData()) || !hFile.Close())
			return false;
	// Save folder seems okay
	return true;
}

const char *C4ConfigNetwork::GetLeagueServerAddress()
{
	// Alternate (GUI configurable) league server
	if (UseAlternateServer)
		return AlternateServerAddress;
	// Standard (registry/config file configurable) official league server
	else
		return ServerAddress;
}

void C4ConfigControls::ResetKeys()
{
	StdCompilerNull Comp; Comp.Compile(mkParAdapt(*this, true));
}

#endif

const char *C4Config::AtExeRelativePath(const char *szFilename)
{
	// Specified file is located in ExePath: return relative path
	return GetRelativePathS(szFilename, General.ExePath);
}

void C4Config::ForceRelativePath(StdStrBuf *sFilename)
{
	assert(sFilename);
	// Specified file is located in ExePath?
	const char *szRelative = GetRelativePathS(sFilename->getData(), General.ExePath);
	if (szRelative != sFilename->getData())
	{
		// return relative path
		StdStrBuf sTemp; sTemp.Copy(szRelative);
		sFilename->Take(sTemp);
	}
	else
	{
		// not in ExePath: Is it a global path?
		if (IsGlobalPath(sFilename->getData()))
		{
			// then shorten it (e.g. C:\Temp\Missions.c4f\Goldmine.c4s to Missions.c4f\Goldmine.c4s)
			StdStrBuf sTemp; sTemp.Copy(GetC4Filename(sFilename->getData()));
			sFilename->Take(sTemp);
		}
	}
}

void C4ConfigGeneral::DefaultLanguage()
{
	// No language defined: default to German or English by system language
	if (!Language[0])
	{
		if (isGermanSystem())
			SCopy("DE - Deutsch", Language);
		else
			SCopy("US - English", Language);
	}
	// No fallback sequence defined: use primary language list
	if (!LanguageEx[0])
		GetLanguageSequence(Language, LanguageEx);
}

bool C4Config::Init()
{
	return true;
}

const char *C4Config::GetSubkeyPath(const char *strSubkey)
{
	static char key[1024 + 1];
#ifdef _WIN32
	sprintf(key, "Software\\%s\\%s\\%s", C4CFG_Company, C4CFG_Product, strSubkey);
#else
	sprintf(key, "%s", strSubkey);
#endif
	return key;
}

int C4ConfigGeneral::GetLanguageSequence(const char *strSource, char *strTarget)
{
	// Copy a condensed list of language codes from the source list to the target string,
	// skipping any whitespace or long language descriptions. Language sequences are
	// comma separated.
	int iCount = 0;
	char strLang[2 + 1];
	for (int i = 0; SCopySegment(strSource, i, strLang, ',', 2, true); i++)
		if (strLang[0])
		{
			if (strTarget[0]) SAppendChar(',', strTarget);
			SAppend(strLang, strTarget);
			iCount++;
		}
	return iCount;
}

#ifdef C4ENGINE

void C4ConfigStartup::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(HideMsgStartDedicated,    "HideMsgStartDedicated",    false));
	pComp->Value(mkNamingAdapt(HideMsgPlrTakeOver,       "HideMsgPlrTakeOver",       false));
	pComp->Value(mkNamingAdapt(HideMsgPlrNoTakeOver,     "HideMsgPlrNoTakeOver",     false));
	pComp->Value(mkNamingAdapt(HideMsgNoOfficialLeague,  "HideMsgNoOfficialLeague",  false));
	pComp->Value(mkNamingAdapt(HideMsgIRCDangerous,      "HideMsgIRCDangerous",      false));
	pComp->Value(mkNamingAdapt(AlphabeticalSorting,      "AlphabeticalSorting",      false));
	pComp->Value(mkNamingAdapt(LastPortraitFolderIdx,    "LastPortraitFolderIdx",    0));
}

#endif

void C4Config::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(General,   "General"));
#ifdef C4ENGINE
	pComp->Value(mkNamingAdapt(Controls,  "Controls"));
	for (int i = 0; i < C4ConfigMaxGamepads; ++i)
		pComp->Value(mkNamingAdapt(Gamepads[i], FormatString("Gamepad%d", i).getData()));
	pComp->Value(mkNamingAdapt(Graphics,  "Graphics"));
	pComp->Value(mkNamingAdapt(Sound,     "Sound"));
	pComp->Value(mkNamingAdapt(Network,   "Network"));
	pComp->Value(mkNamingAdapt(Lobby,     "Lobby"));
	pComp->Value(mkNamingAdapt(IRC,       "IRC"));
	pComp->Value(mkNamingAdapt(Developer, "Developer"));
	pComp->Value(mkNamingAdapt(Startup,   "Startup"));
	pComp->Value(mkNamingAdapt(Cooldowns, "Cooldowns"));
	pComp->Value(mkNamingAdapt(Toasts,    "Toasts"));
#endif
}

void C4Config::ExpandEnvironmentVariables(char *strPath, int iMaxLen)
{
#ifdef _WIN32
	char buf[_MAX_PATH + 1];
	ExpandEnvironmentStrings(strPath, buf, _MAX_PATH);
	SCopy(buf, strPath, iMaxLen);
#else // __linux__ or __APPLE___
	StdStrBuf home(getenv("HOME"), false);
	char *rest;
	if (home && (rest = const_cast<char *>(SSearch(strPath, "$HOME"))) && (SLen(strPath) - 5 + home.getLength() <= iMaxLen))
	{
		// String replace... there might be a more elegant way to do this.
		memmove(rest + home.getLength() - SLen("$HOME"), rest, SLen(rest) + 1);
		strncpy(rest - SLen("$HOME"), home.getData(), home.getLength());
	}
#endif
}

#ifdef C4ENGINE
void C4Config::AdaptToCurrentVersion()
{
	switch (General.Version)
	{
#ifdef __APPLE__
	case 349:
		// Mac: set Preloading to false due to it being crash-prone
		General.Preloading = false;
		break;
#endif

	case 347:
		// reset max channels
		Sound.MaxChannels = C4AudioSystem::MaxChannels;
		[[fallthrough]];

	case 346:
		// reenable ingame music
		Sound.RXMusic = true;
		break;

	default:
		break;
	}

	General.Version = C4XVERBUILD;
}
#endif
