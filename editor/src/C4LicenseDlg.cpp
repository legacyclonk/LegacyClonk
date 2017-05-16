/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Displays developer mode license */

#include "C4Explorer.h"

#include "C4LicenseDlg.h"

BEGIN_MESSAGE_MAP(C4LicenseDlg, CDialog)
	//{{AFX_MSG_MAP(C4LicenseDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


C4LicenseDlg::C4LicenseDlg(CWnd* pParent /*=NULL*/)
	: CDialog(C4LicenseDlg::IDD, pParent)
	{
	//{{AFX_DATA_INIT(C4LicenseDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	}


void C4LicenseDlg::DoDataExchange(CDataExchange* pDX)
	{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(C4LicenseDlg)
	DDX_Control(pDX, IDOK, m_ButtonAccept);
	DDX_Control(pDX, IDCANCEL, m_ButtonCancel);
	DDX_Control(pDX, IDC_EDITTEXT, m_EditText);
	//}}AFX_DATA_MAP
	}


BOOL C4LicenseDlg::OnInitDialog() 
	{
	CDialog::OnInitDialog();
	
	SetWindowText(LoadResStr("IDS_DLG_LICENSE"));
	m_ButtonAccept.SetWindowText(LoadResStr("IDS_BTN_ACCEPT"));
	m_ButtonCancel.SetWindowText(LoadResStr("IDS_BTN_CANCEL"));

	m_EditText.SetWindowText(LoadResStr("IDS_DEV_LICENSE"));
	
	return TRUE;
	}
