/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* External editor programs tab in options */

#if !defined(AFX_C4EDITORPAGE_H__B6376260_B4BC_11D2_8888_0040052C10D3__INCLUDED_)
#define AFX_C4EDITORPAGE_H__B6376260_B4BC_11D2_8888_0040052C10D3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif 

class C4PageEditor: public CPropertyPage
	{
	DECLARE_DYNCREATE(C4PageEditor)

	public:
		C4PageEditor();
		~C4PageEditor();

	public:
		void SetData();
		void GetData();

		//{{AFX_DATA(C4PageEditor)
	enum { IDD = IDD_PAGEEDITOR };
	CStatic	m_StaticPNG;
	CEdit	m_EditEditorPNG;
	CButton	m_ButtonEditorPNG;
		CEdit	m_EditEditorZip;
		CEdit	m_EditEditorText;
		CEdit	m_EditEditorSound;
		CEdit	m_EditEditorScript;
		CEdit	m_EditEditorRichText;
		CEdit	m_EditEditorMusic;
		CEdit	m_EditEditorBitmap;
		CButton	m_CheckUseShell;
		CStatic	m_StaticZip;
		CButton	m_ButtonEditorZip;
		CStatic	m_StaticText;
		CStatic	m_StaticSound;
		CStatic	m_StaticScript;
		CStatic	m_StaticRichText;
		CStatic	m_StaticMusic;
		CStatic	m_StaticBitmap;
		CButton	m_ButtonEditorText;
		CButton	m_ButtonEditorSound;
		CButton	m_ButtonEditorScript;
		CButton	m_ButtonEditorRichText;
		CButton	m_ButtonEditorMusic;
		CButton	m_ButtonEditorBitmap;
		CString	m_EditorBitmap;
		CString	m_EditorMusic;
		CString	m_EditorRichText;
		CString	m_EditorScript;
		CString	m_EditorSound;
		CString	m_EditorText;
		CString	m_EditorZip;
		BOOL	m_UseShell;
	CString	m_EditorPNG;
	//}}AFX_DATA

		//{{AFX_VIRTUAL(C4PageEditor)
		protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		//}}AFX_VIRTUAL

	protected:
		void EnableControls(bool fEnable);
		//{{AFX_MSG(C4PageEditor)
		virtual BOOL OnInitDialog();
		afx_msg void OnButtonEditorBitmap();
		afx_msg void OnButtonEditorMusic();
		afx_msg void OnButtonEditorRichText();
		afx_msg void OnButtonEditorScript();
		afx_msg void OnButtonEditorSound();
		afx_msg void OnButtonEditorText();
	afx_msg void OnButtonEditorZip();
	afx_msg void OnCheckUseShell();
	afx_msg void OnButtonEditorPNG();
	//}}AFX_MSG
		DECLARE_MESSAGE_MAP()

	};

//{{AFX_INSERT_LOCATION}}

#endif // !defined(AFX_C4EDITORPAGE_H__B6376260_B4BC_11D2_8888_0040052C10D3__INCLUDED_)
