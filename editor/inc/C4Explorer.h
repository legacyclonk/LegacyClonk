/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Main framework class & headers */

#if !defined(AFX_C4EXPLORER_H__13AB4984_6FFC_11D2_8888_0040052C10D3__INCLUDED_)
#define AFX_C4EXPLORER_H__13AB4984_6FFC_11D2_8888_0040052C10D3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "stdafx.h"

#include "..\res\resource.h"

#include "C4Frontend.h"

#include "DIBitmap.h"
#include "StringEx.h"
#include "ButtonEx.h"
#include "StaticEx.h"
#include "RadioEx.h"
#include "ListBoxEx.h"
#include "TabCtrlEx.h"
#include "InputDlg.h"
#include "Slider.h"

#include "C4ItemTypes.h"
#include "C4ExplorerCfg.h"
#include "C4SplashDlg.h"
#include "C4EditSlot.h"
#include "C4DefListEx.h"
#include "C4IdListCtrl.h"
#include "C4IdCtrlButtons.h"

const int C4EX_ProcessRouting_None		= 0,
					C4EX_ProcessRouting_Status	= 1,
					C4EX_ProcessRouting_Refresh = 2,
					C4EX_ProcessRouting_Reload	= 4;

class C4ExplorerApp: public CWinApp
	{
	public:
		C4ExplorerApp();
		 ~C4ExplorerApp();

	public:
		int ComCtlVersion;
		C4EditSlots EditSlots;
		C4ExplorerCfg Config;
		
		CPalette Palette;
		
		CDIBitmap dibPaper;
		CDIBitmap dibButton;
		
		C4RankSystem PlayerRanks;
		C4RankSystem ClonkRanks;
		
	protected:
		C4SplashDlg SplashDlg;

	public:
		void SetProcessRouting(int iProcessRouting, const char *szProcessFormat=NULL);
		void HideSplash();
		void SetUserGroupMaker();
		BYTE GetCharsetCode(const char *strCharset);

	protected:
		int ProcessRouting;
		char ProcessFormat[1024+1];
		static BOOL ProcessCallback(const char *szText, int iProcess);
		void CheckConfig();

	private:
		HMODULE hRichEditDLL;	

		//{{AFX_VIRTUAL(C4ExplorerApp)
		public:
		virtual BOOL InitInstance();
		virtual int ExitInstance();
		//}}AFX_VIRTUAL

		//{{AFX_MSG(C4ExplorerApp)
	//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	};

#include "C4Utilities.h"

//{{AFX_INSERT_LOCATION}}

#endif // !defined(AFX_C4EXPLORER_H__13AB4984_6FFC_11D2_8888_0040052C10D3__INCLUDED_)
