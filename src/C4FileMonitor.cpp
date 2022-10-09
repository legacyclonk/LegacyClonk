/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2008, guenther
 * Copyright (c) 2017-2021, The LegacyClonk Team and contributors
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

#include <C4Include.h>
#include <C4FileMonitor.h>
#include <C4Application.h>
#include <C4Log.h>

#include <StdFile.h>

#ifdef __linux__

#include <cerrno>

#include <sys/inotify.h>

C4FileMonitor::C4FileMonitor(ChangeNotify pCallback) : pCallback(pCallback), fStarted(false)
{
	fd = inotify_init();
	if (fd == -1) LogF("inotify_init %s", strerror(errno));
}

C4FileMonitor::~C4FileMonitor()
{
	if (fStarted)
	{
		C4InteractiveThread &Thread = Application.InteractiveThread;
		Thread.RemoveProc(this);
		Thread.ClearCallback(Ev_FileChange, this);
	}
	while (close(fd) == -1 && errno == EINTR) {}
}

void C4FileMonitor::StartMonitoring()
{
	if (fStarted) return;
	C4InteractiveThread &Thread = Application.InteractiveThread;
	Thread.AddProc(this);
	Thread.SetCallback(Ev_FileChange, this);
	fStarted = true;
}

void C4FileMonitor::AddDirectory(const char *file)
{
	// Add IN_CLOSE_WRITE?
	int wd = inotify_add_watch(fd, file, IN_CREATE | IN_MODIFY | IN_MOVED_TO | IN_MOVE_SELF | IN_ONLYDIR);
	if (wd == -1)
		LogF("inotify_add_watch %s", strerror(errno));
	watch_descriptors[wd] = file;
}

bool C4FileMonitor::Execute(int iTimeout) // some other thread
{
	char buf[sizeof(inotify_event) + _MAX_FNAME + 1];
	if (read(fd, buf, sizeof(buf)) > 0)
	{
		const char *file = watch_descriptors[reinterpret_cast<inotify_event *>(buf)->wd];
		uint32_t mask = reinterpret_cast<inotify_event *>(buf)->mask;
		C4InteractiveThread &Thread = Application.InteractiveThread;

		if (mask & IN_CREATE || mask & IN_MODIFY || mask & IN_MOVED_TO || mask & IN_MOVE_SELF)
		{
			Thread.PushEvent(Ev_FileChange, file);
		}
		// FIXME: (*(inotify_event*)buf).name);
	}
	else
	{
		Log("inotify buffer too small");
	}
	return true;
}

void C4FileMonitor::OnThreadEvent(C4InteractiveEventType eEvent, const std::any &eventData) // main thread
{
	if (eEvent != Ev_FileChange) return;
	pCallback(std::any_cast<const char *>(eventData), nullptr);
}

void C4FileMonitor::GetFDs(fd_set *pFDs, int *pMaxFD)
{
	FD_SET(fd, pFDs);
	if (pMaxFD) *pMaxFD = (std::max)(*pMaxFD, fd);
}

#elif defined(_WIN32)

C4FileMonitor::C4FileMonitor(ChangeNotify pCallback)
	: pCallback(pCallback), pWatches(nullptr), fStarted(false)
{
	hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
}

C4FileMonitor::~C4FileMonitor()
{
	if (fStarted)
	{
		C4InteractiveThread &Thread = Application.InteractiveThread;
		Thread.RemoveProc(this);
		Thread.ClearCallback(Ev_FileChange, this);
	}
	while (pWatches)
	{
		CloseHandle(pWatches->hDir);
		TreeWatch *pDelete = pWatches;
		pWatches = pWatches->Next;
		delete pDelete;
	}
	CloseHandle(hEvent);
}

void C4FileMonitor::StartMonitoring()
{
	C4InteractiveThread &Thread = Application.InteractiveThread;
	Thread.AddProc(this);
	Thread.SetCallback(Ev_FileChange, this);
	fStarted = true;
}

const DWORD C4FileMonitorNotifies = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE;

void C4FileMonitor::AddDirectory(const char *szDir)
{
	// Create file handle
	HANDLE hDir = CreateFile(szDir, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, 0);
	if (hDir == INVALID_HANDLE_VALUE) return;
	// Create tree watch structure
	TreeWatch *pWatch = new TreeWatch();
	pWatch->hDir = hDir;
	pWatch->DirName = szDir;
	// Build description of async operation
	pWatch->ov = {};
	pWatch->ov.hEvent = hEvent;
	// Add to list
	pWatch->Next = pWatches;
	pWatches = pWatch;
	// Start async directory change notification
	if (!ReadDirectoryChangesW(hDir, pWatch->Buffer, sizeof(pWatch->Buffer), FALSE, C4FileMonitorNotifies, nullptr, &pWatch->ov, nullptr))
		if (GetLastError() != ERROR_IO_PENDING)
		{
			delete pWatch;
			return;
		}
}

bool C4FileMonitor::Execute(int iTimeout)
{
	// Check event
	if (WaitForSingleObject(hEvent, iTimeout) != WAIT_OBJECT_0)
		return true;
	// Check handles
	for (TreeWatch *pWatch = pWatches; pWatch; pWatch = pWatch->Next)
	{
		DWORD dwBytes = 0;
		// Has a notification?
		if (GetOverlappedResult(pWatch->hDir, &pWatch->ov, &dwBytes, FALSE))
		{
			// Read notifications
			const char *pPos = pWatch->Buffer;
			for (;;)
			{
				const _FILE_NOTIFY_INFORMATION *pNotify = reinterpret_cast<const _FILE_NOTIFY_INFORMATION *>(pPos);
				// Handle
				HandleNotify(pWatch->DirName.getData(), pNotify);
				// Get next entry
				if (!pNotify->NextEntryOffset) break;
				pPos += pNotify->NextEntryOffset;
				if (pPos >= pWatch->Buffer + std::min<size_t>(sizeof(pWatch->Buffer), dwBytes))
					break;
				break;
			}
			// Restart directory change notification (flush queue)
			ReadDirectoryChangesW(pWatch->hDir, pWatch->Buffer, sizeof(pWatch->Buffer), FALSE, C4FileMonitorNotifies, nullptr, &pWatch->ov, nullptr);
			dwBytes = 0;
			while (GetOverlappedResult(pWatch->hDir, &pWatch->ov, &dwBytes, FALSE))
			{
				ReadDirectoryChangesW(pWatch->hDir, pWatch->Buffer, sizeof(pWatch->Buffer), FALSE, C4FileMonitorNotifies, nullptr, &pWatch->ov, nullptr);
				dwBytes = 0;
			}
		}
	}
	ResetEvent(hEvent);
	return true;
}

void C4FileMonitor::OnThreadEvent(C4InteractiveEventType eEvent, const std::any &eventData) // main thread
{
	if (eEvent != Ev_FileChange) return;

	pCallback(std::any_cast<const StdStrBuf &>(eventData).getData(), 0);
}

HANDLE C4FileMonitor::GetEvent()
{
	return hEvent;
}

void C4FileMonitor::HandleNotify(const char *szDir, const _FILE_NOTIFY_INFORMATION *pNotify)
{
	// Get filename length
	UINT iCodePage = CP_ACP /* future: CP_UTF8 */;
	int iFileNameBytes = WideCharToMultiByte(iCodePage, 0,
		pNotify->FileName, pNotify->FileNameLength / 2, nullptr, 0, nullptr, nullptr);
	// Set up filename buffer
	StdStrBuf Path(szDir);
	Path.AppendChar(DirectorySeparator);
	Path.Grow(iFileNameBytes);
	char *pFilename = Path.getMPtr(SLen(Path.getData()));
	// Convert filename
	int iWritten = WideCharToMultiByte(iCodePage, 0,
		pNotify->FileName, pNotify->FileNameLength / 2,
		pFilename, iFileNameBytes,
		nullptr, nullptr);
	if (iWritten != iFileNameBytes)
		Path.Shrink(iFileNameBytes + 1);
	// Send notification
	Application.InteractiveThread.PushEvent(Ev_FileChange, Path);
}

#else

// Stubs
C4FileMonitor::C4FileMonitor(ChangeNotify) {}
C4FileMonitor::~C4FileMonitor() {}
bool C4FileMonitor::Execute(int iTimeout) { return false; /* blarg... function must return a value */ }
void C4FileMonitor::StartMonitoring() {}
void C4FileMonitor::OnThreadEvent(C4InteractiveEventType eEvent, const std::any &eventData) {}
void C4FileMonitor::AddDirectory(const char *szDir) {}

// Signal for calling Execute()
#ifdef _WIN32
HANDLE C4FileMonitor::GetEvent() { return 0; }
#else
void C4FileMonitor::GetFDs(fd_set *pFDs, int *pMaxFD) {}
#endif

#endif
