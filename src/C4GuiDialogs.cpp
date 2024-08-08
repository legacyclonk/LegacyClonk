/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
 * Copyright (c) 2017-2021, The LegacyClonk Team and contributors
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

// generic user interface
// dialog base classes and some user dialogs

#include "C4GuiDialogs.h"
#include "C4GuiEdit.h"
#include "C4GuiResource.h"
#include <C4Gui.h>

#include <C4FullScreen.h>
#include <C4LoaderScreen.h>
#include <C4Application.h>
#include <C4Viewport.h>
#include <C4Console.h>
#include <C4Def.h>
#include <C4Wrappers.h>

#include <StdGL.h>

#include <format>

#ifdef _WIN32
#include "StdRegistry.h"
#include "res/engine_resource.h"
#endif

namespace C4GUI
{
namespace
{
	inline Button *newDlgCloseButton(const C4Rect &bounds) { return new CloseButton{LoadResStr(C4ResStrTableKey::IDS_DLG_CLOSE), bounds, true}; }
	inline Button *newRetryButton(const C4Rect &bounds) { return new CloseButton{LoadResStr(C4ResStrTableKey::IDS_BTN_RETRY), bounds, true}; }
}

// EM window class
class DialogWindow : public CStdWindow
{
#ifdef _WIN32
private:
	static constexpr auto WindowStyle = WS_VISIBLE | WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX;
#endif
public:
	DialogWindow(Dialog &dialog) : dialog{dialog} {}
	virtual void Close() override;

#ifdef _WIN32
	bool Init(CStdApp *app, const char *title, const class C4Rect &bounds, CStdWindow *parent = nullptr) override;

	std::pair<DWORD, DWORD> GetWindowStyle() const override { return {WindowStyle, 0}; }
	WNDCLASSEX GetWindowClass(HINSTANCE instance) const override;
	bool GetPositionData(std::string &id, std::string &subKey, bool &storeSize) const override;
#endif

private:
	Dialog &dialog;

#ifdef _WIN32
	friend LRESULT APIENTRY DialogWinProc(HWND, UINT, WPARAM, LPARAM);
#endif
};

// FrameDecoration

void FrameDecoration::Clear()
{
	idSourceDef = 0;
	dwBackClr = C4GUI_StandardBGColor;
	iBorderTop = iBorderLeft = iBorderRight = iBorderBottom = 0;
	fHasGfxOutsideClientArea = false;
	fctTop.Default();
	fctTopRight.Default();
	fctRight.Default();
	fctBottomRight.Default();
	fctBottom.Default();
	fctBottomLeft.Default();
	fctLeft.Default();
	fctTopLeft.Default();
}

bool FrameDecoration::SetFacetByAction(C4Def *pOfDef, C4FacetEx &rfctTarget, const char *szFacetName)
{
	// get action
	const std::string actName{std::format("FrameDeco{}", szFacetName)};
	int cnt; C4ActionDef *pAct = pOfDef->ActMap;
	for (cnt = pOfDef->ActNum; cnt; --cnt, ++pAct)
		if (actName == pAct->Name)
			break;
	if (!cnt) return false;
	// set facet by it
	rfctTarget.Set(pOfDef->Graphics.GetBitmap(), pAct->Facet.x, pAct->Facet.y, pAct->Facet.Wdt, pAct->Facet.Hgt, pAct->Facet.tx, pAct->Facet.ty);
	return true;
}

bool FrameDecoration::SetByDef(C4Section &section, C4ID idSourceDef)
{
	// get source def
	C4Def *pSrcDef = Game.Defs.ID2Def(idSourceDef);
	if (!pSrcDef) return false;
	// script compiled?
	if (!pSrcDef->Script.IsReady()) return false;
	// reset old
	Clear();
	this->idSourceDef = idSourceDef;
	// query values
	dwBackClr = pSrcDef->Script.Call(section, std::format(PSF_FrameDecoration, "BackClr").c_str()).getInt();
	iBorderTop = pSrcDef->Script.Call(section, std::format(PSF_FrameDecoration, "BorderTop").c_str()).getInt();
	iBorderLeft = pSrcDef->Script.Call(section, std::format(PSF_FrameDecoration, "BorderLeft").c_str()).getInt();
	iBorderRight = pSrcDef->Script.Call(section, std::format(PSF_FrameDecoration, "BorderRight").c_str()).getInt();
	iBorderBottom = pSrcDef->Script.Call(section, std::format(PSF_FrameDecoration, "BorderBottom").c_str()).getInt();
	// get gfx
	SetFacetByAction(pSrcDef, fctTop, "Top");
	SetFacetByAction(pSrcDef, fctTopRight, "TopRight");
	SetFacetByAction(pSrcDef, fctRight, "Right");
	SetFacetByAction(pSrcDef, fctBottomRight, "BottomRight");
	SetFacetByAction(pSrcDef, fctBottom, "Bottom");
	SetFacetByAction(pSrcDef, fctBottomLeft, "BottomLeft");
	SetFacetByAction(pSrcDef, fctLeft, "Left");
	SetFacetByAction(pSrcDef, fctTopLeft, "TopLeft");
	// check for gfx outside main area
	fHasGfxOutsideClientArea = (fctTopLeft.TargetY < 0) || (fctTop.TargetY < 0) || (fctTopRight.TargetY < 0)
		|| (fctTopLeft.TargetX < 0) || (fctLeft.TargetX < 0) || (fctBottomLeft.TargetX < 0)
		|| (fctTopRight.TargetX + fctTopRight.Wdt > iBorderRight) || (fctRight.TargetX + fctRight.Wdt > iBorderRight) || (fctBottomRight.TargetX + fctBottomRight.Wdt > iBorderRight)
		|| (fctBottomLeft.TargetY + fctBottomLeft.Hgt > iBorderBottom) || (fctBottom.TargetY + fctBottom.Hgt > iBorderBottom) || (fctBottomRight.TargetY + fctBottomRight.Hgt > iBorderBottom);
	// k, done
	return true;
}

bool FrameDecoration::UpdateGfx(C4Section &section)
{
	// simply re-set by def
	return SetByDef(section, idSourceDef);
}

void FrameDecoration::Draw(C4FacetEx &cgo, C4Rect &rcBounds)
{
	int ox = cgo.TargetX + rcBounds.x, oy = cgo.TargetY + rcBounds.y;

	// draw BG
	lpDDraw->DrawBoxDw(cgo.Surface, ox, oy, ox + rcBounds.Wdt - 1, oy + rcBounds.Hgt - 1, dwBackClr);

	const auto drawHorizontal = [this, &cgo, &rcBounds, ox](C4FacetEx facet, std::int32_t y)
	{
		if (facet.Wdt <= 0)
		{
			return;
		}

		for (std::int32_t x = iBorderLeft; x < rcBounds.Wdt - iBorderRight; x += facet.Wdt)
		{
			int w = std::min<int>(facet.Wdt, rcBounds.Wdt - iBorderRight - x);
			facet.Wdt = w;
			facet.Draw(cgo.Surface, ox + x, y + facet.TargetY);
		}
	};
	const auto drawVertical = [this, &cgo, &rcBounds, oy](C4FacetEx facet, std::int32_t x)
	{
		if (facet.Hgt <= 0)
		{
			return;
		}

		for (std::int32_t y = iBorderTop; y < rcBounds.Hgt - iBorderBottom; y += facet.Hgt)
		{
			int h = std::min<int>(facet.Hgt, rcBounds.Hgt - iBorderBottom - y);
			facet.Hgt = h;
			facet.Draw(cgo.Surface, x + facet.TargetX, oy + y);
		}
	};

	// draw borders
	drawHorizontal(fctTop, oy);
	drawVertical(fctLeft, ox);
	drawVertical(fctRight, ox + rcBounds.Wdt - iBorderRight);
	drawHorizontal(fctBottom, oy + rcBounds.Hgt - iBorderBottom);

	// draw edges
	fctTopLeft.Draw(cgo.Surface, ox + fctTopLeft.TargetX, oy + fctTopLeft.TargetY);
	fctTopRight.Draw(cgo.Surface, ox + rcBounds.Wdt - iBorderRight + fctTopRight.TargetX, oy + fctTopRight.TargetY);
	fctBottomLeft.Draw(cgo.Surface, ox + fctBottomLeft.TargetX, oy + rcBounds.Hgt - iBorderBottom + fctBottomLeft.TargetY);
	fctBottomRight.Draw(cgo.Surface, ox + rcBounds.Wdt - iBorderRight + fctBottomRight.TargetX, oy + rcBounds.Hgt - iBorderBottom + fctBottomRight.TargetY);
}

// DialogWindow

#ifdef _WIN32

bool DialogWindow::Init(CStdApp *const app, const char *const title, const C4Rect &bounds, CStdWindow *const parent)
{
	// calculate required size
	RECT size{0, 0, bounds.Wdt, bounds.Hgt};
	if (!AdjustWindowRectEx(&size, WindowStyle, false, 0))
	{
		return false;
	}

	return CStdWindow::Init(app, title, {bounds.x, bounds.y, size.right - size.left, size.bottom - size.top}, parent);
}

// Dialog

LRESULT APIENTRY DialogWinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg != WM_NCCREATE)
	{
		Dialog *const dialog{&reinterpret_cast<DialogWindow *>(GetWindowLongPtr(hwnd, GWLP_USERDATA))->dialog};
		// Process message
		switch (uMsg)
		{
		case WM_KEYDOWN:
			if (Game.DoKeyboardInput(wParam, KEYEV_Down, !!(lParam & 0x20000000), Application.IsControlDown(), Application.IsShiftDown(), !!(lParam & 0x40000000), dialog)) return 0;
			break;

		case WM_KEYUP:
			if (Game.DoKeyboardInput(wParam, KEYEV_Up, !!(lParam & 0x20000000), Application.IsControlDown(), Application.IsShiftDown(), false, dialog)) return 0;
			break;

		case WM_SYSKEYDOWN:
			if (wParam == 18) break;
			if (Game.DoKeyboardInput(wParam, KEYEV_Down, !!(lParam & 0x20000000), Application.IsControlDown(), Application.IsShiftDown(), !!(lParam & 0x40000000), dialog)) return 0;
			break;

		case WM_CLOSE:
			dialog->Close(false);
			break;

		case WM_PAINT:
			// 2do: only draw specific dlg?
			break;
			return 0;

		case WM_LBUTTONDOWN:   Game.pGUI->MouseInput(C4MC_Button_LeftDown,    LOWORD(lParam), HIWORD(lParam), wParam, dialog, nullptr); break;
		case WM_LBUTTONUP:     Game.pGUI->MouseInput(C4MC_Button_LeftUp,      LOWORD(lParam), HIWORD(lParam), wParam, dialog, nullptr); break;
		case WM_RBUTTONDOWN:   Game.pGUI->MouseInput(C4MC_Button_RightDown,   LOWORD(lParam), HIWORD(lParam), wParam, dialog, nullptr); break;
		case WM_RBUTTONUP:     Game.pGUI->MouseInput(C4MC_Button_RightUp,     LOWORD(lParam), HIWORD(lParam), wParam, dialog, nullptr); break;
		case WM_LBUTTONDBLCLK: Game.pGUI->MouseInput(C4MC_Button_LeftDouble,  LOWORD(lParam), HIWORD(lParam), wParam, dialog, nullptr); break;
		case WM_RBUTTONDBLCLK: Game.pGUI->MouseInput(C4MC_Button_RightDouble, LOWORD(lParam), HIWORD(lParam), wParam, dialog, nullptr); break;

		case WM_MOUSEMOVE:
			Game.pGUI->MouseInput(C4MC_Button_None, LOWORD(lParam), HIWORD(lParam), wParam, dialog, nullptr);
			break;

		case WM_MOUSEWHEEL:
			Game.pGUI->MouseInput(C4MC_Button_Wheel, LOWORD(lParam), HIWORD(lParam), wParam, dialog, nullptr);
			break;
		}
	}

	return CStdWindow::DefaultWindowProc(hwnd, uMsg, wParam, lParam);
}

WNDCLASSEX DialogWindow::GetWindowClass(const HINSTANCE instance) const
{
	return {
		.cbSize = sizeof(WNDCLASSEX),
		.style = CS_DBLCLKS | CS_BYTEALIGNCLIENT,
		.lpfnWndProc = &DialogWinProc,
		.cbClsExtra = 0,
		.cbWndExtra = 0,
		.hInstance = instance,
		.hIcon = LoadIcon(instance, MAKEINTRESOURCE(IDI_00_C4X)),
		.hCursor = LoadCursor(nullptr, IDC_ARROW),
		.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BACKGROUND),
		.lpszMenuName = nullptr,
		.lpszClassName = L"C4GUIdlg", // keep for backwards compatibility
		.hIconSm = LoadIcon(instance, MAKEINTRESOURCE(IDI_00_C4X))
	};
}

bool DialogWindow::GetPositionData(std::string &id, std::string &subKey, bool &storeSize) const
{
	if (const char *const dialogID{dialog.GetID()}; dialogID && *dialogID)
	{
		id = std::string{"ConsoleGUI_"} + dialogID;
		subKey = Config.GetSubkeyPath("Console");
		storeSize = true;
		return true;
	}

	return false;
}

#endif // _WIN32

void DialogWindow::Close()
{
	// FIXME: Close the dialog of this window
}

bool Dialog::CreateConsoleWindow()
{
	// already created?
	if (pWindow) return true;
	// create it!
	pWindow = new DialogWindow(*this);
	if (!pWindow->Init(&Application, TitleString.isNull() ? "???" : TitleString.getData(), rcBounds, &Console))
	{
		delete pWindow;
		pWindow = nullptr;
		return false;
	}
	// create rendering context
	if (lpDDraw) pCtx = lpDDraw->CreateContext(pWindow, &Application);
	return true;
}

void Dialog::DestroyConsoleWindow()
{
	if (pWindow) pWindow->Clear();
	delete pWindow; pWindow = nullptr;
	delete pCtx;    pCtx    = nullptr;
}

Dialog::Dialog(int32_t iWdt, int32_t iHgt, const char *szTitle, bool fViewportDlg) :
	Window(), pTitle(nullptr), pCloseBtn(nullptr), fDelOnClose(false), fViewportDlg(fViewportDlg), pWindow(nullptr), pCtx(nullptr), pFrameDeco(nullptr)
{
	// zero fields
	pActiveCtrl = nullptr;
	fShow = fOK = false;
	iFade = 100; eFade = eFadeNone;
	// add title
	rcBounds.Wdt = iWdt;
	SetTitle(szTitle);
	// set size - calcs client rect as well
	SetBounds(C4Rect(0, 0, iWdt, iHgt));
	// create key callbacks
	C4CustomKey::CodeList Keys;
	Keys.push_back(C4KeyCodeEx(K_TAB));
	if (Config.Controls.GamepadGuiControl)
	{
		Keys.push_back(C4KeyCodeEx(KEY_Gamepad(0, KEY_JOY_Right)));
	}
	pKeyAdvanceControl = new C4KeyBinding(Keys, "GUIAdvanceFocus", KEYSCOPE_Gui,
		new DlgKeyCBEx<Dialog, bool>(*this, false, &Dialog::KeyAdvanceFocus), C4CustomKey::PRIO_Dlg);
	Keys.clear();
	Keys.push_back(C4KeyCodeEx(K_TAB, KEYS_Shift));
	if (Config.Controls.GamepadGuiControl)
	{
		Keys.push_back(C4KeyCodeEx(KEY_Gamepad(0, KEY_JOY_Left)));
	}
	pKeyAdvanceControlB = new C4KeyBinding(Keys, "GUIAdvanceFocusBack", KEYSCOPE_Gui,
		new DlgKeyCBEx<Dialog, bool>(*this, true, &Dialog::KeyAdvanceFocus), C4CustomKey::PRIO_Dlg);
	Keys.clear();
	Keys.push_back(C4KeyCodeEx(KEY_Any, KEYS_Alt));
	Keys.push_back(C4KeyCodeEx(KEY_Any, C4KeyShiftState(KEYS_Alt | KEYS_Shift)));
	pKeyHotkey = new C4KeyBinding(Keys, "GUIHotkey", KEYSCOPE_Gui,
		new DlgKeyCBPassKey<Dialog>(*this, &Dialog::KeyHotkey), C4CustomKey::PRIO_Ctrl);
	Keys.clear();
	Keys.push_back(C4KeyCodeEx(K_RETURN));
	if (Config.Controls.GamepadGuiControl)
	{
		Keys.push_back(C4KeyCodeEx(KEY_Gamepad(0, KEY_JOY_AnyLowButton)));
	}
	pKeyEnter = new C4KeyBinding(Keys, "GUIDialogOkay", KEYSCOPE_Gui,
		new DlgKeyCB<Dialog>(*this, &Dialog::KeyEnter), C4CustomKey::PRIO_Dlg);
	Keys.clear();
	Keys.push_back(C4KeyCodeEx(K_ESCAPE));
	if (Config.Controls.GamepadGuiControl)
	{
		Keys.push_back(C4KeyCodeEx(KEY_Gamepad(0, KEY_JOY_AnyHighButton)));
	}
	pKeyEscape = new C4KeyBinding(Keys, "GUIDialogAbort", KEYSCOPE_Gui,
		new DlgKeyCB<Dialog>(*this, &Dialog::KeyEscape), C4CustomKey::PRIO_Dlg);
	Keys.clear();
	Keys.push_back(C4KeyCodeEx(KEY_Any));
	Keys.push_back(C4KeyCodeEx(KEY_Any, KEYS_Shift));
	pKeyFocusDefControl = new C4KeyBinding(Keys, "GUIFocusDefault", KEYSCOPE_Gui,
		new DlgKeyCB<Dialog>(*this, &Dialog::KeyFocusDefault), C4CustomKey::PRIO_Dlg);
}

void Dialog::SetTitle(const char *szTitle, bool fShowCloseButton)
{
	// always keep local copy of title
	TitleString.Copy(szTitle);
	// console mode dialogs: Use window bar
	if (!Application.isFullScreen && !IsViewportDialog())
	{
		if (pWindow) pWindow->SetTitle(szTitle ? szTitle : "");
		return;
	}
	// set new
	if (szTitle && *szTitle)
	{
		int32_t iTextHgt = WoodenLabel::GetDefaultHeight(&GetRes()->TextFont);
		if (pTitle)
		{
			pTitle->GetBounds() = C4Rect(-GetMarginLeft(), -iTextHgt, rcBounds.Wdt, iTextHgt);
			// noupdate if title is same - this is necessary to prevent scrolling reset when refilling internal menus
			if (SEqual(pTitle->GetText(), szTitle)) return;
			pTitle->SetText(szTitle);
		}
		else
			AddElement(pTitle = new WoodenLabel(szTitle, C4Rect(-GetMarginLeft(), -iTextHgt, rcBounds.Wdt, iTextHgt), C4GUI_CaptionFontClr, &GetRes()->TextFont, ALeft, false));
		pTitle->SetToolTip(szTitle);
		pTitle->SetDragTarget(this);
		pTitle->SetAutoScrollTime(C4GUI_TitleAutoScrollTime);
		if (fShowCloseButton)
		{
			pTitle->SetRightIndent(20); // for close button
			if (!pCloseBtn)
			{
				AddElement(pCloseBtn = new CallbackButton<Dialog, IconButton>(Ico_Close, pTitle->GetToprightCornerRect(16, 16, 4, 4, 0), 0, &Dialog::OnUserClose));
				pCloseBtn->SetToolTip(LoadResStr(C4ResStrTableKey::IDS_MNU_CLOSE));
			}
			else
				pCloseBtn->GetBounds() = pTitle->GetToprightCornerRect(16, 16, 4, 4, 0);
		}
	}
	else
	{
		delete pTitle;    pTitle    = nullptr;
		delete pCloseBtn; pCloseBtn = nullptr;
	}
}

Dialog::~Dialog()
{
	// kill key bindings
	delete pKeyAdvanceControl;
	delete pKeyAdvanceControlB;
	delete pKeyHotkey;
	delete pKeyEscape;
	delete pKeyEnter;
	delete pKeyFocusDefControl;
	// clear window
	DestroyConsoleWindow();
	// avoid endless delete/close-recursion
	fDelOnClose = false;
	// free deco
	if (pFrameDeco) pFrameDeco->Deref();
}

void Dialog::UpdateSize()
{
	// update title bar position
	if (pTitle)
	{
		int32_t iTextHgt = WoodenLabel::GetDefaultHeight(&GetRes()->TextFont);
		pTitle->SetBounds(C4Rect(-GetMarginLeft(), -iTextHgt, rcBounds.Wdt, iTextHgt));
		if (pCloseBtn) pCloseBtn->SetBounds(pTitle->GetToprightCornerRect(16, 16, 4, 4, 0));
	}
	// inherited
	Window::UpdateSize();
	// update assigned window
	if (pWindow)
	{
		auto wdt = rcBounds.Wdt, hgt = rcBounds.Hgt;
#ifdef _WIN32
		RECT rect{0, 0, wdt, hgt};
		if (::AdjustWindowRectEx(&rect, ConsoleDlgWindowStyle, false, 0))
		{
			wdt = rect.right - rect.left;
			hgt = rect.bottom - rect.top;
		}
#endif
		pWindow->SetSize(wdt, hgt);
	}
}

void Dialog::RemoveElement(Element *pChild)
{
	// inherited
	Window::RemoveElement(pChild);
	// clear ptr
	if (pChild == pActiveCtrl) pActiveCtrl = nullptr;
}

void Dialog::Draw(C4FacetEx &cgo)
{
#ifndef USE_CONSOLE
	// select rendering context
	if (pCtx) if (!pCtx->Select()) return;
#endif
	Screen *pScreen;
	// evaluate fading
	switch (eFade)
	{
	case eFadeNone: break; // no fading
	case eFadeIn:
		// fade in
		if ((iFade += 10) >= 100)
		{
			if (pScreen = GetScreen())
			{
				if (pScreen->GetTopDialog() == this)
					pScreen->ActivateDialog(this);
			}
			eFade = eFadeNone;
		}
		break;
	case eFadeOut:
		// fade out
		if ((iFade -= 10) <= 0)
		{
			fVisible = fShow = false;
			if (pScreen = GetScreen())
				pScreen->RecheckActiveDialog();
			eFade = eFadeNone;
		}
	}
	// set fade
	if (iFade < 100)
	{
		if (iFade <= 0) return;
		lpDDraw->ActivateBlitModulation(((100 - iFade) * 255 / 100) << 24 | 0xffffff);
	}
	// separate window: Clear background
	if (pWindow)
		lpDDraw->DrawBoxDw(cgo.Surface, rcBounds.x, rcBounds.y, rcBounds.Wdt - 1, rcBounds.Hgt - 1, C4GUI_StandardBGColor & 0xffffff);
	// draw window + contents (evaluates IsVisible)
	Window::Draw(cgo);
	// reset blit modulation
	if (iFade < 100) lpDDraw->DeactivateBlitModulation();
	// blit output to own window
	if (pWindow) Application.DDraw->PageFlip();
#ifndef USE_CONSOLE
	// switch back to original context
	if (pCtx) pGL->GetMainCtx().Select();
#endif
}

void Dialog::DrawElement(C4FacetEx &cgo)
{
	// custom border?
	if (pFrameDeco)
		pFrameDeco->Draw(cgo, rcBounds);
	else
	{
		// standard border/bg then
		// draw background
		lpDDraw->DrawBoxDw(cgo.Surface, cgo.TargetX + rcBounds.x, cgo.TargetY + rcBounds.y, rcBounds.x + rcBounds.Wdt - 1 + cgo.TargetX, rcBounds.y + rcBounds.Hgt - 1 + cgo.TargetY, C4GUI_StandardBGColor);
		// draw frame
		Draw3DFrame(cgo);
	}
}

bool Dialog::CharIn(const char *c)
{
	// reroute to active control
	if (pActiveCtrl && pActiveCtrl->CharIn(c)) return true;
	// unprocessed: Focus default control
	// Except for space, which may have been processed as a key already
	// (changing focus here would render buttons unusable, because they switch on KeyUp)
	Control *pDefCtrl = GetDefaultControl();
	if (pDefCtrl && pDefCtrl != pActiveCtrl && (!c || *c != 0x20))
	{
		SetFocus(pDefCtrl, false);
		if (pActiveCtrl && pActiveCtrl->CharIn(c))
			return true;
	}
	return false;
}

bool Dialog::KeyHotkey(C4KeyCodeEx key)
{
#ifdef USE_SDL_MAINLOOP
	const auto wKey = SDL_GetKeyName(SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(key.Key)))[0];
#else
	uint16_t wKey = uint16_t(key.Key);
#endif

	// do hotkey procs for standard alphanumerics only
	if (Inside<uint16_t>(TOUPPERIFX11(wKey), 'A', 'Z')) if (OnHotkey(char(TOUPPERIFX11(wKey)))) return true;
	if (Inside<uint16_t>(TOUPPERIFX11(wKey), '0', '9')) if (OnHotkey(char(TOUPPERIFX11(wKey)))) return true;
	return false;
}

bool Dialog::KeyFocusDefault()
{
	// unprocessed key: Focus default control
	Control *pDefCtrl = GetDefaultControl();
	if (pDefCtrl && pDefCtrl != pActiveCtrl)
		SetFocus(pDefCtrl, false);
	// never mark this as processed, so a later char message to the control may be sent (for deselected chat)
	return false;
}

void Dialog::MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, uint32_t dwKeyParam)
{
	// inherited will do...
	Window::MouseInput(rMouse, iButton, iX, iY, dwKeyParam);
}

void Dialog::SetFocus(Control *pCtrl, bool fByMouse)
{
	// no change?
	if (pCtrl == pActiveCtrl) return;
	// leave old focus
	if (pActiveCtrl)
	{
		Control *pC = pActiveCtrl;
		pActiveCtrl = nullptr;
		pC->OnLooseFocus();
		// if leaving the old focus set a new one, abort here because it looks like the control didn't want to lose focus
		if (pActiveCtrl) return;
	}
	// set new
	if (pActiveCtrl = pCtrl) pCtrl->OnGetFocus(fByMouse);
}

void Dialog::AdvanceFocus(bool fBackwards)
{
	// get element to start from
	Element *pCurrElement = pActiveCtrl;
	// find new control
	for (;;)
	{
		// get next element
		pCurrElement = GetNextNestedElement(pCurrElement, fBackwards);
		// end reached: start from beginning
		if (!pCurrElement && pActiveCtrl) if (!(pCurrElement = GetNextNestedElement(nullptr, fBackwards))) return;
		// cycled?
		if (pCurrElement == pActiveCtrl)
		{
			// but current is no longer a focus element? Then defocus it and return
			if (pCurrElement && !pCurrElement->IsFocusElement())
				SetFocus(nullptr, false);
			return;
		}
		// for list elements, check whether the child can be selected
		if (pCurrElement->GetParent() && !pCurrElement->GetParent()->IsSelectedChild(pCurrElement)) continue;
		// check if this is a new control
		Control *pFocusCtrl = pCurrElement->IsFocusElement();
		if (pFocusCtrl && pFocusCtrl != pActiveCtrl && pFocusCtrl->IsVisible())
		{
			// set focus here...
			SetFocus(pFocusCtrl, false);
			// ...done!
			return;
		}
	}
	// never reached
}

bool Dialog::Show(Screen *pOnScreen, bool fCB)
{
	// already shown?
	if (fShow) return false;
	// default screen
	if (!pOnScreen) if (!(pOnScreen = Screen::GetScreenS())) return false;
	// show there
	pOnScreen->ShowDialog(this, false);
	fVisible = true;
	// developer mode: Create window
	if (!Application.isFullScreen && !IsViewportDialog())
		if (!CreateConsoleWindow()) return false;
	// CB
	if (fCB) OnShown();
	return true;
}

void Dialog::Close(bool fOK)
{
	// already closed?
	if (!fShow) return;
	// set OK flag
	this->fOK = fOK;
	// get screen
	Screen *pScreen = GetScreen();
	if (pScreen) pScreen->CloseDialog(this, false); else fShow = false;
	// developer mode: Remove window
	if (pWindow) DestroyConsoleWindow();
	// do callback - last call, because it might do perilous things
	OnClosed(fOK);
}

void Dialog::OnClosed(bool fOK)
{
	// developer mode: Remove window
	if (pWindow) DestroyConsoleWindow();
	// delete when closing?
	if (fDelOnClose)
	{
		fDelOnClose = false;
		delete this;
	}
}

bool Dialog::DoModal()
{
	// main message loop
	while (fShow)
	{
		int32_t iResult = 1;
		while ((iResult != HR_Timer) && fShow)
		{
			// dialog idle proc
			OnIdle();
			// handle messages - this may block until the next timer
			iResult = Application.HandleMessage();
			// quit
			if (iResult == HR_Failure || !IsGUIValid()) return false; // game GUI and lobby will deleted in Game::Clear()
		}
		// Idle proc may have done something nasty
		if (!IsGUIValid()) return false;
	}
	// return whether dlg was OK
	return fOK;
}

bool Dialog::Execute()
{
	// process messages
	int32_t iResult;
	while ((iResult = Application.HandleMessage()) == HR_Message)
		// check status
		if (!IsGUIValid() || !fShow) return false;
	if (iResult == HR_Failure) return false;
	// check status
	if (!IsGUIValid() || !fShow) return false;
	return true;
}

bool Dialog::IsActive(bool fForKeyboard)
{
	// must be fully visible
	if (!IsShown() || IsFading()) return false;
	// screen-less dialogs are always inactive (not yet added)
	Screen *pScreen = GetScreen();
	if (!pScreen) return false;
	// no keyboard focus if screen is in context mode
	if (fForKeyboard && pScreen->HasContext()) return false;
	// always okay in shared mode: all dlgs accessible by mouse
	if (!pScreen->IsExclusive() && !fForKeyboard) return true;
	// exclusive mode or keyboard input: Only one dlg active
	return pScreen->pActiveDlg == this;
}

bool Dialog::FadeIn(Screen *pOnScreen)
{
	// default screen
	if (!pOnScreen) if (!(pOnScreen = Screen::GetScreenS())) return false;
	// fade in there
	pOnScreen->ShowDialog(this, true);
	iFade = 0;
	eFade = eFadeIn;
	fVisible = true;
	OnShown();
	// done, success
	return true;
}

void Dialog::FadeOut(bool fCloseWithOK)
{
	// only if shown, or being faded in
	if (!IsShown() && (!fVisible || eFade != eFadeIn)) return;
	// set OK flag
	this->fOK = fCloseWithOK;
	// fade out
	Screen *pOnScreen = GetScreen();
	if (!pOnScreen) return;
	pOnScreen->CloseDialog(this, true);
	eFade = eFadeOut;
	// do callback - last call, because it might do perilous things
	OnClosed(fCloseWithOK);
}

void Dialog::ApplyElementOffset(int32_t &riX, int32_t &riY)
{
	// inherited
	Window::ApplyElementOffset(riX, riY);
	// apply viewport offset, if a viewport is assigned
	C4Viewport *pVP = GetViewport();
	if (pVP)
	{
		C4Rect rcVP(pVP->GetOutputRect());
		riX -= rcVP.x; riY -= rcVP.y;
	}
}

void Dialog::ApplyInvElementOffset(int32_t &riX, int32_t &riY)
{
	// inherited
	Window::ApplyInvElementOffset(riX, riY);
	// apply viewport offset, if a viewport is assigned
	C4Viewport *pVP = GetViewport();
	if (pVP)
	{
		C4Rect rcVP(pVP->GetOutputRect());
		riX += rcVP.x; riY += rcVP.y;
	}
}

void Dialog::SetClientSize(int32_t iToWdt, int32_t iToHgt)
{
	// calc new bounds
	iToWdt += GetMarginLeft() + GetMarginRight();
	iToHgt += GetMarginTop() + GetMarginBottom();
	rcBounds.x += (rcBounds.Wdt - iToWdt) / 2;
	rcBounds.y += (rcBounds.Hgt - iToHgt) / 2;
	rcBounds.Wdt = iToWdt; rcBounds.Hgt = iToHgt;
	// reflect changes
	UpdatePos();
}

// FullscreenDialog

FullscreenDialog::FullscreenDialog(const char *szTitle, const char *szSubtitle)
	: Dialog(Screen::GetScreenS()->GetClientRect().Wdt, Screen::GetScreenS()->GetClientRect().Hgt, nullptr /* create own title */, false), pFullscreenTitle(nullptr), pBtnHelp(nullptr)
{
	// set margins
	int32_t iScreenX = Screen::GetScreenS()->GetClientRect().Wdt;
	int32_t iScreenY = Screen::GetScreenS()->GetClientRect().Hgt;
	if (iScreenX < 500) iDlgMarginX = 2; else iDlgMarginX = iScreenX / 50;
	if (iScreenY < 320) iDlgMarginY = 2; else iDlgMarginY = iScreenY * 2 / 75;
	// set size - calcs client rect as well
	SetBounds(C4Rect(0, 0, iScreenX, iScreenY));
	// create title
	SetTitle(szTitle);
	// create subtitle (only with upperboard)
	if (szSubtitle && *szSubtitle && HasUpperBoard())
	{
		AddElement(pSubTitle = new Label(szSubtitle, rcClientRect.Wdt, C4UpperBoardHeight - GetRes()->CaptionFont.GetLineHeight() / 2 - 25 - GetMarginTop(), ARight, C4GUI_CaptionFontClr, &GetRes()->TextFont));
		pSubTitle->SetToolTip(szTitle);
	}
	else pSubTitle = nullptr;
}

void FullscreenDialog::SetTitle(const char *szTitle)
{
	delete pFullscreenTitle; pFullscreenTitle = nullptr;
	// change title text; creates or removes title bar if necessary
	if (szTitle && *szTitle)
	{
		// not using dlg label, which is a wooden label
		if (HasUpperBoard())
			pFullscreenTitle = new Label(szTitle, 0, C4UpperBoardHeight / 2 - GetRes()->TitleFont.GetLineHeight() / 2 - GetMarginTop(), ALeft, C4GUI_CaptionFontClr, &GetRes()->TitleFont);
		else
			// non-woodbar: Title is centered and in big font
			pFullscreenTitle = new Label(szTitle, GetClientRect().Wdt / 2, C4UpperBoardHeight / 2 - GetRes()->TitleFont.GetLineHeight() / 2 - GetMarginTop(), ACenter, C4GUI_FullscreenCaptionFontClr, &GetRes()->TitleFont);
		AddElement(pFullscreenTitle);
		pFullscreenTitle->SetToolTip(szTitle);
	}
}

void FullscreenDialog::DrawElement(C4FacetEx &cgo)
{
	// draw upper board
	if (HasUpperBoard())
		lpDDraw->BlitSurfaceTile(Game.GraphicsResource.fctUpperBoard.Surface, cgo.Surface, 0, std::min<int32_t>(iFade - Game.GraphicsResource.fctUpperBoard.Hgt, 0), cgo.Wdt, Game.GraphicsResource.fctUpperBoard.Hgt);
}

int32_t FullscreenDialog::GetMarginTop()
{
	return (HasUpperBoard() ? C4UpperBoard::Height() : C4GUI_FullscreenDlg_TitleHeight)
	+ iDlgMarginY;
}

void FullscreenDialog::UpdateOwnPos()
{
	// inherited to update client rect
	Dialog::UpdateOwnPos();
	// reposition help button
	UpdateHelpButtonPos();
}

void FullscreenDialog::UpdateHelpButtonPos()
{
	// reposition help button
	if (pBtnHelp) pBtnHelp->SetBounds(C4Rect(GetBounds().Wdt - 4 - 32 - GetMarginLeft(), 4 - GetMarginTop(), 32, 32));
}

void FullscreenDialog::DrawBackground(C4FacetEx &cgo, C4Facet &rFromFct)
{
	// draw across fullscreen bounds - zoom 1px border to prevent flashing borders by blit offsets
	Screen *pScr = GetScreen();
	C4Facet cgoScreen = cgo;
	C4Rect &rcScreenBounds = pScr ? pScr->GetBounds() : GetBounds();
	cgoScreen.X = rcScreenBounds.x - 1; cgoScreen.Y = rcScreenBounds.y - 1;
	cgoScreen.Wdt = rcScreenBounds.Wdt + 2; cgoScreen.Hgt = rcScreenBounds.Hgt + 2;
	rFromFct.DrawFullScreen(cgoScreen);
}

// MessageDialog

MessageDialog::MessageDialog(const char *szMessage, const char *szCaption, uint32_t dwButtons, Icons icoIcon, DlgSize eSize, bool *piConfigDontShowAgainSetting, bool fDefaultNo, int32_t zOrdering)
	: Dialog(eSize, 100 /* will be resized */, szCaption, false), piConfigDontShowAgainSetting(piConfigDontShowAgainSetting), zOrdering(zOrdering)
{
	CStdFont &rUseFont = GetRes()->TextFont;
	// get positions
	ComponentAligner caMain(GetClientRect(), C4GUI_DefDlgIndent, C4GUI_DefDlgIndent, true);
	// place icon
	C4Rect rcIcon = caMain.GetFromLeft(C4GUI_IconWdt); rcIcon.Hgt = C4GUI_IconHgt;
	Icon *pIcon = new Icon(rcIcon, icoIcon); AddElement(pIcon);
	// centered text for small dialogs and/or dialogs w/o much text (i.e.: no linebreaks)
	bool fTextCentered;
	if (eSize != dsRegular)
		fTextCentered = true;
	else
	{
		int32_t iMsgWdt = 0, iMsgHgt = 0;
		rUseFont.GetTextExtent(szMessage, iMsgWdt, iMsgHgt);
		fTextCentered = ((iMsgWdt <= caMain.GetInnerWidth() - C4GUI_IconWdt - C4GUI_DefDlgIndent * 2) && iMsgHgt <= rUseFont.GetLineHeight());
	}
	// centered text dialog: waste some icon space on the right to balance dialog
	if (fTextCentered) caMain.GetFromRight(C4GUI_IconWdt);
	// place message label
	// use text with line breaks
	StdStrBuf sMsgBroken;
	int iMsgHeight = rUseFont.BreakMessage(szMessage, caMain.GetInnerWidth(), &sMsgBroken, true);
	lblText = new Label("", caMain.GetFromTop(iMsgHeight), fTextCentered ? ACenter : ALeft, C4GUI_MessageFontClr, &rUseFont, false);
	lblText->SetText(sMsgBroken.getData(), false);
	AddElement(lblText);
	// place do-not-show-again-checkbox
	if (piConfigDontShowAgainSetting)
	{
		int w = 100, h = 20;
		const char *szCheckText = LoadResStr(C4ResStrTableKey::IDS_MSG_DONTSHOW);
		CheckBox::GetStandardCheckBoxSize(&w, &h, szCheckText, nullptr);
		CheckBox *pCheck = new C4GUI::CheckBox(caMain.GetFromTop(h, w), szCheckText, !!*piConfigDontShowAgainSetting);
		pCheck->SetOnChecked(new C4GUI::CallbackHandler<MessageDialog>(this, &MessageDialog::OnDontShowAgainCheck));
		AddElement(pCheck);
	}
	if (!fTextCentered) caMain.ExpandLeft(C4GUI_DefDlgIndent * 2 + C4GUI_IconWdt);
	// place button(s)
	ComponentAligner caButtonArea(caMain.GetFromTop(C4GUI_ButtonAreaHgt), 0, 0);
	int32_t iButtonCount = 0;
	int32_t i = 1; while (i) { if (dwButtons & i) ++iButtonCount; i = i << 1; }
	fHasOK = !!(dwButtons & btnOK) || !!(dwButtons & btnYes);
	Button *btnFocus = nullptr;
	if (iButtonCount)
	{
		C4Rect rcBtn = caButtonArea.GetCentered(iButtonCount * C4GUI_DefButton2Wdt + (iButtonCount - 1) * C4GUI_DefButton2HSpace, C4GUI_ButtonHgt);
		rcBtn.Wdt = C4GUI_DefButton2Wdt;
		// OK
		if (dwButtons & btnOK)
		{
			Button *pBtnOK = newOKButton(rcBtn);
			AddElement(pBtnOK);
			rcBtn.x += C4GUI_DefButton2Wdt + C4GUI_DefButton2HSpace;
			if (!fDefaultNo) btnFocus = pBtnOK;
		}
		// Retry
		if (dwButtons & btnRetry)
		{
			Button *pBtnRetry = newRetryButton(rcBtn);
			AddElement(pBtnRetry);
			rcBtn.x += C4GUI_DefButton2Wdt + C4GUI_DefButton2HSpace;
			if (!btnFocus) btnFocus = pBtnRetry;
		}
		// Cancel
		if (dwButtons & btnAbort)
		{
			Button *pBtnAbort = newCancelButton(rcBtn);
			AddElement(pBtnAbort);
			rcBtn.x += C4GUI_DefButton2Wdt + C4GUI_DefButton2HSpace;
			if (!btnFocus) btnFocus = pBtnAbort;
		}
		// Yes
		if (dwButtons & btnYes)
		{
			Button *pBtnYes = newYesButton(rcBtn);
			AddElement(pBtnYes);
			rcBtn.x += C4GUI_DefButton2Wdt + C4GUI_DefButton2HSpace;
			if (!btnFocus && !fDefaultNo) btnFocus = pBtnYes;
		}
		// No
		if (dwButtons & btnNo)
		{
			Button *pBtnNo = newNoButton(rcBtn);
			AddElement(pBtnNo);
			if (!btnFocus) btnFocus = pBtnNo;
		}
	}
	if (btnFocus) SetFocus(btnFocus, false);
	// resize to actually needed size
	SetClientSize(GetClientRect().Wdt, GetClientRect().Hgt - caMain.GetHeight());
}

// ConfirmationDialog

ConfirmationDialog::ConfirmationDialog(const char *szMessage, const char *szCaption, BaseCallbackHandler *pCB, uint32_t dwButtons, bool fSmall, Icons icoIcon)
	: MessageDialog(szMessage, szCaption, dwButtons, icoIcon, fSmall ? MessageDialog::dsSmall : MessageDialog::dsRegular)
{
	if (this->pCB = pCB) pCB->Ref();
	// always log confirmation messages
	spdlog::debug("[Cnf] {}: {}", szCaption, szMessage);
	// confirmations always get deleted on close
	SetDelOnClose();
}

void ConfirmationDialog::OnClosed(bool fOK)
{
	// confirmed only on OK
	BaseCallbackHandler *pStackCB = fOK ? pCB : nullptr;
	if (pStackCB) pStackCB->Ref();
	// caution: this will usually delete the dlg (this)
	// so the CB-interface is backed up
	MessageDialog::OnClosed(fOK);
	if (pStackCB)
	{
		pStackCB->DoCall(nullptr);
		pStackCB->DeRef();
	}
}

// ProgressDialog

ProgressDialog::ProgressDialog(const char *szMessage, const char *szCaption, int32_t iMaxProgress, int32_t iInitialProgress, Icons icoIcon)
	: Dialog(C4GUI_ProgressDlgWdt, 100, szCaption, false)
{
	StdStrBuf broken;
	rcBounds.Hgt = (std::max)(GetRes()->TextFont.BreakMessage(szMessage, C4GUI_ProgressDlgWdt - 3 * C4GUI_DefDlgIndent - C4GUI_IconWdt, &broken, true), C4GUI_IconHgt) + C4GUI_ProgressDlgVRoom;
	SetBounds(rcBounds);
	// get positions
	ComponentAligner caMain(GetClientRect(), C4GUI_DefDlgIndent, C4GUI_DefDlgIndent, true);
	ComponentAligner caButtonArea(caMain.GetFromBottom(C4GUI_ButtonAreaHgt), 0, 0);
	C4Rect rtProgressBar = caMain.GetFromBottom(C4GUI_ProgressDlgPBHgt);
	// place icon
	C4Rect rcIcon = caMain.GetFromLeft(C4GUI_IconWdt); rcIcon.Hgt = C4GUI_IconHgt;
	Icon *pIcon = new Icon(rcIcon, icoIcon); AddElement(pIcon);
	// place message label
	// use text with line breaks
	Label *pLblMessage = new Label(broken.getData(), caMain.GetAll().GetMiddleX(), caMain.GetAll().y, ACenter, C4GUI_MessageFontClr, &GetRes()->TextFont);
	AddElement(pLblMessage);
	// place progress bar
	pBar = new ProgressBar(rtProgressBar, iMaxProgress);
	pBar->SetProgress(iInitialProgress);
	pBar->SetToolTip(LoadResStr(C4ResStrTableKey::IDS_DLGTIP_PROGRESS));
	AddElement(pBar);
	// place abort button
	Button *pBtnAbort = newCancelButton(caButtonArea.GetCentered(C4GUI_DefButtonWdt, C4GUI_ButtonHgt));
	AddElement(pBtnAbort);
}

// Some dialog wrappers in Screen class

bool Screen::ShowMessage(const char *szMessage, const char *szCaption, Icons icoIcon, bool *piConfigDontShowAgainSetting)
{
	// always log messages
	spdlog::debug("[Msg] {}: {}", szCaption, szMessage);
	if (piConfigDontShowAgainSetting && *piConfigDontShowAgainSetting) return true;
#ifdef USE_CONSOLE
	// skip in console mode
	return true;
#endif
	return ShowRemoveDlg(new MessageDialog(szMessage, szCaption, MessageDialog::btnOK, icoIcon, MessageDialog::dsRegular, piConfigDontShowAgainSetting));
}

bool Screen::ShowErrorMessage(const char *szMessage)
{
	return ShowMessage(szMessage, LoadResStr(C4ResStrTableKey::IDS_DLG_ERROR), Ico_Error);
}

bool Screen::ShowMessageModal(const char *szMessage, const char *szCaption, uint32_t dwButtons, Icons icoIcon, bool *pbConfigDontShowAgainSetting)
{
	// always log messages
	spdlog::debug("[Modal] {}: {}", szCaption, szMessage);
	// skip if user doesn't want to see it
	if (pbConfigDontShowAgainSetting && *pbConfigDontShowAgainSetting) return true;
	// create message dlg and show modal
	return ShowModalDlg(new MessageDialog(szMessage, szCaption, dwButtons, icoIcon, MessageDialog::dsRegular, pbConfigDontShowAgainSetting));
}

bool Screen::ShowModalDlg(Dialog *pDlg, bool fDestruct)
{
#ifdef USE_CONSOLE
	// no modal dialogs in console build
	// (there's most likely no way to close them!)
	if (fDestruct) delete pDlg;
	return true;
#endif
	// safety
	if (!pDlg) return false;
	// show it
	if (!pDlg->Show(this, true)) { delete pDlg; return false; }
	// wait until it is closed
	bool fResult = pDlg->DoModal();
	// free dlg if this class is still valid (may have been deleted in game clear)
	if (!IsGUIValid()) return false;
	if (fDestruct) delete pDlg;
	// return result
	return fResult;
}

bool Screen::ShowRemoveDlg(Dialog *pDlg)
{
	// safety
	if (!pDlg) return false;
	// mark removal when done
	pDlg->SetDelOnClose();
	// show it
	if (!pDlg->Show(this, true)) { delete pDlg; return false; }
	// done, success
	return true;
}

// InputDialog

InputDialog::InputDialog(const char *szMessage, const char *szCaption, Icons icoIcon, BaseInputCallback *pCB, bool fChatLayout)
	: Dialog(fChatLayout ? Config.Graphics.ResX * 4 / 5 : C4GUI_InputDlgWdt, fChatLayout ? C4GUI::Edit::GetDefaultEditHeight() + 2 : 100, szCaption, false), pEdit(nullptr), pChatLbl(nullptr), pCB(pCB), fChatLayout(fChatLayout)
{
	if (fChatLayout)
	{
		// chat input layout
		C4GUI::ComponentAligner caChat(GetContainedClientRect(), 1, 1);
		// normal chatbox layout: Left chat label
		int32_t w = 40, h;
		C4GUI::GetRes()->TextFont.GetTextExtent(szMessage, w, h, true);
		pChatLbl = new C4GUI::WoodenLabel(szMessage, caChat.GetFromLeft(w + 4), C4GUI_CaptionFontClr, &C4GUI::GetRes()->TextFont);
		caChat.ExpandLeft(2); // undo margin
		rcEditBounds = caChat.GetAll();
		SetCustomEdit(new Edit(rcEditBounds));
		pChatLbl->SetToolTip(LoadResStr(C4ResStrTableKey::IDS_DLGTIP_CHAT));
		AddElement(pChatLbl);
	}
	else
	{
		StdStrBuf broken;
		rcBounds.Hgt = (std::max)(GetRes()->TextFont.BreakMessage(szMessage, C4GUI_InputDlgWdt - 3 * C4GUI_DefDlgIndent - C4GUI_IconWdt, &broken, true), C4GUI_IconHgt) + C4GUI_InputDlgVRoom;
		SetBounds(rcBounds);
		// regular input dialog layout
		// get positions
		ComponentAligner caMain(GetClientRect(), C4GUI_DefDlgIndent, C4GUI_DefDlgIndent, true);
		ComponentAligner caButtonArea(caMain.GetFromBottom(C4GUI_ButtonAreaHgt), 0, 0);
		rcEditBounds = caMain.GetFromBottom(Edit::GetDefaultEditHeight());
		// place icon
		C4Rect rcIcon = caMain.GetFromLeft(C4GUI_IconWdt); rcIcon.Hgt = C4GUI_IconHgt;
		Icon *pIcon = new Icon(rcIcon, icoIcon); AddElement(pIcon);
		// place message label
		// use text with line breaks
		Label *pLblMessage = new Label(broken.getData(), caMain.GetAll().GetMiddleX(), caMain.GetAll().y, ACenter, C4GUI_MessageFontClr, &GetRes()->TextFont);
		AddElement(pLblMessage);
		// place input edit
		SetCustomEdit(new Edit(rcEditBounds));
		// place buttons
		C4Rect rcBtn = caButtonArea.GetCentered(2 * C4GUI_DefButton2Wdt + C4GUI_DefButton2HSpace, C4GUI_ButtonHgt);
		rcBtn.Wdt = C4GUI_DefButton2Wdt;
		// OK
		Button *pBtnOK = newOKButton(rcBtn);
		AddElement(pBtnOK);
		rcBtn.x += rcBtn.Wdt + C4GUI_DefButton2HSpace;
		// Cancel
		Button *pBtnAbort = newCancelButton(rcBtn);
		AddElement(pBtnAbort);
		rcBtn.x += rcBtn.Wdt + C4GUI_DefButton2HSpace;
	}
	// input dlg always closed in the end
	SetDelOnClose();
}

void InputDialog::SetMaxText(int32_t iMaxLen)
{
	pEdit->SetMaxText(iMaxLen);
}

void InputDialog::SetInputText(const char *szToText)
{
	pEdit->SelectAll(); pEdit->DeleteSelection();
	if (szToText)
	{
		pEdit->InsertText(szToText, false);
		pEdit->SelectAll();
	}
}

void InputDialog::SetCustomEdit(Edit *pCustomEdit)
{
	// del old
	delete pEdit;
	// add new
	pEdit = pCustomEdit;
	pEdit->SetBounds(rcEditBounds);
	if (fChatLayout)
	{
		pEdit->SetToolTip(LoadResStr(C4ResStrTableKey::IDS_DLGTIP_CHAT));
		pChatLbl->SetClickFocusControl(pEdit); // 2do: to all, to allies, etc.
	}
	AddElement(pEdit);
	SetFocus(pEdit, false);
}

void InputDialog::OnClosed(bool fOK)
{
	if (pCB && fOK)
	{
		pCB->OnOK(StdStrBuf::MakeRef(pEdit->GetText()));
	}
	Dialog::OnClosed(fOK);
}

const char *InputDialog::GetInputText()
{
	return pEdit->GetText();
}

// InfoDialog

InfoDialog::InfoDialog(const char *szCaption, int32_t iLineCount)
	: Dialog(C4GUI_InfoDlgWdt, GetRes()->TextFont.GetLineHeight() * iLineCount + C4GUI_InfoDlgVRoom, szCaption, false), iScroll(0), pSec1Timer(nullptr)
{
	// timer
	pSec1Timer = new C4Sec1TimerCallback<InfoDialog>(this);
	CreateSubComponents();
}

InfoDialog::InfoDialog(const char *szCaption, int iLineCount, const StdStrBuf &sText)
	: Dialog(C4GUI_InfoDlgWdt, GetRes()->TextFont.GetLineHeight() * iLineCount + C4GUI_InfoDlgVRoom, szCaption, false), iScroll(0), pSec1Timer(nullptr)
{
	// ctor - init w/o timer
	CreateSubComponents();
	// fill in initial text
	for (size_t i = 0; i < sText.getLength(); ++i)
	{
		size_t i0 = i;
		while (sText[i] != '|' && sText[i]) ++i;
		StdStrBuf sLine = sText.copyPart(i0, i - i0);
		pTextWin->AddTextLine(sLine.getData(), &GetRes()->TextFont, C4GUI_MessageFontClr, false, true);
	}
	pTextWin->UpdateHeight();
}

InfoDialog::~InfoDialog()
{
	if (pSec1Timer) pSec1Timer->Release();
}

void InfoDialog::CreateSubComponents()
{
	// get positions
	ComponentAligner caMain(GetClientRect(), C4GUI_DefDlgIndent, C4GUI_DefDlgIndent, true);
	ComponentAligner caButtonArea(caMain.GetFromBottom(C4GUI_ButtonAreaHgt), 0, 0);
	// place info box
	pTextWin = new TextWindow(caMain.GetAll(), 0, 0, 0, 100, 4096, "  ", true, nullptr, 0);
	AddElement(pTextWin);
	// place close button
	Button *pBtnClose = newDlgCloseButton(caButtonArea.GetCentered(C4GUI_DefButtonWdt, C4GUI_ButtonHgt));
	AddElement(pBtnClose); pBtnClose->SetToolTip(LoadResStr(C4ResStrTableKey::IDS_MNU_CLOSE));
}

void InfoDialog::AddLine(const char *szText)
{
	// add line to text window
	if (!pTextWin) return;
	pTextWin->AddTextLine(szText, &GetRes()->TextFont, C4GUI_MessageFontClr, false, true);
}

void InfoDialog::BeginUpdateText()
{
	// safety
	if (!pTextWin) return;
	// backup scrolling
	iScroll = pTextWin->GetScrollPos();
	// clear text window, so new text can be added
	pTextWin->ClearText(false);
}

void InfoDialog::EndUpdateText()
{
	// safety
	if (!pTextWin) return;
	// update text height
	pTextWin->UpdateHeight();
	// restore scrolling
	pTextWin->SetScrollPos(iScroll);
}

void InfoDialog::OnSec1Timer()
{
	// always update
	UpdateText();
}

TimedDialog::TimedDialog(uint32_t time, const char *message, const char *caption, uint32_t buttons, Icons icon, DlgSize size, bool *configDontShowAgainSetting, bool defaultNo, int32_t zOrdering)
	: MessageDialog{message, caption, buttons, icon, size, configDontShowAgainSetting, defaultNo, zOrdering}, time{time}
{
	sec1Timer = new C4Sec1TimerCallback<TimedDialog>{this};
}

TimedDialog::~TimedDialog()
{
	sec1Timer->Release();
}

void TimedDialog::OnSec1Timer()
{
	if (--time == 0)
	{
		Close(false);
		return;
	}

	UpdateText();
}

void TimedDialog::SetText(const char *message)
{
	lblText->SetText(message);

	ComponentAligner caMain{GetClientRect(), C4GUI_DefDlgIndent, C4GUI_DefDlgIndent, true};
	caMain.ExpandTop(-lblText->GetHeight());
	ComponentAligner caButtonArea{caMain.GetFromTop(C4GUI_ButtonAreaHgt), 0, 0};
	const C4Rect &bounds{caButtonArea.GetCentered(C4GUI_DefButtonWdt, C4GUI_DefButton2HSpace)};

	int32_t oldButtonY{-1};
	int32_t oldButtonHeight{0};

	for (Element *element{GetFirst()}; element; element = element->GetNext())
	{
		if (element != pCloseBtn)
		{
			if (auto *btn = dynamic_cast<Button *>(element); btn)
			{
				int32_t &y{btn->GetBounds().y};
				if (oldButtonY == -1)
				{
					oldButtonY = y;
					oldButtonHeight = btn->GetHeight();
				}

				y = bounds.y;
			}
		}
	}

	GetClientRect().Hgt = bounds.y + oldButtonHeight + C4GUI_DefButton2HSpace;
	if (pTitle)
	{
		GetBounds().Hgt = GetClientRect().Hgt + pTitle->GetHeight();
	}
	UpdateSize();
}

void CloseButton::OnPress()
{
	Dialog *pDlg;
	if ((pDlg = GetDlg()))
	{
		pDlg->UserClose(fCloseResult);
	}
}

void CloseIconButton::OnPress()
{
	Dialog *pDlg;
	if ((pDlg = GetDlg()))
	{
		pDlg->UserClose(fCloseResult);
	}
}
} // end of namespace
