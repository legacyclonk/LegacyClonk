/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2024, The LegacyClonk Team and contributors
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

/* Handles viewport editing in console mode */

#include <C4EditCursor.h>

#include <C4Console.h>
#include <C4Object.h>
#include <C4Application.h>
#include <C4Random.h>
#include "C4Viewport.h"
#include <C4Wrappers.h>

#ifdef _WIN32
#include "StdStringEncodingConverter.h"
#include "res/engine_resource.h"
#endif

#ifdef WITH_DEVELOPER_MODE
#include <gtk/gtk.h>
#endif

C4EditCursor::C4EditCursor()
{
	Default();
}

C4EditCursor::~C4EditCursor()
{
	Clear();
}

void C4EditCursor::Execute()
{
	// alt check
	bool fAltIsDown = Application.IsAltDown();
	if (fAltIsDown != fAltWasDown)
	{
		if ((fAltWasDown = fAltIsDown))
		{
			AltDown();
		}
		else
		{
			AltUp();
		}
	}
	// drawing
	switch (mode)
	{
	case ConsoleMode::Edit:
		// Hold selection
		if (holdLeft)
			EMMoveObject(EMMO_Move, 0, 0, nullptr, &selection);
		break;

	case ConsoleMode::Draw:
		switch (Console.ToolsDlg.tool)
		{
		case ToolMode::Fill:
			if (holdLeft) if (!Game.HaltCount) if (Console.Editing) ApplyToolFill();
			break;
		}
		break;
	}
	// selection update
	if (fSelectionChanged)
	{
		fSelectionChanged = false;
		UpdateStatusBar();
		Console.PropertyDlg.Update(selection);
		Console.ObjectListDlg.Update(selection);
	}
}

bool C4EditCursor::Init()
{
#ifdef _WIN32
	if (!(hMenu = LoadMenu(Application.hInstance, MAKEINTRESOURCE(IDR_CONTEXTMENUS))))
		return false;
#else // _WIN32
#ifdef WITH_DEVELOPER_MODE
	menuContext = gtk_menu_new();

	itemDelete =       gtk_menu_item_new_with_label(LoadResStrGtk(C4ResStrTableKey::IDS_MNU_DELETE).c_str());
	itemDuplicate =    gtk_menu_item_new_with_label(LoadResStrGtk(C4ResStrTableKey::IDS_MNU_DUPLICATE).c_str());
	itemGrabContents = gtk_menu_item_new_with_label(LoadResStrGtk(C4ResStrTableKey::IDS_MNU_CONTENTS).c_str());
	itemProperties =   gtk_menu_item_new_with_label(""); // Set dynamically in DoContextMenu

	gtk_menu_shell_append(GTK_MENU_SHELL(menuContext), itemDelete);
	gtk_menu_shell_append(GTK_MENU_SHELL(menuContext), itemDuplicate);
	gtk_menu_shell_append(GTK_MENU_SHELL(menuContext), itemGrabContents);
	gtk_menu_shell_append(GTK_MENU_SHELL(menuContext), GTK_WIDGET(gtk_separator_menu_item_new()));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuContext), itemProperties);

	g_signal_connect(G_OBJECT(itemDelete),       "activate", G_CALLBACK(OnDelete),       this);
	g_signal_connect(G_OBJECT(itemDuplicate),    "activate", G_CALLBACK(OnDuplicate),    this);
	g_signal_connect(G_OBJECT(itemGrabContents), "activate", G_CALLBACK(OnGrabContents), this);
	g_signal_connect(G_OBJECT(itemProperties),   "activate", G_CALLBACK(OnProperties),   this);

	gtk_widget_show_all(menuContext);
#endif // WITH_DEVELOPER_MODe
#endif // _WIN32
	Console.UpdateModeCtrls(mode);

	return true;
}

void C4EditCursor::ClearPointers(C4Object *pObj)
{
	if (target == pObj) target = nullptr;
	if (selection.ClearPointers(pObj))
		OnSelectionChanged();
}

bool C4EditCursor::Move(C4Viewport *const cvp, int32_t iX, int32_t iY, uint16_t wKeyFlags)
{
	// Viewport Space
	const std::int32_t viewOffsetX {iX - viewSpaceX};
	const std::int32_t viewOffsetY {iY - viewSpaceY};
	viewSpaceX = iX;
	viewSpaceY = iY;
	// Map space
	const std::int32_t offsetX {cvp->ViewX + iX - X};
	const std::int32_t offsetY {cvp->ViewY + iY - Y};
	X = cvp->ViewX + iX;
	Y = cvp->ViewY + iY;

	if(holdRight)
	{
		if(cvp->fIsNoOwnerViewport)
		{
			cvp->ViewX -= viewOffsetX;
			cvp->ViewY -= viewOffsetY;
			cvp->UpdateViewPosition();
			cvp->ScrollBarsByViewPosition();
			// Allow the context menu on right click to be opened when we didn't drag the viewport.
			if(std::abs(viewOffsetX) > 1 || std::abs(viewOffsetY) > 1)
			{
				dragViewport = true;
			}
		}
	}

	switch (mode)
	{
	case ConsoleMode::Edit:
		// Hold
		if (!dragFrame && holdLeft)
		{
			MoveSelection(offsetX, offsetY);
			UpdateDropTarget(wKeyFlags);
		}
		// Update target
		// Shift always indicates a target outside the current selection
		else
		{
			target = ((wKeyFlags & MK_SHIFT) && selection.Last) ? selection.Last->Obj : nullptr;
			do
			{
				target = Game.FindObject(0, X, Y, 0, 0, OCF_NotContained, nullptr, nullptr, nullptr, nullptr, ANY_OWNER, target);
			} while ((wKeyFlags & MK_SHIFT) && target && selection.GetLink(target));
		}
		break;

	case ConsoleMode::Draw:
		switch (Console.ToolsDlg.tool)
		{
		case ToolMode::Brush:
			if (holdLeft) ApplyToolBrush();
			break;
		case ToolMode::Line: case ToolMode::Rect:
			break;
		}
		break;
	}

	// Update
	UpdateStatusBar();
	return true;
}

bool C4EditCursor::UpdateStatusBar()
{
	std::string text;
	switch (mode)
	{
	case ConsoleMode::Play:
		if (Game.MouseControl.GetCaption())
		{
			std::string caption{Game.MouseControl.GetCaption()};
			text = std::move(caption).substr(0, caption.find('|'));
		}
		break;

	case ConsoleMode::Edit:
		text = std::format("{}/{} ({})", X, Y, (target ? (target->GetName()) : LoadResStr(C4ResStrTableKey::IDS_CNS_NOTHING)));
		break;

	case ConsoleMode::Draw:
		text = std::format("{}/{} ({})", X, Y, (MatValid(GBackMat(X, Y)) ? Game.Material.Map[GBackMat(X, Y)].Name : LoadResStr(C4ResStrTableKey::IDS_CNS_NOTHING)));
		break;
	}
	return Console.UpdateCursorBar(text.c_str());
}

void C4EditCursor::OnSelectionChanged()
{
	fSelectionChanged = true;
}

bool C4EditCursor::LeftButtonDown(bool fControl)
{
	// Hold
	holdLeft = true;

	switch (mode)
	{
	case ConsoleMode::Edit:
		if (fControl)
		{
			// Toggle target
			if (target)
			{
				if (!selection.Remove(target))
				{
					selection.Add(target, C4ObjectList::stNone);
					OnSelectionChanged();
				}
			}
		}
		else
		{
			// Click on unselected: select single
			if (target && !selection.GetLink(target))
			{
				if(!Application.IsShiftDown())
				{
					selection.Clear();
				}

				selection.Add(target, C4ObjectList::stNone);
				OnSelectionChanged();
			}
			// Click on nothing: drag frame
			if (!target)
			{
				if(!Application.IsShiftDown())
				{
					selection.Clear();
					OnSelectionChanged();
				}
				dragFrame = true;
				X2 = X;
				Y2 = Y;
			}
		}
		break;

	case ConsoleMode::Draw:
		switch (Console.ToolsDlg.tool)
		{
		case ToolMode::Brush: ApplyToolBrush(); break;
		case ToolMode::Line: dragLine  = true; X2 = X; Y2 = Y; break;
		case ToolMode::Rect: dragFrame = true; X2 = X; Y2 = Y; break;
		case ToolMode::Fill:
			if (Game.HaltCount)
			{
				holdLeft = false; Console.Message(LoadResStr(C4ResStrTableKey::IDS_CNS_FILLNOHALT)); return false;
			}
			break;
		case ToolMode::Picker: ApplyToolPicker(); break;
		}
		break;
	}

	dropTarget = nullptr;

	return true;
}

bool C4EditCursor::RightButtonDown(bool fControl)
{
	holdRight = true;

	switch (mode)
	{
	case ConsoleMode::Edit:
		if (!fControl)
		{
			if (target && selection.IsClear())
			{
				selection.Add(target, C4ObjectList::stNone);
				OnSelectionChanged();
			}
		}
		break;
	}

	return true;
}

bool C4EditCursor::LeftButtonUp()
{
	// Finish edit/tool
	switch (mode)
	{
	case ConsoleMode::Edit:
		if (dragFrame) FrameSelection();
		if (dropTarget) PutContents();
		break;

	case ConsoleMode::Draw:
		switch (Console.ToolsDlg.tool)
		{
		case ToolMode::Line:
			if (dragLine) ApplyToolLine();
			break;
		case ToolMode::Rect:
			if (dragFrame) ApplyToolRect();
			break;
		}
		break;
	}

	// Release
	holdLeft = false;
	dragFrame = false;
	dragLine = false;
	dropTarget = nullptr;
	// Update
	UpdateStatusBar();
	return true;
}

#ifdef _WIN32

bool SetMenuItemEnable(HMENU hMenu, WORD id, bool fEnable)
{
	return EnableMenuItem(hMenu, id, MF_BYCOMMAND | MF_ENABLED | (fEnable ? 0 : MF_GRAYED));
}

bool SetMenuItemText(HMENU hMenu, WORD id, const char *szText)
{
	std::wstring text{StdStringEncodingConverter::WinAcpToUtf16(szText)};
	MENUITEMINFO minfo{};
	minfo.cbSize = sizeof(minfo);
	minfo.fMask = MIIM_ID | MIIM_TYPE | MIIM_DATA;
	minfo.fType = MFT_STRING;
	minfo.wID = id;
	minfo.dwTypeData = text.data();
	minfo.cch = checked_cast<UINT>(text.size());
	return SetMenuItemInfo(hMenu, id, FALSE, &minfo);
}

#endif

bool C4EditCursor::RightButtonUp()
{
	target = nullptr;

	if(dragViewport)
	{
		dragViewport = false;
	}
	else if(!holdLeft)
	{
		DoContextMenu();
	}

	// Update
	UpdateStatusBar();
	// Release
	holdRight = false;
	return true;
}

void C4EditCursor::MiddleButtonUp()
{
	if (holdLeft) return;

	ApplyToolPicker();
}

bool C4EditCursor::Delete()
{
	if (!EditingOK()) return false;
	EMMoveObject(EMMO_Remove, 0, 0, nullptr, &selection);
	if (Game.Control.isCtrlHost())
	{
		OnSelectionChanged();
	}
	return true;
}

bool C4EditCursor::OpenPropTools()
{
	switch (mode)
	{
	case ConsoleMode::Edit:
		Console.PropertyDlg.Open();
		Console.PropertyDlg.Update(selection);
		break;
	case ConsoleMode::Draw:
		Console.ToolsDlg.Open();
		break;
	}
	return true;
}

bool C4EditCursor::Duplicate()
{
	EMMoveObject(EMMO_Duplicate, 0, 0, nullptr, &selection);
	return true;
}

void C4EditCursor::Draw(C4FacetEx &cgo)
{
	// Draw selection marks
	C4Object *cobj; C4ObjectLink *clnk; C4Facet frame;
	for (clnk = selection.First; clnk && (cobj = clnk->Obj); clnk = clnk->Next)
	{
		// target pos (parallax)
		int32_t cotx = cgo.TargetX, coty = cgo.TargetY; cobj->TargetPos(cotx, coty, cgo);
		frame.Set(cgo.Surface,
			cobj->x + cobj->Shape.x + cgo.X - cotx,
			cobj->y + cobj->Shape.y + cgo.Y - coty,
			cobj->Shape.Wdt,
			cobj->Shape.Hgt);
		DrawSelectMark(frame);
		// highlight selection if shift is pressed
		if (Application.IsShiftDown())
		{
			uint32_t dwOldMod = cobj->ColorMod;
			uint32_t dwOldBlitMode = cobj->BlitMode;
			cobj->ColorMod = 0xffffff;
			cobj->BlitMode = C4GFXBLIT_CLRSFC_MOD2 | C4GFXBLIT_ADDITIVE;
			cobj->Draw(cgo, -1);
			cobj->DrawTopFace(cgo, -1);
			cobj->ColorMod = dwOldMod;
			cobj->BlitMode = dwOldBlitMode;
		}
	}
	// Highlight target on hover.
	if(target)
	{
		const std::uint32_t dwOldMod {target->ColorMod};
		const std::uint32_t dwOldBlitMode {target->BlitMode};
		target->ColorMod = 0x5555aa;
		target->BlitMode = C4GFXBLIT_CLRSFC_MOD2 | C4GFXBLIT_ADDITIVE;
		target->Draw(cgo, -1);
		target->DrawTopFace(cgo, -1);
		target->ColorMod = dwOldMod;
		target->BlitMode = dwOldBlitMode;
	}
	// Draw drag frame
	if (dragFrame && mode != ConsoleMode::Draw)
	{
		Application.DDraw->DrawFrame(cgo.Surface, (std::min)(X, X2) + cgo.X - cgo.TargetX, (std::min)(Y, Y2) + cgo.Y - cgo.TargetY, (std::max)(X, X2) + cgo.X - cgo.TargetX, (std::max)(Y, Y2) + cgo.Y - cgo.TargetY, CWhite);
	}
	// Draw drop target
	if (dropTarget)
	{
		Game.GraphicsResource.fctDropTarget.Draw(cgo.Surface,
			dropTarget->x +                       cgo.X - cgo.TargetX - Game.GraphicsResource.fctDropTarget.Wdt / 2,
			dropTarget->y + dropTarget->Shape.y + cgo.Y - cgo.TargetY - Game.GraphicsResource.fctDropTarget.Hgt);
	}
	if(mode == ConsoleMode::Draw)
	{
		// Draw cursor outline for Brush and Line
		C4ToolsDlg *pTools = &Console.ToolsDlg;
		if(pTools)
		{
			std::int32_t screenPosX1 {viewSpaceX};
			std::int32_t screenPosY1 {viewSpaceY};
			std::int32_t radius {pTools->grade};
			const std::int32_t viewportOffsetX {X - viewSpaceX};
			const std::int32_t viewportOffsetY {Y - viewSpaceY};
			std::int32_t screenPosX2 {X2 - viewportOffsetX};
			std::int32_t screenPosY2 {Y2 - viewportOffsetY};
			const std::int32_t zoom {Game.Landscape.MapZoom};
			if(Game.Landscape.Mode != C4LSC_Exact)
			{
				// Snap circle to map grid.
				screenPosX1 = std::round(X / zoom) * zoom - (viewportOffsetX);
				screenPosY1 = std::round(Y / zoom) * zoom - (viewportOffsetY);

				screenPosX2 = std::round(X2 / zoom) * zoom - (viewportOffsetX);
				screenPosY2 = std::round(Y2 / zoom) * zoom - (viewportOffsetY);

				radius = pTools->grade + ((pTools->grade / (zoom / 2)) - 1) * (zoom / 2);

				if(Console.ToolsDlg.selectedTool == ToolMode::Brush || Console.ToolsDlg.selectedTool == ToolMode::Line)
				{
					// Drawn circles are offset to the left when the radius is greater than one grid unit.
					screenPosX1 += (radius > zoom / 2 ? 0 : zoom / 2);
					screenPosX2 += (radius > zoom / 2 ? 0 : zoom / 2);

					// Circles always use the center of the grid on the Y axis.
					screenPosY1 += zoom / 2;
					screenPosY2 += zoom / 2;
				}
			}
			switch (Console.ToolsDlg.selectedTool)
			{
			case ToolMode::Brush:
				Application.DDraw->DrawCircleOutline(cgo.Surface, screenPosX1, screenPosY1, radius, CWhite);
				break;
			case ToolMode::Line:
				if(dragLine)
				{
					Application.DDraw->DrawCapsuleOutline(cgo.Surface, screenPosX1, screenPosY1, screenPosX2, screenPosY2, radius, CWhite);
				}
				else
				{
					Application.DDraw->DrawCircleOutline(cgo.Surface, screenPosX1, screenPosY1, radius, CWhite);
				}
				break;
			case ToolMode::Rect:
				if(dragFrame)
				{
					if(Game.Landscape.Mode != C4LSC_Exact)
					{
						screenPosX1 = std::round((X < X2 ? X : X2) / zoom) * zoom - (viewportOffsetX);
						screenPosY1 = std::round((Y < Y2 ? Y : Y2) / zoom) * zoom - (viewportOffsetY);
						screenPosX2 = std::round((X > X2 ? X : X2) / zoom + 1) * zoom - (viewportOffsetX);
						screenPosY2 = std::round((Y > Y2 ? Y : Y2) / zoom + 1) * zoom - (viewportOffsetY);
					}
					Application.DDraw->DrawFrame(cgo.Surface, screenPosX1, screenPosY1, screenPosX2, screenPosY2, CWhite);
				}
				else
				{
					std::int32_t boxSize {1};
					if(Game.Landscape.Mode != C4LSC_Exact)
					{
						boxSize = zoom;
					}
					Application.DDraw->DrawFrame(cgo.Surface, screenPosX1, screenPosY1, screenPosX1 + boxSize, screenPosY1 + boxSize, CWhite);
				}
				break;
			default:
				break;
			}
		}
	}
}

void C4EditCursor::DrawSelectMark(C4Facet &cgo)
{
	if ((cgo.Wdt < 1) || (cgo.Hgt < 1)) return;

	if (!cgo.Surface) return;

	Application.DDraw->DrawPix(cgo.Surface, static_cast<float>(cgo.X),     static_cast<float>(cgo.Y),     0xFFFFFF);
	Application.DDraw->DrawPix(cgo.Surface, static_cast<float>(cgo.X + 1), static_cast<float>(cgo.Y),     0xFFFFFF);
	Application.DDraw->DrawPix(cgo.Surface, static_cast<float>(cgo.X),     static_cast<float>(cgo.Y + 1), 0xFFFFFF);

	Application.DDraw->DrawPix(cgo.Surface, static_cast<float>(cgo.X),     static_cast<float>(cgo.Y + cgo.Hgt - 1), 0xFFFFFF);
	Application.DDraw->DrawPix(cgo.Surface, static_cast<float>(cgo.X + 1), static_cast<float>(cgo.Y + cgo.Hgt - 1), 0xFFFFFF);
	Application.DDraw->DrawPix(cgo.Surface, static_cast<float>(cgo.X),     static_cast<float>(cgo.Y + cgo.Hgt - 2), 0xFFFFFF);

	Application.DDraw->DrawPix(cgo.Surface, static_cast<float>(cgo.X + cgo.Wdt - 1), static_cast<float>(cgo.Y),     0xFFFFFF);
	Application.DDraw->DrawPix(cgo.Surface, static_cast<float>(cgo.X + cgo.Wdt - 2), static_cast<float>(cgo.Y),     0xFFFFFF);
	Application.DDraw->DrawPix(cgo.Surface, static_cast<float>(cgo.X + cgo.Wdt - 1), static_cast<float>(cgo.Y + 1), 0xFFFFFF);

	Application.DDraw->DrawPix(cgo.Surface, static_cast<float>(cgo.X + cgo.Wdt - 1), static_cast<float>(cgo.Y + cgo.Hgt - 1), 0xFFFFFF);
	Application.DDraw->DrawPix(cgo.Surface, static_cast<float>(cgo.X + cgo.Wdt - 2), static_cast<float>(cgo.Y + cgo.Hgt - 1), 0xFFFFFF);
	Application.DDraw->DrawPix(cgo.Surface, static_cast<float>(cgo.X + cgo.Wdt - 1), static_cast<float>(cgo.Y + cgo.Hgt - 2), 0xFFFFFF);
}

void C4EditCursor::MoveSelection(int32_t iXOff, int32_t iYOff)
{
	EMMoveObject(EMMO_Move, iXOff, iYOff, nullptr, &selection);
}

void C4EditCursor::FrameSelection()
{
	C4Object *cobj; C4ObjectLink *clnk;
	for (clnk = Game.Objects.First; clnk && (cobj = clnk->Obj); clnk = clnk->Next)
		if (cobj->Status) if (cobj->OCF & OCF_NotContained)
		{
			// Contained as in: "Part of selection"
			if(selection.IsContained(cobj))
			{
				continue;
			}
			if (Inside(cobj->x, (std::min)(X, X2), (std::max)(X, X2)) && Inside(cobj->y, (std::min)(Y, Y2), (std::max)(Y, Y2)))
				selection.Add(cobj, C4ObjectList::stNone);
		}
	Console.PropertyDlg.Update(selection);
}

bool C4EditCursor::In(const char *szText)
{
	EMMoveObject(EMMO_Script, 0, 0, nullptr, &selection, szText);
	return true;
}

void C4EditCursor::Default()
{
	fAltWasDown = false;
	mode = ConsoleMode::Play;
	X = Y = X2 = Y2 = viewSpaceX = viewSpaceY = 0;
	target = dropTarget = nullptr;
#ifdef _WIN32
	hMenu = nullptr;
#endif
	holdLeft = holdRight = dragFrame = dragLine = dragViewport = false;
	selection.Default();
	fSelectionChanged = false;
}

void C4EditCursor::Clear()
{
#ifdef _WIN32
	if (hMenu) DestroyMenu(hMenu); hMenu = nullptr;
#endif
	selection.Clear();
}

bool C4EditCursor::SetMode(ConsoleMode iMode)
{
	// Store focus
#ifdef _WIN32
	HWND hFocus = GetFocus();
#endif
	// Update console buttons (always)
	Console.UpdateModeCtrls(iMode);
	// No change
	if (iMode == mode) return true;
	// Set mode
	mode = iMode;
	// Update prop tools by mode
	switch (mode)
	{
	case ConsoleMode::Play:
		if (Console.ToolsDlg.Active)
		{
			Console.ToolsDlg.Clear();
		}
		if(Console.PropertyDlg.Active)
		{
			Console.PropertyDlg.Clear();
		}
		break;
	case ConsoleMode::Edit:
		Console.ToolsDlg.Clear();
		OpenPropTools();
		break;
	case ConsoleMode::Draw:
		Console.PropertyDlg.Clear();
		OpenPropTools();
		Console.ToolsDlg.ChangeGrade(0); // Refresh Grade to account for map zoom.
		break;
	}
	// Update cursor
	if (mode == ConsoleMode::Play) Game.MouseControl.ShowCursor();
	else Game.MouseControl.HideCursor();
	// Restore focus
#ifdef _WIN32
	SetFocus(hFocus);
#endif
	// Done
	return true;
}

bool C4EditCursor::ToggleMode()
{
	if (!EditingOK()) return false;

	// Step through modes
	ConsoleMode iNewMode;
	switch (mode)
	{
	case ConsoleMode::Play: iNewMode = ConsoleMode::Edit; break;
	case ConsoleMode::Edit: iNewMode = ConsoleMode::Draw; break;
	case ConsoleMode::Draw: iNewMode = ConsoleMode::Play; break;
	default:             iNewMode = ConsoleMode::Play; break;
	}

	// Set new mode
	SetMode(iNewMode);

	return true;
}

void C4EditCursor::ApplyToolBrush()
{
	if (!EditingOK()) return;
	C4ToolsDlg *pTools = &Console.ToolsDlg;
	// execute/send control
	EMControl(CID_EMDrawTool, new C4ControlEMDrawTool(EMDT_Brush, Game.Landscape.Mode, X, Y, 0, 0, pTools->grade, !!pTools->modeIft, pTools->material, pTools->texture));
}

void C4EditCursor::ApplyToolLine()
{
	if (!EditingOK()) return;
	C4ToolsDlg *pTools = &Console.ToolsDlg;
	// execute/send control
	EMControl(CID_EMDrawTool, new C4ControlEMDrawTool(EMDT_Line, Game.Landscape.Mode, X, Y, X2, Y2, pTools->grade, !!pTools->modeIft, pTools->material, pTools->texture));
}

void C4EditCursor::ApplyToolRect()
{
	if (!EditingOK()) return;
	C4ToolsDlg *pTools = &Console.ToolsDlg;
	// execute/send control
	EMControl(CID_EMDrawTool, new C4ControlEMDrawTool(EMDT_Rect, Game.Landscape.Mode, X, Y, X2, Y2, pTools->grade, !!pTools->modeIft, pTools->material, pTools->texture));
}

void C4EditCursor::ApplyToolFill()
{
	if (!EditingOK()) return;
	C4ToolsDlg *pTools = &Console.ToolsDlg;
	// execute/send control
	EMControl(CID_EMDrawTool, new C4ControlEMDrawTool(EMDT_Fill, Game.Landscape.Mode, X, Y, 0, Y2, pTools->grade, false, pTools->material));
}

bool C4EditCursor::DoContextMenu()
{
	bool fObjectSelected = selection.ObjectCount();
#ifdef _WIN32
	POINT point; GetCursorPos(&point);
	HMENU hContext = GetSubMenu(hMenu, 0);
	SetMenuItemEnable(hContext, IDM_VIEWPORT_DELETE,     fObjectSelected && Console.Editing);
	SetMenuItemEnable(hContext, IDM_VIEWPORT_DUPLICATE,  fObjectSelected && Console.Editing);
	SetMenuItemEnable(hContext, IDM_VIEWPORT_CONTENTS,   fObjectSelected && selection.GetObject()->Contents.ObjectCount() && Console.Editing);
	SetMenuItemEnable(hContext, IDM_VIEWPORT_PROPERTIES, mode != ConsoleMode::Play);
	SetMenuItemText(hContext, IDM_VIEWPORT_DELETE,     LoadResStr(C4ResStrTableKey::IDS_MNU_DELETE));
	SetMenuItemText(hContext, IDM_VIEWPORT_DUPLICATE,  LoadResStr(C4ResStrTableKey::IDS_MNU_DUPLICATE));
	SetMenuItemText(hContext, IDM_VIEWPORT_CONTENTS,   LoadResStr(C4ResStrTableKey::IDS_MNU_CONTENTS));
	SetMenuItemText(hContext, IDM_VIEWPORT_PROPERTIES, LoadResStrChoice(mode == ConsoleMode::Edit, C4ResStrTableKey::IDS_CNS_PROPERTIES, C4ResStrTableKey::IDS_CNS_TOOLS));
	int32_t iItem = TrackPopupMenu(
		hContext,
		TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_NONOTIFY,
		point.x, point.y, 0,
		Console.hWindow,
		nullptr);
	switch (iItem)
	{
	case IDM_VIEWPORT_DELETE:     Delete();        break;
	case IDM_VIEWPORT_DUPLICATE:  Duplicate();     break;
	case IDM_VIEWPORT_CONTENTS:   GrabContents();  break;
	case IDM_VIEWPORT_PROPERTIES: OpenPropTools(); break;
	}
#elif defined(WITH_DEVELOPER_MODE)
	gtk_widget_set_sensitive(itemDelete,       fObjectSelected && Console.Editing);
	gtk_widget_set_sensitive(itemDuplicate,    fObjectSelected && Console.Editing);
	gtk_widget_set_sensitive(itemGrabContents, fObjectSelected && Selection.GetObject()->Contents.ObjectCount() && Console.Editing);
	gtk_widget_set_sensitive(itemProperties,   Mode != ConsoleMode::Play);

	GtkLabel *label = GTK_LABEL(gtk_bin_get_child(GTK_BIN(itemProperties)));
	if (Mode == ConsoleMode::Edit)
	{
		gtk_label_set_text(label, LoadResStrGtk(C4ResStrTableKey::IDS_CNS_PROPERTIES).c_str());
	}
	else
	{
		gtk_label_set_text(label, LoadResStrGtk(C4ResStrTableKey::IDS_CNS_TOOLS).c_str());
	}

	gtk_menu_popup_at_pointer(GTK_MENU(menuContext), nullptr);
#endif
	return true;
}

void C4EditCursor::GrabContents()
{
	// Set selection
	C4Object *pFrom;
	if (!(pFrom = selection.GetObject())) return;
	selection.Copy(pFrom->Contents);
	Console.PropertyDlg.Update(selection);
	holdLeft = true;

	// Exit all objects
	EMMoveObject(EMMO_Exit, 0, 0, nullptr, &selection);
}

void C4EditCursor::UpdateDropTarget(uint16_t wKeyFlags)
{
	C4Object *cobj; C4ObjectLink *clnk;

	dropTarget = nullptr;

	if (wKeyFlags & MK_CONTROL)
		if (selection.GetObject())
			for (clnk = Game.Objects.First; clnk && (cobj = clnk->Obj); clnk = clnk->Next)
				if (cobj->Status)
					if (!cobj->Contained)
						if (Inside<int32_t>(X - (cobj->x + cobj->Shape.x), 0, cobj->Shape.Wdt - 1))
							if (Inside<int32_t>(Y - (cobj->y + cobj->Shape.y), 0, cobj->Shape.Hgt - 1))
								if (!selection.GetLink(cobj))
								{
									dropTarget = cobj; break;
								}
}

void C4EditCursor::PutContents()
{
	if (!dropTarget) return;
	EMMoveObject(EMMO_Enter, 0, 0, dropTarget, &selection);
}

C4Object *C4EditCursor::GetTarget()
{
	return target;
}

bool C4EditCursor::EditingOK()
{
	if (!Console.Editing)
	{
		holdLeft = false;
		Console.Message(LoadResStr(C4ResStrTableKey::IDS_CNS_NONETEDIT));
		return false;
	}
	return true;
}

ConsoleMode C4EditCursor::GetMode() const
{
	return mode;
}

void C4EditCursor::ApplyToolPicker()
{
	int32_t iMaterial;
	uint8_t byIndex;
	switch (Game.Landscape.Mode)
	{
	case C4LSC_Static:
		// Material-texture from map
		if ((byIndex = Game.Landscape.GetMapIndex(X / Game.Landscape.MapZoom, Y / Game.Landscape.MapZoom)))
		{
			const C4TexMapEntry *pTex = Game.TextureMap.GetEntry(byIndex & (IFT - 1));
			if (pTex)
			{
				Console.ToolsDlg.SelectMaterial(pTex->GetMaterialName());
				Console.ToolsDlg.SelectTexture(pTex->GetTextureName());
				Console.ToolsDlg.SetIFT(byIndex & ~(IFT - 1));
			}
		}
		else
			Console.ToolsDlg.SelectMaterial(C4TLS_MatSky);
		break;
	case C4LSC_Exact:
		// Material only from landscape
		if (MatValid(iMaterial = GBackMat(X, Y)))
		{
			Console.ToolsDlg.SelectMaterial(Game.Material.Map[iMaterial].Name);
			Console.ToolsDlg.SetIFT(GBackIFT(X, Y));
		}
		else
			Console.ToolsDlg.SelectMaterial(C4TLS_MatSky);
		break;
	}
	holdLeft = false;
}

void C4EditCursor::EMMoveObject(C4ControlEMObjectAction eAction, int32_t tx, int32_t ty, C4Object *pTargetObj, const C4ObjectList *pObjs, const char *szScript)
{
	// construct object list
	int32_t iObjCnt = 0; int32_t *pObjIDs = nullptr;
	if (pObjs && (iObjCnt = pObjs->ObjectCount()))
	{
		pObjIDs = new int32_t[iObjCnt];
		// fill
		int32_t i = 0;
		for (C4ObjectLink *pLnk = pObjs->First; pLnk; pLnk = pLnk->Next, i++)
			if (pLnk->Obj && pLnk->Obj->Status)
				pObjIDs[i] = pLnk->Obj->Number;
	}

	// execute control
	EMControl(CID_EMMoveObj, new C4ControlEMMoveObject(eAction, tx, ty, pTargetObj, iObjCnt, pObjIDs, szScript, Config.Developer.ConsoleScriptStrictness));
}

void C4EditCursor::EMControl(C4PacketType eCtrlType, C4ControlPacket *pCtrl)
{
	Game.Control.DoInput(eCtrlType, pCtrl, CDT_Decide);
}

#ifdef WITH_DEVELOPER_MODE

// GTK+ callbacks

void C4EditCursor::OnDelete(GtkWidget *widget, gpointer data)
{
	static_cast<C4EditCursor *>(data)->Delete();
}

void C4EditCursor::OnDuplicate(GtkWidget *widget, gpointer data)
{
	static_cast<C4EditCursor *>(data)->Duplicate();
}

void C4EditCursor::OnGrabContents(GtkWidget *widget, gpointer data)
{
	static_cast<C4EditCursor *>(data)->GrabContents();
}

void C4EditCursor::OnProperties(GtkWidget *widget, gpointer data)
{
	static_cast<C4EditCursor *>(data)->OpenPropTools();
}

#endif

bool C4EditCursor::AltDown()
{
	// alt only has an effect in draw mode (picker)
	if (mode == ConsoleMode::Draw)
	{
		Console.ToolsDlg.SetAlternateTool();
	}
	// key not processed - allow further usages of Alt
	return false;
}

bool C4EditCursor::AltUp()
{
	if (mode == ConsoleMode::Draw)
	{
		Console.ToolsDlg.ResetAlternateTool();
	}
	// key not processed - allow further usages of Alt
	return false;
}
