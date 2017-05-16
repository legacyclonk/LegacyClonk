/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
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

#ifndef INC_STD_X_PRIVATE_H
#define INC_STD_X_PRIVATE_H

class CStdAppPrivate
{
public:
#ifdef WITH_GLIB
	GMainLoop *loop;

	// IOChannels required to wake up the main loop
	GIOChannel *pipe_channel;
	GIOChannel *x_channel;
	GIOChannel *stdin_channel;
#endif

	CStdAppPrivate() :
#ifdef USE_X11
		PrimarySelection(), ClipboardSelection(),
		LastEventTime(CurrentTime), modefound(false), pending_desktop(false),
		xim(0), xic(0),
#endif
		argc(0), argv(0) {}
	static CStdWindow *GetWindow(unsigned long wnd);
	static void SetWindow(unsigned long wnd, CStdWindow *pWindow);
#ifdef USE_X11
	bool SwitchToFullscreen(CStdApp *pApp, Window wnd);
	void SwitchToDesktop(CStdApp *pApp, bool fMinimize, Window wnd);
	void SetEWMHFullscreen(CStdApp *pApp, bool fFullScreen, Window wnd);
	struct ClipboardData
	{
		StdStrBuf Text;
	} PrimarySelection, ClipboardSelection;
	unsigned long LastEventTime;
	typedef std::map<unsigned long, CStdWindow *> WindowListT;
	static WindowListT WindowList;
	XF86VidModeModeInfo oldmode, targetmode;
	bool modefound;
	int pending_desktop;
	static const int PENDING_DESKTOP_DELAY = 3;
	XIM xim;
	XIC xic;
	Bool detectable_autorepeat_supported;
#endif
	int argc; char **argv;
	// Used to signal a network event
	int Pipe[2];
};

#endif // INC_STD_X_PRIVATE_H
