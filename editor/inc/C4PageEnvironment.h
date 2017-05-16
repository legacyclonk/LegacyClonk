/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Environment page in scenario properties */

#if !defined(AFX_C4PAGEENVIRONMENT_H__0D8ECE60_0B23_11D3_8888_0040052C10D3__INCLUDED_)
#define AFX_C4PAGEENVIRONMENT_H__0D8ECE60_0B23_11D3_8888_0040052C10D3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class C4PageEnvironment: public CDialog
	{
	public:
		C4PageEnvironment(CWnd* pParent = NULL);   
	
	public:
		C4DefListEx *pDefs;
	
	public:
		virtual void OnOK();
		void GetData(C4Scenario &rC4S);
		void SetData(C4Scenario &rC4S);

	protected:
		bool NoInitialize;
		void UpdateText();

	public:
		//{{AFX_DATA(C4PageEnvironment)
		enum { IDD = IDD_PAGEENVIRONMENT };
		CStaticEx m_StaticEnvironment;
		CSlider	m_SliderEnvironment;
		C4IdListCtrl	m_ListEnvironment;
		C4IdCtrlButtons	m_ButtonsEnvironment;
		CSlider	m_SliderNests;
		CSlider	m_SliderAnimals;
		CSlider	m_SliderVegetation;
		CSlider	m_SliderInEarth;
		CStaticEx m_StaticVegetationTypes;
		CStaticEx m_StaticVegetationAmounts;
		CStaticEx m_StaticNests;
		CStaticEx m_StaticInEarthTypes;
		CStaticEx m_StaticInEarthAmounts;
		CStaticEx m_StaticAnimals;
		CSlider	m_SliderVegetationAmount;
		CSlider	m_SliderInEarthAmount;
		C4IdListCtrl	m_ListVegetation;
		C4IdListCtrl	m_ListNests;
		C4IdListCtrl	m_ListInEarth;
		C4IdListCtrl	m_ListAnimals;
		C4IdCtrlButtons	m_ButtonsVegetation;
		C4IdCtrlButtons	m_ButtonsNests;
		C4IdCtrlButtons	m_ButtonsInEarth;
		C4IdCtrlButtons	m_ButtonsAnimals;
		//}}AFX_DATA


		//{{AFX_VIRTUAL(C4PageEnvironment)
		protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		//}}AFX_VIRTUAL

		//{{AFX_MSG(C4PageEnvironment)
		afx_msg void OnPaint();
		afx_msg BOOL OnEraseBkgnd(CDC* pDC);
		virtual BOOL OnInitDialog();
		afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
		//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_C4PAGEENVIRONMENT_H__0D8ECE60_0B23_11D3_8888_0040052C10D3__INCLUDED_)
