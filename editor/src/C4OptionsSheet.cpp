/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Options dialog */

#include "C4Explorer.h"

#include "C4OptionsSheet.h"

//IMPLEMENT_DYNAMIC(C4OptionsSheet, CPropertySheet)

BEGIN_MESSAGE_MAP(C4OptionsSheet, CPropertySheet)
	//{{AFX_MSG_MAP(C4OptionsSheet)
	//ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

C4OptionsSheet::C4OptionsSheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
	{
	
	m_psh.dwFlags |= PSH_NOAPPLYNOW;

	LoadImageList(IDB_TABICON, TabImageList, 16, 16);
	
	AddPage(&ProgramPage);
	AddPage(&EditorPage);
	AddPage(&PageDeveloper);

	}

C4OptionsSheet::~C4OptionsSheet()
	{
	}

BOOL C4OptionsSheet::OnInitDialog() 
	{
	BOOL bResult = CPropertySheet::OnInitDialog();

	// Page data set on page construction

	// Set tab info
	int iPage = 0;
	SetTabInfo(iPage++,6,LoadResStr("IDS_DLG_PROGRAM"));
	SetTabInfo(iPage++,14,LoadResStr("IDS_DLG_EDITOR"));
	SetTabInfo(iPage++,14,LoadResStr("IDS_DLG_DEVELOPER"));

	return bResult;
	}

void C4OptionsSheet::GetData()
	{
	ProgramPage.GetData();
	EditorPage.GetData();
	PageDeveloper.GetData();
	}

int C4OptionsSheet::DoModal() 
	{	
	int Result=CPropertySheet::DoModal();
	
	if (Result==IDOK) GetData();

	return Result;
	}

bool C4OptionsSheet::SetTabInfo(int iTab, int iImage, const char * szText)
	{
	CTabCtrl* pTabCtrl = GetTabControl();
	pTabCtrl->SetImageList(&TabImageList);
	TC_ITEM Item;
	Item.mask = TCIF_IMAGE | TCIF_TEXT;
	Item.iImage = iImage;	
	Item.pszText = (char*) szText;
	pTabCtrl->SetItem(iTab, &Item);
	return true;
	}

