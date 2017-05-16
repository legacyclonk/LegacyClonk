/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Developer mode page in options */

#include "C4Explorer.h"

#include "C4PageDeveloper.h"

IMPLEMENT_DYNCREATE(C4PageDeveloper, CPropertyPage)

BEGIN_MESSAGE_MAP(C4PageDeveloper, CPropertyPage)
	//{{AFX_MSG_MAP(C4PageDeveloper)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

C4PageDeveloper::C4PageDeveloper() : CPropertyPage(C4PageDeveloper::IDD)
	{
	m_psp.dwFlags &= ~(PSP_HASHELP);
	//{{AFX_DATA_INIT(C4PageDeveloper)
	m_DebugMode = FALSE;
	m_VerboseObjects = FALSE;
	m_SendDefReload = FALSE;
	m_AutoEditScan = FALSE;
	//}}AFX_DATA_INIT
	SetData();
	}

C4PageDeveloper::~C4PageDeveloper()
	{
	}

void C4PageDeveloper::DoDataExchange(CDataExchange* pDX)
	{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(C4PageDeveloper)
	DDX_Control(pDX, IDC_STATICFRONTEND, m_StaticFrontend);
	DDX_Control(pDX, IDC_STATICENGINE, m_StaticEngine);
	DDX_Control(pDX, IDC_CHECKAUTOEDITSCAN, m_CheckAutoEditScan);
	DDX_Control(pDX, IDC_CHECKSENDDEFRELOAD, m_CheckSendDefReload);
	DDX_Control(pDX, IDC_CHECKVERBOSEOBJECTS, m_CheckVerboseObjects);
	DDX_Control(pDX, IDC_CHECKDEBUGMODE, m_CheckDebugMode);
	DDX_Check(pDX, IDC_CHECKDEBUGMODE, m_DebugMode);
	DDX_Check(pDX, IDC_CHECKVERBOSEOBJECTS, m_VerboseObjects);
	DDX_Check(pDX, IDC_CHECKSENDDEFRELOAD, m_SendDefReload);
	DDX_Check(pDX, IDC_CHECKAUTOEDITSCAN, m_AutoEditScan);
	//}}AFX_DATA_MAP
	}

void C4PageDeveloper::SetData()
	{
	m_DebugMode = GetCfg()->General.AlwaysDebug;
	m_VerboseObjects = GetCfg()->Graphics.VerboseObjectLoading;
	m_SendDefReload = GetCfg()->Developer.AutoFileReload;
	m_AutoEditScan = GetCfg()->Developer.AutoEditScan;
	}

void C4PageDeveloper::GetData()
	{
	GetCfg()->General.AlwaysDebug = m_DebugMode;
	GetCfg()->Graphics.VerboseObjectLoading = m_VerboseObjects;
	GetCfg()->Developer.AutoFileReload = m_SendDefReload;
	GetCfg()->Developer.AutoEditScan = m_AutoEditScan;
	}

void C4PageDeveloper::UpdateText()
	{
	m_StaticFrontend.SetWindowText(LoadResStr("IDS_CTL_FRONTEND"));
	m_StaticEngine.SetWindowText(LoadResStr("IDS_CTL_ENGINE"));
	m_CheckVerboseObjects.SetWindowText(LoadResStr("IDS_CTL_VERBOSEOBJECTS"));
	m_CheckDebugMode.SetWindowText(LoadResStr("IDS_CTL_DEBUGMODE"));
	m_CheckSendDefReload.SetWindowText(LoadResStr("IDS_CTL_SENDDEFRELOAD"));
	m_CheckAutoEditScan.SetWindowText(LoadResStr("IDS_CTL_AUTOEDITSCAN"));
	}	

BOOL C4PageDeveloper::OnInitDialog() 
	{
	CPropertyPage::OnInitDialog();
	
	UpdateText();
	
	return TRUE; 
	}
