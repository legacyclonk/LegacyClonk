/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Scenario definition preset dialog */

#include "C4Explorer.h"

#include "C4ScenarioDefinitionsDlg.h"

BEGIN_MESSAGE_MAP(C4ScenarioDefinitionsDlg, CDialog)
	//{{AFX_MSG_MAP(C4ScenarioDefinitionsDlg)
	ON_WM_ERASEBKGND()
	ON_BN_CLICKED(IDC_RADIODEFBYCHOICE, OnRadioDefByChoice)
	ON_BN_CLICKED(IDC_RADIOPREDEFINEDDEFS, OnRadioSpecifiedDefs)
	ON_BN_CLICKED(IDC_RADIOLOCALONLY, OnRadioLocalOnly)
	ON_BN_CLICKED(IDC_BUTTONUSEACTIVATED, OnButtonUseActivated)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


C4ScenarioDefinitionsDlg::C4ScenarioDefinitionsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(C4ScenarioDefinitionsDlg::IDD, pParent)
	{
	//{{AFX_DATA_INIT(C4ScenarioDefinitionsDlg)
	m_Selection = -1;
	//}}AFX_DATA_INIT
	}

void C4ScenarioDefinitionsDlg::DoDataExchange(CDataExchange* pDX)
	{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(C4ScenarioDefinitionsDlg)
	DDX_Control(pDX, IDOK, m_ButtonOK);
	DDX_Control(pDX, IDCANCEL, m_ButtonCancel);
	DDX_Control(pDX, IDC_STATICSCENARIOUSES, m_StaticScenarioUses);
	DDX_Control(pDX, IDC_RADIODEFBYCHOICE, m_RadioDefByChoice);
	DDX_Control(pDX, IDC_RADIOLOCALONLY, m_RadioLocalOnly);
	DDX_Control(pDX, IDC_RADIOPREDEFINEDDEFS, m_RadioSpecifiedDefs);
	DDX_Control(pDX, IDC_LISTSPECIFIEDDEFS, m_ListDefinitions);
	DDX_Control(pDX, IDC_BUTTONUSEACTIVATED, m_ButtonUseActivated);
	DDX_Radio(pDX, IDC_RADIODEFBYCHOICE, m_Selection);
	//}}AFX_DATA_MAP
	}

BOOL C4ScenarioDefinitionsDlg::OnInitDialog() 
	{
	CDialog::OnInitDialog();
	
	// Init definition module list
	LoadImageList(IDB_ITEMSTATE,StateList,16,16);	
	m_ListDefinitions.SetImageList(&StateList,TVSIL_STATE);
	CRect Rect; m_ListDefinitions.GetClientRect(&Rect);
	m_ListDefinitions.InsertColumn(0,"Modul",LVCFMT_LEFT,Rect.Width());

	// Set dialog data
	SetData();

	// Enable controls
	EnableControls();

	// Text
	UpdateText();
	
	return TRUE; 
	}

void C4ScenarioDefinitionsDlg::UpdateText()
	{
	// Caption
	CString sCaption = LoadResStr("IDS_DLG_DEFINITIONS"); sCaption += " - "; sCaption += Title;
	SetWindowText( sCaption );
	// Control
	m_ButtonUseActivated.Set("IDS_BTN_USEACTIVATED");
	m_ButtonOK.Set("IDS_BTN_OK");
	m_ButtonCancel.Set("IDS_BTN_CANCEL");
	m_StaticScenarioUses.Set("IDS_CTL_SCENARIOUSES");
	m_RadioDefByChoice.Set("IDS_CTL_DEFBYCHOICE");
	m_RadioLocalOnly.Set("IDS_CTL_LOCALONLY");
	m_RadioSpecifiedDefs.Set("IDS_CTL_SPECIFIEDDEFS");
	}

bool C4ScenarioDefinitionsDlg::Init(const char *szFilename, bool fOriginal, bool fDeveloper)
	{
	CWaitCursor WaitCursor;

	// Store item name
	Filename = szFilename;
	Original = fOriginal;
	Developer = fDeveloper;

	// Open scenario file
	C4Group hGroup;
	if (!hGroup.Open(Filename)) return false;

	// Load core
	if (!C4S.Load(hGroup)) return false;

	// Load title
	SCopy(C4S.Head.Title, Title, C4MaxTitle);
	C4ComponentHost TitleHost;
	if (TitleHost.LoadEx("Title", hGroup, C4CFN_Title, GetCfg()->General.LanguageEx))	
	{
		StdStrBuf buf;
		TitleHost.GetLanguageString(GetCfg()->General.LanguageEx, buf);
		SCopy(buf.getData(), Title, C4MaxTitle);
	}

	// Close scenario file
	hGroup.Close();

	return true;
	}

int C4ScenarioDefinitionsDlg::DoModal() 
	{
	// Dialog modal	
	int iResult = CDialog::DoModal();
	// Ok
	if (iResult == IDOK)
		{
		// Save scenario
		CWaitCursor WaitCursor;
		SetStatus("IDS_STATUS_UPDATESCENARIO");
		if (!Save()) C4MessageBox("IDS_FAIL_SAVE");
		}
	// Done
	return iResult;
	}

void C4ScenarioDefinitionsDlg::SetData() // Dialog runtime only
	{
	// None specified
	m_Selection = 0;
	// Check specified
	int iItem = 0;
	for (int cnt=0; cnt<C4S_MaxDefinitions; cnt++)
		if (C4S.Definitions.Definition[cnt][0])
			{
			InsertDefinitionModule(iItem++,C4S.Definitions.Definition[cnt]);
			m_Selection = 2;
			}
	// Local only
	if (C4S.Definitions.LocalOnly) m_Selection = 1;
	// Update controls
	UpdateData(FALSE);
	}

void C4ScenarioDefinitionsDlg::GetData() // Dialog runtime only
	{
	UpdateData(TRUE);
	switch (m_Selection)
		{
		case 0: // Local and activated: clear all specifications
			C4S.Definitions.Default();
			break;
		case 1: // Local only
			C4S.Definitions.LocalOnly = 1;
			break;
		case 2: // Local and specified
			C4S.Definitions.Default();
			for (int cnt=0; cnt<C4S_MaxDefinitions; cnt++)
				m_ListDefinitions.GetItemText(cnt,0,C4S.Definitions.Definition[cnt],_MAX_PATH);			
			break;
		}
	}

void C4ScenarioDefinitionsDlg::EnableControls()
	{
	UpdateData(TRUE);
	m_ListDefinitions.EnableWindow(m_Selection == 2);
	m_ButtonUseActivated.EnableWindow(m_Selection == 2);

	//if (!Developer) m_ButtonUseActivated.ModifyStyle(WS_VISIBLE,0);

	}

bool C4ScenarioDefinitionsDlg::Save()
	{
	// Save scenario data
	C4Group hGroup;
	if (!hGroup.Open(Filename)) return false;
	if (!C4S.Save(hGroup)) return false;
	hGroup.Close();
	// Success
	return true;
	}

void C4ScenarioDefinitionsDlg::OnOK() 
	{

	// Registered only
	/*if (!GetCfg()->Registered()) FREEWARE
		{	C4MessageBox("IDS_MSG_NOUNREGPROPSAVE"); return; }*/

	// Item original
	if (Original)
		{
		CString sText; sText.Format(LoadResStr("IDS_MSG_ITEMORIGINAL"),Title);
		C4MessageBox(sText); return;
		}

	// Get data
	GetData();
	
	// Finish dialog
	CDialog::OnOK();

	}

void C4ScenarioDefinitionsDlg::OnRadioDefByChoice()
	{
	EnableControls();
	}

void C4ScenarioDefinitionsDlg::OnRadioSpecifiedDefs()
	{
	EnableControls();
	}

void C4ScenarioDefinitionsDlg::OnRadioLocalOnly()
	{
	EnableControls();
	}

void C4ScenarioDefinitionsDlg::OnButtonUseActivated()
	{
	// Set specified by current configuration
	// store filenames AtExeRelativePath
	// (Objects.c4d is always activated)
	int iItem = 0;
	char szModule[_MAX_PATH+1];
	m_ListDefinitions.DeleteAllItems();
	for (int cnt=0; SGetModule(GetCfg()->Explorer.Definitions,cnt,szModule,_MAX_PATH); cnt++)
		InsertDefinitionModule(iItem++,szModule);
	}

void C4ScenarioDefinitionsDlg::InsertDefinitionModule(int iItem, const char *szFilename)
	{
	if (!szFilename[0]) return;
	bool fAvailable = C4Group_IsGroup( GetCfg()->AtExeRelativePath(szFilename) ) != 0;
	m_ListDefinitions.InsertItem(iItem,GetCfg()->AtExeRelativePath(szFilename));
	m_ListDefinitions.SetItemState(iItem,INDEXTOSTATEIMAGEMASK(fAvailable ? 7 : 6),TVIS_STATEIMAGEMASK);
	}

/*BOOL C4ScenarioDefinitionsDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
	{
	GetApp()->WinHelp(pHelpInfo->iCtrlId, HELP_CONTEXTPOPUP);
	return true;
	}*/
