/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Allows transparent CtlColor on CRichEditCtrl */

#include "C4Explorer.h"
#include "RichEditEx.h"

BEGIN_MESSAGE_MAP(CRichEditEx, CRichEditCtrl)
	//{{AFX_MSG_MAP(CRichEditEx)
	ON_WM_CTLCOLOR_REFLECT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CRichEditEx::CRichEditEx()
	{
	}

CRichEditEx::~CRichEditEx()
	{
	}

HBRUSH CRichEditEx::CtlColor(CDC* pDC, UINT nCtlColor) 
	{
  pDC->SetBkMode(TRANSPARENT);
  return (HBRUSH)GetStockObject(HOLLOW_BRUSH);
	}		
