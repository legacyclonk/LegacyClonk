/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Custom message box */

#include "C4Explorer.h"
#include "C4ExplorerDlg.h"

#include "C4MessageBox.h"

C4MessageBoxDlg::C4MessageBoxDlg(CWnd* pParent /*=NULL*/)
	: CDialog(C4MessageBoxDlg::IDD, pParent)
	{
	//{{AFX_DATA_INIT(C4MessageBoxDlg)
	m_Text = _T("");
	m_Check = FALSE;
	//}}AFX_DATA_INIT
	m_Type = C4MsgBox_Ok;
	m_CheckText.Empty();
	m_Check = FALSE;
	}

void C4MessageBoxDlg::DoDataExchange(CDataExchange* pDX)
	{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(C4MessageBoxDlg)
	DDX_Control(pDX, IDC_CHECKPROMPT, m_CheckPrompt);
	DDX_Control(pDX, IDOK, m_ButtonOK);
	DDX_Control(pDX, IDCANCEL, m_ButtonCancel);
	DDX_Control(pDX, IDC_STATICTEXT, m_StaticText);
	DDX_Text(pDX, IDC_STATICTEXT, m_Text);
	DDX_Check(pDX, IDC_CHECKPROMPT, m_Check);
	//}}AFX_DATA_MAP
	}

BEGIN_MESSAGE_MAP(C4MessageBoxDlg, CDialog)
	//{{AFX_MSG_MAP(C4MessageBoxDlg)
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL C4MessageBoxDlg::OnInitDialog() 
	{
	CDialog::OnInitDialog();
	
	// Set window text
	SetWindowText(LoadResStr("IDS_DLG_EXPLORER"));

	// Set control text
	switch (m_Type)
		{
		case C4MsgBox_Ok:
			m_ButtonOK.Set("IDS_BTN_OK");
			m_ButtonCancel.ModifyStyle(WS_VISIBLE,0);
			break;
		case C4MsgBox_OkCancel:
			m_ButtonOK.Set("IDS_BTN_OK");
			m_ButtonCancel.Set("IDS_BTN_CANCEL");
		case C4MsgBox_YesNo:
			m_ButtonOK.Set("IDS_BTN_YES"); 
			m_ButtonCancel.Set("IDS_BTN_NO");
			break;
		case C4MsgBox_Retry:
			m_ButtonOK.Set("IDS_BTN_RETRY"); 
			m_ButtonCancel.Set("IDS_BTN_CANCEL");
		}

	// Enable check promt
	if (!m_CheckText.IsEmpty())
		{
		m_CheckPrompt.Set(m_CheckText);
		m_CheckPrompt.ModifyStyle(0, WS_VISIBLE);
		RECT rect; int extendY = 20;
		GetWindowRect(&rect); rect.bottom += extendY; MoveWindow(&rect);
		m_ButtonOK.GetWindowRect(&rect); ScreenToClient(&rect);	rect.top += extendY; rect.bottom += extendY; m_ButtonOK.MoveWindow(&rect);
		m_ButtonCancel.GetWindowRect(&rect); ScreenToClient(&rect);	rect.top += extendY; rect.bottom += extendY; m_ButtonCancel.MoveWindow(&rect);
		}

	// Set topmost
	SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE); 

	return TRUE; 
	}

/*BOOL C4MessageBoxDlg::OnEraseBkgnd(CDC* pDC) 
	{
	GetApp()->dibPaper.DrawTiles(pDC, this);
	return true;
	}*/

void C4MessageBoxDlg::OnOK()
	{
	// Enter with focus on cancel button: cancel (don't default to ok button)
	if (GetFocus() == &m_ButtonCancel)
		C4MessageBoxDlg::OnCancel();
	else
		CDialog::OnOK();
	}

void C4MessageBoxDlg::OnCancel()
	{
	// Always update dialog data (even if on cancel button)
	UpdateData(TRUE);
	CDialog::OnCancel();
	}




int C4MessageBox(const char *szText, int iType, const char *szCheckPrompt, BOOL *fpCheck)
	{
	C4MessageBoxDlg MessageBoxDlg;
	// Ressource string text
	if (SEqual2(szText, "IDS_"))
		MessageBoxDlg.m_Text = LoadResStr(szText);
	// Normal text
	else
		MessageBoxDlg.m_Text = szText;
	// Set type
	MessageBoxDlg.m_Type = iType;
	// Set check prompt
	if (szCheckPrompt) MessageBoxDlg.m_CheckText = szCheckPrompt;
	if (fpCheck) MessageBoxDlg.m_Check = *fpCheck;
	// Do dialog
	int iResult = MessageBoxDlg.DoModal();
	// Evaluate check
	if (fpCheck)
	{
		// This is a Yes/No box: the check value is _always_ returned
		if (iType == C4MsgBox_YesNo)
			*fpCheck = MessageBoxDlg.m_Check;
		// This is an Ok/Cancel box: the check value is only returned if the user has clicked 'Ok'
		else
			if (iResult == IDOK)
				*fpCheck = MessageBoxDlg.m_Check;
	}
	// Reset focus to main window
	CWnd *pWnd = AfxGetMainWnd();
	if (pWnd)
		pWnd->SetFocus();
	// Done
	return iResult;
	}

