/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Main framework class */

#include "C4Explorer.h"

#include "C4ExplorerDlg.h"
#include "C4LicenseDlg.h"

#include "C4Language.h"

// Some logging stubs not really used in frontend...

bool Log(const char *szMessage) { return FALSE; }

BOOL LogF(const char *strMessage, ...) { return FALSE; }

bool LogFatal(const char *szMessage) { return false; }

void StdCompilerWarnCallback(void *pData, const char *szPosition, const char *szError)
	{
	const char *szName = reinterpret_cast<const char *>(pData);
	if(!szPosition || !*szPosition)
		LogF("WARNING: %s: %s", szName, szError);
	else
		LogF("WARNING: %s (%s): %s", szName, szPosition, szError);
	}


C4ExplorerApp theApp;

BEGIN_MESSAGE_MAP(C4ExplorerApp, CWinApp)
	//{{AFX_MSG_MAP(C4ExplorerApp)
	//}}AFX_MSG_MAP
	//ON_COMMAND(ID_HELP, CWinApp::OnHelp)
	//ON_COMMAND(ID_HELP_FINDER, CWinApp::OnHelpFinder)
END_MESSAGE_MAP()

C4ExplorerApp::C4ExplorerApp()
	{
	hRichEditDLL=NULL;
	ProcessRouting=C4EX_ProcessRouting_None;
	SCopy("%s",ProcessFormat);
	}

BOOL C4ExplorerApp::InitInstance()
	{
	
	// Nur eine Instanz darf zur Zeit laufen
	if (OpenMutex(MUTEX_ALL_ACCESS, true, "Clonk4FE_OneInstanceOnly_Mutex") != NULL) 
		return false;
	else 
		CreateMutex(NULL, true, "Clonk4FE_OneInstanceOnly_Mutex");

	// App registry
	SetRegistryKey(C4CFG_Company);

	// System
	Randomize();

	// Config
	CheckConfig();

	// Init C4Group
	C4Group_SetTempPath(Config.General.TempPath);
	C4Group_SetProcessCallback(&ProcessCallback);
	C4Group_SetSortList(C4CFN_FLS);

	// Init external language packs
	Languages.Init();

	// Load language string table
	if (!Languages.LoadLanguage(Config.General.LanguageEx))
	{
		MessageBox(NULL, "Error loading language string table.", C4EDITORCAPTION, MB_OK);
		return false;
	}

	// User status
	SetUserGroupMaker();

	// Clear editing leftover
	EditSlots.ClearLeftover();

	// Editor available only to registered players for now
	/*if (!GetCfg()->Registered())  FREEWARE
	{
		MessageBox(NULL, LoadResStr("IDS_MSG_EDITORREGONLY"), C4EDITORCAPTION, MB_OK | MB_ICONERROR);
		return false;
	}*/

	// Display developer mode license dialog and do password check
	if (!GetCfg()->Explorer.LicenseAccepted)
	{
		// License dialog
		C4LicenseDlg dlgLicense;
		if (dlgLicense.DoModal() != IDOK) return false;
	}
	// Now we're good
	GetCfg()->Explorer.LicenseAccepted = true;

 	// Show splash dialog
	SplashDlg.Show();

	// Init libraries
	hRichEditDLL = LoadLibrary("RichEd32.dll");
	if (!hRichEditDLL)
		{
		MessageBox(NULL, LoadResStr("IDS_FAIL_RICHEDDLL"), C4EDITORCAPTION, MB_OK | MB_ICONERROR);
		return false;
		}

	// ComCtl32-Version ermitteln
	ComCtlVersion = GetComCtlVersion();
	if (!ComCtlVersion)
		{
		TRACE("\nComCtl-Version ist Mist");
		ComCtlVersion = 400;  // Amen
		}

	// Palette
	LoadPalette(IDB_PALETTE, Palette);

	// Bitmaps
	dibPaper.Load(IDB_PAPER);
	dibButton.Load(IDB_BUTTON);

	// Ranks
	PlayerRanks.Init(GetCfg()->GetSubkeyPath("PlayerRanks"), LoadResStr("IDS_RANKS_PLAYER"), 200);
	ClonkRanks.Init(GetCfg()->GetSubkeyPath("ClonkRanks"), LoadResStr("IDS_RANKS_CLONK"), 1000);

	// Main dialog
	C4ExplorerDlg dlgMain;
	m_pMainWnd = &dlgMain;
	dlgMain.DoModal();

	// Configuration
	GetCfg()->General.Definitions[0]=0; // No leftover parameters
	GetCfg()->Save();

	return FALSE;
	}

int C4ExplorerApp::ExitInstance() 
	{

	// Free libraries
	if (hRichEditDLL) FreeLibrary(hRichEditDLL);	

	// Clear external language packs and string table
	Languages.Clear();
	Languages.ClearLanguage();

	return CWinApp::ExitInstance();
	}

BYTE C4ExplorerApp::GetCharsetCode(const char *strCharset)
	{
	// Match charset name to WinGDI codes
	if (SEqualNoCase(strCharset, "SHIFTJIS"))			return SHIFTJIS_CHARSET;		// 128
	if (SEqualNoCase(strCharset, "HANGUL"))				return HANGUL_CHARSET;			// 129
	if (SEqualNoCase(strCharset, "JOHAB"))				return JOHAB_CHARSET;				// 130
	if (SEqualNoCase(strCharset, "CHINESEBIG5"))	return CHINESEBIG5_CHARSET;	// 136
	if (SEqualNoCase(strCharset, "GREEK"))				return GREEK_CHARSET;				// 161
	if (SEqualNoCase(strCharset, "TURKISH"))			return TURKISH_CHARSET;			// 162
	if (SEqualNoCase(strCharset, "VIETNAMESE"))		return VIETNAMESE_CHARSET;	// 163
	if (SEqualNoCase(strCharset, "HEBREW"))				return HEBREW_CHARSET;			// 177
	if (SEqualNoCase(strCharset, "ARABIC"))				return ARABIC_CHARSET;			// 178
	if (SEqualNoCase(strCharset, "BALTIC"))				return BALTIC_CHARSET;			// 186
	if (SEqualNoCase(strCharset, "RUSSIAN"))			return RUSSIAN_CHARSET;			// 204
	if (SEqualNoCase(strCharset, "THAI"))					return THAI_CHARSET;				// 222
	if (SEqualNoCase(strCharset, "EASTEUROPE"))		return EASTEUROPE_CHARSET;	// 238
	// Default ANSI/WESTERN 0
	return 0;
	}

void C4ExplorerApp::CheckConfig()
	{
	// Load and init
	Config.Load();
	Config.Init();
	// Validate and default modules
	Config.ValidateModules(Config.General.ExePath,Config.General.Participants, true);
	Config.ValidateModules(Config.General.ExePath,Config.Explorer.Definitions);
	// Ensure standard objects
	if (!SIsModule( Config.Explorer.Definitions, Config.AtExePath(C4CFN_Objects) ))
		SAddModule( Config.Explorer.Definitions, Config.AtExePath(C4CFN_Objects) );
	}

void C4ExplorerApp::SetUserGroupMaker()
	{
	if (Config.Registered())
		C4Group_SetMaker(Config.General.Name);
	else
		C4Group_SetMaker(LoadResStr("IDS_SEC_UNREGUSER"));
	}

void C4ExplorerApp::HideSplash()
	{
	SplashDlg.Hide();
	}

BOOL C4ExplorerApp::ProcessCallback(const char *szText, int iProcess)
	{
	// Debug trace
	TRACE("%s\n\r",szText);
	// Status (filename only)
	if (GetApp()->ProcessRouting & C4EX_ProcessRouting_Status)
		{
		CString sMessage;
		sMessage.Format(GetApp()->ProcessFormat,GetFilename(szText));
		SetStatus(sMessage);
		}
	// Refresh
	if (GetApp()->ProcessRouting & C4EX_ProcessRouting_Refresh)
		if (GetApp()->m_pMainWnd)
			GetApp()->m_pMainWnd->SendMessage(WM_USER_REFRESHITEM,0,(LPARAM)szText);
	// Reload
	if (GetApp()->ProcessRouting & C4EX_ProcessRouting_Reload)
		if (GetApp()->m_pMainWnd)
			GetApp()->m_pMainWnd->SendMessage(WM_USER_RELOADITEM,0,(LPARAM)szText);
	// Done
	return TRUE;
	}

C4ExplorerApp::~C4ExplorerApp()
	{

	}

void C4ExplorerApp::SetProcessRouting(int iProcessRouting, const char *szProcessFormat)
	{
	ProcessRouting = iProcessRouting;
	if (szProcessFormat) SCopy(szProcessFormat,ProcessFormat,1024);
	else SCopy("%s",ProcessFormat);
	}

#if _MSC_VER >= 1300
// vc7 wants to link it?
LRESULT APIENTRY FullScreenWinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
	return 0;
	}
#endif