/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2022, The LegacyClonk Team and contributors
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

#include "C4Windows.h"

#include "StdSync.h"

#ifndef _WIN32
#include <cstring>
#include <limits>
#include <stdexcept>

#include <fcntl.h>
#include <unistd.h>
#endif

#ifdef _WIN32

CStdEvent::CStdEvent(const bool initialState)
	: event{CreateEvent(nullptr, true, initialState, nullptr)}
{
	if (!event)
	{
		throw std::runtime_error{"CreateEvent failed"};
	}
}

CStdEvent::~CStdEvent()
{
	CloseHandle(event);
}

void CStdEvent::Set()
{
	if (!SetEvent(event))
	{
		throw std::runtime_error{"SetEvent failed"};
	}
}

void CStdEvent::Reset()
{
	if (!(ResetEvent(event)))
	{
		throw std::runtime_error{"ResetEvent failed"};
	}
}

bool CStdEvent::WaitFor(const std::uint32_t milliseconds)
{
	switch (WaitForSingleObject(event, milliseconds))
	{
	case WAIT_OBJECT_0:
		return true;

	case WAIT_TIMEOUT:
		return false;

	default:
		throw std::runtime_error{"WaitForSingleObject failed"};
	}
}

#else

int StdSync::Poll(const std::span<pollfd> fds, const std::uint32_t timeout)
{
	const auto clampedTimeout = std::clamp(static_cast<int>(timeout), static_cast<int>(Infinite), std::numeric_limits<int>::max());

	for (;;)
	{
		const int result{poll(fds.data(), static_cast<nfds_t>(fds.size()), clampedTimeout)};

		if (result == -1 && (errno == EINTR || errno == EAGAIN || errno == ENOMEM))
		{
			continue;
		}

		return result;
	}
}

[[noreturn]] static void ThrowError(const char *const message)
{
	throw std::runtime_error{std::string{message} + ": " + std::strerror(errno)};
}

static void ThrowIfFailed(const bool result, const char *const message)
{
	if (!result)
	{
		ThrowError(message);
	}
}

CStdEvent::CStdEvent(const bool initialState)
{
	ThrowIfFailed(pipe(fd) != -1, "pipe failed");

	ThrowIfFailed(fcntl(fd[0], F_SETFL, fcntl(fd[0], F_GETFL) | O_NONBLOCK) != -1, "fcntl failed");
	ThrowIfFailed(fcntl(fd[1], F_SETFL, fcntl(fd[1], F_GETFL) | O_NONBLOCK) != -1, "fcntl failed");

	if (initialState)
	{
		Set();
	}
}

CStdEvent::~CStdEvent()
{
	close(fd[0]);
	close(fd[1]);
}

void CStdEvent::Set()
{
	for (char c{42}; write(fd[1], &c, 1) == -1; )
	{
		switch (errno)
		{
		case EINTR:
			continue;

		case EAGAIN:
			return;

		default:
			ThrowError("write failed");
		}
	}
}

void CStdEvent::Reset()
{
	for (char c; ;)
	{
		if (read(fd[0], &c, 1) == -1)
		{
			switch (errno)
			{
			case EINTR:
				continue;

			case EAGAIN:
				return;

			default:
				ThrowError("read failed");
			}
		}
	}
}

bool CStdEvent::WaitFor(const std::uint32_t milliseconds)
{
	std::array<pollfd, 1> pollFD{pollfd{.fd = fd[0], .events = POLLIN}};

	switch (StdSync::Poll(pollFD, milliseconds))
	{
	case -1:
		ThrowError("poll failed");

	case 0:
		return false;

	default:
		return true;
	}
}

#endif
