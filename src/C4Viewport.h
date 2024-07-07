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

/* A viewport to each player */

#pragma once

#include "C4ForwardDeclarations.h"
#include "C4Rect.h"
#include <C4Region.h>
#include "Fixed.h"
#include "StdColors.h"
#include <StdWindow.h>

#ifdef WITH_DEVELOPER_MODE
#include <StdGtkWindow.h>
typedef CStdGtkWindow C4ViewportBase;
#else
typedef CStdWindow C4ViewportBase;
#endif

class C4Viewport;

class C4ViewportWindow : public C4ViewportBase
{
#ifdef _WIN32
public:
	static constexpr auto WindowStyle = WS_VISIBLE | WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX;
#endif
public:
	C4Viewport *cvp;
	C4ViewportWindow(C4Viewport *cvp) : cvp(cvp) {}
#ifdef _WIN32
	std::pair<DWORD, DWORD> GetWindowStyle() const override { return {WindowStyle, WS_EX_ACCEPTFILES}; }
	WNDCLASSEX GetWindowClass(HINSTANCE instance) const override;
	bool GetPositionData(std::string &id, std::string &subKey, bool &storeSize) const override;
#elif defined(WITH_DEVELOPER_MODE)
	virtual GtkWidget *InitGUI() override;

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
	bool HideCursor() const override { return true; }
	virtual void HandleMessage(XEvent &) override;
#endif
	virtual void Close() override;

#ifdef _WIN32
	static LRESULT APIENTRY WinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif
};

class C4Viewport
{
	friend class C4GraphicsSystem;
	friend class C4MouseControl;

public:
	C4Viewport();
	~C4Viewport();
	C4RegionList Regions;
	C4Fixed dViewX, dViewY;
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
	int32_t GetPlayer() { return Player; }
	void CenterPosition();
	C4Section &GetViewSection();
	C4Section &GetViewRootSection();

	auto MapPointToChildSectionPoint(const std::int32_t x, const std::int32_t y)
	{
		return GetViewRootSection().PointToChildPoint(x, y);
	}

protected:
	int32_t Player;
	bool PlayerLock;
	int32_t OutX, OutY;
	bool ResetMenuPositions;
	C4RegionList *SetRegions;
	CStdGLCtx *pCtx; // rendering context for OpenGL
	C4ViewportWindow *pWindow;
	CClrModAddMap ClrModMap; // color modulation map for viewport drawing
	void DrawMouseButtons(C4FacetEx &cgo);
	void DrawPlayerStartup(C4FacetEx &cgo);
	void Draw(C4FacetEx &cgo, bool fDrawOverlay);
	void DrawSection(C4FacetEx &cgo, C4Section &section, C4Player *plr, bool fowEnabled);
	void DrawParallaxObjects(C4FacetEx &cgo, C4Section &section);
	void DrawOverlay(C4FacetEx &cgo, C4Section &viewSection);
	void DrawMenu(C4FacetEx &cgo);
	void DrawCursorInfo(C4FacetEx &cgo);
	void DrawPlayerInfo(C4FacetEx &cgo, C4Section &section);
	void DrawPlayerControls(C4FacetEx &cgo);
	void BlitOutput();
	void AdjustPosition();
	bool UpdateOutputSize();
	bool ViewPositionByScrollBars();
	bool ScrollBarsByViewPosition();

	friend class C4ViewportWindow;
};
