/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Player page in scenario properties */

class C4PagePlayerStart: public CDialog
	{

	public:
		C4PagePlayerStart(CWnd* pParent = NULL);
	
	protected:
		C4SPlrStart PlayerData[C4S_MaxPlayer];
		int SelectedPlayer;
		bool Extended;
		C4ExplorerApp* App;     
		CDIBitmap* pBkDIB; 

	public:
		C4DefListEx *pDefs;

	public:
		void SetData(C4Scenario &rC4S);
		void GetData(C4Scenario &rC4S);

	protected:
		bool NoInitialize;
		void ShowControls(bool fExtended);
		void GetPlayerData(int iPlayer);
		void SetPlayerData(int iPlayer);
		bool SelectPlayer(int iPlayer);
		void MarkPlayer(int iPlayer);
		void UpdateText();

	public:
		virtual void OnOK();
		//{{AFX_DATA(C4PagePlayerStart)
		enum { IDD = IDD_PAGEPLAYERSTART };
		CSlider	m_SliderVehicles;
		CSlider	m_SliderMaterial;
		CSlider	m_SliderMagic;
		CSlider	m_SliderKnowledge;
		CSlider	m_SliderHomebaseProduction;
		CSlider	m_SliderHomebaseMaterial;
		CSlider	m_SliderCrew;
		CSlider	m_SliderBuildings;
		int m_Wealth;
		CButtonEx m_ButtonExtend;
		CSpinButtonCtrl	m_SpinWealth;
		CStaticEx	PlayerStatic;
		CStaticEx	WealthStatic;
		CRadioEx	PlayerAllRadio;
		CRadioEx	Player1Radio;
		CRadioEx	Player2Radio;
		CRadioEx	Player3Radio;
		CRadioEx	Player4Radio;
		CEdit	WealthEdit;
		CStaticEx m_StaticCrew;
		CStaticEx	VehicleStatic;
		CStaticEx	BuildingStatic;
		CStaticEx	MaterialStatic;
		C4IdListCtrl m_ListCrew;
		C4IdListCtrl m_ListBuildings;
		C4IdListCtrl m_ListVehicles;
		C4IdListCtrl m_ListMaterial;
		C4IdCtrlButtons	m_ButtonsCrew;
		C4IdCtrlButtons	VehicleButtons;
		C4IdCtrlButtons	BuildingButtons;
		C4IdCtrlButtons	MaterialButtons;
		CStaticEx m_StaticMagic;
		CStaticEx	RegenUnitStatic;
		CStaticEx	BuyUnitStatic;
		CStaticEx	BuildUnitStatic;
		C4IdListCtrl m_ListMagic;
		C4IdListCtrl m_ListKnowledge;
		C4IdListCtrl m_ListHomebase;
		C4IdListCtrl m_ListProduction;
		C4IdCtrlButtons	BuyUnitButtons;
		C4IdCtrlButtons	m_ButtonsMagic;
		C4IdCtrlButtons	BuildUnitButtons;
		C4IdCtrlButtons	RegenUnitButtons;
	//}}AFX_DATA

		//{{AFX_VIRTUAL(C4PagePlayerStart)
		virtual void DoDataExchange(CDataExchange* pDX);
		//}}AFX_VIRTUAL

		//{{AFX_MSG(C4PagePlayerStart)
		afx_msg void OnButtonExtend();
		afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
		afx_msg BOOL OnEraseBkgnd(CDC* pDC);
		virtual BOOL OnInitDialog();
		afx_msg void OnLostFocusCrewEdit();
		afx_msg void OnLostFocusWealthEdit();
		afx_msg void OnChangingCrewSpin(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnChangingWealthSpin(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnPaint();
		afx_msg void OnClickedPlayer(UINT ID);
		//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	};
