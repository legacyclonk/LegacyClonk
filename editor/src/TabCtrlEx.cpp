/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Extended tab control with owner draw tab */

#include "C4Explorer.h"

//---------------------------------------------------------------------------

CTabCtrlEx::CTabCtrlEx()
	{
	// Background
	pBkDIB = &GetApp()->dibPaper;
	}

//---------------------------------------------------------------------------

CTabCtrlEx::~CTabCtrlEx()
	{
	}

//---------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(CTabCtrlEx, CTabCtrl)
	//{{AFX_MSG_MAP(CTabCtrlEx)
	ON_WM_DRAWITEM_REFLECT()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//---------------------------------------------------------------------------

// Hierdurch wird der Tabstreifen für den Bkgrnd des Parent durchsichtig.
// Mit CtlColor klappt das nicht, da die Msg vom TabCtrl scheinbar nicht
// benutzt wird.

BOOL CTabCtrlEx::OnEraseBkgnd(CDC*) 
{
	return true;
}

//---------------------------------------------------------------------------

void CTabCtrlEx::DrawItem(DRAWITEMSTRUCT* pDIS) 
{

	CDC* pDC = CDC::FromHandle(pDIS->hDC);
	pDC->SetBkMode(TRANSPARENT);

	CRect Rect = pDIS->rcItem;
	CRect ShadowRect = Rect;
	ShadowRect.OffsetRect(1,1);

	// Rect verstellen, wenn nichtselektiertes Tab
	if (GetApp()->ComCtlVersion > 400 && !(pDIS->itemState & ODS_SELECTED))	Rect.bottom += 2;

	// Text ermitteln
	char Text[100+1];
	TC_ITEM TabItem;
	TabItem.mask = TCIF_TEXT;
	TabItem.cchTextMax = 100;
	TabItem.pszText = Text;
	GetItem(pDIS->itemID, &TabItem);

	// Textposition ermitteln
	CSize TextSize = pDC->GetTextExtent(Text);
	TextSize.cx /= 2;
	TextSize.cy /= 2;
	CPoint TextPos = Rect.CenterPoint();
	TextPos -= TextSize;

	// Textposition korrigieren
	if (pDIS->itemState & ODS_SELECTED) TextPos.y--;
	else if (GetApp()->ComCtlVersion > 400) TextPos.y++;
	
	// Farben festlegen
	COLORREF TextColor = BLACK;
	COLORREF ShadowColor = WHITE;
	if (pDIS->itemState & ODS_DISABLED) TextColor = 0x00888888;

	// Hintergrundbitmap malen
	pBkDIB->Draw(pDIS->hDC, Rect.left, Rect.top, Rect.Width(), Rect.Height(), false);

	// Text malen
	pDC->SetTextColor(ShadowColor);
	pDC->TextOut(TextPos.x+1, TextPos.y+1, Text);
	pDC->SetTextColor(TextColor);
	pDC->TextOut(TextPos.x, TextPos.y, Text);
}

//---------------------------------------------------------------------------
