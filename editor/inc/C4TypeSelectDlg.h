#if !defined(AFX_TYPESELECTDLG_H__64266361_86AC_11D2_8888_0040052C10D3__INCLUDED_)
#define AFX_TYPESELECTDLG_H__64266361_86AC_11D2_8888_0040052C10D3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class C4TypeSelectDlg: public CDialog
	{
	public:
		C4TypeSelectDlg(CWnd* pParent = NULL);
		 ~C4TypeSelectDlg();

	public:
		void Clear();

	public:
		int NewMode;
		int Selection;
		CString Location;

	//{{AFX_DATA(C4TypeSelectDlg)
	enum { IDD = IDD_TYPESELECT };
	CButtonEx m_ButtonOK;
	CButtonEx m_ButtonCancel;
	CStaticEx m_StaticLocation;
	CListCtrl	m_ListCtrl;
	//}}AFX_DATA


	//{{AFX_VIRTUAL(C4TypeSelectDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:
	void UpdateText();
	CImageList ImageList;

	//{{AFX_MSG(C4TypeSelectDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	//afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnDblclkListtype(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TYPESELECTDLG_H__64266361_86AC_11D2_8888_0040052C10D3__INCLUDED_)
