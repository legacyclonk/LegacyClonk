/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
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

/* Handles viewport editing in console mode */

#include <C4EditCursor.h>

#include <C4Console.h>
#include <C4Object.h>
#include <C4Application.h>
#include <C4Random.h>
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
	if (fAltIsDown != fAltWasDown) if (fAltWasDown = fAltIsDown) AltDown(); else AltUp();
	// drawing
	switch (Mode)
	{
	case C4CNS_ModeEdit:
		// Hold selection
		if (Hold)
			EMMoveObject(EMMO_Move, 0, 0, nullptr, &Selection);
		break;

	case C4CNS_ModeDraw:
		switch (Console.ToolsDlg.Tool)
		{
		case C4TLS_Fill:
			if (Hold) if (!Game.HaltCount) if (Console.Editing) ApplyToolFill();
			break;
		}
		break;
	}
	// selection update
	if (fSelectionChanged)
	{
		fSelectionChanged = false;
		UpdateStatusBar();
		Console.PropertyDlg.Update(Selection);
		Console.ObjectListDlg.Update(Selection);
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
	Console.UpdateModeCtrls(Mode);

	// FIXME
	Section = Game.Sections.front().get();

	return true;
}

void C4EditCursor::ClearPointers(C4Object *pObj)
{
	if (Target == pObj) Target = nullptr;
	if (Selection.ClearPointers(pObj))
		OnSelectionChanged();
}

bool C4EditCursor::Move(int32_t iX, int32_t iY, uint16_t wKeyFlags)
{
	// Offset movement
	int32_t xoff = iX - X; int32_t yoff = iY - Y; X = iX; Y = iY;

	switch (Mode)
	{
	case C4CNS_ModeEdit:
		// Hold
		if (!DragFrame && Hold)
		{
			MoveSelection(xoff, yoff);
			UpdateDropTarget(wKeyFlags);
		}
		// Update target
		// Shift always indicates a target outside the current selection
		else
		{
			Target = ((wKeyFlags & MK_SHIFT) && Selection.Last) ? Selection.Last->Obj : nullptr;
			do
			{
				Target = Section->FindObject(0, X, Y, 0, 0, OCF_NotContained, nullptr, nullptr, nullptr, nullptr, ANY_OWNER, Target);
			} while ((wKeyFlags & MK_SHIFT) && Target && Selection.GetLink(Target));
		}
		break;

	case C4CNS_ModeDraw:
		switch (Console.ToolsDlg.Tool)
		{
		case C4TLS_Brush:
			if (Hold) ApplyToolBrush();
			break;
		case C4TLS_Line: case C4TLS_Rect:
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
	switch (Mode)
	{
	case C4CNS_ModePlay:
		if (Game.MouseControl.GetCaption())
		{
			std::string caption{Game.MouseControl.GetCaption()};
			text = std::move(caption).substr(0, caption.find('|'));
		}
		break;

	case C4CNS_ModeEdit:
		text = std::format("{}/{} ({})", X, Y, (Target ? (Target->GetName()) : LoadResStr(C4ResStrTableKey::IDS_CNS_NOTHING)));
		break;

	case C4CNS_ModeDraw:
		text = std::format("{}/{} ({})", X, Y, (Section->MatValid(Section->Landscape.GetMat(X, Y)) ? Section->Material.Map[Section->Landscape.GetMat(X, Y)].Name : LoadResStr(C4ResStrTableKey::IDS_CNS_NOTHING)));
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
	Hold = true;

	switch (Mode)
	{
	case C4CNS_ModeEdit:
		if (fControl)
		{
			// Toggle target
			if (Target)
				if (!Selection.Remove(Target))
					Selection.Add(Target, C4ObjectList::stNone);
		}
		else
		{
			// Click on unselected: select single
			if (Target && !Selection.GetLink(Target))
			{
				Selection.Clear(); Selection.Add(Target, C4ObjectList::stNone);
			}
			// Click on nothing: drag frame
			if (!Target)
			{
				Selection.Clear(); DragFrame = true; X2 = X; Y2 = Y;
			}
		}
		break;

	case C4CNS_ModeDraw:
		switch (Console.ToolsDlg.Tool)
		{
		case C4TLS_Brush: ApplyToolBrush(); break;
		case C4TLS_Line: DragLine  = true; X2 = X; Y2 = Y; break;
		case C4TLS_Rect: DragFrame = true; X2 = X; Y2 = Y; break;
		case C4TLS_Fill:
			if (Game.HaltCount)
			{
				Hold = false; Console.Message(LoadResStr(C4ResStrTableKey::IDS_CNS_FILLNOHALT)); return false;
			}
			break;
		case C4TLS_Picker: ApplyToolPicker(); break;
		}
		break;
	}

	DropTarget = nullptr;

	OnSelectionChanged();
	return true;
}

bool C4EditCursor::RightButtonDown(bool fControl)
{
	switch (Mode)
	{
	case C4CNS_ModeEdit:
		if (!fControl)
		{
			// Check whether cursor is on anything in the selection
			bool fCursorIsOnSelection = false;
			for (C4ObjectLink *pLnk = Selection.First; pLnk; pLnk = pLnk->Next)
				if (pLnk->Obj->At(X, Y))
				{
					fCursorIsOnSelection = true;
					break;
				}
			if (!fCursorIsOnSelection)
			{
				// Click on unselected
				if (Target && !Selection.GetLink(Target))
				{
					Selection.Clear(); Selection.Add(Target, C4ObjectList::stNone);
				}
				// Click on nothing
				if (!Target) Selection.Clear();
			}
		}
		break;
	}

	OnSelectionChanged();
	return true;
}

bool C4EditCursor::LeftButtonUp()
{
	// Finish edit/tool
	switch (Mode)
	{
	case C4CNS_ModeEdit:
		if (DragFrame) FrameSelection();
		if (DropTarget) PutContents();
		break;

	case C4CNS_ModeDraw:
		switch (Console.ToolsDlg.Tool)
		{
		case C4TLS_Line:
			if (DragLine) ApplyToolLine();
			break;
		case C4TLS_Rect:
			if (DragFrame) ApplyToolRect();
			break;
		}
		break;
	}

	// Release
	Hold = false;
	DragFrame = false;
	DragLine = false;
	DropTarget = nullptr;
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
	Target = nullptr;

	DoContextMenu();

	// Update
	UpdateStatusBar();
	return true;
}

void C4EditCursor::MiddleButtonUp()
{
	if (Hold) return;

	ApplyToolPicker();
}

bool C4EditCursor::Delete()
{
	if (!EditingOK()) return false;
	EMMoveObject(EMMO_Remove, 0, 0, nullptr, &Selection);
	if (Game.Control.isCtrlHost())
	{
		OnSelectionChanged();
	}
	return true;
}

bool C4EditCursor::OpenPropTools()
{
	switch (Mode)
	{
	case C4CNS_ModeEdit: case C4CNS_ModePlay:
		Console.PropertyDlg.Open();
		Console.PropertyDlg.Update(Selection);
		break;
	case C4CNS_ModeDraw:
		Console.ToolsDlg.Open();
		break;
	}
	return true;
}

bool C4EditCursor::Duplicate()
{
	EMMoveObject(EMMO_Duplicate, 0, 0, nullptr, &Selection);
	return true;
}

void C4EditCursor::Draw(C4FacetEx &cgo)
{
	// Draw selection marks
	C4Object *cobj; C4ObjectLink *clnk; C4Facet frame;
	for (clnk = Selection.First; clnk && (cobj = clnk->Obj); clnk = clnk->Next)
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
	// Draw drag frame
	if (DragFrame)
		Application.DDraw->DrawFrame(cgo.Surface, (std::min)(X, X2) + cgo.X - cgo.TargetX, (std::min)(Y, Y2) + cgo.Y - cgo.TargetY, (std::max)(X, X2) + cgo.X - cgo.TargetX, (std::max)(Y, Y2) + cgo.Y - cgo.TargetY, CWhite);
	// Draw drag line
	if (DragLine)
		Application.DDraw->DrawLine(cgo.Surface, X + cgo.X - cgo.TargetX, Y + cgo.Y - cgo.TargetY, X2 + cgo.X - cgo.TargetX, Y2 + cgo.Y - cgo.TargetY, CWhite);
	// Draw drop target
	if (DropTarget)
		Game.GraphicsResource.fctDropTarget.Draw(cgo.Surface,
			DropTarget->x +                       cgo.X - cgo.TargetX - Game.GraphicsResource.fctDropTarget.Wdt / 2,
			DropTarget->y + DropTarget->Shape.y + cgo.Y - cgo.TargetY - Game.GraphicsResource.fctDropTarget.Hgt);
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
	EMMoveObject(EMMO_Move, iXOff, iYOff, nullptr, &Selection);
}

void C4EditCursor::FrameSelection()
{
	Selection.Clear();
	C4Object *cobj; C4ObjectLink *clnk;
	for (clnk = Section->Objects.First; clnk && (cobj = clnk->Obj); clnk = clnk->Next)
		if (cobj->Status) if (cobj->OCF & OCF_NotContained)
		{
			if (Inside(cobj->x, (std::min)(X, X2), (std::max)(X, X2)) && Inside(cobj->y, (std::min)(Y, Y2), (std::max)(Y, Y2)))
				Selection.Add(cobj, C4ObjectList::stNone);
		}
	Console.PropertyDlg.Update(Selection);
}

bool C4EditCursor::In(const char *szText)
{
	EMMoveObject(EMMO_Script, 0, 0, nullptr, &Selection, szText);
	return true;
}

void C4EditCursor::Default()
{
	fAltWasDown = false;
	Mode = C4CNS_ModePlay;
	X = Y = X2 = Y2 = 0;
	Target = DropTarget = nullptr;
#ifdef _WIN32
	hMenu = nullptr;
#endif
	Hold = DragFrame = DragLine = false;
	Selection.Default();
	fSelectionChanged = false;
	Section = nullptr;
}

void C4EditCursor::Clear()
{
#ifdef _WIN32
	if (hMenu) DestroyMenu(hMenu); hMenu = nullptr;
#endif
	Selection.Clear();
}

bool C4EditCursor::SetMode(int32_t iMode)
{
	// Store focus
#ifdef _WIN32
	HWND hFocus = GetFocus();
#endif
	// Update console buttons (always)
	Console.UpdateModeCtrls(iMode);
	// No change
	if (iMode == Mode) return true;
	// Set mode
	Mode = iMode;
	// Update prop tools by mode
	bool fOpenPropTools = false;
	switch (Mode)
	{
	case C4CNS_ModeEdit: case C4CNS_ModePlay:
		if (Console.ToolsDlg.Active || Console.PropertyDlg.Active) fOpenPropTools = true;
		Console.ToolsDlg.Clear();
		if (fOpenPropTools) OpenPropTools();
		break;

	case C4CNS_ModeDraw:
		if (Console.ToolsDlg.Active || Console.PropertyDlg.Active) fOpenPropTools = true;
		Console.PropertyDlg.Clear();
		if (fOpenPropTools) OpenPropTools();
		break;
	}
	// Update cursor
	if (Mode == C4CNS_ModePlay) Game.MouseControl.ShowCursor();
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
	int32_t iNewMode;
	switch (Mode)
	{
	case C4CNS_ModePlay: iNewMode = C4CNS_ModeEdit; break;
	case C4CNS_ModeEdit: iNewMode = C4CNS_ModeDraw; break;
	case C4CNS_ModeDraw: iNewMode = C4CNS_ModePlay; break;
	default:             iNewMode = C4CNS_ModePlay; break;
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
	EMControl(CID_EMDrawTool, new C4ControlEMDrawTool(EMDT_Brush, Section->Landscape.Mode, X, Y, 0, 0, pTools->Grade, !!pTools->ModeIFT, Section->Number, pTools->Material, pTools->Texture));
}

void C4EditCursor::ApplyToolLine()
{
	if (!EditingOK()) return;
	C4ToolsDlg *pTools = &Console.ToolsDlg;
	// execute/send control
	EMControl(CID_EMDrawTool, new C4ControlEMDrawTool(EMDT_Line, Section->Landscape.Mode, X, Y, X2, Y2, pTools->Grade, !!pTools->ModeIFT, Section->Number, pTools->Material, pTools->Texture));
}

void C4EditCursor::ApplyToolRect()
{
	if (!EditingOK()) return;
	C4ToolsDlg *pTools = &Console.ToolsDlg;
	// execute/send control
	EMControl(CID_EMDrawTool, new C4ControlEMDrawTool(EMDT_Rect, Section->Landscape.Mode, X, Y, X2, Y2, pTools->Grade, !!pTools->ModeIFT, Section->Number, pTools->Material, pTools->Texture));
}

void C4EditCursor::ApplyToolFill()
{
	if (!EditingOK()) return;
	C4ToolsDlg *pTools = &Console.ToolsDlg;
	// execute/send control
	EMControl(CID_EMDrawTool, new C4ControlEMDrawTool(EMDT_Fill, Section->Landscape.Mode, X, Y, 0, Y2, pTools->Grade, false, Section->Number, pTools->Material));
}

bool C4EditCursor::DoContextMenu()
{
	bool fObjectSelected = Selection.ObjectCount();
#ifdef _WIN32
	POINT point; GetCursorPos(&point);
	HMENU hContext = GetSubMenu(hMenu, 0);
	SetMenuItemEnable(hContext, IDM_VIEWPORT_DELETE,     fObjectSelected && Console.Editing);
	SetMenuItemEnable(hContext, IDM_VIEWPORT_DUPLICATE,  fObjectSelected && Console.Editing);
	SetMenuItemEnable(hContext, IDM_VIEWPORT_CONTENTS,   fObjectSelected && Selection.GetObject()->Contents.ObjectCount() && Console.Editing);
	SetMenuItemEnable(hContext, IDM_VIEWPORT_PROPERTIES, Mode != C4CNS_ModePlay);
	SetMenuItemText(hContext, IDM_VIEWPORT_DELETE,     LoadResStr(C4ResStrTableKey::IDS_MNU_DELETE));
	SetMenuItemText(hContext, IDM_VIEWPORT_DUPLICATE,  LoadResStr(C4ResStrTableKey::IDS_MNU_DUPLICATE));
	SetMenuItemText(hContext, IDM_VIEWPORT_CONTENTS,   LoadResStr(C4ResStrTableKey::IDS_MNU_CONTENTS));
	SetMenuItemText(hContext, IDM_VIEWPORT_PROPERTIES, LoadResStrChoice(Mode == C4CNS_ModeEdit, C4ResStrTableKey::IDS_CNS_PROPERTIES, C4ResStrTableKey::IDS_CNS_TOOLS));
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
	gtk_widget_set_sensitive(itemProperties,   Mode != C4CNS_ModePlay);

	GtkLabel *label = GTK_LABEL(gtk_bin_get_child(GTK_BIN(itemProperties)));
	if (Mode == C4CNS_ModeEdit)
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
	if (!(pFrom = Selection.GetObject())) return;
	Selection.Copy(pFrom->Contents);
	Console.PropertyDlg.Update(Selection);
	Hold = true;

	// Exit all objects
	EMMoveObject(EMMO_Exit, 0, 0, nullptr, &Selection);
}

void C4EditCursor::UpdateDropTarget(uint16_t wKeyFlags)
{
	C4Object *cobj; C4ObjectLink *clnk;

	DropTarget = nullptr;

	if (wKeyFlags & MK_CONTROL)
		if (Selection.GetObject())
			for (clnk = Section->Objects.First; clnk && (cobj = clnk->Obj); clnk = clnk->Next)
				if (cobj->Status)
					if (!cobj->Contained)
						if (Inside<int32_t>(X - (cobj->x + cobj->Shape.x), 0, cobj->Shape.Wdt - 1))
							if (Inside<int32_t>(Y - (cobj->y + cobj->Shape.y), 0, cobj->Shape.Hgt - 1))
								if (!Selection.GetLink(cobj))
								{
									DropTarget = cobj; break;
								}
}

void C4EditCursor::PutContents()
{
	if (!DropTarget) return;
	EMMoveObject(EMMO_Enter, 0, 0, DropTarget, &Selection);
}

C4Object *C4EditCursor::GetTarget()
{
	return Target;
}

bool C4EditCursor::EditingOK()
{
	if (!Console.Editing)
	{
		Hold = false;
		Console.Message(LoadResStr(C4ResStrTableKey::IDS_CNS_NONETEDIT));
		return false;
	}
	return true;
}

int32_t C4EditCursor::GetMode()
{
	return Mode;
}

void C4EditCursor::ApplyToolPicker()
{
	int32_t iMaterial;
	uint8_t byIndex;
	switch (Section->Landscape.Mode)
	{
	case C4LSC_Static:
		// Material-texture from map
		if (byIndex = Section->Landscape.GetMapIndex(X /Section->Landscape.MapZoom, Y / Section->Landscape.MapZoom))
		{
			const C4TexMapEntry *pTex = Section->TextureMap.GetEntry(byIndex & (IFT - 1));
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
		if (Section->MatValid(iMaterial = Section->Landscape.GetMat(X, Y)))
		{
			Console.ToolsDlg.SelectMaterial(Section->Material.Map[iMaterial].Name);
			Console.ToolsDlg.SetIFT(Section->Landscape.GBackIFT(X, Y));
		}
		else
			Console.ToolsDlg.SelectMaterial(C4TLS_MatSky);
		break;
	}
	Hold = false;
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
	EMControl(CID_EMMoveObj, new C4ControlEMMoveObject(eAction, tx, ty, Section->Number, pTargetObj, iObjCnt, pObjIDs, szScript, Config.Developer.ConsoleScriptStrictness));
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
	if (Mode == C4CNS_ModeDraw)
	{
		Console.ToolsDlg.SetAlternateTool();
	}
	// key not processed - allow further usages of Alt
	return false;
}

bool C4EditCursor::AltUp()
{
	if (Mode == C4CNS_ModeDraw)
	{
		Console.ToolsDlg.ResetAlternateTool();
	}
	// key not processed - allow further usages of Alt
	return false;
}
