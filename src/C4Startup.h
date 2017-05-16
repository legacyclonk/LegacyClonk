/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2005, Sven2
 * Copyright (c) 2017-2019, The LegacyClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

// Startup screen for non-parameterized engine start

#pragma once

#include "C4Gui.h"

#define C4CFN_StartupBackgroundMain "StartupMainMenuBG"

// special colors for startup designs
const int32_t C4StartupFontClr         = 0xff000000,
              C4StartupFontClrDisabled = 0xff7f7f7f,
              C4StartupEditBGColor     = 0xff000000,
              C4StartupEditBorderColor = 0x00a4947a,
              C4StartupBtnFontClr      = 0xff202020,
              C4StartupBtnBorderColor1 = 0x00ccc3b4,
              C4StartupBtnBorderColor2 = 0x0094846a;

// graphics needed only by startup
class C4StartupGraphics
{
private:
	bool LoadFile(C4FacetExID &rToFct, const char *szFilename);

public:
	// backgrounds
	C4FacetExID fctScenSelBG; // for scenario selection screen
	C4FacetExID fctPlrSelBG;  // for player selection screen
	C4FacetExID fctPlrPropBG; // for player property subpage
	C4FacetExID fctNetBG;     // for network screen
	C4FacetExID fctAboutBG;   // for about screen

	// big buttons used in main menu
	C4FacetExID fctMainButtons, fctMainButtonsDown;
	C4GUI::DynBarFacet barMainButtons, barMainButtonsDown;

	// scroll bars in book
	C4FacetExID fctBookScroll;
	C4GUI::ScrollBarFacets sfctBookScroll, sfctBookScrollR, sfctBookScrollG, sfctBookScrollB;

	// color preview
	C4FacetExID fctCrew, fctCrewClr; // ColorByOwner-surface of fctCrew

	// scenario selection: Scenario and folder icons
	C4FacetExID fctScenSelIcons;
	// scenario selection: Title overlay
	C4FacetExID fctScenSelTitleOverlay;
	// scenario selection and player selection book fonts
	CStdFont BookFontCapt, BookFont, BookFontTitle, BookSmallFont;

	// context button for combo boxes on white
	C4FacetExID fctContext;

	// player selection: player property control type buttons
	C4FacetExID fctPlrCtrlType;

	// options dlg gfx
	C4FacetExID fctOptionsDlgPaper, fctOptionsIcons, fctOptionsTabClip;

	// net dlg gfx
	C4FacetExID fctNetGetRef;

	bool Init();
	bool InitFonts();

	CStdFont &GetBlackFontByHeight(int32_t iHgt, float *pfZoom); // get optimal font for given control size
};

// base class for all startup dialogs
class C4StartupDlg : public C4GUI::FullscreenDialog
{
public:
	C4StartupDlg(const char *szTitle) : C4GUI::FullscreenDialog(szTitle, NULL) {}
};

class C4Startup
{
public:
	C4Startup();
	~C4Startup();

public:
	C4StartupGraphics Graphics;

	enum DialogID { SDID_Main = 0, SDID_ScenSel, SDID_ScenSelNetwork, SDID_NetJoin, SDID_Options, SDID_About, SDID_PlrSel, SDID_Back };

private:
	bool fInStartup, fAborted, fLastDlgWasBack;
	static C4Startup *pInstance; // singleton instance
	static DialogID eLastDlgID;
	static bool fFirstRun;

	C4StartupDlg *pLastDlg, *pCurrDlg; // startup dlg that is currently shown, and dialog that was last shown

protected:
	// break modal loop and...
	void Start(); // ...start game
	void Exit();  // ...quit to system

	bool DoStartup(); // run main dlg
	class C4StartupDlg *SwitchDialog(DialogID eToDlg, bool fFade = true); // do transition to another dialog

	friend class C4StartupMainDlg;
	friend class C4StartupNetDlg;
	friend class C4StartupScenSelDlg;
	friend class C4StartupOptionsDlg;
	friend class C4StartupAboutDlg;
	friend class C4StartupPlrSelDlg;

public:
	static C4Startup *EnsureLoaded(); // create and load startup data if not done yet
	static void Unload(); // make sure startup data is destroyed
	static bool Execute(); // run startup - return false if game is to be closed
	static bool SetStartScreen(const char *szScreen); // set screen that is shown first by case insensitive identifier

	static C4Startup *Get() { assert(pInstance); return pInstance; }
	static bool WasFirstRun() { return fFirstRun; }
};
