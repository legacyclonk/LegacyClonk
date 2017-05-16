/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Extends CComboBox to handle some icons */

#include "C4Explorer.h"

#include "IconCombo.h"

//---------------------------------------------------------------------------

CIconCombo::CIconCombo()
{
	ItemHeight = 16;
}

//---------------------------------------------------------------------------

CIconCombo::~CIconCombo()
{
}

//---------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(CIconCombo, CComboBox)
	//{{AFX_MSG_MAP(CIconCombo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//---------------------------------------------------------------------------

void CIconCombo::DrawItem(DRAWITEMSTRUCT* pDIS)
{
	// Nur malen, wenn ImgList vorhanden
	if (!ImageList.m_hImageList) return;

	CDC* pDC = CDC::FromHandle(pDIS->hDC);

	// Icon malen
	if (pDIS->itemState & ODS_SELECTED)
		ImageList.Draw(pDC, pDIS->itemID, CPoint(pDIS->rcItem.left,pDIS->rcItem.top), ILD_BLEND25);
	else
		ImageList.Draw(pDC, pDIS->itemID, CPoint(pDIS->rcItem.left,pDIS->rcItem.top), ILD_TRANSPARENT);
}

//---------------------------------------------------------------------------

void CIconCombo::MeasureItem(MEASUREITEMSTRUCT* pMIS) 
{
	pMIS->itemHeight = ItemHeight;
}

//---------------------------------------------------------------------------

void CIconCombo::Init(UINT BmpID, int Width, int Height, int ColorDepth, bool PreFilled, int iSelItem)
{
	ItemHeight = Height;

	// Imagelist erzeugen
	if (ColorDepth == 4) ImageList.Create(BmpID, ItemHeight, 0, TRANSPCOLOR);
	else if (ColorDepth >= 8) LoadImageList(BmpID, ImageList, Width, Height);
	else ASSERT(false);

	// ItemHeight setzen
	SetItemHeight(-1, ItemHeight);
	SetItemHeight(0, ItemHeight);

	// Liste füllen, wenn nicht schon im ResEditor passiert
	if (!PreFilled)
	{
		CString NumString;
		for (int i=0; i < ImageList.GetImageCount(); i++)
		{
			NumString.Format("%i", i);  // Dummy
			AddString(NumString);
		}
	}

	if (iSelItem != -2) SetCurSel(iSelItem);
}

//---------------------------------------------------------------------------
