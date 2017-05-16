/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Displays a single info bitmap */

#include "C4Explorer.h"

#include "C4AboutDlg.h"
#include "FileVersion.h"

BEGIN_MESSAGE_MAP(C4AboutDlg, CDialog)
 //{{AFX_MSG_MAP(C4AboutDlg)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

C4AboutDlg::C4AboutDlg(CWnd* pParent /*=NULL*/)
	: CDialog(C4AboutDlg::IDD, pParent)
	{
	//{{AFX_DATA_INIT(C4AboutDlg)
	//}}AFX_DATA_INIT
	}


void C4AboutDlg::DoDataExchange(CDataExchange* pDX)
	{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(C4AboutDlg)
	//}}AFX_DATA_MAP
	}

void C4AboutDlg::OnPaint() 
	{
	// Create a nice font for the version strings
	CFont font;
	LOGFONT LogFont; ZeroMem(&LogFont, sizeof(LogFont));
	SCopy("Verdana", LogFont.lfFaceName);
	LogFont.lfHeight = 10 * 10;
	//LogFont.lfWeight = FW_BOLD;
	font.CreatePointFontIndirect(&LogFont);
	// Draw version info	
	CPaintDC DC(this);
	DC.SetBkMode(TRANSPARENT);
	CRect Rect2(m_RectVersion.left+2,m_RectVersion.top+2,m_RectVersion.right+2,m_RectVersion.bottom+2);
	DC.SelectObject(font);
	DC.SetTextColor(BLACK);
	DC.DrawText(m_Version, -1, &Rect2, DT_CENTER);
	}

BOOL C4AboutDlg::OnEraseBkgnd(CDC* pDC) 
	{
	dibSplash.Draw(pDC->m_hDC);
	return true;
	}

BOOL C4AboutDlg::OnInitDialog() 
	{
	CDialog::OnInitDialog();
	// Load bitmaps
	dibSplash.Load(IDB_SPLASH);
	// Dialog auf Bitmapgröße resizen
	CSize BmpSize = dibSplash.GetSize();
	MoveWindow(0,0, BmpSize.cx, BmpSize.cy);
	// Center dialog
	CenterWindow(GetDesktopWindow());	
	// Determine versions
	char szModule[_MAX_PATH+1]; GetModuleFileName(NULL,szModule,_MAX_PATH);
	CFileVersion FEVersion( szModule );
	CFileVersion RXVersion( GetCfg()->AtExePath(C4CFN_Engine) );	
	m_Version.Format("Clonk Editor %s\nEngine %s", FEVersion.GetFileVersion(), RXVersion.GetFileVersion());
	m_RectVersion = CRect( 0,BmpSize.cy-50, BmpSize.cx,BmpSize.cy );
	// Done
	return TRUE; 
	}

void C4AboutDlg::OnLButtonDown(UINT nFlags, CPoint point) 
	{
	OnOK();
	}
