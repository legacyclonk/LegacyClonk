/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Main page in options */

#include "C4Explorer.h"
#include "C4PageGame.h"

BEGIN_MESSAGE_MAP(C4PageGame, CDialog)
	//{{AFX_MSG_MAP(C4PageGame)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_CONTEXTMENU()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


C4PageGame::C4PageGame(CWnd* pParent /*=NULL*/)
	: CDialog(C4PageGame::IDD, pParent)
	{
	//{{AFX_DATA_INIT(C4PageGame)
	m_Access = FALSE;
	m_Icon = -1;
	m_MaxPlayers = 0;
	//}}AFX_DATA_INIT
	LoadImageList(IDB_DECOGAME, ListDeco, 16, 16);
	}


void C4PageGame::DoDataExchange(CDataExchange* pDX)
	{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(C4PageGame)
	DDX_Control(pDX, IDC_BUTTONSSKIPDEFS, m_ButtonsSkipDefs);
	DDX_Control(pDX, IDC_SLIDERSKIPDEFS, m_SliderSkipDefs);
	DDX_Control(pDX, IDC_STATICSKIPDEFS, m_StaticSkipDefs);
	DDX_Control(pDX, IDC_LISTSKIPDEFS, m_ListSkipDefs);
	DDX_Control(pDX, IDC_STATICRULES, m_StaticRules);
	DDX_Control(pDX, IDC_STATICGOALS, m_StaticGoals);
	DDX_Control(pDX, IDC_SLIDERRULES, m_SliderRules);
	DDX_Control(pDX, IDC_SLIDERGOALS, m_SliderGoals);
	DDX_Control(pDX, IDC_LISTRULES, m_ListRules);
	DDX_Control(pDX, IDC_LISTGOALS, m_ListGoals);
	DDX_Control(pDX, IDC_BUTTONSRULES, m_ButtonsRules);
	DDX_Control(pDX, IDC_BUTTONSGOALS, m_ButtonsGoals);
	DDX_Control(pDX, IDC_DECOACCESS, m_DecoAccess);
	DDX_Control(pDX, IDC_STATICMAXPLAYERS, m_StaticMaxPlayers);
	DDX_Control(pDX, IDC_STATICICON, m_StaticIcon);
	DDX_Control(pDX, IDC_STATICACCESS, m_StaticAccess);
	DDX_Control(pDX, IDC_SPINMAXPLAYERS, m_SpinMaxPlayers);
	DDX_Control(pDX, IDC_EDITMAXPLAYERS, m_EditMaxPlayers);
	DDX_Control(pDX, IDC_CHECKACCESS, m_CheckAccess);
	DDX_Control(pDX, IDC_COMBOICON, m_ComboIcon);
	DDX_Check(pDX, IDC_CHECKACCESS, m_Access);
	DDX_CBIndex(pDX, IDC_COMBOICON, m_Icon);
	DDX_Text(pDX, IDC_EDITMAXPLAYERS, m_MaxPlayers);
	//}}AFX_DATA_MAP
	}

void C4PageGame::OnPaint() 
	{
	CPaintDC DC(this);
	DrawDecoPic(ListDeco, DC, m_DecoAccess, 5);
	}

BOOL C4PageGame::OnEraseBkgnd(CDC* pDC) 
	{
	GetApp()->dibPaper.DrawTiles(pDC, this);
	return true;
	}

void C4PageGame::UpdateText()
	{
	m_StaticGoals.Set("IDS_CTL_GOALS");
	m_StaticRules.Set("IDS_CTL_RULES");
	m_StaticSkipDefs.Set("IDS_CTL_SKIPDEFS");
	m_StaticMaxPlayers.Set("IDS_CTL_MAXPLAYERS");
	m_StaticIcon.Set("IDS_CTL_ICON");
	m_StaticAccess.Set("IDS_CTL_ACCESS");
	m_CheckAccess.Set("IDS_CTL_ENABLEROUNDOPTIONS");
	}

BOOL C4PageGame::OnInitDialog() 
	{
	CDialog::OnInitDialog();
	// Lists
	m_ListGoals.Init(C4D_Goal,"IDS_SELECT_GOALS",pDefs,&m_ButtonsGoals,&m_SliderGoals);
	m_ListRules.Init(C4D_Rule,"IDS_SELECT_RULES",pDefs,&m_ButtonsRules,&m_SliderRules);
	m_ListSkipDefs.Init(C4D_All,"IDS_SELECT_SKIPDEFS",pDefs,&m_ButtonsSkipDefs,&m_SliderSkipDefs);
	//m_ListGoals.EnableCounts(false);
	//m_ListRules.EnableCounts(false);
	m_ListSkipDefs.EnableCounts(false);
	// Sliders
	m_SliderGoals.Init();
	m_SliderRules.Init();
	m_SliderSkipDefs.Init();
	// Spin ranges
	m_SpinMaxPlayers.SetRange(1,12);
	// Text
	UpdateText();
	// Combo image & selection
	m_ComboIcon.Init(IDB_ICONSCENARIO);
	m_ComboIcon.SetCurSel(m_Icon);
	// Disable no-initialize controls (too bad)
	if (NoInitialize)
		{
		m_StaticGoals.EnableWindow(false);
		m_ListGoals.EnableWindow(false);
		m_ButtonsGoals.EnableWindow(false);
		m_SliderGoals.EnableWindow(false);
		m_StaticRules.EnableWindow(false);
		m_ListRules.EnableWindow(false);
		m_ButtonsRules.EnableWindow(false);
		m_SliderRules.EnableWindow(false);
		}
	return TRUE; 
	}

void C4PageGame::SetData(C4Scenario &rC4S)
	{
	m_ListGoals.Set(rC4S.Game.Goals);
	m_ListRules.Set(rC4S.Game.Rules);
	m_ListSkipDefs.Set(rC4S.Definitions.SkipDefs);
	m_Icon = rC4S.Head.Icon;
	m_MaxPlayers = rC4S.Head.MaxPlayer;
	// Store no initialize flag
	NoInitialize = (rC4S.Head.NoInitialize!=0);
	}

void C4PageGame::GetData(C4Scenario &rC4S)
	{
	UpdateData(TRUE);
	m_ListGoals.Get(rC4S.Game.Goals);
	m_ListRules.Get(rC4S.Game.Rules);
	m_ListSkipDefs.Get(rC4S.Definitions.SkipDefs);
	rC4S.Head.Icon = m_Icon;
	rC4S.Head.MaxPlayer = m_MaxPlayers;
	}

void C4PageGame::OnContextMenu(CWnd* pWnd, CPoint point) 
	{
	// Check id list hit
	if (!(pWnd == &m_ListGoals || pWnd == &m_ListRules || pWnd == &m_ListSkipDefs)) return;
	// Id list ctrl context menu
	C4IdListCtrl* pIDCtrl = (C4IdListCtrl*) pWnd;
	pIDCtrl->DoContextMenu(point);
	}
