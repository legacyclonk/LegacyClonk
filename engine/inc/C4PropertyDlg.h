/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Console mode dialog for object properties and script interface */

#ifndef INC_C4PropertyDlg
#define INC_C4PropertyDlg

#include "C4ObjectList.h"

#ifdef WITH_DEVELOPER_MODE
# include <gtk/gtkwidget.h>
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
		BOOL Open();
		BOOL Update();
		BOOL Update(C4ObjectList &rSelection);
		bool Active;
#ifdef _WIN32
		HWND hDialog;
	friend BOOL CALLBACK PropertyDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
#else
#ifdef WITH_DEVELOPER_MODE
//		GtkWidget* window;
		GtkWidget* vbox;
		GtkWidget* textview;
		GtkWidget* entry;

		gulong handlerHide;

		static void OnScriptActivate(GtkWidget* widget, gpointer data);
		static void OnWindowHide(GtkWidget* widget, gpointer data);
//		static void OnDestroy(GtkWidget* widget, gpointer data);
#endif
#endif
	protected:
		C4ID idSelectedDef;
		C4ObjectList Selection;
	};

#endif
