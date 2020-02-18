/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2005, Günther
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

/* A wrapper class to OS dependent event and window interfaces, X11 version */

#include <Standard.h>
#include <StdWindow.h>
#include <StdGL.h>
#include <StdDDraw2.h>
#include <StdFile.h>
#include <StdBuf.h>

#ifdef USE_X11
#include "res/lc.xpm"
#include <X11/Xlib.h>
#include <X11/xpm.h>
#include <X11/Xatom.h>
#include <X11/extensions/xf86vmode.h>
#endif

#include <string>
#include <map>
#include <sstream>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

#ifdef WITH_GLIB
#include <glib.h>
#endif

#include "StdXPrivate.h"

/* CStdWindow */

CStdWindow::CStdWindow() :
	Active(false)
#ifdef USE_X11
	, wnd(0), renderwnd(0), dpy(nullptr), Hints(nullptr), HasFocus(false)
#endif
{}

CStdWindow::~CStdWindow()
{
	Clear();
}

CStdWindow *CStdWindow::Init(CStdApp *pApp)
{
	return Init(pApp, STD_PRODUCT);
}

CStdWindow *CStdWindow::Init(CStdApp *pApp, const char *Title, CStdWindow *pParent, bool HideCursor)
{
#ifndef USE_X11
	return this;
#else
	Active = true;
	dpy = pApp->dpy;

	if (!FindFBConfig()) return nullptr;

	XVisualInfo *info = glXGetVisualFromFBConfig(dpy, FBConfig);

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
	attr.colormap = XCreateColormap(dpy, DefaultRootWindow(dpy), info->visual, AllocNone);
	unsigned long attrmask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
	Pixmap bitmap;
	if (HideCursor)
	{
		// Hide the mouse cursor
		// We do not care what color the invisible cursor has
		XColor cursor_color{};
		bitmap = XCreateBitmapFromData(dpy, DefaultRootWindow(dpy), "\000", 1, 1);
		attr.cursor = XCreatePixmapCursor(dpy, bitmap, bitmap, &cursor_color, &cursor_color, 0, 0);
		attrmask |= CWCursor;
	}

	wnd = XCreateWindow(dpy, DefaultRootWindow(dpy),
		0, 0, 640, 480, 0, info->depth, InputOutput, info->visual,
		attrmask, &attr);
	if (HideCursor)
	{
		XFreeCursor(dpy, attr.cursor);
		XFreePixmap(dpy, bitmap);
	}
	if (!wnd)
	{
		Log("Error creating window.");
		return nullptr;
	}
	// Update the XWindow->CStdWindow-Map
	CStdAppPrivate::SetWindow(wnd, this);
	if (!pApp->Priv->xic && pApp->Priv->xim)
	{
		pApp->Priv->xic = XCreateIC(pApp->Priv->xim,
			XNClientWindow, wnd,
			XNFocusWindow, wnd,
			XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
			XNResourceName, STD_PRODUCT,
			XNResourceClass, STD_PRODUCT,
			nullptr);
		if (!pApp->Priv->xic)
		{
			Log("Failed to create input context.");
			XCloseIM(pApp->Priv->xim);
			pApp->Priv->xim = nullptr;
		}
		else
		{
			long ic_event_mask;
			if (XGetICValues(pApp->Priv->xic, XNFilterEvents, &ic_event_mask, nullptr) == nullptr)
				attr.event_mask |= ic_event_mask;
			XSelectInput(dpy, wnd, attr.event_mask);
			XSetICFocus(pApp->Priv->xic);
		}
	}
	// We want notification of closerequests and be killed if we hang
	Atom WMProtocols[2];
	char *WMProtocolnames[] = { "WM_DELETE_WINDOW", "_NET_WM_PING" };
	XInternAtoms(dpy, WMProtocolnames, 2, false, WMProtocols);
	XSetWMProtocols(dpy, wnd, WMProtocols, 2);
	// Let the window manager know our pid so it can kill us
	Atom PID = XInternAtom(pApp->dpy, "_NET_WM_PID", false);
	int32_t pid = getpid();
	if (PID != None) XChangeProperty(pApp->dpy, wnd, PID, XA_CARDINAL, 32, PropModeReplace, reinterpret_cast<const unsigned char *>(&pid), 1);
	// Title and stuff
	XTextProperty title_property;
	StdStrBuf tbuf; tbuf.Copy(Title ? Title : "");
	char *tbufstr = tbuf.getMData();
	XStringListToTextProperty(&tbufstr, 1, &title_property);
	// State and Icon
	XWMHints *wm_hint = XAllocWMHints();
	wm_hint->flags = StateHint | InputHint | IconPixmapHint | IconMaskHint;
	wm_hint->initial_state = NormalState;
	wm_hint->input = True;
	// Trust XpmCreatePixmapFromData to not modify the xpm...
	XpmCreatePixmapFromData(dpy, wnd, const_cast<char **>(c4x_xpm), &wm_hint->icon_pixmap, &wm_hint->icon_mask, 0);
	// Window class
	XClassHint *class_hint = XAllocClassHint();
	class_hint->res_name = STD_PRODUCT;
	class_hint->res_class = STD_PRODUCT;
	XSetWMProperties(dpy, wnd, &title_property, &title_property, pApp->Priv->argv, pApp->Priv->argc, 0, wm_hint, class_hint);
	// Set "parent". Clonk does not use "real" parent windows, but multiple toplevel windows.
	if (pParent) XSetTransientForHint(dpy, wnd, pParent->wnd);
	// Show window
	XMapWindow(dpy, wnd);
	// Clean up
	XFree(title_property.value);
	Hints = wm_hint;
	XFree(class_hint);

	// Render into whole window
	renderwnd = wnd;

	return this;
#endif // USE_X11
}

void CStdWindow::Clear()
{
#ifdef USE_X11
	// Destroy window
	if (wnd)
	{
		CStdAppPrivate::SetWindow(wnd, nullptr);
		XUnmapWindow(dpy, wnd);
		XDestroyWindow(dpy, wnd);
		if (Hints) XFree(Hints);

		// Might be necessary when the last window is closed
		XFlush(dpy);
	}
	wnd = renderwnd = 0;
#endif
}

#ifdef USE_X11
bool CStdWindow::FindFBConfig()
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
	int fbcount;
	GLXFBConfig *config = glXChooseFBConfig(dpy, DefaultScreen(dpy), attrListDbl, &fbcount);

	if (!config)
	{
		Log("  gl: no doublebuffered config.");
		XFree(config);
		// a singlebuffered is probably better than the default
		config = glXChooseFBConfig(dpy, DefaultScreen(dpy), attrListSgl, &fbcount);
	}
#endif
	if (!config)
	{
#ifndef USE_CONSOLE
		XFree(config);
#endif
		Log("  gl: no visual at all.");
		return false;
	}

	struct {int framebuffer; int samples; } best{-1, -1};

	for (int i = 0; i < fbcount; ++i)
	{
		int samples;
		int framebuffer;
		glXGetFBConfigAttrib(dpy, config[i], GLX_SAMPLE_BUFFERS, &framebuffer);
		glXGetFBConfigAttrib(dpy, config[i], GLX_SAMPLES, &samples);

		if (best.framebuffer < 0 || (framebuffer && samples > best.samples))
		{
			best.framebuffer = framebuffer;
			best.samples = samples;
		}
	}

	FBConfig = config[best.framebuffer];
	XFree(config);

	return true;
}
#endif // USE_X11

bool CStdWindow::RestorePosition(const char *, const char *, bool)
{
	// The Windowmanager is responsible for window placement.
	return true;
}

bool CStdWindow::GetSize(RECT *pRect)
{
#ifdef USE_X11
	Window winDummy;
	unsigned int borderDummy;
	int x, y;
	unsigned int width, height;
	unsigned int depth;
	XGetGeometry(dpy, wnd, &winDummy, &x, &y,
		&width, &height, &borderDummy, &depth);
	pRect->right = width + x;
	pRect->bottom = height + y;
	pRect->top = y;
	pRect->left = x;
#else
	pRect->left = pRect->right = pRect->top = pRect->bottom = 0;
#endif
	return true;
}

void CStdWindow::SetSize(unsigned int X, unsigned int Y)
{
#ifdef USE_X11
	XResizeWindow(dpy, wnd, X, Y);
#endif
}

void CStdWindow::SetTitle(const char *Title)
{
#ifdef USE_X11
	XTextProperty title_property;
	StdStrBuf tbuf(Title, true);
	char *tbufstr = tbuf.getMData();
	XStringListToTextProperty(&tbufstr, 1, &title_property);
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

void CStdWindow::SetDisplayMode(DisplayMode mode)
{
#ifdef USE_X11
	const auto fullscreen = mode == DisplayMode::Fullscreen;

	static Atom atoms[4];
	static char* names[] = {"_NET_WM_STATE", "_NET_WM_STATE_FULLSCREEN", "_NET_WM_MAXIMIZED_VERT", "_NET_WM_MAXIMIZED_HORZ"};
	if (!atoms[0]) XInternAtoms(dpy, names, 4, false, atoms);
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
