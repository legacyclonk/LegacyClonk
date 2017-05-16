/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Environment page in scenario properties */

#include "C4Explorer.h"

#include "C4ScenarioDlg.h"

BEGIN_MESSAGE_MAP(C4PageEnvironment, CDialog)
	//{{AFX_MSG_MAP(C4PageEnvironment)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_CONTEXTMENU()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


C4PageEnvironment::C4PageEnvironment(CWnd* pParent /*=NULL*/)
	: CDialog(C4PageEnvironment::IDD, pParent)
	{
	//{{AFX_DATA_INIT(C4PageEnvironment)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	pDefs = NULL;

	}

void C4PageEnvironment::DoDataExchange(CDataExchange* pDX)
	{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(C4PageEnvironment)
	DDX_Control(pDX, IDC_STATICENVIRONMENT, m_StaticEnvironment);
	DDX_Control(pDX, IDC_SLIDERENVIRONMENT, m_SliderEnvironment);
	DDX_Control(pDX, IDC_LISTENVIRONMENT, m_ListEnvironment);
	DDX_Control(pDX, IDC_BUTTONSENVIRONMENT, m_ButtonsEnvironment);
	DDX_Control(pDX, IDC_SLIDERNESTS, m_SliderNests);
	DDX_Control(pDX, IDC_SLIDERANIMALS, m_SliderAnimals);
	DDX_Control(pDX, IDC_SLIDERVEGETATION, m_SliderVegetation);
	DDX_Control(pDX, IDC_SLIDERINEARTH, m_SliderInEarth);
	DDX_Control(pDX, IDC_STATICVEGETATIONTYPES, m_StaticVegetationTypes);
	DDX_Control(pDX, IDC_STATICVEGETATIONAMOUNTS, m_StaticVegetationAmounts);
	DDX_Control(pDX, IDC_STATICNESTS, m_StaticNests);
	DDX_Control(pDX, IDC_STATICINEARTHTYPES, m_StaticInEarthTypes);
	DDX_Control(pDX, IDC_STATICINEARTHAMOUNTS, m_StaticInEarthAmounts);
	DDX_Control(pDX, IDC_STATICANIMALS, m_StaticAnimals);
	DDX_Control(pDX, IDC_SLIDERVEGETATIONAMOUNT, m_SliderVegetationAmount);
	DDX_Control(pDX, IDC_SLIDERINEARTHAMOUNT, m_SliderInEarthAmount);
	DDX_Control(pDX, IDC_LISTVEGETATION, m_ListVegetation);
	DDX_Control(pDX, IDC_LISTNESTS, m_ListNests);
	DDX_Control(pDX, IDC_LISTINEARTH, m_ListInEarth);
	DDX_Control(pDX, IDC_LISTANIMALS, m_ListAnimals);
	DDX_Control(pDX, IDC_BUTTONSVEGETATION, m_ButtonsVegetation);
	DDX_Control(pDX, IDC_BUTTONSNESTS, m_ButtonsNests);
	DDX_Control(pDX, IDC_BUTTONSINEARTH, m_ButtonsInEarth);
	DDX_Control(pDX, IDC_BUTTONSANIMALS, m_ButtonsAnimals);
	//}}AFX_DATA_MAP
	}

void C4PageEnvironment::SetData(C4Scenario &rC4S)
	{
	m_ListAnimals.Set(rC4S.Animals.FreeLife);
	m_ListNests.Set(rC4S.Animals.EarthNest);
	m_ListVegetation.Set(rC4S.Landscape.Vegetation);
	m_ListInEarth.Set(rC4S.Landscape.InEarth);
	m_SliderVegetationAmount.Set(rC4S.Landscape.VegLevel);
	m_SliderInEarthAmount.Set(rC4S.Landscape.InEarthLevel);
	m_ListEnvironment.Set(rC4S.Environment.Objects);
	// Store no initialize flag
	NoInitialize = (rC4S.Head.NoInitialize!=0);
	}

void C4PageEnvironment::GetData(C4Scenario &rC4S)
	{
	m_ListAnimals.Get(rC4S.Animals.FreeLife);
	m_ListNests.Get(rC4S.Animals.EarthNest);
	m_ListVegetation.Get(rC4S.Landscape.Vegetation);
	m_ListInEarth.Get(rC4S.Landscape.InEarth);
	m_SliderVegetationAmount.Get(rC4S.Landscape.VegLevel);
	m_SliderInEarthAmount.Get(rC4S.Landscape.InEarthLevel);
	m_ListEnvironment.Get(rC4S.Environment.Objects);
	}

BOOL C4PageEnvironment::OnInitDialog() 
	{
	CDialog::OnInitDialog();	
	// Text
	UpdateText();
	// Lists
	m_ListAnimals.Init(C4D_SelectAnimal,"IDS_SELECT_ANIMALS",pDefs,&m_ButtonsAnimals,&m_SliderAnimals);
	m_ListNests.Init(C4D_SelectNest,"IDS_SELECT_NESTS",pDefs,&m_ButtonsNests,&m_SliderNests);
	m_ListVegetation.Init(C4D_SelectVegetation,"IDS_SELECT_VEGETATION",pDefs,&m_ButtonsVegetation,&m_SliderVegetation);
	m_ListInEarth.Init(C4D_SelectInEarth,"IDS_SELECT_INEARTH",pDefs,&m_ButtonsInEarth,&m_SliderInEarth);
	m_ListEnvironment.Init(C4D_Environment,"IDS_SELECT_ENVIRONMENT",pDefs,&m_ButtonsEnvironment,&m_SliderEnvironment);
	// Sliders
	m_SliderVegetationAmount.Init();
	m_SliderInEarthAmount.Init();
	// Disable no-initialize controls
	if (NoInitialize)
		{
		m_StaticVegetationTypes.EnableWindow(false);
		m_StaticVegetationAmounts.EnableWindow(false);
		m_StaticNests.EnableWindow(false);
		m_StaticInEarthTypes.EnableWindow(false);
		m_StaticInEarthAmounts.EnableWindow(false);
		m_StaticAnimals.EnableWindow(false);
		m_StaticEnvironment.EnableWindow(false);
		m_SliderVegetationAmount.EnableWindow(false);
		m_SliderInEarthAmount.EnableWindow(false);
		m_ListVegetation.EnableWindow(false);
		m_ListNests.EnableWindow(false);
		m_ListInEarth.EnableWindow(false);
		m_ListAnimals.EnableWindow(false);
		m_ListEnvironment.EnableWindow(false);
		m_SliderVegetation.EnableWindow(false);
		m_SliderNests.EnableWindow(false);
		m_SliderInEarth.EnableWindow(false);
		m_SliderAnimals.EnableWindow(false);
		m_SliderEnvironment.EnableWindow(false);
		m_ButtonsVegetation.EnableWindow(false);
		m_ButtonsNests.EnableWindow(false);
		m_ButtonsInEarth.EnableWindow(false);
		m_ButtonsAnimals.EnableWindow(false);
		m_ButtonsEnvironment.EnableWindow(false);
		}
	// Hide environment list if no environment objects available
	if (!pDefs->GetDefCount(C4D_Environment))
		{
		m_StaticEnvironment.ModifyStyle(WS_VISIBLE,0);
		m_ButtonsEnvironment.ModifyStyle(WS_VISIBLE,0);
		m_ListEnvironment.ModifyStyle(WS_VISIBLE,0);
		m_SliderEnvironment.ModifyStyle(WS_VISIBLE,0);
		}
	return TRUE;
	}

void C4PageEnvironment::UpdateText()
	{
	m_StaticAnimals.Set("IDS_CTL_ANIMALS");
	m_StaticNests.Set("IDS_CTL_NESTS");
	m_StaticVegetationTypes.Set("IDS_CTL_VEGETATIONTYPES");
	m_StaticVegetationAmounts.Set("IDS_CTL_VEGETATIONAMOUNTS");
	m_StaticInEarthTypes.Set("IDS_CTL_INEARTHTYPES");
	m_StaticInEarthAmounts.Set("IDS_CTL_INEARTHAMOUNTS");
	m_StaticEnvironment.Set("IDS_CTL_ENVIRONMENTOBJECTS");
	}

void C4PageEnvironment::OnPaint() 
	{
	CPaintDC DC(this);

	}

BOOL C4PageEnvironment::OnEraseBkgnd(CDC* pDC) 
	{
	GetApp()->dibPaper.DrawTiles(pDC, this);
	return true;
	}

void C4PageEnvironment::OnContextMenu(CWnd* pWnd, CPoint point) 
	{
	// Check id list hit
	if (!(pWnd == &m_ListAnimals || pWnd == &m_ListNests
		|| pWnd == &m_ListVegetation || pWnd == &m_ListInEarth
			|| pWnd == &m_ListEnvironment))
			return;
	// Id list ctrl context menu
	C4IdListCtrl* pIDCtrl = (C4IdListCtrl*) pWnd;
	pIDCtrl->DoContextMenu(point);
	}

void C4PageEnvironment::OnOK()
	{

	}
