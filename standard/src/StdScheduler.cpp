
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

static int pipe(int *phandles)
{ 
	// This doesn't work with select(), rendering the non-event-solution
	// unusable for Win32. Oh well, it isn't desirable performance-wise, anyway.
	return _pipe(phandles, 10, O_BINARY);
}
#endif

#ifdef HAVE_UNISTD_H
// For pipe()
#include <unistd.h>
#endif

// *** StdSchedulerProc

// Keep calling Execute until timeout has elapsed
bool StdSchedulerProc::ExecuteUntil(int iTimeout)
{
	// Infinite?
	if(iTimeout < 0)
		for(;;)
			if(!Execute())
				return false;
	// Calculate endpoint
	unsigned int iStopTime = timeGetTime() + iTimeout;
	for(;;)
	{
		// Call execute with given timeout
		if(!Execute(Max(iTimeout, 0)))
			return false;
		// Calculate timeout
		unsigned int iTime = timeGetTime();
		if(iTime >= iStopTime)
			break;
		iTimeout = int(iStopTime - iTime);
	}
	// All ok.
	return true;
}

// Is this process currently signaled?
bool StdSchedulerProc::IsSignaled()
{
#ifdef STDSCHEDULER_USE_EVENTS
	return GetEvent() && WaitForSingleObject(GetEvent(), 0) == WAIT_OBJECT_0;
#else
	// Initialize file descriptor sets
  fd_set fds[2]; int iMaxFDs = 0;
	FD_ZERO(&fds[0]); FD_ZERO(&fds[1]);

	// Get file descriptors
	GetFDs(fds, &iMaxFDs);

	// Timeout immediatly
	timeval to = { 0, 0 };

	// Test
	return select(iMaxFDs + 1, &fds[0], &fds[1], NULL, &to) > 0;
#endif
}

#ifndef STDSCHEDULER_USE_EVENTS
static bool FD_INTERSECTS(int n, fd_set * a, fd_set * b)
{
	for (int i = 0; i < n; ++i)
		if (FD_ISSET(i, a) && FD_ISSET(i, b))
			return true;
	return false;
}
#endif

// *** StdScheduler

StdScheduler::StdScheduler()
	: ppProcs(NULL), iProcCnt(0), iProcCapacity(0)
{
#ifdef STDSCHEDULER_USE_EVENTS
	hUnblocker = CreateEvent(NULL, false, false, NULL);
	pEventHandles = NULL;
	ppEventProcs = NULL;
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
	for(int i = 0; i < iProcCnt; i++)
		if(ppProcs[i] == pProc)
			return i;
	return -1;
}

void StdScheduler::Clear()
{
	delete[] ppProcs; ppProcs = NULL;
#ifdef STDSCHEDULER_USE_EVENTS
	delete[] pEventHandles; pEventHandles = NULL;
	delete[] ppEventProcs; ppEventProcs = NULL;
#endif
	iProcCnt = iProcCapacity = 0;
}

void StdScheduler::Set(StdSchedulerProc **ppnProcs, int inProcCnt)
{
	// Remove previous data
	Clear();
	// Set size
	Enlarge(inProcCnt - iProcCapacity);
	// Copy new
	iProcCnt = inProcCnt;
	for(int i = 0; i < iProcCnt; i++)
		ppProcs[i] = ppnProcs[i];
}

void StdScheduler::Add(StdSchedulerProc *pProc)
{
	// Alrady in list?
	if(hasProc(pProc)) return;
	// Enlarge 
	if(iProcCnt >= iProcCapacity) Enlarge(1);
	// Add
	ppProcs[iProcCnt] = pProc;
	iProcCnt++;
}

void StdScheduler::Remove(StdSchedulerProc *pProc)
{
	// Search
	int iPos = getProc(pProc);
	// Not found?
	if(iPos >= iProcCnt) return;
	// Remove
	for(int i = iPos + 1; i < iProcCnt; i++)
		ppProcs[i-1] = ppProcs[i];
	iProcCnt--;
}

bool StdScheduler::Execute(int iTimeout)
{
	// Needs at least one process to work properly
	if(!iProcCnt) return false;

	// Get timeout
	int i; int iProcTimeout;
	for(i = 0; i < iProcCnt; i++)
		if((iProcTimeout = ppProcs[i]->GetTimeout()) >= 0)
			if(iTimeout == -1 || iTimeout > iProcTimeout)
				iTimeout = iProcTimeout;

#ifdef STDSCHEDULER_USE_EVENTS

	// Collect event handles
	int iEventCnt = 0; HANDLE hEvent;
	for(i = 0; i < iProcCnt; i++)
		if(hEvent = ppProcs[i]->GetEvent())
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

	if(ret != WAIT_TIMEOUT)
	{
		// Which event?
		int iEventNr = ret - WAIT_OBJECT_0;

		// Execute the signaled proces
		if(iEventNr < iEventCnt - 1)
			if(!ppEventProcs[iEventNr]->Execute())
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
	for(i = 0; i < iProcCnt; i++)
		ppProcs[i]->GetFDs(fds, &iMaxFDs);

	// Build timeout structure
	timeval to = { iTimeout / 1000, (iTimeout % 1000) * 1000 };

	// Wait for something to happen
	int cnt = select(iMaxFDs + 1, &fds[0], &fds[1], NULL, iTimeout < 0 ? NULL : &to);

	bool fSuccess = true;

	if(cnt > 0)
	{
		// Unblocker? Flush
		if(FD_ISSET(Unblocker[0], &fds[0]))
		{
			char c;
			read(Unblocker[0], &c, 1);
		}

		// Which process?
		fd_set test_fds[2];
		for(i = 0; i < iProcCnt; i++)
		{
			// Get FDs for this process alone
			int test_iMaxFDs = 0;
			FD_ZERO(&test_fds[0]); FD_ZERO(&test_fds[1]);
			ppProcs[i]->GetFDs(test_fds, &test_iMaxFDs);

			// Check intersection
			if (FD_INTERSECTS(test_iMaxFDs + 1, &test_fds[0], &fds[0]) ||
				FD_INTERSECTS(test_iMaxFDs + 1, &test_fds[1], &fds[1]))
			{
				if(!ppProcs[i]->Execute(0))
				{
					OnError(ppProcs[i]);
					fSuccess = false;
				}
			}
		}
	}
	else if (cnt < 0)
	{
		printf("StdScheduler::Execute: select failed %s\n",strerror(errno));
	}

#endif

	// Execute all processes with timeout
	for(i = 0; i < iProcCnt; i++)
		if(ppProcs[i]->GetTimeout() == 0)
			if(!ppProcs[i]->Execute())
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
	for(int i = 0; i < iProcCnt; i++)
		ppnProcs[i] = ppProcs[i];
	delete[] ppProcs;
	ppProcs = ppnProcs;
#ifdef STDSCHEDULER_USE_EVENTS
	// Allocate dummy arrays (one handle neede for unlocker!)
	delete[] pEventHandles; pEventHandles = new HANDLE[iProcCapacity + 1];
	delete[] ppEventProcs;  ppEventProcs = new StdSchedulerProc *[iProcCapacity];
#endif
}

// *** StdSchedulerThread

StdSchedulerThread::StdSchedulerThread()
	: fThread(false)
{

}

StdSchedulerThread::~StdSchedulerThread()
{	
	Clear();
}

void StdSchedulerThread::Clear()
{
	// Stop thread
	if(fThread) Stop();
	// Clear scheduler
	StdScheduler::Clear();
}

void StdSchedulerThread::Set(StdSchedulerProc **ppProcs, int iProcCnt)
{
	// Thread is running? Stop it first
	bool fGotThread = fThread;
	if(fGotThread) Stop();
	// Set
	StdScheduler::Set(ppProcs, iProcCnt);
	// Restart
	if(fGotThread) Start();
}

void StdSchedulerThread::Add(StdSchedulerProc *pProc)
{
	// Thread is running? Stop it first
	bool fGotThread = fThread;
	if(fGotThread) Stop();
	// Set
	StdScheduler::Add(pProc);
	// Restart
	if(fGotThread) Start();
}

void StdSchedulerThread::Remove(StdSchedulerProc *pProc)
{
	// Thread is running? Stop it first
	bool fGotThread = fThread;
	if(fGotThread) Stop();
	// Set
  StdScheduler::Remove(pProc);
	// Restart
	if(fGotThread) Start();
}

bool StdSchedulerThread::Start()
{
	// already running? stop
	if(fThread) Stop();
	// begin thread
	fRunThreadRun = true;
#ifdef HAVE_WINTHREAD
	iThread = _beginthread(_ThreadFunc, 0, this);
	fThread = (iThread != -1);
#elif HAVE_PTHREAD
	fThread = !pthread_create(&Thread, NULL, _ThreadFunc, this);
#endif
	// success?
	return fThread;
}

void StdSchedulerThread::Stop()
{
	// Not running?
	if(!fThread) return;
	// Set flag
	fRunThreadRun = false;
	// Unblock
	UnBlock();
#ifdef HAVE_WINTHREAD
	// Wait for thread to terminate itself
	HANDLE hThread = reinterpret_cast<HANDLE>(iThread);
	if(WaitForSingleObject(hThread, 10000) == WAIT_TIMEOUT)
		// ... or kill it in case it refuses to do so
		TerminateThread(hThread, -1);
#elif HAVE_PTHREAD
	// wait for thread to terminate itself
	// (without security - let's trust these unwashed hackers for once)
	pthread_join(Thread, NULL);
#endif
	fThread = false;
	// ok
	return;
}

#ifdef HAVE_WINTHREAD
void __cdecl StdSchedulerThread::_ThreadFunc(void *pPar)
{
	StdSchedulerThread *pThread = reinterpret_cast<StdSchedulerThread *>(pPar);
	_endthreadex(pThread->ThreadFunc());
}
#elif HAVE_PTHREAD
void *StdSchedulerThread::_ThreadFunc(void *pPar)
{
	StdSchedulerThread *pThread = reinterpret_cast<StdSchedulerThread *>(pPar);
	return reinterpret_cast<void *>(pThread->ThreadFunc());
}
#endif

unsigned int StdSchedulerThread::ThreadFunc()
{
	// Keep calling Execute until someone gets fed up and calls StopThread()
	while(fRunThreadRun)
		Execute();
	return(0);
}



StdThread::StdThread() : fStarted(false), fStopSignaled(false)
{

}

bool StdThread::Start()
{
	// already running? stop
	if(fStarted) Stop();
	// begin thread
	fStopSignaled = false;
#ifdef HAVE_WINTHREAD
	iThread = _beginthread(_ThreadFunc, 0, this);
	fStarted = (iThread != -1);
#elif HAVE_PTHREAD
	fStarted = !pthread_create(&Thread, NULL, _ThreadFunc, this);
#endif
	// success?
	return fStarted;
}

void StdThread::SignalStop()
{
	// Not running?
	if(!fStarted) return;
	// Set flag
	fStopSignaled = true;
}

void StdThread::Stop()
{
	// Not running?
	if(!fStarted) return;
	// Set flag
	fStopSignaled = true;
#ifdef HAVE_WINTHREAD
	// Wait for thread to terminate itself
	HANDLE hThread = reinterpret_cast<HANDLE>(iThread);
	if(WaitForSingleObject(hThread, 10000) == WAIT_TIMEOUT)
		// ... or kill him in case he refuses to do so
		TerminateThread(hThread, -1);
#elif HAVE_PTHREAD
	// wait for thread to terminate itself
	// (whithout security - let's trust these unwashed hackers for once)
	pthread_join(Thread, NULL);
#endif
	fStarted = false;
	// ok
	return;
}

#ifdef HAVE_WINTHREAD
void __cdecl StdThread::_ThreadFunc(void *pPar)
{
	StdThread *pThread = reinterpret_cast<StdThread *>(pPar);
	_endthreadex(pThread->ThreadFunc());
}
#elif HAVE_PTHREAD
void *StdThread::_ThreadFunc(void *pPar)
{
	StdThread *pThread = reinterpret_cast<StdThread *>(pPar);
	return reinterpret_cast<void *>(pThread->ThreadFunc());
}
#endif

unsigned int StdThread::ThreadFunc()
{
	// Keep calling Execute until someone gets fed up and calls Stop()
	while(!IsStopSignaled())
		Execute();
	return(0);
}

bool StdThread::IsStopSignaled()
{
	return fStopSignaled;
}
