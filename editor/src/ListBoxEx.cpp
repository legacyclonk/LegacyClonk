/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Adds some functions to CListBox */

#include "C4Explorer.h"

/////////////////////////////////////////////////////////////////////////////
// CListBoxEx

CListBoxEx::CListBoxEx()
{
}

CListBoxEx::~CListBoxEx()
{
}


BEGIN_MESSAGE_MAP(CListBoxEx, CListBox)
	//{{AFX_MSG_MAP(CListBoxEx)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CListBoxEx message handlers

bool CListBoxEx::AddItems(const char *szModuleList)
	{
	char szName[256+1];

	for (int cnt=0; SCopySegment(szModuleList,cnt,szName,';',256); cnt++)
		{
		SClearFrontBack(szName);
		if (szName[0]) AddString(szName);
		}

	return true;
	}

bool CListBoxEx::GetItems(char *sList, int iSize)
	{
	int iLen;
	sList[0]=0;
	for (int cnt=0; (iLen=GetTextLen(cnt))!=LB_ERR; cnt++)
		if (SLen(sList)+iLen<=iSize)
			{			
			SNewSegment(sList);
			GetText(cnt,sList+SLen(sList));
			}
	return true;
	}

bool CListBoxEx::GetItems(CString &rList)
	{
	int iLen;
	CString sItem;
	rList.Empty();
	for (int cnt=0; (iLen=GetTextLen(cnt))!=LB_ERR; cnt++)
		{			
		GetText(cnt,sItem);
		if (!rList.IsEmpty()) rList+="; ";
		rList+=sItem;
		}
	return true;
	}

bool CListBoxEx::DeleteSelectedItem()
	{
	DeleteString(GetCurSel());
	return true;
	}
