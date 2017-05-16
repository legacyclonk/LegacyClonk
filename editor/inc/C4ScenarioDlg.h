/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Scenario properties dialog */

#if !defined(AFX_C4SCENARIODLG_H__78F43461_00BF_11D3_8888_0040052C10D3__INCLUDED_)
#define AFX_C4SCENARIODLG_H__78F43461_00BF_11D3_8888_0040052C10D3__INCLUDED_

#include "C4PageGame.h"
#include "C4PageWeather.h"	
#include "C4PageEnvironment.h"
#include "C4PagePlayerStart.h"
#include "C4PageLandscape.h"

#if _MSC_VER >= 1000
#pragma once
#endif 

class C4ScenarioDlg: public CDialog
	{

	enum EEditPage { EP_Game,	EP_PlayerStart, EP_Landscape, EP_Weather, EP_Environment, } EditPage;

	public:
		C4ScenarioDlg(CWnd* pParent = NULL);   
		enum { IDD = IDD_SCENARIO };
		CString Filename;

	protected:
		char Title[C4MaxTitle+1];
		CString Author;
		C4Scenario C4S;
		C4PageGame PageGame;
		C4PageWeather PageWeather;
		C4PageEnvironment PageEnvironment;
		C4PagePlayerStart PagePlayerStart;
		C4PageLandscape PageLandscape;
		int iLastPage;
		CDialog* pCurrentPage;
		C4DefListEx Defs;

	public:
		bool Init(const char *szFilename, bool fOriginal);
		void GetData();
		void SetData();

	protected:
		void CheckDefinitionPreset();
		bool Original;
		void CreatePage(CDialog& Page, UINT TemplateID, bool Show=true	);
		void InvalidateTabStrip(bool BordersOnly);
		void UpdateText();
		bool Save();
		void HideCurrentPage();

	public:

		//{{AFX_DATA(C4ScenarioDlg)
		CTabCtrlEx	m_TabScenario;
		CButtonEx m_ButtonOK;
		CButtonEx m_ButtonCancel;
		//}}AFX_DATA

		//{{AFX_VIRTUAL(C4ScenarioDlg)
		virtual void DoDataExchange(CDataExchange* pDX);   
		virtual int DoModal();
		//}}AFX_VIRTUAL

		//{{AFX_MSG(C4ScenarioDlg)
		//afx_msg BOOL OnEraseBkgnd(CDC* pDC);
		virtual BOOL OnInitDialog();
		virtual void OnOK();
		afx_msg void OnSelChangeTabScenario(NMHDR* pNMHDR, LRESULT* pResult);
	//afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
		DECLARE_MESSAGE_MAP()

	};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_C4SCENARIODLG_H__78F43461_00BF_11D3_8888_0040052C10D3__INCLUDED_)
