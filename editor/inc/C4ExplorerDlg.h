/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Main dialog class */

#if !defined(AFX_C4EXPLORERDLG_H__13AB4986_6FFC_11D2_8888_0040052C10D3__INCLUDED_)
#define AFX_C4EXPLORERDLG_H__13AB4986_6FFC_11D2_8888_0040052C10D3__INCLUDED_

#include "C4ViewItem.h"
#include "C4ViewCtrl.h"	
#include "C4RegistrationDlg.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#if _MSC_VER >= 1300
#define HTASK DWORD // quick hack for OnActivateApp compatibility
#endif // _MSC_VER >= 1300

class C4ExplorerDlg: public CDialog
	{
	
	public:
		C4ExplorerDlg(CWnd* pParent = NULL);
	
	protected:
		int Mode;
		HICON m_hIcon;
		CDIBitmap *m_pPicture;
		C4PNG *m_pPNGPicture;
		CString SelectedSongname;
		bool StatusWipeFlag;
		CRect PlayerModeWindowSize;
		int ClientAreaWidth,ClientAreaHeight;
		bool ColorizePicture;
		DWORD PictureColorDw;
		CString DirectJoinAddress;
    HANDLE hFileChangeNotification;

	public:
		void ReloadItems(const char *szItems, bool fRefreshParent = false, bool fReloadOnly = false);
		C4ViewCtrl* GetViewCtrl();
		void OnClearViewItem(C4ViewItem *pViewItem);
		void ResetFocus();
		void Clear();
		void UpdateItemInfo(C4ViewItem *pViewItem);
		void UpdateUsedEngine();
		int GetMode();
		void CheckRegistration();

	protected: 
		bool m_CommandLineEnabled;
		CString m_Caption;
		void RepositionControl(CWnd &rControl, int iAlign, int iLastWidth, int iLastHeight, int iNewWidth, int iNewHeight);
		void ResizeControls(int iLastWidth, int iLastHeight, int iWidth, int iHeight);
		bool SetStatus(const char *szStatus);
		//bool StartConditionCheck(C4ViewItem *pViewItem);
		void SetMode(int iMode, bool fReloadView, bool fKeepHidden = false);
		void FillDescRTF(DWORD Cookie, EDITSTREAMCALLBACK pESCB, bool fLargeFrame);
		void UpdateLanguage();
		void UpdateText();
		void SetAuthor(const CString &rsNewAuthor); // set new author label text; adjust label height if necessary
		static DWORD CALLBACK DescStreamer(DWORD Cookie, BYTE* pBuff, LONG Bytes, LONG* pBytesDone);

		//{{AFX_DATA(C4ExplorerDlg)
	enum { IDD = IDD_EXPLORER };
	CEdit	m_EditStatus;
	CStaticEx	m_StaticSwitch;
		CStatic	m_StaticDivider;
		CStatic	m_FrameSmall;
		CStaticEx m_StaticAuthor;
		CButtonEx m_ButtonProperties;
		CButtonEx m_ButtonStart;
		CButtonEx m_ButtonRename;
		CButtonEx m_ButtonNew;
		CButtonEx m_ButtonDelete;
		CButtonEx m_ButtonActivate;
		CStatic	m_FrameLarge;
		CStatic	m_FrameCenter;
		CStatic	m_StaticPicture;
		C4ViewCtrl	m_ViewCtrl;
		CRichEditCtrl	m_Desc;
	//}}AFX_DATA

	public:
		void OnSwitchToEngine();

		//{{AFX_VIRTUAL(C4ExplorerDlg)
		virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

		//{{AFX_MSG(C4ExplorerDlg)
		afx_msg LRESULT OnUpdateItem(WPARAM wParam, LPARAM lParam);
		afx_msg LRESULT OnReloadItem(WPARAM wParam, LPARAM lParam);
		afx_msg LRESULT OnRefreshItem(WPARAM wParam, LPARAM lParam);
		afx_msg LRESULT OnInsertItem(WPARAM wParam, LPARAM lParam);
		afx_msg LRESULT OnRemoveItem(WPARAM wParam, LPARAM lParam);
		afx_msg LRESULT OnSetStatus(WPARAM wParam, LPARAM lParam);
		virtual BOOL OnInitDialog();
		afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
		afx_msg void OnDestroy();
		afx_msg void OnPaint();
		afx_msg HCURSOR OnQueryDragIcon();
		afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
		afx_msg void OnStart();
		afx_msg void OnRadioView1();
		afx_msg void OnRadioView2();
		afx_msg void OnButtonDelete();
		afx_msg void OnButtonRename();
		virtual void OnOK();
		virtual void OnCancel();
		afx_msg void OnButtonActivate();
		afx_msg void OnButtonNew();
		afx_msg void OnButtonProperties();
		afx_msg void OnHelpAbout();
		afx_msg void OnHelpWebsite();
		afx_msg void OnOptionsOptions();
		afx_msg void OnOptionsRegistration();
		afx_msg void OnTimer(UINT nIDEvent);
		afx_msg void OnActivateApp( BOOL bActive, HTASK hTask );
		afx_msg void OnActivate( UINT nState, CWnd* pWndOther, BOOL bMinimized );
		afx_msg void OnMouseMove(UINT nFlags, CPoint point);
		afx_msg void OnSize(UINT nType, int cx, int cy);
		afx_msg void OnChangeCommandLine();
		afx_msg void OnHelpOnlineDocs();
		//}}AFX_MSG

		DECLARE_MESSAGE_MAP()
			
	};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_C4EXPLORERDLG_H__13AB4986_6FFC_11D2_8888_0040052C10D3__INCLUDED_)
