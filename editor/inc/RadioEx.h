/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Extends CButton as owner draw radio button with bitmap background */

#if !defined(AFX_RADIOEX_H__CF4B6563_8656_11D2_8888_0040052C10D3__INCLUDED_)
#define AFX_RADIOEX_H__CF4B6563_8656_11D2_8888_0040052C10D3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CRadioEx: public CButton
	{
	public:
		CRadioEx();

	public:
		void Invalidate();
		void Set(const char *idCaption = 0);
		void SetWindowText(LPCSTR lpszString);
		virtual ~CRadioEx();
		
		//{{AFX_VIRTUAL(CRadioEx)
		//}}AFX_VIRTUAL

		//{{AFX_MSG(CRadioEx)
		afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
		afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
		//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	
	};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RADIOEX_H__CF4B6563_8656_11D2_8888_0040052C10D3__INCLUDED_)
