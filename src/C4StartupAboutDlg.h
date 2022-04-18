/*
 * LegacyClonk
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013-2016, The OpenClonk Team and contributors
 * Copyright (c) 2018-2020, The LegacyClonk Team and contributors
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

// Credits screen

#pragma once

#include "C4Startup.h"
#include <vector>

// startup dialog: credits
class C4StartupAboutDlg : public C4StartupDlg
{
public:
	C4StartupAboutDlg();
	~C4StartupAboutDlg() override;

protected:
	bool OnEnter() override { DoBack(); return true; }
	bool OnEscape() override { DoBack(); return true; }
	void DrawElement(C4FacetEx &cgo) override;
	bool KeyBack() { DoBack(); return true; }
	void OnBackBtn(C4GUI::Control *btn) { currentPage ? SwitchPage(currentPage - 1) : DoBack();}
	void OnAdvanceButton(C4GUI::Control *btn) { SwitchPage(1); }
	void OnUpdateBtn(C4GUI::Control *btn);

private:
	uint32_t currentPage = 0;
	C4GUI::CallbackButton<C4StartupAboutDlg> *btnAdvance;
	std::vector<std::vector<std::pair<C4GUI::TextWindow *, C4GUI::Label *>>> aboutPages;

	std::pair<C4GUI::TextWindow *, C4GUI::Label *> CreateTextWindowWithText(C4Rect &rect, const std::string &text, const std::string &title = "");
	std::pair<C4GUI::TextWindow *, C4GUI::Label *> DrawPersonList(struct PersonList &, const char *title, C4Rect &rect, uint8_t flags = 0);
	C4GUI::Label *DrawCaption(C4Rect &, const char *);
	void SwitchPage(uint32_t number);
public:

	void DoBack(); // back to main menu
	virtual bool HasBackground() override { return true; }
};
