/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Custom message box */

#if !defined(AFX_C4MESSAGEBOX_H__8F1F7B22_B464_11D2_8888_0040052C10D3__INCLUDED_)
#define AFX_C4MESSAGEBOX_H__8F1F7B22_B464_11D2_8888_0040052C10D3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class C4MessageBoxDlg: public CDialog
	{
	public:
		int m_Type;
		CString m_CheckText;
		C4MessageBoxDlg(CWnd* pParent = NULL);  

		//{{AFX_DATA(C4MessageBoxDlg)
	enum { IDD = IDD_MESSAGE };
	CRadioEx m_CheckPrompt;
		CButtonEx m_ButtonOK;
		CButtonEx m_ButtonCancel;
		CStaticEx m_StaticText;
		CString	m_Text;
	BOOL	m_Check;
	//}}AFX_DATA

		//{{AFX_VIRTUAL(C4MessageBoxDlg)
	public:
		virtual void OnOK();
		virtual void OnCancel();
	protected:
		virtual void DoDataExchange(CDataExchange* pDX);   
		//}}AFX_VIRTUAL

		//{{AFX_MSG(C4MessageBoxDlg)
		virtual BOOL OnInitDialog();
		//afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	};

//{{AFX_INSERT_LOCATION}}

#endif // !defined(AFX_C4MESSAGEBOX_H__8F1F7B22_B464_11D2_8888_0040052C10D3__INCLUDED_)
