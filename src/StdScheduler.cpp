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
	: ppProcs(nullptr), iProcCnt(0), iProcCapacity(0)
{
#ifdef STDSCHEDULER_USE_EVENTS
	hUnblocker = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	pEventHandles = nullptr;
	ppEventProcs = nullptr;
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

int StdScheduler::getProc(StdSchedulerProc *pProc)
{
	for (int i = 0; i < iProcCnt; i++)
		if (ppProcs[i] == pProc)
			return i;
	return -1;
}

void StdScheduler::Clear()
{
	delete[] ppProcs; ppProcs = nullptr;
#ifdef STDSCHEDULER_USE_EVENTS
	delete[] pEventHandles; pEventHandles = nullptr;
	delete[] ppEventProcs; ppEventProcs = nullptr;
#endif
	iProcCnt = iProcCapacity = 0;
}

void StdScheduler::Add(StdSchedulerProc *pProc)
{
	// Alrady in list?
	if (hasProc(pProc)) return;
	// Enlarge
	if (iProcCnt >= iProcCapacity) Enlarge(1);
	// Add
	ppProcs[iProcCnt] = pProc;
	iProcCnt++;
}

void StdScheduler::Remove(StdSchedulerProc *pProc)
{
	// Search
	int iPos = getProc(pProc);
	// Not found?
	if (iPos >= iProcCnt) return;
	// Remove
	for (int i = iPos + 1; i < iProcCnt; i++)
		ppProcs[i - 1] = ppProcs[i];
	iProcCnt--;
}

bool StdScheduler::Execute(int iTimeout)
{
	// Needs at least one process to work properly
	if (!iProcCnt) return false;

	// Get timeout
	int i; int iProcTimeout;
	for (i = 0; i < iProcCnt; i++)
		if ((iProcTimeout = ppProcs[i]->GetTimeout()) >= 0)
			if (iTimeout == -1 || iTimeout > iProcTimeout)
				iTimeout = iProcTimeout;

#ifdef STDSCHEDULER_USE_EVENTS

	// Collect event handles
	int iEventCnt = 0; HANDLE hEvent;
	for (i = 0; i < iProcCnt; i++)
		if (hEvent = ppProcs[i]->GetEvent())
		{
			pEventHandles[iEventCnt] = hEvent;
			ppEventProcs[iEventCnt] = ppProcs[i];
			iEventCnt++;
		}

	// Add Unblocker
	pEventHandles[iEventCnt++] = hUnblocker;

	// Wait for something to happen
	DWORD ret = WaitForMultipleObjects(iEventCnt, pEventHandles, FALSE, iTimeout < 0 ? INFINITE : iTimeout);

	bool fSuccess = true;

	if (ret != WAIT_TIMEOUT)
	{
		// Which event?
		int iEventNr = ret - WAIT_OBJECT_0;

		// Execute the signaled proces
		if (iEventNr < iEventCnt - 1)
			if (!ppEventProcs[iEventNr]->Execute())
			{
				OnError(ppEventProcs[iEventNr]);
				fSuccess = false;
			}
	}

#else

	// Initialize file descriptor sets
	fd_set fds[2]; int iMaxFDs = Unblocker[0];
	FD_ZERO(&fds[0]); FD_ZERO(&fds[1]);

	// Add Unblocker
	FD_SET(Unblocker[0], &fds[0]);

	// Collect file descriptors
	for (i = 0; i < iProcCnt; i++)
		ppProcs[i]->GetFDs(fds, &iMaxFDs);

	// Build timeout structure
	timeval to = { iTimeout / 1000, (iTimeout % 1000) * 1000 };

	// Wait for something to happen
	int cnt = select(iMaxFDs + 1, &fds[0], &fds[1], nullptr, iTimeout < 0 ? nullptr : &to);

	bool fSuccess = true;

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
		for (i = 0; i < iProcCnt; i++)
		{
			// Get FDs for this process alone
			int test_iMaxFDs = 0;
			FD_ZERO(&test_fds[0]); FD_ZERO(&test_fds[1]);
			ppProcs[i]->GetFDs(test_fds, &test_iMaxFDs);

			// Check intersection
			if (FD_INTERSECTS(test_iMaxFDs + 1, &test_fds[0], &fds[0]) ||
				FD_INTERSECTS(test_iMaxFDs + 1, &test_fds[1], &fds[1]))
			{
				if (!ppProcs[i]->Execute(0))
				{
					OnError(ppProcs[i]);
					fSuccess = false;
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
	for (i = 0; i < iProcCnt; i++)
		if (ppProcs[i]->GetTimeout() == 0)
			if (!ppProcs[i]->Execute())
			{
				OnError(ppProcs[i]);
				fSuccess = false;
			}

	return fSuccess;
}

void StdScheduler::UnBlock()
{
#ifdef STDSCHEDULER_USE_EVENTS
	SetEvent(hUnblocker);
#else
	char c = 42;
	write(Unblocker[1], &c, 1);
#endif
}

void StdScheduler::Enlarge(int iBy)
{
	iProcCapacity += iBy;
	// Realloc
	StdSchedulerProc **ppnProcs = new StdSchedulerProc *[iProcCapacity];
	// Set data
	for (int i = 0; i < iProcCnt; i++)
		ppnProcs[i] = ppProcs[i];
	delete[] ppProcs;
	ppProcs = ppnProcs;
#ifdef STDSCHEDULER_USE_EVENTS
	// Allocate dummy arrays (one handle neede for unlocker!)
	delete[] pEventHandles; pEventHandles = new HANDLE            [iProcCapacity + 1];
	delete[] ppEventProcs;  ppEventProcs  = new StdSchedulerProc *[iProcCapacity];
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
