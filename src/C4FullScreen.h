/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2020, The LegacyClonk Team and contributors
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
	bool Init(CStdApp *app);
#ifdef USE_X11
	bool HideCursor() const override { return true; }
	virtual void HandleMessage(XEvent &e) override;
#elif USE_SDL_MAINLOOP
	virtual void HandleMessage(SDL_Event &e) override;
#elif defined(_WIN32)
	bool Init(CStdApp *app, const char *title, const class C4Rect &bounds, CStdWindow *parent = nullptr) override;
	void Clear() override;
	void SetSize(unsigned int cx, unsigned int cy) override;
	HWND GetRenderWindow() const override { return hRenderWindow; }

protected:
	WNDCLASSEX GetWindowClass(HINSTANCE instance) const override;

private:
	HWND hRenderWindow{nullptr};
#endif

private:
};

extern C4FullScreen FullScreen;
