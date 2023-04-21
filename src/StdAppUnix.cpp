/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2005, Günther
 * Copyright (c) 2017-2023, The LegacyClonk Team and contributors
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

#include "StdApp.h"

#include <array>
#include <string>

#include <sys/time.h>

#ifdef USE_X11
#include <string_view>

#include <X11/Xmd.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/xf86vmode.h>
#include <X11/XKBlib.h>
#endif

#ifdef WITH_GLIB
#include <glib.h>
#endif

#include "C4Log.h"

CStdApp::CStdApp()
	: mainThread{std::this_thread::get_id()}
{
}

CStdApp::~CStdApp()
{
}

#ifdef USE_X11
static Atom ClipboardAtoms[1];
#endif

#ifdef WITH_GLIB
template<auto Callback>
static gboolean ForwardPipeInput(GIOChannel *, GIOCondition, gpointer data)
{
	(static_cast<CStdApp *>(data)->*Callback)();
	return true;
}
#endif

void CStdApp::Init(const int argc, char **const argv)
{
	setlocale(LC_ALL, "");
	setlocale(LC_NUMERIC, "");

#ifdef USE_X11
	// Clear XMODIFIERS as key input gets evaluated twice otherwise
	unsetenv("XMODIFIERS");
#endif

	this->argc = argc;
	this->argv = argv;

	static char dir[PATH_MAX];
	SCopy(argv[0], dir);

#ifndef USE_SDL_MAINLOOP
	if (dir[0] != '/')
	{
		SInsert(dir, "/");
		SInsert(dir, GetWorkingDirectory());
	}
#endif

	Location = dir;

	// Build command line.
	static std::string s("\"");
	for (int i = 1; i < argc; ++i)
	{
		s.append(argv[i]);
		s.append("\" \"");
	}
	s.append("\"");
	szCmdLine = s.c_str();

#ifdef WITH_GLIB
	loop = g_main_loop_new(nullptr, false);
#endif

#ifdef USE_X11
	dpy = XOpenDisplay(nullptr);
	if (!dpy)
	{
		throw StartupException{"Error opening display."};
	}

	int eventBase;
	int errorBase;
	if (!XF86VidModeQueryExtension(dpy, &eventBase, &errorBase) ||
		!XF86VidModeQueryVersion(dpy, &xf86vmode_major_version, &xf86vmode_minor_version))
	{
		xf86vmode_major_version = -1;
		xf86vmode_minor_version = 0;
		Log("XF86VidMode Extension missing, resolution switching will not work");
	}

	// So a repeated keypress-event is not preceded with a keyrelease.
	XkbSetDetectableAutoRepeat(dpy, true, &detectable_autorepeat_supported);

	XSetLocaleModifiers("");

	inputMethod = XOpenIM(dpy, nullptr, nullptr, nullptr);
	if (!inputMethod)
	{
		Log("Failed to open input method");
	}

	const char *names[]{"CLIPBOARD"};
	XInternAtoms(dpy, const_cast<char **>(names), 1, false, ClipboardAtoms);

#ifdef WITH_GLIB
	xChannel = g_io_channel_unix_new(XConnectionNumber(dpy));
	g_io_add_watch(xChannel, G_IO_IN, &ForwardPipeInput<&CStdApp::OnXInput>, this);
#endif
#elif defined(USE_SDL_MAINLOOP)
	try
	{
		sdlVideoSubSys.emplace(SDL_INIT_VIDEO);
	}
	catch (const std::runtime_error &)
	{
		throw StartupException{"Error initializing SDL."};
	}
#endif

	if (pipe(Pipe) != 0)
	{
		throw StartupException{"Error creating Pipe"};
	}

#ifdef WITH_GLIB
	pipeChannel = g_io_channel_unix_new(Pipe[0]);
	g_io_add_watch(pipeChannel, G_IO_IN, &ForwardPipeInput<&CStdApp::OnPipeInput>, this);
#endif

	DoInit();
}

bool CStdApp::InitTimer()
{
	gettimeofday(&LastExecute, nullptr);
	return true;
}

void CStdApp::Clear()
{
#ifdef USE_X11
	if (dpy)
	{
		XCloseDisplay(dpy);
		dpy = nullptr;
	}
#endif

	close(Pipe[0]);
	close(Pipe[1]);

#ifdef WITH_GLIB
	g_main_loop_unref(loop);

	if (pipeChannel)
	{
		g_io_channel_unref(pipeChannel);
	}

#ifdef USE_X11
	if (xChannel)
	{
		g_io_channel_unref(xChannel);
	}
#endif
#endif

#ifdef USE_SDL_MAINLOOP
	sdlVideoSubSys.reset();
	SDL_Quit();
#endif
}

void CStdApp::Quit()
{
	fQuitMsgReceived = true;
}

void CStdApp::Execute()
{
	const time_t seconds{LastExecute.tv_sec};
	timeval tv;
	gettimeofday(&tv, nullptr);

	if (DoNotDelay)
	{
		DoNotDelay = false;
		LastExecute = tv;
	}
	else if (LastExecute.tv_sec < tv.tv_sec - 2)
	{
		LastExecute = tv;
	}
	else
	{
		LastExecute.tv_usec += Delay;
		if (LastExecute.tv_usec > 1000000)
		{
			++LastExecute.tv_sec;
			LastExecute.tv_usec -= 1000000;
		}
	}

	// This will make the FPS look "prettier" in some situations
	// But who cares...
	if (seconds != LastExecute.tv_sec)
	{
		pWindow->Sec1Timer();
	}
}

void CStdApp::NextTick(bool)
{
	DoNotDelay = true;
}

void CStdApp::Run()
{
	while (HandleMessage(INFINITE, true) != HR_Failure)
	{
	}
}

void CStdApp::ResetTimer(const unsigned int d)
{
	Delay = d * 1000;
}

C4AppHandleResult CStdApp::HandleMessage(const unsigned int timeout, const bool checkTimer)
{
	// quit check for nested HandleMessage-calls
	if (fQuitMsgReceived) return HR_Failure;

	bool doExecute{checkTimer};

	timeval tv;
	if (DoNotDelay)
	{
		tv = {0, 0};
	}
	else if (checkTimer)
	{
		gettimeofday(&tv, nullptr);
		tv.tv_usec = LastExecute.tv_usec - tv.tv_usec + Delay - 1000000 * (tv.tv_sec - LastExecute.tv_sec);

		// Check if the given timeout comes first
		// (don't call Execute then, because it assumes it has been called because of a timer event!)
		if (timeout != INFINITE && timeout * 1000 < tv.tv_usec)
		{
			tv.tv_usec = timeout * 1000;
			doExecute = false;
		}

		if (tv.tv_usec < 0)
		{
			tv.tv_usec = 0;
		}
	}
	else
	{
		tv.tv_usec = timeout * 1000;
	}

	tv.tv_sec = 0;

#ifdef USE_X11
	while (XPending(dpy))
	{
		HandleXMessage();
	}
#elif defined(USE_SDL_MAINLOOP)
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		HandleSDLEvent(event);
	}
#endif

#ifdef WITH_GLIB
	const auto tvTimeout = static_cast<unsigned int>(tv.tv_sec * 1000 + tv.tv_usec / 1000);
	bool timeoutElapsed{false};
	guint timeoutHandle{0};

	// Guarantee that we do not block until something interesting occurs
	// when using a timeout
	if (checkTimer || timeout != INFINITE)
	{
		// The timeout handler sets timeout_elapsed to true when
		// the timeout elapsed, this is required for a correct return
		// value.
		timeoutHandle = g_timeout_add_full(
			G_PRIORITY_HIGH,
			tvTimeout,
			[](gpointer data) -> gboolean { *static_cast<bool *>(data) = true; return false; },
			&timeoutElapsed,
			nullptr
		);
	}

	g_main_context_iteration(g_main_loop_get_context(loop), true);

	if (timeoutHandle && !timeoutElapsed)
	{
		// FIXME: do not add a new timeout instead of deleting the old one in the next call
		g_source_remove(timeoutHandle);
	}

	if (timeoutElapsed && doExecute)
	{
		Execute();
	}

	while (g_main_context_pending(g_main_loop_get_context(loop)))
	{
		g_main_context_iteration(g_main_loop_get_context(loop), false);
	}

	return timeoutElapsed ? (doExecute ? HR_Timer : HR_Timeout) : HR_Message;
#else
	// Watch dpy to see when it has input.
	int maxFD{0};
	fd_set fds;
	FD_ZERO(&fds);

#ifdef USE_CONSOLE
	FD_SET(STDIN_FILENO, &fds);
#endif

	const auto setFD = [&maxFD, &fds](const int fd)
	{
		FD_SET(fd, &fds);
		maxFD = std::max(maxFD, fd);
	};

#ifdef USE_X11
	// Stop waiting for the next frame when more events arrive
	XFlush(dpy);
	setFD(XConnectionNumber(dpy));
#endif

	setFD(Pipe[0]);

	switch (select(maxFD + 1, &fds, nullptr, nullptr, (checkTimer || timeout != INFINITE) ? &tv : nullptr))
	{
	case -1:
		if (errno == EINTR)
		{
			return HR_Message;
		}

		LogF("select error: %s", std::strerror(errno));
		return HR_Failure;

	// timeout
	case 0:
		if (doExecute)
		{
			Execute();
			return HR_Timer;
		}

		return HR_Timeout;

	default:
		if (FD_ISSET(Pipe[0], &fds))
		{
			OnPipeInput();
		}

#ifdef USE_X11
		if (FD_ISSET(XConnectionNumber(dpy), &fds))
		{
			OnXInput();
		}
#endif

#ifdef USE_CONSOLE
		if (FD_ISSET(STDIN_FILENO, &fds))
		{
			if (!ReadStdInCommand())
			{
				return HR_Failure;
			}
		}
#endif

		return HR_Message;
	}
#endif
}

bool CStdApp::SignalNetworkEvent()
{
	char c{1};
	write(Pipe[1], &c, 1);
	return true;
}

bool CStdApp::Copy(const std::string_view text, const bool clipboard)
{
#ifdef USE_X11
	const auto copy = [=, this](std::string &data, const Atom atom)
	{
		XSetSelectionOwner(dpy, atom, pWindow->wnd, LastEventTime);
		if (XGetSelectionOwner(dpy, atom) != pWindow->wnd)
		{
			return false;
		}

		data = text;
		return true;
	};

	if (clipboard)
	{
		return copy(clipboardSelection, ClipboardAtoms[0]);
	}
	else
	{
		return copy(primarySelection, XA_PRIMARY);
	}
#elif defined(USE_SDL_MAINLOOP)
	return !SDL_SetClipboardText(std::string{text}.c_str());
#elif defined(USE_CONSOLE)
	return false;
#endif
}

std::string CStdApp::Paste(const bool clipboard)
{
#ifdef USE_X11
	if (!IsClipboardFull())
	{
		return "";
	}

	XConvertSelection(dpy, clipboard ? ClipboardAtoms[0] : XA_PRIMARY, XA_STRING, XA_STRING, pWindow->wnd, LastEventTime);

	// Give the owner some time to respond
	HandleMessage(50, false);

	// Get the length of the data, so we can request it all at once
	Atom type;
	int format;
	unsigned long size;
	unsigned long bytesLeft;
	unsigned char *data;

	const auto getWindowProperty = [&](const bool deleteNow)
	{
		return XGetWindowProperty(
			dpy,
			pWindow->wnd,
			XA_STRING,
			0,
			0,
			deleteNow,
			AnyPropertyType,
			&type,
			&format,
			&size,
			&bytesLeft,
			&data
		);
	};

	getWindowProperty(false);

	if (bytesLeft == 0)
	{
		return "";
	}

	if (getWindowProperty(true) != Success)
	{
		return "";
	}

	std::string result{reinterpret_cast<char *>(data)};
	XFree(data);
	return result;
#elif defined(USE_SDL_MAINLOOP)
	char *const text{SDL_GetClipboardText()};
	if (text)
	{
		std::string ret{text};
		free(text);
		return ret;
	}
#endif
	return "";
}

bool CStdApp::IsClipboardFull(const bool clipboard)
{
#ifdef USE_X11
	return XGetSelectionOwner(dpy, clipboard ? ClipboardAtoms[0] : XA_PRIMARY) != None;
#elif defined(USE_SDL_MAINLOOP)
	return SDL_HasClipboardText() == SDL_TRUE;
#elif defined(USE_CONSOLE)
	return false;
#endif
}

void CStdApp::OnPipeInput()
{
	char c;
	read(Pipe[0], &c, 1);
	OnNetworkEvents();
}

bool CStdApp::ReadStdInCommand()
{
	// Surely not the most efficient way to do it, but we won't have to read much data anyway.
	char c;
	if (read(STDIN_FILENO, &c, 1) != 1)
	{
		return false;
	}

	if (c == '\n')
	{
		if (!CmdBuf.isNull())
		{
			OnCommand(CmdBuf.getData());
			CmdBuf.Clear();
		}
	}
	else if (std::isprint(c))
	{
		CmdBuf.AppendChar(c);
	}

	return true;
}

#ifdef USE_X11
static unsigned int KeyMaskFromKeyEvent(Display *const dpy, XKeyEvent *const key)
{
	unsigned int mask{key->state};
	const KeySym sym{XkbKeycodeToKeysym(dpy, key->keycode, 0, 1)};

	// We need to correct the keymask since the event.xkey.state
	// is the state _before_ the event, but we want to store the
	// current state.
	if (sym == XK_Control_L || sym == XK_Control_R) mask ^= MK_CONTROL;
	if (sym == XK_Shift_L   || sym == XK_Shift_R)   mask ^= MK_SHIFT;
	if (sym == XK_Alt_L     || sym == XK_Alt_R)     mask ^= MK_ALT;
	return mask;
}

void CStdApp::HandleXMessage()
{
	XEvent event;
	XNextEvent(dpy, &event);
	const auto filtered = static_cast<bool>(XFilterEvent(&event, event.xany.window));

	switch (event.type)
	{
	case FocusIn:
		if (inputContext)
		{
			XSetICFocus(inputContext);
		}
		break;

	case FocusOut:
		if (inputContext)
		{
			XUnsetICFocus(inputContext);
		}
		break;

	case EnterNotify:
		KeyMask = event.xcrossing.state;
		break;

	case KeyPress:
		if (!filtered)
		{
			KeyMask = KeyMaskFromKeyEvent(dpy, &event.xkey);

			std::array<char, 10> buf{};
			if (inputContext)
			{
				Status status;
				XmbLookupString(inputContext, &event.xkey, buf.data(), buf.size(), nullptr, &status);
				if (status == XLookupKeySym)
				{
					fputs("FIXME: XmbLookupString returned XLookupKeySym", stderr);
				}
				else if (status == XBufferOverflow)
				{
					fputs("FIXME: XmbLookupString returned XBufferOverflow\n", stderr);
				}
			}
			else
			{
				static XComposeStatus status;
				XLookupString(&event.xkey, buf.data(), buf.size(), nullptr, &status);
			}

			if (buf[0])
			{
				if (const auto it = windows.find(event.xany.window); it != windows.end() && !IsAltDown())
				{
					it->second->CharIn(buf.data());
				}
			}
		}
		[[fallthrough]];

	case KeyRelease:
		KeyMask = KeyMaskFromKeyEvent(dpy, &event.xkey);
		LastEventTime = event.xkey.time;
		break;

	case ButtonPress:
		// We can take this directly since there are no key presses
		// involved. TODO: We probably need to correct button state
		// here though.
		KeyMask = event.xbutton.state;
		LastEventTime = event.xbutton.time;
		break;

	case SelectionRequest:
		{
			// We should compare the timestamp with the timespan when we owned the selection
			// But slow network connections are not supported anyway, so do not bother
			std::string &clipboardData{event.xselectionrequest.selection == XA_PRIMARY ? primarySelection : clipboardSelection};
			XEvent response{
				.xselection = {
					.type = SelectionNotify,
					.display = dpy,
					.requestor = event.xselectionrequest.requestor,
					.selection = event.xselectionrequest.selection,
					.target = event.xselectionrequest.target
				}
			};

			// Note: we're implementing the spec only partially here
			if (!clipboardData.empty())
			{
				response.xselection.property = event.xselectionrequest.property;
				XChangeProperty(
					dpy,
					response.xselection.requestor,
					response.xselection.property,
					response.xselection.target,
					8,
					PropModeReplace,
					reinterpret_cast<const unsigned char *>(clipboardData.c_str()),
					clipboardData.size()
				);
			}
			else
			{
				response.xselection.property = None;
			}

			XSendEvent(dpy, response.xselection.requestor, false, NoEventMask, &response);
		}
		break;

	case SelectionClear:
		if (event.xselectionrequest.selection == XA_PRIMARY)
		{
			primarySelection.clear();
		}
		else
		{
			clipboardSelection.clear();
		}

		break;

	case ClientMessage:
		{
			using namespace std::literals::string_view_literals;

			if (XGetAtomName(dpy, event.xclient.message_type) == "WM_PROTOCOLS"sv)
			{
				const std::string_view data{XGetAtomName(dpy,event.xclient.data.l[0])};
				if (data == "WM_DELETE_WINDOW")
				{
					if (const auto it = windows.find(event.xclient.window); it != windows.end())
					{
						it->second->Close();
					}
				}
				else if (data == "_NET_WM_PING")
				{
					event.xclient.window = DefaultRootWindow(dpy);
					XSendEvent(dpy, DefaultRootWindow(dpy), false, SubstructureNotifyMask | SubstructureRedirectMask, &event);
				}
			}
		}
		break;

	case MappingNotify:
		XRefreshKeyboardMapping(&event.xmapping);
		break;

	case DestroyNotify:
		if (const auto it = windows.find(event.xany.window); it != windows.end())
		{
			it->second->wnd = 0;
			it->second->Close();
			windows.erase(it);
		}

		windows.erase(event.xany.window);
		break;
	}

	if (const auto it = windows.find(event.xany.window); it != windows.end())
	{
		it->second->HandleMessage(event);
	}
}

void CStdApp::OnXInput()
{
	while (XEventsQueued(dpy, QueuedAfterReading))
	{
		HandleXMessage();
	}
}

void CStdApp::NewWindow(CStdWindow *const window)
{
	windows.emplace(window->wnd, window);
}
#elif defined(USE_SDL_MAINLOOP)
static void UpdateKeyMaskFromModifiers(const std::uint16_t modifiers)
{
}

void CStdApp::HandleSDLEvent(SDL_Event &event)
{
	switch (event.type)
	{
	case SDL_QUIT:
		Quit();
		return;

	case SDL_KEYDOWN:
	case SDL_KEYUP:
	{
		KeyMask = event.key.keysym.mod;
		break;
	}
	}

	if (pWindow)
	{
		pWindow->HandleMessage(event);
	}
}
#endif
