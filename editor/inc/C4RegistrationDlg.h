/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Registration code entry dialog */

#if !defined(AFX_C4REGISTRATIONDLG_H__D8BD6242_A66A_11D2_8888_0040052C10D3__INCLUDED_)
#define AFX_C4REGISTRATIONDLG_H__D8BD6242_A66A_11D2_8888_0040052C10D3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class C4RegistrationDlg : public CDialog
	{
	public:
		C4RegistrationDlg(CWnd* pParent = NULL);   

	public:
		//{{AFX_DATA(C4RegistrationDlg)
	enum { IDD = IDD_REGISTRATION };
	CStaticEx m_StaticWebCode;
	CEdit	m_EditWebCode;
	CStaticEx m_StaticNick;
	CStaticEx m_StaticKeyFilename;
	CStaticEx m_StaticFullName;
	CStaticEx m_StaticCuid;
	CButtonEx m_FrameKeyFile;
	CEdit	m_EditNick;
	CEdit	m_EditFullName;
	CEdit	m_EditCuid;
	CButtonEx m_ButtonOK;
	//}}AFX_DATA

		//{{AFX_VIRTUAL(C4RegistrationDlg)
	protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

		//{{AFX_MSG(C4RegistrationDlg)
		//afx_msg BOOL OnEraseBkgnd(CDC* pDC);
		virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditCuid();
	afx_msg void OnChangeEditFullname();
	afx_msg void OnChangeEditNick();
	afx_msg void OnChangeEditWebCode();
	//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	protected:
		CString m_DataFullname;
		CString m_DataNick;
		CString m_DataCuid;
		CString m_DataWebCode;
	};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_C4REGISTRATIONDLG_H__D8BD6242_A66A_11D2_8888_0040052C10D3__INCLUDED_)
