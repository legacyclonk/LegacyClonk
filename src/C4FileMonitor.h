// by guenther, 2008

// An inotify wrapper

#ifndef STD_FILE_MONITOR_H_INC
#define STD_FILE_MONITOR_H_INC

#include <StdScheduler.h>
#include <C4InteractiveThread.h>
#include <map>

class C4FileMonitor: public StdSchedulerProc, public C4InteractiveThread::Callback {

public:

	typedef void (*ChangeNotify)(const char *, const char *);

	C4FileMonitor(ChangeNotify pCallback);
	~C4FileMonitor();

	void StartMonitoring();
	void AddDirectory(const char *szDir);
	void AddTree(const char *szDir);
	//void Remove(const char * file);

	// StdSchedulerProc:
	virtual bool Execute(int iTimeout = -1);

	// Signal for calling Execute()
#ifdef STDSCHEDULER_USE_EVENTS
	virtual HANDLE GetEvent();
#else
	virtual void GetFDs(fd_set *pFDs, int *pMaxFD);
#endif

	// C4InteractiveThread::Callback:
	virtual void OnThreadEvent(C4InteractiveEventType eEvent, void *pEventData);

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
		StdCopyStrBuf DirName;
		OVERLAPPED ov;
		char Buffer[1024];
		TreeWatch *Next;
		};
	TreeWatch *pWatches;

	void HandleNotify(const char *szDir, const struct _FILE_NOTIFY_INFORMATION *pNotify);
#endif
};

#endif // STD_FILE_MONITOR_H_INC
