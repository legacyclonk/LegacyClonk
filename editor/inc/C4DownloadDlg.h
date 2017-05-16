#if !defined(AFX_DOWNLOADDLG_H__9BE94F7F_D032_4DD1_9F7C_4ED5110CAD3D__INCLUDED_)
#define AFX_DOWNLOADDLG_H__9BE94F7F_D032_4DD1_9F7C_4ED5110CAD3D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class C4WebLibDownloadInfo;

const int C4MaxActiveDownload = 5;

class C4DownloadDlg : public CDialog
{

public:

	C4DownloadDlg(CWnd* pParent = NULL);
	~C4DownloadDlg();

	void AddDL(C4WebLibDownloadInfo *pDLInfo);
	void EndDL(C4WebLibDownloadInfo *pDLInfo, BOOL fSuccess);

	void UpdateText();

protected:

	enum ColumnIndex
	{
		CI_FileName,
		CI_Size,
		CI_Progress,
		CI_Target
	};

	struct DLEntry
	{
		CString Name;
		C4WebLibDownloadInfo *DLInfo;
		DLEntry *Next;
		time_t RemovalTimer;
		CString Target;
		enum
		{
			DLS_OK,
			DLS_Error,
			DLS_Cancelled,
		} 
		  Status;
	}
	  *pDLList;

	void RefreshView();

	DLEntry *FindDLInfo(C4WebLibDownloadInfo *pDL);
	void RemoveDLEntry(DLEntry *pDLEntry);

	bool fDlgInit;
	int iMinSizeX, iMinSizeY;

	int iActiveDownloadCnt;

protected:

	// MFC stuff

	//{{AFX_MSG(C4DownloadDlg)
	afx_msg void OnTimer(UINT nIDEvent);
	//afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnCancelDL();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	//{{AFX_DATA(C4DownloadDlg)
	enum { IDD = IDD_DOWNLOAD };
	CStaticEx m_ctrlActiveDL;
	CButtonEx m_ctrlClose;
	CButtonEx m_ctrlCancelDL;
	CListCtrl	m_ctrlDLList;
	//}}AFX_DATA

	//{{AFX_VIRTUAL(C4DownloadDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV-Unterstützung
	//}}AFX_VIRTUAL

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ fügt unmittelbar vor der vorhergehenden Zeile zusätzliche Deklarationen ein.

#endif // AFX_DOWNLOADDLG_H__9BE94F7F_D032_4DD1_9F7C_4ED5110CAD3D__INCLUDED_
