/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Game configuration as stored in registry */

#include <C4Include.h>
#include <C4Config.h>

#ifndef BIG_C4INCLUDE
#include <C4Log.h>
#ifdef C4ENGINE
#include <C4Application.h>
#include <C4Network2.h>
#include <C4Language.h>
#endif
#endif

#include <StdFile.h>
#include <StdWindow.h>
#include <StdRegistry.h>

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

bool isGermanSystem()
{
#ifdef _WIN32
	if (PRIMARYLANGID(GetUserDefaultLangID()) == LANG_GERMAN) return true;
#elif defined(__APPLE__) and defined(C4ENGINE)
	bool isGerman();
	if (isGerman()) return true;
#else
	if (strstr(setlocale(LC_MESSAGES, 0), "de")) return true;
#endif
	return false;
}

C4Config *pConfig;

void C4ConfigGeneral::CompileFunc(StdCompiler *pComp)
{
	// For those without the ability to intuitively guess what the falses and trues mean:
	// its mkNamingAdapt(field, name, default, fPrefillDefault, fStoreDefault)
	// where fStoreDefault writes out the value to the config even if it's the same as the default.
#define s mkStringAdaptM
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
	pComp->Value(mkNamingAdapt(s(UserPath), "UserPath", "%APPDATA%\\Clonk Rage",                        false, true));
#elif defined(__linux__)
	pComp->Value(mkNamingAdapt(s(UserPath), "UserPath", "$HOME/.clonk/rage",                            false, true));
#elif defined(__APPLE__)
	pComp->Value(mkNamingAdapt(s(UserPath), "UserPath", "$HOME/Library/Application Support/Clonk Rage", false, true));
#endif
	pComp->Value(mkNamingAdapt(SaveGameFolder, "SaveGameFolder", "Savegames.c4f", false, true));
	pComp->Value(mkNamingAdapt(SaveDemoFolder, "SaveDemoFolder", "Records.c4f",   false, true));
#ifdef C4ENGINE
	pComp->Value(mkNamingAdapt(s(MissionAccess), "MissionAccess", "", false, true));
#endif
	pComp->Value(mkNamingAdapt(FPS,              "FPS",              0));
	pComp->Value(mkNamingAdapt(Record,           "Record",           0));
	pComp->Value(mkNamingAdapt(DefRec,           "DefRec",           0));
	pComp->Value(mkNamingAdapt(ScreenshotFolder, "ScreenshotFolder", "Screenshots", false, true));
	pComp->Value(mkNamingAdapt(FairCrew,         "NoCrew",           0));
	pComp->Value(mkNamingAdapt(FairCrewStrength, "DefCrewStrength",  1000));
	pComp->Value(mkNamingAdapt(ScrollSmooth,     "ScrollSmooth",     4));
	pComp->Value(mkNamingAdapt(AlwaysDebug,      "DebugMode",        0));
#ifdef _WIN32
	pComp->Value(mkNamingAdapt(MMTimer, "MMTimer", 1));
#endif
	pComp->Value(mkNamingAdapt(s(RXFontName),        "FontName",             "Endeavour", false, true));
	pComp->Value(mkNamingAdapt(RXFontSize,           "FontSize",             14,          false, true));
	pComp->Value(mkNamingAdapt(GamepadEnabled,       "GamepadEnabled",       true));
	pComp->Value(mkNamingAdapt(FirstStart,           "FirstStart",           true));
	pComp->Value(mkNamingAdapt(UserPortraitsWritten, "UserPortraitsWritten", false));
	pComp->Value(mkNamingAdapt(ConfigResetSafety,    "ConfigResetSafety",    static_cast<int32_t>(ConfigResetSafetyVal)));
}

void C4ConfigDeveloper::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(AutoFileReload, "AutoFileReload", 1));
}

void C4ConfigGraphics::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(ResX,                 "ResolutionX",          800,   false, true));
	pComp->Value(mkNamingAdapt(ResY,                 "ResolutionY",          600,   false, true));
	pComp->Value(mkNamingAdapt(ShowAllResolutions,   "ShowAllResolutions",   0,     false, true));
	pComp->Value(mkNamingAdapt(SplitscreenDividers,  "SplitscreenDividers",  1));
	pComp->Value(mkNamingAdapt(ShowPlayerInfoAlways, "ShowPlayerInfoAlways", 1,     false, true));
	pComp->Value(mkNamingAdapt(ShowPortraits,        "ShowPortraits",        1,     false, true));
	pComp->Value(mkNamingAdapt(AddNewCrewPortraits,  "AddNewCrewPortraits",  1,     false, true));
	pComp->Value(mkNamingAdapt(SaveDefaultPortraits, "SaveDefaultPortraits", 1,     false, true));
	pComp->Value(mkNamingAdapt(ShowCommands,         "ShowCommands",         1,     false, true));
	pComp->Value(mkNamingAdapt(ShowCommandKeys,      "ShowCommandKeys",      1,     false, true));
	pComp->Value(mkNamingAdapt(ShowStartupMessages,  "ShowStartupMessages",  1,     false, true));
	pComp->Value(mkNamingAdapt(ColorAnimation,       "ColorAnimation",       0,     false, true));
	pComp->Value(mkNamingAdapt(SmokeLevel,           "SmokeLevel",           200,   false, true));
	pComp->Value(mkNamingAdapt(VerboseObjectLoading, "VerboseObjectLoading", 0));
	pComp->Value(mkNamingAdapt(VideoModule,          "VideoModule",          0,     false, true));
	pComp->Value(mkNamingAdapt(UpperBoard,           "UpperBoard",           1,     false, true));
	pComp->Value(mkNamingAdapt(ShowClock,            "ShowClock",            0,     false, true));
	pComp->Value(mkNamingAdapt(ShowCrewNames,        "ShowCrewNames",        1,     false, true));
	pComp->Value(mkNamingAdapt(ShowCrewCNames,       "ShowCrewCNames",       1,     false, true));
	pComp->Value(mkNamingAdapt(NewGfxCfg,            "NewGfxCfg",            0));
	pComp->Value(mkNamingAdapt(NewGfxCfgGL,          "NewGfxCfgGL",          0));
	pComp->Value(mkNamingAdapt(MsgBoard,             "MsgBoard",             1));
	pComp->Value(mkNamingAdapt(PXSGfx,               "PXSGfx",               1));
	pComp->Value(mkNamingAdapt(Engine,               "Engine",               0,     false, true));
	pComp->Value(mkNamingAdapt(TexIndent,            "TexIndent",            0));
	pComp->Value(mkNamingAdapt(TexIndentGL,          "TexIndentGL",          0));
	pComp->Value(mkNamingAdapt(BlitOff,              "BlitOff",              -50));
	pComp->Value(mkNamingAdapt(BlitOffGL,            "BlitOffGL",            0));
	pComp->Value(mkNamingAdapt(Gamma1,               "Gamma1",               0));
	pComp->Value(mkNamingAdapt(Gamma2,               "Gamma2",               0x808080));
	pComp->Value(mkNamingAdapt(Gamma3,               "Gamma3",               0xffffff));
	pComp->Value(mkNamingAdapt(Currency,             "Currency",             0));
	pComp->Value(mkNamingAdapt(RenderInactiveEM,     "RenderInactiveEM",     1));
	pComp->Value(mkNamingAdapt(DisableGamma,         "DisableGamma",         0,     false, true));
	pComp->Value(mkNamingAdapt(Monitor,              "Monitor",              0)); // 0 = D3DADAPTER_DEFAULT
	pComp->Value(mkNamingAdapt(FireParticles,        "FireParticles",        true));
	pComp->Value(mkNamingAdapt(MaxRefreshDelay,      "MaxRefreshDelay",      30));
	pComp->Value(mkNamingAdapt(DDrawCfg.Shader,      "Shader",               false, false, true));
	pComp->Value(mkNamingAdapt(AutoFrameSkip,        "AutoFrameSkip",        false));
}

void C4ConfigSound::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(RXSound,     "Sound",       1,   false, true));
	pComp->Value(mkNamingAdapt(RXMusic,     "Music",       1,   false, true));
	pComp->Value(mkNamingAdapt(FEMusic,     "MenuMusic",   1,   false, true));
	pComp->Value(mkNamingAdapt(FESamples,   "MenuSound",   1,   false, true));
	pComp->Value(mkNamingAdapt(FMMode,      "FMMode",      1));
	pComp->Value(mkNamingAdapt(Verbose,     "Verbose",     0));
	pComp->Value(mkNamingAdapt(MusicVolume, "MusicVolume", 100, false, true));
	pComp->Value(mkNamingAdapt(SoundVolume, "SoundVolume", 100, false, true));
}

void C4ConfigNetwork::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(ControlRate,           "ControlRate",           2,         false, true));
	pComp->Value(mkNamingAdapt(s(WorkPath),           "WorkPath",              "Network", false, true));
	pComp->Value(mkNamingAdapt(Lobby,                 "Lobby",                 0));
	pComp->Value(mkNamingAdapt(NoRuntimeJoin,         "NoRuntimeJoin",         0,         false, true));
	pComp->Value(mkNamingAdapt(MaxResSearchRecursion, "MaxResSearchRecursion", 1,         false, true));
	pComp->Value(mkNamingAdapt(Comment,               "Comment",               "",        false, true));
#ifdef C4ENGINE
	pComp->Value(mkNamingAdapt(PortTCP,       "PortTCP",       C4NetStdPortTCP,       false, true));
	pComp->Value(mkNamingAdapt(PortUDP,       "PortUDP",       C4NetStdPortUDP,       false, true));
	pComp->Value(mkNamingAdapt(PortDiscovery, "PortDiscovery", C4NetStdPortDiscovery, false, true));
	pComp->Value(mkNamingAdapt(PortRefServer, "PortRefServer", C4NetStdPortRefServer, false, true));
#endif
	pComp->Value(mkNamingAdapt(ControlMode,     "ControlMode",     0));
	pComp->Value(mkNamingAdapt(LocalName,       "LocalName",       "Unknown",       false, true));
	pComp->Value(mkNamingAdapt(Nick,            "Nick",            "",              false, true));
	pComp->Value(mkNamingAdapt(MaxLoadFileSize, "MaxLoadFileSize", 5 * 1024 * 1024, false, true));

	pComp->Value(mkNamingAdapt(MasterServerSignUp,        "MasterServerSignUp",     1));
	pComp->Value(mkNamingAdapt(MasterReferencePeriod,     "MasterReferencePeriod",  120));
	pComp->Value(mkNamingAdapt(LeagueServerSignUp,        "LeagueServerSignUp",     0));
	pComp->Value(mkNamingAdapt(s(ServerAddress),          "ServerAddress",          C4CFG_LeagueServer));
	pComp->Value(mkNamingAdapt(UseAlternateServer,        "UseAlternateServer",     0));
	pComp->Value(mkNamingAdapt(s(AlternateServerAddress), "AlternateServerAddress", C4CFG_LeagueServer));
	pComp->Value(mkNamingAdapt(s(UpdateServerAddress),    "UpdateServerAddress",    C4CFG_UpdateServer));
	pComp->Value(mkNamingAdapt(s(LastPassword),           "LastPassword",           "Wipf"));
	pComp->Value(mkNamingAdapt(AutomaticUpdate,           "AutomaticUpdate",        0, false, true));
	pComp->Value(mkNamingAdapt(LastUpdateTime,            "LastUpdateTime",         0));
	pComp->Value(mkNamingAdapt(AsyncMaxWait,              "AsyncMaxWait",           2));

	pComp->Value(mkNamingAdapt(s(PuncherAddress), "PuncherAddress", "clonk.de:11115")); // maybe store default for this one?
}

void C4ConfigLobby::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(AllowPlayerSave, "AllowPlayerSave", 0, false, false));
	pComp->Value(mkNamingAdapt(CountdownTime,   "CountdownTime",   5, false, false));
}

void C4ConfigIRC::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(s(Server),        "Server2",          "irc.euirc.net", false, true));
	pComp->Value(mkNamingAdapt(s(Nick),          "Nick",             "",              false, true));
	pComp->Value(mkNamingAdapt(s(RealName),      "RealName",         "",              false, true));
	pComp->Value(mkNamingAdapt(s(Channel),       "Channel",          "#clonken",      false, true));
	pComp->Value(mkNamingAdapt(AllowAllChannels, "AllowAllChannels", 0,               false, true));
}

void C4ConfigGamepad::CompileFunc(StdCompiler *pComp, bool fButtonsOnly)
{
	/* The defaults here are for a Logitech Dual Action under Linux-SDL. Better than nothing, I guess. */
	if (!fButtonsOnly)
	{
		for (int i = 0; i < 6; ++i)
		{
			pComp->Value(mkNamingAdapt(AxisMin[i],        FormatString("Axis%dMin",        i).getData(), 0u));
			pComp->Value(mkNamingAdapt(AxisMax[i],        FormatString("Axis%dMax",        i).getData(), 0u));
			pComp->Value(mkNamingAdapt(AxisCalibrated[i], FormatString("Axis%dCalibrated", i).getData(), false));
		}
	}
	pComp->Value(mkNamingAdapt(Button[0],  "Button1",  -1));
	pComp->Value(mkNamingAdapt(Button[1],  "Button2",  -1));
	pComp->Value(mkNamingAdapt(Button[2],  "Button3",  -1));
	pComp->Value(mkNamingAdapt(Button[3],  "Button4",  -1));
	pComp->Value(mkNamingAdapt(Button[4],  "Button5",  -1));
	pComp->Value(mkNamingAdapt(Button[5],  "Button6",  -1));
	pComp->Value(mkNamingAdapt(Button[6],  "Button7",  -1));
	pComp->Value(mkNamingAdapt(Button[7],  "Button8",  -1));
	pComp->Value(mkNamingAdapt(Button[8],  "Button9",  -1));
	pComp->Value(mkNamingAdapt(Button[9],  "Button10", -1));
	pComp->Value(mkNamingAdapt(Button[10], "Button11", -1));
	pComp->Value(mkNamingAdapt(Button[11], "Button12", -1));
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

	pComp->Value(mkNamingAdapt(Keyboard[0][ 0], "Kbd1Key1",  KEY('Q', XK_q, SDLK_q)));
	pComp->Value(mkNamingAdapt(Keyboard[0][ 1], "Kbd1Key2",  KEY('W', XK_w, SDLK_w)));
	pComp->Value(mkNamingAdapt(Keyboard[0][ 2], "Kbd1Key3",  KEY('E', XK_e, SDLK_e)));
	pComp->Value(mkNamingAdapt(Keyboard[0][ 3], "Kbd1Key4",  KEY('A', XK_a, SDLK_a)));
	pComp->Value(mkNamingAdapt(Keyboard[0][ 4], "Kbd1Key5",  KEY('S', XK_s, SDLK_s)));
	pComp->Value(mkNamingAdapt(Keyboard[0][ 5], "Kbd1Key6",  KEY('D', XK_d, SDLK_d)));
	pComp->Value(mkNamingAdapt(Keyboard[0][ 6], "Kbd1Key7",  fGer ? KEY('Y', XK_y,    SDLK_y)    : KEY('Z', XK_z, SDLK_z)));
	pComp->Value(mkNamingAdapt(Keyboard[0][ 7], "Kbd1Key8",  KEY('X', XK_x, SDLK_x)));
	pComp->Value(mkNamingAdapt(Keyboard[0][ 8], "Kbd1Key9",  KEY('C', XK_c, SDLK_c)));
	pComp->Value(mkNamingAdapt(Keyboard[0][ 9], "Kbd1Key10", fGer ? KEY(226, XK_less, SDLK_LESS) : KEY('R', XK_r, SDLK_r)));
	pComp->Value(mkNamingAdapt(Keyboard[0][10], "Kbd1Key11", KEY('V', XK_v, SDLK_v)));
	pComp->Value(mkNamingAdapt(Keyboard[0][11], "Kbd1Key12", KEY('F', XK_f, SDLK_f)));

	pComp->Value(mkNamingAdapt(Keyboard[1][ 0], "Kbd2Key1",  KEY(103, XK_KP_Home,      SDLK_KP7)));
	pComp->Value(mkNamingAdapt(Keyboard[1][ 1], "Kbd2Key2",  KEY(104, XK_KP_Up,        SDLK_KP8)));
	pComp->Value(mkNamingAdapt(Keyboard[1][ 2], "Kbd2Key3",  KEY(105, XK_KP_Page_Up,   SDLK_KP9)));
	pComp->Value(mkNamingAdapt(Keyboard[1][ 3], "Kbd2Key4",  KEY(100, XK_KP_Left,      SDLK_KP4)));
	pComp->Value(mkNamingAdapt(Keyboard[1][ 4], "Kbd2Key5",  KEY(101, XK_KP_Begin,     SDLK_KP5)));
	pComp->Value(mkNamingAdapt(Keyboard[1][ 5], "Kbd2Key6",  KEY(102, XK_KP_Right,     SDLK_KP6)));
	pComp->Value(mkNamingAdapt(Keyboard[1][ 6], "Kbd2Key7",  KEY( 97, XK_KP_End,       SDLK_KP1)));
	pComp->Value(mkNamingAdapt(Keyboard[1][ 7], "Kbd2Key8",  KEY( 98, XK_KP_Down,      SDLK_KP2)));
	pComp->Value(mkNamingAdapt(Keyboard[1][ 8], "Kbd2Key9",  KEY( 99, XK_KP_Page_Down, SDLK_KP3)));
	pComp->Value(mkNamingAdapt(Keyboard[1][ 9], "Kbd2Key10", KEY( 96, XK_KP_Insert,    SDLK_KP0)));
	pComp->Value(mkNamingAdapt(Keyboard[1][10], "Kbd2Key11", KEY(110, XK_KP_Delete,    SDLK_KP_PERIOD)));
	pComp->Value(mkNamingAdapt(Keyboard[1][11], "Kbd2Key12", KEY(107, XK_KP_Add,       SDLK_KP_PLUS)));

	pComp->Value(mkNamingAdapt(Keyboard[2][ 0], "Kbd3Key1",  KEY('I', XK_i,          SDLK_i)));
	pComp->Value(mkNamingAdapt(Keyboard[2][ 1], "Kbd3Key2",  KEY('O', XK_o,          SDLK_o)));
	pComp->Value(mkNamingAdapt(Keyboard[2][ 2], "Kbd3Key3",  KEY('P', XK_p,          SDLK_p)));
	pComp->Value(mkNamingAdapt(Keyboard[2][ 3], "Kbd3Key4",  KEY('K', XK_k,          SDLK_k)));
	pComp->Value(mkNamingAdapt(Keyboard[2][ 4], "Kbd3Key5",  KEY('L', XK_l,          SDLK_l)));
	pComp->Value(mkNamingAdapt(Keyboard[2][ 5], "Kbd3Key6",  fGer ? KEY(192, XK_odiaeresis, SDLK_WORLD_4) : KEY(0xBA, XK_semicolon, SDLK_SEMICOLON)));
	pComp->Value(mkNamingAdapt(Keyboard[2][ 6], "Kbd3Key7",  KEY(188, XK_comma,      SDLK_COMMA)));
	pComp->Value(mkNamingAdapt(Keyboard[2][ 7], "Kbd3Key8",  KEY(190, XK_period,     SDLK_PERIOD)));
	pComp->Value(mkNamingAdapt(Keyboard[2][ 8], "Kbd3Key9",  fGer ? KEY(189, XK_minus,      SDLK_MINUS)   : KEY(0xBF, XK_slash,     SDLK_SLASH)));
	pComp->Value(mkNamingAdapt(Keyboard[2][ 9], "Kbd3Key10", KEY('M', XK_m,          SDLK_m)));
	pComp->Value(mkNamingAdapt(Keyboard[2][10], "Kbd3Key11", KEY(222, XK_adiaeresis, SDLK_WORLD_3)));
	pComp->Value(mkNamingAdapt(Keyboard[2][11], "Kbd3Key12", KEY(186, XK_udiaeresis, SDLK_WORLD_2)));

	pComp->Value(mkNamingAdapt(Keyboard[3][ 0], "Kbd4Key1",  KEY(VK_INSERT, XK_Insert,    SDLK_INSERT)));
	pComp->Value(mkNamingAdapt(Keyboard[3][ 1], "Kbd4Key2",  KEY(VK_HOME,   XK_Home,      SDLK_HOME)));
	pComp->Value(mkNamingAdapt(Keyboard[3][ 2], "Kbd4Key3",  KEY(VK_PRIOR,  XK_Page_Up,   SDLK_PAGEUP)));
	pComp->Value(mkNamingAdapt(Keyboard[3][ 3], "Kbd4Key4",  KEY(VK_DELETE, XK_Delete,    SDLK_DELETE)));
	pComp->Value(mkNamingAdapt(Keyboard[3][ 4], "Kbd4Key5",  KEY(VK_UP,     XK_Up,        SDLK_UP)));
	pComp->Value(mkNamingAdapt(Keyboard[3][ 5], "Kbd4Key6",  KEY(VK_NEXT,   XK_Page_Down, SDLK_PAGEDOWN)));
	pComp->Value(mkNamingAdapt(Keyboard[3][ 6], "Kbd4Key7",  KEY(VK_LEFT,   XK_Left,      SDLK_LEFT)));
	pComp->Value(mkNamingAdapt(Keyboard[3][ 7], "Kbd4Key8",  KEY(VK_DOWN,   XK_Down,      SDLK_DOWN)));
	pComp->Value(mkNamingAdapt(Keyboard[3][ 8], "Kbd4Key9",  KEY(VK_RIGHT,  XK_Right,     SDLK_RIGHT)));
	pComp->Value(mkNamingAdapt(Keyboard[3][ 9], "Kbd4Key10", KEY(VK_END,    XK_End,       SDLK_END)));
	pComp->Value(mkNamingAdapt(Keyboard[3][10], "Kbd4Key11", KEY(VK_RETURN, XK_Return,    SDLK_RETURN)));
	pComp->Value(mkNamingAdapt(Keyboard[3][11], "Kbd4Key12", KEY(VK_BACK,   XK_BackSpace, SDLK_BACKSPACE)));

	if (fKeysOnly) return;

	pComp->Value(mkNamingAdapt(MouseAScroll,      "MouseAutoScroll",   0));
	pComp->Value(mkNamingAdapt(GamepadGuiControl, "GamepadGuiControl", 0, false, true));

#undef KEY
#undef s
#endif // USE_CONSOLE
}

C4Config::C4Config()
{
	pConfig = this;
	Default();
}

C4Config::~C4Config()
{
	fConfigLoaded = false;
	pConfig = nullptr;
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
				StdStrBuf home(getenv("HOME"));
				if (home) { home += "/"; }
				filename.Copy(home);
#ifdef __APPLE__
				filename += "Library/Preferences/de.clonk.rage.config";
#else
				filename += ".clonk/rage/config";
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
					StdStrBuf filename(getenv("HOME"));
					if (filename) { filename += "/"; }
					filename += ".clonk";
					// Upgrade from older installations which had a single ~/.clonk config file
					if (!DirectoryExists(filename.getData()))
					{
						EraseItem(filename.getData());
						CreateDirectory(filename.getData());
					}
					filename += "/rage";
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
	catch (StdCompiler::Exception *pExc)
	{
		// Configuration file syntax error?
#ifdef C4ENGINE
		LogF("Error loading configuration: %s"/*restbl not yet loaded*/, pExc->Msg.getData());
#endif
		delete pExc;
		return false;
	}

	// Config postinit
	General.DeterminePaths(forceWorkingDirectory);
#ifdef C4ENGINE
#ifdef HAVE_WINSOCK
	bool fWinSock = AcquireWinSock();
#endif
	if (SEqual(Network.LocalName.getData(), "Unknown"))
	{
		char LocalName[25 + 1]; *LocalName = 0;
		gethostname(LocalName, 25);
		if (*LocalName) Network.LocalName.Copy(LocalName);
	}
#ifdef HAVE_WINSOCK
	if (fWinSock) ReleaseWinSock();
#endif
#endif
	General.DefaultLanguage();
#if defined(USE_GL)
	if (Graphics.Engine != GFXENGN_NOGFX) Graphics.Engine = GFXENGN_OPENGL;
#endif
	// OpenGL
	DDrawCfg.Set(Graphics.NewGfxCfgGL, (float)Graphics.TexIndentGL / 1000.0f, (float)Graphics.BlitOffGL / 100.0f);
	// Warning against invalid ports
#ifdef C4ENGINE
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
				filename += "Library/Preferences/de.clonk.rage.config";
#else
				filename += ".clonk/rage/config";
#endif
			}
			StdCompilerINIWrite IniWrite;
			IniWrite.Decompile(*this);
			IniWrite.getOutput().SaveToFile(filename.getData());
		}
	}
	catch (StdCompiler::Exception *pExc)
	{
#ifdef C4ENGINE
		LogF(LoadResStr("IDS_ERR_CONFSAVE"), pExc->Msg.getData());
#endif
		delete pExc;
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

const char *C4Config::AtNetworkPath(const char *szFilename)
{
	SCopy(Network.WorkPath, AtPathFilename, _MAX_PATH);
	SAppend(szFilename, AtPathFilename, _MAX_PATH);
	return AtPathFilename;
}

const char *C4Config::AtScreenshotPath(const char *szFilename)
{
	int len;
	SCopy(General.ScreenshotPath, AtPathFilename, _MAX_PATH);
	if (len = SLen(AtPathFilename))
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

#endif

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

void C4ConfigStartup::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(HideMsgMMTimerChange,     "HideMsgMMTimerChange",     0));
	pComp->Value(mkNamingAdapt(HideMsgStartDedicated,    "HideMsgStartDedicated",    0));
	pComp->Value(mkNamingAdapt(HideMsgPlrTakeOver,       "HideMsgPlrTakeOver",       0));
	pComp->Value(mkNamingAdapt(HideMsgPlrNoTakeOver,     "HideMsgPlrNoTakeOver",     0));
	pComp->Value(mkNamingAdapt(HideMsgNoOfficialLeague,  "HideMsgNoOfficialLeague",  0));
	pComp->Value(mkNamingAdapt(HideMsgIRCDangerous,      "HideMsgIRCDangerous",      0));
	pComp->Value(mkNamingAdapt(AlphabeticalSorting,      "AlphabeticalSorting",      0));
	pComp->Value(mkNamingAdapt(LastPortraitFolderIdx,    "LastPortraitFolderIdx",    0));
}

void C4Config::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(General,   "General"));
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
}

void C4Config::ExpandEnvironmentVariables(char *strPath, int iMaxLen)
{
#ifdef _WIN32
	char buf[_MAX_PATH + 1];
	ExpandEnvironmentStrings(strPath, buf, _MAX_PATH);
	SCopy(buf, strPath, iMaxLen);
#else // __linux__ or __APPLE___
	StdStrBuf home(getenv("HOME"));
	char *rest;
	if (home && (rest = (char *)SSearch(strPath, "$HOME")) && (SLen(strPath) - 5 + home.getLength() <= iMaxLen))
	{
		// String replace... there might be a more elegant way to do this.
		memmove(rest + home.getLength() - SLen("$HOME"), rest, SLen(rest) + 1);
		strncpy(rest - SLen("$HOME"), home.getData(), home.getLength());
	}
#endif
}
