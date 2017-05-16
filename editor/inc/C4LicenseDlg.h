/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Displays developer mode license */

#if !defined(AFX_C4LICENSEDLG_H__D8A54963_38C5_11D3_A77D_E064E708C4BC__INCLUDED_)
#define AFX_C4LICENSEDLG_H__D8A54963_38C5_11D3_A77D_E064E708C4BC__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class C4LicenseDlg : public CDialog
{
// Construction
public:
	C4LicenseDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(C4LicenseDlg)
	enum { IDD = IDD_LICENSE };
	CButton	m_ButtonAccept;
	CButton	m_ButtonCancel;
	CEdit	m_EditText;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(C4LicenseDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(C4LicenseDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_C4LICENSEDLG_H__D8A54963_38C5_11D3_A77D_E064E708C4BC__INCLUDED_)
