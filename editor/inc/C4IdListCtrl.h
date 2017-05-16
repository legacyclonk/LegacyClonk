/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Object lists in scenario properties */

class C4IdCtrlButtons;

class C4IdListCtrl: public CListBox
	{
	public:
		C4IdListCtrl();
		~C4IdListCtrl();

	protected:
		C4IDList IDList;
		C4DefListEx *pDefs;
		DWORD Category;        // Kategorie der Objekte
		CString CategoryName;  // Kategorie als anzeigbarer Text
		bool Counts;           // Zählt auch Menge?
		int ItemHeight;        // Höhe eines Listitems (48)
		bool HasFocus;
		CSlider *pSlider;

	public:
		int GetVisibleCount();
		bool AttachSlider(CSlider *pBuddy);
		void MoveSelection(int iOffset);
		void InvalidateItem(int iIndex);
		void MoveSelectedItems(int iOffset);
		void DoContextMenu(CPoint point);
		void InsertNewItem();
		void UpdateSlider();
		void DecreaseSelectedCounts();
		void IncreaseSelectedCounts();
		void DeleteSelectedItems();
		void EnableCounts(bool fEnable);
		void Init(DWORD dwCategory, const char* idCategoryName, C4DefListEx *pDefList, C4IdCtrlButtons *pButtons=NULL, CSlider *pSlider=NULL);
		void Get(C4IDList &rList);
		void Set(C4IDList &rList);
		void FillList();
		void PressKey(UINT VKey) { OnKeyDown(VKey, 0, 0); }
		int GetSelectedItem();
		BOOL DeleteItem(int iIndex);
		BOOL SelectItem(int iIndex);
		C4ID GetSelectedID();
		
	//{{AFX_VIRTUAL(C4IdListCtrl)
	public:
		virtual void DrawItem(DRAWITEMSTRUCT* pDIS);
	protected:
		virtual void PreSubclassWindow();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

	protected:
		//{{AFX_MSG(C4IdListCtrl)
		afx_msg void OnKeyDown(UINT VKey, UINT nRep, UINT Flags);
		afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
		afx_msg void OnSetFocus(CWnd* pOldWnd);
		afx_msg void OnKillFocus(CWnd* pNewWnd);
		afx_msg void OnSelChange();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	};
