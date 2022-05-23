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

/* A simple scheduler for ccoperative multitasking */

#pragma once

#include "Standard.h"

#include <set>
#include <thread>
#include <vector>

// Events are Windows-specific
#ifdef _WIN32
	#define STDSCHEDULER_USE_EVENTS
	#ifndef STDSCHEDULER_USE_EVENTS
		#include <winsock2.h>
	#endif
#else
	#include <sys/select.h>
#endif

// helper
inline int MaxTimeout(int iTimeout1, int iTimeout2)
{
	return (iTimeout1 == -1 || iTimeout2 == -1) ? -1 : (std::max)(iTimeout1, iTimeout2);
}

// Abstract class for a process
class StdSchedulerProc
{
public:
	virtual ~StdSchedulerProc() {}

	// Do whatever the process wishes to do. Should not block longer than the timeout value.
	// Is called whenever the process is signaled or a timeout occurs.
	virtual bool Execute(int iTimeout = -1) = 0;

	// Signal for calling Execute()
#ifdef STDSCHEDULER_USE_EVENTS
	virtual HANDLE GetEvent() { return 0; }
#else
	virtual void GetFDs(fd_set *pFDs, int *pMaxFD) {}
#endif

	// Call Execute() after this time has elapsed (no garantuees regarding accuracy)
	// -1 means no timeout (infinity).
	virtual int GetTimeout() { return -1; }
};

// A simple process scheduler
class StdScheduler
{
public:
	StdScheduler();
	virtual ~StdScheduler();

private:
	// Process list
	std::set<StdSchedulerProc *> procs;

	// Unblocker
#ifdef STDSCHEDULER_USE_EVENTS
	HANDLE unblocker;
#else
	int Unblocker[2];
#endif

#ifdef STDSCHEDULER_USE_EVENTS
	std::vector<HANDLE> eventHandles;
	std::vector<StdSchedulerProc *> eventProcs;
#endif

public:
	std::size_t getProcCnt() const { return procs.size(); }
	bool hasProc(StdSchedulerProc *pProc) { return procs.find(pProc) != procs.end(); }

	void Clear();
	void Add(StdSchedulerProc *pProc);
	void Remove(StdSchedulerProc *pProc);

	bool Execute(int iTimeout = -1);
	void UnBlock();

protected:
	// overridable
	virtual void OnError(StdSchedulerProc *proc) { (void) proc; }
};

// A simple process scheduler thread
class StdSchedulerThread : public StdScheduler
{
public:
	StdSchedulerThread();
	virtual ~StdSchedulerThread();

private:
	// thread control
	bool fRunThreadRun;

	bool fThread;
	std::thread thread;

public:
	void Clear();
	void Add(StdSchedulerProc *pProc);
	void Remove(StdSchedulerProc *pProc);

	bool Start();
	void Stop();

private:
	unsigned int ThreadFunc();
};
