/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Options dialog */

#if !defined(AFX_C4OPTIONSSHEET_H__A28467C3_9470_11D2_8888_0040052C10D3__INCLUDED_)
#define AFX_C4OPTIONSSHEET_H__A28467C3_9470_11D2_8888_0040052C10D3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "C4PageProgram.h"
#include "C4PageEditor.h"	
#include "C4PageDeveloper.h"

class C4OptionsSheet: public CPropertySheet
	{
	//DECLARE_DYNAMIC(C4OptionsSheet)

	public:
		C4OptionsSheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
		virtual ~C4OptionsSheet();

	protected:
		CImageList TabImageList;
		C4PageProgram ProgramPage;
		C4PageEditor EditorPage;

	protected:
		void GetData();
		bool SetTabInfo(int iTab, int iImage, const char *szText);

	public:
		//{{AFX_VIRTUAL(C4OptionsSheet)
		virtual BOOL OnInitDialog();
		virtual int DoModal();
		//}}AFX_VIRTUAL

	protected:
		C4PageDeveloper PageDeveloper;
		//{{AFX_MSG(C4OptionsSheet)
		//afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
		//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	};

//{{AFX_INSERT_LOCATION}}

#endif // !defined(AFX_C4OPTIONSSHEET_H__A28467C3_9470_11D2_8888_0040052C10D3__INCLUDED_)
