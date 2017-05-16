/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Main page in options */

#if !defined(AFX_C4GAMEPAGE_H__78F43462_00BF_11D3_8888_0040052C10D3__INCLUDED_)
#define AFX_C4GAMEPAGE_H__78F43462_00BF_11D3_8888_0040052C10D3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif

#include "IconCombo.h"

class C4PageGame: public CDialog
	{	
	public:
		C4PageGame(CWnd* pParent = NULL); 
	protected:
		CImageList ListDeco;
		bool NoInitialize;
	public:
		C4DefListEx *pDefs;
	public:
		void GetData(C4Scenario &C4S);
		void SetData(C4Scenario &C4S);
	protected:
		void UpdateText();	
	public:
		//{{AFX_DATA(C4PageGame)
	enum { IDD = IDD_PAGEGAME };
	C4IdCtrlButtons	m_ButtonsSkipDefs;
	CSlider	m_SliderSkipDefs;
	CStaticEx m_StaticSkipDefs;
	C4IdListCtrl	m_ListSkipDefs;
		CStaticEx m_StaticRules;
		CStaticEx m_StaticGoals;
		CSlider	m_SliderRules;
		CSlider	m_SliderGoals;
		C4IdListCtrl	m_ListRules;
		C4IdListCtrl	m_ListGoals;
		C4IdCtrlButtons	m_ButtonsRules;
		C4IdCtrlButtons	m_ButtonsGoals;
		CStaticEx m_DecoAccess;
		CStaticEx m_StaticMaxPlayers;
		CStaticEx m_StaticIcon;
		CStaticEx m_StaticAccess;
		CSpinButtonCtrl	m_SpinMaxPlayers;
		CEdit	m_EditMaxPlayers;
		CRadioEx m_CheckAccess;
		CIconCombo m_ComboIcon;
		BOOL	m_Access;
		int		m_Icon;
		int		m_MaxPlayers;
	//}}AFX_DATA

		//{{AFX_VIRTUAL(C4PageGame)
		virtual void DoDataExchange(CDataExchange* pDX);   
		//}}AFX_VIRTUAL

		//{{AFX_MSG(C4PageGame)
		afx_msg void OnPaint();
		afx_msg BOOL OnEraseBkgnd(CDC* pDC);
		virtual BOOL OnInitDialog();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_C4GAMEPAGE_H__78F43462_00BF_11D3_8888_0040052C10D3__INCLUDED_)
