/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2005, Günther
 * Copyright (c) 2017-2026, The LegacyClonk Team and contributors
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
#include "StdSync.h"
#include "spdlog/common.h"

#include <array>
#include <string>

#if !defined(_WIN32)
#include <poll.h>
#endif

// TODO: Rename file from unix to something more general since it is now also used by windows.

#include <chrono>

#include "C4Log.h"

CStdApp::CStdApp()
	: mainThread{std::this_thread::get_id()}
{
}

CStdApp::~CStdApp()
{
}

void CStdApp::Init(const int argc, char **const argv)
{
	setlocale(LC_ALL, "");
	setlocale(LC_NUMERIC, "C");

	this->argc = argc;
	this->argv = argv;

	static char dir[_MAX_PATH];
	SCopy(argv[0], dir);

#if !defined(USE_SDL_MAINLOOP)
	if (dir[0] != '/')
	{
		SInsert(dir, "/");
		SInsert(dir, GetWorkingDirectory());
	}
#endif

	location = dir;

	// Build command line.
	static std::string s("\"");
	for (int i = 1; i < argc; ++i)
	{
		s.append(argv[i]);
		s.append("\" \"");
	}
	s.append("\"");
	szCmdLine = s.c_str();

#if defined(USE_SDL_MAINLOOP)
	try
	{
		sdlVideoSubSys.emplace(SDL_INIT_VIDEO);
	}
	catch (const std::runtime_error &)
	{
		throw StartupException{"Error initializing SDL."};
	}
#endif

#if !defined(_WIN32)
	if (pipe(Pipe) != 0)
	{
		throw StartupException{"Error creating Pipe"};
	}
#endif

	DoInit();
}

bool CStdApp::InitTimer()
{
	std::timespec_get(&lastExecute, TIME_UTC);
	return true;
}

void CStdApp::Clear()
{
#if !defined(_WIN32)
	close(Pipe[0]);
	close(Pipe[1]);
#endif

#if defined(USE_SDL_MAINLOOP)
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
	const time_t seconds{lastExecute.tv_sec};
	std::timespec tv;
	std:timespec_get(&tv, TIME_UTC);

	if (doNotDelay)
	{
		doNotDelay = false;
		lastExecute = tv;
	}
	else if (lastExecute.tv_sec < tv.tv_sec - 2)
	{
		lastExecute = tv;
	}
	else
	{
		lastExecute.tv_nsec += delayNs;
		if (lastExecute.tv_nsec > 1000000000)
		{
			++lastExecute.tv_sec;
			lastExecute.tv_nsec -= 1000000000;
		}
	}

	// This will make the FPS look "prettier" in some situations
	// But who cares...
	if (seconds != lastExecute.tv_sec)
	{
		pWindow->Sec1Timer();
	}
}

void CStdApp::NextTick(bool)
{
	doNotDelay = true;
}

void CStdApp::Run()
{
	while (HandleMessage(StdSync::Infinite, true) != HR_Failure)
	{
	}
}

void CStdApp::ResetTimer(const unsigned int delayMS)
{
	delayNs = delayMS * 1000000;
}

C4AppHandleResult CStdApp::HandleMessage(const unsigned int timeout, const bool checkTimer)
{
	// quit check for nested HandleMessage-calls
	if (fQuitMsgReceived) return HR_Failure;

	bool doExecute{checkTimer};

	std::timespec tv;
	if (doNotDelay)
	{
		tv = {0, 0};
	}
	else if (checkTimer)
	{
		std::timespec_get(&tv, TIME_UTC);
		tv.tv_nsec = lastExecute.tv_nsec - tv.tv_nsec + delayNs - 1000000000 * (tv.tv_sec - lastExecute.tv_sec);

		// Check if the given timeout comes first
		// (don't call Execute then, because it assumes it has been called because of a timer event!)
		if (timeout != StdSync::Infinite && timeout * 1000000 < tv.tv_nsec)
		{
			tv.tv_nsec = timeout * 1000000;
			doExecute = false;
		}

		if (tv.tv_nsec < 0)
		{
			tv.tv_nsec = 0;
		}
	}
	else
	{
		tv.tv_nsec = timeout * 1000000;
	}

	tv.tv_sec = 0;

#if defined(USE_SDL_MAINLOOP)
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		HandleSDLEvent(event);
	}
#endif

#if defined(_WIN32)
	if (doExecute && tv.tv_nsec == 0)
	{
		// TODO: Check if this is the correct way. These events were processed by the StdAppWin32 before.
		if(NetworkEvent.GetEvent())
		{
			OnNetworkEvents();
		}

		Execute();
		return HR_Timer;
	}

	return HR_Timeout;

#else

	std::array<pollfd, 3> fds;
	fds.fill({.fd = -1, .events = POLLIN});

	fds[0].fd = Pipe[0];

#ifdef USE_CONSOLE
	fds[2].fd = STDIN_FILENO;
#endif

	switch (StdSync::Poll(fds, (checkTimer || timeout != StdSync::Infinite) ? tv.tv_nsec / 1000000 : StdSync::Infinite))
	{
	case -1:
		LogNTr(spdlog::level::err, "poll error: {}", std::strerror(errno));
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
		if (fds[0].revents & POLLIN)
		{
			OnPipeInput();
		}

#ifdef USE_CONSOLE
		if (fds[2].revents & POLLIN)
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

	return HR_Message;
}

bool CStdApp::SignalNetworkEvent()
{
	char c{1};
	write(pipe[1], &c, 1);
	return true;
}

bool CStdApp::Copy(const std::string_view text, const bool clipboard)
{
#if defined(USE_SDL_MAINLOOP)
	return !SDL_SetClipboardText(std::string{text}.c_str());
#elif defined(USE_CONSOLE)
	return false;
#endif
}

std::string CStdApp::Paste(const bool clipboard)
{
#if defined(USE_SDL_MAINLOOP)
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
#if defined(USE_SDL_MAINLOOP)
	return SDL_HasClipboardText() == true;
#endif
}

void CStdApp::OnPipeInput()
{
	char c;
	read(pipe[0], &c, 1);
	OnNetworkEvents();
}

bool CStdApp::ReadStdInCommand()
{
	// Surely not the most efficient way to do it, but we won't have to read much data anyway.
// TODO: Reinvestigate whether this is something we need for windows when switching to SDL
#if !defined(_WIN32)
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
	else if (C4Strings::IsPrint(c))
	{
		CmdBuf.AppendChar(c);
	}
#endif

	return true;
}

#if defined(USE_SDL_MAINLOOP)
static void UpdateKeyMaskFromModifiers(const std::uint16_t modifiers)
{
}

void CStdApp::HandleSDLEvent(SDL_Event &event)
{
	switch (event.type)
	{
	case SDL_EVENT_QUIT :
		Quit();
		return;

	case SDL_EVENT_KEY_DOWN :
	case SDL_EVENT_KEY_UP :
	{
		keyMask = event.key.mod;
		break;
	}
	}

	if (pWindow)
	{
		pWindow->HandleMessage(event);
	}
}
#endif
