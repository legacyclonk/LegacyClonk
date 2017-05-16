/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

#if !defined(AFX_STATICEX_H__CF4B6562_8656_11D2_8888_0040052C10D3__INCLUDED_)
#define AFX_STATICEX_H__CF4B6562_8656_11D2_8888_0040052C10D3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CStaticEx: public CStatic
	{
	public:
		CStaticEx();

		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CStaticEx)
		//}}AFX_VIRTUAL

	public:
		void Set(const char *szText = 0);
		void Invalidate();
		void SetWindowText(LPCSTR lpszString);
		virtual ~CStaticEx();

	protected:
		//{{AFX_MSG(CStaticEx)
		afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
		//}}AFX_MSG

		DECLARE_MESSAGE_MAP()
	};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STATICEX_H__CF4B6562_8656_11D2_8888_0040052C10D3__INCLUDED_)
