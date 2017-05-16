// DownloadDlg.cpp: Implementierungsdatei
//

#include "C4Explorer.h"
#include "C4DownloadDlg.h"

#include "C4WebLib.h"

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld C4DownloadDlg 


C4DownloadDlg::C4DownloadDlg(CWnd* pParent /*=NULL*/)
	: CDialog(C4DownloadDlg::IDD, pParent),
	  pDLList(NULL),
		fDlgInit(false)
{
	//{{AFX_DATA_INIT(C4DownloadDlg)
	//}}AFX_DATA_INIT
}

C4DownloadDlg::~C4DownloadDlg()
{
	while(pDLList)
	{
		DLEntry *pDLEntry = pDLList;
		pDLList = pDLEntry->Next;
		delete pDLEntry;
	}
}

void C4DownloadDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(C4DownloadDlg)
	DDX_Control(pDX, IDC_LBL_ACTIVEDL, m_ctrlActiveDL);
	DDX_Control(pDX, IDCANCEL, m_ctrlClose);
	DDX_Control(pDX, IDC_CANCELDL, m_ctrlCancelDL);
	DDX_Control(pDX, IDC_DLLIST, m_ctrlDLList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(C4DownloadDlg, CDialog)
	//{{AFX_MSG_MAP(C4DownloadDlg)
	ON_WM_TIMER()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_BN_CLICKED(IDC_CANCELDL, OnCancelDL)
	ON_WM_SHOWWINDOW()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Behandlungsroutinen für Nachrichten C4DownloadDlg 

#define TimerID 100

BOOL C4DownloadDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// create timer
	SetTimer(TimerID, 300, NULL);

	// set up the dl list
	ListView_SetExtendedListViewStyle(m_ctrlDLList.m_hWnd, LVS_EX_HEADERDRAGDROP);

	// create columns
	m_ctrlDLList.InsertColumn(CI_FileName, "FILENAME");
	m_ctrlDLList.InsertColumn(CI_Size, "FILESIZE");
	m_ctrlDLList.InsertColumn(CI_Progress, "FILEPROGRESS");
	m_ctrlDLList.InsertColumn(CI_Target, "TARGET");

	DWORD OldStyle = GetWindowLong(m_ctrlDLList, GWL_STYLE);
	SetWindowLong(m_ctrlDLList, GWL_STYLE, (OldStyle & ~LVS_TYPEMASK) | LVS_REPORT);

	// set auto width
	m_ctrlDLList.SetColumnWidth(CI_FileName, LVSCW_AUTOSIZE_USEHEADER);
	m_ctrlDLList.SetColumnWidth(CI_Size, LVSCW_AUTOSIZE_USEHEADER);
	m_ctrlDLList.SetColumnWidth(CI_Progress, LVSCW_AUTOSIZE_USEHEADER);
	m_ctrlDLList.SetColumnWidth(CI_Target, LVSCW_AUTOSIZE_USEHEADER);

	// set size minimum
	RECT WindowRect; GetClientRect(&WindowRect);
	iMinSizeX = WindowRect.right; iMinSizeY = WindowRect.bottom;

	// Update text
	UpdateText();

	iActiveDownloadCnt = 0;
	fDlgInit = true;
	return TRUE;
}

const double C4DL_REMOVAL_DELAY = 2.0;

void C4DownloadDlg::OnTimer(UINT nIDEvent) 
{
	if(nIDEvent == TimerID)
	{
		// search 		
		DLEntry *pPos, *pLast;
		for(pPos = pDLList, pLast = NULL; pPos; !pPos || (pLast = pPos, pPos = pPos->Next))
			while(pPos && !pPos->DLInfo 
				         && pPos->RemovalTimer != -1 
								 && difftime(time(NULL), pPos->RemovalTimer) > C4DL_REMOVAL_DELAY)
			{
				RemoveDLEntry(pPos);
				pPos = pLast ? pLast->Next : pDLList;
			}
		// refresh view
		RefreshView();
		// no entry left? hide window
		if(!pDLList) ShowWindow(SW_HIDE);
	}
	
	CDialog::OnTimer(nIDEvent);
}

CString FileSizeStr(int iSize)
{
	CString strResult;
	if(iSize >= 1024 * 1024)
		strResult.Format("%d MB", iSize / 1024 / 1024);
	else if(iSize >= 1024)
		strResult.Format("%d KB", iSize / 1024);
	else
		strResult.Format("%d Bytes", iSize);
	return strResult;
}

void C4DownloadDlg::RefreshView()
{
	int i = 0;
	for(DLEntry *pDLEntry = pDLList; pDLEntry; pDLEntry = pDLEntry->Next, i++)
	{

		C4WebLibDownloadInfo *pDLInfo = pDLEntry->DLInfo;
		CStdHttpProgress *pProgress = pDLInfo ? pDLInfo->pStdHttpProgress : NULL;

		// what should appear in list?
		const char *strFileName = pDLEntry->Name;
		const char *strTarget = pDLEntry->Target;
		CString strSize, strProgress;
		if(pProgress && pProgress->iFileSize != -1)
		{
			strSize = FileSizeStr(pProgress->iFileSize);
			strProgress.Format("%d %%", int(double(pProgress->iReceived) / pProgress->iFileSize * 100));
		}
		else if(!pProgress)
		{
			strSize = "";
			switch(pDLEntry->Status)
			{
			case DLEntry::DLS_OK: strProgress = LoadResStr("IDS_DL_STATUSDONE"); break;
			case DLEntry::DLS_Error: strProgress = LoadResStr("IDS_DL_STATUSERROR"); break;
			case DLEntry::DLS_Cancelled: strProgress = LoadResStr("IDS_DL_STATUSCANCELLED"); break;
			}
		}
		else
		{
			strSize = "?";
			if(!pDLEntry->DLInfo->fStart)
				strProgress = LoadResStr("IDS_DL_STATUSWAITING");
			else
				strProgress = LoadResStr("IDS_DL_STATUSCONNECTING");
		}

		// insert entry (if neccessary)
		if(i >= m_ctrlDLList.GetItemCount())
			m_ctrlDLList.InsertItem(i, strFileName);

		// set it
		m_ctrlDLList.SetItemText(i, CI_FileName, strFileName);
		m_ctrlDLList.SetItemText(i, CI_Size, strSize);
		m_ctrlDLList.SetItemText(i, CI_Progress, strProgress);
		m_ctrlDLList.SetItemText(i, CI_Target, strTarget);

		// store dl info ptr
		m_ctrlDLList.SetItemData(i, reinterpret_cast<DWORD>(pDLEntry));

	}

	// delete remaining entries
	while(m_ctrlDLList.GetItemCount() > i)
		m_ctrlDLList.DeleteItem(m_ctrlDLList.GetItemCount() - 1);

	// set auto width
	m_ctrlDLList.SetColumnWidth(CI_FileName, LVSCW_AUTOSIZE_USEHEADER);
	m_ctrlDLList.SetColumnWidth(CI_Size, LVSCW_AUTOSIZE_USEHEADER);
	m_ctrlDLList.SetColumnWidth(CI_Progress, LVSCW_AUTOSIZE_USEHEADER);

}

BOOL C4DownloadDlg::OnEraseBkgnd(CDC* pDC) 
{
	GetApp()->dibPaper.DrawTiles(pDC, this);
	return TRUE;
}

void C4DownloadDlg::AddDL(C4WebLibDownloadInfo *pDLInfo)
{
	// already in list?
	if(FindDLInfo(pDLInfo)) return;
	DLEntry *pDLEntry = new DLEntry();
	pDLEntry->Name = GetFilenameWeb(pDLInfo->strAddr);
	pDLEntry->DLInfo = pDLInfo;
	pDLEntry->Next = NULL;
	pDLEntry->Status = DLEntry::DLS_OK;
	pDLEntry->RemovalTimer = -1;
	char path[_MAX_PATH + 1];
	SCopy(GetCfg()->AtExeRelativePath(pDLInfo->strTargetFile), path);
	if (SCharCount('\\', path))
	{
		TruncatePath(path);
		pDLEntry->Target = path;
	}
	else
		pDLEntry->Target = LoadResStr("IDS_FN_ROOT");
	// add it
	for (DLEntry *pPos = pDLList; pPos && pPos->Next; pPos = pPos->Next);
	(pPos ? pPos->Next : pDLList) = pDLEntry;
	// show item
//	RefreshView();
	// activate download
	if(iActiveDownloadCnt < C4MaxActiveDownload)
	{
		pDLInfo->fStart = true;
		iActiveDownloadCnt++;
	}
}

void C4DownloadDlg::EndDL(C4WebLibDownloadInfo *pDLInfo, BOOL fSuccess)
{
	DLEntry *pDLEntry = FindDLInfo(pDLInfo);
	if(!pDLEntry) return;
	// DLInfo ist invalid now (the object that is pointed on will
	// be deleted)
	pDLEntry->DLInfo = NULL;
	if(pDLEntry->Status != DLEntry::DLS_Cancelled)
		pDLEntry->Status = fSuccess ? DLEntry::DLS_OK : DLEntry::DLS_Error;
	// success -> remove the entry after some time
	if(fSuccess) pDLEntry->RemovalTimer = time(NULL);
	
	if(pDLInfo->fStart)
	{
		// search for another download to activate	
		for(DLEntry *pPos = pDLList; pPos; pPos = pPos->Next)
			if(pPos->DLInfo)
				if(!pPos->DLInfo->fStart)
				{
					pPos->DLInfo->fStart = true;
					break;
				}
		if(!pPos) iActiveDownloadCnt--;
	}
}

void C4DownloadDlg::RemoveDLEntry(DLEntry *pDLEntry)
{
	// search it
	if(pDLList == pDLEntry)
		pDLList = pDLEntry->Next;
	else
	{
		for(DLEntry *pPos = pDLList; pPos; pPos = pPos->Next)
			if(pPos->Next == pDLEntry)
				break;
		if(pPos)
			pPos->Next = pDLEntry->Next;
	}
	// delete the entry
	delete pDLEntry;
}

C4DownloadDlg::DLEntry *C4DownloadDlg::FindDLInfo(C4WebLibDownloadInfo *pDL)
{
	for(DLEntry *pPos = pDLList; pPos; pPos=pPos->Next)
		if(pPos->DLInfo == pDL)
			return pPos;
	return NULL;
}

void C4DownloadDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	
	if(!fDlgInit) return;

	if(nType != SIZE_MINIMIZED)
	{

		// calc border size
		RECT pos, pos2;
		m_ctrlActiveDL.GetWindowRect(&pos); ScreenToClient(&pos);
		int iBorder = pos.left;

		// rearrange the controls
		m_ctrlClose.GetWindowRect(&pos); ScreenToClient(&pos);
		pos.left = pos.left - pos.right + cx - iBorder; pos.right = cx - iBorder;
		pos.top = pos.top - pos.bottom + cy - iBorder; pos.bottom = cy - iBorder;
		m_ctrlClose.SetWindowPos(NULL, pos.left, pos.top, pos.right - pos.left, pos.bottom - pos.top, SWP_NOCOPYBITS);
		
		m_ctrlCancelDL.GetWindowRect(&pos2); ScreenToClient(&pos2);
		pos2.top = pos.top; pos2.bottom = pos.bottom;
		m_ctrlCancelDL.SetWindowPos(NULL, pos2.left, pos2.top, pos2.right - pos2.left, pos2.bottom - pos2.top, SWP_NOCOPYBITS);

		m_ctrlDLList.GetWindowRect(&pos2); ScreenToClient(&pos2);
		pos2.right = cx - iBorder;
		pos2.bottom = pos.top - iBorder;
		m_ctrlDLList.SetWindowPos(NULL, pos2.left, pos2.top, pos2.right - pos2.left, pos2.bottom - pos2.top, SWP_NOCOPYBITS);

	}
}

void C4DownloadDlg::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{
	if(!fDlgInit) return;
	lpMMI->ptMinTrackSize.x = iMinSizeX;
	lpMMI->ptMinTrackSize.y = iMinSizeY;
}


void C4DownloadDlg::OnCancelDL() 
{
	// get the view item info
	int iSel = m_ctrlDLList.GetSelectionMark();
	if(iSel < 0) return;
	DLEntry *pDLEntry = reinterpret_cast<DLEntry *>(m_ctrlDLList.GetItemData(iSel));
	// for security: entry in list?
	for(DLEntry *pPos = pDLList; pPos && pPos != pDLEntry; pPos = pPos->Next);
	if(pPos != pDLEntry) return;
	// ok, set "cancel" flag
	if(pDLEntry->DLInfo)
	{
		pDLEntry->DLInfo->pStdHttpProgress->bContinue = false;
		pDLEntry->Status = DLEntry::DLS_Cancelled;
	}
	else
		RemoveDLEntry(pDLEntry);
}

void C4DownloadDlg::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CDialog::OnShowWindow(bShow, nStatus);	
	if (bShow) 
	{
		RestoreWindowPosition(m_hWnd, "WindowDownload", GetCfg()->GetSubkeyPath("Explorer"));	
		UpdateText();
	}
	else
		StoreWindowPosition(m_hWnd, "WindowDownload", GetCfg()->GetSubkeyPath("Explorer"));	
}

void C4DownloadDlg::OnClose() 
{
	StoreWindowPosition(m_hWnd, "WindowDownload", GetCfg()->GetSubkeyPath("Explorer"));	
	CDialog::OnClose();
}

void C4DownloadDlg::OnDestroy() 
{
	StoreWindowPosition(m_hWnd, "WindowDownload", GetCfg()->GetSubkeyPath("Explorer"));	
	CDialog::OnDestroy();
}

void C4DownloadDlg::UpdateText()
{
	if (!IsWindow(m_hWnd)) return;
	// Set window text
	SetWindowText(LoadResStr("IDS_DL_CAPTION"));
	m_ctrlActiveDL.Set("IDS_DL_ACTIVEDOWNLOADS");
	m_ctrlCancelDL.Set("IDS_DL_CANCEL");
	m_ctrlClose.Set("IDS_DL_CLOSE");
	// Set column text
	LVCOLUMN columnInf; ZeroMem(&columnInf, sizeof(LVCOLUMN)); columnInf.mask = LVCF_TEXT; 
	columnInf.pszText = LoadResStr("IDS_DL_FILENAME");
	m_ctrlDLList.SetColumn(CI_FileName, &columnInf);
	columnInf.pszText = LoadResStr("IDS_DL_FILESIZE");
	m_ctrlDLList.SetColumn(CI_Size, &columnInf);
	columnInf.pszText = LoadResStr("IDS_DL_FILEPROGRESS");
	m_ctrlDLList.SetColumn(CI_Progress, &columnInf);
	columnInf.pszText = LoadResStr("IDS_DL_TARGET");
	m_ctrlDLList.SetColumn(CI_Target, &columnInf);
}
