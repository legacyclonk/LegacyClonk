/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
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

/* Main class to execute the game fullscreen mode */

#pragma once

#include "C4MainMenu.h"
#include <StdWindow.h>

#include "C4Game.h"

class C4FullScreen : public CStdWindow
{
public:
	C4MainMenu *pMenu;

public:
	C4FullScreen();
	~C4FullScreen();
	void Execute();
	bool ViewportCheck();
	void Sec1Timer() override { Game.Sec1Timer(); Application.DoSec1Timers(); }
	bool ShowAbortDlg(); // show game abort dialog (Escape pressed)
	bool ActivateMenuMain();
	void CloseMenu();
	bool MenuKeyControl(uint8_t byCom); // direct keyboard callback
	// User requests close
	virtual void Close() override;
	virtual void CharIn(const char *c) override;
#ifdef USE_X11
	virtual void HandleMessage(XEvent &e) override;
#elif USE_SDL_MAINLOOP
	virtual void HandleMessage(SDL_Event &e) override;
#endif
};

extern C4FullScreen FullScreen;
