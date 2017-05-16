// About/credits screen

#include <C4Include.h>
#include <C4StartupAboutDlg.h>
#include <C4UpdateDlg.h>

#ifndef BIG_C4INCLUDE
#include <C4StartupMainDlg.h>
#include <C4Wrappers.h>
#endif

// C4StartupAboutDlg

C4StartupAboutDlg::C4StartupAboutDlg() : C4StartupDlg("")
{
	UpdateSize();

	// key bindings: No longer back on any key
	pKeyBack = nullptr;

	// version info in topright corner
	C4Rect rcClient = GetContainedClientRect();
	StdStrBuf sVersion; sVersion.Format(LoadResStr("IDS_DLG_VERSION"), C4VERSION);
	CStdFont &rUseFont = C4GUI::GetRes()->TextFont;
	int32_t iInfoWdt = std::min<int32_t>(rcClient.Wdt / 2, rUseFont.GetTextWidth("General info text width") * 2);
	C4GUI::ComponentAligner caInfo(C4Rect(rcClient.x + rcClient.Wdt - iInfoWdt, rcClient.y, iInfoWdt, rcClient.Hgt / 8), 0, 0, false);
	AddElement(new C4GUI::Label(sVersion.getData(), caInfo.GetGridCell(0, 1, 0, 4), ARight));

	// bottom line buttons
	C4GUI::ComponentAligner caMain(rcClient, 0, 0, true);
	C4GUI::ComponentAligner caButtons(caMain.GetFromBottom(caMain.GetHeight() * 1 / 8), 0, 0, false);
	C4GUI::CallbackButton<C4StartupAboutDlg> *btn;
	int32_t iButtonWidth = caButtons.GetInnerWidth() / 4;
	AddElement(btn = new C4GUI::CallbackButton<C4StartupAboutDlg>(LoadResStr("IDS_BTN_BACK"), caButtons.GetGridCell(0, 3, 0, 1, iButtonWidth, C4GUI_ButtonHgt, true), &C4StartupAboutDlg::OnBackBtn));
	btn->SetToolTip(LoadResStr("IDS_DLGTIP_BACKMAIN"));
	AddElement(btn = new C4GUI::CallbackButton<C4StartupAboutDlg>(LoadResStr("IDS_BTN_CHECKFORUPDATES"), caButtons.GetGridCell(2, 3, 0, 1, iButtonWidth, C4GUI_ButtonHgt, true), &C4StartupAboutDlg::OnUpdateBtn));
	btn->SetToolTip(LoadResStr("IDS_DESC_CHECKONLINEFORNEWVERSIONS"));
}

C4StartupAboutDlg::~C4StartupAboutDlg()
{
	delete pKeyBack;
}

void C4StartupAboutDlg::DoBack()
{
	C4Startup::Get()->SwitchDialog(C4Startup::SDID_Main);
}

void C4StartupAboutDlg::DrawElement(C4FacetEx &cgo)
{
	// draw background - do not use bg drawing proc, because it stretches
	// pre-clear background instead to prevent blinking borders
	if (!IsFading()) lpDDraw->FillBG();
	C4Startup::Get()->Graphics.fctAboutBG.Draw(cgo, false);
}

void C4StartupAboutDlg::MouseInput(C4GUI::CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, uint32_t dwKeyParam)
{
	// otherwise, inherited for tooltips
	C4StartupDlg::MouseInput(rMouse, iButton, iX, iY, dwKeyParam);
}

void C4StartupAboutDlg::OnUpdateBtn(C4GUI::Control *btn)
{
	C4UpdateDlg::CheckForUpdates(GetScreen());
}
