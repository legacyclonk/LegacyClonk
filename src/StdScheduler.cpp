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
// For pipe()
#include <unistd.h>
#endif

// *** StdSchedulerProc

#ifndef _WIN32
static bool FD_INTERSECTS(int n, fd_set *a, fd_set *b)
{
	for (int i = 0; i < n; ++i)
		if (FD_ISSET(i, a) && FD_ISSET(i, b))
			return true;
	return false;
}
#endif

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

	// Initialize file descriptor sets
	fd_set fds[2];
	const auto unblockerFD = unblocker.GetFDs()[0];
	int maxFDs{unblockerFD};

	FD_ZERO(&fds[0]); FD_ZERO(&fds[1]);

	// Add Unblocker
	FD_SET(unblockerFD, &fds[0]);

	// Collect file descriptors
	for (auto *const proc : procs)
	{
		proc->GetFDs(fds, &maxFDs);
	}

	// Build timeout structure
	timeval to { iTimeout / 1000, (iTimeout % 1000) * 1000 };

	// Wait for something to happen
	const int cnt{select(maxFDs + 1, &fds[0], &fds[1], nullptr, iTimeout < 0 ? nullptr : &to)};

	bool success{true};

	if (cnt > 0)
	{
		// Unblocker? Flush
		if (FD_ISSET(unblockerFD, &fds[0]))
		{
			unblocker.Reset();
		}

		// Which process?
		fd_set test_fds[2];
		for (auto *const proc : procs)
		{
			// Get FDs for this process alone
			int test_iMaxFDs = 0;
			FD_ZERO(&test_fds[0]); FD_ZERO(&test_fds[1]);
			proc->GetFDs(test_fds, &test_iMaxFDs);

			// Check intersection
			if (FD_INTERSECTS(test_iMaxFDs + 1, &test_fds[0], &fds[0]) ||
				FD_INTERSECTS(test_iMaxFDs + 1, &test_fds[1], &fds[1]))
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
		printf("StdScheduler::Execute: select failed %s\n", strerror(errno));
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
	fRunThreadRun = true;
	thread = std::thread{[this]
	{
		C4Thread::SetCurrentThreadName("StdSchedulerThread");

		while (fRunThreadRun)
		{
			Execute();
		}
	}};
	fThread = true;
	// success?
	return true;
}

void StdSchedulerThread::Stop()
{
	// Not running?
	if (!fThread) return;
	// Set flag
	fRunThreadRun = false;
	// Unblock
	UnBlock();
	thread.join();
	fThread = false;
	// ok
	return;
}
