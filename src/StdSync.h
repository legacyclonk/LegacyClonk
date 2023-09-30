/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2017-2020, The LegacyClonk Team and contributors
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

/* synchronization helper classes */

#pragma once

#include "C4Windows.h"

#include <atomic>
#include <array>
#include <limits>
#include <mutex>
#include <chrono>
#include <condition_variable>

#ifndef _WIN32
#include <ranges>
#include <span>

#include <poll.h>
#endif

namespace StdSync
{
	static inline constexpr auto Infinite = std::numeric_limits<std::uint32_t>::max();

#ifndef _WIN32
	template<typename T> requires std::ranges::contiguous_range<T> && std::ranges::sized_range<T>
	int Poll(T &&fds, const std::uint32_t timeout)
	{
		const auto clampedTimeout = std::clamp(static_cast<int>(timeout), static_cast<int>(Infinite), std::numeric_limits<int>::max());

		for (;;)
		{
			const int result{poll(std::ranges::data(fds), static_cast<nfds_t>(std::ranges::size(fds)), clampedTimeout)};

			if (result == -1 && (errno == EINTR || errno == EAGAIN || errno == ENOMEM))
			{
				continue;
			}

			return result;
		}
	}
#endif
}

class CStdCSec
{
public:
	virtual ~CStdCSec() = default;

protected:
	std::recursive_mutex mutex;

public:
	virtual void Enter() { mutex.lock(); }
	virtual void Leave() { mutex.unlock(); }
};

class CStdEvent
{
public:
	CStdEvent(bool initialState = false);
	~CStdEvent();

#ifdef _WIN32
private:
	CStdEvent(bool initialState, bool manualReset);
#endif

public:
	void Set();
	void Reset();
	bool WaitFor(std::uint32_t milliseconds);

#ifdef _WIN32
	HANDLE GetEvent() const { return event; }

public:
	static CStdEvent AutoReset(bool initialState = false);
#else
	int GetFD() const noexcept { return fd[0].load(std::memory_order_acquire); }
#endif

private:
#ifdef _WIN32
	HANDLE event;
#else
	std::array<std::atomic_int, 2> fd;
#endif
};

class CStdLock
{
public:
	CStdLock(CStdCSec *pSec) : sec(pSec)
	{
		sec->Enter();
	}

	~CStdLock()
	{
		Clear();
	}

protected:
	CStdCSec *sec;

public:
	void Clear()
	{
		if (sec)
		{
			sec->Leave();
		}

		sec = nullptr;
	}
};

class CStdCSecExCallback
{
public:
	// is called with CSec exlusive locked!
	virtual void OnShareFree(class CStdCSecEx *pCSec) = 0;
	virtual ~CStdCSecExCallback() {}
};

class CStdCSecEx : public CStdCSec
{
public:
	CStdCSecEx()
		: lShareCnt(0), ShareFreeEvent(false), pCallbClass(nullptr) {}
	CStdCSecEx(CStdCSecExCallback *pCallb)
		: lShareCnt(0), ShareFreeEvent(false), pCallbClass(pCallb) {}
	~CStdCSecEx() {}

protected:
	// share counter
	long lShareCnt;
	// event: exclusive access permitted
	CStdEvent ShareFreeEvent;
	// callback
	CStdCSecExCallback *pCallbClass;

public:
	// (cycles forever if shared locked by calling thread!)
	void Enter() override
	{
		// lock
		CStdCSec::Enter();
		// wait for share-free
		while (lShareCnt)
		{
			// reset event
			ShareFreeEvent.Reset();
			// leave section for waiting
			CStdCSec::Leave();
			// wait
			ShareFreeEvent.WaitFor(StdSync::Infinite);
			// reenter section
			CStdCSec::Enter();
		}
	}

	void Leave() override
	{
		// set event
		ShareFreeEvent.Set();
		// unlock
		CStdCSec::Leave();
	}

	void EnterShared()
	{
		// lock
		CStdCSec::Enter();
		// add share
		lShareCnt++;
		// unlock
		CStdCSec::Leave();
	}

	void LeaveShared()
	{
		// lock
		CStdCSec::Enter();
		// remove share
		if (!--lShareCnt)
		{
			// do callback
			if (pCallbClass)
				pCallbClass->OnShareFree(this);
			// set event
			ShareFreeEvent.Set();
		}
		// unlock
		CStdCSec::Leave();
	}
};

class CStdShareLock
{
public:
	CStdShareLock(CStdCSecEx *pSec) : sec(pSec)
	{
		sec->EnterShared();
	}

	~CStdShareLock()
	{
		Clear();
	}

protected:
	CStdCSecEx *sec;

public:
	void Clear()
	{
		if (sec)
		{
			sec->LeaveShared();
		}

		sec = nullptr;
	}
};
