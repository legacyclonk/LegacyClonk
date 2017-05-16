/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Program page in options */

#if !defined(AFX_C4PROGRAMPAGE_H__A28467C2_9470_11D2_8888_0040052C10D3__INCLUDED_)
#define AFX_C4PROGRAMPAGE_H__A28467C2_9470_11D2_8888_0040052C10D3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class C4PageProgram: public CPropertyPage
	{
	//DECLARE_DYNCREATE(C4PageProgram)

	public:
		C4PageProgram();
		~C4PageProgram();

	public:
		void GetData();
		void SetData();

	protected:
		void UpdateText();
		CString GetLanguageFallbacks();
		void UpdateLanguageInfo();

	protected:
		CString m_LanguageNames;
		CString m_LanguageInfos;
		CString m_LanguageFallbacks;

	public:
		//{{AFX_DATA(C4PageProgram)
	enum { IDD = IDD_PAGEPROGRAM };
		CStatic	m_StaticLanguageInfo;
		CComboBox	m_ComboLanguage;
		CStatic	m_StaticLanguage;
		CString	m_Language;
	//}}AFX_DATA

		//{{AFX_VIRTUAL(C4PageProgram)
		protected:
		virtual void DoDataExchange(CDataExchange* pDX);   
		//}}AFX_VIRTUAL

	protected:
		//{{AFX_MSG(C4PageProgram)
		virtual BOOL OnInitDialog();
		afx_msg void OnSelEndOkComboLanguage();
		afx_msg void OnEditChangeComboLanguage();
	//}}AFX_MSG
		DECLARE_MESSAGE_MAP()

	};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_C4PROGRAMPAGE_H__A28467C2_9470_11D2_8888_0040052C10D3__INCLUDED_)
