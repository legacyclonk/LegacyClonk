/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Extends CButton as owner draw radio button with bitmap background */

#include "C4Explorer.h"

CRadioEx::CRadioEx()
	{
	}

CRadioEx::~CRadioEx()
	{
	}


BEGIN_MESSAGE_MAP(CRadioEx, CButton)
	//{{AFX_MSG_MAP(CRadioEx)
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

HBRUSH CRadioEx::CtlColor(CDC* pDC, UINT nCtlColor) 
	{
  pDC->SetBkMode(TRANSPARENT);
  return (HBRUSH)GetStockObject(HOLLOW_BRUSH);
	}

void CRadioEx::SetWindowText(LPCSTR lpszString)
	{
	CButton::SetWindowText(lpszString);
	Invalidate();
	}

void CRadioEx::Set(const char *idCaption)
	{
	// Caption
	if (idCaption) SetWindowText(LoadResStr(idCaption));
	}

void CRadioEx::Invalidate()
	{
	CRect Rect;
	GetWindowRect(Rect); 
	GetParent()->ScreenToClient(Rect);
	GetParent()->InvalidateRect(Rect);
	}

void CRadioEx::OnLButtonDown(UINT nFlags, CPoint point) 
	{
	CButton::OnLButtonDown(nFlags, point);
	}
