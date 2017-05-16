/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* External editor programs tab in options */

#include "C4Explorer.h"

#include "C4PageEditor.h"

IMPLEMENT_DYNCREATE(C4PageEditor, CPropertyPage)

C4PageEditor::C4PageEditor() : CPropertyPage(C4PageEditor::IDD)
	{
	m_psp.dwFlags &= ~(PSP_HASHELP);
	//{{AFX_DATA_INIT(C4PageEditor)
	m_EditorBitmap = _T("");
	m_EditorMusic = _T("");
	m_EditorRichText = _T("");
	m_EditorScript = _T("");
	m_EditorSound = _T("");
	m_EditorText = _T("");
	m_EditorZip = _T("");
	m_UseShell = FALSE;
	m_EditorPNG = _T("");
	//}}AFX_DATA_INIT
	SetData();
	}

C4PageEditor::~C4PageEditor()
	{
	}

void C4PageEditor::DoDataExchange(CDataExchange* pDX)
	{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(C4PageEditor)
	DDX_Control(pDX, IDC_STATICPNG, m_StaticPNG);
	DDX_Control(pDX, IDC_EDITEDITORPNG, m_EditEditorPNG);
	DDX_Control(pDX, IDC_BUTTONEDITORPNG, m_ButtonEditorPNG);
	DDX_Control(pDX, IDC_EDITEDITORZIP, m_EditEditorZip);
	DDX_Control(pDX, IDC_EDITEDITORTEXT, m_EditEditorText);
	DDX_Control(pDX, IDC_EDITEDITORSOUND, m_EditEditorSound);
	DDX_Control(pDX, IDC_EDITEDITORSCRIPT, m_EditEditorScript);
	DDX_Control(pDX, IDC_EDITEDITORRICHTEXT, m_EditEditorRichText);
	DDX_Control(pDX, IDC_EDITEDITORMUSIC, m_EditEditorMusic);
	DDX_Control(pDX, IDC_EDITEDITORBITMAP, m_EditEditorBitmap);
	DDX_Control(pDX, IDC_CHECKUSESHELL, m_CheckUseShell);
	DDX_Control(pDX, IDC_STATICZIP, m_StaticZip);
	DDX_Control(pDX, IDC_BUTTONEDITORZIP, m_ButtonEditorZip);
	DDX_Control(pDX, IDC_STATICTEXT, m_StaticText);
	DDX_Control(pDX, IDC_STATICSOUND, m_StaticSound);
	DDX_Control(pDX, IDC_STATICSCRIPT, m_StaticScript);
	DDX_Control(pDX, IDC_STATICRICHTEXT, m_StaticRichText);
	DDX_Control(pDX, IDC_STATICMUSIC, m_StaticMusic);
	DDX_Control(pDX, IDC_STATICBITMAP, m_StaticBitmap);
	DDX_Control(pDX, IDC_BUTTONEDITORTEXT, m_ButtonEditorText);
	DDX_Control(pDX, IDC_BUTTONEDITORSOUND, m_ButtonEditorSound);
	DDX_Control(pDX, IDC_BUTTONEDITORSCRIPT, m_ButtonEditorScript);
	DDX_Control(pDX, IDC_BUTTONEDITORRICHTEXT, m_ButtonEditorRichText);
	DDX_Control(pDX, IDC_BUTTONEDITORMUSIC, m_ButtonEditorMusic);
	DDX_Control(pDX, IDC_BUTTONEDITORBITMAP, m_ButtonEditorBitmap);
	DDX_Text(pDX, IDC_EDITEDITORBITMAP, m_EditorBitmap);
	DDX_Text(pDX, IDC_EDITEDITORMUSIC, m_EditorMusic);
	DDX_Text(pDX, IDC_EDITEDITORRICHTEXT, m_EditorRichText);
	DDX_Text(pDX, IDC_EDITEDITORSCRIPT, m_EditorScript);
	DDX_Text(pDX, IDC_EDITEDITORSOUND, m_EditorSound);
	DDX_Text(pDX, IDC_EDITEDITORTEXT, m_EditorText);
	DDX_Text(pDX, IDC_EDITEDITORZIP, m_EditorZip);
	DDX_Check(pDX, IDC_CHECKUSESHELL, m_UseShell);
	DDX_Text(pDX, IDC_EDITEDITORPNG, m_EditorPNG);
	//}}AFX_DATA_MAP
	}


BEGIN_MESSAGE_MAP(C4PageEditor, CPropertyPage)
	//{{AFX_MSG_MAP(C4PageEditor)
	ON_BN_CLICKED(IDC_BUTTONEDITORBITMAP, OnButtonEditorBitmap)
	ON_BN_CLICKED(IDC_BUTTONEDITORMUSIC, OnButtonEditorMusic)
	ON_BN_CLICKED(IDC_BUTTONEDITORRICHTEXT, OnButtonEditorRichText)
	ON_BN_CLICKED(IDC_BUTTONEDITORSCRIPT, OnButtonEditorScript)
	ON_BN_CLICKED(IDC_BUTTONEDITORSOUND, OnButtonEditorSound)
	ON_BN_CLICKED(IDC_BUTTONEDITORTEXT, OnButtonEditorText)
	ON_BN_CLICKED(IDC_BUTTONEDITORZIP, OnButtonEditorZip)
	ON_BN_CLICKED(IDC_CHECKUSESHELL, OnCheckUseShell)
	ON_BN_CLICKED(IDC_BUTTONEDITORPNG, OnButtonEditorPNG)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL C4PageEditor::OnInitDialog() 
	{
	CPropertyPage::OnInitDialog();

	m_CheckUseShell.SetWindowText(LoadResStr("IDS_CTL_USESHELL"));

	m_StaticText.SetWindowText(LoadResStr("IDS_CTL_TEXT"));
	m_StaticRichText.SetWindowText(LoadResStr("IDS_CTL_RICHTEXT"));
	m_StaticScript.SetWindowText(LoadResStr("IDS_CTL_SCRIPT"));
	m_StaticBitmap.SetWindowText(LoadResStr("IDS_CTL_BITMAP"));
	m_StaticPNG.SetWindowText(LoadResStr("IDS_CTL_BITMAP_PNG"));
	m_StaticSound.SetWindowText(LoadResStr("IDS_CTL_SOUND"));
	m_StaticMusic.SetWindowText(LoadResStr("IDS_CTL_MUSIC"));
	m_StaticZip.SetWindowText(LoadResStr("IDS_CTL_ZIP"));
	
	m_ButtonEditorText.SetWindowText(LoadResStr("IDS_BTN_BROWSE"));
	m_ButtonEditorRichText.SetWindowText(LoadResStr("IDS_BTN_BROWSE"));
	m_ButtonEditorScript.SetWindowText(LoadResStr("IDS_BTN_BROWSE"));
	m_ButtonEditorBitmap.SetWindowText(LoadResStr("IDS_BTN_BROWSE"));
	m_ButtonEditorPNG.SetWindowText(LoadResStr("IDS_BTN_BROWSE"));
	m_ButtonEditorSound.SetWindowText(LoadResStr("IDS_BTN_BROWSE"));
	m_ButtonEditorMusic.SetWindowText(LoadResStr("IDS_BTN_BROWSE"));
	m_ButtonEditorZip.SetWindowText(LoadResStr("IDS_BTN_BROWSE"));

	EnableControls(!m_UseShell);

	return TRUE;
	}

void C4PageEditor::GetData()
	{
	SCopy(m_EditorText,GetCfg()->Explorer.EditorText,CFG_MaxString);
	SCopy(m_EditorRichText,GetCfg()->Explorer.EditorRichText,CFG_MaxString);
	SCopy(m_EditorScript,GetCfg()->Explorer.EditorScript,CFG_MaxString);
	SCopy(m_EditorBitmap,GetCfg()->Explorer.EditorBitmap,CFG_MaxString);
	SCopy(m_EditorPNG,GetCfg()->Explorer.EditorPNG,CFG_MaxString);
	SCopy(m_EditorSound,GetCfg()->Explorer.EditorSound,CFG_MaxString);
	SCopy(m_EditorMusic,GetCfg()->Explorer.EditorMusic,CFG_MaxString);
	SCopy(m_EditorZip,GetCfg()->Explorer.EditorZip,CFG_MaxString);
	GetCfg()->Explorer.EditorUseShell = m_UseShell;
	}

void C4PageEditor::SetData()
	{
	m_EditorText = GetCfg()->Explorer.EditorText;
	m_EditorRichText = GetCfg()->Explorer.EditorRichText;
	m_EditorScript = GetCfg()->Explorer.EditorScript;
	m_EditorBitmap = GetCfg()->Explorer.EditorBitmap;
	m_EditorPNG = GetCfg()->Explorer.EditorPNG;
	m_EditorSound = GetCfg()->Explorer.EditorSound;
	m_EditorMusic = GetCfg()->Explorer.EditorMusic;
	m_EditorZip = GetCfg()->Explorer.EditorZip;
	m_UseShell = GetCfg()->Explorer.EditorUseShell;
	}

void C4PageEditor::OnButtonEditorBitmap() 
	{
	CString Filename = OpenFileDlg(LoadResStr("IDS_FN_EXECUTABLES"),m_EditorBitmap);
	if (!Filename.IsEmpty()) { m_EditorBitmap = Filename;	UpdateData(FALSE);}
	}

void C4PageEditor::OnButtonEditorPNG() 
	{
	CString Filename = OpenFileDlg(LoadResStr("IDS_FN_EXECUTABLES"),m_EditorPNG);
	if (!Filename.IsEmpty()) { m_EditorPNG = Filename;	UpdateData(FALSE);}
	}

void C4PageEditor::OnButtonEditorMusic() 
	{
	CString Filename = OpenFileDlg(LoadResStr("IDS_FN_EXECUTABLES"),m_EditorMusic);
	if (!Filename.IsEmpty()) { m_EditorMusic = Filename;	UpdateData(FALSE);}
	}

void C4PageEditor::OnButtonEditorRichText() 
	{
	CString Filename = OpenFileDlg(LoadResStr("IDS_FN_EXECUTABLES"),m_EditorRichText);
	if (!Filename.IsEmpty()) { m_EditorRichText = Filename;	UpdateData(FALSE);}
	}

void C4PageEditor::OnButtonEditorScript() 
	{
	CString Filename = OpenFileDlg(LoadResStr("IDS_FN_EXECUTABLES"),m_EditorScript);
	if (!Filename.IsEmpty()) { m_EditorScript = Filename;	UpdateData(FALSE);}
	}

void C4PageEditor::OnButtonEditorSound() 
	{
	CString Filename = OpenFileDlg(LoadResStr("IDS_FN_EXECUTABLES"),m_EditorSound);
	if (!Filename.IsEmpty()) { m_EditorSound = Filename; UpdateData(FALSE);}
	}

void C4PageEditor::OnButtonEditorText() 
	{
	CString Filename = OpenFileDlg(LoadResStr("IDS_FN_EXECUTABLES"),m_EditorText);
	if (!Filename.IsEmpty()) { m_EditorText = Filename;	UpdateData(FALSE);}
	}

void C4PageEditor::OnButtonEditorZip() 
	{
	CString Filename = OpenFileDlg(LoadResStr("IDS_FN_EXECUTABLES"),m_EditorZip);
	if (!Filename.IsEmpty()) { m_EditorZip = Filename;	UpdateData(FALSE);}
	}

void C4PageEditor::EnableControls(bool fEnable)
	{
	m_EditEditorZip.EnableWindow(fEnable);
	m_EditEditorText.EnableWindow(fEnable);
	m_EditEditorSound.EnableWindow(fEnable);
	m_EditEditorScript.EnableWindow(fEnable);
	m_EditEditorRichText.EnableWindow(fEnable);
	m_EditEditorMusic.EnableWindow(fEnable);
	m_EditEditorBitmap.EnableWindow(fEnable);
	m_EditEditorPNG.EnableWindow(fEnable);
	m_StaticZip.EnableWindow(fEnable);
	m_ButtonEditorZip.EnableWindow(fEnable);
	m_StaticText.EnableWindow(fEnable);
	m_StaticSound.EnableWindow(fEnable);
	m_StaticScript.EnableWindow(fEnable);
	m_StaticRichText.EnableWindow(fEnable);
	m_StaticMusic.EnableWindow(fEnable);
	m_StaticBitmap.EnableWindow(fEnable);
	m_StaticPNG.EnableWindow(fEnable);
	m_ButtonEditorText.EnableWindow(fEnable);
	m_ButtonEditorSound.EnableWindow(fEnable);
	m_ButtonEditorScript.EnableWindow(fEnable);
	m_ButtonEditorRichText.EnableWindow(fEnable);
	m_ButtonEditorMusic.EnableWindow(fEnable);
	m_ButtonEditorBitmap.EnableWindow(fEnable);
	m_ButtonEditorPNG.EnableWindow(fEnable);
	}

void C4PageEditor::OnCheckUseShell() 
	{
	m_UseShell = (m_CheckUseShell.GetCheck()==TRUE);
	EnableControls(!m_UseShell);
	}

