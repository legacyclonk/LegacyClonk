/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Prompts the user for a single string */

#if !defined(AFX_INPUTDLG_H__8FB13584_F049_11D2_A77D_F50DF0FF2F2B__INCLUDED_)
#define AFX_INPUTDLG_H__8FB13584_F049_11D2_A77D_F50DF0FF2F2B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CInputDlg : public CDialog
{
// Construction
public:
	CInputDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CInputDlg)
	enum { IDD = IDD_INPUT };
	CEdit	m_EditInput;
	CButton	m_ButtonOK;
	CButton	m_ButtonCancel;
	CString	m_Input;
	CString	m_Prompt;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInputDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CInputDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INPUTDLG_H__8FB13584_F049_11D2_A77D_F50DF0FF2F2B__INCLUDED_)
