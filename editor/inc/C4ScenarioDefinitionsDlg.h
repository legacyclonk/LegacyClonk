/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Scenario definition preset dialog */

#if !defined(AFX_C4SCENARIODEFINITIONSDLG_H__B2FD7280_1F77_11D3_8ED0_0040052C10D3__INCLUDED_)
#define AFX_C4SCENARIODEFINITIONSDLG_H__B2FD7280_1F77_11D3_8ED0_0040052C10D3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class C4ScenarioDefinitionsDlg: public CDialog
	{
	public:
		bool Init(const char *szFilename, bool fOriginal, bool fDeveloper);
		C4ScenarioDefinitionsDlg(CWnd* pParent = NULL);  

		//{{AFX_DATA(C4ScenarioDefinitionsDlg)
		enum { IDD = IDD_SCENARIODEFINITIONS };
		CButtonEx m_ButtonOK;
		CButtonEx m_ButtonCancel;
		CStaticEx m_StaticScenarioUses;
		CRadioEx m_RadioDefByChoice;
		CRadioEx m_RadioLocalOnly;
		CRadioEx m_RadioSpecifiedDefs;
		CListCtrl	m_ListDefinitions;
		CButtonEx m_ButtonUseActivated;
		int		m_Selection;
		//}}AFX_DATA

		//{{AFX_VIRTUAL(C4ScenarioDefinitionsDlg)
		virtual int DoModal();
		virtual void DoDataExchange(CDataExchange* pDX);  
		//}}AFX_VIRTUAL

	protected:
		CImageList StateList;
		void InsertDefinitionModule(int iItem, const char *szFilename);
		bool Developer;
		bool Save();
		void EnableControls();
		void GetData();
		void SetData();
		CString Filename;
		char Title[C4MaxTitle+1];
		bool Original;
		C4Scenario C4S;

		void UpdateText();

		//{{AFX_MSG(C4ScenarioDefinitionsDlg)
		virtual BOOL OnInitDialog();
		virtual void OnOK();
		//afx_msg BOOL OnEraseBkgnd(CDC* pDC);
		afx_msg void OnRadioDefByChoice();
		afx_msg void OnRadioSpecifiedDefs();
		afx_msg void OnRadioLocalOnly();
		afx_msg void OnButtonUseActivated();
	//afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_C4SCENARIODEFINITIONSDLG_H__B2FD7280_1F77_11D3_8ED0_0040052C10D3__INCLUDED_)
