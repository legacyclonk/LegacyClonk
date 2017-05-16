/* by Günther, 2007 */

/* A window listing all objects in the game */

#ifndef INC_C4ObjectListDlg
#define INC_C4ObjectListDlg

#ifdef WITH_DEVELOPER_MODE
#include <gtk/gtkwidget.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtktreeselection.h>
#endif // WITH_DEVELOPER_MODE

#include "C4ObjectList.h"

class C4ObjectListDlg: public C4ObjectListChangeListener
{
	public:
	C4ObjectListDlg();
	virtual ~C4ObjectListDlg();
	void Execute();
	void Open();
	void Update(C4ObjectList &rSelection);

	virtual void OnObjectRemove(C4ObjectList * pList, C4ObjectLink * pLnk);
	virtual void OnObjectAdded(C4ObjectList * pList, C4ObjectLink * pLnk);
	virtual void OnObjectRename(C4ObjectList * pList, C4ObjectLink * pLnk);

#ifdef WITH_DEVELOPER_MODE
	private:
	GtkWidget * window;
	GtkWidget * treeview;
	GObject * model;
	bool updating_selection;

	static void OnDestroy(GtkWidget * widget, C4ObjectListDlg * dlg);
	static void OnSelectionChanged(GtkTreeSelection * selection, C4ObjectListDlg * dlg);
#endif // WITH_DEVELOPER_MODE
};

#endif //INC_C4ObjectListDlg
