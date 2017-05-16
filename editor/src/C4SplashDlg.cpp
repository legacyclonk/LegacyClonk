/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Startup splash screen */

#include "C4Explorer.h"

BEGIN_MESSAGE_MAP(C4SplashDlg, CDialog)
	//{{AFX_MSG_MAP(C4SplashDlg)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


C4SplashDlg::C4SplashDlg(CWnd* pParent /*=NULL*/)
	: CDialog(C4SplashDlg::IDD, pParent)
	{
	SplashAnimationAvailable=false;
	}

void C4SplashDlg::DoDataExchange(CDataExchange* pDX)
	{
	CDialog::DoDataExchange(pDX);
	}

bool C4SplashDlg::Show()
	{
	// Erzeugen
	Create(IDD_INTRO);

	// Nichtmodal zeigen
	ShowWindow(SW_SHOW);
	UpdateWindow();

	return true;
	}

void C4SplashDlg::Hide()
	{
	DestroyWindow();	
	}

BOOL C4SplashDlg::OnInitDialog() 
	{
	CDialog::OnInitDialog();

	// Bild laden
	SplashBitmap.Load(IDB_SPLASH);

	// Icons zuweisen
	SetIcon(GetApp()->LoadIcon(IDR_MAINFRAME), true);
	SetIcon(GetApp()->LoadIcon(IDR_MAINFRAME), false);

	// Window Caption setzen
	SetWindowText("Loading...");

	// Größe an Bild anpassen
	MoveWindow(0,0, 320,240);
	CenterWindow();

	return true;
	}

void C4SplashDlg::OnPaint() 
	{
	CPaintDC dc(this); // device context for painting
	SplashBitmap.Draw(&dc, this);	
	}
