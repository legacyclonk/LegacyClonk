/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Startup splash screen */

#if !defined(AFX_C4INTRODLG_H__CBDE5BA0_BF4D_11D2_8888_0040052C10D3__INCLUDED_)
#define AFX_C4INTRODLG_H__CBDE5BA0_BF4D_11D2_8888_0040052C10D3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class C4SplashDlg: public CDialog
	{
	public:
		void Hide();
		bool Show();
		C4SplashDlg(CWnd* pParent = NULL);

		//{{AFX_DATA(C4SplashDlg)
		enum { IDD = IDD_INTRO };
		//}}AFX_DATA


		//{{AFX_VIRTUAL(C4SplashDlg)
		protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		//}}AFX_VIRTUAL

	protected:
		CDIBitmap SplashBitmap;
		bool SplashAnimationAvailable;

		//{{AFX_MSG(C4SplashDlg)
		virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_C4INTRODLG_H__CBDE5BA0_BF4D_11D2_8888_0040052C10D3__INCLUDED_)
