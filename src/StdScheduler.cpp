/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2017-2022, The LegacyClonk Team and contributors
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

#include "C4Thread.h"
#include "StdScheduler.h"

#include <cstring>

#include <stdio.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>

#ifdef _WIN32

#include <process.h>
#include <mmsystem.h>

#endif

#ifndef _WIN32
#include <ranges>
#include <unordered_map>

// For pipe()
#include <unistd.h>
#endif

// *** StdSchedulerProc

// *** StdScheduler

void StdScheduler::Clear()
{
	procs.clear();
#ifdef _WIN32
	eventHandles.clear();
	eventProcs.clear();
#endif
}

void StdScheduler::Add(StdSchedulerProc *const proc)
{
	procs.insert(proc);
}

void StdScheduler::Remove(StdSchedulerProc *const proc)
{
	procs.erase(proc);
}

bool StdScheduler::Execute(int iTimeout)
{
	// Needs at least one process to work properly
	if (!procs.size()) return false;

	// Get timeout
	for (auto *const proc : procs)
	{
		if (const int procTimeout{proc->GetTimeout()}; procTimeout >= 0)
		{
			if (iTimeout == -1 || iTimeout > procTimeout)
			{
				iTimeout = procTimeout;
			}
		}
	}

#ifdef _WIN32
	eventHandles.clear();
	eventProcs.clear();

	// Collect event handles
	for (auto *const proc : procs)
	{
		if (const HANDLE event{proc->GetEvent()}; event)
		{
			eventHandles.emplace_back(event);
			eventProcs.emplace_back(proc);
		}
	}

	// Add Unblocker
	eventHandles.emplace_back(unblocker.GetEvent());

	// Wait for something to happen
	const DWORD ret{WaitForMultipleObjects(eventHandles.size(), eventHandles.data(), false, iTimeout < 0 ? INFINITE : iTimeout)};

	bool success{false};

	if (ret != WAIT_TIMEOUT)
	{
		// Which event?
		const auto eventNumber = ret - WAIT_OBJECT_0;

		// Execute the signaled proces
		if (eventNumber < eventHandles.size() - 1)
		{
			if (!eventProcs[eventNumber]->Execute())
			{
				OnError(eventProcs[eventNumber]);
				success = false;
			}
		}
	}

#else
	fds.resize(1);

	struct FdRange
	{
		std::size_t Offset;
		std::size_t Size;
	};

	std::unordered_map<StdSchedulerProc *, FdRange> fdMap;

	for (auto *const proc : procs)
	{
		const std::size_t oldSize{fds.size()};
		proc->GetFDs(fds);

		assert(fds.size() >= oldSize);

		if (fds.size() != oldSize)
		{
			fdMap.emplace(std::piecewise_construct, std::forward_as_tuple(proc), std::forward_as_tuple(oldSize, fds.size() - oldSize));
		}
	}

	// Wait for something to happen
	const int cnt{StdSync::Poll(fds, iTimeout)};

	bool success{true};

	if (cnt > 0)
	{
		// Unblocker? Flush
		if (fds[0].revents & POLLIN)
		{
			unblocker.Reset();
		}

		const std::span<pollfd> fdSpan{fds};

		for (const auto &[proc, range] : fdMap)
		{
			if (std::ranges::any_of(fdSpan.subspan(range.Offset, range.Size), std::identity{}, &pollfd::revents))
			{
				if (!proc->Execute(0))
				{
					OnError(proc);
					success = false;
				}
			}
		}
	}
	else if (cnt < 0)
	{
		printf("StdScheduler::Execute: poll failed %s\n", strerror(errno));
	}

#endif

	for (auto *const proc : procs)
	{
		if (proc->GetTimeout() == 0)
		{
			if (!proc->Execute())
			{
				OnError(proc);
				success = false;
			}
		}
	}

	return success;
}

void StdScheduler::UnBlock()
{
	unblocker.Set();
}

// *** StdSchedulerThread

StdSchedulerThread::~StdSchedulerThread()
{
	// Stop thread
	if (fThread) Stop();
}

void StdSchedulerThread::Add(StdSchedulerProc *pProc)
{
	// Thread is running? Stop it first
	bool fGotThread = fThread;
	if (fGotThread) Stop();
	// Set
	StdScheduler::Add(pProc);
	// Restart
	if (fGotThread) Start();
}

void StdSchedulerThread::Remove(StdSchedulerProc *pProc)
{
	// Thread is running? Stop it first
	bool fGotThread = fThread;
	if (fGotThread) Stop();
	// Set
	StdScheduler::Remove(pProc);
	// Restart
	if (fGotThread) Start();
}

bool StdSchedulerThread::Start()
{
	// already running? stop
	if (fThread) Stop();
	// begin thread
	runThreadRun.store(true, std::memory_order_release);
	thread = C4Thread::Create({"StdSchedulerThread"}, [this]
	{
		while (runThreadRun.load(std::memory_order_acquire))
		{
			Execute();
		}
	});
	fThread = true;
	// success?
	return true;
}

void StdSchedulerThread::Stop()
{
	// Not running?
	if (!fThread) return;
	// Set flag
	runThreadRun.store(false, std::memory_order_release);
	// Unblock
	UnBlock();
	thread.join();
	fThread = false;
	// ok
	return;
}
