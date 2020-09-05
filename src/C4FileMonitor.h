/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2008, guenther
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

// An inotify wrapper

#pragma once

#include <StdScheduler.h>
#include <C4InteractiveThread.h>
#include <map>

class C4FileMonitor : public StdSchedulerProc, public C4InteractiveThread::Callback
{
public:
	typedef void(*ChangeNotify)(const char *, const char *);

	C4FileMonitor(ChangeNotify pCallback);
	~C4FileMonitor();

	void StartMonitoring();
	void AddDirectory(const char *szDir);

	// StdSchedulerProc:
	virtual bool Execute(int iTimeout = -1);

	// Signal for calling Execute()
#ifdef STDSCHEDULER_USE_EVENTS
	virtual HANDLE GetEvent();
#else
	virtual void GetFDs(fd_set *pFDs, int *pMaxFD);
#endif

	// C4InteractiveThread::Callback:
	virtual void OnThreadEvent(C4InteractiveEventType eEvent, const std::any &eventData);

private:
	bool fStarted;
	ChangeNotify pCallback;

#if defined(HAVE_SYS_INOTIFY_H) || defined(HAVE_SYS_SYSCALL_H)
	int fd;
	std::map<int, const char *> watch_descriptors;
#elif defined(_WIN32)
	HANDLE hEvent;

	struct TreeWatch
	{
		HANDLE hDir;
		StdStrBuf DirName;
		OVERLAPPED ov;
		char Buffer[1024];
		TreeWatch *Next;
	};
	TreeWatch *pWatches;

	void HandleNotify(const char *szDir, const struct _FILE_NOTIFY_INFORMATION *pNotify);
#endif
};
