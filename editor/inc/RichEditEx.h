/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Overrides CtlColor on CRichEditCtrl */

#if !defined(AFX_RICHEDITEX_H__A66381A9_33C5_11D4_8ED0_0040052C10D3__INCLUDED_)
#define AFX_RICHEDITEX_H__A66381A9_33C5_11D4_8ED0_0040052C10D3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CRichEditEx: public CRichEditCtrl
	{
	public:
		CRichEditEx();

	public:
		virtual ~CRichEditEx();

	protected:
		//{{AFX_MSG(CRichEditEx)
		afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
		//}}AFX_MSG

		DECLARE_MESSAGE_MAP()
	};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RICHEDITEX_H__A66381A9_33C5_11D4_8ED0_0040052C10D3__INCLUDED_)
