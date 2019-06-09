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

#include <C4Include.h>
#include <C4FullScreen.h>

#ifndef BIG_C4INCLUDE
#include <C4Application.h>
#include <C4UserMessages.h>
#include <C4Viewport.h>
#include <C4Language.h>
#include <C4Gui.h>
#include <C4Network2.h>
#include <C4GameDialogs.h>
#include <C4GamePadCon.h>
#include <C4Player.h>
#include <C4GameOverDlg.h>
#endif

#ifdef _WIN32
#include <windowsx.h>

LRESULT APIENTRY FullScreenWinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Process message
	switch (uMsg)
	{
	case WM_TIMER:
		if (wParam == SEC1_TIMER) { FullScreen.Sec1Timer(); }
		return true;
	case WM_DESTROY:
		Application.Quit();
		return 0;
	case WM_CLOSE:
		FullScreen.Close();
		return 0;
	case MM_MCINOTIFY:
		if (wParam == MCI_NOTIFY_SUCCESSFUL)
			Application.MusicSystem.NotifySuccess();
		return true;
	case WM_KEYUP:
		if (Game.DoKeyboardInput(wParam, KEYEV_Up, !!(lParam & 0x20000000), Application.IsControlDown(), Application.IsShiftDown(), false, nullptr))
			return 0;
		break;
	case WM_KEYDOWN:
		if (Game.DoKeyboardInput(wParam, KEYEV_Down, !!(lParam & 0x20000000), Application.IsControlDown(), Application.IsShiftDown(), !!(lParam & 0x40000000), nullptr))
			return 0;
		break;
	case WM_SYSKEYDOWN:
		if (wParam == VK_MENU) return 0; // ALT
		if (Game.DoKeyboardInput(wParam, KEYEV_Down, Application.IsAltDown(), Application.IsControlDown(), Application.IsShiftDown(), !!(lParam & 0x40000000), nullptr))
			return 0;
		if (wParam == VK_F10) return 0;
		break;
	case WM_CHAR:
	{
		char c[2];
		c[0] = (char)wParam;
		c[1] = 0;
		// GUI: forward
		if (Game.pGUI)
			if (Game.pGUI->CharIn(c))
				return 0;
		return false;
	}
	case WM_USER_LOG:
		if (SEqual2((const char *)lParam, "IDS_"))
			Log(LoadResStr((const char *)lParam));
		else
			Log((const char *)lParam);
		return false;
	case WM_LBUTTONDOWN:
		Game.GraphicsSystem.MouseMove(C4MC_Button_LeftDown, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam, nullptr);
		break;
	case WM_LBUTTONUP:     Game.GraphicsSystem.MouseMove(C4MC_Button_LeftUp,      GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam, nullptr); break;
	case WM_RBUTTONDOWN:   Game.GraphicsSystem.MouseMove(C4MC_Button_RightDown,   GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam, nullptr); break;
	case WM_RBUTTONUP:     Game.GraphicsSystem.MouseMove(C4MC_Button_RightUp,     GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam, nullptr); break;
	case WM_LBUTTONDBLCLK: Game.GraphicsSystem.MouseMove(C4MC_Button_LeftDouble,  GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam, nullptr); break;
	case WM_RBUTTONDBLCLK: Game.GraphicsSystem.MouseMove(C4MC_Button_RightDouble, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam, nullptr); break;
	case WM_MOUSEMOVE:     Game.GraphicsSystem.MouseMove(C4MC_Button_None,        GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam, nullptr); break;
	case WM_MOUSEWHEEL:
	{
		POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
		ScreenToClient(hwnd, &point);
		Game.GraphicsSystem.MouseMove(C4MC_Button_Wheel, point.x, point.y, wParam, nullptr);
		break;
	}
	// Hide cursor in client area
	case WM_SETCURSOR:
		if (LOWORD(lParam) == HTCLIENT)
		{
			SetCursor(nullptr);
		}
		else
		{
			static HCURSOR arrowCursor = nullptr;
			if (!arrowCursor) arrowCursor = LoadCursor(NULL, IDC_ARROW);
			SetCursor(arrowCursor);
		}
		break;
	case WM_SIZE:
	{
		const auto width = LOWORD(lParam);
		const auto height = HIWORD(lParam);

		if (width != 0 && height != 0)
		{
			// this might be called from C4Window::Init in which case Application.pWindow is not yet set
			if (Application.pWindow) ::SetWindowPos(Application.pWindow->hRenderWindow, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_NOZORDER);

			Application.SetResolution(width, height);
		}
	}
		break;
	case WM_ACTIVATEAPP:
		if (Config.Graphics.UseDisplayMode == DisplayMode::Fullscreen && !wParam)
		{
			ShowWindow(hwnd, SW_SHOWMINIMIZED);
		}
		return 0;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void C4FullScreen::CharIn(const char *c) { Game.pGUI->CharIn(c); }

#elif defined(USE_X11)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
void C4FullScreen::HandleMessage(XEvent &e)
{
	// Parent handling
	CStdWindow::HandleMessage(e);

	switch (e.type)
	{
	case KeyPress:
	{
		// Do not take into account the state of the various modifiers and locks
		// we don't need that for keyboard control
		uint32_t key = XkbKeycodeToKeysym(e.xany.display, e.xkey.keycode, 0, 0);
		Game.DoKeyboardInput(key, KEYEV_Down, Application.IsAltDown(), Application.IsControlDown(), Application.IsShiftDown(), false, nullptr);
		break;
	}
	case KeyRelease:
	{
		uint32_t key = XkbKeycodeToKeysym(e.xany.display, e.xkey.keycode, 0, 0);
		Game.DoKeyboardInput(key, KEYEV_Up, e.xkey.state & Mod1Mask, e.xkey.state & ControlMask, e.xkey.state & ShiftMask, false, nullptr);
		break;
	}
	case ButtonPress:
	{
		static int last_left_click, last_right_click;
		switch (e.xbutton.button)
		{
		case Button1:
			if (timeGetTime() - last_left_click < 400)
			{
				Game.GraphicsSystem.MouseMove(C4MC_Button_LeftDouble,
					e.xbutton.x, e.xbutton.y, e.xbutton.state, nullptr);
				last_left_click = 0;
			}
			else
			{
				Game.GraphicsSystem.MouseMove(C4MC_Button_LeftDown,
					e.xbutton.x, e.xbutton.y, e.xbutton.state, nullptr);
				last_left_click = timeGetTime();
			}
			break;
		case Button2:
			Game.GraphicsSystem.MouseMove(C4MC_Button_MiddleDown,
				e.xbutton.x, e.xbutton.y, e.xbutton.state, nullptr);
			break;
		case Button3:
			if (timeGetTime() - last_right_click < 400)
			{
				Game.GraphicsSystem.MouseMove(C4MC_Button_RightDouble,
					e.xbutton.x, e.xbutton.y, e.xbutton.state, nullptr);
				last_right_click = 0;
			}
			else
			{
				Game.GraphicsSystem.MouseMove(C4MC_Button_RightDown,
					e.xbutton.x, e.xbutton.y, e.xbutton.state, nullptr);
				last_right_click = timeGetTime();
			}
			break;
		case Button4:
			Game.GraphicsSystem.MouseMove(C4MC_Button_Wheel,
				e.xbutton.x, e.xbutton.y, e.xbutton.state + (short(32) << 16), nullptr);
			break;
		case Button5:
			Game.GraphicsSystem.MouseMove(C4MC_Button_Wheel,
				e.xbutton.x, e.xbutton.y, e.xbutton.state + (short(-32) << 16), nullptr);
			break;
		default:
			break;
		}
	}
	break;
	case ButtonRelease:
		switch (e.xbutton.button)
		{
		case Button1:
			Game.GraphicsSystem.MouseMove(C4MC_Button_LeftUp, e.xbutton.x, e.xbutton.y, e.xbutton.state, nullptr);
			break;
		case Button2:
			Game.GraphicsSystem.MouseMove(C4MC_Button_MiddleUp, e.xbutton.x, e.xbutton.y, e.xbutton.state, nullptr);
			break;
		case Button3:
			Game.GraphicsSystem.MouseMove(C4MC_Button_RightUp, e.xbutton.x, e.xbutton.y, e.xbutton.state, nullptr);
			break;
		default:
			break;
		}
		break;
	case MotionNotify:
		Game.GraphicsSystem.MouseMove(C4MC_Button_None, e.xbutton.x, e.xbutton.y, e.xbutton.state, nullptr);
		break;
	case FocusIn:
		Application.Active = true;
		break;
	case FocusOut: case UnmapNotify:
		Application.Active = false;
		break;
	case ConfigureNotify:
		Application.SetResolution(e.xconfigure.width, e.xconfigure.height);
		break;
	}
}

#elif defined(USE_SDL_MAINLOOP)
// SDL version

namespace
{
	void sdlToC4MCBtn(const SDL_MouseButtonEvent &e,
		int32_t &button, uint32_t &flags)
	{
		static int lastLeftClick = 0, lastRightClick = 0;

		button = C4MC_Button_None;
		flags = 0;

		switch (e.button)
		{
		case SDL_BUTTON_LEFT:
			if (e.state == SDL_PRESSED)
				if (timeGetTime() - lastLeftClick < 400)
				{
					lastLeftClick = 0;
					button = C4MC_Button_LeftDouble;
				}
				else
				{
					lastLeftClick = timeGetTime();
					button = C4MC_Button_LeftDown;
				}
			else
				button = C4MC_Button_LeftUp;
			break;
		case SDL_BUTTON_RIGHT:
			if (e.state == SDL_PRESSED)
				if (timeGetTime() - lastRightClick < 400)
				{
					lastRightClick = 0;
					button = C4MC_Button_RightDouble;
				}
				else
				{
					lastRightClick = timeGetTime();
					button = C4MC_Button_RightDown;
				}
			else
				button = C4MC_Button_RightUp;
			break;
		case SDL_BUTTON_MIDDLE:
			if (e.state == SDL_PRESSED)
				button = C4MC_Button_MiddleDown;
			else
				button = C4MC_Button_MiddleUp;
			break;
		case SDL_BUTTON_WHEELUP:
			button = C4MC_Button_Wheel;
			flags = (+32) << 16;
			break;
		case SDL_BUTTON_WHEELDOWN:
			button = C4MC_Button_Wheel;
			flags = (-32) << 16;
			break;
		}
	}

	bool isSpecialKey(unsigned unicode)
	{
		if (unicode >= 0xe00)
			return true;
		if (unicode < 32 || unicode == 127)
			return true;
		return false;
	}
}

#include "StdGL.h"

void C4FullScreen::HandleMessage(SDL_Event &e)
{
	switch (e.type)
	{
	case SDL_KEYDOWN:
	{
		// Only forward real characters to UI. (Nothing outside of "private use" range.)
		// This works without iconv for some reason. Yay!
		// FIXME: convert to UTF-8
		char c[2];
		c[0] = e.key.keysym.unicode;
		c[1] = 0;
		if (Game.pGUI && !isSpecialKey(e.key.keysym.unicode))
			Game.pGUI->CharIn(c);
		Game.DoKeyboardInput(e.key.keysym.sym, KEYEV_Down,
			e.key.keysym.mod & (KMOD_LALT | KMOD_RALT),
			e.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL),
			e.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT),
			false, nullptr);
		break;
	}
	case SDL_KEYUP:
		Game.DoKeyboardInput(e.key.keysym.sym, KEYEV_Up,
			e.key.keysym.mod & (KMOD_LALT | KMOD_RALT),
			e.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL),
			e.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT), false, nullptr);
		break;
	case SDL_MOUSEMOTION:
		Game.GraphicsSystem.MouseMove(C4MC_Button_None, e.motion.x, e.motion.y, 0, nullptr);
		break;
	case SDL_MOUSEBUTTONUP:
	case SDL_MOUSEBUTTONDOWN:
		int32_t button;
		uint32_t flags;
		sdlToC4MCBtn(e.button, button, flags);
		Game.GraphicsSystem.MouseMove(button, e.button.x, e.button.y, flags, nullptr);
		break;
	case SDL_JOYAXISMOTION:
	case SDL_JOYHATMOTION:
	case SDL_JOYBALLMOTION:
	case SDL_JOYBUTTONDOWN:
	case SDL_JOYBUTTONUP:
		Application.pGamePadControl->FeedEvent(e);
		break;
	case SDL_VIDEORESIZE:
		Application.SetResolution(e.resize.w, e.resize.h);
		break;
	}
}

#endif // _WIN32, USE_X11, USE_SDL_MAINLOOP

#ifndef _WIN32
void C4FullScreen::CharIn(const char *c)
{
	if (Game.pGUI)
	{
		StdStrBuf c2; c2.Take(Languages.IconvClonk(c));
		Game.pGUI->CharIn(c2.getData());
	}
}
#endif

C4FullScreen::C4FullScreen()
{
	pMenu = nullptr;
}

C4FullScreen::~C4FullScreen()
{
	delete pMenu;
}

void C4FullScreen::Close()
{
	if (Game.IsRunning)
		ShowAbortDlg();
	else
		Application.Quit();
}

void C4FullScreen::Execute()
{
	// Execute menu
	if (pMenu) pMenu->Execute();
	// Draw
	Game.GraphicsSystem.Execute();
}

bool C4FullScreen::ViewportCheck()
{
	int iPlrNum; C4Player *pPlr;
	// Not active
	if (!Active) return false;
	// Determine film mode
	bool fFilm = (Game.C4S.Head.Replay && Game.C4S.Head.Film);
	// Check viewports
	switch (Game.GraphicsSystem.GetViewportCount())
	{
	// No viewports: create no-owner viewport
	case 0:
		iPlrNum = NO_OWNER;
		// Film mode: create viewport for first player (instead of no-owner)
		if (fFilm)
			if (pPlr = Game.Players.First)
				iPlrNum = pPlr->Number;
		// Create viewport
		Game.CreateViewport(iPlrNum, iPlrNum == NO_OWNER);
		// Non-film (observer mode)
		if (!fFilm)
		{
			// Activate mouse control
			Game.MouseControl.Init(iPlrNum);
			// Display message for how to open observer menu (this message will be cleared if any owned viewport opens)
			StdStrBuf sKey;
			sKey.Format("<c ffff00><%s></c>", Game.KeyboardInput.GetKeyCodeNameByKeyName("FullscreenMenuOpen", false).getData());
			Game.GraphicsSystem.FlashMessage(FormatString(LoadResStr("IDS_MSG_PRESSORPUSHANYGAMEPADBUTT"), sKey.getData()).getData());
		}
		break;
	// One viewport: do nothing
	case 1:
		break;
	// More than one viewport: remove all no-owner viewports
	default:
		Game.GraphicsSystem.CloseViewport(NO_OWNER, true);
		break;
	}
	// Look for no-owner viewport
	C4Viewport *pNoOwnerVp = Game.GraphicsSystem.GetViewport(NO_OWNER);
	// No no-owner viewport found
	if (!pNoOwnerVp)
	{
		// Close any open fullscreen menu
		CloseMenu();
	}
	// No-owner viewport present
	else
	{
		// movie mode: player present, and no valid viewport assigned?
		if (Game.C4S.Head.Replay && Game.C4S.Head.Film && (pPlr = Game.Players.First))
			// assign viewport to joined player
			pNoOwnerVp->Init(pPlr->Number, true);
	}
	// Done
	return true;
}

bool C4FullScreen::ShowAbortDlg()
{
	// no gui?
	if (!Game.pGUI) return false;
	// abort dialog already shown
	if (C4AbortGameDialog::IsShown()) return false;
	// not while game over dialog is open
	if (C4GameOverDlg::IsShown()) return false;
	// show abort dialog
	return Game.pGUI->ShowRemoveDlg(new C4AbortGameDialog());
}

bool C4FullScreen::ActivateMenuMain()
{
	// Not during game over dialog
	if (C4GameOverDlg::IsShown()) return false;
	// Close previous
	CloseMenu();
	// Open menu
	pMenu = new C4MainMenu();
	return pMenu->ActivateMain(NO_OWNER);
}

void C4FullScreen::CloseMenu()
{
	if (pMenu && pMenu->IsActive()) pMenu->Close(false);
	delete pMenu;
	pMenu = nullptr;
}

bool C4FullScreen::MenuKeyControl(uint8_t byCom)
{
	if (pMenu) return pMenu->KeyControl(byCom);
	return false;
}
