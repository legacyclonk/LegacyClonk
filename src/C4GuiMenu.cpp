/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
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

// generic user interface
// context menu

#include "C4Config.h"
#include "C4GuiResource.h"
#include <C4Include.h>
#include <C4Gui.h>
#include <C4FacetEx.h>
#include <C4Wrappers.h>
#include <C4MouseControl.h>

#include "StdApp.h"
#include <StdWindow.h>

namespace C4GUI
{

int32_t ContextMenu::iGlobalMenuIndex = 0;

// ContextMenu::Entry

void ContextMenu::Entry::DrawElement(C4FacetEx &cgo)
{
	// icon
	if (icoIcon > Ico_None)
	{
		// get icon counts
		int32_t iXMax, iYMax;
		GetRes()->fctIcons.GetPhaseNum(iXMax, iYMax);
		if (!iXMax)
			iXMax = 6;
		// load icon
		const C4Facet &rfctIcon = GetRes()->fctIcons.GetPhase(icoIcon % iXMax, icoIcon / iXMax);
		rfctIcon.DrawX(cgo.Surface, rcBounds.x + cgo.TargetX, rcBounds.y + cgo.TargetY, rcBounds.Hgt, rcBounds.Hgt);
	}
	// print out label
	if (!!sText)
		lpDDraw->TextOut(sText.getData(), GetRes()->TextFont, 1.0f, cgo.Surface, cgo.TargetX + rcBounds.x + GetIconIndent(), rcBounds.y + cgo.TargetY, C4GUI_ContextFontClr, ALeft);
	// submenu arrow
	if (pSubmenuHandler)
	{
		C4Facet &rSubFct = GetRes()->fctSubmenu;
		rSubFct.Draw(cgo.Surface, cgo.TargetX + rcBounds.x + rcBounds.Wdt - rSubFct.Wdt, cgo.TargetY + rcBounds.y + (rcBounds.Hgt - rSubFct.Hgt) / 2);
	}
}

ContextMenu::Entry::Entry(const char *szText, Icons icoIcon, MenuHandler *pMenuHandler, ContextHandler *pSubmenuHandler)
	: Element(), cHotkey(0), icoIcon(icoIcon), pMenuHandler(pMenuHandler), pSubmenuHandler(pSubmenuHandler)
{
	// set text with hotkey
	if (szText)
	{
		sText.Copy(szText);
		ExpandHotkeyMarkup(sText, cHotkey);
		// adjust size
		GetRes()->TextFont.GetTextExtent(sText.getData(), rcBounds.Wdt, rcBounds.Hgt, true);
	}
	else
	{
		rcBounds.Wdt = 40;
		rcBounds.Hgt = GetRes()->TextFont.GetLineHeight();
	}
	// regard icon
	rcBounds.Wdt += GetIconIndent();
	// submenu arrow
	if (pSubmenuHandler) rcBounds.Wdt += GetRes()->fctSubmenu.Wdt + 2;
}

// ContextMenu

ContextMenu::ContextMenu() : Window(), pTarget(nullptr), pSelectedItem(nullptr), pSubmenu(nullptr)
{
	iMenuIndex = ++iGlobalMenuIndex;
	// set min size
	rcBounds.Wdt = 40; rcBounds.Hgt = 7;
	// key bindings
	C4CustomKey::CodeList Keys;
	Keys.push_back(C4KeyCodeEx(K_UP));
	if (Config.Controls.GamepadGuiControl)
	{
		Keys.push_back(C4KeyCodeEx(KEY_Gamepad(0, KEY_JOY_Up)));
	}
	pKeySelUp = new C4KeyBinding(Keys, "GUIContextSelUp", KEYSCOPE_Gui,
		new C4KeyCB<ContextMenu>(*this, &ContextMenu::KeySelUp), C4CustomKey::PRIO_Context);

	Keys.clear();
	Keys.push_back(C4KeyCodeEx(K_DOWN));
	if (Config.Controls.GamepadGuiControl)
	{
		Keys.push_back(C4KeyCodeEx(KEY_Gamepad(0, KEY_JOY_Down)));
	}
	pKeySelDown = new C4KeyBinding(Keys, "GUIContextSelDown", KEYSCOPE_Gui,
		new C4KeyCB<ContextMenu>(*this, &ContextMenu::KeySelDown), C4CustomKey::PRIO_Context);

	Keys.clear();
	Keys.push_back(C4KeyCodeEx(K_RIGHT));
	if (Config.Controls.GamepadGuiControl)
	{
		Keys.push_back(C4KeyCodeEx(KEY_Gamepad(0, KEY_JOY_Right)));
	}
	pKeySubmenu = new C4KeyBinding(Keys, "GUIContextSubmenu", KEYSCOPE_Gui,
		new C4KeyCB<ContextMenu>(*this, &ContextMenu::KeySubmenu), C4CustomKey::PRIO_Context);

	Keys.clear();
	Keys.push_back(C4KeyCodeEx(K_LEFT));
	if (Config.Controls.GamepadGuiControl)
	{
		Keys.push_back(C4KeyCodeEx(KEY_Gamepad(0, KEY_JOY_Left)));
	}
	pKeyBack = new C4KeyBinding(Keys, "GUIContextBack", KEYSCOPE_Gui,
		new C4KeyCB<ContextMenu>(*this, &ContextMenu::KeyBack), C4CustomKey::PRIO_Context);

	Keys.clear();
	Keys.push_back(C4KeyCodeEx(K_ESCAPE));
	if (Config.Controls.GamepadGuiControl)
	{
		Keys.push_back(C4KeyCodeEx(KEY_Gamepad(0, KEY_JOY_AnyHighButton)));
	}
	pKeyAbort = new C4KeyBinding(Keys, "GUIContextAbort", KEYSCOPE_Gui,
		new C4KeyCB<ContextMenu>(*this, &ContextMenu::KeyAbort), C4CustomKey::PRIO_Context);

	Keys.clear();
	Keys.push_back(C4KeyCodeEx(K_RETURN));
	if (Config.Controls.GamepadGuiControl)
	{
		Keys.push_back(C4KeyCodeEx(KEY_Gamepad(0, KEY_JOY_AnyLowButton)));
	}
	pKeyConfirm = new C4KeyBinding(Keys, "GUIContextConfirm", KEYSCOPE_Gui,
		new C4KeyCB<ContextMenu>(*this, &ContextMenu::KeyConfirm), C4CustomKey::PRIO_Context);

	pKeyHotkey = new C4KeyBinding(C4KeyCodeEx(KEY_Any), "GUIContextHotkey", KEYSCOPE_Gui,
		new C4KeyCBPassKey<ContextMenu>(*this, &ContextMenu::KeyHotkey), C4CustomKey::PRIO_Context);
}

ContextMenu::~ContextMenu()
{
	// del any submenu
	delete pSubmenu; pSubmenu = nullptr;
	// forward RemoveElement to screen
	Screen *pScreen = GetScreen();
	if (pScreen) pScreen->RemoveElement(this);
	// clear key bindings
	delete pKeySelUp;
	delete pKeySelDown;
	delete pKeySubmenu;
	delete pKeyBack;
	delete pKeyAbort;
	delete pKeyConfirm;
	delete pKeyHotkey;
	// clear children to get appropriate callbacks
	Clear();
}

void ContextMenu::Abort(bool fByUser)
{
	// effect
	if (fByUser) GUISound("DoorClose");
	// simply del menu: dtor will remove itself
	delete this;
}

void ContextMenu::DrawElement(C4FacetEx &cgo)
{
	// draw context menu bg
	lpDDraw->DrawBoxDw(cgo.Surface, rcBounds.x + cgo.TargetX, rcBounds.y + cgo.TargetY,
		rcBounds.x + rcBounds.Wdt + cgo.TargetX - 1, rcBounds.y + rcBounds.Hgt + cgo.TargetY - 1,
		C4GUI_ContextBGColor);
	// context bg: mark selected item
	if (pSelectedItem)
	{
		// get marked item bounds
		C4Rect rcSelArea = pSelectedItem->GetBounds();
		// do indent
		rcSelArea.x += GetClientRect().x;
		rcSelArea.y += GetClientRect().y;
		// draw
		lpDDraw->DrawBoxDw(cgo.Surface, rcSelArea.x + cgo.TargetX, rcSelArea.y + cgo.TargetY,
			rcSelArea.x + rcSelArea.Wdt + cgo.TargetX - 1, rcSelArea.y + rcSelArea.Hgt + cgo.TargetY - 1,
			C4GUI_ContextSelColor);
	}
	// draw frame
	Draw3DFrame(cgo);
}

void ContextMenu::MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, uint32_t dwKeyParam)
{
	// inherited
	Window::MouseInput(rMouse, iButton, iX, iY, dwKeyParam);
	// mouse is in client area?
	if (GetClientRect().Contains(iX + rcBounds.x, iY + rcBounds.y))
	{
		// reset selection
		Element *pPrevSelectedItem = pSelectedItem;
		pSelectedItem = nullptr;
		// get client component the mouse is over
		iX -= GetMarginLeft(); iY -= GetMarginTop();
		for (Element *pCurr = GetFirst(); pCurr; pCurr = pCurr->GetNext())
			if (pCurr->GetBounds().Contains(iX, iY))
				pSelectedItem = pCurr;
		// selection change sound
		if (pSelectedItem != pPrevSelectedItem)
		{
			SelectionChanged(true);
			// selection by mouse: Check whether submenu can be opened
			CheckOpenSubmenu();
		}
		// check mouse click
		if (iButton == C4MC_Button_LeftDown)
		{
			DoOK(); return;
		}
	}
}

void ContextMenu::MouseLeaveEntry(CMouse &rMouse, Entry *pOldEntry)
{
	// no submenu open? then deselect any selected item
	if (pOldEntry == pSelectedItem && !pSubmenu)
	{
		pSelectedItem = nullptr;
		SelectionChanged(true);
	}
}

bool ContextMenu::KeySelUp()
{
	// not if focus is in submenu
	if (pSubmenu) return false;
	Element *pPrevSelectedItem = pSelectedItem;
	// select prev
	if (pSelectedItem) pSelectedItem = pSelectedItem->GetPrev();
	// nothing selected or beginning reached: cycle
	if (!pSelectedItem) pSelectedItem = GetLastContained();
	// selection might have changed
	if (pSelectedItem != pPrevSelectedItem) SelectionChanged(true);
	return true;
}

bool ContextMenu::KeySelDown()
{
	// not if focus is in submenu
	if (pSubmenu) return false;
	Element *pPrevSelectedItem = pSelectedItem;
	// select next
	if (pSelectedItem) pSelectedItem = pSelectedItem->GetNext();
	// nothing selected or end reached: cycle
	if (!pSelectedItem) pSelectedItem = GetFirstContained();
	// selection might have changed
	if (pSelectedItem != pPrevSelectedItem) SelectionChanged(true);
	return true;
}

bool ContextMenu::KeySubmenu()
{
	// not if focus is in submenu
	if (pSubmenu) return false;
	CheckOpenSubmenu();
	return true;
}

bool ContextMenu::KeyBack()
{
	// not if focus is in submenu
	if (pSubmenu) return false;
	// close submenu on keyboard input
	if (IsSubmenu()) { Abort(true); return true; }
	return false;
}

bool ContextMenu::KeyAbort()
{
	// not if focus is in submenu
	if (pSubmenu) return false;
	Abort(true);
	return true;
}

bool ContextMenu::KeyConfirm()
{
	// not if focus is in submenu
	if (pSubmenu) return false;
	CheckOpenSubmenu();
	DoOK();
	return true;
}

bool ContextMenu::KeyHotkey(C4KeyCodeEx key)
{
	// not if focus is in submenu
	if (pSubmenu) return false;
	Element *pPrevSelectedItem = pSelectedItem;
	C4KeyCode wKey = TOUPPERIFX11(key.Key);
	if (Inside<C4KeyCode>(wKey, 'A', 'Z') || Inside<C4KeyCode>(wKey, '0', '9'))
	{
		// process hotkeys
		char ch = char(wKey);
		for (Element *pCurr = GetFirst(); pCurr; pCurr = pCurr->GetNext())
			if (pCurr->OnHotkey(ch))
			{
				pSelectedItem = pCurr;
				if (pSelectedItem != pPrevSelectedItem) SelectionChanged(true);
				CheckOpenSubmenu();
				DoOK();
				return true;
			}
		return false;
	}
	// unrecognized hotkey
	return false;
}

void ContextMenu::UpdateElementPositions()
{
	// first item at zero offset
	Element *pCurr = GetFirst();
	if (!pCurr) return;
	pCurr->GetBounds().y = 0;
	int32_t iMinWdt = std::max<int32_t>(20, pCurr->GetBounds().Wdt);
	int32_t iOverallHgt = pCurr->GetBounds().Hgt;
	// others stacked under it
	while (pCurr = pCurr->GetNext())
	{
		iMinWdt = (std::max)(iMinWdt, pCurr->GetBounds().Wdt);
		int32_t iYSpace = pCurr->GetListItemTopSpacing();
		int32_t iNewY = iOverallHgt + iYSpace;
		iOverallHgt += pCurr->GetBounds().Hgt + iYSpace;
		if (iNewY != pCurr->GetBounds().y)
		{
			pCurr->GetBounds().y = iNewY;
			pCurr->UpdateOwnPos();
		}
	}
	// don't make smaller
	iMinWdt = (std::max)(iMinWdt, rcBounds.Wdt - GetMarginLeft() - GetMarginRight());
	// all entries same size
	for (pCurr = GetFirst(); pCurr; pCurr = pCurr->GetNext())
		if (pCurr->GetBounds().Wdt != iMinWdt)
		{
			pCurr->GetBounds().Wdt = iMinWdt;
			pCurr->UpdateOwnPos();
		}
	// update own size
	rcBounds.Wdt = iMinWdt + GetMarginLeft() + GetMarginRight();
	rcBounds.Hgt = std::max<int32_t>(iOverallHgt, 8) + GetMarginTop() + GetMarginBottom();
	UpdateSize();
}

void ContextMenu::RemoveElement(Element *pChild)
{
	// inherited
	Window::RemoveElement(pChild);
	// target lost?
	if (pChild == pTarget) { Abort(false); return; }
	// submenu?
	if (pChild == pSubmenu) pSubmenu = nullptr;
	// clear selection var
	if (pChild == pSelectedItem)
	{
		pSelectedItem = nullptr;
		SelectionChanged(false);
	}
	// forward to any submenu
	if (pSubmenu) pSubmenu->RemoveElement(pChild);
	// forward to mouse
	if (GetScreen())
		GetScreen()->Mouse.RemoveElement(pChild);
	// update positions
	UpdateElementPositions();
}

bool ContextMenu::AddElement(Element *pChild)
{
	// add it
	Window::AddElement(pChild);
	// update own size and positions
	UpdateElementPositions();
	// success
	return true;
}

void ContextMenu::ElementSizeChanged(Element *pOfElement)
{
	// inherited
	Window::ElementSizeChanged(pOfElement);
	// update positions of all list items
	UpdateElementPositions();
}

void ContextMenu::ElementPosChanged(Element *pOfElement)
{
	// inherited
	Window::ElementSizeChanged(pOfElement);
	// update positions of all list items
	UpdateElementPositions();
}

void ContextMenu::SelectionChanged(bool fByUser)
{
	// any selection?
	if (pSelectedItem)
	{
		// effect
		if (fByUser) GUISound("Command");
	}
	// close any submenu from prev selection
	if (pSubmenu) pSubmenu->Abort(true);
}

Screen *ContextMenu::GetScreen()
{
	// context menus don't have a parent; get screen by static var
	return Screen::GetScreenS();
}

bool ContextMenu::CtxMouseInput(CMouse &rMouse, int32_t iButton, int32_t iScreenX, int32_t iScreenY, uint32_t dwKeyParam)
{
	// check submenu
	if (pSubmenu)
		if (pSubmenu->CtxMouseInput(rMouse, iButton, iScreenX, iScreenY, dwKeyParam)) return true;
	// check bounds
	if (!rcBounds.Contains(iScreenX, iScreenY)) return false;
	// inside menu: do input in local coordinates
	MouseInput(rMouse, iButton, iScreenX - rcBounds.x, iScreenY - rcBounds.y, dwKeyParam);
	return true;
}

bool ContextMenu::CharIn(const char *c)
{
	// forward to submenu
	if (pSubmenu) return pSubmenu->CharIn(c);
	return false;
}

void ContextMenu::Draw(C4FacetEx &cgo)
{
	// draw self
	Window::Draw(cgo);
	// draw submenus on top
	if (pSubmenu) pSubmenu->Draw(cgo);
}

void ContextMenu::Open(Element *pTarget, int32_t iScreenX, int32_t iScreenY)
{
	// set pos
	rcBounds.x = iScreenX; rcBounds.y = iScreenY;
	UpdatePos();
	// set target
	this->pTarget = pTarget;
	// effect :)
	GUISound("DoorOpen");
	// done
}

void ContextMenu::CheckOpenSubmenu()
{
	// safety
	if (!GetScreen()) return;
	// anything selected?
	if (!pSelectedItem) return;
	// get as entry
	Entry *pSelEntry = static_cast<Entry *>(pSelectedItem);
	// has submenu handler?
	ContextHandler *pSubmenuHandler = pSelEntry->pSubmenuHandler;
	if (!pSubmenuHandler) return;
	// create submenu then
	if (pSubmenu) pSubmenu->Abort(false);
	pSubmenu = pSubmenuHandler->OnSubcontext(pTarget);
	// get open pos
	int32_t iX = GetClientRect().x + pSelEntry->GetBounds().x + pSelEntry->GetBounds().Wdt;
	int32_t iY = GetClientRect().y + pSelEntry->GetBounds().y + pSelEntry->GetBounds().Hgt / 2;
	int32_t iScreenWdt = GetScreen()->GetBounds().Wdt, iScreenHgt = GetScreen()->GetBounds().Hgt;
	if (iY + pSubmenu->GetBounds().Hgt >= iScreenHgt)
	{
		// bottom too narrow: open to top, if height is sufficient
		// otherwise, open to top from bottom screen pos
		if (iY < pSubmenu->GetBounds().Hgt) iY = iScreenHgt;
		iY -= pSubmenu->GetBounds().Hgt;
	}
	if (iX + pSubmenu->GetBounds().Wdt >= iScreenWdt)
	{
		// right too narrow: try opening left of this menu
		// otherwise, open to left from right screen border
		if (GetClientRect().x < pSubmenu->GetBounds().Wdt)
			iX = iScreenWdt;
		else
			iX = GetClientRect().x;
		iX -= pSubmenu->GetBounds().Wdt;
	}
	// open it
	pSubmenu->Open(pTarget, iX, iY);
}

bool ContextMenu::IsSubmenu()
{
	return GetScreen() && GetScreen()->pContext != this;
}

void ContextMenu::DoOK()
{
	// safety
	if (!GetScreen()) return;
	// anything selected?
	if (!pSelectedItem) return;
	// get as entry
	Entry *pSelEntry = static_cast<Entry *>(pSelectedItem);
	// get CB; take over pointer
	MenuHandler *pCallback = pSelEntry->GetAndZeroCallback();
	Element *pTarget = this->pTarget;
	if (!pCallback) return;
	// close all menus (deletes this class!) w/o sound
	GetScreen()->AbortContext(false);
	// sound
	GUISound("Click");
	// do CB
	pCallback->OnOK(pTarget);
	// free CB class
	delete pCallback;
}

} // end of namespace
