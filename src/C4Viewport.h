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

/* A viewport to each player */

#pragma once

#include <C4Region.h>

#include <StdWindow.h>

#include <C4Shape.h>

#ifdef WITH_DEVELOPER_MODE
#include <StdGtkWindow.h>
typedef CStdGtkWindow C4ViewportBase;
#else
typedef CStdWindow C4ViewportBase;
#endif

class C4ViewportWindow : public C4ViewportBase
{
public:
	C4Viewport *cvp;
	C4ViewportWindow(C4Viewport *cvp) : cvp(cvp) {}
#ifdef _WIN32
	virtual CStdWindow *Init(CStdApp *pApp, const char *Title, CStdWindow *pParent, bool);
#elif defined(WITH_DEVELOPER_MODE)
	virtual GtkWidget *InitGUI();

	static gboolean OnKeyPressStatic(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
	static gboolean OnKeyReleaseStatic(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
	static gboolean OnScrollStatic(GtkWidget *widget, GdkEventScroll *event, gpointer user_data);
	static gboolean OnButtonPressStatic(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
	static gboolean OnButtonReleaseStatic(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
	static gboolean OnMotionNotifyStatic(GtkWidget *widget, GdkEventMotion *event, gpointer user_data);
	static gboolean OnConfigureStatic(GtkWidget *widget, GdkEventConfigure *event, gpointer user_data);
	static void OnRealizeStatic(GtkWidget *widget, gpointer user_data);
	static gboolean OnExposeStatic(GtkWidget *widget, GdkEventExpose *event, gpointer user_data);
	static void OnDragDataReceivedStatic(GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *data, guint info, guint time, gpointer user_data);

	static gboolean OnConfigureDareaStatic(GtkWidget *widget, GdkEventConfigure *event, gpointer user_data);

	static void OnVScrollStatic(GtkAdjustment *adjustment, gpointer user_data);
	static void OnHScrollStatic(GtkAdjustment *adjustment, gpointer user_data);

	GtkWidget *h_scrollbar;
	GtkWidget *v_scrollbar;
	GtkWidget *drawing_area;
#elif defined(USE_X11) && !defined(WITH_DEVELOPER_MODE)
	virtual void HandleMessage(XEvent &);
#endif
	virtual void Close();
};

class C4Viewport
{
	friend class C4GraphicsSystem;
	friend class C4MouseControl;

public:
	C4Viewport();
	~C4Viewport();
	C4RegionList Regions;
	FIXED dViewX, dViewY;
	int32_t ViewX, ViewY, ViewWdt, ViewHgt;
	int32_t BorderLeft, BorderTop, BorderRight, BorderBottom;
	int32_t ViewOffsX, ViewOffsY;
	int32_t DrawX, DrawY;
	bool fIsNoOwnerViewport; // this viewport is found for searches of NO_OWNER-viewports; even if it has a player assigned (for network obs)
	void Default();
	void Clear();
	void Execute();
	void ClearPointers(C4Object *pObj);
	void SetOutputSize(int32_t iDrawX, int32_t iDrawY, int32_t iOutX, int32_t iOutY, int32_t iOutWdt, int32_t iOutHgt);
	void UpdateViewPosition(); // update view position: Clip properly; update border variables
	bool Init(int32_t iPlayer, bool fSetTempOnly);
	bool Init(CStdWindow *pParent, CStdApp *pApp, int32_t iPlayer);
#ifdef _WIN32
	bool DropFiles(HANDLE hDrop);
#endif
	bool TogglePlayerLock();
	void NextPlayer();
	C4Rect GetOutputRect() { return C4Rect(OutX, OutY, ViewWdt, ViewHgt); }
	bool IsViewportMenu(class C4Menu *pMenu);
	C4Viewport *GetNext() { return Next; }
	int32_t GetPlayer() { return Player; }
	void CenterPosition();

protected:
	int32_t Player;
	bool PlayerLock;
	int32_t OutX, OutY;
	bool ResetMenuPositions;
	C4RegionList *SetRegions;
	C4Viewport *Next;
	CStdDDrawContext *pCtx; // rendering context
	C4ViewportWindow *pWindow;
	CClrModAddMap ClrModMap; // color modulation map for viewport drawing
	void DrawMouseButtons(C4FacetEx &cgo);
	void DrawPlayerStartup(C4FacetEx &cgo);
	void Draw(C4FacetEx &cgo, bool fDrawOverlay);
	void DrawOverlay(C4FacetEx &cgo);
	void DrawMenu(C4FacetEx &cgo);
	void DrawCursorInfo(C4FacetEx &cgo);
	void DrawPlayerInfo(C4FacetEx &cgo);
	void DrawPlayerControls(C4FacetEx &cgo);
	void BlitOutput();
	void AdjustPosition();
	bool UpdateOutputSize();
	bool ViewPositionByScrollBars();
	bool ScrollBarsByViewPosition();

#ifdef _WIN32
	friend LRESULT APIENTRY ViewportWinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif
	friend class C4ViewportWindow;
};

#define C4ViewportClassName "C4Viewport"
#define C4ViewportWindowStyle (WS_VISIBLE | WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX)
