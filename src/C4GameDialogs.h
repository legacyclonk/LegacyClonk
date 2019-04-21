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

// main game dialogs (abort game dlg, observer dlg)

#pragma once

#include <C4Gui.h>

class C4AbortGameDialog : public C4GUI::Dialog
{
	bool fRestart = false;
	void OnYesBtn(C4GUI::Control *btn);
	void OnNoBtn(C4GUI::Control *btn);
	void OnRestartBtn(C4GUI::Control *btn);

public:
	C4AbortGameDialog();
	~C4AbortGameDialog();

protected:
	static bool is_shown;

	// callbacks to halt game
	virtual void OnShown(); // inc game halt counter
	virtual void OnClosed(bool fOK); // dec game halt counter

	virtual const char *GetID() { return "AbortGameDialog"; }

	// align by screen, not viewport
	virtual bool IsFreePlaceDialog() { return true; }

	// true for dialogs that receive full keyboard and mouse input even in shared mode
	virtual bool IsExclusiveDialog() { return true; }

	bool fGameHalted;

public:
	static bool IsShown() { return is_shown; }
};
