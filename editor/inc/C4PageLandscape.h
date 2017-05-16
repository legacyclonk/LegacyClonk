/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Landscape page in scenario properties */

#if !defined(AFX_C4PAGELANDSCAPE_H__8351C0E2_0E97_11D3_8889_0040052C10D3__INCLUDED_)
#define AFX_C4PAGELANDSCAPE_H__8351C0E2_0E97_11D3_8889_0040052C10D3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "C4LandscapeCtrl.h"

class C4PageLandscape: public CDialog
	{
	public:
		virtual void OnOK();
		void GetData(C4Scenario &rC4S);
		void SetData(C4Scenario &rC4S);
		C4PageLandscape(CWnd* pParent = NULL);  

		C4SLandscape LandscapeData;

		//{{AFX_DATA(C4PageLandscape)
		enum { IDD = IDD_PAGELANDSCAPE };
		C4LandscapeCtrl m_LandscapeCtrl;
		CStaticEx m_StaticZoom;
		CStaticEx m_StaticWidth;
		CStaticEx m_StaticWaterLevel;
		CStaticEx m_StaticShow;
		CStaticEx m_StaticRandom;
		CStaticEx m_StaticPreview;
		CStaticEx m_StaticPhase;
		CStaticEx m_StaticPeriod;
		CStaticEx m_StaticMapType;
		CStaticEx m_StaticHeight;
		CStaticEx m_StaticAmplitude;
		CSlider	m_SliderHeight;
		CSlider	m_SliderZoom;
		CSlider	m_SliderWidth;
		CSlider	m_SliderWaterLevel;
		CSlider	m_SliderRandom;
		CSlider	m_SliderPhase;
		CSlider	m_SliderPeriod;
		CSlider	m_SliderAmplitude;
		CRadioEx m_RadioPlayers1;
		CRadioEx m_RadioPlayers2;
		CRadioEx m_RadioPlayers3;
		CRadioEx m_RadioPlayers4;
		CEdit	m_EditZoom;
		CRadioEx m_CheckShowScreen;
		CRadioEx m_CheckShowLayers;
		CRadioEx m_CheckOpenTop;
		CRadioEx m_CheckOpenBottom;
		CRadioEx m_CheckMapPlayerExtend;
		int		m_PreviewForPlayers;
		BOOL	m_MapPlayerExtend;
		BOOL	m_OpenBottom;
		BOOL	m_OpenTop;
		BOOL	m_ShowLayers;
		BOOL	m_ShowScreen;
		int		m_Zoom;
	//}}AFX_DATA


		//{{AFX_VIRTUAL(C4PageLandscape)
		protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		//}}AFX_VIRTUAL

	protected:
		int m_LandscapeMode;
		void UpdateZoomCtrl();
		void OnSliderChanged();
		void EnableControls();
		void UpdateLandscape();
		void GetLandscapeData();
		void SetLandscapeData();
		void UpdateText();

		//{{AFX_MSG(C4PageLandscape)
		afx_msg void OnPaint();
		afx_msg BOOL OnEraseBkgnd(CDC* pDC);
		virtual BOOL OnInitDialog();
	afx_msg void OnCheckMapPlayerExtend();
	afx_msg void OnCheckOpenBottom();
	afx_msg void OnCheckOpenTop();
	afx_msg void OnCheckShowLayers();
	afx_msg void OnCheckShowScreen();
	afx_msg void OnRadioPlayers1();
	afx_msg void OnRadioPlayers2();
	afx_msg void OnRadioPlayers3();
	afx_msg void OnRadioPlayers4();
	//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_C4PAGELANDSCAPE_H__8351C0E2_0E97_11D3_8889_0040052C10D3__INCLUDED_)
