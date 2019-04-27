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

/* A simple scheduler for ccoperative multitasking */

#ifndef STDSCHEDULER_H
#define STDSCHEDULER_H

#include "Standard.h"

// Events are Windows-specific
#ifdef _WIN32
#define STDSCHEDULER_USE_EVENTS
#define HAVE_WINTHREAD
#ifndef STDSCHEDULER_USE_EVENTS
#include <winsock2.h>
#endif
#else
#include <sys/select.h>
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#endif

// helper
inline int MaxTimeout(int iTimeout1, int iTimeout2)
{
	return (iTimeout1 == -1 || iTimeout2 == -1) ? -1 : Max(iTimeout1, iTimeout2);
}

// Abstract class for a process
class StdSchedulerProc
{
public:
	virtual ~StdSchedulerProc() { }

	// Do whatever the process wishes to do. Should not block longer than the timeout value.
	// Is called whenever the process is signaled or a timeout occurs.
	virtual bool Execute(int iTimeout = -1) = 0;

	// As Execute, but won't return until the given timeout value has elapsed or a failure occurs
	bool ExecuteUntil(int iTimeout = -1);

	// Signal for calling Execute()
#ifdef STDSCHEDULER_USE_EVENTS
	virtual HANDLE GetEvent() { return 0; }
#else
	virtual void GetFDs(fd_set *pFDs, int *pMaxFD) { }
#endif

	// Call Execute() after this time has elapsed (no garantuees regarding accuracy)
	// -1 means no timeout (infinity).
	virtual int GetTimeout() { return -1; }

	// Is the process signal currently set?
	bool IsSignaled();

};

// A simple process scheduler
class StdScheduler
{
public:
	StdScheduler();
	virtual ~StdScheduler();

private:
	// Process list
	StdSchedulerProc **ppProcs;
	int iProcCnt, iProcCapacity;

	// Unblocker
#ifdef STDSCHEDULER_USE_EVENTS
	HANDLE hUnblocker;
#else
	int Unblocker[2];
#endif

	// Dummy lists (preserved to reduce allocs)
#ifdef STDSCHEDULER_USE_EVENTS
	HANDLE *pEventHandles;
	StdSchedulerProc **ppEventProcs;

#endif

public:
	int getProcCnt() const { return iProcCnt; }
	int getProc(StdSchedulerProc *pProc);
	bool hasProc(StdSchedulerProc *pProc) { return getProc(pProc) >= 0; }

	void Clear();
	void Set(StdSchedulerProc **ppProcs, int iProcCnt);
	void Add(StdSchedulerProc *pProc);
	void Remove(StdSchedulerProc *pProc);

	bool Execute(int iTimeout = -1);
	void UnBlock();

protected:
	// overridable
	virtual void OnError(StdSchedulerProc *pProc) { }

private:
	void Enlarge(int iBy);

};

// A simple process scheduler thread
class StdSchedulerThread : public StdScheduler
{
public:
	StdSchedulerThread();
	virtual ~StdSchedulerThread();

private:

	// thread control
	bool fRunThreadRun, fWait;
	
	bool fThread;
#ifdef HAVE_WINTHREAD
	unsigned long iThread;
#elif HAVE_PTHREAD
	pthread_t Thread;
#endif

public:
	void Clear();
	void Set(StdSchedulerProc **ppProcs, int iProcCnt);
	void Add(StdSchedulerProc *pProc);
	void Remove(StdSchedulerProc *pProc);

	bool Start();
	void Stop();

private:

	// thread func
#ifdef HAVE_WINTHREAD
	static void __cdecl _ThreadFunc(void *);
#elif defined(HAVE_PTHREAD)
	static void *_ThreadFunc(void *);
#endif
	unsigned int ThreadFunc();

};

class StdThread
{
private:
	bool fStarted;
	bool fStopSignaled;

#ifdef HAVE_WINTHREAD
	unsigned long iThread;
#elif HAVE_PTHREAD
	pthread_t Thread;
#endif

public:
	StdThread();
	virtual ~StdThread() { Stop(); }

	bool Start();
	void SignalStop(); // mark thread to stop but don't wait
	void Stop();

	bool IsStarted() { return fStarted; }

protected:
	virtual void Execute() = 0;

	bool IsStopSignaled();

private:
	// thread func
#ifdef HAVE_WINTHREAD
	static void __cdecl _ThreadFunc(void *);
#elif defined(HAVE_PTHREAD)
	static void *_ThreadFunc(void *);
#endif
	unsigned int ThreadFunc();
};

#endif
