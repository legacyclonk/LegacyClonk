/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Sound page in options */

#include "C4Explorer.h"

#include "C4PageSound.h"

IMPLEMENT_DYNCREATE(C4PageSound, CPropertyPage)

BEGIN_MESSAGE_MAP(C4PageSound, CPropertyPage)
	//{{AFX_MSG_MAP(C4PageSound)
	ON_BN_CLICKED(IDC_CHECKGAMEFX, OnCheckGameEffects)
	ON_BN_CLICKED(IDC_BUTTONVOLUME, OnButtonVolume)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

C4PageSound::C4PageSound() : CPropertyPage(C4PageSound::IDD)
	{
	m_psp.dwFlags &= ~(PSP_HASHELP);
	//{{AFX_DATA_INIT(C4PageSound)
	m_ExplorerFX = FALSE;
	m_ExplorerMusic = FALSE;
	m_GameFX = FALSE;
	m_GameMusic = FALSE;
	//}}AFX_DATA_INIT
	SetData();
	}

C4PageSound::~C4PageSound()
	{
	}

void C4PageSound::DoDataExchange(CDataExchange* pDX)
	{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(C4PageSound)
	DDX_Control(pDX, IDC_STATICVOLUME, m_StaticVolume);
	DDX_Control(pDX, IDC_BUTTONVOLUME, m_ButtonVolume);
	DDX_Control(pDX, IDC_STATICSOUNDGAME, m_StaticGame);
	DDX_Control(pDX, IDC_STATICSOUNDEXPLORER, m_StaticExplorer);
	DDX_Control(pDX, IDC_CHECKGAMEMUSIC, m_CheckGameMusic);
	DDX_Control(pDX, IDC_CHECKGAMEFX, m_CheckGameFX);
	DDX_Control(pDX, IDC_CHECKEXPLORERMUSIC, m_CheckExplorerMusic);
	DDX_Control(pDX, IDC_CHECKEXPLORERFX, m_CheckExplorerFX);
	DDX_Check(pDX, IDC_CHECKEXPLORERFX, m_ExplorerFX);
	DDX_Check(pDX, IDC_CHECKEXPLORERMUSIC, m_ExplorerMusic);
	DDX_Check(pDX, IDC_CHECKGAMEFX, m_GameFX);
	DDX_Check(pDX, IDC_CHECKGAMEMUSIC, m_GameMusic);
	//}}AFX_DATA_MAP
	}

void C4PageSound::GetData()
	{
	GetCfg()->Sound.RXSound = m_GameFX;
	GetCfg()->Sound.RXMusic = m_GameMusic;
	GetCfg()->Sound.FEMusic = m_ExplorerMusic;
	GetCfg()->Sound.FESamples = m_ExplorerFX;
	}

void C4PageSound::SetData()
	{
	m_GameFX				= GetCfg()->Sound.RXSound;
	m_GameMusic			= GetCfg()->Sound.RXMusic;
	m_ExplorerMusic = GetCfg()->Sound.FEMusic;
	m_ExplorerFX		= GetCfg()->Sound.FESamples;
	}

void C4PageSound::UpdateText()
	{
	m_StaticGame.SetWindowText(LoadResStr("IDS_CTL_GAME"));
	m_StaticExplorer.SetWindowText(LoadResStr("IDS_CTL_MENUSYSTEM"));
	m_StaticVolume.SetWindowText(LoadResStr("IDS_CTL_VOLUME"));
	m_CheckGameMusic.SetWindowText(LoadResStr("IDS_CTL_MUSIC"));
	m_CheckGameFX.SetWindowText(LoadResStr("IDS_CTL_SOUNDFX"));
	m_CheckExplorerMusic.SetWindowText(LoadResStr("IDS_CTL_MUSIC"));
	m_CheckExplorerFX.SetWindowText(LoadResStr("IDS_CTL_SOUNDFX"));
	m_ButtonVolume.SetWindowText(LoadResStr("IDS_BTN_VOLUME"));
	}

BOOL C4PageSound::OnInitDialog() 
	{
	CPropertyPage::OnInitDialog();
	UpdateText();
	OnCheckGameEffects();
	return TRUE;
	}

void C4PageSound::OnCheckGameEffects() 
	{

	}

void C4PageSound::OnButtonVolume() 
	{
	ShellExecute(NULL, NULL, "sndvol32.exe", NULL, NULL, SW_SHOWNORMAL);		
	}
