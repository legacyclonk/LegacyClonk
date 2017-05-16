/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Landscape page in scenario properties */

#include "C4Explorer.h"

#include "C4PageLandscape.h"

const int MM_Dynamic   = 0,
					MM_Static    = 1,
					MM_Exact	   = 2,
					MM_Created   = 3;


BEGIN_MESSAGE_MAP(C4PageLandscape, CDialog)
	//{{AFX_MSG_MAP(C4PageLandscape)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_BN_CLICKED(IDC_CHECKMAPPLAYEREXTEND, OnCheckMapPlayerExtend)
	ON_BN_CLICKED(IDC_CHECKOPENBOTTOM, OnCheckOpenBottom)
	ON_BN_CLICKED(IDC_CHECKOPENTOP, OnCheckOpenTop)
	ON_BN_CLICKED(IDC_CHECKSHOWLAYERS, OnCheckShowLayers)
	ON_BN_CLICKED(IDC_CHECKSHOWSCREEN, OnCheckShowScreen)
	ON_BN_CLICKED(IDC_RADIOPLAYERS1, OnRadioPlayers1)
	ON_BN_CLICKED(IDC_RADIOPLAYERS2, OnRadioPlayers2)
	ON_BN_CLICKED(IDC_RADIOPLAYERS3, OnRadioPlayers3)
	ON_BN_CLICKED(IDC_RADIOPLAYERS4, OnRadioPlayers4)
	ON_CONTROL(SLN_CHANGE, IDC_SLIDERWIDTH, OnSliderChanged)
	ON_CONTROL(SLN_CHANGE, IDC_SLIDERHEIGHT, OnSliderChanged)
	ON_CONTROL(SLN_CHANGE, IDC_SLIDERZOOM, OnSliderChanged)
	ON_CONTROL(SLN_CHANGE, IDC_SLIDERAMPLITUDE, OnSliderChanged)
	ON_CONTROL(SLN_CHANGE, IDC_SLIDERPERIOD, OnSliderChanged)
	ON_CONTROL(SLN_CHANGE, IDC_SLIDERPHASE, OnSliderChanged)
	ON_CONTROL(SLN_CHANGE, IDC_SLIDERRANDOM, OnSliderChanged)
	ON_CONTROL(SLN_CHANGE, IDC_SLIDERWATERLEVEL, OnSliderChanged)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


C4PageLandscape::C4PageLandscape(CWnd* pParent /*=NULL*/)
	: CDialog(C4PageLandscape::IDD, pParent)
	{
	//{{AFX_DATA_INIT(C4PageLandscape)
	m_PreviewForPlayers = 0;							// Set by currently activated players
	m_ShowLayers = TRUE;
	m_ShowScreen = TRUE;
	//}}AFX_DATA_INIT
	}


void C4PageLandscape::DoDataExchange(CDataExchange* pDX)
	{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(C4PageLandscape)
	DDX_Control(pDX, IDC_CTRLLANDSCAPE, m_LandscapeCtrl);
	DDX_Control(pDX, IDC_STATICZOOM, m_StaticZoom);
	DDX_Control(pDX, IDC_STATICWIDTH, m_StaticWidth);
	DDX_Control(pDX, IDC_STATICWATERLEVEL, m_StaticWaterLevel);
	DDX_Control(pDX, IDC_STATICSHOW, m_StaticShow);
	DDX_Control(pDX, IDC_STATICRANDOM, m_StaticRandom);
	DDX_Control(pDX, IDC_STATICPREVIEWFOR, m_StaticPreview);
	DDX_Control(pDX, IDC_STATICPHASE, m_StaticPhase);
	DDX_Control(pDX, IDC_STATICPERIOD, m_StaticPeriod);
	DDX_Control(pDX, IDC_STATICMAPTYPE, m_StaticMapType);
	DDX_Control(pDX, IDC_STATICHEIGHT, m_StaticHeight);
	DDX_Control(pDX, IDC_STATICAMPLITUDE, m_StaticAmplitude);
	DDX_Control(pDX, IDC_SLIDERHEIGHT, m_SliderHeight);
	DDX_Control(pDX, IDC_SLIDERZOOM, m_SliderZoom);
	DDX_Control(pDX, IDC_SLIDERWIDTH, m_SliderWidth);
	DDX_Control(pDX, IDC_SLIDERWATERLEVEL, m_SliderWaterLevel);
	DDX_Control(pDX, IDC_SLIDERRANDOM, m_SliderRandom);
	DDX_Control(pDX, IDC_SLIDERPHASE, m_SliderPhase);
	DDX_Control(pDX, IDC_SLIDERPERIOD, m_SliderPeriod);
	DDX_Control(pDX, IDC_SLIDERAMPLITUDE, m_SliderAmplitude);
	DDX_Control(pDX, IDC_RADIOPLAYERS1, m_RadioPlayers1);
	DDX_Control(pDX, IDC_RADIOPLAYERS2, m_RadioPlayers2);
	DDX_Control(pDX, IDC_RADIOPLAYERS3, m_RadioPlayers3);
	DDX_Control(pDX, IDC_RADIOPLAYERS4, m_RadioPlayers4);
	DDX_Control(pDX, IDC_EDITZOOM, m_EditZoom);
	DDX_Control(pDX, IDC_CHECKSHOWSCREEN, m_CheckShowScreen);
	DDX_Control(pDX, IDC_CHECKSHOWLAYERS, m_CheckShowLayers);
	DDX_Control(pDX, IDC_CHECKOPENTOP, m_CheckOpenTop);
	DDX_Control(pDX, IDC_CHECKOPENBOTTOM, m_CheckOpenBottom);
	DDX_Control(pDX, IDC_CHECKMAPPLAYEREXTEND, m_CheckMapPlayerExtend);
	DDX_Radio(pDX, IDC_RADIOPLAYERS1, m_PreviewForPlayers);
	DDX_Check(pDX, IDC_CHECKMAPPLAYEREXTEND, m_MapPlayerExtend);
	DDX_Check(pDX, IDC_CHECKOPENBOTTOM, m_OpenBottom);
	DDX_Check(pDX, IDC_CHECKOPENTOP, m_OpenTop);
	DDX_Check(pDX, IDC_CHECKSHOWLAYERS, m_ShowLayers);
	DDX_Check(pDX, IDC_CHECKSHOWSCREEN, m_ShowScreen);
	DDX_Text(pDX, IDC_EDITZOOM, m_Zoom);
	//}}AFX_DATA_MAP
	}
	
BOOL C4PageLandscape::OnInitDialog() 
	{
	CDialog::OnInitDialog();
	
	// Sliders
	m_SliderWidth.Init(RGB(255,0,0));
	m_SliderHeight.Init(RGB(215,0,0));
	m_SliderZoom.Init(RGB(195,0,0));
	m_SliderAmplitude.Init(RGB(0,215,0));
	m_SliderPeriod.Init(RGB(0,175,0));
	m_SliderPhase.Init(RGB(0,195,0));
	m_SliderRandom.Init(RGB(0,155,0));
	m_SliderWaterLevel.Init(RGB(0,0,255));
	m_SliderHeight.EnableRandom(false);
	m_SliderZoom.EnableRandom(false);
	m_SliderWidth.EnableRandom(false);

	// Landscape control
	m_LandscapeCtrl.EnableOptions(m_ShowScreen,m_ShowLayers,m_PreviewForPlayers+1);
	m_LandscapeCtrl.Update(LandscapeData);

	// Landscape mode
	m_LandscapeMode = MM_Dynamic; 
	if (m_LandscapeCtrl.IsStaticMap()) m_LandscapeMode = MM_Static;
	if (LandscapeData.ExactLandscape) m_LandscapeMode = MM_Exact;
	if (m_LandscapeCtrl.IsCreatedMap()) m_LandscapeMode = MM_Created;

	// Controls
	EnableControls();

	// Text
	UpdateText();
	
	return TRUE; 
	}

void C4PageLandscape::SetData(C4Scenario &rC4S)
	{
	// Data to dialog buffer
	LandscapeData = rC4S.Landscape;
	// Data to controls
	SetLandscapeData();
	}

void C4PageLandscape::GetData(C4Scenario &rC4S)
	{
	// Data from controls
	GetLandscapeData();
	// Data from dialog buffer
	rC4S.Landscape = LandscapeData;
	}

void C4PageLandscape::UpdateText()
	{
	m_StaticWidth.Set("IDS_CTL_WIDTH");
	m_StaticWaterLevel.Set("IDS_CTL_WATERLEVEL");
	m_StaticShow.Set("IDS_CTL_SHOW");
	m_StaticRandom.Set("IDS_CTL_RANDOM");
	m_StaticPreview.Set("IDS_CTL_PREVIEWFOR");
	m_StaticPhase.Set("IDS_CTL_PHASE");
	m_StaticPeriod.Set("IDS_CTL_PERIOD");
	m_StaticZoom.Set("IDS_CTL_ZOOM");
	m_StaticHeight.Set("IDS_CTL_HEIGHT");
	m_StaticAmplitude.Set("IDS_CTL_AMPLITUDE");
	m_RadioPlayers1.Set("IDS_CTL_PLAYERS1");
	m_RadioPlayers2.Set("IDS_CTL_PLAYERS2");
	m_RadioPlayers3.Set("IDS_CTL_PLAYERS3");
	m_RadioPlayers4.Set("IDS_CTL_PLAYERS4");
	m_CheckShowScreen.Set("IDS_CTL_SHOWSCREEN");
	m_CheckShowLayers.Set("IDS_CTL_SHOWLAYERS");
	m_CheckOpenTop.Set("IDS_CTL_OPENTOP");
	m_CheckOpenBottom.Set("IDS_CTL_OPENBOTTOM");
	m_CheckMapPlayerExtend.Set("IDS_CTL_MAPPLAYEREXTEND");

	// Landscape type
	CString sText;
	switch (m_LandscapeMode)
		{
		case MM_Dynamic: sText = LoadResStr("IDS_CTL_DYNAMICMAP"); break;
		case MM_Static: sText = LoadResStr("IDS_CTL_STATICMAP"); break;
		case MM_Exact: sText = LoadResStr("IDS_CTL_EXACTMAP"); break;
		case MM_Created: sText = LoadResStr("IDS_CTL_CREATEDMAP"); break;
		}
	if (m_LandscapeCtrl.IsLocalMaterials())
		sText+=LoadResStr("IDS_CTL_LOCALMATS");
	sText+=":";
	m_StaticMapType.Set(sText);

	}


void C4PageLandscape::OnPaint() 
	{
	CPaintDC DC(this);

	}

BOOL C4PageLandscape::OnEraseBkgnd(CDC* pDC) 
	{
	GetApp()->dibPaper.DrawTiles(pDC, this);
	return true;
	}

void C4PageLandscape::SetLandscapeData()
	{
	// Data to controls
	m_SliderHeight.Set(LandscapeData.MapHgt);
	m_SliderZoom.Set(LandscapeData.MapZoom);
	m_SliderWidth.Set(LandscapeData.MapWdt);
	m_SliderWaterLevel.Set(LandscapeData.LiquidLevel);
	m_SliderRandom.Set(LandscapeData.Random);
	m_SliderPhase.Set(LandscapeData.Phase);
	m_SliderPeriod.Set(LandscapeData.Period);
	m_SliderAmplitude.Set(LandscapeData.Amplitude);	
	m_Zoom = LandscapeData.MapZoom.Evaluate();
	m_MapPlayerExtend = LandscapeData.MapPlayerExtend;
	if (m_LandscapeCtrl.IsStaticMap()) m_MapPlayerExtend = false; // Static map override
	m_OpenTop = LandscapeData.TopOpen;
	m_OpenBottom = LandscapeData.BottomOpen;
	// Runtime update
	if (m_hWnd) UpdateData(false);
	}

void C4PageLandscape::GetLandscapeData()
	{
	// Runtime update
	if (m_hWnd) UpdateData(true);
	// Data from controls
	m_SliderHeight.Get(LandscapeData.MapHgt);
	m_SliderZoom.Get(LandscapeData.MapZoom);
	m_SliderWidth.Get(LandscapeData.MapWdt);
	m_SliderWaterLevel.Get(LandscapeData.LiquidLevel);
	m_SliderRandom.Get(LandscapeData.Random);
	m_SliderPhase.Get(LandscapeData.Phase);
	m_SliderPeriod.Get(LandscapeData.Period);
	m_SliderAmplitude.Get(LandscapeData.Amplitude);	
	LandscapeData.MapPlayerExtend = m_MapPlayerExtend;
	LandscapeData.TopOpen = m_OpenTop;
	LandscapeData.BottomOpen = m_OpenBottom;
	}

void C4PageLandscape::OnCheckMapPlayerExtend() 
	{
	UpdateLandscape();
	EnableControls();
	}

void C4PageLandscape::OnCheckOpenBottom() 
	{
	UpdateLandscape();
	}

void C4PageLandscape::OnCheckOpenTop() 
	{
	UpdateLandscape();
	}

void C4PageLandscape::OnCheckShowScreen() 
	{
	UpdateLandscape();
	}

void C4PageLandscape::OnCheckShowLayers() 
	{
	UpdateLandscape();
	}

void C4PageLandscape::OnRadioPlayers1() 
	{
	UpdateLandscape();
	}

void C4PageLandscape::OnRadioPlayers2() 
	{
	UpdateLandscape();
	}

void C4PageLandscape::OnRadioPlayers3() 
	{
	UpdateLandscape();
	}

void C4PageLandscape::OnRadioPlayers4() 
	{
	UpdateLandscape();
	}

void C4PageLandscape::UpdateLandscape()
	{
	// Data from control to dialog buffer
	GetLandscapeData();
	// Update view options
	m_LandscapeCtrl.EnableOptions(m_ShowScreen,m_ShowLayers,m_PreviewForPlayers+1);
	// Redraw landscape control
	m_LandscapeCtrl.Update(LandscapeData);
	}

void C4PageLandscape::EnableControls()
	{

	// Preview player selection
	m_StaticPreview.EnableWindow(m_MapPlayerExtend);
	m_RadioPlayers1.EnableWindow(m_MapPlayerExtend);
	m_RadioPlayers2.EnableWindow(m_MapPlayerExtend);
	m_RadioPlayers3.EnableWindow(m_MapPlayerExtend);
	m_RadioPlayers4.EnableWindow(m_MapPlayerExtend);

	// Landscape mode
	m_SliderWidth.EnableWindow((m_LandscapeMode == MM_Dynamic) || (m_LandscapeMode == MM_Created));
	m_SliderHeight.EnableWindow((m_LandscapeMode == MM_Dynamic) || (m_LandscapeMode == MM_Created));
	m_SliderZoom.EnableWindow(m_LandscapeMode != MM_Exact);
	m_SliderAmplitude.EnableWindow(m_LandscapeMode == MM_Dynamic);
	m_SliderPhase.EnableWindow(m_LandscapeMode == MM_Dynamic);
	m_SliderPeriod.EnableWindow(m_LandscapeMode == MM_Dynamic);
	m_SliderRandom.EnableWindow(m_LandscapeMode == MM_Dynamic);
	m_SliderWaterLevel.EnableWindow(m_LandscapeMode == MM_Dynamic);
	m_CheckShowLayers.EnableWindow(m_LandscapeMode == MM_Dynamic);
	m_CheckMapPlayerExtend.EnableWindow((m_LandscapeMode == MM_Dynamic) || (m_LandscapeMode == MM_Created));

	}

void C4PageLandscape::OnSliderChanged()
	{
	UpdateLandscape();
	UpdateZoomCtrl();
	}

void C4PageLandscape::UpdateZoomCtrl()
	{
	m_Zoom = m_SliderZoom.GetValue();
	CString sZoom; sZoom.Format("%i",m_Zoom);
	m_EditZoom.SetWindowText(sZoom);
	}

void C4PageLandscape::OnOK()
	{

	}
