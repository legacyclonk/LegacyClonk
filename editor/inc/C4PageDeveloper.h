/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Developer mode page in options */

#if !defined(AFX_C4PAGEDEVELOPER_H__65FB0FA2_20B8_11D3_8ED0_0040052C10D3__INCLUDED_)
#define AFX_C4PAGEDEVELOPER_H__65FB0FA2_20B8_11D3_8ED0_0040052C10D3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class C4PageDeveloper : public CPropertyPage
{
	DECLARE_DYNCREATE(C4PageDeveloper)

public:
	void GetData();
	void SetData();
	C4PageDeveloper();
	~C4PageDeveloper();

	//{{AFX_DATA(C4PageDeveloper)
	enum { IDD = IDD_PAGEDEVELOPER };
	CButton	m_StaticFrontend;
	CButton	m_StaticEngine;
	CButton	m_CheckAutoEditScan;
	CButton	m_CheckSendDefReload;
	CButton	m_CheckVerboseObjects;
	CButton	m_CheckDebugMode;
	BOOL	m_DebugMode;
	BOOL	m_VerboseObjects;
	BOOL	m_SendDefReload;
	BOOL	m_AutoEditScan;
	//}}AFX_DATA


	//{{AFX_VIRTUAL(C4PageDeveloper)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:
	void UpdateText();
	// Generated message map functions
	//{{AFX_MSG(C4PageDeveloper)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_C4PAGEDEVELOPER_H__65FB0FA2_20B8_11D3_8ED0_0040052C10D3__INCLUDED_)
