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

#include <C4Include.h>
#include <C4Startup.h>

#include <C4StartupMainDlg.h>
#include <C4StartupScenSelDlg.h>
#include <C4StartupNetDlg.h>
#include <C4StartupOptionsDlg.h>
#include <C4StartupAboutDlg.h>
#include <C4StartupPlrSelDlg.h>
#include <C4Game.h>
#include <C4Application.h>
#include <C4Log.h>

bool C4StartupGraphics::LoadFile(C4FacetExID &rToFct, const char *szFilename)
{
	return Game.GraphicsResource.LoadFile(rToFct, szFilename, Game.GraphicsResource.Files);
}

bool C4StartupGraphics::Init()
{
	// load startup specific graphics from gfxsys groupset
	fctScenSelBG.GetFace().SetBackground();
	if (!LoadFile(fctScenSelBG, "StartupScenSelBG")) return false;
	Game.SetInitProgress(45);
	if (!LoadFile(fctPlrSelBG, "StartupPlrSelBG")) return false;
	Game.SetInitProgress(50);
	if (!LoadFile(fctPlrPropBG, "StartupPlrPropBG")) return false;
	Game.SetInitProgress(55);
	if (!LoadFile(fctNetBG, "StartupNetworkBG")) return false;
	Game.SetInitProgress(57);
	if (!LoadFile(fctAboutBG, "LoaderWatercave1")) return false;
	Game.SetInitProgress(60);
	if (!LoadFile(fctMainButtons, "StartupBigButton")) return false;
	Game.SetInitProgress(62);
	barMainButtons.SetHorizontal(fctMainButtons);
	if (!LoadFile(fctMainButtonsDown, "StartupBigButtonDown")) return false;
	Game.SetInitProgress(64);
	barMainButtonsDown.SetHorizontal(fctMainButtonsDown);
	if (!LoadFile(fctBookScroll, "StartupBookScroll")) return false;
	sfctBookScroll.Set(fctBookScroll);
	sfctBookScrollR.Set(fctBookScroll, 1);
	sfctBookScrollG.Set(fctBookScroll, 2);
	sfctBookScrollB.Set(fctBookScroll, 3);
	Game.SetInitProgress(66);
	if (!LoadFile(fctContext, "StartupContext")) return false;
	fctContext.Set(fctContext.Surface, 0, 0, fctContext.Hgt, fctContext.Hgt);
	Game.SetInitProgress(67);
	if (!LoadFile(fctScenSelIcons, "StartupScenSelIcons")) return false;
	Game.SetInitProgress(68);
	fctScenSelIcons.Wdt = fctScenSelIcons.Hgt; // icon width is determined by icon height
	if (!LoadFile(fctScenSelTitleOverlay, "StartupScenSelTitleOv")) return false;
	Game.SetInitProgress(70);
	if (!LoadFile(fctPlrCtrlType, "StartupPlrCtrlType")) return false;
	fctPlrCtrlType.Set(fctPlrCtrlType.Surface, 0, 0, 128, 52);
	Game.SetInitProgress(72);
	if (!LoadFile(fctOptionsDlgPaper, "StartupDlgPaper")) return false;
	Game.SetInitProgress(74);
	if (!LoadFile(fctOptionsIcons, "StartupOptionIcons")) return false;
	fctOptionsIcons.Set(fctOptionsIcons.Surface, 0, 0, fctOptionsIcons.Hgt, fctOptionsIcons.Hgt);
	Game.SetInitProgress(76);
	if (!LoadFile(fctOptionsTabClip, "StartupTabClip")) return false;
	Game.SetInitProgress(80);
	if (!LoadFile(fctNetGetRef, "StartupNetGetRef")) return false;
	fctNetGetRef.Wdt = 40;
#ifndef USE_CONSOLE
	Game.SetInitProgress(82);
	if (!InitFonts()) return false;
#endif
	Game.SetInitProgress(100);
	return true;
}

#ifndef USE_CONSOLE
bool C4StartupGraphics::InitFonts()
{
	const char *szFont = Config.General.RXFontName;
	if (!Game.FontLoader.InitFont(BookFontCapt, szFont, C4FontLoader::C4FT_Caption, Config.General.RXFontSize, &Game.GraphicsResource.Files, false))
	{
		LogFatal("Font Error (1)"); return false;
	}
	Game.SetInitProgress(85);
	if (!Game.FontLoader.InitFont(BookFont, szFont, C4FontLoader::C4FT_Main, Config.General.RXFontSize, &Game.GraphicsResource.Files, false))
	{
		LogFatal("Font Error (2)"); return false;
	}
	Game.SetInitProgress(90);
	if (!Game.FontLoader.InitFont(BookFontTitle, szFont, C4FontLoader::C4FT_Title, Config.General.RXFontSize, &Game.GraphicsResource.Files, false))
	{
		LogFatal("Font Error (3)"); return false;
	}
	Game.SetInitProgress(95);
	if (!Game.FontLoader.InitFont(BookSmallFont, szFont, C4FontLoader::C4FT_MainSmall, Config.General.RXFontSize, &Game.GraphicsResource.Files, false))
	{
		LogFatal("Font Error (4)"); return false;
	}
	return true;
}
#endif

CStdFont &C4StartupGraphics::GetBlackFontByHeight(int32_t iHgt, float *pfZoom)
{
	// get optimal font for given control size
	CStdFont *pUseFont;
	if (iHgt <= BookSmallFont.GetLineHeight()) pUseFont = &BookSmallFont;
	else if (iHgt <= BookFont.GetLineHeight()) pUseFont = &BookFont;
	else if (iHgt <= BookFontCapt.GetLineHeight()) pUseFont = &BookFontCapt;
	else pUseFont = &BookFontTitle;
	// determine zoom
	if (pfZoom)
	{
		int32_t iLineHgt = pUseFont->GetLineHeight();
		if (iLineHgt)
			*pfZoom = (float)iHgt / (float)iLineHgt;
		else
			*pfZoom = 1.0f; // error
	}
	return *pUseFont;
}

// statics
C4Startup::DialogID C4Startup::eLastDlgID = C4Startup::SDID_Main;

// startup singleton instance
C4Startup *C4Startup::pInstance = nullptr;

C4Startup::C4Startup() : fInStartup(false), fAborted(false), pLastDlg(nullptr), pCurrDlg(nullptr)
{
	// must be single!
	assert(!pInstance);
	pInstance = this;
}

C4Startup::~C4Startup()
{
	pInstance = nullptr;
	if (Game.pGUI)
	{
		delete pLastDlg;
		delete pCurrDlg;
	}
}

void C4Startup::Start()
{
	assert(fInStartup);
	// record if desired
	if (Config.General.Record) Game.Record = true;
	// flag game start
	fAborted = false;
	fInStartup = false;
	fLastDlgWasBack = false;
};

void C4Startup::Exit()
{
	assert(fInStartup);
	// flag game start
	fAborted = true;
	fInStartup = false;
};

C4StartupDlg *C4Startup::SwitchDialog(DialogID eToDlg, bool fFade)
{
	// can't go back twice, because dialog is not remembered: Always go back to main in this case
	if (eToDlg == SDID_Back && (fLastDlgWasBack || !pLastDlg)) eToDlg = SDID_Main;
	fLastDlgWasBack = false;
	// create new dialog
	C4StartupDlg *pToDlg = nullptr;
	switch (eToDlg)
	{
	case SDID_Main:
		pToDlg = new C4StartupMainDlg();
		break;
	case SDID_ScenSel:
		pToDlg = new C4StartupScenSelDlg(false);
		break;
	case SDID_ScenSelNetwork:
		pToDlg = new C4StartupScenSelDlg(true);
		break;
	case SDID_NetJoin:
		pToDlg = new C4StartupNetDlg();
		break;
	case SDID_Options:
		pToDlg = new C4StartupOptionsDlg();
		break;
	case SDID_About:
		pToDlg = new C4StartupAboutDlg();
		break;
	case SDID_PlrSel:
		pToDlg = new C4StartupPlrSelDlg();
		break;
	case SDID_Back:
		pToDlg = pLastDlg;
		fLastDlgWasBack = true;
		break;
	};
	assert(pToDlg);
	if (!pToDlg) return nullptr;
	if (pToDlg != pLastDlg)
	{
		// remember current position
		eLastDlgID = eToDlg;
		// kill any old dialog
		delete pLastDlg;
	}
	// retain current dialog as last, so it can fade out and may be used later
	if (pLastDlg = pCurrDlg)
		if (fFade)
		{
			if (!pLastDlg->IsShown()) pLastDlg->Show(Game.pGUI, false);
			pLastDlg->FadeOut(true);
		}
		else
		{
			delete pLastDlg;
			pLastDlg = nullptr;
		}
	// Okay; now using this dialog
	pCurrDlg = pToDlg;
	// fade in new dlg
	if (fFade)
	{
		if (!pToDlg->FadeIn(Game.pGUI))
		{
			delete pToDlg; pCurrDlg = nullptr;
			return nullptr;
		}
	}
	else
	{
		if (!pToDlg->Show(Game.pGUI, true))
		{
			delete pToDlg; pCurrDlg = nullptr;
			return nullptr;
		}
	}
	return pToDlg;
}

bool C4Startup::DoStartup()
{
	assert(!fInStartup);
	assert(Game.pGUI);
	// now in startup!
	fInStartup = true;
	fLastDlgWasBack = false;

	// Play some music!
	Application.MusicSystem->PlayFrontendMusic();

	// clear any previous
	delete pLastDlg; pLastDlg = nullptr;
	delete pCurrDlg; pCurrDlg = nullptr;

	// start with the last dlg that was shown - at first startup main dialog
	if (!SwitchDialog(eLastDlgID)) return false;

	// show error dlg if restart
	if (Game.fQuitWithError || GetFatalError())
	{
		Game.fQuitWithError = false;
		// preferred: Show fatal error
		const char *szErr = GetFatalError();
		if (szErr)
		{
			Game.pGUI->ShowMessage(szErr, LoadResStr("IDS_DLG_LOG"), C4GUI::Ico_Error);
		}
		else
		{
			// fallback to showing complete log
			StdStrBuf sLastLog;
			if (GetLogSection(Game.StartupLogPos, Game.QuitLogPos - Game.StartupLogPos, sLastLog))
				if (!sLastLog.isNull())
					Game.pGUI->ShowRemoveDlg(new C4GUI::InfoDialog(LoadResStr("IDS_DLG_LOG"), 10, sLastLog));
		}
		ResetFatalError();
	}

	// while state startup: keep looping
	while (fInStartup && Game.pGUI && !pCurrDlg->IsAborted())
		if (Application.HandleMessage() == HR_Failure) return false;

	// check whether startup was aborted; first checking Game.pGUI
	// (because an external call to Game.Clear() would invalidate dialogs)
	if (!Game.pGUI) return false;
	delete pLastDlg; pLastDlg = nullptr;
	if (pCurrDlg)
	{
		// deinit last shown dlg
		if (pCurrDlg->IsAborted())
		{
			// force abort flag if dlg abort done by user
			fAborted = true;
		}
		else if (pCurrDlg->IsShown())
		{
			pCurrDlg->Close(true);
		}
		delete pCurrDlg;
		pCurrDlg = nullptr;
	}

	// now no more in startup!
	fInStartup = false;

	// after startup: cleanup
	if (Game.pGUI) Game.pGUI->CloseAllDialogs(true);

	// reinit keyboard to reflect any config changes that might have been done
	// this is a good time to do it, because no GUI dialogs are opened
	if (Game.pGUI) if (!Game.InitKeyboard()) LogFatal(LoadResStr("IDS_ERR_NOKEYBOARD"));

	// all okay; return whether startup finished with a game start selection
	return !fAborted;
}

C4Startup *C4Startup::EnsureLoaded()
{
	// create and load startup data if not done yet
	assert(Game.pGUI);
	if (!pInstance)
	{
		Game.SetInitProgress(40);
		C4Startup *pStartup = new C4Startup();
		// load startup specific gfx
		if (!pStartup->Graphics.Init())
		{
			LogFatal(LoadResStr("IDS_ERR_NOGFXSYS")); delete pStartup; return nullptr;
		}
	}
	return pInstance;
}

void C4Startup::Unload()
{
	// make sure startup data is destroyed
	delete pInstance; pInstance = nullptr;
}

bool C4Startup::Execute()
{
	// ensure gfx are loaded
	C4Startup *pStartup = EnsureLoaded();
	if (!pStartup) return false;
	// exec it
	bool fResult = pStartup->DoStartup();
	return fResult;
}

bool C4Startup::SetStartScreen(const char *szScreen)
{
	// set dialog ID to be shown to specified value
	if (SEqualNoCase(szScreen, "main"))
		eLastDlgID = SDID_Main;
	if (SEqualNoCase(szScreen, "scen"))
		eLastDlgID = SDID_ScenSel;
	if (SEqualNoCase(szScreen, "netscen"))
		eLastDlgID = SDID_ScenSelNetwork;
	else if (SEqualNoCase(szScreen, "net"))
		eLastDlgID = SDID_NetJoin;
	else if (SEqualNoCase(szScreen, "options"))
		eLastDlgID = SDID_Options;
	else if (SEqualNoCase(szScreen, "plrsel"))
		eLastDlgID = SDID_PlrSel;
	else if (SEqualNoCase(szScreen, "about"))
		eLastDlgID = SDID_About;
	else return false;
	return true;
}
