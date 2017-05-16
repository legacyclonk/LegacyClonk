/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Prompts the user for a single string */

#include "C4Explorer.h"

#include "..\res\resource.h"

/////////////////////////////////////////////////////////////////////////////
// CInputDlg dialog


CInputDlg::CInputDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CInputDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CInputDlg)
	m_Input = _T("");
	m_Prompt = _T("");
	//}}AFX_DATA_INIT
}


void CInputDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CInputDlg)
	DDX_Control(pDX, IDC_EDITINPUT, m_EditInput);
	DDX_Control(pDX, IDOK, m_ButtonOK);
	DDX_Control(pDX, IDCANCEL, m_ButtonCancel);
	DDX_Text(pDX, IDC_EDITINPUT, m_Input);
	DDX_Text(pDX, IDC_STATICPROMPT, m_Prompt);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CInputDlg, CDialog)
	//{{AFX_MSG_MAP(CInputDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInputDlg message handlers

BOOL CInputDlg::OnInitDialog() 
	{
	CDialog::OnInitDialog();
	
	m_ButtonOK.SetWindowText(LoadResStr("IDS_BTN_OK"));
	m_ButtonCancel.SetWindowText(LoadResStr("IDS_BTN_CANCEL"));
	
	return TRUE;
	}
