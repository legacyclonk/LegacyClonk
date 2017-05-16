/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Extends CButton as owner draw with bitmap background */

#include "C4Explorer.h"

BEGIN_MESSAGE_MAP(CButtonEx, CButton)
	//{{AFX_MSG_MAP(CButtonEx)
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CButtonEx::CButtonEx()
	{
	pBkDIB = &GetApp()->dibButton;
	//if (!ClickSnd.HasContent()) ClickSnd.Load(IDW_CLICK);
	}

CButtonEx::~CButtonEx()
	{
	}

void CButtonEx::DrawItem(DRAWITEMSTRUCT* pDIS)
	{

	CDC* pDC = CDC::FromHandle(pDIS->hDC);
	pDC->SetBkMode(TRANSPARENT);

	// Die verschiedenen Rechtecke ermitteln
	CRect Rect = pDIS->rcItem;
	CRect DownRect = Rect;
	CRect ShadowDownRect = Rect;
	DownRect.OffsetRect(1,1);
	ShadowDownRect.OffsetRect(2,2);

	// Buttontext ermitteln
	CString Text;
	GetWindowText(Text);

	// Farben festlegen
	COLORREF TextColor = 0x00000000;
	COLORREF ShadowColor = 0x00FFFFFF;
	if (pDIS->itemState & ODS_DISABLED) TextColor = 0x00888888;

	// Flags für DrawText festlegen
	UINT DrawTextMode = DT_CENTER | DT_VCENTER | DT_SINGLELINE;

	// Hintergrund malen
	CSize bmpsize = pBkDIB->GetSize();
	pBkDIB->Draw(*pDC, (Rect.right-bmpsize.cx)/2,(Rect.bottom-bmpsize.cy)/2, 0,0, false);

	if (pDIS->itemState & ODS_SELECTED)
		{
		// Gedrückten Button malen
		pDC->DrawEdge(&Rect, EDGE_SUNKEN, BF_RECT);
		pDC->SetTextColor(TextColor);
		pDC->DrawText(Text, DownRect, DrawTextMode);
		}
	else
		{
		// Normalen Button malen
		pDC->DrawEdge(&Rect, EDGE_RAISED, BF_RECT);
		pDC->SetTextColor(TextColor);
		pDC->DrawText(Text, Rect, DrawTextMode);
		}

	if (pDIS->itemState & ODS_FOCUS)
		{	
		// Focus malen
		CRect FocusRect = Rect;
		FocusRect.DeflateRect(3,3);
		pDC->DrawFocusRect(&FocusRect);
		}
	}

void CButtonEx::OnLButtonDown(UINT nFlags, CPoint Point) 
	{
	// Sound spielen
	CButton::OnLButtonDown(nFlags, Point);
	}

void CButtonEx::Set(const char* idCaption)
	{
	// Caption
	if (idCaption) SetWindowText(LoadResStr(idCaption));
	// Invalidate
	Invalidate();
	}

void CButtonEx::Invalidate()
	{
	CRect Rect;
	GetWindowRect(Rect); 
	GetParent()->ScreenToClient(Rect);
	GetParent()->InvalidateRect(Rect);
	}

void CButtonEx::SetBitmap(CDIBitmap *pBmp)
	{
	pBkDIB = pBmp;
	}
