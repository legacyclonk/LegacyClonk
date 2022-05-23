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

/* Drawing tools dialog for landscape editing in console mode */

#include <C4Include.h>
#include <C4ToolsDlg.h>
#include <C4Console.h>
#include <C4Application.h>
#include <StdRegistry.h>
#ifndef USE_CONSOLE
#include <StdGL.h>
#endif

#include "C4Wrappers.h"

#ifdef WITH_DEVELOPER_MODE

#include <C4Language.h>
#include <C4DevmodeDlg.h>

#include <gtk/gtkwindow.h>
#include <gtk/gtkimage.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkvscale.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkcombobox.h>
#include <gtk/gtkstock.h>
#include <gtk/gtk.h>

#include <res/Brush.h>
#include <res/Line.h>
#include <res/Rect.h>
#include <res/Fill.h>
#include <res/Picker.h>

#include <res/Dynamic.h>
#include <res/Static.h>
#include <res/Exact.h>

#include <res/Ift.h>
#include <res/NoIft.h>

namespace
{
	void SelectComboBoxText(GtkComboBox *combobox, const char *text)
	{
		GtkTreeModel *model = gtk_combo_box_get_model(combobox);

		GtkTreeIter iter;
		for (gboolean ret = gtk_tree_model_get_iter_first(model, &iter); ret; ret = gtk_tree_model_iter_next(model, &iter))
		{
			gchar *col_text;
			gtk_tree_model_get(model, &iter, 0, &col_text, -1);

			if (SEqualNoCase(text, col_text))
			{
				g_free(col_text);
				gtk_combo_box_set_active_iter(combobox, &iter);
				return;
			}

			g_free(col_text);
		}
	}

	gboolean RowSeparatorFunc(GtkTreeModel *model, GtkTreeIter *iter, void *user_data)
	{
		gchar *text;
		gtk_tree_model_get(model, iter, 0, &text, -1);

		if (SEqual(text, "------")) { g_free(text); return TRUE; }
		g_free(text);
		return FALSE;
	}

	GtkWidget *CreateImageFromInlinedPixbuf(const guint8 *pixbuf_data)
	{
		GdkPixbuf *pixbuf = gdk_pixbuf_new_from_inline(-1, pixbuf_data, FALSE, nullptr);
		GtkWidget *image = gtk_image_new_from_pixbuf(pixbuf);
		g_object_unref(pixbuf);
		return image;
	}
}

#endif

#ifdef _WIN32

#include <commctrl.h>

INT_PTR CALLBACK ToolsDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	int32_t iValue;
	switch (Msg)
	{
	case WM_CLOSE:
		Console.ToolsDlg.Clear();
		break;

	case WM_DESTROY:
		StoreWindowPosition(hDlg, "Property", Config.GetSubkeyPath("Console"), false);
		break;

	case WM_INITDIALOG:
		return TRUE;

	case WM_PAINT:
		PostMessage(hDlg, WM_USER, 0, 0); // For user paint
		return FALSE;

	case WM_USER:
		Console.ToolsDlg.UpdatePreview();
		return TRUE;

	case WM_VSCROLL:
		switch (LOWORD(wParam))
		{
		case SB_THUMBTRACK: case SB_THUMBPOSITION:
			iValue = HIWORD(wParam);
			Console.ToolsDlg.SetGrade(C4TLS_GradeMax - iValue);
			break;
		case SB_PAGEUP: case SB_PAGEDOWN:
		case SB_LINEUP: case SB_LINEDOWN:
			iValue = SendDlgItemMessage(hDlg, IDC_SLIDERGRADE, TBM_GETPOS, 0, 0);
			Console.ToolsDlg.SetGrade(C4TLS_GradeMax - iValue);
			break;
		}
		return TRUE;

	case WM_COMMAND:
		// Evaluate command
		switch (LOWORD(wParam))
		{
		case IDOK:
			return TRUE;

		case IDC_BUTTONMODEDYNAMIC:
			Console.ToolsDlg.SetLandscapeMode(C4LSC_Dynamic);
			return TRUE;

		case IDC_BUTTONMODESTATIC:
			Console.ToolsDlg.SetLandscapeMode(C4LSC_Static);
			return TRUE;

		case IDC_BUTTONMODEEXACT:
			Console.ToolsDlg.SetLandscapeMode(C4LSC_Exact);
			return TRUE;

		case IDC_BUTTONBRUSH:
			Console.ToolsDlg.SetTool(C4TLS_Brush, false);
			return TRUE;

		case IDC_BUTTONLINE:
			Console.ToolsDlg.SetTool(C4TLS_Line, false);
			return TRUE;

		case IDC_BUTTONRECT:
			Console.ToolsDlg.SetTool(C4TLS_Rect, false);
			return TRUE;

		case IDC_BUTTONFILL:
			Console.ToolsDlg.SetTool(C4TLS_Fill, false);
			return TRUE;

		case IDC_BUTTONPICKER:
			Console.ToolsDlg.SetTool(C4TLS_Picker, false);
			return TRUE;

		case IDC_BUTTONIFT:
			Console.ToolsDlg.SetIFT(true);
			return TRUE;

		case IDC_BUTTONNOIFT:
			Console.ToolsDlg.SetIFT(false);
			return TRUE;

		case IDC_COMBOMATERIAL:
			switch (HIWORD(wParam))
			{
			case CBN_SELCHANGE:
				int32_t cursel = SendDlgItemMessage(hDlg, IDC_COMBOMATERIAL, CB_GETCURSEL, 0, 0);
				std::string material;
				material.resize(SendDlgItemMessage(hDlg, IDC_COMBOMATERIAL, CB_GETLBTEXTLEN, cursel, 0));
				SendDlgItemMessage(hDlg, IDC_COMBOMATERIAL, CB_GETLBTEXT, cursel, reinterpret_cast<LPARAM>(material.data()));
				Console.ToolsDlg.SetMaterial(material.c_str());
				break;
			}
			return TRUE;

		case IDC_COMBOTEXTURE:
			switch (HIWORD(wParam))
			{
			case CBN_SELCHANGE:
				int32_t cursel = SendDlgItemMessage(hDlg, IDC_COMBOTEXTURE, CB_GETCURSEL, 0, 0);
				std::string texture;
				texture.resize(SendDlgItemMessage(hDlg, IDC_COMBOTEXTURE, CB_GETLBTEXTLEN, cursel, 0));
				SendDlgItemMessage(hDlg, IDC_COMBOTEXTURE, CB_GETLBTEXT, cursel, reinterpret_cast<LPARAM>(texture.data()));
				Console.ToolsDlg.SetTexture(texture.c_str());
				break;
			}
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}

#endif

C4ToolsDlg::C4ToolsDlg()
{
	Default();
#ifdef _WIN32
	hbmBrush = hbmLine = hbmRect = hbmFill = nullptr;
	hbmIFT = hbmNoIFT = nullptr;
#endif
}

C4ToolsDlg::~C4ToolsDlg()
{
	Clear();
#ifdef WITH_DEVELOPER_MODE
	if (hbox != nullptr)
	{
		g_signal_handler_disconnect(G_OBJECT(C4DevmodeDlg::GetWindow()), handlerHide);
		C4DevmodeDlg::RemovePage(hbox);
		hbox = nullptr;
	}
#endif // WITH_DEVELOPER_MODE

	// Unload bitmaps
#ifdef _WIN32
	if (hbmBrush) DeleteObject(hbmBrush);
	if (hbmLine)  DeleteObject(hbmLine);
	if (hbmRect)  DeleteObject(hbmRect);
	if (hbmFill)  DeleteObject(hbmFill);
	if (hbmIFT)   DeleteObject(hbmIFT);
	if (hbmNoIFT) DeleteObject(hbmNoIFT);
#endif
}

bool C4ToolsDlg::Open()
{
	// Create dialog window
#ifdef _WIN32
	if (hDialog) return true;
	hDialog = CreateDialog(Application.hInstance,
		MAKEINTRESOURCE(IDD_TOOLS),
		Console.hWindow,
		ToolsDlgProc);
	if (!hDialog) return false;
	// Set text
	SetWindowText(hDialog, LoadResStr("IDS_DLG_TOOLS"));
	SetDlgItemText(hDialog, IDC_STATICMATERIAL, LoadResStr("IDS_CTL_MATERIAL"));
	SetDlgItemText(hDialog, IDC_STATICTEXTURE, LoadResStr("IDS_CTL_TEXTURE"));
	// Load bitmaps if necessary
	LoadBitmaps();
	// create target ctx for OpenGL rendering
#ifndef USE_CONSOLE
	if (lpDDraw && !pGLCtx) pGLCtx = lpDDraw->CreateContext(GetDlgItem(hDialog, IDC_PREVIEW), &Application);
#endif
	// Show window
	RestoreWindowPosition(hDialog, "Property", Config.GetSubkeyPath("Console"));
	SetWindowPos(hDialog, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	ShowWindow(hDialog, SW_SHOWNORMAL | SW_SHOWNA);
#elif defined(WITH_DEVELOPER_MODE)
	if (hbox == nullptr)
	{
		hbox = gtk_hbox_new(FALSE, 12);
		GtkWidget *vbox = gtk_vbox_new(FALSE, 6);

		GtkWidget *image_brush =  CreateImageFromInlinedPixbuf(brush_pixbuf_data);
		GtkWidget *image_line =   CreateImageFromInlinedPixbuf(line_pixbuf_data);
		GtkWidget *image_rect =   CreateImageFromInlinedPixbuf(rect_pixbuf_data);
		GtkWidget *image_fill =   CreateImageFromInlinedPixbuf(fill_pixbuf_data);
		GtkWidget *image_picker = CreateImageFromInlinedPixbuf(picker_pixbuf_data);

		GtkWidget *image_dynamic = CreateImageFromInlinedPixbuf(dynamic_pixbuf_data);
		GtkWidget *image_static =  CreateImageFromInlinedPixbuf(static_pixbuf_data);
		GtkWidget *image_exact =   CreateImageFromInlinedPixbuf(exact_pixbuf_data);

		GtkWidget *image_ift =    CreateImageFromInlinedPixbuf(ift_pixbuf_data);
		GtkWidget *image_no_ift = CreateImageFromInlinedPixbuf(no_ift_pixbuf_data);

		landscape_dynamic = gtk_toggle_button_new();
		landscape_static =  gtk_toggle_button_new();
		landscape_exact =   gtk_toggle_button_new();

		gtk_container_add(GTK_CONTAINER(landscape_dynamic), image_dynamic);
		gtk_container_add(GTK_CONTAINER(landscape_static),  image_static);
		gtk_container_add(GTK_CONTAINER(landscape_exact),   image_exact);

		gtk_box_pack_start(GTK_BOX(vbox), landscape_dynamic, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), landscape_static,  FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), landscape_exact,   FALSE, FALSE, 0);

		gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
		vbox = gtk_vbox_new(FALSE, 12);
		gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE,  TRUE,  0);
		GtkWidget *local_hbox = gtk_hbox_new(FALSE, 6);

		brush =  gtk_toggle_button_new();
		line =   gtk_toggle_button_new();
		rect =   gtk_toggle_button_new();
		fill =   gtk_toggle_button_new();
		picker = gtk_toggle_button_new();

		gtk_container_add(GTK_CONTAINER(brush), image_brush);
		gtk_container_add(GTK_CONTAINER(line), image_line);
		gtk_container_add(GTK_CONTAINER(rect), image_rect);
		gtk_container_add(GTK_CONTAINER(fill), image_fill);
		gtk_container_add(GTK_CONTAINER(picker), image_picker);

		gtk_box_pack_start(GTK_BOX(local_hbox), brush,  FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(local_hbox), line,   FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(local_hbox), rect,   FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(local_hbox), fill,   FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(local_hbox), picker, FALSE, FALSE, 0);

		gtk_box_pack_start(GTK_BOX(vbox), local_hbox, FALSE, FALSE, 0);
		local_hbox = gtk_hbox_new(FALSE, 12);
		gtk_box_pack_start(GTK_BOX(vbox), local_hbox, TRUE,  TRUE,  0);

		preview = gtk_image_new();
		gtk_box_pack_start(GTK_BOX(local_hbox), preview, FALSE, FALSE, 0);

		scale = gtk_vscale_new(nullptr);
		gtk_box_pack_start(GTK_BOX(local_hbox), scale, FALSE, FALSE, 0);

		vbox = gtk_vbox_new(FALSE, 6);

		ift = gtk_toggle_button_new();
		no_ift = gtk_toggle_button_new();

		gtk_container_add(GTK_CONTAINER(ift),    image_ift);
		gtk_container_add(GTK_CONTAINER(no_ift), image_no_ift);

		gtk_box_pack_start(GTK_BOX(vbox), ift,    FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), no_ift, FALSE, FALSE, 0);

		gtk_box_pack_start(GTK_BOX(local_hbox), vbox, FALSE, FALSE, 0);

		vbox = gtk_vbox_new(FALSE, 6);

		materials = gtk_combo_box_new_text();
		textures =  gtk_combo_box_new_text();

		gtk_combo_box_set_row_separator_func(GTK_COMBO_BOX(materials), RowSeparatorFunc, nullptr, nullptr);
		gtk_combo_box_set_row_separator_func(GTK_COMBO_BOX(textures),  RowSeparatorFunc, nullptr, nullptr);

		gtk_box_pack_start(GTK_BOX(vbox), materials, TRUE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), textures,  TRUE, FALSE, 0);

		gtk_box_pack_start(GTK_BOX(local_hbox), vbox, TRUE, TRUE, 0); // ???
		gtk_widget_show_all(hbox);

		C4DevmodeDlg::AddPage(hbox, GTK_WINDOW(Console.window), LoadResStrUtf8("IDS_DLG_TOOLS").getData());

		handlerDynamic =   g_signal_connect(G_OBJECT(landscape_dynamic), "toggled",       G_CALLBACK(OnButtonModeDynamic), this);
		handlerStatic =    g_signal_connect(G_OBJECT(landscape_static),  "toggled",       G_CALLBACK(OnButtonModeStatic),  this);
		handlerExact =     g_signal_connect(G_OBJECT(landscape_exact),   "toggled",       G_CALLBACK(OnButtonModeExact),   this);
		handlerBrush =     g_signal_connect(G_OBJECT(brush),             "toggled",       G_CALLBACK(OnButtonBrush),       this);
		handlerLine =      g_signal_connect(G_OBJECT(line),              "toggled",       G_CALLBACK(OnButtonLine),        this);
		handlerRect =      g_signal_connect(G_OBJECT(rect),              "toggled",       G_CALLBACK(OnButtonRect),        this);
		handlerFill =      g_signal_connect(G_OBJECT(fill),              "toggled",       G_CALLBACK(OnButtonFill),        this);
		handlerPicker =    g_signal_connect(G_OBJECT(picker),            "toggled",       G_CALLBACK(OnButtonPicker),      this);
		handlerIft =       g_signal_connect(G_OBJECT(ift),               "toggled",       G_CALLBACK(OnButtonIft),         this);
		handlerNoIft =     g_signal_connect(G_OBJECT(no_ift),            "toggled",       G_CALLBACK(OnButtonNoIft),       this);
		handlerMaterials = g_signal_connect(G_OBJECT(materials),         "changed",       G_CALLBACK(OnComboMaterial),     this);
		handlerTextures =  g_signal_connect(G_OBJECT(textures),          "changed",       G_CALLBACK(OnComboTexture),      this);
		handlerScale =     g_signal_connect(G_OBJECT(scale),             "value-changed", G_CALLBACK(OnGrade),             this);

		handlerHide = g_signal_connect(G_OBJECT(C4DevmodeDlg::GetWindow()), "hide", G_CALLBACK(OnWindowHide), this);
	}

	C4DevmodeDlg::SwitchPage(hbox);
#endif

	Active = true;
	// Update contols
	InitGradeCtrl();
	UpdateLandscapeModeCtrls();
	UpdateToolCtrls();
	UpdateIFTControls();
	InitMaterialCtrls();
	EnableControls();
	return true;
}

void C4ToolsDlg::Default()
{
#ifdef _WIN32
	hDialog = nullptr;
#ifndef USE_CONSOLE
	pGLCtx = nullptr;
#endif
#elif defined(WITH_DEVELOPER_MODE)
	hbox = nullptr;
#endif
	Active = false;
	Tool = SelectedTool = C4TLS_Brush;
	Grade = C4TLS_GradeDefault;
	ModeIFT = true;
	SCopy("Earth", Material);
	SCopy("Rough", Texture);
}

void C4ToolsDlg::Clear()
{
#ifdef _WIN32
#ifndef USE_CONSOLE
	delete pGLCtx; pGLCtx = nullptr;
#endif
	if (hDialog) DestroyWindow(hDialog); hDialog = nullptr;
#endif
	Active = false;
}

bool C4ToolsDlg::SetTool(int32_t iTool, bool fTemp)
{
	Tool = iTool;
	if (!fTemp) SelectedTool = Tool;
	UpdateToolCtrls();
	UpdatePreview();
	return true;
}

void C4ToolsDlg::UpdateToolCtrls()
{
#ifdef _WIN32
	SendDlgItemMessage(hDialog, IDC_BUTTONBRUSH,  BM_SETSTATE, (Tool == C4TLS_Brush),  0);
	UpdateWindow(GetDlgItem(hDialog, IDC_BUTTONBRUSH));
	SendDlgItemMessage(hDialog, IDC_BUTTONLINE,   BM_SETSTATE, (Tool == C4TLS_Line),   0);
	UpdateWindow(GetDlgItem(hDialog, IDC_BUTTONLINE));
	SendDlgItemMessage(hDialog, IDC_BUTTONRECT,   BM_SETSTATE, (Tool == C4TLS_Rect),   0);
	UpdateWindow(GetDlgItem(hDialog, IDC_BUTTONRECT));
	SendDlgItemMessage(hDialog, IDC_BUTTONFILL,   BM_SETSTATE, (Tool == C4TLS_Fill),   0);
	UpdateWindow(GetDlgItem(hDialog, IDC_BUTTONFILL));
	SendDlgItemMessage(hDialog, IDC_BUTTONPICKER, BM_SETSTATE, (Tool == C4TLS_Picker), 0);
	UpdateWindow(GetDlgItem(hDialog, IDC_BUTTONPICKER));
#elif defined(WITH_DEVELOPER_MODE)
	g_signal_handler_block(brush,  handlerBrush);
	g_signal_handler_block(line,   handlerLine);
	g_signal_handler_block(rect,   handlerRect);
	g_signal_handler_block(fill,   handlerFill);
	g_signal_handler_block(picker, handlerPicker);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(brush),  Tool == C4TLS_Brush);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(line),   Tool == C4TLS_Line);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rect),   Tool == C4TLS_Rect);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fill),   Tool == C4TLS_Fill);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(picker), Tool == C4TLS_Picker);

	g_signal_handler_unblock(brush,  handlerBrush);
	g_signal_handler_unblock(line,   handlerLine);
	g_signal_handler_unblock(rect,   handlerRect);
	g_signal_handler_unblock(fill,   handlerFill);
	g_signal_handler_unblock(picker, handlerPicker);
#endif
}

void C4ToolsDlg::InitMaterialCtrls()
{
	// Materials
#ifdef _WIN32
	SendDlgItemMessage(hDialog, IDC_COMBOMATERIAL, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(C4TLS_MatSky));
	for (int32_t cnt = 0; cnt < Game.Material.Num; cnt++)
		SendDlgItemMessage(hDialog, IDC_COMBOMATERIAL, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Game.Material.Map[cnt].Name));
	SendDlgItemMessage(hDialog, IDC_COMBOMATERIAL, CB_SELECTSTRING, 0, reinterpret_cast<LPARAM>(Material));
#elif defined(WITH_DEVELOPER_MODE)
	GtkListStore *list = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(materials)));

	g_signal_handler_block(materials, handlerMaterials);
	gtk_list_store_clear(list);

	gtk_combo_box_append_text(GTK_COMBO_BOX(materials), C4TLS_MatSky);
	for (int32_t cnt = 0; cnt < Game.Material.Num; cnt++)
	{
		gtk_combo_box_append_text(GTK_COMBO_BOX(materials), Game.Material.Map[cnt].Name);
	}
	g_signal_handler_unblock(materials, handlerMaterials);
	SelectComboBoxText(GTK_COMBO_BOX(materials), Material);
#endif
	// Textures
	UpdateTextures();
}

void C4ToolsDlg::UpdateTextures()
{
	// Refill dlg
#ifdef _WIN32
	SendDlgItemMessage(hDialog, IDC_COMBOTEXTURE, CB_RESETCONTENT, 0, 0);
#elif defined(WITH_DEVELOPER_MODE)
	GtkListStore *list = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(textures)));
	gtk_list_store_clear(list);
#endif
	// bottom-most: any invalid textures
	bool fAnyEntry = false; int32_t cnt; const char *szTexture;
	if (Game.Landscape.Mode != C4LSC_Exact)
		for (cnt = 0; (szTexture = Game.TextureMap.GetTexture(cnt)); cnt++)
		{
			if (!Game.TextureMap.GetIndex(Material, szTexture, false))
			{
				fAnyEntry = true;
#ifdef _WIN32
				SendDlgItemMessage(hDialog, IDC_COMBOTEXTURE, CB_INSERTSTRING, 0, reinterpret_cast<LPARAM>(szTexture));
#elif defined(WITH_DEVELOPER_MODE)
				gtk_combo_box_prepend_text(GTK_COMBO_BOX(textures), szTexture);
#endif
			}
		}
	// separator
	if (fAnyEntry)
	{
#ifdef _WIN32
		SendDlgItemMessage(hDialog, IDC_COMBOTEXTURE, CB_INSERTSTRING, 0, reinterpret_cast<LPARAM>("-------"));
#elif defined(WITH_DEVELOPER_MODE)
		gtk_combo_box_prepend_text(GTK_COMBO_BOX(textures), "-------");
#endif
	}

	// atop: valid textures
	for (cnt = 0; (szTexture = Game.TextureMap.GetTexture(cnt)); cnt++)
	{
		// Current material-texture valid? Always valid for exact mode
		if (Game.TextureMap.GetIndex(Material, szTexture, false) || Game.Landscape.Mode == C4LSC_Exact)
		{
#ifdef _WIN32
			SendDlgItemMessage(hDialog, IDC_COMBOTEXTURE, CB_INSERTSTRING, 0, reinterpret_cast<LPARAM>(szTexture));
#elif defined(WITH_DEVELOPER_MODE)
			gtk_combo_box_prepend_text(GTK_COMBO_BOX(textures), szTexture);
#endif
		}
	}
	// reselect current
#ifdef _WIN32
	SendDlgItemMessage(hDialog, IDC_COMBOTEXTURE, CB_SELECTSTRING, 0, reinterpret_cast<LPARAM>(Texture));
#elif defined(WITH_DEVELOPER_MODE)
	g_signal_handler_block(textures, handlerTextures);
	SelectComboBoxText(GTK_COMBO_BOX(textures), Texture);
	g_signal_handler_unblock(textures, handlerTextures);
#endif
}

void C4ToolsDlg::SetMaterial(const char *szMaterial)
{
	SCopy(szMaterial, Material, C4M_MaxName);
	AssertValidTexture();
	EnableControls();
	if (Game.Landscape.Mode == C4LSC_Static) UpdateTextures();
	UpdatePreview();
}

void C4ToolsDlg::SetTexture(const char *szTexture)
{
	// assert valid (for separator selection)
	if (!Game.TextureMap.GetTexture(szTexture))
	{
		// ensure correct texture is in dlg
#ifdef _WIN32
		SendDlgItemMessage(hDialog, IDC_COMBOTEXTURE, CB_SELECTSTRING, 0, reinterpret_cast<LPARAM>(Texture));
#elif defined(WITH_DEVELOPER_MODE)
		g_signal_handler_block(textures, handlerTextures);
		SelectComboBoxText(GTK_COMBO_BOX(textures), Texture);
		g_signal_handler_unblock(textures, handlerTextures);
#endif
		return;
	}
	SCopy(szTexture, Texture, C4M_MaxName);
	UpdatePreview();
}

bool C4ToolsDlg::SetIFT(bool fIFT)
{
	if (fIFT) ModeIFT = 1; else ModeIFT = 0;
	UpdateIFTControls();
	UpdatePreview();
	return true;
}

void C4ToolsDlg::UpdatePreview()
{
#ifdef _WIN32
	if (!hDialog) return;
#elif defined(WITH_DEVELOPER_MODE)
	if (!hbox) return;
#endif

	RECT rect;
#ifdef _WIN32
	GetClientRect(GetDlgItem(hDialog, IDC_PREVIEW), &rect);
#else
	/* TODO: Set size request for image to read size from image's size request? */
	rect.left = 0;
	rect.top = 0;
	rect.bottom = 64;
	rect.right = 64;
#endif

	const auto previewWidth = static_cast<std::int32_t>(rect.right - rect.left);
	const auto previewHeight = static_cast<std::int32_t>(rect.bottom - rect.top);
	const auto surfacePreview = std::make_unique<C4Surface>(previewWidth, previewHeight);

	// fill bg
#ifdef _WIN32
	Application.DDraw->DrawBox(surfacePreview.get(), 0, 0, previewWidth - 1, previewHeight - 1, CGray4);
#endif
	uint8_t bCol = 0;
	CPattern Pattern1;
	CPattern Pattern2;
	// Sky material: sky as pattern only
	if (SEqual(Material, C4TLS_MatSky))
	{
		Pattern1.SetColors(nullptr, nullptr);
		Pattern1.Set(Game.Landscape.Sky.Surface, 0, false);
	}
	// Material-Texture
	else
	{
		bCol = Mat2PixColDefault(Game.Material.Get(Material));
		// Get/Create TexMap entry
		uint8_t iTex = Game.TextureMap.GetIndex(Material, Texture, true);
		if (iTex)
		{
			// Define texture pattern
			const C4TexMapEntry *pTex = Game.TextureMap.GetEntry(iTex);
			// Security
			if (pTex)
			{
				// Set drawing pattern
				Pattern2 = pTex->getPattern();
				// get and set extended texture of material
				C4Material *pMat = pTex->GetMaterial();
				if (pMat && !(pMat->OverlayType & C4MatOv_None))
				{
					Pattern1 = pMat->MatPattern;
				}
			}
		}
	}
#ifdef _WIN32
	if (IsWindowEnabled(GetDlgItem(hDialog, IDC_PREVIEW)))
#elif defined(WITH_DEVELOPER_MODE)
	if (GTK_WIDGET_SENSITIVE(preview))
#endif
		Application.DDraw->DrawPatternedCircle(surfacePreview.get(),
			previewWidth / 2, previewHeight / 2,
			Grade,
			bCol, Pattern1, Pattern2, *Game.Landscape.GetPal());

	Application.DDraw->AttachPrimaryPalette(surfacePreview.get());

#ifdef _WIN32
#ifndef USE_CONSOLE
	if (pGL && pGLCtx)
	{
		if (pGLCtx->Select())
		{
			pGL->Blit(surfacePreview.get(), 0, 0, static_cast<float>(previewWidth), static_cast<float>(previewHeight), pGL->lpPrimary, rect.left, rect.top, previewWidth, previewHeight);
			pGL->PageFlip(nullptr, nullptr, nullptr);
			pGL->GetMainCtx().Select();
		}
	}
#endif
#elif defined(WITH_DEVELOPER_MODE)
	// TODO: Can we optimize this?
	GdkPixbuf *pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 64, 64);
	guchar *data = gdk_pixbuf_get_pixels(pixbuf);
	surfacePreview->Lock();
	for (int x = 0; x < 64; ++x) for (int y = 0; y < 64; ++y)
	{
		uint32_t dw = surfacePreview->GetPixDw(x, y, true);
		*data = (dw >> 16) & 0xff; ++data;
		*data = (dw >> 8) & 0xff; ++data;
		*data = (dw) & 0xff; ++data;
		*data = 0xff - ((dw >> 24) & 0xff); ++data;
	}

	surfacePreview->Unlock();
	gtk_image_set_from_pixbuf(GTK_IMAGE(preview), pixbuf);
	g_object_unref(pixbuf);
#endif
}

void C4ToolsDlg::InitGradeCtrl()
{
#ifdef _WIN32
	if (!hDialog) return;
	HWND hwndTrack = GetDlgItem(hDialog, IDC_SLIDERGRADE);
	SendMessage(hwndTrack, TBM_SETPAGESIZE, 0, 5);
	SendMessage(hwndTrack, TBM_SETLINESIZE, 0, 1);
	SendMessage(hwndTrack, TBM_SETRANGE, FALSE,
		MAKELONG(C4TLS_GradeMin, C4TLS_GradeMax));
	SendMessage(hwndTrack, TBM_SETPOS, TRUE, C4TLS_GradeMax - Grade);
	UpdateWindow(hwndTrack);
#elif defined(WITH_DEVELOPER_MODE)
	if (!hbox) return;
	g_signal_handler_block(scale, handlerScale);
	gtk_range_set_increments(GTK_RANGE(scale), 1, 5);
	gtk_range_set_range(GTK_RANGE(scale), C4TLS_GradeMin, C4TLS_GradeMax);
	gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
	gtk_range_set_value(GTK_RANGE(scale), C4TLS_GradeMax - Grade);
	g_signal_handler_unblock(scale, handlerScale);
#endif
}

bool C4ToolsDlg::SetGrade(int32_t iGrade)
{
	Grade = BoundBy(iGrade, C4TLS_GradeMin, C4TLS_GradeMax);
	UpdatePreview();
	return true;
}

bool C4ToolsDlg::ChangeGrade(int32_t iChange)
{
	Grade = BoundBy(Grade + iChange, C4TLS_GradeMin, C4TLS_GradeMax);
	UpdatePreview();
	InitGradeCtrl();
	return true;
}

bool C4ToolsDlg::PopMaterial()
{
#ifdef _WIN32
	if (!hDialog) return false;
	SetFocus(GetDlgItem(hDialog, IDC_COMBOMATERIAL));
	SendDlgItemMessage(hDialog, IDC_COMBOMATERIAL, CB_SHOWDROPDOWN, TRUE, 0);
#elif defined(WITH_DEVELOPER_MODE)
	if (!hbox) return false;
	gtk_widget_grab_focus(materials);
	gtk_combo_box_popup(GTK_COMBO_BOX(materials));
#endif
	return true;
}

bool C4ToolsDlg::PopTextures()
{
#ifdef _WIN32
	if (!hDialog) return false;
	SetFocus(GetDlgItem(hDialog, IDC_COMBOTEXTURE));
	SendDlgItemMessage(hDialog, IDC_COMBOTEXTURE, CB_SHOWDROPDOWN, TRUE, 0);
#elif defined(WITH_DEVELOPER_MODE)
	if (!hbox) return false;
	gtk_widget_grab_focus(textures);
	gtk_combo_box_popup(GTK_COMBO_BOX(textures));
#endif
	return true;
}

void C4ToolsDlg::UpdateIFTControls()
{
#ifdef _WIN32
	if (!hDialog) return;
	SendDlgItemMessage(hDialog, IDC_BUTTONNOIFT, BM_SETSTATE, (ModeIFT == 0), 0);
	UpdateWindow(GetDlgItem(hDialog, IDC_BUTTONNOIFT));
	SendDlgItemMessage(hDialog, IDC_BUTTONIFT,   BM_SETSTATE, (ModeIFT == 1), 0);
	UpdateWindow(GetDlgItem(hDialog, IDC_BUTTONIFT));
#elif defined(WITH_DEVELOPER_MODE)
	if (!hbox) return;
	g_signal_handler_block(no_ift, handlerNoIft);
	g_signal_handler_block(ift,    handlerIft);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(no_ift), ModeIFT == 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ift),    ModeIFT == 1);

	g_signal_handler_unblock(no_ift, handlerNoIft);
	g_signal_handler_unblock(ift,    handlerIft);
#endif
}

void C4ToolsDlg::UpdateLandscapeModeCtrls()
{
	int32_t iMode = Game.Landscape.Mode;
#ifdef _WIN32
	// Dynamic: enable only if dynamic anyway
	SendDlgItemMessage(hDialog, IDC_BUTTONMODEDYNAMIC, BM_SETSTATE, (iMode == C4LSC_Dynamic), 0);
	EnableWindow(GetDlgItem(hDialog, IDC_BUTTONMODEDYNAMIC), (iMode == C4LSC_Dynamic));
	UpdateWindow(GetDlgItem(hDialog, IDC_BUTTONMODEDYNAMIC));
	// Static: enable only if map available
	SendDlgItemMessage(hDialog, IDC_BUTTONMODESTATIC, BM_SETSTATE, (iMode == C4LSC_Static), 0);
	EnableWindow(GetDlgItem(hDialog, IDC_BUTTONMODESTATIC), (Game.Landscape.Map != nullptr));
	UpdateWindow(GetDlgItem(hDialog, IDC_BUTTONMODESTATIC));
	// Exact: enable always
	SendDlgItemMessage(hDialog, IDC_BUTTONMODEEXACT, BM_SETSTATE, (iMode == C4LSC_Exact), 0);
	UpdateWindow(GetDlgItem(hDialog, IDC_BUTTONMODEEXACT));
	// Set dialog caption
	SetWindowText(hDialog, LoadResStr(iMode == C4LSC_Dynamic ? "IDS_DLG_DYNAMIC" : iMode == C4LSC_Static ? "IDS_DLG_STATIC" : "IDS_DLG_EXACT"));
#elif defined(WITH_DEVELOPER_MODE)
	g_signal_handler_block(landscape_dynamic, handlerDynamic);
	g_signal_handler_block(landscape_static,  handlerStatic);
	g_signal_handler_block(landscape_exact,   handlerExact);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(landscape_dynamic), iMode == C4LSC_Dynamic);
	gtk_widget_set_sensitive(landscape_dynamic, iMode == C4LSC_Dynamic);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(landscape_static), iMode == C4LSC_Static);
	gtk_widget_set_sensitive(landscape_static, Game.Landscape.Map != nullptr);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(landscape_exact), iMode == C4LSC_Exact);

	g_signal_handler_unblock(landscape_dynamic, handlerDynamic);
	g_signal_handler_unblock(landscape_static,  handlerStatic);
	g_signal_handler_unblock(landscape_exact,   handlerExact);

	C4DevmodeDlg::SetTitle(hbox, LoadResStrUtf8(iMode == C4LSC_Dynamic ? "IDS_DLG_DYNAMIC" : iMode == C4LSC_Static ? "IDS_DLG_STATIC" : "IDS_DLG_EXACT").getData());
#endif
}

bool C4ToolsDlg::SetLandscapeMode(int32_t iMode, bool fThroughControl)
{
	int32_t iLastMode = Game.Landscape.Mode;
	// Exact to static: confirm data loss warning
	if (iLastMode == C4LSC_Exact)
		if (iMode == C4LSC_Static)
			if (!fThroughControl)
				if (!Console.Message(LoadResStr("IDS_CNS_EXACTTOSTATIC"), true))
					return false;
	// send as control
	if (!fThroughControl)
	{
		Game.Control.DoInput(CID_EMDrawTool, new C4ControlEMDrawTool(EMDT_SetMode, iMode), CDT_Decide);
		return true;
	}
	// Set landscape mode
	Game.Landscape.SetMode(iMode);
	// Exact to static: redraw landscape from map
	if (iLastMode == C4LSC_Exact)
		if (iMode == C4LSC_Static)
			Game.Landscape.MapToLandscape();
	// Assert valid tool
	if (iMode != C4LSC_Exact)
		if (SelectedTool == C4TLS_Fill)
			SetTool(C4TLS_Brush, false);
	// Update controls
	UpdateLandscapeModeCtrls();
	EnableControls();
	UpdateTextures();
	// Success
	return true;
}

void C4ToolsDlg::EnableControls()
{
	int32_t iLandscapeMode = Game.Landscape.Mode;
#ifdef _WIN32
	// Set bitmap buttons
	SendDlgItemMessage(hDialog, IDC_BUTTONBRUSH,       BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>((iLandscapeMode >= C4LSC_Static) ? hbmBrush  : hbmBrush2));
	SendDlgItemMessage(hDialog, IDC_BUTTONLINE,        BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>((iLandscapeMode >= C4LSC_Static) ? hbmLine   : hbmLine2));
	SendDlgItemMessage(hDialog, IDC_BUTTONRECT,        BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>((iLandscapeMode >= C4LSC_Static) ? hbmRect   : hbmRect2));
	SendDlgItemMessage(hDialog, IDC_BUTTONFILL,        BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>((iLandscapeMode >= C4LSC_Exact)  ? hbmFill   : hbmFill2));
	SendDlgItemMessage(hDialog, IDC_BUTTONPICKER,      BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>((iLandscapeMode >= C4LSC_Static) ? hbmPicker : hbmPicker2));
	SendDlgItemMessage(hDialog, IDC_BUTTONIFT,         BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(hbmIFT));
	SendDlgItemMessage(hDialog, IDC_BUTTONNOIFT,       BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(hbmNoIFT));
	SendDlgItemMessage(hDialog, IDC_BUTTONMODEDYNAMIC, BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(hbmDynamic));
	SendDlgItemMessage(hDialog, IDC_BUTTONMODESTATIC,  BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(hbmStatic));
	SendDlgItemMessage(hDialog, IDC_BUTTONMODEEXACT,   BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(hbmExact));
	// Enable drawing controls
	EnableWindow(GetDlgItem(hDialog, IDC_BUTTONBRUSH),    (iLandscapeMode >= C4LSC_Static));
	EnableWindow(GetDlgItem(hDialog, IDC_BUTTONLINE),     (iLandscapeMode >= C4LSC_Static));
	EnableWindow(GetDlgItem(hDialog, IDC_BUTTONRECT),     (iLandscapeMode >= C4LSC_Static));
	EnableWindow(GetDlgItem(hDialog, IDC_BUTTONFILL),     (iLandscapeMode >= C4LSC_Exact));
	EnableWindow(GetDlgItem(hDialog, IDC_BUTTONPICKER),   (iLandscapeMode >= C4LSC_Static));
	EnableWindow(GetDlgItem(hDialog, IDC_BUTTONIFT),      (iLandscapeMode >= C4LSC_Static));
	EnableWindow(GetDlgItem(hDialog, IDC_BUTTONNOIFT),    (iLandscapeMode >= C4LSC_Static));
	EnableWindow(GetDlgItem(hDialog, IDC_COMBOMATERIAL),  (iLandscapeMode >= C4LSC_Static));
	EnableWindow(GetDlgItem(hDialog, IDC_COMBOTEXTURE),   (iLandscapeMode >= C4LSC_Static) && !SEqual(Material, C4TLS_MatSky));
	EnableWindow(GetDlgItem(hDialog, IDC_STATICMATERIAL), (iLandscapeMode >= C4LSC_Static));
	EnableWindow(GetDlgItem(hDialog, IDC_STATICTEXTURE),  (iLandscapeMode >= C4LSC_Static) && !SEqual(Material, C4TLS_MatSky));
	EnableWindow(GetDlgItem(hDialog, IDC_SLIDERGRADE),    (iLandscapeMode >= C4LSC_Static));
	EnableWindow(GetDlgItem(hDialog, IDC_PREVIEW),        (iLandscapeMode >= C4LSC_Static));
#elif defined(WITH_DEVELOPER_MODE)
	gtk_widget_set_sensitive(brush,     iLandscapeMode >= C4LSC_Static);
	gtk_widget_set_sensitive(line,      iLandscapeMode >= C4LSC_Static);
	gtk_widget_set_sensitive(rect,      iLandscapeMode >= C4LSC_Static);
	gtk_widget_set_sensitive(fill,      iLandscapeMode >= C4LSC_Exact);
	gtk_widget_set_sensitive(picker,    iLandscapeMode >= C4LSC_Static);
	gtk_widget_set_sensitive(ift,       iLandscapeMode >= C4LSC_Static);
	gtk_widget_set_sensitive(no_ift,    iLandscapeMode >= C4LSC_Static);
	gtk_widget_set_sensitive(materials, iLandscapeMode >= C4LSC_Static);
	gtk_widget_set_sensitive(textures,  iLandscapeMode >= C4LSC_Static && !SEqual(Material, C4TLS_MatSky));
	gtk_widget_set_sensitive(scale,     iLandscapeMode >= C4LSC_Static);
	gtk_widget_set_sensitive(preview,   iLandscapeMode >= C4LSC_Static);
#endif // _WIN32/WITH_DEVELOPER_MODE
	UpdatePreview();
}

#ifdef _WIN32
void C4ToolsDlg::LoadBitmaps()
{
	HINSTANCE hInst = Application.hInstance;
	if (!hbmBrush)   hbmBrush =   LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BRUSH));
	if (!hbmLine)    hbmLine =    LoadBitmap(hInst, MAKEINTRESOURCE(IDB_LINE));
	if (!hbmRect)    hbmRect =    LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RECT));
	if (!hbmFill)    hbmFill =    LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FILL));
	if (!hbmPicker)  hbmPicker =  LoadBitmap(hInst, MAKEINTRESOURCE(IDB_PICKER));
	if (!hbmBrush2)  hbmBrush2 =  LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BRUSH2));
	if (!hbmLine2)   hbmLine2 =   LoadBitmap(hInst, MAKEINTRESOURCE(IDB_LINE2));
	if (!hbmRect2)   hbmRect2 =   LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RECT2));
	if (!hbmFill2)   hbmFill2 =   LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FILL2));
	if (!hbmPicker2) hbmPicker2 = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_PICKER2));
	if (!hbmIFT)     hbmIFT =     LoadBitmap(hInst, MAKEINTRESOURCE(IDB_IFT));
	if (!hbmNoIFT)   hbmNoIFT =   LoadBitmap(hInst, MAKEINTRESOURCE(IDB_NOIFT));
	if (!hbmDynamic) hbmDynamic = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_DYNAMIC));
	if (!hbmStatic)  hbmStatic =  LoadBitmap(hInst, MAKEINTRESOURCE(IDB_STATIC));
	if (!hbmExact)   hbmExact =   LoadBitmap(hInst, MAKEINTRESOURCE(IDB_EXACT));
}
#endif

void C4ToolsDlg::AssertValidTexture()
{
	// Static map mode only
	if (Game.Landscape.Mode != C4LSC_Static) return;
	// Ignore if sky
	if (SEqual(Material, C4TLS_MatSky)) return;
	// Current material-texture valid
	if (Game.TextureMap.GetIndex(Material, Texture, false)) return;
	// Find valid material-texture
	const char *szTexture;
	for (int32_t iTexture = 0; szTexture = Game.TextureMap.GetTexture(iTexture); iTexture++)
	{
		if (Game.TextureMap.GetIndex(Material, szTexture, false))
		{
			SelectTexture(szTexture); return;
		}
	}
	// No valid texture found
}

bool C4ToolsDlg::SelectTexture(const char *szTexture)
{
#ifdef _WIN32
	SendDlgItemMessage(hDialog, IDC_COMBOTEXTURE, CB_SELECTSTRING, 0, reinterpret_cast<LPARAM>(szTexture));
#elif defined(WITH_DEVELOPER_MODE)
	g_signal_handler_block(textures, handlerTextures);
	SelectComboBoxText(GTK_COMBO_BOX(textures), szTexture);
	g_signal_handler_unblock(textures, handlerTextures);
#endif
	SetTexture(szTexture);
	return true;
}

bool C4ToolsDlg::SelectMaterial(const char *szMaterial)
{
#ifdef _WIN32
	SendDlgItemMessage(hDialog, IDC_COMBOMATERIAL, CB_SELECTSTRING, 0, reinterpret_cast<LPARAM>(szMaterial));
#elif defined(WITH_DEVELOPER_MODE)
	g_signal_handler_block(materials, handlerMaterials);
	SelectComboBoxText(GTK_COMBO_BOX(materials), szMaterial);
	g_signal_handler_unblock(materials, handlerMaterials);
#endif
	SetMaterial(szMaterial);
	return true;
}

void C4ToolsDlg::SetAlternateTool()
{
	// alternate tool is the picker in any mode
	SetTool(C4TLS_Picker, true);
}

void C4ToolsDlg::ResetAlternateTool()
{
	// reset tool to selected tool in case alternate tool was set
	SetTool(SelectedTool, true);
}

#ifdef WITH_DEVELOPER_MODE

// GTK+ callbacks

void C4ToolsDlg::OnButtonModeDynamic(GtkWidget *widget, gpointer data)
{
	static_cast<C4ToolsDlg *>(data)->SetLandscapeMode(C4LSC_Dynamic);
}

void C4ToolsDlg::OnButtonModeStatic(GtkWidget *widget, gpointer data)
{
	static_cast<C4ToolsDlg *>(data)->SetLandscapeMode(C4LSC_Static);
}

void C4ToolsDlg::OnButtonModeExact(GtkWidget *widget, gpointer data)
{
	static_cast<C4ToolsDlg *>(data)->SetLandscapeMode(C4LSC_Exact);
}

void C4ToolsDlg::OnButtonBrush(GtkWidget *widget, gpointer data)
{
	static_cast<C4ToolsDlg *>(data)->SetTool(C4TLS_Brush, false);
}

void C4ToolsDlg::OnButtonLine(GtkWidget *widget, gpointer data)
{
	static_cast<C4ToolsDlg *>(data)->SetTool(C4TLS_Line, false);
}

void C4ToolsDlg::OnButtonRect(GtkWidget *widget, gpointer data)
{
	static_cast<C4ToolsDlg *>(data)->SetTool(C4TLS_Rect, false);
}

void C4ToolsDlg::OnButtonFill(GtkWidget *widget, gpointer data)
{
	static_cast<C4ToolsDlg *>(data)->SetTool(C4TLS_Fill, false);
}

void C4ToolsDlg::OnButtonPicker(GtkWidget *widget, gpointer data)
{
	static_cast<C4ToolsDlg *>(data)->SetTool(C4TLS_Picker, false);
}

void C4ToolsDlg::OnButtonIft(GtkWidget *widget, gpointer data)
{
	static_cast<C4ToolsDlg *>(data)->SetIFT(true);
}

void C4ToolsDlg::OnButtonNoIft(GtkWidget *widget, gpointer data)
{
	static_cast<C4ToolsDlg *>(data)->SetIFT(false);
}

void C4ToolsDlg::OnComboMaterial(GtkWidget *widget, gpointer data)
{
	gchar *text = gtk_combo_box_get_active_text(GTK_COMBO_BOX(widget));
	static_cast<C4ToolsDlg *>(data)->SetMaterial(text);
	g_free(text);
}

void C4ToolsDlg::OnComboTexture(GtkWidget *widget, gpointer data)
{
	gchar *text = gtk_combo_box_get_active_text(GTK_COMBO_BOX(widget));
	static_cast<C4ToolsDlg *>(data)->SetTexture(text);
	g_free(text);
}

void C4ToolsDlg::OnGrade(GtkWidget *widget, gpointer data)
{
	C4ToolsDlg *dlg = static_cast<C4ToolsDlg *>(data);
	int value = static_cast<int>(gtk_range_get_value(GTK_RANGE(dlg->scale)) + 0.5);
	dlg->SetGrade(C4TLS_GradeMax - value);
}

void C4ToolsDlg::OnWindowHide(GtkWidget *widget, gpointer data)
{
	static_cast<C4ToolsDlg *>(data)->Active = false;
}

#endif
