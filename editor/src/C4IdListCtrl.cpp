/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Object lists in scenario properties */

#include "C4Explorer.h"

#include "C4IdSelectDlg.h"

BEGIN_MESSAGE_MAP(C4IdListCtrl, CListBox)
	//{{AFX_MSG_MAP(C4IdListCtrl)
	ON_WM_KEYDOWN()
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_CONTROL_REFLECT(LBN_SELCHANGE, OnSelChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

C4IdListCtrl::~C4IdListCtrl()
	{
	}

C4IdListCtrl::C4IdListCtrl()
	{
	Category = C4D_All;
	Counts = true;
	ItemHeight = 48;
	pDefs = NULL;
	HasFocus = false;
	pSlider = NULL;
	//PlusSnd.Load(IDW_CLICK2);
	//MinusSnd.Load(IDW_CLICK3);
	//InsertSnd.Load(IDW_CLICK);
	}

void C4IdListCtrl::DrawItem(DRAWITEMSTRUCT* pDIS) 
	{
	if (!IsWindowEnabled()) return;

	CDC* pDC = CDC::FromHandle(pDIS->hDC);
	CRect ItemRect = pDIS->rcItem;

	// Textdaten
	CString NumString;
	NumString.Format("%ix", IDList.GetCount(pDIS->itemData));
	CSize TextSize = pDC->GetTextExtent(NumString);
	int TextPosX = (ItemRect.left + ItemRect.Width() / 2) - (TextSize.cx / 2);
	int TextPosY = ItemRect.bottom - TextSize.cy;
	pDC->SetBkMode(TRANSPARENT);

	// Selected
	if (pDIS->itemState & ODS_SELECTED) 
		{
		// Background
		pDC->FillSolidRect(&pDIS->rcItem, GetSysColor(HasFocus ? COLOR_HIGHLIGHT : COLOR_BTNFACE));
		pDC->SetTextColor(HasFocus ? WHITE : BLACK);
		// Item
		pDefs->ImgList.Draw(pDC, pDefs->GetIndex(IDList.GetID(pDIS->itemData)), 
			CPoint(ItemRect.left, ItemRect.top), ILD_TRANSPARENT);
		// Count
		if (Counts) pDC->TextOut(TextPosX, TextPosY, NumString);
		}
	// Unselected
	else 
		if (pDIS->itemID != -1) 
			{
			pDC->FillSolidRect(&pDIS->rcItem, WHITE);
			pDefs->ImgList.Draw(pDC, pDefs->GetIndex(IDList.GetID(pDIS->itemData)), 
				CPoint(ItemRect.left, ItemRect.top), ILD_TRANSPARENT);
			pDC->SetTextColor(BLACK);
			if (Counts) pDC->TextOut(TextPosX, TextPosY, NumString);
			}

	// Leeren bei Focus ohne Item
	if (pDIS->itemID == -1) pDC->FillSolidRect(&pDIS->rcItem, WHITE);  
	
	// Focus malen
	if (pDIS->itemState & ODS_FOCUS) pDC->DrawFocusRect(ItemRect);
	}

HBRUSH C4IdListCtrl::CtlColor(CDC* pDC, UINT nCtlColor) 
	{
	HBRUSH hBrush;
	if (IsWindowEnabled()) hBrush = (HBRUSH) GetStockObject(WHITE_BRUSH);
	else hBrush = (HBRUSH) GetStockObject(LTGRAY_BRUSH);
	return hBrush;
	}

void C4IdListCtrl::PreSubclassWindow() 
	{
	// Größe anpassen
	CRect Rect;
	GetWindowRect(Rect);
	Rect.bottom = Rect.top + ItemHeight + 4;
	GetParent()->ScreenToClient(Rect);
	MoveWindow(Rect);
	}

void C4IdListCtrl::FillList()
	{
	// Clear
	ResetContent();
	// Fill
	for (int i = 0; IDList.GetID(i); i++)
		{
		int iItem = AddString("");
		SetItemData(iItem, i);
		}
	// Invalidate
	Invalidate();
	// Update slider
	UpdateSlider();
	}

void C4IdListCtrl::OnKeyDown(UINT VKey, UINT nRep, UINT Flags) 
	{
	switch (VKey)
		{
		case VK_ADD:
			IncreaseSelectedCounts();
			break;
		case VK_SUBTRACT:
			DecreaseSelectedCounts();
			break;
		case VK_DELETE:
			DeleteSelectedItems();
			break;
		case VK_INSERT:
			InsertNewItem();
			break;
		case VK_PRIOR:
			MoveSelectedItems(-1);
			break;
		case VK_NEXT:
			MoveSelectedItems(+1);
			break;
		default:
			CListBox::OnKeyDown(VKey,nRep,Flags);
			break;
		}
	}

void C4IdListCtrl::Set(C4IDList &rList)
	{
	// Set variable
	IDList = rList;
	// Runtime set: reinit
	if (m_hWnd) 
		{
		IDList.ConsolidateValids(*pDefs);
		FillList();
		}
	}

void C4IdListCtrl::Get(C4IDList &rList)
	{
	// Get variable
	rList = IDList;
	}

void C4IdListCtrl::Init(DWORD dwCategory, 
												const char* idCategoryName, 
												C4DefListEx *pDefList,
												C4IdCtrlButtons *pButtons, 
												CSlider *pSlider)
	{
	
	// Defition reference
	pDefs = pDefList;

	// Category and category name
	Category = dwCategory;
	CategoryName = LoadResStr(idCategoryName);
	
	// Item size
	SetItemHeight(0, ItemHeight);
	SetColumnWidth(32);
	
	// List contents
	IDList.ConsolidateValids(*pDefs);
	FillList();

	// Attach buttons
	if (pButtons)
		pButtons->Set(this);

	// Attach slider
	if (pSlider)
		AttachSlider(pSlider);

	}

void C4IdListCtrl::EnableCounts(bool fEnable)
	{
	Counts = fEnable;
	}

C4ID C4IdListCtrl::GetSelectedID()
	{
	int iSelUnit = GetCurSel();
	if (iSelUnit == LB_ERR) return C4ID_None;
	int iSelID = GetItemData(iSelUnit);
	return IDList.GetID(iSelID);
	}

BOOL C4IdListCtrl::SelectItem(int iIndex)
	{
	int iCount = GetCount();
	// List empty
	if (!Inside(iIndex,0,iCount-1)) return FALSE;
	// Deselect all
	SelItemRange(FALSE,0,iCount-1);
	// Select single item
	SetSel(iIndex,TRUE);
	// Done
	return TRUE;
	}

int C4IdListCtrl::GetSelectedItem()
	{
	int iItem;
	if (GetSelItems(1,&iItem)!=1) return -1;
	return iItem;
	}

void C4IdListCtrl::IncreaseSelectedCounts()
	{
	int iItems = GetSelCount();
	// None selected
	if (iItems<1) return;
	// Get selected items
	int *ipItems = new int [iItems];
	if (GetSelItems(iItems,ipItems)!=iItems) { delete ipItems; return; }
	// Increase counts
	for (int cnt=0; cnt<iItems; cnt++)
		{
		// Determine max count
		int iMaxCount = 10;
		C4Def* pDef = pDefs->ID2Def(IDList.GetID(ipItems[cnt]));
		if (pDef) iMaxCount = pDef->MaxUserSelect;
		// Set new count
		int iCount = IDList.GetCount(ipItems[cnt]);
		if (iCount + 1 <= iMaxCount) iCount++;
		if (!Counts) iCount=0;
		IDList.SetCount(ipItems[cnt],iCount);
		// Invalidate
		InvalidateItem(ipItems[cnt]);
		}	
	// Deallocate buffer
	delete ipItems;
	}

void C4IdListCtrl::DecreaseSelectedCounts()
	{
	int iItems = GetSelCount();
	// None selected
	if (iItems<1) return;
	// Get selected items
	int *ipItems = new int [iItems];
	if (GetSelItems(iItems,ipItems)!=iItems) { delete ipItems; return; }
	// Decrease counts
	for (int cnt=0; cnt<iItems; cnt++)
		{
		// Determine max count
		int iMaxCount = 10;
		C4Def* pDef = pDefs->ID2Def(IDList.GetID(ipItems[cnt]));
		if (pDef) iMaxCount = pDef->MaxUserSelect;
		// Set new count
		int iCount = IDList.GetCount(ipItems[cnt]);
		if (iCount > 0) iCount--;
		if (!Counts) iCount=0;
		IDList.SetCount(ipItems[cnt],iCount);
		// Invalidate
		InvalidateItem(ipItems[cnt]);
		}	
	// Remove empty counts (back to front)
	for (int cnt=iItems-1; cnt>=0; cnt--)
		if (!IDList.GetCount(ipItems[cnt]))
			{
			IDList.DeleteItem(ipItems[cnt]);
			DeleteItem(ipItems[cnt]);
			}
	// Deallocate buffer
	delete ipItems;
	}

void C4IdListCtrl::InsertNewItem()
	{
	// Do selection dialog
	C4IdSelectDlg SelectDlg;
	SelectDlg.Category = Category;
	SelectDlg.CategoryName = CategoryName;
	SelectDlg.pDefs = pDefs;
	if (SelectDlg.DoModal() != IDOK) return;
	// Add selected items
	IDList.Add(SelectDlg.IDList);
	// Cutoff max counts
	C4ID ID; 
	int32_t Count; 
	C4Def* pDef;
	for (int i=0; IDList.GetID(i) != C4ID_None; i++)
		{
		ID = IDList.GetID(i, &Count);
		pDef = pDefs->ID2Def(ID);
		if (ID && pDef)	IDList.SetIDCount(ID, min(Count, pDef->MaxUserSelect));
		}
	// Sort list
	IDList.SortByCategory(*pDefs);
	// Refill list
	FillList();
	}

void C4IdListCtrl::OnSetFocus(CWnd* pOldWnd) 
	{
	CListBox::OnSetFocus(pOldWnd);
	HasFocus=true;
	Invalidate();
	}

void C4IdListCtrl::OnKillFocus(CWnd* pNewWnd) 
	{
	CListBox::OnKillFocus(pNewWnd);
	HasFocus=false;
	Invalidate();
	}

void C4IdListCtrl::DeleteSelectedItems()
	{
	int iItems = GetSelCount();
	// None selected
	if (iItems<1) return;
	// Get selected items
	int *ipItems = new int [iItems];
	if (GetSelItems(iItems,ipItems)!=iItems) { delete ipItems; return; }
	// Delete items (back to front)
	for (int cnt=iItems-1; cnt>=0; cnt--)
		{
		IDList.DeleteItem(ipItems[cnt]);
		DeleteItem(ipItems[cnt]);
		}
	// Deallocate buffer
	delete ipItems;
	}

BOOL C4IdListCtrl::DeleteItem(int iIndex)
	{
	// Delete list box item
	if (DeleteString(iIndex)==LB_ERR) return FALSE;
	// Decrease list box item data (id list item index) for all items above
	DWORD dwData;
	while ((dwData=GetItemData(iIndex))!=LB_ERR)
		SetItemData(iIndex++,dwData-1);
	// Update slider
	UpdateSlider();
	// Done
	return TRUE;
	}

void C4IdListCtrl::DoContextMenu(CPoint point)
	{
	CString sText;
	// Gain focus
	SetFocus();
	// Get hit item
	CPoint ClientPoint = point;
	ScreenToClient(&ClientPoint);
	BOOL Dummy;
	int iItem = ItemFromPoint(ClientPoint, Dummy);
	// Select single if not on selected
	if (!GetSel(iItem)) SelectItem(iItem);
	// Get selected item info
	C4Def *pDef = pDefs->ID2Def(GetSelectedID());
	UINT uDisable = 0; if (!pDef) uDisable = MF_GRAYED;
	// Create menu
	CMenu Menu;
	Menu.CreatePopupMenu();
	sText.Format("+ %s",(GetSelCount()>1) ? LoadResStr("IDS_MENU_MULTIPLEITEMS") : (pDef ? pDef->GetName() : ""));
	Menu.AppendMenu(MF_STRING | uDisable,1,sText);
	sText.Format("-  %s",(GetSelCount()>1) ? LoadResStr("IDS_MENU_MULTIPLEITEMS") : (pDef ? pDef->GetName() : ""));
	Menu.AppendMenu(MF_STRING | uDisable,2,sText);
	Menu.AppendMenu(MF_SEPARATOR,0,"");
	Menu.AppendMenu(MF_STRING,3,LoadResStr("IDS_MENU_INSERT"));
	Menu.AppendMenu(MF_STRING | uDisable,4,LoadResStr("IDS_MENU_DELETE"));
	// Execute menu
	int MenuID = Menu.TrackPopupMenu( TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTBUTTON, point.x, point.y, this);
	Menu.DestroyMenu();
	// Command to list control
	switch (MenuID)
		{
		case 1: PressKey(VK_ADD); break;
		case 2: PressKey(VK_SUBTRACT); break;
		case 3: PressKey(VK_INSERT); break;
		case 4: PressKey(VK_DELETE); break;
		}
	}

void C4IdListCtrl::MoveSelectedItems(int iOffset)
	{
	int cnt;
	int iItems = GetSelCount();
	int iTotal = GetCount();
	// None selected
	if (iItems<1) return;
	// Get selected items
	int *ipItems = new int [iItems];
	if (GetSelItems(iItems,ipItems)!=iItems) { delete ipItems; return; }
	// Downwards (front to back)
	if (iOffset<0)
		for (cnt=0; cnt<iItems; cnt++)
			{
			// Outside boundary
			if (ipItems[cnt]+iOffset<0) break;
			// Move item
			IDList.SwapItems(ipItems[cnt],ipItems[cnt]+iOffset);
			InvalidateItem(ipItems[cnt]);
			InvalidateItem(ipItems[cnt]+iOffset);
			}
	// Move items upwards (back to front)
	if (iOffset>0)
		for (int cnt=iItems-1; cnt>=0; cnt--)
			{
			// Outside boundary
			if (ipItems[cnt]+iOffset>=iTotal) break;
			// Move item
			IDList.SwapItems(ipItems[cnt],ipItems[cnt]+iOffset);
			InvalidateItem(ipItems[cnt]);
			InvalidateItem(ipItems[cnt]+iOffset);
			}
	// Move selection
	MoveSelection(iOffset);
	// Deallocate buffer
	delete ipItems;
	}

void C4IdListCtrl::InvalidateItem(int iIndex)
	{
	CRect ItemRect;
	GetItemRect(iIndex, ItemRect);
	InvalidateRect(ItemRect);
	}

void C4IdListCtrl::MoveSelection(int iOffset)
	{
	int cnt;
	int iTotal = GetCount();
	// Downwards (front to back)
	if (iOffset<0)
		if (!GetSel(0))
			for (cnt=0; cnt<iTotal; cnt++)
				if (cnt-iOffset<iTotal) SetSel(cnt,GetSel(cnt-iOffset));
				else SetSel(cnt,FALSE);
	// Upwards (back to front)
	if (iOffset>0)
		if (!GetSel(iTotal-1))
			for (cnt=iTotal-1; cnt>=0; cnt--)
				if (cnt-iOffset>=0) SetSel(cnt,GetSel(cnt-iOffset));
				else SetSel(cnt,FALSE);
	}
	
BOOL C4IdListCtrl::OnCommand(WPARAM wParam, LPARAM lParam) 
	{

	switch (HIWORD(wParam))
		{
		case SLN_CHANGE:
			// Scroll view
			int iTopIndexRange = GetCount()-GetVisibleCount();
			if (pSlider)
				SetTopIndex( pSlider->GetValue()*iTopIndexRange/100 );
			return TRUE;
		}	

	return CListBox::OnCommand(wParam, lParam);
	}

bool C4IdListCtrl::AttachSlider(CSlider *pBuddy)
	{
	// Set slider pointer
	pSlider=pBuddy;
	// Attach slider
	pSlider->Attach((CWnd*)this);
	// Update slider
	UpdateSlider();
	// Done
	return true;
	}

void C4IdListCtrl::OnSelChange() 
	{
	// Scroll slider
	int iTopIndexRange = GetCount()-GetVisibleCount();
	if (pSlider)
		if (iTopIndexRange)
			pSlider->SetValue( 100*GetTopIndex()/iTopIndexRange );
	}

int C4IdListCtrl::GetVisibleCount()
	{
	CRect rctClient; GetClientRect(&rctClient);
	return rctClient.Width()/48+2;
	}

void C4IdListCtrl::UpdateSlider()
	{
	if (!pSlider) return;
	// Set slider visibility
	if ( (GetStyle() & WS_VISIBLE) && (GetCount()>GetVisibleCount()) )
		{ pSlider->ModifyStyle(0,WS_VISIBLE); pSlider->Invalidate(); }
	else
		{ pSlider->ModifyStyle(WS_VISIBLE,0); pSlider->Invalidate(); }
	}	
