/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

#include "C4Explorer.h"

CStaticEx::CStaticEx()
	{
	}

CStaticEx::~CStaticEx()
	{
	}

BEGIN_MESSAGE_MAP(CStaticEx, CStatic)
	//{{AFX_MSG_MAP(CStaticEx)
	ON_WM_CTLCOLOR_REFLECT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

HBRUSH CStaticEx::CtlColor(CDC* pDC, UINT nCtlColor) 
	{
  pDC->SetBkMode(TRANSPARENT);
  return (HBRUSH)GetStockObject(HOLLOW_BRUSH);
	}

void CStaticEx::SetWindowText(LPCSTR lpszString)
	{
	CStatic::SetWindowText(lpszString);
	Invalidate();
	}

void CStaticEx::Invalidate()
	{
	CRect Rect;
	GetWindowRect(Rect); 
	GetParent()->ScreenToClient(Rect);
	GetParent()->InvalidateRect(Rect);
	}

void CStaticEx::Set(const char *szText)
	{
	// Caption
	if (szText)
		if (SEqual2(szText, "IDS_"))
			SetWindowText(LoadResStr(szText));
		else
			SetWindowText(szText);
	}
