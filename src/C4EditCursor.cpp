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

#include <C4Include.h>
#include <C4EditCursor.h>
#include <C4ToolsDlg.h>

#include <C4Console.h>
#include <C4Object.h>
#include <C4Application.h>
#include <C4Random.h>
#include <C4Wrappers.h>

#ifdef WITH_DEVELOPER_MODE
#include <C4Language.h>

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
	switch (Mode)
	{
	case ConsoleMode::Edit:
		// Hold selection
		if (Hold)
			EMMoveObject(EMMO_Move, 0, 0, nullptr, &Selection);
		break;

	case ConsoleMode::Draw:
		switch (Console.ToolsDlg.Tool)
		{
		case ToolMode::Fill:
			if (Hold) if (!Game.HaltCount) if (Console.Editing) ApplyToolFill();
			break;
		}
		break;
	}
	// selection update
	if (fSelectionChanged)
	{
		fSelectionChanged = false;
		Console.PropertyDlg.Update(Selection);
		Console.ObjectListDlg.Update(Selection);
	}
}

bool C4EditCursor::Init()
{
	// TODO
#ifdef _WIN32
//	if (!(hMenu = LoadMenu(Application.hInstance, MAKEINTRESOURCE(IDR_CONTEXTMENUS))))
		return true;
#else // _WIN32
#ifdef WITH_DEVELOPER_MODE
	menuContext = gtk_menu_new();

	itemDelete =       gtk_menu_item_new_with_label(LoadResStrUtf8("IDS_MNU_DELETE").getData());
	itemDuplicate =    gtk_menu_item_new_with_label(LoadResStrUtf8("IDS_MNU_DUPLICATE").getData());
	itemGrabContents = gtk_menu_item_new_with_label(LoadResStrUtf8("IDS_MNU_CONTENTS").getData());
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
	case ConsoleMode::Edit:
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
				Target = Game.FindObject(0, X, Y, 0, 0, OCF_NotContained, nullptr, nullptr, nullptr, nullptr, ANY_OWNER, Target);
			} while ((wKeyFlags & MK_SHIFT) && Target && Selection.GetLink(Target));
		}
		break;

	case ConsoleMode::Draw:
		switch (Console.ToolsDlg.Tool)
		{
		case ToolMode::Brush:
			if (Hold) ApplyToolBrush();
			break;
		case ToolMode::Line: case ToolMode::Rect:
			break;
		}
		break;
	}

	return true;
}

StdStrBuf C4EditCursor::GetStatusBarText() const
{
	switch (Mode)
	{
	case ConsoleMode::Play:
		if (const char *const caption{Game.MouseControl.GetCaption()}; caption && *caption)
		{
			StdStrBuf text;
			text.CopyUntil(caption, '|');
			return text;
		}
		else
		{
			return {};
		}

	case ConsoleMode::Edit:
		return StdStrBuf{std::format("{}/{} ({})", X, Y, (Target ? (Target->GetName()) : LoadResStr(C4ResStrTableKey::IDS_CNS_NOTHING))).c_str()};

	case ConsoleMode::Draw:
		return StdStrBuf{std::format("{}/{} ({})", X, Y, (MatValid(GBackMat(X, Y)) ? Game.Material.Map[GBackMat(X, Y)].Name : LoadResStr(C4ResStrTableKey::IDS_CNS_NOTHING))).c_str()};

	default:
#ifdef _MSC_VER
		__assume(0);
#else
		__builtin_unreachable();
#endif
	}
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
	case ConsoleMode::Edit:
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

	case ConsoleMode::Draw:
		switch (Console.ToolsDlg.Tool)
		{
		case ToolMode::Brush: ApplyToolBrush(); break;
		case ToolMode::Line: DragLine  = true; X2 = X; Y2 = Y; break;
		case ToolMode::Rect: DragFrame = true; X2 = X; Y2 = Y; break;
		case ToolMode::Fill:
			if (Game.HaltCount)
			{
				Hold = false; Console.Message(LoadResStr(C4ResStrTableKey::IDS_CNS_FILLNOHALT)); return false;
			}
			break;
		case ToolMode::Picker: ApplyToolPicker(); break;
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
	case ConsoleMode::Edit:
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
	case ConsoleMode::Edit:
		if (DragFrame) FrameSelection();
		if (DropTarget) PutContents();
		break;

	case ConsoleMode::Draw:
		switch (Console.ToolsDlg.Tool)
		{
		case ToolMode::Line:
			if (DragLine) ApplyToolLine();
			break;
		case ToolMode::Rect:
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
	return true;
}

#if 0 //def _WIN32

bool SetMenuItemEnable(HMENU hMenu, WORD id, bool fEnable)
{
	return EnableMenuItem(hMenu, id, MF_BYCOMMAND | MF_ENABLED | (fEnable ? 0 : MF_GRAYED));
}

bool SetMenuItemText(HMENU hMenu, WORD id, const char *szText)
{
	MENUITEMINFO minfo{};
	minfo.cbSize = sizeof(minfo);
	minfo.fMask = MIIM_ID | MIIM_TYPE | MIIM_DATA;
	minfo.fType = MFT_STRING;
	minfo.wID = id;
	minfo.dwTypeData = (char *)szText;
	minfo.cch = checked_cast<UINT>(SLen(szText));
	return SetMenuItemInfo(hMenu, id, FALSE, &minfo);
}

#endif

bool C4EditCursor::RightButtonUp()
{
	Target = nullptr;

	OpenContextMenu();

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
	case ConsoleMode::Edit: case ConsoleMode::Play:
		Console.PropertyDlg.Open();
		Console.PropertyDlg.Update(Selection);
		break;
	case ConsoleMode::Draw:
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
	for (clnk = Game.Objects.First; clnk && (cobj = clnk->Obj); clnk = clnk->Next)
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
	Mode = ConsoleMode::Play;
	X = Y = X2 = Y2 = 0;
	Target = DropTarget = nullptr;
#ifdef _WIN32
	hMenu = nullptr;
#endif
	Hold = DragFrame = DragLine = false;
	Selection.Default();
	fSelectionChanged = false;
}

void C4EditCursor::Clear()
{
#ifdef _WIN32
	if (hMenu) DestroyMenu(hMenu); hMenu = nullptr;
#endif
	Selection.Clear();
}

bool C4EditCursor::SetMode(const ConsoleMode mode)
{
	// Store focus
#ifdef _WIN32
	HWND hFocus = GetFocus();
#endif
	// No change
	if (mode == Mode) return true;
	// Set mode
	Mode = mode;
	// Update prop tools by mode
	bool fOpenPropTools = false;
	switch (Mode)
	{
	case ConsoleMode::Play: case ConsoleMode::Edit:
		if (Console.ToolsDlg.Active || Console.PropertyDlg.Active) fOpenPropTools = true;
		Console.ToolsDlg.Clear();
		if (fOpenPropTools) OpenPropTools();
		break;

	case ConsoleMode::Draw:
		if (Console.ToolsDlg.Active || Console.PropertyDlg.Active) fOpenPropTools = true;
		Console.PropertyDlg.Clear();
		if (fOpenPropTools) OpenPropTools();
		break;
	}
	// Update cursor
	if (Mode == ConsoleMode::Play) Game.MouseControl.ShowCursor();
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
	ConsoleMode newMode;
	switch (Mode)
	{
	case ConsoleMode::Play: newMode = ConsoleMode::Edit; break;
	case ConsoleMode::Edit: newMode = ConsoleMode::Draw; break;
	case ConsoleMode::Draw: newMode = ConsoleMode::Play; break;
	}

	// Set new mode
	SetMode(newMode);

	return true;
}

void C4EditCursor::ApplyToolBrush()
{
	if (!EditingOK()) return;
	C4ToolsDlg *pTools = &Console.ToolsDlg;
	// execute/send control
	EMControl(CID_EMDrawTool, new C4ControlEMDrawTool(EMDT_Brush, Game.Landscape.Mode, X, Y, 0, 0, pTools->Grade, !!pTools->ModeIFT, pTools->Material, pTools->Texture));
}

void C4EditCursor::ApplyToolLine()
{
	if (!EditingOK()) return;
	C4ToolsDlg *pTools = &Console.ToolsDlg;
	// execute/send control
	EMControl(CID_EMDrawTool, new C4ControlEMDrawTool(EMDT_Line, Game.Landscape.Mode, X, Y, X2, Y2, pTools->Grade, !!pTools->ModeIFT, pTools->Material, pTools->Texture));
}

void C4EditCursor::ApplyToolRect()
{
	if (!EditingOK()) return;
	C4ToolsDlg *pTools = &Console.ToolsDlg;
	// execute/send control
	EMControl(CID_EMDrawTool, new C4ControlEMDrawTool(EMDT_Rect, Game.Landscape.Mode, X, Y, X2, Y2, pTools->Grade, !!pTools->ModeIFT, pTools->Material, pTools->Texture));
}

void C4EditCursor::ApplyToolFill()
{
	if (!EditingOK()) return;
	C4ToolsDlg *pTools = &Console.ToolsDlg;
	// execute/send control
	EMControl(CID_EMDrawTool, new C4ControlEMDrawTool(EMDT_Fill, Game.Landscape.Mode, X, Y, 0, Y2, pTools->Grade, false, pTools->Material));
}

bool C4EditCursor::OpenContextMenu()
{
	ContextMenuOpenIn = ImGui::GetCurrentContext();
	return true;
}

void C4EditCursor::DrawContextMenu()
{
	// Make sure the context menu is opened in the same gui context as the right click happened.
	if(ContextMenuOpenIn && ImGui::GetCurrentContext() && ContextMenuOpenIn == ImGui::GetCurrentContext())
	{
		ImGui::OpenPopup("ContextMenu");
		ContextMenuOpenIn = nullptr;
	}
	if (ImGui::BeginPopup("ContextMenu"))
	{
		bool fObjectSelected = Selection.ObjectCount();
		ImGui::BeginDisabled(!fObjectSelected);
		if (ImGui::MenuItem(LoadResStr(C4ResStrTableKey::IDS_MNU_DELETE), "Delete"))
		{
			Delete();
		}

		if (ImGui::MenuItem(LoadResStr(C4ResStrTableKey::IDS_MNU_DUPLICATE), "Ctrl+D"))
		{
			Duplicate();
		}

		if (ImGui::MenuItem(LoadResStr(C4ResStrTableKey::IDS_MNU_CONTENTS)))
		{
			GrabContents();
		}

		ImGui::EndDisabled();

		ImGui::Separator();

		if (ImGui::MenuItem(LoadResStrV((Mode == ConsoleMode::Edit) ? C4ResStrTableKey::IDS_CNS_PROPERTIES : C4ResStrTableKey::IDS_CNS_TOOLS)))
		{
			OpenPropTools();
		}

		ImGui::EndPopup();
	}
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
			for (clnk = Game.Objects.First; clnk && (cobj = clnk->Obj); clnk = clnk->Next)
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

ConsoleMode C4EditCursor::GetMode() const
{
	return Mode;
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
				Console.ToolsDlg.SetMaterial(pTex->GetMaterialName());
				Console.ToolsDlg.SetTexture(pTex->GetTextureName());
				Console.ToolsDlg.SetIFT(byIndex & ~(IFT - 1));
			}
		}
		else
			Console.ToolsDlg.SetMaterial(C4TLS_MatSky);
		break;
	case C4LSC_Exact:
		// Material only from landscape
		if (MatValid(iMaterial = GBackMat(X, Y)))
		{
			Console.ToolsDlg.SetMaterial(Game.Material.Map[iMaterial].Name);
			Console.ToolsDlg.SetIFT(GBackIFT(X, Y));
		}
		else
			Console.ToolsDlg.SetMaterial(C4TLS_MatSky);
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
	EMControl(CID_EMMoveObj, new C4ControlEMMoveObject(eAction, tx, ty, pTargetObj, iObjCnt, pObjIDs, szScript));
}

void C4EditCursor::EMControl(C4PacketType eCtrlType, C4ControlPacket *pCtrl)
{
	Game.Control.DoInput(eCtrlType, pCtrl, CDT_Decide);
}

bool C4EditCursor::AltDown()
{
	// alt only has an effect in draw mode (picker)
	if (Mode == ConsoleMode::Draw)
	{
		Console.ToolsDlg.SetAlternateTool();
	}
	// key not processed - allow further usages of Alt
	return false;
}

bool C4EditCursor::AltUp()
{
	if (Mode == ConsoleMode::Draw)
	{
		Console.ToolsDlg.ResetAlternateTool();
	}
	// key not processed - allow further usages of Alt
	return false;
}
