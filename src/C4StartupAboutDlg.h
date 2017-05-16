/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
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

// About/credits screen

#ifndef INC_C4StartupAboutDlg
#define INC_C4StartupAboutDlg

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

	virtual void MouseInput(C4GUI::CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam); // input: back on any button
	virtual bool OnEnter() { DoBack(); return true; }
	virtual bool OnEscape() { DoBack(); return true; }
	virtual void DrawElement(C4FacetEx &cgo);
	bool KeyBack() { DoBack(); return true; }
	void OnBackBtn(C4GUI::Control *btn) { DoBack(); }
	void OnUpdateBtn(C4GUI::Control *btn);

public:
	void DoBack(); // back to main menu
};

#endif // INC_C4StartupAboutDlg
