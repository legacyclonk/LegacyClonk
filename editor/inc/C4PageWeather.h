/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Weather page in scenario properties */

#if !defined(AFX_C4WEATHERPAGE_H__814E3200_0A60_11D3_8888_0040052C10D3__INCLUDED_)
#define AFX_C4WEATHERPAGE_H__814E3200_0A60_11D3_8888_0040052C10D3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class C4PageWeather : public CDialog
	{
	public:
		C4PageWeather(CWnd* pParent = NULL);   

	public:
		void SetData(C4Scenario &rC4S);
		void GetData(C4Scenario &rC4S);

	protected:
		CImageList DecoWeather;
		void UpdateText();

	public:
		virtual void OnOK();
		//{{AFX_DATA(C4PageWeather)
	enum { IDD = IDD_PAGEWEATHER };
	CStatic	m_BitmapGravity;
		CStatic	m_BitmapVolcanoes;
		CStatic	m_BitmapMeteorites;
		CStatic	m_BitmapEarthquakes;
		CStatic	m_BitmapWind;
		CStatic	m_BitmapTimeAdvance;
		CStatic	m_BitmapThunderstorm;
		CStatic	m_BitmapSeason;
		CStatic	m_BitmapRain;
		CStatic	m_BitmapClimate;
		CStaticEx m_StaticRain;
		CStaticEx m_StaticThunderstorms;
		CStaticEx m_StaticWind;
		CStaticEx m_StaticGravity;
		CStaticEx m_StaticRain2;
		CStaticEx m_StaticThunderstorms2;
		CStaticEx m_StaticWind2;
		CStaticEx m_StaticGravity2;
		CStaticEx m_StaticRain3;
		CStaticEx m_StaticThunderstorms3;
		CStaticEx m_StaticWind3;
		CStaticEx m_StaticGravity3;
	CStaticEx m_StaticVolcanoes3;
	CStaticEx m_StaticTimeAdvance3;
	CStaticEx m_StaticSeason3;
	CStaticEx m_StaticMeteorites3;
	CStaticEx m_StaticEarthquakes3;
	CStaticEx m_StaticClimate3;
	CStaticEx m_StaticVolcanoes2;
	CStaticEx m_StaticTimeAdvance2;
	CStaticEx m_StaticSeason2;
	CStaticEx m_StaticMeteorites2;
	CStaticEx m_StaticEarthquakes2;
	CStaticEx m_StaticClimate2;
	CSlider	m_SliderGravity;
		CStaticEx m_StaticVolcanoes;
		CStaticEx m_StaticTimeAdvance;
		CStaticEx m_StaticSeason;
		CStaticEx m_StaticMeteorites;
		CStaticEx m_StaticEarthquakes;
		CStaticEx m_StaticClimate;
		CSlider	m_SliderWind;
		CSlider	m_SliderVolcanoes;
		CSlider	m_SliderTimeAdvance;
		CSlider	m_SliderThunderstorms;
		CSlider	m_SliderSeason;
		CSlider	m_SliderRain;
		CSlider	m_SliderMeteorites;
		CSlider	m_SliderEarthquakes;
		CSlider	m_SliderClimate;
	//}}AFX_DATA


		//{{AFX_VIRTUAL(C4PageWeather)
		protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		//}}AFX_VIRTUAL

		//{{AFX_MSG(C4PageWeather)
		afx_msg void OnPaint();
		afx_msg BOOL OnEraseBkgnd(CDC* pDC);
		virtual BOOL OnInitDialog();
		//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_C4WEATHERPAGE_H__814E3200_0A60_11D3_8888_0040052C10D3__INCLUDED_)
