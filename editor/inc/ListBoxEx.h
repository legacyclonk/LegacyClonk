/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Adds some functions to CListBox */

#if !defined(AFX_LISTBOXEX_H__8FB13580_F049_11D2_A77D_F50DF0FF2F2B__INCLUDED_)
#define AFX_LISTBOXEX_H__8FB13580_F049_11D2_A77D_F50DF0FF2F2B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CListBoxEx : public CListBox
{
	public:
		CListBoxEx();

	public:
		bool DeleteSelectedItem();
		bool GetItems(CString &rList);
		bool GetItems(char *sList, int iSize);
		bool AddItems(const char *szModuleList);
		virtual ~CListBoxEx();

		DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LISTBOXEX_H__8FB13580_F049_11D2_A77D_F50DF0FF2F2B__INCLUDED_)
