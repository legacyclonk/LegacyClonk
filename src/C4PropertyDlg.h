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
	friend INT_PTR CALLBACK PropertyDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
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
