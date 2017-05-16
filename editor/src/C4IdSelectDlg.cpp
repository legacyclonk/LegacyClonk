/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Object selection dialog in scenario properties */

#include "C4Explorer.h"

#include "C4IdSelectDlg.h"

//---------------------------------------------------------------------------

int C4IdSelectDlg::SortDirection;

C4DefListEx *pIdSelectDefs = NULL;

//---------------------------------------------------------------------------

C4IdSelectDlg::C4IdSelectDlg(CWnd* pParent) : CDialog(C4IdSelectDlg::IDD, pParent)
{
	// Einfache Memberinits
	App = GetApp();
	Category = C4D_All;
	IDList.Clear();
	
	pDefs = NULL;

	// Bilder laden
	//pBkDIB = &App->dibPaper;

	// Werte aus Registry laden
	iSortColumn = GetCfg()->Explorer.IDListSortColumn;
	SortDirection = +1;

	//{{AFX_DATA_INIT(C4IdSelectDlg)
	m_ViewMode = GetCfg()->Explorer.IDListViewMode;
	//}}AFX_DATA_INIT
}

//---------------------------------------------------------------------------

C4IdSelectDlg::~C4IdSelectDlg()
{
	// Save config
	GetCfg()->Explorer.IDListSortColumn= iSortColumn;
	GetCfg()->Explorer.IDListViewMode = m_ViewMode;
}

//---------------------------------------------------------------------------

void C4IdSelectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(C4IdSelectDlg)
	DDX_Control(pDX, IDC_RADIOSYMBOLS, m_RadioViewSymbols);
	DDX_Control(pDX, IDC_RADIODETAILS, m_RadioViewDetails);
	DDX_Control(pDX, IDC_STATICSELECTVIEW, m_StaticViewSelect);
	DDX_Control(pDX, IDC_LISTDEFS, m_ListDefs);
	DDX_Control(pDX, IDOK, m_ButtonOK);
	DDX_Control(pDX, IDCANCEL, m_ButtonCancel);
	DDX_Radio(pDX, IDC_RADIOSYMBOLS, m_ViewMode);
	//}}AFX_DATA_MAP
}

//---------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(C4IdSelectDlg, CDialog)
	//{{AFX_MSG_MAP(C4IdSelectDlg)
	ON_WM_ERASEBKGND()
	ON_NOTIFY(NM_DBLCLK, IDC_LISTDEFS, OnDblclkUnitList)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LISTDEFS, OnColumnClickUnitList)
	ON_BN_CLICKED(IDC_RADIODETAILS, OnRadioDetails)
	ON_BN_CLICKED(IDC_RADIOSYMBOLS, OnRadioSymbols)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//---------------------------------------------------------------------------

BOOL C4IdSelectDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// Text
	UpdateText();

	// Global def reference for sort
	pIdSelectDefs = pDefs;

	// IE3-ListViewStyles
	ListView_SetExtendedListViewStyle(m_ListDefs.m_hWnd, LVS_EX_HEADERDRAGDROP);

	// Extend title with category name
	CString Title;
	GetWindowText(Title);
	Title += ": " + CategoryName;
	SetWindowText(Title);

	// ImageList setzen
	m_ListDefs.SetImageList(&pDefs->ImgList, LVSIL_NORMAL);
	m_ListDefs.SetImageList(&pDefs->ImgList, LVSIL_SMALL);

	// Spalten setzen
	m_ListDefs.InsertColumn(CI_Name, LoadResStr("IDS_CTL_NAME"));
	m_ListDefs.InsertColumn(CI_Value, LoadResStr("IDS_CTL_VALUE"));
	m_ListDefs.InsertColumn(CI_Desc, LoadResStr("IDS_CTL_DESCRIPTION"));
	
	// Defs aus Def-Liste nach Kategorie in das ListCtrl laden
	int iUnit;
	C4Def* pCurDef;	
	for (iUnit=0; (pCurDef = pDefs->GetDef(iUnit, Category)) != 0 ; iUnit++)
	{
		// Item einfügen
		LV_ITEM Item;
		SetZero(Item);
		Item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
		Item.iItem = iUnit;
		Item.cchTextMax = C4D_MaxName + 1;
		Item.pszText = (char*) pCurDef->GetName(); // non-const cast - hope it doesn't explode!
		Item.iImage = pDefs->GetIndex(pCurDef->id);
		Item.lParam = pCurDef->id;
		int iItem = m_ListDefs.InsertItem(&Item);

		// Spaltentexte einfügen
		CString NumString;		
		NumString.Format("%i", pCurDef->Value);
		m_ListDefs.SetItemText(iItem, CI_Value, NumString);
		CString DescString;
		DescString = pCurDef->GetDesc();
		DescString.Replace("\r", ""); DescString.Replace("\n", "");
		m_ListDefs.SetItemText(iItem, CI_Desc, DescString);
	}

	// Aktuelle Ansicht einstellen	
	SetViewMode();

	return true;
}

//---------------------------------------------------------------------------

/*BOOL C4IdSelectDlg::OnEraseBkgnd(CDC* pDC) 
{
	pBkDIB->DrawTiles(pDC, this);
	return true;
}*/

//---------------------------------------------------------------------------

void C4IdSelectDlg::OnOK()
	{
	int iSelItem = -1;
	while ((iSelItem = m_ListDefs.GetNextItem(iSelItem, LVNI_SELECTED)) != -1)
		IDList.IncreaseIDCount(m_ListDefs.GetItemData(iSelItem));

	CDialog::OnOK();
	}

//---------------------------------------------------------------------------

void C4IdSelectDlg::OnDblclkUnitList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	int iSelItem = m_ListDefs.GetNextItem(-1, LVNI_SELECTED);
	if (iSelItem > -1) OnOK();
	*pResult = 0;
}

//---------------------------------------------------------------------------

void C4IdSelectDlg::SetViewMode()
{
	m_ListDefs.SetRedraw(false);

	// Style ändern
	DWORD OldStyle = GetWindowLong(m_ListDefs, GWL_STYLE);
	switch (m_ViewMode)
	{
		case 0:
			SetWindowLong(m_ListDefs, GWL_STYLE, (OldStyle & ~LVS_TYPEMASK) | LVS_ICON); break;
		case 1:
			SetWindowLong(m_ListDefs, GWL_STYLE, (OldStyle & ~LVS_TYPEMASK) | LVS_REPORT); break;
	}

	// Neu sortieren
	m_ListDefs.SortItems(&CompareUnits, iSortColumn);

	// Scrollpos 0
	m_ListDefs.EnsureVisible(0, false);

	// Spaltenbreiten automatisch anpassen
	if (m_ViewMode == 1)  // Report
	{
		int Width;
		Width = LVSCW_AUTOSIZE_USEHEADER;
		m_ListDefs.SetColumnWidth(CI_Name, Width);
		m_ListDefs.SetColumnWidth(CI_Value, Width);
		m_ListDefs.SetColumnWidth(CI_Desc, Width);
	}

	m_ListDefs.SetRedraw(true);
	m_ListDefs.Invalidate();
}

//---------------------------------------------------------------------------

void C4IdSelectDlg::OnColumnClickUnitList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

  // Sortierrichtung
	SortDirection *= -1;

	// Angeclickte Spalte ermitteln und danach sortieren
	iSortColumn = pNMListView->iSubItem;
	m_ListDefs.SortItems(&CompareUnits, iSortColumn);

	*pResult = 0;
}

//---------------------------------------------------------------------------

int CALLBACK C4IdSelectDlg::CompareUnits(LPARAM IDUnit1, LPARAM IDUnit2, LPARAM iColumn)
{

	if (!pIdSelectDefs) return 0;

	C4Def* pDef1 = pIdSelectDefs->ID2Def(IDUnit1);
	C4Def* pDef2 = pIdSelectDefs->ID2Def(IDUnit2);

	switch (iColumn)
	{
		case CI_Name:
			return lstrcmp(pDef1->GetName(), pDef2->GetName()) * SortDirection;
		case CI_Value:
			if (pDef1->Value > pDef2->Value) return -1 * SortDirection;
			else if (pDef1->Value == pDef2->Value) return 0;
			else return +1 * SortDirection;
	}

	return 0;
}

//---------------------------------------------------------------------------

void C4IdSelectDlg::UpdateText()
	{
	SetWindowText(LoadResStr("IDS_DLG_IDSELECT"));
	m_ButtonOK.Set("IDS_BTN_OK");
	m_ButtonCancel.Set("IDS_BTN_CANCEL");
	m_StaticViewSelect.Set("IDS_CTL_VIEW");
	m_RadioViewDetails.Set("IDS_CTL_DETAILS");
	m_RadioViewSymbols.Set("IDS_CTL_SYMBOLS");
	}

void C4IdSelectDlg::OnRadioDetails() 
	{
	m_ViewMode = 1;
	SetViewMode();
	}

void C4IdSelectDlg::OnRadioSymbols() 
	{
	m_ViewMode = 0;
	SetViewMode();
	}
