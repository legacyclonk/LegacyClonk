/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2005, Günther
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

/* A wrapper class to OS dependent event and window interfaces, X11 version */

#include <Standard.h>
#include <StdApp.h>
#include <StdWindow.h>
#include <StdGL.h>
#include <StdDDraw2.h>
#include <StdFile.h>
#include <StdBuf.h>

#ifdef USE_X11
#include "res/lc.xpm"
#include <GL/glx.h>
#include <X11/Xlib.h>
#include <X11/xpm.h>
#include <X11/Xatom.h>
#include <X11/extensions/xf86vmode.h>
#endif

#ifdef WITH_GLIB
#include <glib.h>
#endif

#include <string>
#include <map>
#include <sstream>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

/* CStdWindow */

CStdWindow::~CStdWindow()
{
	Clear();
}

bool CStdWindow::Init(CStdApp *const app, const char *const title, const C4Rect &bounds, CStdWindow *const parent)
{
#ifndef USE_X11
	return true;
#else
	Active = true;
	dpy = app->dpy;

	if (!FindInfo()) return false;

	// Various properties
	XSetWindowAttributes attr;
	attr.border_pixel = 0;
	attr.background_pixel = 0;
	// Which events we want to receive
	attr.event_mask =
		StructureNotifyMask |
		FocusChangeMask |
		KeyPressMask |
		KeyReleaseMask |
		PointerMotionMask |
		ButtonPressMask |
		ButtonReleaseMask;
	attr.colormap = XCreateColormap(dpy, DefaultRootWindow(dpy), static_cast<XVisualInfo *>(Info)->visual, AllocNone);
	unsigned long attrmask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
	Pixmap bitmap;
	const bool hideCursor{HideCursor()};
	if (hideCursor)
	{
		// Hide the mouse cursor
		// We do not care what color the invisible cursor has
		XColor cursor_color{};
		bitmap = XCreateBitmapFromData(dpy, DefaultRootWindow(dpy), "\000", 1, 1);
		attr.cursor = XCreatePixmapCursor(dpy, bitmap, bitmap, &cursor_color, &cursor_color, 0, 0);
		attrmask |= CWCursor;
	}

	wnd = XCreateWindow(dpy, DefaultRootWindow(dpy),
		0, 0, 640, 480, 0, static_cast<XVisualInfo *>(Info)->depth, InputOutput, static_cast<XVisualInfo *>(Info)->visual,
		attrmask, &attr);
	if (hideCursor)
	{
		XFreeCursor(dpy, attr.cursor);
		XFreePixmap(dpy, bitmap);
	}
	if (!wnd)
	{
		Log("Error creating window.");
		return false;
	}
	// Update the XWindow->CStdWindow-Map
	app->NewWindow(this);
	if (!app->inputContext && app->inputMethod)
	{
		app->inputContext = XCreateIC(app->inputMethod,
			XNClientWindow, wnd,
			XNFocusWindow, wnd,
			XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
			XNResourceName, STD_PRODUCT,
			XNResourceClass, STD_PRODUCT,
			nullptr);
		if (!app->inputContext)
		{
			Log("Failed to create input context.");
			XCloseIM(app->inputMethod);
			app->inputMethod = nullptr;
		}
		else
		{
			long ic_event_mask;
			if (XGetICValues(app->inputContext, XNFilterEvents, &ic_event_mask, nullptr) == nullptr)
				attr.event_mask |= ic_event_mask;
			XSelectInput(dpy, wnd, attr.event_mask);
			XSetICFocus(app->inputContext);
		}
	}
	// We want notification of closerequests and be killed if we hang
	Atom WMProtocols[2];
	const char *WMProtocolnames[] = { "WM_DELETE_WINDOW", "_NET_WM_PING" };
	XInternAtoms(dpy, const_cast<char **>(WMProtocolnames), 2, false, WMProtocols);
	XSetWMProtocols(dpy, wnd, WMProtocols, 2);
	// Let the window manager know our pid so it can kill us
	Atom PID = XInternAtom(app->dpy, "_NET_WM_PID", false);
	int32_t pid = getpid();
	if (PID != None) XChangeProperty(app->dpy, wnd, PID, XA_CARDINAL, 32, PropModeReplace, reinterpret_cast<const unsigned char *>(&pid), 1);
	// Title and stuff
	XTextProperty title_property;
	XStringListToTextProperty(const_cast<char **>(&title), 1, &title_property);
	// State and Icon
	XWMHints *wm_hint = XAllocWMHints();
	wm_hint->flags = StateHint | InputHint | IconPixmapHint | IconMaskHint;
	wm_hint->initial_state = NormalState;
	wm_hint->input = True;
	// Trust XpmCreatePixmapFromData to not modify the xpm...
	XpmCreatePixmapFromData(dpy, wnd, const_cast<char **>(c4x_xpm), &wm_hint->icon_pixmap, &wm_hint->icon_mask, nullptr);
	// Window class
	XClassHint *class_hint = XAllocClassHint();
	class_hint->res_name = const_cast<char *>(STD_PRODUCT);
	class_hint->res_class = const_cast<char *>(STD_PRODUCT);
	XSetWMProperties(dpy, wnd, &title_property, &title_property, app->argv, app->argc, nullptr, wm_hint, class_hint);
	// Set "parent". Clonk does not use "real" parent windows, but multiple toplevel windows.
	if (parent) XSetTransientForHint(dpy, wnd, parent->wnd);
	// Show window
	XMapWindow(dpy, wnd);
	// Clean up
	XFree(title_property.value);
	Hints = wm_hint;
	XFree(class_hint);

	// Render into whole window
	renderwnd = wnd;

	return true;
#endif // USE_X11
}

void CStdWindow::Clear()
{
#ifdef USE_X11
	// Destroy window
	if (wnd)
	{
		XUnmapWindow(dpy, wnd);
		XDestroyWindow(dpy, wnd);
		if (Info) XFree(Info);
		if (Hints) XFree(Hints);

		// Might be necessary when the last window is closed
		XFlush(dpy);
	}
	wnd = renderwnd = 0;
#endif
}

#ifdef USE_X11
bool CStdWindow::FindInfo()
{
#ifndef USE_CONSOLE
	// get an appropriate visual
	// attributes for a single buffered visual in RGBA format with at least 4 bits per color
	static int attrListSgl[] = { GLX_RGBA,
		GLX_RED_SIZE, 4, GLX_GREEN_SIZE, 4, GLX_BLUE_SIZE, 4,
		None
	};
	// attributes for a double buffered visual in RGBA format with at least 4 bits per color
	static int attrListDbl[] = { GLX_RGBA, GLX_DOUBLEBUFFER,
		GLX_RED_SIZE, 4, GLX_GREEN_SIZE, 4, GLX_BLUE_SIZE, 4,
		GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
		None
	};
	// doublebuffered is the best
	Info = glXChooseVisual(dpy, DefaultScreen(dpy), attrListDbl);
	if (!Info)
	{
		Log("  gl: no doublebuffered visual.");
		// a singlebuffered is probably better than the default
		Info = glXChooseVisual(dpy, DefaultScreen(dpy), attrListSgl);
	}
#endif
	if (!Info)
	{
		Log("  gl: no singlebuffered visual, either.");
		// just try to get the default
		XVisualInfo vitmpl; int blub;
		vitmpl.visual = DefaultVisual(dpy, DefaultScreen(dpy));
		vitmpl.visualid = XVisualIDFromVisual(vitmpl.visual);
		Info = XGetVisualInfo(dpy, VisualIDMask, &vitmpl, &blub);
	}
	if (!Info)
	{
		Log("  gl: no visual at all.");
		return false;
	}

	return true;
}
#endif // USE_X11

bool CStdWindow::GetSize(C4Rect &rect)
{
#ifdef USE_X11
	Window winDummy;
	unsigned int borderDummy;
	int x, y;
	unsigned int width, height;
	unsigned int depth;
	XGetGeometry(dpy, wnd, &winDummy, &x, &y,
		&width, &height, &borderDummy, &depth);
	rect.x = x;
	rect.y = y;
	rect.Wdt = width;
	rect.Hgt = height;
#else
	rect = {0, 0, 0, 0};
#endif
	return true;
}

void CStdWindow::SetSize(const unsigned int X, const unsigned int Y)
{
#ifdef USE_X11
	XResizeWindow(dpy, wnd, X, Y);
#endif
}

void CStdWindow::SetTitle(const char *const Title)
{
#ifdef USE_X11
	XTextProperty title_property;
	XStringListToTextProperty(const_cast<char **>(&Title), 1, &title_property);
	XSetWMName(dpy, wnd, &title_property);
#endif
}

void CStdWindow::FlashWindow()
{
#ifdef USE_X11
	if (!HasFocus)
	{
		XWMHints *wm_hint = static_cast<XWMHints *>(Hints);
		wm_hint->flags |= XUrgencyHint;
		XSetWMHints(dpy, wnd, wm_hint);
	}
#endif
}

#ifdef USE_X11
void CStdWindow::HandleMessage(XEvent &event)
{
	if (event.type == FocusIn)
	{
		HasFocus = true;

		// Clear urgency flag
		XWMHints *wm_hint = static_cast<XWMHints *>(Hints);
		if (wm_hint->flags & XUrgencyHint)
		{
			wm_hint->flags &= ~XUrgencyHint;
			XSetWMHints(dpy, wnd, wm_hint);
		}
	}
	else if (event.type == FocusOut)
	{
		int detail = reinterpret_cast<XFocusChangeEvent *>(&event)->detail;

		// StdGtkWindow gets two FocusOut events, one of which comes
		// directly after a FocusIn event even when the window has
		// focus. For these FocusOut events, detail is set to
		// NotifyInferior which is why we are ignoring it here.
		if (detail != NotifyInferior)
		{
			HasFocus = false;
		}
	}
}
#endif

void CStdWindow::SetDisplayMode(const DisplayMode mode)
{
#ifdef USE_X11
	const auto fullscreen = mode == DisplayMode::Fullscreen;

	static Atom atoms[4];
	static const char *names[] = {"_NET_WM_STATE", "_NET_WM_STATE_FULLSCREEN", "_NET_WM_MAXIMIZED_VERT", "_NET_WM_MAXIMIZED_HORZ"};
	if (!atoms[0]) XInternAtoms(dpy, const_cast<char **>(names), 4, false, atoms);
	XEvent e;
	e.xclient.type = ClientMessage;
	e.xclient.window = wnd;
	e.xclient.message_type = atoms[0];
	e.xclient.format = 32;
	e.xclient.data.l[0] = fullscreen ? 1 : 0; // _NET_WM_STATE_ADD : _NET_WM_STATE_DELETE
	e.xclient.data.l[1] = atoms[1];
	e.xclient.data.l[2] = 0; // second property to alter
	e.xclient.data.l[3] = 1; // source indication
	e.xclient.data.l[4] = 0;
	XSendEvent(dpy, DefaultRootWindow(dpy), false, SubstructureNotifyMask | SubstructureRedirectMask, &e);
#endif
}

void CStdWindow::SetProgress(uint32_t) {} // stub
