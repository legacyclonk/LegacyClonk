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

/* Handles viewport editing in console mode */

#pragma once

#include "C4ObjectList.h"
#include "C4Control.h"

#ifdef WITH_DEVELOPER_MODE
#include <gtk/gtk.h>
#endif

class C4EditCursor
{
public:
	C4EditCursor();
	~C4EditCursor();

protected:
	bool fAltWasDown;
	bool fSelectionChanged;
	int32_t Mode;
	int32_t X, Y, X2, Y2;
	bool Hold, DragFrame, DragLine;
	C4Object *Target, *DropTarget;
	C4Section *CurrentSection{nullptr};
#ifdef _WIN32
	HMENU hMenu;
#elif defined(WITH_DEVELOPER_MODE)
	GtkWidget *menuContext;

	GtkWidget *itemDelete;
	GtkWidget *itemDuplicate;
	GtkWidget *itemGrabContents;
	GtkWidget *itemProperties;
#endif // _WIN32/WITH_DEVELOPER_MODE
	C4ObjectList Selection;

public:
	void Default();
	void Clear();
	void Execute();
	void ClearPointers(C4Object *pObj);
	void ClearSectionPointers(C4Section &section);
	bool ToggleMode();
	void Draw(C4FacetEx &cgo);
	int32_t GetMode();
	C4Object *GetTarget();
	bool SetMode(int32_t iMode);
	bool In(const char *szText);
	bool Duplicate();
	bool OpenPropTools();
	bool Delete();
	bool LeftButtonUp(C4Section &section);
	bool LeftButtonDown(C4Section &section, bool fControl);
	bool RightButtonUp(C4Section &section);
	bool RightButtonDown(C4Section &section, bool fControl);
	void MiddleButtonUp(C4Section &section);
	bool Move(C4Section &section, int32_t iX, int32_t iY, uint16_t wKeyFlags);
	bool Init();
	bool EditingOK();
	C4ObjectList &GetSelection() { return Selection; }
	void SetHold(bool fToState) { Hold = fToState; }
	void OnSelectionChanged();
	bool AltDown();
	bool AltUp();

protected:
	bool UpdateStatusBar();
	void ApplyToolPicker();
	void PutContents();
	void UpdateDropTarget(uint16_t wKeyFlags);
	void GrabContents();
	bool DoContextMenu();
	void ApplyToolFill();
	void ApplyToolRect();
	void ApplyToolLine();
	void ApplyToolBrush();
	void DrawSelectMark(C4Facet &cgo);
	void FrameSelection();
	void MoveSelection(int32_t iXOff, int32_t iYOff);
	void EMMoveObject(enum C4ControlEMObjectAction eAction, int32_t tx, int32_t ty, C4Object *pTargetObj, const C4ObjectList &objects, const char *szScript = nullptr);
	void EMControl(enum C4PacketType eCtrlType, class C4ControlPacket *pCtrl);
	void UpdateCurrentSection(C4Section &section);
	void ResetSectionDependentState();

#ifdef WITH_DEVELOPER_MODE
	static void OnDelete(GtkWidget *widget, gpointer data);
	static void OnDuplicate(GtkWidget *widget, gpointer data);
	static void OnGrabContents(GtkWidget *widget, gpointer data);
	static void OnProperties(GtkWidget *widget, gpointer data);
#endif
};
