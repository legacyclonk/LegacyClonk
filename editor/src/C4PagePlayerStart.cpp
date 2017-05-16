/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Player page in scenario properties */

#include "C4Explorer.h"

#include "C4ScenarioDlg.h"

BEGIN_MESSAGE_MAP(C4PagePlayerStart, CDialog)
	//{{AFX_MSG_MAP(C4PagePlayerStart)
	ON_BN_CLICKED(IDC_BUTTONEXTEND, OnButtonExtend)
	ON_WM_ERASEBKGND()
	ON_EN_KILLFOCUS(IDC_EQUIPWEALTHEDIT, OnLostFocusWealthEdit)
	ON_NOTIFY(UDN_DELTAPOS, IDC_EQUIPWEALTHSPIN, OnChangingWealthSpin)
	ON_WM_PAINT()
	ON_WM_CONTEXTMENU()
	ON_CONTROL_RANGE(BN_CLICKED, IDC_EQUIPPLAYER1, IDC_EQUIPPLAYERALL, OnClickedPlayer)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
	
C4PagePlayerStart::C4PagePlayerStart(CWnd* pParent) : CDialog(C4PagePlayerStart::IDD, pParent)
	{
	// Einfache Memberinits
	App = GetApp();
	SelectedPlayer = 0;
	pDefs = NULL;
	Extended = false;

	// Bilder laden
	pBkDIB = &App->dibPaper;

	//{{AFX_DATA_INIT(C4PagePlayerStart)
	//}}AFX_DATA_INIT
	}

void C4PagePlayerStart::DoDataExchange(CDataExchange* pDX)
	{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(C4PagePlayerStart)
	DDX_Control(pDX, IDC_SLIDERVEHICLES, m_SliderVehicles);
	DDX_Control(pDX, IDC_SLIDERMATERIAL, m_SliderMaterial);
	DDX_Control(pDX, IDC_SLIDERMAGIC, m_SliderMagic);
	DDX_Control(pDX, IDC_SLIDERKNOWLEDGE, m_SliderKnowledge);
	DDX_Control(pDX, IDC_SLIDERHOMEBASEPROD, m_SliderHomebaseProduction);
	DDX_Control(pDX, IDC_SLIDERHOMEBASEMAT, m_SliderHomebaseMaterial);
	DDX_Control(pDX, IDC_SLIDERCREW, m_SliderCrew);
	DDX_Control(pDX, IDC_SLIDERBUILDINGS, m_SliderBuildings);
	DDX_Text(pDX, IDC_EQUIPWEALTHEDIT, m_Wealth);
	DDX_Control(pDX, IDC_BUTTONEXTEND, m_ButtonExtend);
	DDX_Control(pDX, IDC_EQUIPWEALTHSPIN, m_SpinWealth);
	DDX_Control(pDX, IDC_EQUIPPLAYERSTC, PlayerStatic);
	DDX_Control(pDX, IDC_EQUIPWEALTHSTC, WealthStatic);
	DDX_Control(pDX, IDC_EQUIPPLAYERALL, PlayerAllRadio);
	DDX_Control(pDX, IDC_EQUIPPLAYER1, Player1Radio);
	DDX_Control(pDX, IDC_EQUIPPLAYER2, Player2Radio);
	DDX_Control(pDX, IDC_EQUIPPLAYER3, Player3Radio);
	DDX_Control(pDX, IDC_EQUIPPLAYER4, Player4Radio);
	DDX_Control(pDX, IDC_EQUIPWEALTHEDIT, WealthEdit);
	DDX_Control(pDX, IDC_STATICCREW, m_StaticCrew);
	DDX_Control(pDX, IDC_EQUIPVEHICLESTC, VehicleStatic);
	DDX_Control(pDX, IDC_EQUIPBUILDINGSTC, BuildingStatic);
	DDX_Control(pDX, IDC_EQUIPMATERIALSTC, MaterialStatic);
	DDX_Control(pDX, IDC_LISTCREW, m_ListCrew);
	DDX_Control(pDX, IDC_EQUIPBUILDINGLIST, m_ListBuildings);
	DDX_Control(pDX, IDC_EQUIPVEHICLELIST, m_ListVehicles);
	DDX_Control(pDX, IDC_EQUIPMATERIALLIST, m_ListMaterial);
	DDX_Control(pDX, IDC_BUTTONSCREW, m_ButtonsCrew);
	DDX_Control(pDX, IDC_EQUIPVEHICLEBTNS, VehicleButtons);
	DDX_Control(pDX, IDC_EQUIPBUILDINGBTNS, BuildingButtons);
	DDX_Control(pDX, IDC_EQUIPMATERIALBTNS, MaterialButtons);
	DDX_Control(pDX, IDC_STATICMAGIC, m_StaticMagic);
	DDX_Control(pDX, IDC_EQUIPREGENUNITSTC, RegenUnitStatic);
	DDX_Control(pDX, IDC_EQUIPBUYUNITSTC, BuyUnitStatic);
	DDX_Control(pDX, IDC_EQUIPBUILDUNITSTC, BuildUnitStatic);
	DDX_Control(pDX, IDC_LISTMAGIC, m_ListMagic);
	DDX_Control(pDX, IDC_EQUIPBUILDUNITLIST, m_ListKnowledge);
	DDX_Control(pDX, IDC_EQUIPBUYUNITLIST, m_ListHomebase);
	DDX_Control(pDX, IDC_EQUIPREGENUNITLIST, m_ListProduction);
	DDX_Control(pDX, IDC_EQUIPBUYUNITBTNS, BuyUnitButtons);
	DDX_Control(pDX, IDC_BUTTONSMAGIC, m_ButtonsMagic);
	DDX_Control(pDX, IDC_EQUIPBUILDUNITBTNS, BuildUnitButtons);
	DDX_Control(pDX, IDC_EQUIPREGENUNITBTNS, RegenUnitButtons);
	//}}AFX_DATA_MAP
	}

BOOL C4PagePlayerStart::OnInitDialog() 
	{
	CDialog::OnInitDialog();
	
	// Text
	UpdateText();

	// Spin control
	WealthEdit.SetMargins(3,3);
	m_SpinWealth.SetRange( PlayerData[0].Wealth.Min, PlayerData[0].Wealth.Max );

	// Lists
	m_ListCrew.Init(C4D_CrewMember,"IDS_SELECT_CREW",pDefs);
	m_ListBuildings.Init(C4D_SelectBuilding,"IDS_SELECT_STRUCTURES",pDefs);
	m_ListVehicles.Init(C4D_SelectVehicle,"IDS_SELECT_VEHICLES",pDefs);
	m_ListMaterial.Init(C4D_SelectMaterial,"IDS_SELECT_MATERIAL",pDefs);
	m_ListKnowledge.Init(C4D_SelectKnowledge,"IDS_SELECT_KNOWLEDGE",pDefs);
	m_ListHomebase.Init(C4D_SelectHomebase,"IDS_SELECT_HOMEBASE",pDefs);
	m_ListProduction.Init(C4D_SelectHomebase,"IDS_SELECT_PRODUCTION",pDefs);
	m_ListMagic.Init(C4D_Magic,"IDS_SELECT_MAGIC",pDefs);
	m_ListMagic.EnableCounts(false);
	m_ListKnowledge.EnableCounts(false);

	// List sliders
	m_ListCrew.AttachSlider(&m_SliderCrew);
	m_ListBuildings.AttachSlider(&m_SliderBuildings);
	m_ListVehicles.AttachSlider(&m_SliderVehicles);
	m_ListMaterial.AttachSlider(&m_SliderMaterial);
	m_ListKnowledge.AttachSlider(&m_SliderKnowledge);
	m_ListHomebase.AttachSlider(&m_SliderHomebaseMaterial);
	m_ListProduction.AttachSlider(&m_SliderHomebaseProduction);
	m_ListMagic.AttachSlider(&m_SliderMagic);

	// Buttons zuordnen
	m_ButtonsCrew.Set(&m_ListCrew);
	m_ButtonsMagic.Set(&m_ListMagic);
	BuildingButtons.Set(&m_ListBuildings);
	VehicleButtons.Set(&m_ListVehicles);
	MaterialButtons.Set(&m_ListMaterial);
	BuildUnitButtons.Set(&m_ListKnowledge);
	BuyUnitButtons.Set(&m_ListHomebase);
	RegenUnitButtons.Set(&m_ListProduction);

	// Mark selected players
	MarkPlayer(SelectedPlayer);

	// Show controls
	ShowControls(Extended);

	// Disable no initialize controls
	if (NoInitialize)
		{
		BuildingButtons.EnableWindow(false);
		BuildingStatic.EnableWindow(false);
		m_ListBuildings.EnableWindow(false);
		m_SliderBuildings.EnableWindow(false);
		MaterialButtons.EnableWindow(false);
		MaterialStatic.EnableWindow(false);
		m_ListMaterial.EnableWindow(false);
		m_SliderMaterial.EnableWindow(false);
		VehicleButtons.EnableWindow(false);
		VehicleStatic.EnableWindow(false);
		m_ListVehicles.EnableWindow(false);
		m_SliderVehicles.EnableWindow(false);
		}

	return true;
	}

BOOL C4PagePlayerStart::OnEraseBkgnd(CDC* pDC) 
	{
	pBkDIB->DrawTiles(pDC, this);
	return true;
	}

void C4PagePlayerStart::OnPaint() 
	{
	CPaintDC DC(this);

	}

void C4PagePlayerStart::MarkPlayer(int iPlayer)
	{
	CheckRadioButton(IDC_EQUIPPLAYER1, IDC_EQUIPPLAYERALL, IDC_EQUIPPLAYER1 + iPlayer);
	}

bool C4PagePlayerStart::SelectPlayer(int iPlayer)
	{
	// No change
	if (iPlayer == SelectedPlayer) return false;
	// Query for all player
	if (iPlayer == C4S_MaxPlayer)
		if (C4MessageBox("IDS_MSG_ALLPLAYERDATA",true) != IDOK)
			{
			// Keep old mark
			MarkPlayer(SelectedPlayer);
			return false;
			}
	// Get current data
	GetPlayerData(SelectedPlayer);
	// Equalize all players by selected
	if (iPlayer == C4S_MaxPlayer)
		{
		for (int cnt=0; cnt<C4S_MaxPlayer; cnt++)
			if (cnt != SelectedPlayer)
				PlayerData[cnt] = PlayerData[SelectedPlayer];
		}
	// Select new player
	SelectedPlayer = iPlayer;
	// Set new data
	SetPlayerData(SelectedPlayer);
	// Mark new player
	MarkPlayer(SelectedPlayer);
	// Ok
	return true;
	}

void C4PagePlayerStart::OnClickedPlayer(UINT ID)
	{
  // Abwahl ignorieren, sonst DuroLoop(tm)!
	if (!((CButton*)GetDlgItem(ID))->GetCheck()) return;
	// Neuen Spieler ermitteln
	int iNewPlayer = ID - IDC_EQUIPPLAYER1;
	// Selektieren
	SelectPlayer(iNewPlayer);
	}

void C4PagePlayerStart::OnLostFocusWealthEdit()
	{
	// Wert limitieren
	LimitEditValue(WealthEdit, PlayerData[0].Wealth.Min, PlayerData[0].Wealth.Max);
	}

void C4PagePlayerStart::OnChangingWealthSpin(NMHDR* pNMHDR, LRESULT* pResult) 
	{
	OnLostFocusWealthEdit();
	*pResult = 0;
	}

void C4PagePlayerStart::SetData(C4Scenario &rC4S)
	{
	// Data to dialog buffer
	for (int cnt=0; cnt<C4S_MaxPlayer; cnt++)
		{
		PlayerData[cnt] = rC4S.PlrStart[cnt];
		// Adjust old style specifications
		if (PlayerData[cnt].ReadyCrew.IsClear()) // Crew
			{
			C4ID idCrew = C4ID_Clonk; if (PlayerData[cnt].NativeCrew!=C4ID_None) idCrew = PlayerData[cnt].NativeCrew;
			PlayerData[cnt].ReadyCrew.SetIDCount( idCrew, PlayerData[cnt].Crew.Evaluate(), TRUE );
			}
		if (PlayerData[cnt].Magic.IsClear()) // Magic
			{
			PlayerData[cnt].Magic.Load(*pDefs,C4D_Magic);
			}
		// Additional validity check on magic selection (don't show ROCK=1 hack in dialog)
		PlayerData[cnt].Magic.ConsolidateValids(*pDefs, C4D_Magic);
		}
	// Determine selected player
	SelectedPlayer = 0; 
	// All players selected
	if ( PlayerData[0].EquipmentEqual(PlayerData[1])
		&& PlayerData[1].EquipmentEqual(PlayerData[2])
		&& PlayerData[2].EquipmentEqual(PlayerData[3]) )
			SelectedPlayer = C4S_MaxPlayer;
	// Data to controls
	SetPlayerData(SelectedPlayer);
	// Store no initialize flag
	NoInitialize = (rC4S.Head.NoInitialize!=0);
	}

void C4PagePlayerStart::GetData(C4Scenario &rC4S)
	{
	// Data from controls
	GetPlayerData(SelectedPlayer);
	// Data from dialog buffer
	for (int cnt=0; cnt<C4S_MaxPlayer; cnt++)
		{
		rC4S.PlrStart[cnt] = PlayerData[cnt];
		// Magic selection empty: player obviously wants no spells to be active
		if (rC4S.PlrStart[cnt].Magic.IsClear())
			// Ye olde hack: put something invalid into the list to prevent all-magic-default
			rC4S.PlrStart[cnt].Magic.IncreaseIDCount(C4Id("ROCK"));		
		}
	}

void C4PagePlayerStart::UpdateText()
	{
	m_ButtonExtend.Set("IDS_BTN_EXTENDED");
	m_StaticCrew.Set("IDS_CTL_CREW");
	m_StaticMagic.Set("IDS_CTL_MAGIC2");
	VehicleStatic.Set("IDS_CTL_VEHICLES");
	BuildingStatic.Set("IDS_CTL_BUILDINGS");
	PlayerStatic.Set("IDS_CTL_VALUESFOR");
	WealthStatic.Set("IDS_CTL_WEALTH");
	PlayerAllRadio.Set("IDS_CTL_ALL");
	Player1Radio.Set("IDS_CTL_PLAYER1");
	Player2Radio.Set("IDS_CTL_PLAYER2");
	Player3Radio.Set("IDS_CTL_PLAYER3");
	Player4Radio.Set("IDS_CTL_PLAYER4");
	MaterialStatic.Set("IDS_CTL_MATERIAL");
	BuildUnitStatic.Set("IDS_CTL_KNOWLEDGE");
	RegenUnitStatic.Set("IDS_CTL_PRODUCTION");
	BuyUnitStatic.Set("IDS_CTL_HOMEBASE");
	}

void C4PagePlayerStart::SetPlayerData(int iPlayer)
	{
	// Take first player for all
	if (iPlayer == C4S_MaxPlayer) iPlayer = 0;
	// Set control variables
	m_Wealth = PlayerData[iPlayer].Wealth.Std;
	m_ListCrew.Set(PlayerData[iPlayer].ReadyCrew);
	m_ListMagic.Set(PlayerData[iPlayer].Magic);
	m_ListBuildings.Set(PlayerData[iPlayer].ReadyBase);
	m_ListVehicles.Set(PlayerData[iPlayer].ReadyVehic);
	m_ListMaterial.Set(PlayerData[iPlayer].ReadyMaterial);
	m_ListKnowledge.Set(PlayerData[iPlayer].BuildKnowledge);
	m_ListHomebase.Set(PlayerData[iPlayer].HomeBaseMaterial);
	m_ListProduction.Set(PlayerData[iPlayer].HomeBaseProduction);
	// Runtime update
	if (m_hWnd) UpdateData(false);
	}

void C4PagePlayerStart::GetPlayerData(int iPlayer)
	{
	// Runtime update
	if (m_hWnd) UpdateData(true);
	// All players
	if (iPlayer == C4S_MaxPlayer)
		{
		for (int cnt=0; cnt<C4S_MaxPlayer; cnt++)
			{
			PlayerData[cnt].Wealth.Std = m_Wealth;
			m_ListCrew.Get(PlayerData[cnt].ReadyCrew);
			m_ListMagic.Get(PlayerData[cnt].Magic);
			m_ListBuildings.Get(PlayerData[cnt].ReadyBase);
			m_ListVehicles.Get(PlayerData[cnt].ReadyVehic);
			m_ListMaterial.Get(PlayerData[cnt].ReadyMaterial);
			m_ListKnowledge.Get(PlayerData[cnt].BuildKnowledge);
			m_ListHomebase.Get(PlayerData[cnt].HomeBaseMaterial);
			m_ListProduction.Get(PlayerData[cnt].HomeBaseProduction);
			}
		}
	// One player
	else
		{
		PlayerData[iPlayer].Wealth.Std = m_Wealth;
		m_ListCrew.Get(PlayerData[iPlayer].ReadyCrew);
		m_ListMagic.Get(PlayerData[iPlayer].Magic);
		m_ListBuildings.Get(PlayerData[iPlayer].ReadyBase);
		m_ListVehicles.Get(PlayerData[iPlayer].ReadyVehic);
		m_ListMaterial.Get(PlayerData[iPlayer].ReadyMaterial);
		m_ListKnowledge.Get(PlayerData[iPlayer].BuildKnowledge);
		m_ListHomebase.Get(PlayerData[iPlayer].HomeBaseMaterial);
		m_ListProduction.Get(PlayerData[iPlayer].HomeBaseProduction);
		}
	}

void C4PagePlayerStart::OnContextMenu(CWnd* pWnd, CPoint point) 
	{
	CString sText;
	// Check id list hit
	if (!(pWnd == &m_ListBuildings || pWnd == &m_ListVehicles
		|| pWnd == &m_ListCrew || pWnd == &m_ListMagic
		|| pWnd == &m_ListMaterial || pWnd == &m_ListKnowledge
		|| pWnd == &m_ListHomebase || pWnd == &m_ListProduction))
			return;
	// Id list ctrl context menu
	C4IdListCtrl* pIDCtrl = (C4IdListCtrl*) pWnd;
	pIDCtrl->DoContextMenu(point);
	}

void C4PagePlayerStart::OnButtonExtend()
	{
	Extended = !Extended;
	m_ButtonExtend.Set(Extended ? "IDS_BTN_SIMPLE" : "IDS_BTN_EXTENDED");
	ShowControls(Extended);
	}

void C4PagePlayerStart::ShowControls(bool fExtended)
	{
	DWORD dwAdd,dwRemove;
	
	if (!fExtended) { dwAdd=0; dwRemove=WS_VISIBLE; } else { dwAdd=WS_VISIBLE; dwRemove=0; }
	m_StaticCrew.ModifyStyle(dwAdd,dwRemove);
	VehicleStatic.ModifyStyle(dwAdd,dwRemove);
	BuildingStatic.ModifyStyle(dwAdd,dwRemove);
	MaterialStatic.ModifyStyle(dwAdd,dwRemove);
	m_ListCrew.ModifyStyle(dwAdd,dwRemove);
	m_ListBuildings.ModifyStyle(dwAdd,dwRemove);
	m_ListVehicles.ModifyStyle(dwAdd,dwRemove);
	m_ListMaterial.ModifyStyle(dwAdd,dwRemove);
	m_ButtonsCrew.ModifyStyle(dwAdd,dwRemove);
	VehicleButtons.ModifyStyle(dwAdd,dwRemove);
	BuildingButtons.ModifyStyle(dwAdd,dwRemove);
	MaterialButtons.ModifyStyle(dwAdd,dwRemove);

	if (fExtended) { dwAdd=0; dwRemove=WS_VISIBLE; } else { dwAdd=WS_VISIBLE; dwRemove=0; }
	m_StaticMagic.ModifyStyle(dwAdd,dwRemove);
	RegenUnitStatic.ModifyStyle(dwAdd,dwRemove);
	BuyUnitStatic.ModifyStyle(dwAdd,dwRemove);
	BuildUnitStatic.ModifyStyle(dwAdd,dwRemove);
	m_ListMagic.ModifyStyle(dwAdd,dwRemove);
	m_ListKnowledge.ModifyStyle(dwAdd,dwRemove);
	m_ListHomebase.ModifyStyle(dwAdd,dwRemove);
	m_ListProduction.ModifyStyle(dwAdd,dwRemove);
	BuyUnitButtons.ModifyStyle(dwAdd,dwRemove);
	m_ButtonsMagic.ModifyStyle(dwAdd,dwRemove);
	BuildUnitButtons.ModifyStyle(dwAdd,dwRemove);
	RegenUnitButtons.ModifyStyle(dwAdd,dwRemove);

	m_ListCrew.UpdateSlider();
	m_ListBuildings.UpdateSlider();
	m_ListVehicles.UpdateSlider();
	m_ListMaterial.UpdateSlider();
	m_ListMagic.UpdateSlider();
	m_ListKnowledge.UpdateSlider();
	m_ListHomebase.UpdateSlider();
	m_ListProduction.UpdateSlider();

	Invalidate();
	}

void C4PagePlayerStart::OnOK()
{

}
