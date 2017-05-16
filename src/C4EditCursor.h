/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Handles viewport editing in console mode */

#pragma once

#include "C4ObjectList.h"
#include "C4Control.h"

#ifdef WITH_DEVELOPER_MODE
#include <gtk/gtkwidget.h>
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
	BOOL Hold, DragFrame, DragLine;
	C4Object *Target, *DropTarget;
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
	bool ToggleMode();
	void Draw(C4FacetEx &cgo);
	int32_t GetMode();
	C4Object *GetTarget();
	BOOL SetMode(int32_t iMode);
	BOOL In(const char *szText);
	BOOL Duplicate();
	BOOL OpenPropTools();
	bool Delete();
	BOOL LeftButtonUp();
	BOOL LeftButtonDown(BOOL fControl);
	BOOL RightButtonUp();
	BOOL RightButtonDown(BOOL fControl);
	bool Move(int32_t iX, int32_t iY, WORD wKeyFlags);
	BOOL Init();
	BOOL EditingOK();
	C4ObjectList &GetSelection() { return Selection; }
	void SetHold(BOOL fToState) { Hold = fToState; }
	void OnSelectionChanged();
	bool AltDown();
	bool AltUp();

protected:
	BOOL UpdateStatusBar();
	void ApplyToolPicker();
	void PutContents();
	void UpdateDropTarget(WORD wKeyFlags);
	void GrabContents();
	BOOL DoContextMenu();
	void ApplyToolFill();
	void ApplyToolRect();
	void ApplyToolLine();
	void ApplyToolBrush();
	void DrawSelectMark(C4Facet &cgo);
	void FrameSelection();
	void MoveSelection(int32_t iXOff, int32_t iYOff);
	void EMMoveObject(enum C4ControlEMObjectAction eAction, int32_t tx, int32_t ty, C4Object *pTargetObj, const C4ObjectList *pObjs = NULL, const char *szScript = NULL);
	void EMControl(enum C4PacketType eCtrlType, class C4ControlPacket *pCtrl);

#ifdef WITH_DEVELOPER_MODE
	static void OnDelete(GtkWidget *widget, gpointer data);
	static void OnDuplicate(GtkWidget *widget, gpointer data);
	static void OnGrabContents(GtkWidget *widget, gpointer data);
	static void OnProperties(GtkWidget *widget, gpointer data);
#endif
};
