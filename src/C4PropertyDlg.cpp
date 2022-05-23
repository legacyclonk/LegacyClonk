/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2022, The LegacyClonk Team and contributors
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

#include <C4Include.h>
#include <C4PropertyDlg.h>

#include <C4Console.h>
#include <C4Application.h>
#include <C4Object.h>
#include <C4Wrappers.h>
#include <C4Player.h>

#include <StdRegistry.h>

#ifdef WITH_DEVELOPER_MODE
#include <C4DevmodeDlg.h>
#include <C4Language.h>

#include <gtk/gtkentry.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtktextview.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkscrolledwindow.h>
#endif

#ifdef _WIN32
INT_PTR CALLBACK PropertyDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_CLOSE:
		Console.PropertyDlg.Clear();
		break;

	case WM_DESTROY:
		StoreWindowPosition(hDlg, "Property", Config.GetSubkeyPath("Console"), false);
		break;

	case WM_INITDIALOG:
		SendMessage(hDlg, DM_SETDEFID, IDOK, 0);
		return TRUE;

	case WM_COMMAND:
		// Evaluate command
		switch (LOWORD(wParam))
		{
		case IDOK:
			// IDC_COMBOINPUT to Console.EditCursor.In()
			char buffer[16000];
			GetDlgItemText(hDlg, IDC_COMBOINPUT, buffer, 16000);
			if (buffer[0])
				Console.EditCursor.In(buffer);
			return TRUE;

		case IDC_BUTTONRELOADDEF:
			Game.ReloadDef(Console.PropertyDlg.idSelectedDef);
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}
#endif

C4PropertyDlg::C4PropertyDlg()
{
	Default();
}

C4PropertyDlg::~C4PropertyDlg()
{
	Clear();

#ifdef WITH_DEVELOPER_MODE
	if (vbox != nullptr)
	{
		g_signal_handler_disconnect(G_OBJECT(C4DevmodeDlg::GetWindow()), handlerHide);
		C4DevmodeDlg::RemovePage(vbox);
		vbox = nullptr;
	}
#endif // WITH_DEVELOPER_MODE
}

bool C4PropertyDlg::Open()
{
#ifdef _WIN32
	if (hDialog) return true;
	hDialog = CreateDialog(Application.hInstance,
		MAKEINTRESOURCE(IDD_PROPERTIES),
		Console.hWindow,
		PropertyDlgProc);
	if (!hDialog) return false;
	// Set text
	SetWindowText(hDialog, LoadResStr("IDS_DLG_PROPERTIES"));
	// Enable controls
	EnableWindow(GetDlgItem(hDialog, IDOK), Console.Editing);
	EnableWindow(GetDlgItem(hDialog, IDC_COMBOINPUT), Console.Editing);
	EnableWindow(GetDlgItem(hDialog, IDC_BUTTONRELOADDEF), Console.Editing);
	// Show window
	RestoreWindowPosition(hDialog, "Property", Config.GetSubkeyPath("Console"));
	SetWindowPos(hDialog, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	ShowWindow(hDialog, SW_SHOWNORMAL | SW_SHOWNA);
#else // _WIN32
#ifdef WITH_DEVELOPER_MODE
	if (vbox == nullptr)
	{
		vbox = gtk_vbox_new(FALSE, 6);

		GtkWidget *scrolled_wnd = gtk_scrolled_window_new(nullptr, nullptr);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_wnd), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_wnd), GTK_SHADOW_IN);

		textview = gtk_text_view_new();
		entry = gtk_entry_new();

		gtk_container_add(GTK_CONTAINER(scrolled_wnd), textview);
		gtk_box_pack_start(GTK_BOX(vbox), scrolled_wnd, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);

		gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), FALSE);
		gtk_widget_set_sensitive(entry, Console.Editing);

		gtk_widget_show_all(vbox);

		C4DevmodeDlg::AddPage(vbox, GTK_WINDOW(Console.window), LoadResStrUtf8("IDS_DLG_PROPERTIES").getData());

		g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(OnScriptActivate), this);

		handlerHide = g_signal_connect(G_OBJECT(C4DevmodeDlg::GetWindow()), "hide", G_CALLBACK(OnWindowHide), this);
	}

	C4DevmodeDlg::SwitchPage(vbox);
#endif // WITH_DEVELOPER_MODE
#endif // _WIN32
	Active = true;
	return true;
}

bool C4PropertyDlg::Update(C4ObjectList &rSelection)
{
	if (!Active) return false;
	// Set new selection
	Selection.Copy(rSelection);
	// Update input control
	UpdateInputCtrl(Selection.GetObject());
	// Update contents
	return Update();
}

bool C4PropertyDlg::Update()
{
	if (!Active) return false;

	StdStrBuf Output;

	idSelectedDef = C4ID_None;

	// Compose info text by selected object(s)
	switch (Selection.ObjectCount())
	{
	// No selection
	case 0:
		Output.Ref(LoadResStr("IDS_CNS_NOOBJECT"));
		break;
	// One selected object
	case 1:
	{
		C4Object *cobj = Selection.GetObject();
		// Type
		Output.AppendFormat(LoadResStr("IDS_CNS_TYPE"), cobj->GetName(), C4IdText(cobj->Def->id));
		// Owner
		if (ValidPlr(cobj->Owner))
		{
			Output.Append(LineFeed);
			Output.AppendFormat(LoadResStr("IDS_CNS_OWNER"), Game.Players.Get(cobj->Owner)->GetName());
		}
		// Contents
		if (cobj->Contents.ObjectCount())
		{
			Output.Append(LineFeed);
			Output.Append(LoadResStr("IDS_CNS_CONTENTS"));
			Output.Append(static_cast<const StdStrBuf &>(cobj->Contents.GetNameList(Game.Defs)));
		}
		// Action
		if (cobj->Action.Act != ActIdle)
		{
			Output.Append(LineFeed);
			Output.Append(LoadResStr("IDS_CNS_ACTION"));
			Output.Append(cobj->Def->ActMap[cobj->Action.Act].Name);
		}
		// Locals
		int cnt; bool fFirstLocal = true;
		for (cnt = 0; cnt < cobj->Local.GetSize(); cnt++)
			if (cobj->Local[cnt])
			{
				// Header
				if (fFirstLocal) { Output.Append(LineFeed); Output.Append(LoadResStr("IDS_CNS_LOCALS")); fFirstLocal = false; }
				Output.Append(LineFeed);
				// Append id
				Output.AppendFormat(" Local(%d) = ", cnt);
				// write value
				Output.Append(static_cast<const StdStrBuf &>(cobj->Local[cnt].GetDataString()));
			}
		// Locals (named)
		for (cnt = 0; cnt < cobj->LocalNamed.GetAnzItems(); cnt++)
		{
			// Header
			if (fFirstLocal) { Output.Append(LineFeed); Output.Append(LoadResStr("IDS_CNS_LOCALS")); fFirstLocal = false; }
			Output.Append(LineFeed);
			// Append name
			Output.AppendFormat(" %s = ", cobj->LocalNamed.pNames->pNames[cnt]);
			// write value
			Output.Append(static_cast<const StdStrBuf &>(cobj->LocalNamed.pData[cnt].GetDataString()));
		}
		// Effects
		for (C4Effect *pEffect = cobj->pEffects; pEffect; pEffect = pEffect->pNext)
		{
			// Header
			if (pEffect == cobj->pEffects)
			{
				Output.Append(LineFeed);
				Output.Append(LoadResStr("IDS_CNS_EFFECTS"));
			}
			Output.Append(LineFeed);
			// Effect name
			Output.AppendFormat(" %s: Interval %d", pEffect->Name, pEffect->iIntervall);
		}
		// Store selected def
		idSelectedDef = cobj->id;
		break;
	}
	// Multiple selected objects
	default:
		Output.Format(LoadResStr("IDS_CNS_MULTIPLEOBJECTS"), Selection.ObjectCount());
		break;
	}
	// Update info edit control
#ifdef _WIN32
	const auto iLine = SendDlgItemMessage(hDialog, IDC_EDITOUTPUT, EM_GETFIRSTVISIBLELINE, 0, 0);
	SetDlgItemText(hDialog, IDC_EDITOUTPUT, Output.getData());
	SendDlgItemMessage(hDialog, IDC_EDITOUTPUT, EM_LINESCROLL, 0, iLine);
	UpdateWindow(GetDlgItem(hDialog, IDC_EDITOUTPUT));
#elif defined(WITH_DEVELOPER_MODE)
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
	gtk_text_buffer_set_text(buffer, C4Language::IconvUtf8(Output.getData()).getData(), -1);
#endif
	return true;
}

void C4PropertyDlg::Default()
{
#ifdef _WIN32
	hDialog = nullptr;
#elif defined(WITH_DEVELOPER_MODE)
	vbox = nullptr;
#endif
	Active = false;
	idSelectedDef = C4ID_None;
	Selection.Default();
}

void C4PropertyDlg::Clear()
{
	Selection.Clear();
#ifdef _WIN32
	if (hDialog) DestroyWindow(hDialog); hDialog = nullptr;
#endif
	Active = false;
}

void C4PropertyDlg::UpdateInputCtrl(C4Object *pObj)
{
	int cnt;
#ifdef _WIN32
	HWND hCombo = GetDlgItem(hDialog, IDC_COMBOINPUT);
	// Remember old window text
	char szLastText[500 + 1];
	GetWindowText(hCombo, szLastText, 500);
	// Clear
	SendMessage(hCombo, CB_RESETCONTENT, 0, 0);
#else // _WIN32
#ifdef WITH_DEVELOPER_MODE

	GtkEntryCompletion *completion = gtk_entry_get_completion(GTK_ENTRY(entry));
	GtkListStore *store;

	// Uncouple list store from completion so that the completion is not
	// notified for every row we are going to insert. This enhances
	// performance significantly.
	if (!completion)
	{
		completion = gtk_entry_completion_new();
		store = gtk_list_store_new(1, G_TYPE_STRING);

		gtk_entry_completion_set_text_column(completion, 0);
		gtk_entry_set_completion(GTK_ENTRY(entry), completion);
		g_object_unref(G_OBJECT(completion));
	}
	else
	{
		store = GTK_LIST_STORE(gtk_entry_completion_get_model(completion));
		g_object_ref(G_OBJECT(store));
		gtk_entry_completion_set_model(completion, nullptr);
	}

	GtkTreeIter iter;
	gtk_list_store_clear(store);
#endif // WITH_DEVELOPER_MODE
#endif // _WIN32

	// add global and standard functions
	for (C4AulFunc *pFn = Game.ScriptEngine.GetFirstFunc(); pFn; pFn = Game.ScriptEngine.GetNextFunc(pFn))
		if (pFn->GetPublic())
		{
#ifdef _WIN32
			SendMessage(hCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>((std::string{pFn->Name} + "()").c_str()));
#elif defined(WITH_DEVELOPER_MODE)
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter, 0, pFn->Name, -1);
#endif
		}
	// Add object script functions
#ifdef _WIN32
	bool fDivider = false;
#endif
	C4AulScriptFunc *pRef;
	// Object script available
	if (pObj && pObj->Def)
		// Scan all functions
		for (cnt = 0; pRef = pObj->Def->Script.GetSFunc(cnt); cnt++)
			// Public functions only
			if (pRef->Access == AA_PUBLIC)
			{
#ifdef _WIN32
				// Insert divider if necessary
				if (!fDivider) { SendMessage(hCombo, CB_INSERTSTRING, 0, reinterpret_cast<LPARAM>("----------")); fDivider = true; }
#endif
				// Add function
#ifdef _WIN32
				SendMessage(hCombo, CB_INSERTSTRING, 0, reinterpret_cast<LPARAM>((std::string{pRef->Name} + "()").c_str()));
#elif defined(WITH_DEVELOPER_MODE)
				gtk_list_store_append(store, &iter);
				gtk_list_store_set(store, &iter, 0, pRef->Name, -1);
#endif
			}

#ifdef _WIN32
	// Restore old text
	SetWindowText(hCombo, szLastText);
#elif WITH_DEVELOPER_MODE
	// Reassociate list store with completion
	gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(store));
#endif
}

void C4PropertyDlg::Execute()
{
	if (!Tick35) Update();
}

void C4PropertyDlg::ClearPointers(C4Object *pObj)
{
	Selection.ClearPointers(pObj);
}

#ifdef WITH_DEVELOPER_MODE
// GTK+ callbacks
void C4PropertyDlg::OnScriptActivate(GtkWidget *widget, gpointer data)
{
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(widget));
	if (text && text[0])
		Console.EditCursor.In(text);
}

void C4PropertyDlg::OnWindowHide(GtkWidget *widget, gpointer user_data)
{
	static_cast<C4PropertyDlg *>(user_data)->Active = false;
}

#endif
