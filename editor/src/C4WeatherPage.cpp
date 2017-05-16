/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Weather page in scenario properties */

#include "C4Explorer.h"

#include "C4PageWeather.h"

BEGIN_MESSAGE_MAP(C4PageWeather, CDialog)
	//{{AFX_MSG_MAP(C4PageWeather)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


C4PageWeather::C4PageWeather(CWnd* pParent /*=NULL*/)
	: CDialog(C4PageWeather::IDD, pParent)
	{
	//{{AFX_DATA_INIT(C4PageWeather)
	//}}AFX_DATA_INIT
	LoadImageList(IDB_DECOWEATHER, DecoWeather, 32, 32);
	}


void C4PageWeather::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(C4PageWeather)
	DDX_Control(pDX, IDC_BITMAPGRAVITY, m_BitmapGravity);
	DDX_Control(pDX, IDC_BITMAPVOLCANOES, m_BitmapVolcanoes);
	DDX_Control(pDX, IDC_BITMAPMETEORITES, m_BitmapMeteorites);
	DDX_Control(pDX, IDC_BITMAPEARTHQUAKES, m_BitmapEarthquakes);
	DDX_Control(pDX, IDC_BITMAPWIND, m_BitmapWind);
	DDX_Control(pDX, IDC_BITMAPTIMEADVANCE, m_BitmapTimeAdvance);
	DDX_Control(pDX, IDC_BITMAPTHUNDERSTORM, m_BitmapThunderstorm);
	DDX_Control(pDX, IDC_BITMAPSEASON, m_BitmapSeason);
	DDX_Control(pDX, IDC_BITMAPRAIN, m_BitmapRain);
	DDX_Control(pDX, IDC_BITMAPCLIMATE, m_BitmapClimate);
	DDX_Control(pDX, IDC_STATICRAIN, m_StaticRain);
	DDX_Control(pDX, IDC_STATICTHUNDERSTORMS, m_StaticThunderstorms);
	DDX_Control(pDX, IDC_STATICWIND, m_StaticWind);
	DDX_Control(pDX, IDC_STATICGRAVITY, m_StaticGravity);
	DDX_Control(pDX, IDC_STATICRAIN2, m_StaticRain2);
	DDX_Control(pDX, IDC_STATICTHUNDERSTORMS2, m_StaticThunderstorms2);
	DDX_Control(pDX, IDC_STATICWIND2, m_StaticWind2);
	DDX_Control(pDX, IDC_STATICGRAVITY2, m_StaticGravity2);
	DDX_Control(pDX, IDC_STATICRAIN3, m_StaticRain3);
	DDX_Control(pDX, IDC_STATICTHUNDERSTORMS3, m_StaticThunderstorms3);
	DDX_Control(pDX, IDC_STATICWIND3, m_StaticWind3);
	DDX_Control(pDX, IDC_STATICGRAVITY3, m_StaticGravity3);
	DDX_Control(pDX, IDC_STATICVOLCANOES3, m_StaticVolcanoes3);
	DDX_Control(pDX, IDC_STATICTIMEADVANCE3, m_StaticTimeAdvance3);
	DDX_Control(pDX, IDC_STATICSEASON3, m_StaticSeason3);
	DDX_Control(pDX, IDC_STATICMETEORITES3, m_StaticMeteorites3);
	DDX_Control(pDX, IDC_STATICEARTHQUAKES3, m_StaticEarthquakes3);
	DDX_Control(pDX, IDC_STATICCLIMATE3, m_StaticClimate3);
	DDX_Control(pDX, IDC_STATICVOLCANOES2, m_StaticVolcanoes2);
	DDX_Control(pDX, IDC_STATICTIMEADVANCE2, m_StaticTimeAdvance2);
	DDX_Control(pDX, IDC_STATICSEASON2, m_StaticSeason2);
	DDX_Control(pDX, IDC_STATICMETEORITES2, m_StaticMeteorites2);
	DDX_Control(pDX, IDC_STATICEARTHQUAKES2, m_StaticEarthquakes2);
	DDX_Control(pDX, IDC_STATICCLIMATE2, m_StaticClimate2);
	DDX_Control(pDX, IDC_SLIDERGRAVITY, m_SliderGravity);
	DDX_Control(pDX, IDC_STATICVOLCANOES, m_StaticVolcanoes);
	DDX_Control(pDX, IDC_STATICTIMEADVANCE, m_StaticTimeAdvance);
	DDX_Control(pDX, IDC_STATICSEASON, m_StaticSeason);
	DDX_Control(pDX, IDC_STATICMETEORITES, m_StaticMeteorites);
	DDX_Control(pDX, IDC_STATICEARTHQUAKES, m_StaticEarthquakes);
	DDX_Control(pDX, IDC_STATICCLIMATE, m_StaticClimate);
	DDX_Control(pDX, IDC_SLIDERWIND, m_SliderWind);
	DDX_Control(pDX, IDC_SLIDERVOLCANOES, m_SliderVolcanoes);
	DDX_Control(pDX, IDC_SLIDERTIMEADVANCE, m_SliderTimeAdvance);
	DDX_Control(pDX, IDC_SLIDERTHUNDERSTORMS, m_SliderThunderstorms);
	DDX_Control(pDX, IDC_SLIDERSEASON, m_SliderSeason);
	DDX_Control(pDX, IDC_SLIDERRAIN, m_SliderRain);
	DDX_Control(pDX, IDC_SLIDERMETEORITES, m_SliderMeteorites);
	DDX_Control(pDX, IDC_SLIDEREARTHQUAKES, m_SliderEarthquakes);
	DDX_Control(pDX, IDC_SLIDERCLIMATE, m_SliderClimate);
	//}}AFX_DATA_MAP
}

BOOL C4PageWeather::OnInitDialog() 
	{
	CDialog::OnInitDialog();
	
	// Text 
	UpdateText();

	// Sliders
	m_SliderClimate.Init(RGB(200,160,80));
	m_SliderSeason.Init(RGB(160,120,40));
	m_SliderTimeAdvance.Init(RGB(120,80,0));

	m_SliderRain.Init(RGB(40,40,130));
	m_SliderThunderstorms.Init(RGB(90,90,200));
	m_SliderWind.Init(RGB(140,140,250));

	m_SliderEarthquakes.Init(RGB(255,80,50));
	m_SliderVolcanoes.Init(RGB(220,60,30));
	m_SliderMeteorites.Init(RGB(200,30,0));

	m_SliderGravity.Init(RGB(80,80,80));

	return TRUE; 
	}

void C4PageWeather::OnPaint() 
	{
	CPaintDC DC(this);

	DrawDecoPic(DecoWeather, DC, m_BitmapClimate, 0);
	DrawDecoPic(DecoWeather, DC, m_BitmapSeason, 1);
	DrawDecoPic(DecoWeather, DC, m_BitmapTimeAdvance, 2);
	DrawDecoPic(DecoWeather, DC, m_BitmapRain, 3);
	DrawDecoPic(DecoWeather, DC, m_BitmapThunderstorm, 4);
	DrawDecoPic(DecoWeather, DC, m_BitmapWind, 5);
	DrawDecoPic(DecoWeather, DC, m_BitmapEarthquakes, 6);
	DrawDecoPic(DecoWeather, DC, m_BitmapVolcanoes, 7);
	DrawDecoPic(DecoWeather, DC, m_BitmapMeteorites, 8);
	DrawDecoPic(DecoWeather, DC, m_BitmapGravity, 9);

	}

BOOL C4PageWeather::OnEraseBkgnd(CDC* pDC) 
	{
	GetApp()->dibPaper.DrawTiles(pDC, this);
	return true;
	}


void C4PageWeather::UpdateText()
	{
	m_StaticClimate.Set("IDS_CTL_CLIMATE");
	m_StaticClimate2.Set("IDS_CTL_CLIMATE2");
	m_StaticClimate3.Set("IDS_CTL_CLIMATE3");
	m_StaticSeason.Set("IDS_CTL_SEASON");
	m_StaticSeason2.Set("IDS_CTL_SEASON2");
	m_StaticSeason3.Set("IDS_CTL_SEASON3");
	m_StaticTimeAdvance.Set("IDS_CTL_TIMEADVANCE");
	m_StaticTimeAdvance2.Set("IDS_CTL_TIMEADVANCE2");
	m_StaticTimeAdvance3.Set("IDS_CTL_TIMEADVANCE3");

	m_StaticRain.Set("IDS_CTL_RAIN");
	m_StaticThunderstorms.Set("IDS_CTL_THUNDERSTORMS");
	m_StaticWind.Set("IDS_CTL_WIND");
	m_StaticRain2.Set("IDS_CTL_RAIN2");
	m_StaticThunderstorms2.Set("IDS_CTL_THUNDERSTORMS2");
	m_StaticWind2.Set("IDS_CTL_WIND2");
	m_StaticRain3.Set("IDS_CTL_RAIN3");
	m_StaticThunderstorms3.Set("IDS_CTL_THUNDERSTORMS3");
	m_StaticWind3.Set("IDS_CTL_WIND3");

	m_StaticEarthquakes.Set("IDS_CTL_EARTHQUAKES");
	m_StaticEarthquakes2.Set("IDS_CTL_EARTHQUAKES2");
	m_StaticEarthquakes3.Set("IDS_CTL_EARTHQUAKES3");
	m_StaticVolcanoes.Set("IDS_CTL_VOLCANOES");
	m_StaticVolcanoes2.Set("IDS_CTL_VOLCANOES2");
	m_StaticVolcanoes3.Set("IDS_CTL_VOLCANOES3");
	m_StaticMeteorites.Set("IDS_CTL_METEORITES");
	m_StaticMeteorites2.Set("IDS_CTL_METEORITES2");
	m_StaticMeteorites3.Set("IDS_CTL_METEORITES3");

	m_StaticGravity.Set("IDS_CTL_GRAVITY");
	m_StaticGravity2.Set("IDS_CTL_GRAVITY2");
	m_StaticGravity3.Set("IDS_CTL_GRAVITY3");
	}

void C4PageWeather::GetData(C4Scenario &rC4S)
	{
	m_SliderClimate.Get(rC4S.Weather.Climate);
	m_SliderSeason.Get(rC4S.Weather.StartSeason);
	m_SliderTimeAdvance.Get(rC4S.Weather.YearSpeed);

	m_SliderRain.Get(rC4S.Weather.Rain);
	m_SliderThunderstorms.Get(rC4S.Weather.Lightning);
	m_SliderWind.Get(rC4S.Weather.Wind);

	m_SliderEarthquakes.Get(rC4S.Disasters.Earthquake);
	m_SliderVolcanoes.Get(rC4S.Disasters.Volcano);
	m_SliderMeteorites.Get(rC4S.Disasters.Meteorite);

	m_SliderGravity.Get(rC4S.Landscape.Gravity);
	}

void C4PageWeather::SetData(C4Scenario &rC4S)
	{
	m_SliderClimate.Set(rC4S.Weather.Climate);
	m_SliderSeason.Set(rC4S.Weather.StartSeason);
	m_SliderTimeAdvance.Set(rC4S.Weather.YearSpeed);

	m_SliderRain.Set(rC4S.Weather.Rain);
	m_SliderThunderstorms.Set(rC4S.Weather.Lightning);
	m_SliderWind.Set(rC4S.Weather.Wind);

	m_SliderEarthquakes.Set(rC4S.Disasters.Earthquake);
	m_SliderVolcanoes.Set(rC4S.Disasters.Volcano);
	m_SliderMeteorites.Set(rC4S.Disasters.Meteorite);

	m_SliderGravity.Set(rC4S.Landscape.Gravity);
	}

void C4PageWeather::OnOK()
{

}
