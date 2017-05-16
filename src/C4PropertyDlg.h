/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Console mode dialog for object properties and script interface */

#pragma once

#include "C4ObjectList.h"

#ifdef WITH_DEVELOPER_MODE
#include <gtk/gtkwidget.h>
#endif

class C4PropertyDlg
{
public:
	C4PropertyDlg();
	~C4PropertyDlg();
	void Default();
	void Clear();
	void Execute();
	void ClearPointers(C4Object *pObj);
	void UpdateInputCtrl(C4Object *pObj);
	bool Open();
	bool Update();
	bool Update(C4ObjectList &rSelection);
	bool Active;
#ifdef _WIN32
	HWND hDialog;
	friend BOOL CALLBACK PropertyDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
#elif defined(WITH_DEVELOPER_MODE)
	GtkWidget *vbox;
	GtkWidget *textview;
	GtkWidget *entry;

	gulong handlerHide;

	static void OnScriptActivate(GtkWidget *widget, gpointer data);
	static void OnWindowHide(GtkWidget *widget, gpointer data);
#endif

protected:
	C4ID idSelectedDef;
	C4ObjectList Selection;
};
