/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* The main content view */

const int C4VC_Player				= 0,
					C4VC_Developer		= 1,
					C4VC_Undetermined	= 2;

class C4ViewCtrl: public CTreeCtrl
	{
	
	friend class C4WebLib;
	friend class C4WebLibEntry; // allow web lib to alter view...
	friend class C4ExplorerDlg;
	friend class C4ViewItem;

	public:
		C4ViewCtrl();
		virtual ~C4ViewCtrl();
	protected:
		int Mode;
		CString RootFilename;
		CImageList ImageList;
		CImageList StateList;
		CList <C4ViewItem, C4ViewItem&> ViewItemList;
		COleDropSource OleDropDource;
	private:
		bool fDragCopy;
		bool fEditLabelOK;
		CImageList* pDragImageList;
		C4ViewItem* pDragItem;
	public:
		void OnContextMenu();
		void OnSwitchToEngine();
		bool OnLaunch();
		bool RefreshItem(const char *szItemPath);
		LRESULT OnInit(WPARAM wParam, LPARAM lParam);
		bool Init(CString &rRoot, int iMode);
		void Clear();
		void Default();
		bool OnExplode();
		void OnOK();
		void OnDefinitions();
		void OnStart();
		void OnDuplicate();
		bool OnProperties();
		bool OnReload();
		bool OnActivate();
		bool OnPack();
		bool OnRename();
		bool OnDelete();
		bool OnEdit();
		void OnNew();
		void OnItemReload();
		bool SelectViewItem(CString &rByPath);
		void SelectFirstItem();
		bool ReloadItems(const char *szItemList, bool fRefreshParents=false, bool fReloadOnly = false);
		void RemoveItem(CString byPath, bool fRemoveEditSlot=false);
		bool ReloadItem(CString &ItemPath, bool fRefreshParent=false, bool fReloadOnly = false);
		bool IsOriginal(C4ViewItem *pViewItem);
		bool EditLabelCancel();
		bool EditLabelOK();
		int AddIcon(HBITMAP hIcon);
		C4ViewItem* GetSelectedViewItem();
	private:
		bool TransferTypeCheck(int iSource, int iTarget, CString &ImportAs, const char **idImportName);
		bool TransferItem(C4ViewItem *pViewItem, C4ViewItem *pTargetItem, bool fMove);
		bool TransferItem(const char *szFilename, C4ViewItem *pTargetParent, bool fMove, bool fConfirm=true, bool fUnregistered=false);
		bool RemoveDummyItem(HTREEITEM hFromParent);
	protected:
		bool RefreshChildren(C4ViewItem *pViewItem);
		bool RefreshParentItems(C4ViewItem *pViewItem);
		bool ExplodeItem(C4ViewItem *pViewItem);
		bool ActivationByScenario;
		void SetActivationState(const char *szDefinitions, bool fFixed);
		void UpdateActivationState(C4ViewItem *pViewItem);
		void UpdateUsedEngine(C4ViewItem *pViewItem, bool fNoCheck = false);
		bool SelectViewItem(C4ViewItem *pViewItem);
		bool ModificationOkay(C4ViewItem *pViewItem, bool fDelete=false);
		void DoContextMenu(CPoint point);
		C4ViewItem * GetViewItem(const char *szByPath);
		CString GetItemTitle(const char *szFilename);
		CString GetItemTitle(C4ViewItem *pViewItem);
		CString GetFullItemTitle(const char *szFilename);
		CString GetFullItemTitle(C4ViewItem *pViewItem);
		bool PackItem(C4ViewItem *pViewItem);
		bool UnpackItem(C4ViewItem *pViewItem);
		void UpdateItemInfo(C4ViewItem *pViewItem);
		void ClearViewItems(CString byPath);
		bool LoadItem(CString &ItemPath, C4ViewItem* pParentViewItem);
		bool RefreshItem(C4ViewItem *pViewItem);
		bool Insert(C4ViewItem &rViewItem, C4ViewItem *pParentViewItem = NULL);
		bool LoadItems(C4ViewItem *pParentViewItem = NULL);
		C4ViewItem *GetViewItem(CString &byPath);
		C4ViewItem *GetParentViewItem(C4ViewItem *pViewItem);
		C4ViewItem *GetParentViewItem(HTREEITEM hItem);
		C4ViewItem* GetViewItem(HTREEITEM hItem);
		HTREEITEM GetInsertPosition(C4ViewItem *pViewItem, HTREEITEM hParentItem);

		//{{AFX_MSG(C4ViewCtrl)
		afx_msg void OnItemExpanding(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnSelChanged(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnItemExpanded(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnKeydown(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnDoubleClick(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnReturn(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnMouseMove(UINT nFlags, CPoint point);
		afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
		afx_msg void OnDropFiles( HDROP hDropInfo );
		afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
		afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
		afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	};

extern C4ViewItem *pUsedEngine;
