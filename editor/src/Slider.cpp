/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Slider control */

#include "C4Explorer.h"

BEGIN_MESSAGE_MAP(CSlider, CStatic)
	//{{AFX_MSG_MAP(CSlider)
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_KEYDOWN()
	ON_WM_KILLFOCUS()
	ON_WM_RBUTTONDOWN()
	ON_WM_SETFOCUS()
	ON_WM_RBUTTONUP()
	ON_WM_GETDLGCODE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CSlider::CSlider(): HThumbWidth(5)
	{
	iValue = 0;
	iRandom = 0;
	ValPos = 5;
	RndPos = 0;
	MinValue = 0;
	MaxValue = 100;
	MinRandom = 0;
	MaxRandom = 100;
	HasRandom = true;
	Sound = true;
	Color = GetSysColor(COLOR_BTNFACE);
	FocusColor = GetSysColor(COLOR_HIGHLIGHT);
	BackgroundColor = WHITE;
	pNotify = NULL;
	}

CSlider::~CSlider()
	{
	}

int CSlider::GetValue()
	{
	// hWnd muß hier schon bestehen
	ASSERT(m_hWnd);

	CRect ClientRect;
	GetClientRect(ClientRect);
	return Scale(ValPos, HThumbWidth, ClientRect.right - HThumbWidth, MinValue, MaxValue);
	}

int CSlider::GetRandom()
	{
	// hWnd muß hier schon bestehen
	ASSERT(m_hWnd);

	CRect ClientRect;
	GetClientRect(ClientRect);
	return Scale(RndPos, 0, ClientRect.right, MinRandom, MaxRandom); 
	}

void CSlider::SetValue(int NewValue)
	{
	// hWnd muß hier schon bestehen
	//ASSERT(m_hWnd);

	// Neuen Value limitieren und setzen
	int GivenValue = NewValue;
	NewValue = Limit(NewValue, MinValue, MaxValue);
	if (GivenValue != NewValue) TRACE("\nCSlider::SetValue(): out of range!");

	if (!m_hWnd) return;

	CRect ClientRect;
	GetClientRect(ClientRect);
	ValPos = Scale(NewValue, MinValue, MaxValue, HThumbWidth, ClientRect.right - HThumbWidth);
	ValPos = max(ValPos, HThumbWidth);  // Scale ist nicht so ganz sicher...

	// Neuzeichnen lassen
	InvalRect = ClientRect;
	InvalidateRect(InvalRect);
	}

void CSlider::SetRandom(int NewRandom)
	{
	// hWnd muß hier schon bestehen
	ASSERT(m_hWnd);

	// Neuen RndPos limitieren und setzen
	int GivenRandom = NewRandom;
	NewRandom = Limit(NewRandom, MinRandom, MaxRandom);
	if (GivenRandom != NewRandom) TRACE("\nCSlider::SetRandom(): out of range!");
	CRect ClientRect;
	GetClientRect(ClientRect);
	RndPos = Scale(NewRandom, MinRandom, MaxRandom, 0, ClientRect.right);

	// Neuzeichnen lassen
	InvalRect = ClientRect;
	InvalidateRect(InvalRect);
	}

void CSlider::OnPaint() 
	{
	CPaintDC DC(this);

	// Rects ermitteln
	CRect ClientRect;
	GetClientRect(ClientRect);
	CRect ThumbRect;
	ThumbRect.SetRect(ValPos - HThumbWidth, 0, ValPos + HThumbWidth, ClientRect.bottom);
	CRect RandRect;
	RandRect.SetRect(max(ThumbRect.left - RndPos, 3), ThumbRect.top + 3,
		min(ThumbRect.right + RndPos, ClientRect.right - 3), ThumbRect.bottom - 3);

	// Farben ermitteln
	COLORREF ClientColor = BackgroundColor;
	if (!IsWindowEnabled()) ClientColor = GetSysColor(COLOR_BTNFACE);
	COLORREF DrawColor = Color;
	if (GetFocus() == this) DrawColor = FocusColor;

	// Rahmen zeichnen
	DC.FillSolidRect(ClientRect, ClientColor);
	DC.DrawEdge(ClientRect, EDGE_SUNKEN, BF_RECT);

	if (IsWindowEnabled())
		{
		// Random zeichnen
		DC.FillSolidRect(RandRect, DrawColor);

		// Nippel zeichnen
		DC.FillSolidRect(ThumbRect, DrawColor);
		DC.DrawEdge(ThumbRect, EDGE_RAISED, BF_RECT);
		ThumbRect.DeflateRect(1,1);
		if (GetFocus() == this) DC.DrawFocusRect(ThumbRect);
		}
	}

void CSlider::OnMouseMove(UINT Flags, CPoint Point) 
	{
	
	// Value verstellen
	if (Flags & MK_LBUTTON)  
		{
		// Focus holen
		SetFocus();

		// ValPos in Grenzen halten
		int iLastValPos = ValPos;
		CRect ClientRect;
		GetClientRect(ClientRect);
		ValPos = Limit(Point.x, HThumbWidth, ClientRect.right - HThumbWidth);

		// RndPos auf 0
		RndPos = 0;

		// Neuzeichnen lassen
		InvalidateRect(InvalRect);  // Altes Rect
		InvalRect.SetRect(ValPos - HThumbWidth - RndPos, 0, ValPos + HThumbWidth + RndPos, ClientRect.bottom);
		InvalidateRect(InvalRect);  // Neues Rect

		//TRACE("\nValPos: %i, RndPos: %i, Value: %i, Random: %i", ValPos, RndPos, GetValue(), GetRandom());

		// Notify change
		SendChangeNotification();
		}
	
	// RndPos verstellen
	else if (Flags & MK_RBUTTON)  
		{
		// Nichts für Seppel und HasRandom = false
		if (!HasRandom) return;

		// Focus holen
		SetFocus();

		// Neuen RndPos ermitteln und limitieren
		if (Point.x > (ValPos + HThumbWidth)) RndPos = Point.x - (ValPos + HThumbWidth);
		else if (Point.x < (ValPos - HThumbWidth)) RndPos = (ValPos - HThumbWidth) - Point.x;
		else RndPos = 0;
		CRect ClientRect;
		GetClientRect(ClientRect);
		RndPos = Limit(RndPos, 0, ClientRect.right);

		// Neuzeichnen lassen
		InvalidateRect(InvalRect);  // Altes Rect
		InvalRect.SetRect(ValPos - HThumbWidth - RndPos, 0, ValPos + HThumbWidth + RndPos, ClientRect.bottom);
		InvalidateRect(InvalRect);  // Neues Rect

		TRACE("\nValPos: %i, RndPos: %i, Value: %i, Random: %i", ValPos, RndPos, GetValue(), GetRandom());

		// Message senden
		SendChangeNotification();
		}
	}

void CSlider::OnLButtonDown(UINT Flags, CPoint Point) 
	{
	SetCapture();
	OnMouseMove(Flags, Point);
	}

void CSlider::OnLButtonUp(UINT Flags, CPoint Point) 
	{
	ReleaseCapture();
	}

void CSlider::OnRButtonDown(UINT Flags, CPoint Point) 
	{
	SetCapture();
	OnMouseMove(Flags, Point);
	}

void CSlider::OnRButtonUp(UINT Flags, CPoint Point) 
	{
	ReleaseCapture();
	}


void CSlider::PreSubclassWindow() 
	{
	// Fenster vergrößern, damit Schieber auch bei 96dpi noch quadratisch ist
	CRect WindowRect;
	GetWindowRect(WindowRect);
	GetParent()->ScreenToClient(WindowRect);
	WindowRect.bottom = WindowRect.top + HThumbWidth * 2;
	MoveWindow(WindowRect);

	// InvalRect initialisieren
	CRect ClientRect;
	GetClientRect(ClientRect);
	InvalRect = ClientRect;

	CStatic::PreSubclassWindow();
	}


void CSlider::OnKillFocus(CWnd* pNewWnd) 
	{
	InvalidateRect(InvalRect);
	}

void CSlider::OnSetFocus(CWnd* pOldWnd) 
	{
	InvalidateRect(InvalRect);
	}

void CSlider::OnKeyDown(UINT Char, UINT RepCnt, UINT Flags) 
	{
	bool Update = false;

	switch (Char)
		{
		case VK_RIGHT:
			ValPos += 2;
			RndPos = 0;
			Update = true;
			break;
		case VK_LEFT:
			ValPos -= 2;
			RndPos = 0;
			Update = true;
			break;
		case VK_UP:
			if (!HasRandom) return;
			RndPos += 2;
			Update = true;
			break;
		case VK_DOWN:
			if (!HasRandom) return;
			RndPos -= 2;
			Update = true;
			break;
		}

	if (Update)
		{
		// Positionen in Grenzen halten
		CRect ClientRect;
		GetClientRect(ClientRect);
		ValPos = Limit(ValPos, HThumbWidth, ClientRect.right - HThumbWidth);
		RndPos = Limit(RndPos, 0, ClientRect.right);

		// Neuzeichnen lassen
		InvalidateRect(InvalRect);
		InvalRect.SetRect(ValPos - HThumbWidth - RndPos, 0, ValPos + HThumbWidth + RndPos, ClientRect.bottom);
		InvalidateRect(InvalRect);

		TRACE("\nValPos: %i, RndPos: %i, Value: %i, Random: %i", ValPos, RndPos, GetValue(), GetRandom());

		// Message senden
		SendChangeNotification();
		}
	}

UINT CSlider::OnGetDlgCode() 
	{
	return DLGC_WANTARROWS;
	}

void CSlider::Set(C4SVal &rVal)
	{
	MinValue = rVal.Min;
	MaxValue = rVal.Max;
	iValue = rVal.Std;
	iRandom = rVal.Rnd;
	}

void CSlider::Get(C4SVal &rVal)
	{
	iValue = GetValue();
	iRandom = GetRandom();
	rVal.Min = MinValue;
	rVal.Max = MaxValue;
	rVal.Std = iValue;
	rVal.Rnd = iRandom;
	}

void CSlider::Init(COLORREF lColor)
	{
	// Set values
	SetValue(iValue);
	SetRandom(iRandom);
	// Color
	if (lColor) SetColor(lColor);
	}

void CSlider::SetColor(COLORREF lColor, COLORREF lColorBackground)
	{
	Color = FocusColor = lColor;
	BackgroundColor = lColorBackground;
	}

void CSlider::EnableRandom(bool fEnable)
	{
	HasRandom = fEnable;
	}

void CSlider::EnableSound(bool fEnable)
	{
	Sound = fEnable;
	}

void CSlider::SendChangeNotification()
	{
	if (pNotify)
		pNotify->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), SLN_CHANGE), (LPARAM) m_hWnd);
	else
		GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), SLN_CHANGE), (LPARAM) m_hWnd);
	}

bool CSlider::Attach(CWnd *pBuddy)
	{
	// Init
	Init();
	// Disable sound
	EnableSound(false);
	// Disable random
	EnableRandom(false);
	// Set gray color
	SetColor(0xDDDDDD,0xDDDDDD);
	// Set notification target
	pNotify=pBuddy;
	// Done
	return true;
	}

void CSlider::Invalidate()
	{
	CRect Rect;
	GetWindowRect(Rect); 
	GetParent()->ScreenToClient(Rect);
	GetParent()->InvalidateRect(Rect);
	}
