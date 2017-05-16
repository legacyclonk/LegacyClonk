/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Displays a single info bitmap */

#if !defined(AFX_C4ABOUTDLG_H__C3338AE0_A411_11D2_8888_0040052C10D3__INCLUDED_)
#define AFX_C4ABOUTDLG_H__C3338AE0_A411_11D2_8888_0040052C10D3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class C4AboutDlg: public CDialog
	{
	public:
		C4AboutDlg(CWnd* pParent = NULL);

	protected:
		CDIBitmap dibSplash;
		CString m_Version;
		CRect m_RectVersion;

	protected:
		//{{AFX_DATA(C4AboutDlg)
		enum { IDD = IDD_ABOUT };
		CStatic	m_StaticSplash;
		//}}AFX_DATA

		//{{AFX_VIRTUAL(C4AboutDlg)
		virtual void DoDataExchange(CDataExchange* pDX);
		virtual BOOL OnInitDialog();
		//}}AFX_VIRTUAL

		//{{AFX_MSG(C4AboutDlg)
		afx_msg BOOL OnEraseBkgnd(CDC* pDC);
		afx_msg void OnPaint();
		afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
		//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	};

//{{AFX_INSERT_LOCATION}}

#endif // !defined(AFX_C4ABOUTDLG_H__C3338AE0_A411_11D2_8888_0040052C10D3__INCLUDED_)
