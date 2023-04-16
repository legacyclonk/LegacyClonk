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

#include <array>
#include <limits>
#include <mutex>
#include <chrono>
#include <condition_variable>

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
	static constexpr auto Infinite = std::numeric_limits<std::uint32_t>::max();

public:
	CStdEvent(bool initialState = false, bool manualReset = true);
	~CStdEvent();

public:
	void Set();
	void Reset();
	bool WaitFor(std::uint32_t milliseconds);

#ifdef _WIN32
	HANDLE GetEvent() const { return event; }
#else
	std::array<int, 2> GetFDs() const { return {fd[0], fd[1]}; }

#endif

private:
#ifdef _WIN32
	HANDLE event;
#else
	int fd[2]; // Not std::array in order to allow objects to be safely memcpy'd
	bool manualReset;
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
		if (sec) sec->Leave(); sec = nullptr;
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
			ShareFreeEvent.WaitFor(CStdEvent::Infinite);
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
		if (sec) sec->LeaveShared(); sec = nullptr;
	}
};
