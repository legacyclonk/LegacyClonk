/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Object selection dialog in scenario properties */

class C4IdSelectDlg: public CDialog
	{
	public:
		C4IdSelectDlg(CWnd* pParent = NULL);
		~C4IdSelectDlg();

	public:
		DWORD Category;        // Anzuzeigende Kategorie
		CString CategoryName;  // Der Kategoriename
		C4IDList IDList;       // Der Return-Wert = die ausgewählten Units
		C4DefListEx *pDefs;

		//{{AFX_DATA(C4IdSelectDlg)
		enum { IDD = IDD_IDSELECT };
		CRadioEx m_RadioViewSymbols;
		CRadioEx m_RadioViewDetails;
		CStaticEx m_StaticViewSelect;
		CListCtrl	m_ListDefs;
		CButtonEx m_ButtonOK;
		CButtonEx m_ButtonCancel;
		int		m_ViewMode;
		//}}AFX_DATA

	private:
		static int CALLBACK CompareUnits(LPARAM iUnit1, LPARAM iUnit2, LPARAM iColumn);
		void SetViewMode();

		C4ExplorerApp* App;  // Shortcut 
		int iSortColumn;
		static int SortDirection;

		enum ColumnIndex {  // Die Indizes der Spalten
			CI_Name,
			CI_Value,
			CI_Desc
			};

		//{{AFX_VIRTUAL(C4IdSelectDlg)
		protected:
		virtual void OnOK();
		virtual void DoDataExchange(CDataExchange* pDX);
		//}}AFX_VIRTUAL

	protected:
		void UpdateText();
		//{{AFX_MSG(C4IdSelectDlg)
		virtual BOOL OnInitDialog();
		//afx_msg BOOL OnEraseBkgnd(CDC* pDC);
		afx_msg void OnDblclkUnitList(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnColumnClickUnitList(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnRadioDetails();
		afx_msg void OnRadioSymbols();
		//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	};
