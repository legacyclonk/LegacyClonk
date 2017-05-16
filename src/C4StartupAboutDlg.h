// About/credits screen

#pragma once

#include "C4Startup.h"

// startup dialog: About
class C4StartupAboutDlg : public C4StartupDlg
{
public:
	C4StartupAboutDlg();
	~C4StartupAboutDlg();

private:
	class C4KeyBinding *pKeyBack;

protected:
	virtual int32_t GetMarginTop() { return iDlgMarginY + C4GUI_FullscreenDlg_TitleHeight / 2; } // less top margin

	virtual void MouseInput(C4GUI::CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, uint32_t dwKeyParam); // input: back on any button
	virtual bool OnEnter() { DoBack(); return true; }
	virtual bool OnEscape() { DoBack(); return true; }
	virtual void DrawElement(C4FacetEx &cgo);
	bool KeyBack() { DoBack(); return true; }
	void OnBackBtn(C4GUI::Control *btn) { DoBack(); }
	void OnUpdateBtn(C4GUI::Control *btn);

public:
	void DoBack(); // back to main menu
};
