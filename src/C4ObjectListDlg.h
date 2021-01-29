/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2007, Günther
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

/* A window listing all objects in the game */

#pragma once

#ifdef WITH_DEVELOPER_MODE
#include <gtk/gtkwidget.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtktreeselection.h>
#endif // WITH_DEVELOPER_MODE

#include "C4ObjectList.h"

class C4ObjectListDlg : public C4ObjectListChangeListener
{
public:
	C4ObjectListDlg();
	virtual ~C4ObjectListDlg();
	void Execute();
	void Open();
	void Update(C4ObjectList &rSelection);

	virtual void OnObjectRemove(C4ObjectList *pList, const C4ObjectList::iterator &pLnk) override;
	virtual void OnObjectAdded(C4ObjectList *pList, const C4ObjectList::iterator &pLnk) override;
	virtual void OnObjectRename(C4ObjectList *pList, const C4ObjectList::iterator &pLnk) override;

#ifdef WITH_DEVELOPER_MODE
private:
	GtkWidget *window;
	GtkWidget *treeview;
	GObject *model;
	bool updating_selection;

	static void OnDestroy(GtkWidget *widget, C4ObjectListDlg *dlg);
	static void OnSelectionChanged(GtkTreeSelection *selection, C4ObjectListDlg *dlg);
#endif // WITH_DEVELOPER_MODE
};
