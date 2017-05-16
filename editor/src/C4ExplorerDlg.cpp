/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Main dialog class */

#include "C4Explorer.h"

#include "C4ExplorerDlg.h"
#include "C4AboutDlg.h"
#include "C4OptionsSheet.h"
#include "C4Language.h"

BEGIN_MESSAGE_MAP(C4ExplorerDlg, CDialog)
	//{{AFX_MSG_MAP(C4ExplorerDlg)
	ON_MESSAGE(WM_USER_UPDATEITEM, OnUpdateItem)
	ON_MESSAGE(WM_USER_RELOADITEM, OnReloadItem)
	ON_MESSAGE(WM_USER_REFRESHITEM, OnRefreshItem)
	ON_MESSAGE(WM_USER_INSERTITEM, OnInsertItem)
	ON_MESSAGE(WM_USER_REMOVEITEM, OnRemoveItem)
	ON_MESSAGE(WM_USER_SETSTATUS, OnSetStatus)
	ON_WM_SYSCOMMAND()
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_GETMINMAXINFO()
	ON_BN_CLICKED(IDC_BUTTONSTART, OnStart)
	ON_BN_CLICKED(IDC_BUTTONDELETE, OnButtonDelete)
	ON_BN_CLICKED(IDC_BUTTONRENAME, OnButtonRename)
	ON_BN_CLICKED(IDC_BUTTONACTIVATE, OnButtonActivate)
	ON_BN_CLICKED(IDC_BUTTONNEW, OnButtonNew)
	ON_BN_CLICKED(IDC_BUTTONPROPERTIES, OnButtonProperties)
	ON_COMMAND(IDM_HELP_ABOUT, OnHelpAbout)
	ON_COMMAND(IDM_HELP_WEBSITE, OnHelpWebsite)
	ON_COMMAND(IDM_OPTIONS_OPT, OnOptionsOptions)
	ON_COMMAND(IDM_OPTIONS_REG, OnOptionsRegistration)
	ON_WM_TIMER()
	ON_WM_ACTIVATEAPP()
	ON_WM_ACTIVATE()
	ON_WM_CLOSE()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_EN_CHANGE(IDC_EDITSTATUS, OnChangeCommandLine)
	ON_WM_ERASEBKGND()
	ON_WM_HELPINFO()
	ON_COMMAND(IDM_HELP_ONLINEDOCS, OnHelpOnlineDocs)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

C4ExplorerDlg::C4ExplorerDlg(CWnd* pParent) : CDialog(C4ExplorerDlg::IDD, pParent)
	{
	//{{AFX_DATA_INIT(C4ExplorerDlg)
	//}}AFX_DATA_INIT
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pPicture = NULL;
	m_pPNGPicture = NULL;
	m_CommandLineEnabled = false;
	ColorizePicture = false;
	PictureColorDw = 0x0000;
	StatusWipeFlag=false;
	PlayerModeWindowSize.SetRect(0,0,0,0);
	Mode=C4VC_Undetermined;
	DirectJoinAddress.Empty();
	}

void C4ExplorerDlg::DoDataExchange(CDataExchange* pDX)
	{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(C4ExplorerDlg)
	DDX_Control(pDX, IDC_EDITSTATUS, m_EditStatus);
	DDX_Control(pDX, IDC_STATIC_SWITCH, m_StaticSwitch);
	DDX_Control(pDX, IDC_STATICDIVIDER, m_StaticDivider);
	DDX_Control(pDX, IDC_FRAMESMALLDESC, m_FrameSmall);
	DDX_Control(pDX, IDC_STATICAUTHOR, m_StaticAuthor);
	DDX_Control(pDX, IDC_BUTTONPROPERTIES, m_ButtonProperties);
	DDX_Control(pDX, IDC_BUTTONSTART, m_ButtonStart);
	DDX_Control(pDX, IDC_BUTTONRENAME, m_ButtonRename);
	DDX_Control(pDX, IDC_BUTTONNEW, m_ButtonNew);
	DDX_Control(pDX, IDC_BUTTONDELETE, m_ButtonDelete);
	DDX_Control(pDX, IDC_BUTTONACTIVATE, m_ButtonActivate);
	DDX_Control(pDX, IDC_FRAMELARGEDESC, m_FrameLarge);
	DDX_Control(pDX, IDC_FRAMECENTER, m_FrameCenter);
	DDX_Control(pDX, IDC_BITMAPDESC, m_StaticPicture);
	DDX_Control(pDX, IDC_VIEW, m_ViewCtrl);
	DDX_Control(pDX, IDC_RICHEDITDESC, m_Desc);
	//}}AFX_DATA_MAP
	}

BOOL C4ExplorerDlg::OnInitDialog()
	{	
	// Base class call
	CDialog::OnInitDialog();

	// Set control text
	UpdateText();

	// Store original size (used as minimum size when resizing)
	GetWindowRect(PlayerModeWindowSize);
	RECT rct; GetClientRect(&rct); ClientAreaWidth=rct.right; ClientAreaHeight=rct.bottom;

	// Initial centering of window
	CenterWindow(GetDesktopWindow());

	// Set dialog icon
	SetIcon(m_hIcon, TRUE); SetIcon(m_hIcon, FALSE);
	
	// Mode
	SetMode(C4VC_Developer /*GetCfg()->Explorer.Mode*/ , false, true); // FE now runs in developer mode _only_

	// Init view ctrl
	CString InitRoot = GetCfg()->General.ExePath;
	m_ViewCtrl.Init(InitRoot,Mode);
	m_ViewCtrl.SelectFirstItem();

	// Welcome
	SetStatus(LoadResStr("IDS_EDITOR_WELCOME"));

	// Hide splash dialog
	GetApp()->HideSplash();

	// Set timers
	SetTimer(ID_TIMER_AUTOEDITSCAN,2000,NULL);
	SetTimer(ID_TIMER_WIPESTATUS,3000,NULL);

	// Set focus
	m_ViewCtrl.SetFocus();

	// Start with the command line enabled
	m_EditStatus.EnableWindow(TRUE);
	m_EditStatus.SetWindowText(GetCfg()->Explorer.CommandLine);
	m_CommandLineEnabled = true;	

	// Return false to indicate that we have set the focus manually
	return FALSE;
	}

void C4ExplorerDlg::OnSysCommand(UINT nID, LPARAM lParam)
	{
	CDialog::OnSysCommand(nID, lParam);
	}

void C4ExplorerDlg::OnDestroy()
	{
	// Store last window position
	if (Mode == C4VC_Player) 
		StoreWindowPosition(m_hWnd, "WindowPlayer", GetCfg()->GetSubkeyPath("Explorer"));	
	if (Mode == C4VC_Developer)
		StoreWindowPosition(m_hWnd, "WindowDeveloper", GetCfg()->GetSubkeyPath("Explorer"));	
	
	//WinHelp(0L, HELP_QUIT);
	Clear();
	CDialog::OnDestroy();
	}

void C4ExplorerDlg::OnPaint() 
	{
	// Minimized window
	if (IsIconic())
		{
		CPaintDC dc(this); 
		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);
		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;
		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
		}
	// Normal window
	else
		{
		// Draw item picture 
		CPaintDC DC(this);
		CRect Rect;
		
		// Bmp picture (currently not colorized)
		if (m_pPicture && m_pPicture->HasContent()) 
			{
			m_StaticPicture.GetWindowRect(Rect);
			ScreenToClient(Rect);	
			// Regular title picture
			if (!m_pPicture->HasMask())
				m_pPicture->DrawSection(0,0,&DC,Rect);
			// Transparent masked def picture
			else
				{
				Rect.left+=20; Rect.top+=10; Rect.right-=20; Rect.bottom-=10;
				m_pPicture->DrawSection(0,0,&DC,Rect,true,true);
				}
			}
		// Png picture
		else 
			if (m_pPNGPicture && m_pPNGPicture->pImageData)
			{
				m_StaticPicture.GetWindowRect(Rect);
				ScreenToClient(Rect);
				if (ColorizePicture)
					m_pPNGPicture->Draw(DC.m_hDC, Rect, &PictureColorDw);
				else
					m_pPNGPicture->Draw(DC.m_hDC, Rect);
			}
		
		}
	}

HCURSOR C4ExplorerDlg::OnQueryDragIcon()
	{
	return (HCURSOR) m_hIcon;
	}

void C4ExplorerDlg::SetAuthor(const CString &rsNewAuthor)
	{
	// check text width
	/*CSize size = m_StaticAuthor.GetDC()->GetTextExtent(rsNewAuthor);
	// some hack for the margin...
	if (size.cx > 10) size.cx -= 10;
	// resize author control + text control above
	// get their client positions
	RECT rcAuthorPos, rcDescPos, rcDescPos1, rcDescPos2;
	m_StaticAuthor.GetWindowRect(&rcAuthorPos); ScreenToClient(&rcAuthorPos);
	m_Desc.GetWindowRect(&rcDescPos); ScreenToClient(&rcDescPos);
	m_FrameLarge.GetWindowRect(&rcDescPos1); ScreenToClient(&rcDescPos1);
	m_FrameSmall.GetWindowRect(&rcDescPos2); ScreenToClient(&rcDescPos2);
	// calculate offset from author label to desc ctrl
	int iAuthorDescOff = rcAuthorPos.top - rcDescPos.bottom;
	int iAuthorDescOff1 = rcAuthorPos.top - rcDescPos1.bottom;
	int iAuthorDescOff2 = rcAuthorPos.top - rcDescPos2.bottom;
	// calculate height of author label
	int iOldTop = rcAuthorPos.top;
	rcAuthorPos.top = rcAuthorPos.bottom - ((size.cy + 2) * BoundBy<int>(size.cx / Max<int>(rcAuthorPos.right - rcAuthorPos.left, 1) + 1, 1, 5));
	int iHeightChange = rcAuthorPos.top - iOldTop;
	// calculate height of desc ctrl
	rcDescPos.bottom = rcAuthorPos.top - iAuthorDescOff;
	rcDescPos1.bottom = rcAuthorPos.top - iAuthorDescOff1;
	rcDescPos2.bottom = rcAuthorPos.top - iAuthorDescOff2;
	// adjust positions
	m_StaticAuthor.MoveWindow(&rcAuthorPos, FALSE);
	m_Desc.MoveWindow(&rcDescPos);
	m_FrameLarge.MoveWindow(&rcDescPos1);
	m_FrameSmall.MoveWindow(&rcDescPos2);*/
	// set author
	m_StaticAuthor.SetWindowText(rsNewAuthor);
	// if the author label got smaller, there's a dead region that isn't redrawn
	// simply redraw the background
	/*if (iHeightChange > 0) Invalidate();*/
	}

void C4ExplorerDlg::UpdateItemInfo(C4ViewItem *pViewItem)
	{
	CRect Rect;
	
	// Author
	if (pViewItem && !pViewItem->Author.IsEmpty()) 
		{
		CString sText; sText.Format(LoadResStr("IDS_CTL_AUTHOR"),pViewItem->Author);
		if (!pViewItem->ShowAuthor.IsEmpty()) sText.Format(LoadResStr("IDS_CTL_AUTHOR"),pViewItem->ShowAuthor);
		SetAuthor(sText);
		}
	else
		SetAuthor(CString(""));

	// Store pointer to item's bmp or png picture
	m_pPicture = NULL; 
	m_pPNGPicture = NULL;
	if (pViewItem && pViewItem->PNGPicture.pImageData) 
		m_pPNGPicture = &(pViewItem->PNGPicture);
	else 
		if (pViewItem && pViewItem->Picture.HasContent()) 
			m_pPicture = &(pViewItem->Picture);
	// Store item's colorization info
	if (pViewItem)
		if (ColorizePicture = pViewItem->EnableColor)
			if (!pViewItem->ColorByParent)
				PictureColorDw = pViewItem->dwColor;
			else
				if (m_ViewCtrl.GetParentViewItem(pViewItem))
					PictureColorDw = m_ViewCtrl.GetParentViewItem(pViewItem)->dwColor;
				else
					ColorizePicture = false;
	// Invalidate picture region
	m_StaticPicture.GetWindowRect(Rect); 
	ScreenToClient(Rect);
	InvalidateRect(Rect, false);

	bool fLargeDesc=true;
	if (m_pPicture && m_pPicture->HasContent()) fLargeDesc=false;
	if (m_pPNGPicture && m_pPNGPicture->pImageData) fLargeDesc=false;

	// Desc
	FillDescRTF((DWORD) pViewItem, &DescStreamer,fLargeDesc);

	// Center frame
	m_FrameCenter.GetWindowRect(Rect); ScreenToClient(Rect); InvalidateRect(Rect);

	// Buttons
	if (pViewItem)
		{
		// Management buttons
		m_ButtonNew.EnableWindow(true /*!pViewItem->pWebLibEntry*/);
		m_ButtonActivate.Set(pViewItem->Activated ? "IDS_BTN_DEACTIVATE" : "IDS_BTN_ACTIVATE");
		m_ButtonActivate.EnableWindow(pViewItem->EnableActivation);
		m_ButtonRename.EnableWindow(true /*!pViewItem->pWebLibEntry*/);
		m_ButtonDelete.EnableWindow(true /*!pViewItem->pWebLibEntry*/);
		m_ButtonProperties.EnableWindow(pViewItem->EnableProperties);
		m_ButtonStart.Set("IDS_BTN_START");
		m_ButtonStart.EnableWindow(true);	
		}
	else
		{
		// Nothing selected
		m_ButtonActivate.EnableWindow(false);
		m_ButtonRename.EnableWindow(false);
		m_ButtonDelete.EnableWindow(false);
		m_ButtonProperties.EnableWindow(false);
		m_ButtonStart.EnableWindow(true);
		m_ButtonNew.EnableWindow(true);
		}

	// Update command line
	if (m_CommandLineEnabled)
	{
		// Scan command line and remove any old scenario parameter
		CString strCommandLine;
		char strParameter[_MAX_PATH + 1];
		bool fQuoted;
		for (int i = 0; SGetParameter(GetCfg()->Explorer.CommandLine, i, strParameter, _MAX_PATH, &fQuoted); i++)
			if (!SEqualNoCase(GetExtension(strParameter), "c4s"))
			{
				// Append parameter
				if (!strCommandLine.IsEmpty()) strCommandLine += " ";
				if (fQuoted) strCommandLine += "\"";
				strCommandLine += strParameter;
				if (fQuoted) strCommandLine += "\"";
			}
		// Scenario selected: append new scenario parameter
		if (pViewItem && (pViewItem->Type == C4IT_Scenario))
			{
			// Append parameter
			fQuoted = (SCharCount(' ', GetCfg()->AtExeRelativePath(pViewItem->Path)) > 0);
			if (!strCommandLine.IsEmpty()) strCommandLine += " ";
			if (fQuoted) strCommandLine += "\"";
			strCommandLine += GetCfg()->AtExeRelativePath(pViewItem->Path);
			if (fQuoted) strCommandLine += "\"";
			}
		// Set new command line
		m_EditStatus.SetWindowText(strCommandLine);
	}

	UpdateData(FALSE);
	}

static int DescStreamPos;

void C4ExplorerDlg::FillDescRTF(DWORD Cookie, EDITSTREAMCALLBACK pESCB, bool fLargeFrame)
	{
	// Nix Flickerflacker
	m_Desc.SetRedraw(false);

	// Get view item
	C4ViewItem *pViewItem = (C4ViewItem*) Cookie;

	// Editstream vorbereiten und Text streamen		
	EDITSTREAM EditStream;
	EditStream.dwCookie = Cookie;
	EditStream.dwError = 0;
	EditStream.pfnCallback = pESCB;
	int iFormat = SF_RTF;
	DescStreamPos=0;
	m_Desc.StreamIn(iFormat, EditStream);

	// Set font to override any crazy font settings which might be in the RTF
	CHARFORMAT CF; ZeroMem(&CF,sizeof(CF));
	CF.cbSize = sizeof CF; CF.dwMask = CFM_FACE;
	SCopy("Verdana", CF.szFaceName);
	m_Desc.SetSel(0, -1);	m_Desc.SetSelectionCharFormat(CF); m_Desc.SetSel(0,0);

	m_Desc.SetRedraw(true);
	m_Desc.Invalidate();
	}

DWORD CALLBACK C4ExplorerDlg::DescStreamer(DWORD Cookie, BYTE* pBuff, LONG Bytes, LONG* pBytesDone)
	{
	C4ViewItem *pViewItem = (C4ViewItem*) Cookie;
	*pBytesDone=0;
	if (!pViewItem) return 1;

	// Wenn noch so viele Bytes da sind, wie angefordert
	if ((size_t)(DescStreamPos + Bytes) <= pViewItem->Desc.GetDataSize())
		{
		// Ganzen Block kopieren
		memcpy(pBuff, pViewItem->Desc.GetData() + DescStreamPos, Bytes);
		*pBytesDone = Bytes;
		DescStreamPos += Bytes;
		}
	else
		{
		// Restlichen Teilblock kopieren
		int RestBufferSize = pViewItem->Desc.GetDataSize() - DescStreamPos;
		ASSERT(RestBufferSize >= 0);
		memcpy(pBuff, pViewItem->Desc.GetData() + DescStreamPos, RestBufferSize);
		*pBytesDone = RestBufferSize;
		DescStreamPos += RestBufferSize;
		}

	return 0;
	}

void C4ExplorerDlg::OnRadioView1() 
	{
	SetMode(C4VC_Player, true);
	}

void C4ExplorerDlg::OnRadioView2() 
	{
	SetMode(C4VC_Developer, true);
	}

void C4ExplorerDlg::OnButtonDelete() 
	{
	m_ViewCtrl.OnDelete();
	}

void C4ExplorerDlg::OnButtonRename() 
	{
	m_ViewCtrl.OnRename();
	}

void C4ExplorerDlg::OnOK() 
	{
	// Called from view ctrl edit label
	if (m_ViewCtrl.EditLabelOK()) return;
	// Called from view ctrl selection
	if (GetFocus() == &m_ViewCtrl) 
		{ m_ViewCtrl.OnOK(); return; }
	// redirect to OnStart (CDialog::OnOK would close the window...
	//  not really what the user expects when pressing Enter)
	OnStart();
	}

void C4ExplorerDlg::OnCancel() 
	{
	// Called from view ctrl edit label
	if (m_ViewCtrl.EditLabelCancel()) return;
	// Called from dialog
	CDialog::OnCancel();
	}

void C4ExplorerDlg::OnButtonActivate() 
	{
	m_ViewCtrl.OnActivate();		
	}

void C4ExplorerDlg::OnStart() 
{
	// Empty command line
	if (!GetCfg()->Explorer.CommandLine[0])
		return;
	// First parameter must be an executable
	if (!SEqualNoCase(GetExtension(SGetParameter(GetCfg()->Explorer.CommandLine, 0)), "exe")) // For Wine-builds this test would have to be done differently...
	{
		// Automatically prepend engine parameter
		CString strCommandLine; strCommandLine.Format("%s %s", C4CFN_Engine, GetCfg()->Explorer.CommandLine);
		SCopy(strCommandLine, GetCfg()->Explorer.CommandLine, CFG_MaxString);
		m_EditStatus.SetWindowText(strCommandLine);
	}
	// Validate executable parameter (must be an existing file)
	if (!FileExists(SGetParameter(GetCfg()->Explorer.CommandLine, 0)))
	{
		// Complain
		CString strMessage; strMessage.Format(LoadResStr("IDS_MSG_NOTAVALIDFILE"), SGetParameter(GetCfg()->Explorer.CommandLine, 0));
		C4MessageBox(strMessage);
		return;
	}
	// Validate modules
	GetCfg()->ValidateModules(GetCfg()->General.ExePath, GetCfg()->General.Participants);
	// Store participants in configuration
	GetCfg()->ValidateModules(GetCfg()->General.ExePath, GetCfg()->General.Participants);
	// Store definitions in configuration (override engine definitions)
	GetCfg()->ValidateModules(GetCfg()->General.ExePath, GetCfg()->Explorer.Definitions);
	SCopy(GetCfg()->Explorer.Definitions, GetCfg()->General.Definitions);
	// Save configuration
	GetCfg()->Explorer.Reload[0] = 0;
	GetCfg()->Save();
	// Launch
	STARTUPINFO StartupInfo; SetZero(StartupInfo);
	StartupInfo.cb = sizeof StartupInfo;
	PROCESS_INFORMATION ProcessInfo; SetZero(ProcessInfo);
	CreateProcess(NULL, GetCfg()->Explorer.CommandLine, NULL, NULL, TRUE, 0, NULL, NULL, &StartupInfo, &ProcessInfo);
}

void C4ExplorerDlg::OnButtonNew() 
	{
	
	// Player mode: enforce root as target for simplification
	if (Mode==C4VC_Player) m_ViewCtrl.SelectItem(NULL);

	m_ViewCtrl.OnNew();
	}


void C4ExplorerDlg::UpdateText()
	{
	
	// Dialog caption (now hardcoded so it can be found by update system)
	SetWindowText(C4EDITORCAPTION);
	
	// Misc
	m_StaticAuthor.Set();
	m_StaticSwitch.Set("IDS_DLG_EDITORSWITCH");

	// Status
	SetStatus("");

	// Buttons
	m_ButtonProperties.Set("IDS_BTN_PROPERTIES");
	m_ButtonStart.Set("IDS_BTN_START");
	m_ButtonRename.Set("IDS_BTN_RENAME");
	m_ButtonNew.Set("IDS_BTN_NEW");
	m_ButtonDelete.Set("IDS_BTN_DELETE");
	m_ButtonActivate.Set("IDS_BTN_ACTIVATE");

	// Menu
	GetMenu()->ModifyMenu(0,MF_BYPOSITION,0,LoadResStr("IDS_MENU_OPTION"));
	GetMenu()->ModifyMenu(1,MF_BYPOSITION,0,LoadResStr("IDS_MENU_HELP"));
	
	GetMenu()->GetSubMenu(0)->ModifyMenu(IDM_OPTIONS_OPT,MF_BYCOMMAND,IDM_OPTIONS_OPT,LoadResStr("IDS_MENU_OPTIONS"));
	GetMenu()->GetSubMenu(0)->ModifyMenu(IDM_OPTIONS_REG,MF_BYCOMMAND,IDM_OPTIONS_REG,LoadResStr("IDS_MENU_REGISTRATION"));
	
	GetMenu()->GetSubMenu(1)->ModifyMenu(IDM_HELP_WEBSITE,MF_BYCOMMAND,IDM_HELP_WEBSITE,LoadResStr("IDS_MENU_WEBSITE"));
	GetMenu()->GetSubMenu(1)->ModifyMenu(IDM_HELP_ONLINEDOCS,MF_BYCOMMAND,IDM_HELP_ONLINEDOCS,LoadResStr("IDS_MENU_ONLINEDOCS"));
	GetMenu()->GetSubMenu(1)->ModifyMenu(IDM_HELP_ABOUT,MF_BYCOMMAND,IDM_HELP_ABOUT,LoadResStr("IDS_MENU_ABOUT"));
	
	}

void C4ExplorerDlg::OnButtonProperties() 
	{
	m_ViewCtrl.OnProperties();
	}

void C4ExplorerDlg::OnHelpAbout() 
	{
	C4AboutDlg bla; bla.DoModal();
	}

void C4ExplorerDlg::OnHelpWebsite() 
	{
	ShellExecute(NULL, "open", "http://www.clonk.de/", NULL, NULL, SW_SHOWNORMAL);
	}

void C4ExplorerDlg::OnOptionsOptions() 
	{
	// Options sheet instance
	C4OptionsSheet OptionsSheet(LoadResStrNoAmp("IDS_DLG_OPTIONS"),this,0);

	// Re-init external language packs (in case some new ones have been added at run time)
	Languages.Init();

	// Store last values
	CString LastLanguage = GetCfg()->General.Language;

	// Do options sheet
	if (OptionsSheet.DoModal()==IDOK)
		// Save config
		GetCfg()->Save();

	// Update language if necessary
	if (GetCfg()->General.Language != LastLanguage)	
		UpdateLanguage();

	// Reset focus
	m_ViewCtrl.SetFocus();
	
	}

void C4ExplorerDlg::OnOptionsRegistration() 
	{
	C4RegistrationDlg RegistrationDlg;
	RegistrationDlg.DoModal();
	}

void C4ExplorerDlg::UpdateLanguage()
	{
	// Load language string table (if this fails, we should bail out of the application...)
	Languages.LoadLanguage(GetCfg()->General.LanguageEx);
	// Update dialog text
	UpdateText();
	// Redraw menu bar
	DrawMenuBar();
	// Reload item view
	m_ViewCtrl.PostMessage(WM_KEYDOWN, VK_F5, 0);
	// Clear selected item info
	UpdateItemInfo(NULL);
	}

void C4ExplorerDlg::OnTimer(UINT nIDEvent) 
	{
	switch (nIDEvent)
		{
		case ID_TIMER_AUTOEDITSCAN:
			if (GetCfg()->Developer.AutoEditScan)
				GetApp()->EditSlots.UpdateModifiedItems(true);
			break;
		case ID_TIMER_WIPESTATUS:
			if (StatusWipeFlag) SetStatus("");
			StatusWipeFlag=true;
			break;
		}

	}

LRESULT C4ExplorerDlg::OnUpdateItem(WPARAM wParam, LPARAM lParam)
	{
	CString Filename = (const char *) lParam;
	bool fRefreshParent = (wParam!=0);
	return m_ViewCtrl.ReloadItem(Filename,fRefreshParent);
	}

LRESULT C4ExplorerDlg::OnReloadItem(WPARAM wParam, LPARAM lParam)
	{
	CString Filename = (const char *) lParam;
	TRACE("OnReloadItem %s\n",Filename);
	return m_ViewCtrl.ReloadItem(Filename,false);
	}

LRESULT C4ExplorerDlg::OnRefreshItem(WPARAM wParam, LPARAM lParam)
	{
	CString Filename = (const char *) lParam;
	TRACE("OnRefreshItem %s\n",Filename);
	return m_ViewCtrl.RefreshItem(Filename);
	}

LRESULT C4ExplorerDlg::OnInsertItem(WPARAM wParam, LPARAM lParam)
	{
	CString Filename = (const char *) lParam;
	CString strParentPath = NoBackslash(GetPath(Filename));
	// already in tree?
	if(m_ViewCtrl.GetViewItem(Filename))
		return TRUE;
	C4ViewItem *pParentViewItem = NULL;
	if(!SEqual(strParentPath, NoBackslash(GetCfg()->AtExePath(""))))
	{
		pParentViewItem = m_ViewCtrl.GetViewItem(strParentPath);	
		// only if contents already loaded
		if(!pParentViewItem || !pParentViewItem->ContentsLoaded) return TRUE;
	}
	return m_ViewCtrl.LoadItem(Filename, pParentViewItem);
	}

LRESULT C4ExplorerDlg::OnRemoveItem(WPARAM wParam, LPARAM lParam)
{
	CString Filename = (const char *) wParam;
	m_ViewCtrl.RemoveItem(Filename);
	return TRUE;
}

LRESULT C4ExplorerDlg::OnSetStatus(WPARAM wParam, LPARAM lParam)
	{

	if (SEqual2((const char *)lParam, "IDS_"))
		return SetStatus(LoadResStr((const char *)lParam));
	else
		return SetStatus((const char *)lParam);

	return 0;
	}

bool C4ExplorerDlg::SetStatus(const char *szStatus)
	{
	// Command line enabled
	if (m_CommandLineEnabled) return false;
	// Set status
	m_EditStatus.SetWindowText(szStatus);
	m_EditStatus.EnableWindow(FALSE);
	return true;
	}

void C4ExplorerDlg::SetMode(int iMode, bool fReloadView, bool fKeepHidden)
	{
	// Valid modes only
	iMode = BoundBy(iMode,C4VC_Player,C4VC_Developer);
	// No change
	if (iMode == Mode) return;
	// Store window position for last mode
	if (Mode == C4VC_Player) 
		StoreWindowPosition(m_hWnd, "WindowPlayer", GetCfg()->GetSubkeyPath("Explorer"));	
	if (Mode == C4VC_Developer)
		StoreWindowPosition(m_hWnd, "WindowDeveloper", GetCfg()->GetSubkeyPath("Explorer"));	
	// Set new mode
	Mode = iMode;
	// Write back to config
	GetCfg()->Explorer.Mode = Mode;
	// Reload view ctrl
	if (fReloadView)
		{
		m_ViewCtrl.Clear(); 
		m_ViewCtrl.Default();
		CString InitRoot = GetApp()->Config.General.ExePath;
		m_ViewCtrl.Init(InitRoot,Mode);
		}
	// Adjust window size and resize mode
	if (Mode==C4VC_Player) 
		{
		// Restore window position for this mode
		RestoreWindowPosition(m_hWnd, "WindowPlayer", GetCfg()->GetSubkeyPath("Explorer"), fKeepHidden);
		// Context help, no minimize or maximize
		ModifyStyle(WS_MINIMIZEBOX | WS_MAXIMIZEBOX, 0);
		ModifyStyleEx(0, WS_EX_CONTEXTHELP, SWP_DRAWFRAME);
		}
	if (Mode==C4VC_Developer)
		{
		// Restore window position for this mode
		RestoreWindowPosition(m_hWnd, "WindowDeveloper", GetCfg()->GetSubkeyPath("Explorer"), fKeepHidden);
		// Minimize, maximize - no context help
		ModifyStyleEx(WS_EX_CONTEXTHELP, 0);
		ModifyStyle(0, WS_MINIMIZEBOX | WS_MAXIMIZEBOX, SWP_DRAWFRAME);
		}
	// Reset focus
	m_ViewCtrl.SetFocus();	
	m_ViewCtrl.SelectFirstItem();
	}

int C4ExplorerDlg::GetMode()
	{
	return Mode;
	}

void C4ExplorerDlg::OnActivateApp(BOOL bActive, HTASK hTask)
	{
	if (bActive)
		{
		// Store last selected item by path
		C4ViewItem *pViewItem=m_ViewCtrl.GetSelectedViewItem();
		CString sSelectedItemPath; if (pViewItem) sSelectedItemPath=pViewItem->Path;
		// Check modified edit slots
		GetApp()->EditSlots.UpdateModifiedItems();
		// Reselect last item
		m_ViewCtrl.SelectViewItem(sSelectedItemPath);
		// Reset focus
		m_ViewCtrl.SetFocus();
		// Check registration
		CheckRegistration();
		}
	}

void C4ExplorerDlg::OnActivate( UINT nState, CWnd* pWndOther, BOOL bMinimized )
	{
	//TRACE("Activate\r\n");
	}

void C4ExplorerDlg::OnMouseMove(UINT nFlags, CPoint point) 
	{
	// Call default
	CDialog::OnMouseMove(nFlags, point);
	}

/*bool C4ExplorerDlg::StartConditionCheck(C4ViewItem *pViewItem) // NOW OBSOLETE
	{
	CString sMsg;
	
	// No item or no scenario
	if (!pViewItem || (pViewItem->Type!=C4IT_Scenario)) 
		{
		C4MessageBox("IDS_MSG_STARTSELECTSCENARIO");
		return false;	
		}

	// No engine
	if (!pUsedEngine)
		{
		CString errMsg;
		// Network reference: need exact version
		if (pViewItem->EnableJoin)
			errMsg.Format(LoadResStr("IDS_PRC_NOEXACTC4X"), pViewItem->C4XVer1, pViewItem->C4XVer2, pViewItem->C4XVer3, pViewItem->C4XVer4);
		// Normal scenario: need same or higher version
		else
			errMsg.Format(LoadResStr("IDS_MSG_NOENGINE"), pViewItem->C4XVer1, pViewItem->C4XVer2, pViewItem->C4XVer3, pViewItem->C4XVer4);
		C4MessageBox(errMsg);
		return false;
		}

	// Player number check, if not developer mode, savegame or replay
	if ((Mode!=C4VC_Developer) && !pViewItem->SaveGame && !pViewItem->EnableReplay)
		{
		int iPlayerNum = SModuleCount( GetCfg()->General.Participants );
		// Too few players
		if (iPlayerNum<pViewItem->MinPlayer)
			// Fail
			{ sMsg.Format(LoadResStr("IDS_MSG_TOOFEWPLAYERS"),pViewItem->MinPlayer); C4MessageBox(sMsg); return false; }
		// Too many players
		if (iPlayerNum>pViewItem->MaxPlayer)
			{ sMsg.Format(LoadResStr("IDS_MSG_TOOMANYPLAYERS"),pViewItem->MaxPlayer); C4MessageBox(sMsg); return false; }
		}
	
	// Network: mode
	if (pViewItem->EnableJoin)
		{
		if (pViewItem->DeveloperMode && (Mode!=C4VC_Developer))
			{ C4MessageBox("IDS_MSG_STARTDEVMODE"); return false; }
		if (!pViewItem->DeveloperMode && (Mode==C4VC_Developer))
			{ C4MessageBox("IDS_MSG_STARTPLAYERMODE"); return false; }
		}
	
	// Network: no runtime join
	if (pViewItem->EnableJoin)
		if (pViewItem->NoRuntimeJoin)
			{ C4MessageBox("IDS_MSG_NORUNTIMEJOIN"); return false; }
		
	// Unregistered
	if (!GetCfg()->Registered() && !pViewItem->UnregisteredAccess)
		{ C4MessageBox("IDS_MSG_NOUNREGISTERED"); return false; }
	
	// Check specified definitions available
	char szDefsMissing[10*_MAX_PATH+1];
	if (!pViewItem->Definitions.AssertModules(GetCfg()->General.ExePath,szDefsMissing))
		{ sMsg.Format(LoadResStr("IDS_MSG_PRESETDEFSMISSING"),szDefsMissing); C4MessageBox(sMsg); return false; }
	
	// Okay
	return true;
	}*/

void C4ExplorerDlg::Clear()
	{
	m_ViewCtrl.Clear();
	}

void C4ExplorerDlg::OnSize(UINT nType, int cx, int cy) 
	{
	// Resize contents and store new size
	if (!(nType & SIZE_MINIMIZED))
		{
		ResizeControls(ClientAreaWidth,ClientAreaHeight,cx,cy);
		ClientAreaWidth=cx; ClientAreaHeight=cy;
		}
	// Base class size
	CDialog::OnSize(nType, cx, cy);
	// Reset focus
	m_ViewCtrl.SetFocus();
	}

const int RPC_Align_None				 = 0,
					RPC_Align_Top					 = 1,
					RPC_Align_Bottom			 = 2,
					RPC_Align_Left				 = 4,
					RPC_Align_Right				 = 8,
					RPC_Stretch_Left			 = 16,
					RPC_Stretch_Right			 = 32,
					RPC_Stretch_Top				 = 64,
					RPC_Stretch_Bottom		 = 128;

void C4ExplorerDlg::ResizeControls(int iLastWidth, int iLastHeight, int iWidth, int iHeight)
	{
	// Menu divider
	RepositionControl(m_StaticDivider,RPC_Stretch_Left | RPC_Stretch_Right,iLastWidth,iLastHeight,iWidth,iHeight);
	// View ctrl
	RepositionControl(m_ViewCtrl,RPC_Stretch_Bottom | RPC_Stretch_Right,iLastWidth,iLastHeight,iWidth,iHeight);
	// Desc frames & statics
	RepositionControl(m_FrameLarge,RPC_Align_Right | RPC_Stretch_Bottom,iLastWidth,iLastHeight,iWidth,iHeight);
	RepositionControl(m_FrameSmall,RPC_Align_Right | RPC_Stretch_Bottom,iLastWidth,iLastHeight,iWidth,iHeight);
	RepositionControl(m_FrameCenter,RPC_Align_Right,iLastWidth,iLastHeight,iWidth,iHeight);
	RepositionControl(m_StaticAuthor,RPC_Align_Right | RPC_Align_Bottom,iLastWidth,iLastHeight,iWidth,iHeight);
	RepositionControl(m_StaticPicture,RPC_Align_Right,iLastWidth,iLastHeight,iWidth,iHeight);
	RepositionControl(m_Desc,RPC_Align_Right | RPC_Stretch_Bottom,iLastWidth,iLastHeight,iWidth,iHeight);
	// Right hand side top buttons & mode radios
	RepositionControl(m_ButtonNew,RPC_Align_Right,iLastWidth,iLastHeight,iWidth,iHeight);
	RepositionControl(m_ButtonActivate,RPC_Align_Right,iLastWidth,iLastHeight,iWidth,iHeight);
	RepositionControl(m_ButtonDelete,RPC_Align_Right,iLastWidth,iLastHeight,iWidth,iHeight);
	RepositionControl(m_ButtonRename,RPC_Align_Right,iLastWidth,iLastHeight,iWidth,iHeight);
	RepositionControl(m_ButtonProperties,RPC_Align_Right,iLastWidth,iLastHeight,iWidth,iHeight);
	// Right hand side bottom buttons & text
	RepositionControl(m_StaticSwitch, RPC_Align_Right | RPC_Align_Bottom,iLastWidth,iLastHeight,iWidth,iHeight);
	RepositionControl(m_ButtonStart, RPC_Align_Right | RPC_Align_Bottom,iLastWidth,iLastHeight,iWidth,iHeight);
	// Status bar
	RepositionControl(m_EditStatus,RPC_Align_Left | RPC_Align_Bottom | RPC_Stretch_Right,iLastWidth,iLastHeight,iWidth,iHeight);
	}

void C4ExplorerDlg::RepositionControl(CWnd &rControl, int iAlign, int iLastWidth, int iLastHeight, int iNewWidth, int iNewHeight)
	{
	// Get last control position
	RECT rctLast; 
	rControl.GetWindowRect(&rctLast);
	ScreenToClient(&rctLast);
	// Calculate new position
	int iX=rctLast.left,iY=rctLast.top,iWdt=rctLast.right-rctLast.left,iHgt=rctLast.bottom-rctLast.top;
	int iLastX=iX,iLastY=iY,iLastWdt=iWdt,iLastHgt=iHgt;
	// Alignment (by original distance to border)
	if (iAlign & RPC_Align_Left) iX = iX;
	if (iAlign & RPC_Align_Right)	iX = iNewWidth-(iLastWidth-iX);
	if (iAlign & RPC_Align_Top)	iY = iY;
	if (iAlign & RPC_Align_Bottom) iY = iNewHeight-(iLastHeight-iY);
	// Stretching (by original distance to border)
	if (iAlign & RPC_Stretch_Left) { iWdt+=iX-iLastX; iX=iLastX; }
	if (iAlign & RPC_Stretch_Right) iWdt = iNewWidth-iX-(iLastWidth-iLastX-iLastWdt);
	if (iAlign & RPC_Stretch_Bottom) iHgt = iNewHeight-iY-(iLastHeight-iLastY-iLastHgt);
	// Set new position
	rControl.SetWindowPos(NULL,iX,iY,iWdt,iHgt,SWP_NOCOPYBITS);
	}

void C4ExplorerDlg::OnGetMinMaxInfo(MINMAXINFO FAR *lpMMI)
	{
	lpMMI->ptMinTrackSize.x = PlayerModeWindowSize.Width();
	//lpMMI->ptMinTrackSize.y = PlayerModeWindowSize.Height();
	lpMMI->ptMinTrackSize.y = 300; // new hardcoded limmit to allow going smaller than the original size
	}

void C4ExplorerDlg::ResetFocus()
	{
	m_ViewCtrl.SetFocus();
	}

void C4ExplorerDlg::OnClearViewItem(C4ViewItem *pViewItem)
{
	if(&pViewItem->Picture == m_pPicture) m_pPicture = NULL;
	if(&pViewItem->PNGPicture == m_pPNGPicture) m_pPNGPicture = NULL;
}

void C4ExplorerDlg::UpdateUsedEngine()
	{
	m_ViewCtrl.UpdateUsedEngine(m_ViewCtrl.GetSelectedViewItem());
	}

void C4ExplorerDlg::CheckRegistration()
{
	// Store last state
	BOOL wasRegistered = GetCfg()->Registered();
	// Reload registration
	GetCfg()->LoadRegistration();
	// There has been a change
	if (GetCfg()->Registered() != wasRegistered)
		{
		// Update group maker
		GetApp()->SetUserGroupMaker();
		// Update and reload after change
		GetCfg()->Save();
		UpdateText();
		m_ViewCtrl.PostMessage(WM_KEYDOWN, VK_F5, 0);
		}
}

C4ViewCtrl* C4ExplorerDlg::GetViewCtrl()
{
	return &m_ViewCtrl;
}

void C4ExplorerDlg::ReloadItems(const char *szItems, bool fRefreshParent, bool fReloadOnly)
{	
	m_ViewCtrl.ReloadItems(szItems, fRefreshParent, fReloadOnly);
}

void C4ExplorerDlg::OnSwitchToEngine()
{
	// Store participants in configuration
	GetCfg()->ValidateModules(GetCfg()->General.ExePath, GetCfg()->General.Participants);
	// Store definitions in configuration
	GetCfg()->ValidateModules(GetCfg()->General.ExePath, GetCfg()->Explorer.Definitions);
	SCopy(GetCfg()->Explorer.Definitions, GetCfg()->General.Definitions);
	// Save configuration
	GetCfg()->Explorer.Reload[0] = 0;
	GetCfg()->Save();
	// Compose command line
	CString CmdLine; CmdLine.Format("%s /fullscreen /nosplash", C4CFN_Engine);
	// Launch
	STARTUPINFO StartupInfo; SetZero(StartupInfo);
	StartupInfo.cb = sizeof StartupInfo;
	PROCESS_INFORMATION ProcessInfo; SetZero(ProcessInfo);
	char CmdBuf[4096]; SCopy(CmdLine,CmdBuf);
	CreateProcess(NULL, CmdBuf, NULL, NULL, TRUE, 0, NULL, NULL, &StartupInfo, &ProcessInfo);
	// Get outta here...
	PostQuitMessage(0);
}

void C4ExplorerDlg::OnChangeCommandLine() 
{
	if (!m_CommandLineEnabled) return;
	// Store current command line
  CStringEx strText; 
	m_EditStatus.GetWindowText(strText);
	SCopy(strText, GetCfg()->Explorer.CommandLine, CFG_MaxString);
}

void C4ExplorerDlg::OnHelpOnlineDocs() 
{
	CString strUrl; strUrl.Format("http://www.clonk.de/docs/%s", SEqual2(GetCfg()->General.Language, "DE") ? "de" : "en");
	ShellExecute(NULL, "open", strUrl, NULL, NULL, SW_SHOWNORMAL);
}
