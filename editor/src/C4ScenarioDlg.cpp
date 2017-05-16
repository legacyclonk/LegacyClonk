/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Scenario properties dialog */

#include "C4Explorer.h"

#include "C4ScenarioDlg.h"

BEGIN_MESSAGE_MAP(C4ScenarioDlg, CDialog)
	//{{AFX_MSG_MAP(C4ScenarioDlg)
	ON_WM_ERASEBKGND()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TABSCENARIO, OnSelChangeTabScenario)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/*BOOL C4ScenarioDlg::OnEraseBkgnd(CDC* pDC) 
	{
	GetApp()->dibPaper2.DrawTiles(pDC, this);
	return true;
	}*/

C4ScenarioDlg::C4ScenarioDlg(CWnd* pParent /*=NULL*/)
	: CDialog(C4ScenarioDlg::IDD, pParent)
	{
	//{{AFX_DATA_INIT(C4ScenarioDlg)
	pCurrentPage = NULL;
	//}}AFX_DATA_INIT
	//TurnPageSound.Load(IDW_TURNPAGE);
	}


void C4ScenarioDlg::DoDataExchange(CDataExchange* pDX)
	{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(C4ScenarioDlg)
	DDX_Control(pDX, IDC_TABSCENARIO, m_TabScenario);
	DDX_Control(pDX, IDOK, m_ButtonOK);
	DDX_Control(pDX, IDCANCEL, m_ButtonCancel);
	//}}AFX_DATA_MAP
	}

bool C4ScenarioDlg::Init(const char *szFilename, bool fOriginal)
	{
	CWaitCursor WaitCursor;
	// Store item name
	Filename = szFilename;
	Original = fOriginal;
	// Load definitions
	SetStatus("IDS_STATUS_LOADINGDEFS");
	if (!Defs.LoadForScenario( szFilename,
														 GetCfg()->Explorer.Definitions,
														 C4D_Load_FE,
														 GetCfg()->General.LanguageEx))
		C4MessageBox("IDS_MSG_NODEFS");
	// Open scenario file
	C4Group hGroup;
	if (!hGroup.Open(Filename)) return false;
	// Get author
	Author = hGroup.GetMaker();
	// Load core
	if (!C4S.Load(hGroup)) return false;
	// Load title
	SCopy(C4S.Head.Title,Title,C4MaxTitle);
	C4ComponentHost TitleHost;
	if (TitleHost.LoadEx("Title", hGroup, C4CFN_Title, GetCfg()->General.LanguageEx))
	{
		StdStrBuf buf;
		TitleHost.GetLanguageString(GetCfg()->General.LanguageEx, buf);
		SCopy(buf.getData(), Title, C4MaxTitle);
	}
	// Init landscape control
	SetStatus("IDS_STATUS_LOADINGMAP");
	PageLandscape.m_LandscapeCtrl.Init(hGroup, C4S.Landscape);
	// Close scenario file
	hGroup.Close();
	// Set dialog data
	SetData();
	// Done
	return true;
	}

BOOL C4ScenarioDlg::OnInitDialog() 
	{
	CDialog::OnInitDialog();
	// Tabs einfügen
	TC_ITEM Item;
	Item.mask = TCIF_TEXT | TCIF_PARAM;
	int iTab = 0;
	Item.lParam = EP_Game;				Item.pszText = (char *) LoadResStr("IDS_DLG_GAME");				m_TabScenario.InsertItem(iTab++, &Item);
	Item.lParam = EP_PlayerStart;	Item.pszText = (char *) LoadResStr("IDS_DLG_PLAYERSTART");	m_TabScenario.InsertItem(iTab++, &Item);
	Item.lParam = EP_Landscape;		Item.pszText = (char *) LoadResStr("IDS_DLG_LANDSCAPE");		m_TabScenario.InsertItem(iTab++, &Item);
	Item.lParam = EP_Environment;	Item.pszText = (char *) LoadResStr("IDS_DLG_ENVIRONMENT");	m_TabScenario.InsertItem(iTab++, &Item);
	Item.lParam = EP_Weather;			Item.pszText = (char *) LoadResStr("IDS_DLG_WEATHER");			m_TabScenario.InsertItem(iTab++, &Item);
	// Create property pages
	CreatePage(PageGame, PageGame.IDD, true);
	CreatePage(PagePlayerStart, PagePlayerStart.IDD, false);
	CreatePage(PageLandscape, PageLandscape.IDD, false);
	CreatePage(PageWeather, PageWeather.IDD, false);
	CreatePage(PageEnvironment, PageEnvironment.IDD, false);
	// Neuzeichnen wegen transparentem Tabstrip
	InvalidateTabStrip(false);
	// Update dialog text
	UpdateText();
	// Done
	return TRUE;
	}

void C4ScenarioDlg::UpdateText()
	{
	// Caption
	CString Caption; Caption.Format("%s - %s",LoadResStr("IDS_DLG_SCENARIOPROPERTIES"),Title);
	SetWindowText(Caption);
	// Buttons
	m_ButtonOK.Set("IDS_BTN_OK");
	m_ButtonCancel.Set("IDS_BTN_CANCEL");
	}

void C4ScenarioDlg::InvalidateTabStrip(bool BordersOnly)
	{
	CRect Rect1, Rect2, ClientRect, TabRect;
	GetClientRect(ClientRect);

	if (BordersOnly && m_TabScenario.GetRowCount() > 1)
		{
		// Nur Ränder invalidieren
		Rect1.SetRect(3, 3, 15, 80);
		Rect2.SetRect(ClientRect.right-15, 3, ClientRect.right-3, 80);
		InvalidateRect(Rect1);
		InvalidateRect(Rect2);
		}
	else 
		{
		// Ganzen Streifen invalidieren
		m_TabScenario.GetItemRect(0, TabRect);
		int Height = TabRect.Height() * m_TabScenario.GetRowCount() + 14;
		Rect1.SetRect(3, 3, ClientRect.right-3, Height);
		InvalidateRect(Rect1);
		}
	}

void C4ScenarioDlg::CreatePage(CDialog &Page, UINT TemplateID, bool Show)
	{
	// Seite erschaffen, wenn noch nicht vorhanden
	if (!Page.m_hWnd)
		{
		Page.Create(TemplateID, &m_TabScenario);
		// Das adjustete Rect ist bei mir an den drei Kanten immer 2px zu klein
		// Wenn das mal anders sein sollte, verkackt alles
		CRect TabRect;
		m_TabScenario.GetClientRect(TabRect);
		m_TabScenario.AdjustRect(false, TabRect);
		TabRect.left -= 2;
		TabRect.right += 2;
		TabRect.bottom += 2;
		Page.MoveWindow(&TabRect);
		}
	// Sonst nur wieder zeigen
	if (Show) 
		{
		Page.ShowWindow(SW_SHOW);
		pCurrentPage = &Page;
		}
	}

void C4ScenarioDlg::SetData() // By Init, before dialog creation
	{
	// Def page references
	PageGame.pDefs = &Defs;
	PageEnvironment.pDefs = &Defs;
	PagePlayerStart.pDefs = &Defs;
	// Page data
	PageGame.SetData(C4S);
	PageWeather.SetData(C4S);
	PageEnvironment.SetData(C4S);
	PagePlayerStart.SetData(C4S);
	PageLandscape.SetData(C4S);
	}

void C4ScenarioDlg::GetData() // By OnOK, before dialog destruction
	{
	// Get dialog data
	PageGame.GetData(C4S);
	PagePlayerStart.GetData(C4S);
	PageLandscape.GetData(C4S);
	PageWeather.GetData(C4S); // After landscape
	PageEnvironment.GetData(C4S); // After landscape
	}

int C4ScenarioDlg::DoModal() 
	{
	// Dialog modal	
	int iResult = CDialog::DoModal();
	// Ok
	if (iResult == IDOK)
		{
		// Check definition preset ...only if really checking for modules used by defs (C4ID)
		//CheckDefinitionPreset();
		// Save scenario
		CWaitCursor WaitCursor;
		SetStatus("IDS_STATUS_UPDATESCENARIO");
		if (!Save()) C4MessageBox("IDS_FAIL_SAVE");
		// Original item, assume scenario was saved to auto-copy
		if (Original) iResult = IDOK_AUTOCOPY;
		}
	// Done
	return iResult;
	}

bool C4ScenarioDlg::Save()
	{
	// Save scenario data
	C4Group hGroup;
	if (!hGroup.Open(Filename)) return false;
	if (!C4S.Save(hGroup)) return false;
	// Close group
	hGroup.Close();
	// Success
	return true;
	}

void C4ScenarioDlg::OnOK() 
	{

	// Registered only
	/*if (!GetCfg()->Registered()) FREEWARE
		{
		C4MessageBox("IDS_MSG_NOUNREGPROPSAVE");
		return;
		}*/

	// Item original
	if (Original)
		{
		// Prompt for automatic copy to work dir
		CString sText; sText.Format(LoadResStr("IDS_MSG_PROPORIGINALCOPY"),Title);
		if (C4MessageBox(sText,true)!=IDOK) return;
		// Automatic copy target filename
		char szTarget[_MAX_PATH+1],szNumber[20];
		SCopy( GetCfg()->AtExePath( GetFilename(Filename) ), szTarget );
		for (int iIndex=2; ItemExists( szTarget ); iIndex++)
			{
			SCopy( GetCfg()->AtExePath( GetFilename(Filename) ), szTarget );
			sprintf(szNumber,"%i",iIndex);
			SInsert( GetExtension(szTarget)-1, szNumber );
			}
		// Copy item
		CWaitCursor WaitCursor;
		if (! C4Group_CopyItem( Filename, szTarget )) 
			{ C4MessageBox("IDS_FAIL_MODIFY"); return; }
		// Set new filename
		Filename = szTarget;
		}

	// Get data from dialog to variables
	GetData();
	
	// Finish dialog
	CDialog::OnOK();

	}

void C4ScenarioDlg::OnSelChangeTabScenario(NMHDR* pNMHDR, LRESULT* pResult) 
	{
	*pResult = 0;

	// Alte Editseite verstecken
	HideCurrentPage();

	// Neue Editseite anzeigen
	int iItem = m_TabScenario.GetCurSel();
	TC_ITEM Item;
	Item.mask = TCIF_PARAM;
	m_TabScenario.GetItem(iItem, &Item);
	switch (Item.lParam)
		{
		case EP_Game: CreatePage(PageGame, PageGame.IDD); break;
		case EP_PlayerStart: CreatePage(PagePlayerStart, PagePlayerStart.IDD); break;
		case EP_Landscape: CreatePage(PageLandscape, PageLandscape.IDD); break;
		case EP_Weather: CreatePage(PageWeather, PageWeather.IDD); break;
		case EP_Environment: CreatePage(PageEnvironment, PageEnvironment.IDD); break;
		}

	// Gew. Seite speichern, um sie beim Aufrufen der Editseiten wieder auszuwählen
	iLastPage = iItem;

	// Teilweise neuzeichnen wegen transparentem Tabstrip
	InvalidateTabStrip(true);

	}

void C4ScenarioDlg::HideCurrentPage()
	{
	if (pCurrentPage) pCurrentPage->ShowWindow(SW_HIDE);
	pCurrentPage = NULL;
	}

void C4ScenarioDlg::CheckDefinitionPreset()
	{
	// No definitions specified in scenario core
	StdStrBuf buf;
	if (!C4S.Definitions.GetModules(&buf))
		// Not just Objects.c4d currently activated 
		if (!(SIsModule(GetCfg()->Explorer.Definitions,GetCfg()->AtExePath(C4CFN_Objects)) && (SModuleCount(GetCfg()->Explorer.Definitions)==1)))
			// Query
			if (C4MessageBox("IDS_MSG_PRESETUSEDDEFS", C4MsgBox_YesNo) == IDOK)
				// Preset currently activated defs (exe relative)
				C4S.Definitions.SetModules(GetCfg()->Explorer.Definitions,GetCfg()->General.ExePath);
	}
