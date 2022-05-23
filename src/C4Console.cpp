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

/* Handles engine execution in developer mode */

#include <C4Include.h>
#include <C4Console.h>
#include <C4Application.h>

#include <C4GameSave.h>
#include <C4UserMessages.h>
#include <C4Version.h>
#include <C4Language.h>
#include <C4Log.h>
#include <C4Player.h>

#include <StdFile.h>
#include <StdRegistry.h>

#ifdef _WIN32

#include <commdlg.h>

bool SetMenuItemText(HMENU hMenu, WORD id, const char *szText);

#else

namespace
{
	const uint32_t OFN_HIDEREADONLY     = 1 << 0;
	const uint32_t OFN_OVERWRITEPROMPT  = 1 << 1;
	const uint32_t OFN_FILEMUSTEXIST    = 1 << 2;
	const uint32_t OFN_ALLOWMULTISELECT = 1 << 3;

	const uint32_t OFN_EXPLORER         = 0; // ignored
}

#ifdef USE_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#endif // _WIN32

#ifndef WITH_DEVELOPER_MODE
#define WITH_DEVELOPER_MODE 0
#endif

#if WITH_DEVELOPER_MODE

#include <gdk/gdkcursor.h>
#include <gdk/gdkx.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkmessagedialog.h>
#include <gtk/gtkfilechooserdialog.h>
#include <gtk/gtkaboutdialog.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkvseparator.h>
#include <gtk/gtktextview.h>
#include <gtk/gtkmenubar.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkseparatormenuitem.h>

#include <res/Play.h>
#include <res/Halt.h>
#include <res/Mouse.h>
#include <res/Cursor.h>
#include <res/Brush.h>

namespace
{
	GtkWidget *CreateImageFromInlinedPixbuf(const guint8 *pixbuf_data)
	{
		GdkPixbuf *pixbuf = gdk_pixbuf_new_from_inline(-1, pixbuf_data, FALSE, nullptr);
		GtkWidget *image = gtk_image_new_from_pixbuf(pixbuf);
		g_object_unref(pixbuf);
		return image;
	}

	class ImplicitStrBuf : public StdStrBuf
	{
	public:
		ImplicitStrBuf(StdStrBuf &&Buf2) : StdStrBuf(std::forward<StdStrBuf>(Buf2)) {}
		operator const char *() const { return getData(); }
	};

	inline ImplicitStrBuf LoadResStrUtf8I(const char *ident)
	{
		return ImplicitStrBuf(Languages.IconvUtf8(LoadResStr(ident)));
	}
}

#endif

C4Console::C4Console()
{
	Active = false;
	Editing = true;
	ScriptCounter = 0;
	FrameCounter = 0;
	fGameOpen = false;

#ifdef _WIN32
	hWindow = nullptr;
	hbmCursor = nullptr;
	hbmCursor2 = nullptr;
	hbmBrush = nullptr;
	hbmBrush2 = nullptr;
	hbmPlay = nullptr;
	hbmPlay2 = nullptr;
	hbmHalt = nullptr;
	hbmHalt2 = nullptr;
#elif WITH_DEVELOPER_MODE
	cursorDefault = nullptr;
	cursorWait = nullptr;
	itemNet = nullptr;
	txtLog = nullptr;
	txtScript = nullptr;
#endif // WITH_DEVELOPER_MODE / _WIN32

	MenuIndexFile = 0;
	MenuIndexComponents = 1;
	MenuIndexPlayer = 2;
	MenuIndexViewport = 3;
	MenuIndexNet = -1;
	MenuIndexHelp = 4;
}

C4Console::~C4Console()
{
#ifdef _WIN32
	if (hbmCursor)  DeleteObject(hbmCursor);
	if (hbmCursor2) DeleteObject(hbmCursor2);
	if (hbmBrush)   DeleteObject(hbmBrush);
	if (hbmBrush2)  DeleteObject(hbmBrush2);
	if (hbmPlay)    DeleteObject(hbmPlay);
	if (hbmPlay2)   DeleteObject(hbmPlay2);
	if (hbmHalt)    DeleteObject(hbmHalt);
	if (hbmHalt2)   DeleteObject(hbmHalt2);
#elif WITH_DEVELOPER_MODE
	if (cursorDefault) gdk_cursor_unref(cursorDefault);
	if (cursorWait)    gdk_cursor_unref(cursorWait);
#endif // WITH_DEVELOPER_MODE / _WIN32
}

#ifdef _WIN32

INT_PTR CALLBACK ConsoleDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_ACTIVATEAPP:
		Application.Active = wParam != 0;
		return TRUE;

	case WM_TIMER:
		if (wParam == SEC1_TIMER) { Console.Sec1Timer(); }
		return TRUE;

	case WM_DESTROY:
		StoreWindowPosition(hDlg, "Main", Config.GetSubkeyPath("Console"), false);
		Application.Quit();
		return TRUE;

	case WM_CLOSE:
		Console.Close();
		return TRUE;

	case WM_INITDIALOG:
		SendMessage(hDlg, DM_SETDEFID, IDOK, 0);
		Console.UpdateMenuText(GetMenu(hDlg));
		return TRUE;

	case WM_COMMAND:
		// Evaluate command
		switch (LOWORD(wParam))
		{
		case IDOK:
			// IDC_COMBOINPUT to Console.In()
			char buffer[16000];
			GetDlgItemText(hDlg, IDC_COMBOINPUT, buffer, 16000);
			if (buffer[0])
				Console.In(buffer);
			return TRUE;

		case IDC_BUTTONHALT:
			Console.DoHalt();
			return TRUE;

		case IDC_BUTTONPLAY:
			Console.DoPlay();
			return TRUE;

		case IDC_BUTTONMODEPLAY:
			Console.EditCursor.SetMode(C4CNS_ModePlay);
			return TRUE;

		case IDC_BUTTONMODEEDIT:
			Console.EditCursor.SetMode(C4CNS_ModeEdit);
			return TRUE;

		case IDC_BUTTONMODEDRAW:
			Console.EditCursor.SetMode(C4CNS_ModeDraw);
			return TRUE;

		case IDM_FILE_QUIT:         Console.FileQuit();        return TRUE;
		case IDM_FILE_SAVEAS:       Console.FileSaveAs(false); return TRUE;
		case IDM_FILE_SAVE:         Console.FileSave(false);   return TRUE;
		case IDM_FILE_SAVEGAMEAS:   Console.FileSaveAs(true);  return TRUE;
		case IDM_FILE_SAVEGAME:     Console.FileSave(true);    return TRUE;
		case IDM_FILE_OPEN:         Console.FileOpen();        return TRUE;
		case IDM_FILE_RECORD:       Console.FileRecord();      return TRUE;
		case IDM_FILE_OPENWPLRS:    Console.FileOpenWPlrs();   return TRUE;
		case IDM_FILE_CLOSE:        Console.FileClose();       return TRUE;
		case IDM_HELP_ABOUT:        Console.HelpAbout();       return TRUE;
		case IDM_PLAYER_JOIN:       Console.PlayerJoin();      return TRUE;
		case IDM_VIEWPORT_NEW:      Console.ViewportNew();     return TRUE;
		case IDM_COMPONENTS_TITLE:  Console.EditTitle();       return TRUE;
		case IDM_COMPONENTS_INFO:   Console.EditInfo();        return TRUE;
		case IDM_COMPONENTS_SCRIPT: Console.EditScript();      return TRUE;
		}
		// New player viewport
		if (Inside((int)LOWORD(wParam), IDM_VIEWPORT_NEW1, IDM_VIEWPORT_NEW2))
		{
			Game.CreateViewport(LOWORD(wParam) - IDM_VIEWPORT_NEW1);
			return TRUE;
		}
		// Remove player
		if (Inside((int)LOWORD(wParam), IDM_PLAYER_QUIT1, IDM_PLAYER_QUIT2))
		{
			Game.Control.Input.Add(CID_EliminatePlayer, new C4ControlEliminatePlayer(LOWORD(wParam) - IDM_PLAYER_QUIT1));
			return TRUE;
		}
		// Remove client
		if (Inside((int)LOWORD(wParam), IDM_NET_CLIENT1, IDM_NET_CLIENT2))
		{
			if (!Game.Control.isCtrlHost()) return FALSE;
			Game.Clients.CtrlRemove(Game.Clients.getClientByID(LOWORD(wParam) - IDM_NET_CLIENT1), LoadResStr("IDS_MSG_KICKBYMENU"));
			return TRUE;
		}
		return FALSE;

	case WM_COPYDATA:
		COPYDATASTRUCT *pcds = reinterpret_cast<COPYDATASTRUCT *>(lParam);
		if (pcds->dwData == WM_USER_RELOADFILE)
		{
			// get path, ensure proper termination
			const char *szPath = reinterpret_cast<const char *>(pcds->lpData);
			if (szPath[pcds->cbData - 1]) break;
			// reload
			Game.ReloadFile(szPath);
		}
		return FALSE;
	}

	return FALSE;
}

#elif defined(USE_X11) && !WITH_DEVELOPER_MODE

void C4Console::HandleMessage(XEvent &e)
{
	// Parent handling
	C4ConsoleBase::HandleMessage(e);

	switch (e.type)
	{
	case FocusIn:
		Application.Active = true;
		break;
	case FocusOut:
		Application.Active = false;
		break;
	}
}

#endif // _WIN32/USE_X11

CStdWindow *C4Console::Init(CStdApp *pApp)
{
	// Active
	Active = true;
	// Editing (enable even if network)
	Editing = true;
	// Create dialog window
#ifdef _WIN32
	hWindow = CreateDialog(pApp->hInstance, MAKEINTRESOURCE(IDD_CONSOLE), nullptr, ConsoleDlgProc);
	if (!hWindow)
	{
		char *lpMsgBuf;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			GetLastError(),
			0,
			(LPTSTR)&lpMsgBuf,
			0,
			nullptr);
		LogF("Error creating dialog window: %s", lpMsgBuf);
		// Free the buffer.
		LocalFree(lpMsgBuf);
		return nullptr;
	}
	// Restore window position
	RestoreWindowPosition(hWindow, "Main", Config.GetSubkeyPath("Console"));
	// Set icon
	SendMessage(hWindow, WM_SETICON, ICON_BIG,   reinterpret_cast<LPARAM>(LoadIcon(pApp->hInstance, MAKEINTRESOURCE(IDI_00_C4X))));
	SendMessage(hWindow, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(LoadIcon(pApp->hInstance, MAKEINTRESOURCE(IDI_00_C4X))));
	// Set text
	SetCaption(LoadResStr("IDS_CNS_CONSOLE"));
	// Load bitmaps
	hbmMouse   = (HBITMAP)LoadBitmap(pApp->hInstance, MAKEINTRESOURCE(IDB_MOUSE));
	hbmMouse2  = (HBITMAP)LoadBitmap(pApp->hInstance, MAKEINTRESOURCE(IDB_MOUSE2));
	hbmCursor  = (HBITMAP)LoadBitmap(pApp->hInstance, MAKEINTRESOURCE(IDB_CURSOR));
	hbmCursor2 = (HBITMAP)LoadBitmap(pApp->hInstance, MAKEINTRESOURCE(IDB_CURSOR2));
	hbmBrush   = (HBITMAP)LoadBitmap(pApp->hInstance, MAKEINTRESOURCE(IDB_BRUSH));
	hbmBrush2  = (HBITMAP)LoadBitmap(pApp->hInstance, MAKEINTRESOURCE(IDB_BRUSH2));
	hbmPlay    = (HBITMAP)LoadBitmap(pApp->hInstance, MAKEINTRESOURCE(IDB_PLAY));
	hbmPlay2   = (HBITMAP)LoadBitmap(pApp->hInstance, MAKEINTRESOURCE(IDB_PLAY2));
	hbmHalt    = (HBITMAP)LoadBitmap(pApp->hInstance, MAKEINTRESOURCE(IDB_HALT));
	hbmHalt2   = (HBITMAP)LoadBitmap(pApp->hInstance, MAKEINTRESOURCE(IDB_HALT2));
	// Enable controls
	UpdateHaltCtrls(true);
	EnableControls(fGameOpen);
	ClearViewportMenu();
	// Show window and set focus
	ShowWindow(hWindow, SW_SHOWNORMAL);
	UpdateWindow(hWindow);
	SetFocus(hWindow);
	ShowCursor(TRUE);
	// Success
	return this;
#elif WITH_DEVELOPER_MODE
	cursorWait = gdk_cursor_new(GDK_WATCH);
	cursorDefault = gdk_cursor_new(GDK_ARROW);

	// Calls InitGUI
	CStdWindow *retval = C4ConsoleBase::Init(pApp, LoadResStr("IDS_CNS_CONSOLE"), nullptr, false);
	UpdateHaltCtrls(true);
	EnableControls(fGameOpen);
	ClearViewportMenu();
	return retval;
#else
	return C4ConsoleBase::Init(pApp, LoadResStr("IDS_CNS_CONSOLE"), nullptr, false);
#endif // WITH_DEVELOPER_MODE / _WIN32
}

#if WITH_DEVELOPER_MODE
GtkWidget *C4Console::InitGUI()
{
	// Play/Pause and Mode

	GtkWidget *image_play  = CreateImageFromInlinedPixbuf(play_pixbuf_data);
	GtkWidget *image_pause = CreateImageFromInlinedPixbuf(halt_pixbuf_data);

	GtkWidget *image_mode_play = CreateImageFromInlinedPixbuf(mouse_pixbuf_data);
	GtkWidget *image_mode_edit = CreateImageFromInlinedPixbuf(cursor_pixbuf_data);
	GtkWidget *image_mode_draw = CreateImageFromInlinedPixbuf(brush_pixbuf_data);

	btnPlay = gtk_toggle_button_new();
	btnHalt = gtk_toggle_button_new();
	btnModePlay = gtk_toggle_button_new();
	btnModeEdit = gtk_toggle_button_new();
	btnModeDraw = gtk_toggle_button_new();

	gtk_container_add(GTK_CONTAINER(btnPlay), image_play);
	gtk_container_add(GTK_CONTAINER(btnHalt), image_pause);
	gtk_container_add(GTK_CONTAINER(btnModePlay), image_mode_play);
	gtk_container_add(GTK_CONTAINER(btnModeEdit), image_mode_edit);
	gtk_container_add(GTK_CONTAINER(btnModeDraw), image_mode_draw);

	GtkWidget *top_hbox = gtk_hbox_new(FALSE, 18);
	GtkWidget *play_hbox = gtk_hbox_new(FALSE, 6);
	GtkWidget *mode_hbox = gtk_hbox_new(FALSE, 6);

	gtk_box_pack_start(GTK_BOX(play_hbox), btnPlay,     FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(play_hbox), btnHalt,     FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(mode_hbox), btnModePlay, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(mode_hbox), btnModeEdit, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(mode_hbox), btnModeDraw, FALSE, TRUE, 0);

	lblCursor = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(lblCursor), 0.0, 0.5);

	gtk_box_pack_start(GTK_BOX(top_hbox), lblCursor, TRUE,  TRUE, 0);
	gtk_box_pack_start(GTK_BOX(top_hbox), play_hbox, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(top_hbox), mode_hbox, FALSE, TRUE, 0);

	// Statusbar

	GtkWidget *statusbar = gtk_hbox_new(FALSE, 6);

	GtkWidget *status_frame = gtk_frame_new(nullptr);
	gtk_frame_set_shadow_type(GTK_FRAME(status_frame), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(status_frame), statusbar);

	lblFrame = gtk_label_new("Frame: 0");
	lblScript = gtk_label_new("Script: 0");
	lblTime = gtk_label_new("00:00:00 (0 FPS)");

	gtk_misc_set_alignment(GTK_MISC(lblFrame), 0.0, 0.5);
	gtk_misc_set_alignment(GTK_MISC(lblScript), 0.0, 0.5);
	gtk_misc_set_alignment(GTK_MISC(lblTime), 0.0, 0.5);

	GtkWidget *sep1 = gtk_vseparator_new();
	GtkWidget *sep2 = gtk_vseparator_new();

	gtk_box_pack_start(GTK_BOX(statusbar), lblFrame,  TRUE,  TRUE,  0);
	gtk_box_pack_start(GTK_BOX(statusbar), sep1,      FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(statusbar), lblScript, TRUE,  TRUE,  0);
	gtk_box_pack_start(GTK_BOX(statusbar), sep2,      FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(statusbar), lblTime,   TRUE,  TRUE,  0);

	// Log view and script entry

	GtkWidget *scroll = gtk_scrolled_window_new(nullptr, nullptr);

	txtLog = gtk_text_view_new();
	txtScript = gtk_entry_new();

	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(txtLog), FALSE);

	gtk_container_add(GTK_CONTAINER(scroll), txtLog);

	// Menu

	menuBar = gtk_menu_bar_new();

	GtkWidget *itemFile       = gtk_menu_item_new_with_label(LoadResStrUtf8I("IDS_MNU_FILE"));
	GtkWidget *itemComponents = gtk_menu_item_new_with_label(LoadResStrUtf8I("IDS_MNU_COMPONENTS"));
	GtkWidget *itemPlayer     = gtk_menu_item_new_with_label(LoadResStrUtf8I("IDS_MNU_PLAYER"));
	GtkWidget *itemViewport   = gtk_menu_item_new_with_label(LoadResStrUtf8I("IDS_MNU_VIEWPORT"));
	GtkWidget *itemHelp       = gtk_menu_item_new_with_label("?");

	gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), itemFile);
	gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), itemComponents);
	gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), itemPlayer);
	gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), itemViewport);
	gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), itemHelp);

	GtkWidget *menuFile = gtk_menu_new();
	GtkWidget *menuComponents = gtk_menu_new();
	GtkWidget *menuHelp = gtk_menu_new();

	menuPlayer = gtk_menu_new();
	menuViewport = gtk_menu_new();

	gtk_menu_item_set_submenu(GTK_MENU_ITEM(itemFile),       menuFile);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(itemComponents), menuComponents);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(itemPlayer),     menuPlayer);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(itemViewport),   menuViewport);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(itemHelp),       menuHelp);

	fileOpen = gtk_menu_item_new_with_label(LoadResStrUtf8I("IDS_MNU_OPEN"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuFile), fileOpen);

	fileOpenWithPlayers = gtk_menu_item_new_with_label(LoadResStrUtf8I("IDS_MNU_OPENWPLRS"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuFile), fileOpenWithPlayers);

	gtk_menu_shell_append(GTK_MENU_SHELL(menuFile), GTK_WIDGET(gtk_separator_menu_item_new()));

	fileSave = gtk_menu_item_new_with_label(LoadResStrUtf8I("IDS_MNU_SAVESCENARIO"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuFile), fileSave);

	fileSaveAs = gtk_menu_item_new_with_label(LoadResStrUtf8I("IDS_MNU_SAVESCENARIOAS"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuFile), fileSaveAs);

	fileSaveGame = gtk_menu_item_new_with_label(LoadResStrUtf8I("IDS_MNU_SAVEGAME"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuFile), fileSaveGame);

	fileSaveGameAs = gtk_menu_item_new_with_label(LoadResStrUtf8I("IDS_MNU_SAVEGAMEAS"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuFile), fileSaveGameAs);

	fileRecord = gtk_menu_item_new_with_label(LoadResStrUtf8I("IDS_MNU_RECORD"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuFile), fileRecord);

	gtk_menu_shell_append(GTK_MENU_SHELL(menuFile), GTK_WIDGET(gtk_separator_menu_item_new()));

	fileClose = gtk_menu_item_new_with_label(LoadResStrUtf8I("IDS_MNU_CLOSE"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuFile), fileClose);

	gtk_menu_shell_append(GTK_MENU_SHELL(menuFile), GTK_WIDGET(gtk_separator_menu_item_new()));

	fileQuit = gtk_menu_item_new_with_label(LoadResStrUtf8I("IDS_MNU_QUIT"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuFile), fileQuit);

	compObjects = gtk_menu_item_new_with_label(LoadResStrUtf8I("IDS_BTN_OBJECTS"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuComponents), compObjects);

	compScript = gtk_menu_item_new_with_label(LoadResStrUtf8I("IDS_MNU_SCRIPT"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuComponents), compScript);

	compTitle = gtk_menu_item_new_with_label(LoadResStrUtf8I("IDS_MNU_TITLE"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuComponents), compTitle);

	compInfo = gtk_menu_item_new_with_label(LoadResStrUtf8I("IDS_MNU_INFO"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuComponents), compInfo);

	plrJoin = gtk_menu_item_new_with_label(LoadResStrUtf8I("IDS_MNU_JOIN"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuPlayer), plrJoin);

	viewNew = gtk_menu_item_new_with_label(LoadResStrUtf8I("IDS_MNU_NEW"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuViewport), viewNew);

	helpAbout = gtk_menu_item_new_with_label(LoadResStrUtf8I("IDS_MENU_ABOUT"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menuHelp), helpAbout);

	// Window

	GtkWidget *topbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *midbox = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(midbox), 6);

	gtk_box_pack_start(GTK_BOX(midbox), scroll,    TRUE,  TRUE,  0);
	gtk_box_pack_start(GTK_BOX(midbox), txtScript, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(midbox), top_hbox,  FALSE, TRUE,  0);

	gtk_box_pack_start(GTK_BOX(topbox), menuBar,      FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(topbox), midbox,       TRUE,  TRUE,  0);
	gtk_box_pack_start(GTK_BOX(topbox), status_frame, FALSE, FALSE, 0);

	gtk_window_set_default_size(GTK_WINDOW(window), 320, 320);

	gtk_container_add(GTK_CONTAINER(window), topbox);

	// Signals

	handlerPlay = g_signal_connect(G_OBJECT(btnPlay), "toggled", G_CALLBACK(OnPlay), this);
	handlerHalt = g_signal_connect(G_OBJECT(btnHalt), "toggled", G_CALLBACK(OnHalt), this);
	handlerModePlay = g_signal_connect(G_OBJECT(btnModePlay), "toggled", G_CALLBACK(OnModePlay), this);
	handlerModeEdit = g_signal_connect(G_OBJECT(btnModeEdit), "toggled", G_CALLBACK(OnModeEdit), this);
	handlerModeDraw = g_signal_connect(G_OBJECT(btnModeDraw), "toggled", G_CALLBACK(OnModeDraw), this);
	g_signal_connect(G_OBJECT(txtScript),           "activate", G_CALLBACK(OnScriptEntry),    this);
	g_signal_connect(G_OBJECT(fileOpen),            "activate", G_CALLBACK(OnFileOpen),       this);
	g_signal_connect(G_OBJECT(fileOpenWithPlayers), "activate", G_CALLBACK(OnFileOpenWPlrs),  this);
	g_signal_connect(G_OBJECT(fileSave),            "activate", G_CALLBACK(OnFileSave),       this);
	g_signal_connect(G_OBJECT(fileSaveAs),          "activate", G_CALLBACK(OnFileSaveAs),     this);
	g_signal_connect(G_OBJECT(fileSaveGame),        "activate", G_CALLBACK(OnFileSaveGame),   this);
	g_signal_connect(G_OBJECT(fileSaveGameAs),      "activate", G_CALLBACK(OnFileSaveGameAs), this);
	g_signal_connect(G_OBJECT(fileRecord),          "activate", G_CALLBACK(OnFileRecord),     this);
	g_signal_connect(G_OBJECT(fileClose),           "activate", G_CALLBACK(OnFileClose),      this);
	g_signal_connect(G_OBJECT(fileQuit),            "activate", G_CALLBACK(OnFileQuit),       this);
	g_signal_connect(G_OBJECT(compObjects),         "activate", G_CALLBACK(OnCompObjects),    this);
	g_signal_connect(G_OBJECT(compScript),          "activate", G_CALLBACK(OnCompScript),     this);
	g_signal_connect(G_OBJECT(compTitle),           "activate", G_CALLBACK(OnCompTitle),      this);
	g_signal_connect(G_OBJECT(compInfo),            "activate", G_CALLBACK(OnCompInfo),       this);
	g_signal_connect(G_OBJECT(plrJoin),             "activate", G_CALLBACK(OnPlrJoin),        this);
	g_signal_connect(G_OBJECT(viewNew),             "activate", G_CALLBACK(OnViewNew),        this);
	g_signal_connect(G_OBJECT(helpAbout),           "activate", G_CALLBACK(OnHelpAbout),      this);

	return C4ConsoleBase::InitGUI();
}
#endif // WITH_DEVELOPER_MODE

bool C4Console::In(const char *szText)
{
	if (!Active || !szText) return false;
	// begins with '/'? then it's a command
	if (*szText == '/')
	{
		Game.MessageInput.ProcessCommand(szText);
		// done
		return true;
	}
	// begins with '#'? then it's a message. Route cia ProcessInput to allow #/sound
	if (*szText == '#')
	{
		Game.MessageInput.ProcessInput(szText + 1);
		return true;
	}
	// editing enabled?
	if (!EditCursor.EditingOK()) return false;
	// pass through network queue
	Game.Control.DoInput(CID_Script, new C4ControlScript(szText, C4ControlScript::SCOPE_Console), CDT_Decide);
	return true;
}

bool C4Console::Out(const char *szText)
{
#ifdef _WIN32
	if (!Active) return false;
	if (!szText || !*szText) return true;
	int len, len2; char *buffer, *buffer2;
	len = 65000;
	len2 = len + std::min<int>(strlen(szText) + 2, 5000);
	buffer = new char[len2];
	buffer[0] = 0;
	GetDlgItemText(hWindow, IDC_EDITOUTPUT, buffer, len);
	if (buffer[0]) SAppend("\r\n", buffer);
	SAppend(szText, buffer, len2 - 1);
	if (strlen(buffer) > 60000) buffer2 = buffer + strlen(buffer) - 60000; else buffer2 = buffer; // max log length: Otherwise, discard beginning
	SetDlgItemText(hWindow, IDC_EDITOUTPUT, buffer2);
	delete[] buffer;
	const auto lines = SendDlgItemMessage(hWindow, IDC_EDITOUTPUT, EM_GETLINECOUNT, 0, 0);
	SendDlgItemMessage(hWindow, IDC_EDITOUTPUT, EM_LINESCROLL, 0, static_cast<LPARAM>(lines));
	UpdateWindow(hWindow);
#elif WITH_DEVELOPER_MODE
	// Append text to log
	if (!window) return true;

	GtkTextIter end;
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txtLog));
	gtk_text_buffer_get_end_iter(buffer, &end);

	gtk_text_buffer_insert(buffer, &end, C4Language::IconvUtf8(szText).getData(), -1);
	gtk_text_buffer_insert(buffer, &end, "\n", 1);

	gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(txtLog), gtk_text_buffer_get_insert(buffer), 0.0, FALSE, 0.0, 0.0);

	// Cheap hack to get the Console window updated while loading
	while (g_main_context_pending(nullptr))
		g_main_context_iteration(nullptr, FALSE);
#endif // WITH_DEVELOPER_MODE / _WIN32
	return true;
}

bool C4Console::ClearLog()
{
#ifdef _WIN32
	SetDlgItemText(hWindow, IDC_EDITOUTPUT, "");
	SendDlgItemMessage(hWindow, IDC_EDITOUTPUT, EM_LINESCROLL, 0, 0);
	UpdateWindow(hWindow);
#elif WITH_DEVELOPER_MODE
	gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(txtLog)), "", 0);
#endif // WITH_DEVELOPER_MODE / _WIN32
	return true;
}

// Someone defines Status as int....
#ifdef Status
#undef Status
#endif

void C4Console::DoPlay()
{
	Game.Unpause();
}

void C4Console::DoHalt()
{
	Game.Pause();
}

bool C4Console::UpdateStatusBars()
{
	if (!Active) return false;
	// Frame counter
	if (Game.FrameCounter != FrameCounter)
	{
		FrameCounter = Game.FrameCounter;
		const auto &text = FormatString("Frame: %i", FrameCounter);
#ifdef _WIN32
		SetDlgItemText(hWindow, IDC_STATICFRAME, text.getData());
		UpdateWindow(GetDlgItem(hWindow, IDC_STATICFRAME));
#elif WITH_DEVELOPER_MODE
		gtk_label_set_label(GTK_LABEL(lblFrame), text.getData());
#endif // WITH_DEVELOPER_MODE / _WIN32
	}
	// Script counter
	if (Game.Script.Counter != ScriptCounter)
	{
		ScriptCounter = Game.Script.Counter;
		const auto &text = FormatString("Script: %i", ScriptCounter);
#ifdef _WIN32
		SetDlgItemText(hWindow, IDC_STATICSCRIPT, text.getData());
		UpdateWindow(GetDlgItem(hWindow, IDC_STATICSCRIPT));
#elif WITH_DEVELOPER_MODE
		gtk_label_set_label(GTK_LABEL(lblScript), text.getData());
#endif // WITH_DEVELOPER_MODE / _WIN32
	}
	// Time & FPS
	if ((Game.Time != Time) || (Game.FPS != FPS))
	{
		Time = Game.Time;
		FPS = Game.FPS;
		const auto &text = FormatString("%02d:%02d:%02d (%i FPS)", Time / 3600, (Time % 3600) / 60, Time % 60, FPS);
#ifdef _WIN32
		SetDlgItemText(hWindow, IDC_STATICTIME, text.getData());
		UpdateWindow(GetDlgItem(hWindow, IDC_STATICTIME));
#elif WITH_DEVELOPER_MODE
		gtk_label_set_label(GTK_LABEL(lblTime), text.getData());
#endif // WITH_DEVELOPER_MODE
	}
	return true;
}

bool C4Console::UpdateHaltCtrls(bool fHalt)
{
	if (!Active) return false;
#ifdef _WIN32
	SendDlgItemMessage(hWindow, IDC_BUTTONPLAY, BM_SETSTATE, !fHalt, 0);
	UpdateWindow(GetDlgItem(hWindow, IDC_BUTTONPLAY));
	SendDlgItemMessage(hWindow, IDC_BUTTONHALT, BM_SETSTATE, fHalt, 0);
	UpdateWindow(GetDlgItem(hWindow, IDC_BUTTONHALT));
#elif WITH_DEVELOPER_MODE
	// Prevents recursion
	g_signal_handler_block(btnPlay, handlerPlay);
	g_signal_handler_block(btnHalt, handlerHalt);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btnPlay), !fHalt);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btnHalt), fHalt);

	g_signal_handler_unblock(btnPlay, handlerPlay);
	g_signal_handler_unblock(btnHalt, handlerHalt);

#endif // WITH_DEVELOPER_MODE / _WIN32
	return true;
}

bool C4Console::SaveGame(bool fSaveGame)
{
	// Network hosts only
	if (Game.Network.isEnabled() && !Game.Network.isHost())
	{
		Message(LoadResStr("IDS_GAME_NOCLIENTSAVE")); return false;
	}

	// Can't save to child groups
	if (Game.ScenarioFile.GetMother())
	{
		Message(FormatString(LoadResStr("IDS_CNS_NOCHILDSAVE"), GetFilename(Game.ScenarioFile.GetName())).getData());
		return false;
	}

	// Save game to open scenario file
	bool fOkay = true;
#ifdef _WIN32
	SetCursor(LoadCursor(0, IDC_WAIT));
#elif WITH_DEVELOPER_MODE
	// Seems not to work. Don't know why...
	gdk_window_set_cursor(window->window, cursorWait);
#endif

	C4GameSave *pGameSave;
	if (fSaveGame)
		pGameSave = new C4GameSaveSavegame();
	else
		pGameSave = new C4GameSaveScenario(!Console.Active || Game.Landscape.Mode == C4LSC_Exact, false);
	if (!pGameSave->Save(Game.ScenarioFile, false))
	{
		Out("Game::Save failed"); fOkay = false;
	}
	delete pGameSave;

	// Close and reopen scenario file to fix file changes
	if (!Game.ScenarioFile.Close())
	{
		Out("ScenarioFile::Close failed"); fOkay = false;
	}
	if (!Game.ScenarioFile.Open(Game.ScenarioFilename))
	{
		Out("ScenarioFile::Open failed"); fOkay = false;
	}

#ifdef _WIN32
	SetCursor(LoadCursor(0, IDC_ARROW));
#elif WITH_DEVELOPER_MODE
	gdk_window_set_cursor(window->window, nullptr);
#endif

	// Initialize/script notification
	if (Game.fScriptCreatedObjects)
		if (!fSaveGame)
		{
			Message((std::string{LoadResStr("IDS_CNS_SCRIPTCREATEDOBJECTS")} + LoadResStr("IDS_CNS_WARNDOUBLE")).c_str());
			Game.fScriptCreatedObjects = false;
		}

	// Status report
	if (!fOkay) Message(LoadResStr("IDS_CNS_SAVERROR"));
	else Out(LoadResStr(fSaveGame ? "IDS_CNS_GAMESAVED" : "IDS_CNS_SCENARIOSAVED"));

	return fOkay;
}

bool C4Console::FileSave(bool fSaveGame)
{
	// Don't quicksave games over scenarios
	if (fSaveGame)
		if (!Game.C4S.Head.SaveGame)
		{
			Message(LoadResStr("IDS_CNS_NOGAMEOVERSCEN"));
			return false;
		}
	// Save game
	return SaveGame(fSaveGame);
}

bool C4Console::FileSaveAs(bool fSaveGame)
{
	// Do save-as dialog
	char filename[512 + 1];
	SCopy(Game.ScenarioFile.GetName(), filename);
	if (!FileSelect(filename, 512,
		"Clonk 4 Scenario\0*.c4s\0\0",
		OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
		true)) return false;
	DefaultExtension(filename, "c4s");
	bool fOkay = true;
	// Close current scenario file
	if (!Game.ScenarioFile.Close()) fOkay = false;
	// Copy current scenario file to target
	if (!C4Group_CopyItem(Game.ScenarioFilename, filename)) fOkay = false;
	// Open new scenario file
	SCopy(filename, Game.ScenarioFilename);

	SetCaption(GetFilename(Game.ScenarioFilename));
	if (!Game.ScenarioFile.Open(Game.ScenarioFilename)) fOkay = false;
	// Failure message
	if (!fOkay)
	{
		Message(FormatString(LoadResStr("IDS_CNS_SAVEASERROR"), Game.ScenarioFilename).getData());
		return false;
	}
	// Save game
	return SaveGame(fSaveGame);
}

bool C4Console::Message(const char *szMessage, bool fQuery)
{
	if (!Active) return false;
#ifdef _WIN32
	return (IDOK == MessageBox(hWindow, szMessage, C4ENGINECAPTION, fQuery ? (MB_OKCANCEL | MB_ICONEXCLAMATION) : MB_ICONEXCLAMATION));
#elif WITH_DEVELOPER_MODE
	GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, fQuery ? (GTK_BUTTONS_OK_CANCEL) : (GTK_BUTTONS_OK), "%s", szMessage);
	int response = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	return response == GTK_RESPONSE_OK;
#endif
	return false;
}

void C4Console::EnableControls(bool fEnable)
{
	if (!Active) return;
	// disable Editing if no input allowed
	Editing &= !Game.Control.NoInput();

#ifdef _WIN32
	// Set button images (edit modes & halt controls)
	SendDlgItemMessage(hWindow, IDC_BUTTONMODEPLAY, BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(fEnable ? hbmMouse : hbmMouse2));
	SendDlgItemMessage(hWindow, IDC_BUTTONMODEEDIT, BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>((fEnable && Editing) ? hbmCursor : hbmCursor2));
	SendDlgItemMessage(hWindow, IDC_BUTTONMODEDRAW, BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>((fEnable && Editing) ? hbmBrush : hbmBrush2));
	SendDlgItemMessage(hWindow, IDC_BUTTONPLAY, BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(Game.Network.isLobbyActive() || fEnable ? hbmPlay : hbmPlay2));
	SendDlgItemMessage(hWindow, IDC_BUTTONHALT, BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(Game.Network.isLobbyActive() || fEnable ? hbmHalt : hbmHalt2));

	// OK
	EnableWindow(GetDlgItem(hWindow, IDOK), fEnable);

	// Halt controls
	EnableWindow(GetDlgItem(hWindow, IDC_BUTTONPLAY), Game.Network.isLobbyActive() || fEnable);
	EnableWindow(GetDlgItem(hWindow, IDC_BUTTONHALT), Game.Network.isLobbyActive() || fEnable);

	// Edit modes
	EnableWindow(GetDlgItem(hWindow, IDC_BUTTONMODEPLAY), (fEnable));
	EnableWindow(GetDlgItem(hWindow, IDC_BUTTONMODEEDIT), (fEnable && Editing));
	EnableWindow(GetDlgItem(hWindow, IDC_BUTTONMODEDRAW), (fEnable && Editing));

	// Console input
	EnableWindow(GetDlgItem(hWindow, IDC_COMBOINPUT), fEnable);

	// File menu
	// C4Network2 will have to handle that cases somehow (TODO: test)
	EnableMenuItem(GetMenu(hWindow), IDM_FILE_OPEN,      MF_BYCOMMAND | MF_ENABLED);
	EnableMenuItem(GetMenu(hWindow), IDM_FILE_OPENWPLRS, MF_BYCOMMAND | MF_ENABLED);
	EnableMenuItem(GetMenu(hWindow), IDM_FILE_RECORD, MF_BYCOMMAND | ((Game.IsRunning && Game.Control.IsRuntimeRecordPossible()) ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(GetMenu(hWindow), IDM_FILE_SAVEGAME,   MF_BYCOMMAND | ((fEnable && Game.Players.GetCount()) ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(GetMenu(hWindow), IDM_FILE_SAVEGAMEAS, MF_BYCOMMAND | ((fEnable && Game.Players.GetCount()) ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(GetMenu(hWindow), IDM_FILE_SAVE,   MF_BYCOMMAND | (fEnable ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(GetMenu(hWindow), IDM_FILE_SAVEAS, MF_BYCOMMAND | (fEnable ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(GetMenu(hWindow), IDM_FILE_CLOSE,  MF_BYCOMMAND | (fEnable ? MF_ENABLED : MF_GRAYED));

	// Components menu
	EnableMenuItem(GetMenu(hWindow), IDM_COMPONENTS_SCRIPT, MF_BYCOMMAND | ((fEnable && Editing) ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(GetMenu(hWindow), IDM_COMPONENTS_INFO,   MF_BYCOMMAND | ((fEnable && Editing) ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(GetMenu(hWindow), IDM_COMPONENTS_TITLE,  MF_BYCOMMAND | ((fEnable && Editing) ? MF_ENABLED : MF_GRAYED));

	// Player & viewport menu
	EnableMenuItem(GetMenu(hWindow), IDM_PLAYER_JOIN, MF_BYCOMMAND | ((fEnable && Editing) ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(GetMenu(hWindow), IDM_VIEWPORT_NEW, MF_BYCOMMAND | (fEnable ? MF_ENABLED : MF_GRAYED));
#elif WITH_DEVELOPER_MODE
	// Halt controls
	gtk_widget_set_sensitive(btnPlay, Game.Network.isLobbyActive() || fEnable);
	gtk_widget_set_sensitive(btnHalt, Game.Network.isLobbyActive() || fEnable);

	// Edit modes
	gtk_widget_set_sensitive(btnModePlay, Game.Network.isLobbyActive() || fEnable);
	gtk_widget_set_sensitive(btnModeEdit, Game.Network.isLobbyActive() || fEnable);
	gtk_widget_set_sensitive(btnModeDraw, Game.Network.isLobbyActive() || fEnable);

	// Console input
	gtk_widget_set_sensitive(txtScript, Game.Network.isLobbyActive() || fEnable);

	// File menu
	// C4Network2 will have to handle that cases somehow (TODO: test)
	gtk_widget_set_sensitive(fileRecord, Game.IsRunning && Game.Control.IsRuntimeRecordPossible());
	gtk_widget_set_sensitive(fileSaveGame,   fEnable && Game.Players.GetCount());
	gtk_widget_set_sensitive(fileSaveGameAs, fEnable && Game.Players.GetCount());
	gtk_widget_set_sensitive(fileSave,   fEnable);
	gtk_widget_set_sensitive(fileSaveAs, fEnable);
	gtk_widget_set_sensitive(fileClose,  fEnable);

	// Components menu
	gtk_widget_set_sensitive(compObjects, fEnable && Editing);
	gtk_widget_set_sensitive(compScript,  fEnable && Editing);
	gtk_widget_set_sensitive(compInfo,    fEnable && Editing);
	gtk_widget_set_sensitive(compTitle,   fEnable && Editing);

	// Player & viewport menu
	gtk_widget_set_sensitive(plrJoin, fEnable && Editing);
	gtk_widget_set_sensitive(viewNew, fEnable);
#endif // WITH_DEVELOPER_MODE / _WIN32
}

bool C4Console::FileOpen()
{
	// Get scenario file name
	char c4sfile[512 + 1] = "";
	if (!FileSelect(c4sfile, 512,
		"Clonk 4 Scenario\0*.c4s;*.c4f;Scenario.txt\0\0",
		OFN_HIDEREADONLY | OFN_FILEMUSTEXIST
	)) return false;
	// Compose command line
	char cmdline[2000] = "";
	SAppend("\"", cmdline, 1999); SAppend(c4sfile, cmdline, 1999); SAppend("\" ", cmdline, 1999);
	// Open game
	OpenGame(cmdline);
	return true;
}

bool C4Console::FileOpenWPlrs()
{
	// Get scenario file name
	char c4sfile[512 + 1] = "";
	if (!FileSelect(c4sfile, 512,
		"Clonk 4 Scenario\0*.c4s;*.c4f\0\0",
		OFN_HIDEREADONLY | OFN_FILEMUSTEXIST
	)) return false;
	// Get player file name(s)
	char c4pfile[4096 + 1] = "";
	if (!FileSelect(c4pfile, 4096,
		"Clonk 4 Player\0*.c4p\0\0",
		OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT | OFN_EXPLORER
	)) return false;
	// Compose command line
	char cmdline[6000] = "";
	SAppend("\"", cmdline, 5999); SAppend(c4sfile, cmdline, 5999); SAppend("\" ", cmdline, 5999);
	if (DirectoryExists(c4pfile)) // Multiplayer
	{
		const char *cptr = c4pfile + SLen(c4pfile) + 1;
		while (*cptr)
		{
			SAppend("\"", cmdline, 5999);
			SAppend(c4pfile, cmdline, 5999); SAppend(DirSep, cmdline, 5999);
			SAppend(cptr, cmdline, 5999); SAppend("\" ", cmdline, 5999);
			cptr += SLen(cptr) + 1;
		}
	}
	else // Single player
	{
		SAppend("\"", cmdline, 5999); SAppend(c4pfile, cmdline, 5999); SAppend("\" ", cmdline, 5999);
	}
	// Open game
	OpenGame(cmdline);
	return true;
}

bool C4Console::FileClose()
{
	return CloseGame();
}

#ifdef _WIN32
bool C4Console::FileSelect(char *sFilename, int iSize, const char *szFilter, DWORD    dwFlags, bool fSave)
#else
bool C4Console::FileSelect(char *sFilename, int iSize, const char *szFilter, uint32_t dwFlags, bool fSave)
#endif
{
#ifdef _WIN32
	OPENFILENAME ofn{};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWindow;
	ofn.lpstrFilter = szFilter;
	ofn.lpstrFile = sFilename;
	ofn.nMaxFile = iSize;
	ofn.Flags = dwFlags;

	bool fResult;
	if (fSave)
		fResult = GetSaveFileName(&ofn);
	else
		fResult = GetOpenFileName(&ofn);

	// Reset working directory to exe path as Windows file dialog might have changed it
	SetCurrentDirectory(Config.General.ExePath);
	return fResult;
#elif WITH_DEVELOPER_MODE
	GtkWidget *dialog = gtk_file_chooser_dialog_new(fSave ? "Save file..." : "Load file...", GTK_WINDOW(window), fSave ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, fSave ? GTK_STOCK_SAVE : GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, nullptr);

	// TODO: Set dialog modal?

	if (g_path_is_absolute(sFilename))
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), sFilename);
	else if (fSave)
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), sFilename);

	// Install file filter
	while (*szFilter)
	{
		char pattern[16 + 1];

		GtkFileFilter *filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, szFilter);
		szFilter += SLen(szFilter) + 1;

		while (true)
		{
			SCopyUntil(szFilter, pattern, ';', 16);

			int len = SLen(pattern);
			char last_c = szFilter[len];

			szFilter += (len + 1);

			// Got not all of the filter, try again.
			if (last_c != ';' && last_c != '\0')
				continue;

			gtk_file_filter_add_pattern(filter, pattern);
			if (last_c == '\0') break;
		}

		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	}

	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), (dwFlags & OFN_ALLOWMULTISELECT) != 0);
	gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), true);

	int response;
	while (true)
	{
		response = gtk_dialog_run(GTK_DIALOG(dialog));
		if (response == GTK_RESPONSE_CANCEL || response == GTK_RESPONSE_DELETE_EVENT) break;

		bool error = false;

		// Check for OFN_FILEMUSTEXIST
		if ((dwFlags & OFN_ALLOWMULTISELECT) == 0)
		{
			char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

			if ((dwFlags & OFN_FILEMUSTEXIST) && !g_file_test(filename, G_FILE_TEST_IS_REGULAR))
			{
				Message(FormatString("File \"%s\" does not exist", filename).getData(), false);
				error = true;
			}

			g_free(filename);
		}

		if (!error) break;
	}

	if (response != GTK_RESPONSE_ACCEPT)
	{
		gtk_widget_destroy(dialog);
		return false;
	}

	// Build result string
	if ((dwFlags & OFN_ALLOWMULTISELECT) == 0)
	{
		// Just the file name without multiselect
		char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		SCopy(filename, sFilename, iSize);
		g_free(filename);
	}
	else
	{
		// Otherwise its the folder followed by the file names,
		// separated by '\0'-bytes
		char *folder = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
		int len = SLen(folder);

		if (iSize > 0) SCopy(folder, sFilename, (std::min)(len + 1, iSize));
		iSize -= (len + 1); sFilename += (len + 1);
		g_free(folder);

		GSList *files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
		for (GSList *item = files; item != nullptr; item = item->next)
		{
			const char *file = static_cast<const char *>(item->data);
			char *basefile = g_path_get_basename(file);

			int len = SLen(basefile);
			if (iSize > 0) SCopy(basefile, sFilename, (std::min)(len + 1, iSize));
			iSize -= (len + 1); sFilename += (len + 1);

			g_free(basefile);
			g_free(item->data);
		}

		// End of list
		*sFilename = '\0';
		g_slist_free(files);
	}

	gtk_widget_destroy(dialog);
	return true;
#endif // WITH_DEVELOPER_MODE / _WIN32
	return 0;
}

bool C4Console::FileRecord()
{
	// only in running mode
	if (!Game.IsRunning || !Game.Control.IsRuntimeRecordPossible()) return false;
	// start record!
	Game.Control.RequestRuntimeRecord();
	// disable menuitem
#ifdef _WIN32
	EnableMenuItem(GetMenu(hWindow), IDM_FILE_RECORD, MF_BYCOMMAND | MF_GRAYED);
#elif WITH_DEVELOPER_MODE
	gtk_widget_set_sensitive(fileRecord, false);
#endif
	return true;
}

void C4Console::ClearPointers(C4Object *pObj)
{
	EditCursor.ClearPointers(pObj);
	PropertyDlg.ClearPointers(pObj);
}

void C4Console::Default()
{
	EditCursor.Default();
	PropertyDlg.Default();
	ToolsDlg.Default();
}

void C4Console::Clear()
{
	C4ConsoleBase::Clear();

	EditCursor.Clear();
	PropertyDlg.Clear();
	ToolsDlg.Clear();
	ClearViewportMenu();
	ClearPlayerMenu();
	ClearNetMenu();
#ifndef _WIN32
	Application.Quit();
#endif
}

void C4Console::Close()
{
	Application.Quit();
}

bool C4Console::FileQuit()
{
	Close();
	return true;
}

#define C4COPYRIGHT_YEAR    "2008" // might make this dynamic some time...
#define C4COPYRIGHT_COMPANY "RedWolf Design GmbH"

void C4Console::HelpAbout()
{
	StdStrBuf strCopyright;
	strCopyright.Format("Copyright (c) %s %s", C4COPYRIGHT_YEAR, C4COPYRIGHT_COMPANY);
#ifdef _WIN32
	StdStrBuf strMessage; strMessage.Format("%s %s\n\n%s", C4ENGINECAPTION, C4VERSION, strCopyright.getData());
	MessageBox(nullptr, strMessage.getData(), C4ENGINECAPTION, MB_ICONINFORMATION | MB_TASKMODAL);
#elif WITH_DEVELOPER_MODE
	gtk_show_about_dialog(GTK_WINDOW(window), "name", C4ENGINECAPTION, "version", C4VERSION, "copyright", strCopyright.getData(), nullptr);
#endif // WITH_DEVELOPER_MODE / _WIN32
}

void C4Console::ViewportNew()
{
	Game.CreateViewport(NO_OWNER);
}

bool C4Console::UpdateCursorBar(const char *szCursor)
{
	if (!Active) return false;
#ifdef _WIN32
	// Cursor
	SetDlgItemText(hWindow, IDC_STATICCURSOR, szCursor);
	UpdateWindow(GetDlgItem(hWindow, IDC_STATICCURSOR));
#elif WITH_DEVELOPER_MODE
	gtk_label_set_label(GTK_LABEL(lblCursor), Languages.IconvUtf8(szCursor).getData());
#endif
	return true;
}

bool C4Console::UpdateViewportMenu()
{
	if (!Active) return false;
	ClearViewportMenu();
#ifdef _WIN32
	HMENU hMenu = GetSubMenu(GetMenu(hWindow), MenuIndexViewport);
#endif
	for (C4Player *pPlr = Game.Players.First; pPlr; pPlr = pPlr->Next)
	{
		StdStrBuf sText;
		sText.Format(LoadResStr("IDS_CNS_NEWPLRVIEWPORT"), pPlr->GetName());
#ifdef _WIN32
		AddMenuItem(hMenu, IDM_VIEWPORT_NEW1 + pPlr->Number, sText.getData());
#elif WITH_DEVELOPER_MODE
		GtkWidget *menuItem = gtk_menu_item_new_with_label(Languages.IconvUtf8(sText.getData()).getData());
		gtk_menu_shell_append(GTK_MENU_SHELL(menuViewport), menuItem);
		g_signal_connect(G_OBJECT(menuItem), "activate", G_CALLBACK(OnViewNewPlr), GINT_TO_POINTER(pPlr->Number));
		gtk_widget_show(menuItem);
#endif // WITH_DEVELOPER_MODE / _WIN32
	}
	return true;
}

void C4Console::ClearViewportMenu()
{
	if (!Active) return;
#ifdef _WIN32
	HMENU hMenu = GetSubMenu(GetMenu(hWindow), MenuIndexViewport);
	while (DeleteMenu(hMenu, 1, MF_BYPOSITION));
#elif WITH_DEVELOPER_MODE
	GList *children = gtk_container_get_children(GTK_CONTAINER(menuViewport));
	for (GList *item = children; item != nullptr; item = item->next)
	{
		if (item->data != viewNew)
			gtk_container_remove(GTK_CONTAINER(menuViewport), GTK_WIDGET(item->data));
	}
	g_list_free(children);
#endif // WITH_DEVELOPER_MODE / _WIN32
}

#ifdef _WIN32
bool C4Console::AddMenuItem(HMENU hMenu, DWORD dwID, const char *szString, bool fEnabled)
{
	if (!Active) return false;
	MENUITEMINFO minfo{};
	minfo.cbSize = sizeof(minfo);
	minfo.fMask = MIIM_ID | MIIM_TYPE | MIIM_DATA | MIIM_STATE;
	minfo.fType = MFT_STRING;
	minfo.wID = dwID;
	minfo.dwTypeData = const_cast<char *>(szString);
	minfo.cch = SLen(szString);
	if (!fEnabled) minfo.fState |= MFS_GRAYED;
	return InsertMenuItem(hMenu, 0, FALSE, &minfo);
}

#endif // _WIN32
bool C4Console::UpdateModeCtrls(int iMode)
{
	if (!Active) return false;

#ifdef _WIN32
	SendDlgItemMessage(hWindow, IDC_BUTTONMODEPLAY, BM_SETSTATE, (iMode == C4CNS_ModePlay), 0);
	UpdateWindow(GetDlgItem(hWindow, IDC_BUTTONMODEPLAY));
	SendDlgItemMessage(hWindow, IDC_BUTTONMODEEDIT, BM_SETSTATE, (iMode == C4CNS_ModeEdit), 0);
	UpdateWindow(GetDlgItem(hWindow, IDC_BUTTONMODEEDIT));
	SendDlgItemMessage(hWindow, IDC_BUTTONMODEDRAW, BM_SETSTATE, (iMode == C4CNS_ModeDraw), 0);
	UpdateWindow(GetDlgItem(hWindow, IDC_BUTTONMODEDRAW));
#elif WITH_DEVELOPER_MODE
	// Prevents recursion
	g_signal_handler_block(btnModePlay, handlerModePlay);
	g_signal_handler_block(btnModeEdit, handlerModeEdit);
	g_signal_handler_block(btnModeDraw, handlerModeDraw);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btnModePlay), iMode == C4CNS_ModePlay);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btnModeEdit), iMode == C4CNS_ModeEdit);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btnModeDraw), iMode == C4CNS_ModeDraw);

	g_signal_handler_unblock(btnModePlay, handlerModePlay);
	g_signal_handler_unblock(btnModeEdit, handlerModeEdit);
	g_signal_handler_unblock(btnModeDraw, handlerModeDraw);
#endif // WITH_DEVELOPER_MODE / _WIN32
	return true;
}

void C4Console::EditTitle()
{
	if (Game.Network.isEnabled()) return;
	Game.Title.Open();
}

void C4Console::EditScript()
{
	if (Game.Network.isEnabled()) return;
	Game.Script.Open();
	Game.ScriptEngine.ReLink(&Game.Defs);
}

void C4Console::EditInfo()
{
	if (Game.Network.isEnabled()) return;
	Game.Info.Open();
}

void C4Console::EditObjects()
{
	ObjectListDlg.Open();
}

void C4Console::UpdateInputCtrl()
{
	int cnt;
	C4AulScriptFunc *pRef;
#ifdef _WIN32
	HWND hCombo = GetDlgItem(hWindow, IDC_COMBOINPUT);
	// Clear
	SendMessage(hCombo, CB_RESETCONTENT, 0, 0);
#elif WITH_DEVELOPER_MODE
	GtkEntryCompletion *completion = gtk_entry_get_completion(GTK_ENTRY(txtScript));
	if (!completion)
	{
		completion = gtk_entry_completion_new();
		GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
		gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(store));
		g_object_unref(G_OBJECT(store));
		gtk_entry_completion_set_text_column(completion, 0);
		gtk_entry_set_completion(GTK_ENTRY(txtScript), completion);
		g_object_unref(G_OBJECT(completion));
	}

	GtkTreeIter iter;
	GtkListStore *store = GTK_LIST_STORE(gtk_entry_completion_get_model(completion));
	gtk_list_store_clear(store);
#endif // WITH_DEVELOPER_MODE / _WIN32
	// add global and standard functions
	for (C4AulFunc *pFn = Game.ScriptEngine.GetFirstFunc(); pFn; pFn = Game.ScriptEngine.GetNextFunc(pFn))
		if (pFn->GetPublic())
		{
#ifdef _WIN32
			SendMessage(hCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>((std::string{pFn->Name} + "()").c_str()));
#elif WITH_DEVELOPER_MODE
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter, 0, pFn->Name, -1);
#endif
		}
	// Add scenario script functions
#ifdef _WIN32
	if (pRef = Game.Script.GetSFunc(0))
		SendMessage(hCombo, CB_INSERTSTRING, 0, reinterpret_cast<LPARAM>("----------"));
#endif
	for (cnt = 0; pRef = Game.Script.GetSFunc(cnt); cnt++)
	{
#ifdef _WIN32
		SendMessage(hCombo, CB_INSERTSTRING, 0, reinterpret_cast<LPARAM>((std::string{pRef->Name} + "()").c_str()));
#elif WITH_DEVELOPER_MODE
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, pRef->Name, -1);
#endif
	}
}

bool C4Console::UpdatePlayerMenu()
{
	if (!Active) return false;
	ClearPlayerMenu();
#ifdef _WIN32
	HMENU hMenu = GetSubMenu(GetMenu(hWindow), MenuIndexPlayer);
#endif
	for (C4Player *pPlr = Game.Players.First; pPlr; pPlr = pPlr->Next)
	{
		StdStrBuf sText;
		if (Game.Network.isEnabled())
			sText.Format(LoadResStr("IDS_CNS_PLRQUITNET"), pPlr->GetName(), pPlr->AtClientName);
		else
			sText.Format(LoadResStr("IDS_CNS_PLRQUIT"), pPlr->GetName());
#ifdef _WIN32
		AddMenuItem(hMenu, IDM_PLAYER_QUIT1 + pPlr->Number, sText.getData(), (!Game.Network.isEnabled() || Game.Network.isHost()) && Editing);
#elif WITH_DEVELOPER_MODE
		// TODO: Implement AddMenuItem...
		GtkWidget *menuItem = gtk_menu_item_new_with_label(Languages.IconvUtf8(sText.getData()).getData());
		gtk_menu_shell_append(GTK_MENU_SHELL(menuPlayer), menuItem);
		g_signal_connect(G_OBJECT(menuItem), "activate", G_CALLBACK(OnPlrQuit), GINT_TO_POINTER(pPlr->Number));
		gtk_widget_show(menuItem);

		gtk_widget_set_sensitive(menuItem, (!Game.Network.isEnabled() || Game.Network.isHost()) && Editing);
#endif // WITH_DEVELOPER_MODE / _WIN32
	}
	return true;
}

void C4Console::ClearPlayerMenu()
{
	if (!Active) return;
#ifdef _WIN32
	HMENU hMenu = GetSubMenu(GetMenu(hWindow), MenuIndexPlayer);
	while (DeleteMenu(hMenu, 1, MF_BYPOSITION));
#elif WITH_DEVELOPER_MODE
	GList *children = gtk_container_get_children(GTK_CONTAINER(menuPlayer));
	for (GList *item = children; item != nullptr; item = item->next)
	{
		if (item->data != plrJoin)
			gtk_container_remove(GTK_CONTAINER(menuPlayer), GTK_WIDGET(item->data));
	}
	g_list_free(children);
#endif // _WIN32
}

void C4Console::UpdateMenus()
{
	if (!Active) return;
	EnableControls(fGameOpen);
	UpdatePlayerMenu();
	UpdateViewportMenu();
	UpdateNetMenu();
}

void C4Console::PlayerJoin()
{
	// Get player file name(s)
	char c4pfile[4096 + 1] = "";
	if (!FileSelect(c4pfile, 4096,
		"Clonk 4 Player\0*.c4p\0\0",
		OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT | OFN_EXPLORER
	)) return;

	// Compose player file list
	char c4plist[6000] = "";
	// Multiple players
	if (DirectoryExists(c4pfile))
	{
		const char *cptr = c4pfile + SLen(c4pfile) + 1;
		while (*cptr)
		{
			SNewSegment(c4plist);
			SAppend(c4pfile, c4plist); SAppend(DirSep, c4plist); SAppend(cptr, c4plist);
			cptr += SLen(cptr) + 1;
		}
	}
	// Single player
	else
	{
		SAppend(c4pfile, c4plist);
	}

	// Join players (via network/ctrl queue)
	char szPlayerFilename[_MAX_PATH + 1];
	for (int iPar = 0; SCopySegment(c4plist, iPar, szPlayerFilename, ';', _MAX_PATH); iPar++)
		if (szPlayerFilename[0])
			if (Game.Network.isEnabled())
				Game.Network.Players.JoinLocalPlayer(szPlayerFilename, true);
			else
				Game.Players.CtrlJoinLocalNoNetwork(szPlayerFilename, Game.Clients.getLocalID(), Game.Clients.getLocalName());
}

#ifdef _WIN32
void C4Console::UpdateMenuText(HMENU hMenu)
{
	HMENU hSubMenu;
	if (!Active) return;
	// File
	ModifyMenu(hMenu, MenuIndexFile, MF_BYPOSITION | MF_STRING, 0, LoadResStr("IDS_MNU_FILE"));
	hSubMenu = GetSubMenu(hMenu, MenuIndexFile);
	SetMenuItemText(hSubMenu, IDM_FILE_OPEN,       LoadResStr("IDS_MNU_OPEN"));
	SetMenuItemText(hSubMenu, IDM_FILE_OPENWPLRS,  LoadResStr("IDS_MNU_OPENWPLRS"));
	SetMenuItemText(hSubMenu, IDM_FILE_RECORD,     LoadResStr("IDS_MNU_RECORD"));
	SetMenuItemText(hSubMenu, IDM_FILE_SAVE,       LoadResStr("IDS_MNU_SAVESCENARIO"));
	SetMenuItemText(hSubMenu, IDM_FILE_SAVEAS,     LoadResStr("IDS_MNU_SAVESCENARIOAS"));
	SetMenuItemText(hSubMenu, IDM_FILE_SAVEGAME,   LoadResStr("IDS_MNU_SAVEGAME"));
	SetMenuItemText(hSubMenu, IDM_FILE_SAVEGAMEAS, LoadResStr("IDS_MNU_SAVEGAMEAS"));
	SetMenuItemText(hSubMenu, IDM_FILE_CLOSE,      LoadResStr("IDS_MNU_CLOSE"));
	SetMenuItemText(hSubMenu, IDM_FILE_QUIT,       LoadResStr("IDS_MNU_QUIT"));
	// Components
	ModifyMenu(hMenu, MenuIndexComponents, MF_BYPOSITION | MF_STRING, 0, LoadResStr("IDS_MNU_COMPONENTS"));
	hSubMenu = GetSubMenu(hMenu, MenuIndexComponents);
	SetMenuItemText(hSubMenu, IDM_COMPONENTS_SCRIPT, LoadResStr("IDS_MNU_SCRIPT"));
	SetMenuItemText(hSubMenu, IDM_COMPONENTS_TITLE,  LoadResStr("IDS_MNU_TITLE"));
	SetMenuItemText(hSubMenu, IDM_COMPONENTS_INFO,   LoadResStr("IDS_MNU_INFO"));
	// Player
	ModifyMenu(hMenu, MenuIndexPlayer, MF_BYPOSITION | MF_STRING, 0, LoadResStr("IDS_MNU_PLAYER"));
	hSubMenu = GetSubMenu(hMenu, MenuIndexPlayer);
	SetMenuItemText(hSubMenu, IDM_PLAYER_JOIN, LoadResStr("IDS_MNU_JOIN"));
	// Viewport
	ModifyMenu(hMenu, MenuIndexViewport, MF_BYPOSITION | MF_STRING, 0, LoadResStr("IDS_MNU_VIEWPORT"));
	hSubMenu = GetSubMenu(hMenu, MenuIndexViewport);
	SetMenuItemText(hSubMenu, IDM_VIEWPORT_NEW, LoadResStr("IDS_MNU_NEW"));
	// Help
	hSubMenu = GetSubMenu(hMenu, MenuIndexHelp);
	SetMenuItemText(hSubMenu, IDM_HELP_ABOUT, LoadResStr("IDS_MENU_ABOUT"));
}
#endif // _WIN32

void C4Console::UpdateNetMenu()
{
	// Active & network hosting check
	if (!Active) return;
	if (!Game.Network.isHost() || !Game.Network.isEnabled()) return;
	// Clear old
	ClearNetMenu();
	// Insert menu
#ifdef _WIN32
	if (!InsertMenu(GetMenu(hWindow), MenuIndexHelp, MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(CreateMenu()), LoadResStr("IDS_MNU_NET"))) return;
#elif WITH_DEVELOPER_MODE
	itemNet = gtk_menu_item_new_with_label(LoadResStrUtf8I("IDS_MNU_NET"));
	GtkWidget *menuNet = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(itemNet), menuNet);
	gtk_menu_shell_insert(GTK_MENU_SHELL(menuBar), itemNet, MenuIndexHelp);
#endif
	MenuIndexNet = MenuIndexHelp;
	MenuIndexHelp++;

#ifdef _WIN32
	DrawMenuBar(hWindow);
	// Update menu
	HMENU hMenu = GetSubMenu(GetMenu(hWindow), MenuIndexNet);
#endif

	// Host
	const auto &text = FormatString(LoadResStr("IDS_MNU_NETHOST"), Game.Clients.getLocalName(), Game.Clients.getLocalID());
#ifdef _WIN32
	AddMenuItem(hMenu, IDM_NET_CLIENT1 + Game.Clients.getLocalID(), text.getData());
#elif WITH_DEVELOPER_MODE
	GtkWidget *item = gtk_menu_item_new_with_label(text.getData());
	gtk_menu_shell_append(GTK_MENU_SHELL(menuNet), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(OnNetClient), GINT_TO_POINTER(Game.Clients.getLocalID()));
#endif
	// Clients
	for (C4Network2Client *pClient = Game.Network.Clients.GetNextClient(nullptr); pClient; pClient = Game.Network.Clients.GetNextClient(pClient))
	{
		const auto &text = FormatString(LoadResStr(pClient->isActivated() ? "IDS_MNU_NETCLIENT" : "IDS_MNU_NETCLIENTDE"),
			pClient->getName(), pClient->getID());
#ifdef _WIN32
		AddMenuItem(hMenu, IDM_NET_CLIENT1 + pClient->getID(), text.getData());
#elif WITH_DEVELOPER_MODE
		item = gtk_menu_item_new_with_label(text.getData());
		gtk_menu_shell_append(GTK_MENU_SHELL(menuNet), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(OnNetClient), GINT_TO_POINTER(pClient->getID()));
#endif
	}
#if WITH_DEVELOPER_MODE
	gtk_widget_show_all(itemNet);
#endif
	return;
}

void C4Console::ClearNetMenu()
{
	if (!Active) return;
	if (MenuIndexNet < 0) return;
#ifdef _WIN32
	DeleteMenu(GetMenu(hWindow), MenuIndexNet, MF_BYPOSITION);
#elif WITH_DEVELOPER_MODE
	gtk_container_remove(GTK_CONTAINER(menuBar), itemNet);
	itemNet = nullptr;
#endif
	MenuIndexNet = -1;
	MenuIndexHelp--;
#ifdef _WIN32
	DrawMenuBar(hWindow);
#endif
}

void C4Console::SetCaption(const char *szCaption)
{
	if (!Active) return;
#ifdef _WIN32
	// Sorry, the window caption needs to be constant so
	// the window can be found using FindWindow
	SetTitle(C4ENGINECAPTION);
#else
	SetTitle(szCaption);
#endif
}

void C4Console::Execute()
{
	EditCursor.Execute();
#ifdef _WIN32
	PropertyDlg.Execute();
#endif
	ObjectListDlg.Execute();
	UpdateStatusBars();
	Game.GraphicsSystem.Execute();
}

bool C4Console::OpenGame(const char *szCmdLine)
{
	bool fGameWasOpen = fGameOpen;
	// Close any old game
	CloseGame();

	// Default game dependent members
	Default();
	SetCaption(GetFilename(Game.ScenarioFile.GetName()));
	// Init game dependent members
	if (!EditCursor.Init()) return false;
	// Default game - only if open before, because we do not want to default out the GUI
	if (fGameWasOpen) Game.Default();

	// Pretend to be called with a new commandline
	if (szCmdLine)
		Game.ParseCommandLine(szCmdLine);

	// PreInit is required because GUI has been deleted
	if (!Game.PreInit()) { Game.Clear(); return false; }

	// Init game
	if (!Game.Init())
	{
		Game.Clear();
		return false;
	}

	// Console updates
	fGameOpen = true;
	UpdateInputCtrl();
	EnableControls(fGameOpen);
	UpdatePlayerMenu();
	UpdateViewportMenu();

	return true;
}

bool C4Console::CloseGame()
{
	if (!fGameOpen) return false;
	Game.Clear();
	Game.GameOver = false; // No leftover values when exiting on closed game
	Game.GameOverDlgShown = false;
	fGameOpen = false;
	EnableControls(fGameOpen);
	SetCaption(LoadResStr("IDS_CNS_CONSOLE"));
	return true;
}

bool C4Console::TogglePause()
{
	return Game.TogglePause();
}

// GTK+ callbacks
#if WITH_DEVELOPER_MODE

void C4Console::OnScriptEntry(GtkWidget *entry, gpointer data)
{
	C4Console *console = static_cast<C4Console *>(data);
	console->In(gtk_entry_get_text(GTK_ENTRY(console->txtScript)));
	gtk_editable_select_region(GTK_EDITABLE(console->txtScript), 0, -1);
}

void C4Console::OnPlay(GtkWidget *button, gpointer data)
{
	static_cast<C4Console *>(data)->DoPlay();

	// Must update haltctrls even if DoPlay did noting to restore
	// previous settings since GTK may have released this toggle button
	static_cast<C4Console *>(data)->UpdateHaltCtrls(!!Game.HaltCount);
}

void C4Console::OnHalt(GtkWidget *button, gpointer data)
{
	static_cast<C4Console *>(data)->DoHalt();

	// Must update haltctrls even if DoPlay did noting to restore
	// previous settings since GTK may have released this toggle button
	static_cast<C4Console *>(data)->UpdateHaltCtrls(!!Game.HaltCount);
}

void C4Console::OnModePlay(GtkWidget *button, gpointer data)
{
	static_cast<C4Console *>(data)->EditCursor.SetMode(C4CNS_ModePlay);
}

void C4Console::OnModeEdit(GtkWidget *button, gpointer data)
{
	static_cast<C4Console *>(data)->EditCursor.SetMode(C4CNS_ModeEdit);
}

void C4Console::OnModeDraw(GtkWidget *button, gpointer data)
{
	static_cast<C4Console *>(data)->EditCursor.SetMode(C4CNS_ModeDraw);
}

void C4Console::OnFileOpen(GtkWidget *item, gpointer data)
{
	static_cast<C4Console *>(data)->FileOpen();
}

void C4Console::OnFileOpenWPlrs(GtkWidget *item, gpointer data)
{
	static_cast<C4Console *>(data)->FileOpenWPlrs();
}

void C4Console::OnFileSave(GtkWidget *item, gpointer data)
{
	static_cast<C4Console *>(data)->FileSave(false);
}

void C4Console::OnFileSaveAs(GtkWidget *item, gpointer data)
{
	static_cast<C4Console *>(data)->FileSaveAs(false);
}

void C4Console::OnFileSaveGame(GtkWidget *item, gpointer data)
{
	static_cast<C4Console *>(data)->FileSave(true);
}

void C4Console::OnFileSaveGameAs(GtkWidget *item, gpointer data)
{
	static_cast<C4Console *>(data)->FileSaveAs(true);
}

void C4Console::OnFileRecord(GtkWidget *item, gpointer data)
{
	static_cast<C4Console *>(data)->FileRecord();
}

void C4Console::OnFileClose(GtkWidget *item, gpointer data)
{
	static_cast<C4Console *>(data)->FileClose();
}

void C4Console::OnFileQuit(GtkWidget *item, gpointer data)
{
	static_cast<C4Console *>(data)->FileQuit();
}

void C4Console::OnCompObjects(GtkWidget *item, gpointer data)
{
	static_cast<C4Console *>(data)->EditObjects();
}

void C4Console::OnCompScript(GtkWidget *item, gpointer data)
{
	static_cast<C4Console *>(data)->EditScript();
}

void C4Console::OnCompTitle(GtkWidget *item, gpointer data)
{
	static_cast<C4Console *>(data)->EditTitle();
}

void C4Console::OnCompInfo(GtkWidget *item, gpointer data)
{
	static_cast<C4Console *>(data)->EditInfo();
}

void C4Console::OnPlrJoin(GtkWidget *item, gpointer data)
{
	static_cast<C4Console *>(data)->PlayerJoin();
}

void C4Console::OnPlrQuit(GtkWidget *item, gpointer data)
{
	Game.Control.Input.Add(CID_EliminatePlayer, new C4ControlEliminatePlayer(GPOINTER_TO_INT(data)));
}

void C4Console::OnViewNew(GtkWidget *item, gpointer data)
{
	static_cast<C4Console *>(data)->ViewportNew();
}

void C4Console::OnViewNewPlr(GtkWidget *item, gpointer data)
{
	Game.CreateViewport(GPOINTER_TO_INT(data));
}

void C4Console::OnHelpAbout(GtkWidget *item, gpointer data)
{
	static_cast<C4Console *>(data)->HelpAbout();
}

void C4Console::OnNetClient(GtkWidget *item, gpointer data)
{
	if (!Game.Control.isCtrlHost()) return;
	Game.Clients.CtrlRemove(Game.Clients.getClientByID(GPOINTER_TO_INT(data)), LoadResStr("IDS_MSG_KICKBYMENU"));
}

#endif // WITH_DEVELOPER_MODE
