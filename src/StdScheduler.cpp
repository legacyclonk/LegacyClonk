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

#include "StdScheduler.h"
#include <stdio.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>

#ifdef HAVE_IO_H
#include <io.h>
#endif
#ifdef HAVE_SHARE_H
#include <share.h>
#endif

#ifdef _WIN32

#include <process.h>
#include <mmsystem.h>

#endif

#ifdef HAVE_UNISTD_H
// For pipe()
#include <unistd.h>
#endif

// *** StdSchedulerProc

#ifndef STDSCHEDULER_USE_EVENTS
static bool FD_INTERSECTS(int n, fd_set *a, fd_set *b)
{
	for (int i = 0; i < n; ++i)
		if (FD_ISSET(i, a) && FD_ISSET(i, b))
			return true;
	return false;
}
#endif

// *** StdScheduler

StdScheduler::StdScheduler()
{
#ifdef STDSCHEDULER_USE_EVENTS
	unblocker = CreateEvent(nullptr, FALSE, FALSE, nullptr);
#else
	pipe(Unblocker);
	// Experimental castration of the unblocker.
	fcntl(Unblocker[0], F_SETFL, fcntl(Unblocker[0], F_GETFL) | O_NONBLOCK);
#endif
}

StdScheduler::~StdScheduler()
{
	Clear();
}

void StdScheduler::Clear()
{
	procs.clear();
}

void StdScheduler::Add(StdSchedulerProc *pProc)
{
	procs.insert(pProc);
}

void StdScheduler::Remove(StdSchedulerProc *pProc)
{
	procs.erase(pProc);
}

bool StdScheduler::Execute(int timeout)
{
	// Needs at least one process to work properly
	if (procs.empty()) return false;

	// Get timeout
	for (const auto &proc : procs)
	{
		if (const int32_t procTimeout{proc->GetTimeout()}; procTimeout >= 0 && (timeout == -1 || timeout > procTimeout))
		{
			timeout = procTimeout;
		}
	}

#ifdef STDSCHEDULER_USE_EVENTS

	// Collect event handles
	eventHandles.resize(0);
	eventProcs.resize(0);
	for (const auto &proc : procs)
	{
		if (const HANDLE event{proc->GetEvent()}; event)
		{
			eventHandles.push_back(event);
			eventProcs.push_back(proc);
		}
	}

	// Add unblocker
	eventHandles.push_back(unblocker);

	// Wait for something to happen
	DWORD ret = WaitForMultipleObjects(eventHandles.size(), eventHandles.data(), FALSE, timeout < 0 ? INFINITE : timeout);

	bool success = true;

	switch (ret)
	{
	case WAIT_FAILED:
		success = false;
		break;

	case WAIT_TIMEOUT:
		break;

	default:
		// Which event?
		const std::size_t eventNumber{ret - WAIT_OBJECT_0};
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
	fd_set fds[2]; int iMaxFDs = Unblocker[0];
	FD_ZERO(&fds[0]); FD_ZERO(&fds[1]);

	// Add Unblocker
	FD_SET(Unblocker[0], &fds[0]);

	// Collect file descriptors
	for (const auto &proc : procs)
	{
		proc->GetFDs(fds, &iMaxFDs);
	}

	// Build timeout structure
	timeval to = { timeout / 1000, (timeout % 1000) * 1000 };

	// Wait for something to happen
	int cnt = select(iMaxFDs + 1, &fds[0], &fds[1], nullptr, timeout < 0 ? nullptr : &to);

	bool success = true;

	if (cnt > 0)
	{
		// Unblocker? Flush
		if (FD_ISSET(Unblocker[0], &fds[0]))
		{
			char c;
			read(Unblocker[0], &c, 1);
		}

		// Which process?
		fd_set test_fds[2];
		for (const auto &proc : procs)
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

	// Execute all processes with timeout
	for (const auto &proc : procs)
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
#ifdef STDSCHEDULER_USE_EVENTS
	SetEvent(unblocker);
#else
	char c = 42;
	write(Unblocker[1], &c, 1);
#endif
}

// *** StdSchedulerThread

StdSchedulerThread::StdSchedulerThread()
	: fThread(false) {}

StdSchedulerThread::~StdSchedulerThread()
{
	Clear();
}

void StdSchedulerThread::Clear()
{
	// Stop thread
	if (fThread) Stop();
	// Clear scheduler
	StdScheduler::Clear();
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
