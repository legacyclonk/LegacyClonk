/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Plus/Minus/Add buttons for object lists */

#include "C4Explorer.h"

//---------------------------------------------------------------------------

CDIBitmap C4IdCtrlButtons::DIB;

//---------------------------------------------------------------------------

C4IdCtrlButtons::C4IdCtrlButtons()
	{
	Height = 48 + 4;
	Width = 23;
	pUnitList = NULL;
	if (!DIB.HasContent()) 
		DIB.Load(IDB_IDCTRLBUTTONS);
	}

//---------------------------------------------------------------------------

C4IdCtrlButtons::~C4IdCtrlButtons()
{
}

//---------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(C4IdCtrlButtons, CStatic)
	//{{AFX_MSG_MAP(C4IdCtrlButtons)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//---------------------------------------------------------------------------

void C4IdCtrlButtons::OnPaint() 
{
	CPaintDC DC(this);
	DIB.Draw(DC, 0,0, 0,0, false);
}

//---------------------------------------------------------------------------

void C4IdCtrlButtons::OnLButtonDown(UINT Flags, CPoint Point) 
{
	ASSERT(pUnitList);

	// Je nach Positition Click weiterleiten
	if (Point.y < Height * ((float) 1/3)) pUnitList->PressKey(VK_ADD);
	else if (Point.y < Height * ((float) 2/3)) pUnitList->PressKey(VK_SUBTRACT);
	else pUnitList->PressKey(VK_INSERT);
}

//---------------------------------------------------------------------------

void C4IdCtrlButtons::PreSubclassWindow() 
{
	// Passende Größe einstellen
	CRect Rect;
	GetWindowRect(Rect);
	Rect.bottom = Rect.top + Height;
	Rect.right = Rect.left + Width;
	GetParent()->ScreenToClient(Rect);
	MoveWindow(Rect);
}

//---------------------------------------------------------------------------

void C4IdCtrlButtons::Set(C4IdListCtrl *pCtrl)
	{
	pUnitList = pCtrl;
	}
