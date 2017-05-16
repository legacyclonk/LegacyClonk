#if !defined(AFX_C4PAGEBEHAVIOR_H__997BBE21_186A_11D3_8889_0040052C10D3__INCLUDED_)
#define AFX_C4PAGEBEHAVIOR_H__997BBE21_186A_11D3_8889_0040052C10D3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// C4PageBehavior.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// C4PageBehavior dialog

class C4PageBehavior : public CDialog
{

// Construction
public:
	void GetData(C4DefCore &rDef);
	void SetData(C4DefCore &rDef);
	C4PageBehavior(CWnd* pParent = NULL);  
	C4DefListEx* pDefs;

	//{{AFX_DATA(C4PageBehavior)
	enum { IDD = IDD_PAGEBEHAVIOR };
	CButton	m_RadioGrowthType1;
	CButton	m_RadioGrowthType2;
	CButton	m_RadioGrab1;
	CButton	m_RadioGrab2;
	CButton	m_RadioGrab3;
	CStatic	m_StaticBuildTurnTo;
	CStatic	m_StaticNone3;
	CButton	m_StaticFireBehavior;
	CButton	m_StaticSize;
	CStatic	m_StaticNone2;
	CStatic	m_StaticNone;
	CStatic	m_StaticHigh2;
	CStatic	m_StaticHigh;
	CStatic	m_StaticGrowth;
	CButton	m_StaticGrab;
	CStatic	m_StaticFast;
	CStatic	m_StaticContactIncinerate;
	CButton	m_StaticComponents;
	CStatic	m_StaticBurnTurnTo;
	CButton	m_StaticBuilding;
	CButton	m_StaticBorderBound;
	CStatic	m_StaticBlastIncinerate;
	CSliderCtrl	m_SliderGrowth;
	CSliderCtrl	m_SliderContactIncinerate;
	CSliderCtrl	m_SliderBlastIncinerate;
	C4IdListCtrl	m_ListComponents;
	CButton	m_CheckOversize;
	CButton	m_CheckNoHorizontal;
	CButton	m_CheckNoBurnDecay;
	CButton	m_CheckIncompleteActivity;
	CButton	m_CheckGrabPut;
	CButton	m_CheckGrabGet;
	CButton	m_CheckExclusive;
	CButton	m_CheckConstructable;
	CButton	m_CheckCanBeBase;
	CButton	m_CheckBorderTop;
	CButton	m_CheckBorderSides;
	CButton	m_CheckBorderBottom;
	CButton	m_CheckBasement;
	C4IdCtrlButtons	m_ButtonComponent;
	CStatic	m_ButtonBurnTurnTo;
	CStatic	m_ButtonBuildTurnTo;
	BOOL	m_Basement;
	BOOL	m_BorderBottom;
	BOOL	m_BorderSides;
	BOOL	m_BorderTop;
	BOOL	m_CanBeBase;
	BOOL	m_Constructable;
	BOOL	m_Exclusive;
	BOOL	m_GrabGet;
	BOOL	m_GrabPut;
	BOOL	m_IncompleteActivity;
	BOOL	m_NoBurnDecay;
	BOOL	m_NoHorizontal;
	BOOL	m_Oversize;
	CString	m_BuildTurnTo;
	CString	m_BurnTurnTo;
	int		m_Grab;
	int		m_GrowthType;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(C4PageBehavior)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	int m_ContactIncinerate;
	int m_BlastIncinerate;
	int m_Growth;
	void UpdateText();

	// Generated message map functions
	//{{AFX_MSG(C4PageBehavior)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_C4PAGEBEHAVIOR_H__997BBE21_186A_11D3_8889_0040052C10D3__INCLUDED_)
