/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Registration code entry dialog */

#include "C4Explorer.h"

#include "C4RegistrationDlg.h"

BEGIN_MESSAGE_MAP(C4RegistrationDlg, CDialog)
	//{{AFX_MSG_MAP(C4RegistrationDlg)
	ON_WM_ERASEBKGND()
	ON_EN_CHANGE(IDC_EDITCUID, OnChangeEditCuid)
	ON_EN_CHANGE(IDC_EDITFULLNAME, OnChangeEditFullname)
	ON_EN_CHANGE(IDC_EDITNICK, OnChangeEditNick)
	ON_EN_CHANGE(IDC_EDITWEBCODE, OnChangeEditWebCode)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


C4RegistrationDlg::C4RegistrationDlg(CWnd* pParent /*=NULL*/)
	: CDialog(C4RegistrationDlg::IDD, pParent)
	{
	//{{AFX_DATA_INIT(C4RegistrationDlg)
	//}}AFX_DATA_INIT
	}


void C4RegistrationDlg::DoDataExchange(CDataExchange* pDX)
	{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(C4RegistrationDlg)
	DDX_Control(pDX, IDC_STATICWEBCODE, m_StaticWebCode);
	DDX_Control(pDX, IDC_EDITWEBCODE, m_EditWebCode);
	DDX_Control(pDX, IDC_STATICNICK, m_StaticNick);
	DDX_Control(pDX, IDC_STATICKEYFILENAME, m_StaticKeyFilename);
	DDX_Control(pDX, IDC_STATICFULLNAME, m_StaticFullName);
	DDX_Control(pDX, IDC_STATICCUID, m_StaticCuid);
	DDX_Control(pDX, IDC_FRAMEKEYFILE, m_FrameKeyFile);
	DDX_Control(pDX, IDC_EDITNICK, m_EditNick);
	DDX_Control(pDX, IDC_EDITFULLNAME, m_EditFullName);
	DDX_Control(pDX, IDC_EDITCUID, m_EditCuid);
	DDX_Control(pDX, IDOK, m_ButtonOK);
	//}}AFX_DATA_MAP
	}

BOOL C4RegistrationDlg::OnInitDialog() 
	{
	CDialog::OnInitDialog();

	// Text
	SetWindowText(LoadResStr("IDS_DLG_REGISTRATION"));
	m_ButtonOK.Set("IDS_BTN_OK");
	m_StaticCuid.Set("IDS_CTL_CUID");
	m_StaticWebCode.Set("IDS_CTL_WEBCODE");
	m_StaticNick.Set("IDS_CTL_NICK");
	m_StaticFullName.Set("IDS_CTL_FULLNAME");

	// Clear variables
	m_DataCuid.Empty();
	m_DataNick.Empty();
	m_DataFullname.Empty();
	m_DataWebCode.Empty();

	// Enable controls
	m_EditCuid.EnableWindow(GetCfg()->Registered());
	m_EditWebCode.EnableWindow(GetCfg()->Registered());
	m_EditNick.EnableWindow(GetCfg()->Registered());
	m_EditFullName.EnableWindow(GetCfg()->Registered());

	// Registered: set info
	if (GetCfg()->Registered())
		{
		m_DataCuid = GetCfg()->GetRegistrationData("Cuid");
		m_DataWebCode = GetCfg()->GetRegistrationData("WebCode");
		m_DataNick = GetCfg()->GetRegistrationData("Nick");
		m_DataFullname = GetCfg()->General.Name;
		m_EditCuid.SetWindowText(m_DataCuid);
		m_EditNick.SetWindowText(m_DataNick);
		m_EditWebCode.SetWindowText(m_DataWebCode);
		m_EditFullName.SetWindowText(m_DataFullname);
		m_StaticKeyFilename.Set(CString(LoadResStr("IDS_CTL_KEYFILE")) + " " + CString(GetCfg()->GetKeyFilename()));
		}
	// Not registered
	else
		m_StaticKeyFilename.Set(GetCfg()->GetRegistrationError());

	return TRUE;  
	}

void C4RegistrationDlg::OnChangeEditCuid() 
{
	// Beware of SetWindowText -> OnChangeEditCuid recursion (Windows XP only)
	static bool fEditing = false;
	if (fEditing) return;
	fEditing = true;
	// Undo any editing
	m_EditCuid.SetWindowText(m_DataCuid);
	// Done editing
	fEditing = false;
}

void C4RegistrationDlg::OnChangeEditFullname() 
{
	// Beware of SetWindowText -> OnChangeEditCuid recursion (Windows XP only)
	static bool fEditing = false;
	if (fEditing) return;
	fEditing = true;
	// Undo any editing
	m_EditFullName.SetWindowText(m_DataFullname);
	// Done editing
	fEditing = false;
}

void C4RegistrationDlg::OnChangeEditNick() 
{
	// Beware of SetWindowText -> OnChangeEditCuid recursion (Windows XP only)
	static bool fEditing = false;
	if (fEditing) return;
	fEditing = true;
	// Undo any editing
	m_EditNick.SetWindowText(m_DataNick);
	// Done editing
	fEditing = false;
}

void C4RegistrationDlg::OnChangeEditWebCode() 
{
	// Beware of SetWindowText -> OnChangeEditCuid recursion (Windows XP only)
	static bool fEditing = false;
	if (fEditing) return;
	fEditing = true;
	// Undo any editing
	m_EditWebCode.SetWindowText(m_DataWebCode);
	// Done editing
	fEditing = false;
}
