/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Promotion dialog */

#if !defined(AFX_C4PROMOTIONDLG_H__581995C0_27D7_11D3_8ED0_0040052C10D3__INCLUDED_)
#define AFX_C4PROMOTIONDLG_H__581995C0_27D7_11D3_8ED0_0040052C10D3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif 

void DoPlayerPromotion(const char *szModules);

class C4PromotionDlg : public CDialog
	{
	public:
		C4PromotionDlg(CWnd* pParent = NULL);  

		//{{AFX_DATA(C4PromotionDlg)
		enum { IDD = IDD_PROMOTION };
		CButtonEx m_ButtonOK;
		CStaticEx m_StaticText;
		CStaticEx m_StaticComment;
		CString	m_Comment;
		CString	m_Text;
		//}}AFX_DATA


		//{{AFX_VIRTUAL(C4PromotionDlg)
		protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		//}}AFX_VIRTUAL

	protected:
		void UpdateText();

		// Generated message map functions
		//{{AFX_MSG(C4PromotionDlg)
	virtual BOOL OnInitDialog();
		//afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_C4PROMOTIONDLG_H__581995C0_27D7_11D3_8ED0_0040052C10D3__INCLUDED_)



