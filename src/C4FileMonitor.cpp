/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2008, guenther
 * Copyright (c) 2017-2023, The LegacyClonk Team and contributors
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

#ifdef _WIN32
import StdStringEncodingConverter;
#endif

#ifdef __linux__

#include <cerrno>
#include <utility>

#include <sys/inotify.h>
#include <sys/ioctl.h>

static constexpr std::uint32_t NotificationMask{IN_CREATE | IN_MODIFY | IN_MOVED_TO | IN_MOVE_SELF};

C4FileMonitor::C4FileMonitor(ChangeNotifyCallback &&callback)
	: callback{std::move(callback)}
{
	fd = inotify_init();
	if (fd == -1)
	{
		throw std::runtime_error{std::strerror(errno)};
	}
}

C4FileMonitor::~C4FileMonitor()
{
	if (started)
	{
		Application.InteractiveThread.RemoveProc(this);
		Application.InteractiveThread.ClearCallback(Ev_FileChange, this);
	}

	while (close(fd) == -1 && errno == EINTR) {}
}

void C4FileMonitor::StartMonitoring()
{
	Application.InteractiveThread.SetCallback(Ev_FileChange, this);
	Application.InteractiveThread.AddProc(this);
	started = true;
}

void C4FileMonitor::AddDirectory(const char *const path)
{
	const int wd{inotify_add_watch(fd, path, NotificationMask | IN_ONLYDIR)};
	if (wd != -1)
	{
		watchDescriptors[wd] = path;
	}
}

bool C4FileMonitor::Execute(const int timeout) // some other thread
{
	int bytesToRead;
	if (ioctl(fd, FIONREAD, &bytesToRead) < 0)
	{
		return true;
	}

	if (std::cmp_less(bytesToRead, sizeof(inotify_event)))
	{
		return true;
	}

	const auto size = static_cast<std::size_t>(bytesToRead);
	const auto buf = std::make_unique<char[]>(size);

	if (read(fd, buf.get(), static_cast<std::size_t>(bytesToRead)) != bytesToRead)
	{
		return true;
	}

	std::size_t offset{0};
	auto *event = reinterpret_cast<inotify_event *>(buf.get());

	for (;;)
	{
		if (event->mask & NotificationMask)
		{
			Application.InteractiveThread.PushEvent(Ev_FileChange, watchDescriptors[event->wd]);
		}

		const std::size_t eventSize{sizeof(inotify_event) + event->len};

		offset += eventSize;
		if (offset >= size)
		{
			break;
		}

		event = reinterpret_cast<inotify_event *>(reinterpret_cast<char *>(event) + offset);
	}

	return true;
}

void C4FileMonitor::GetFDs(fd_set *pFDs, int *pMaxFD)
{
	FD_SET(fd, pFDs);
	if (pMaxFD) *pMaxFD = std::max(*pMaxFD, fd);
}

#elif defined(_WIN32)

C4FileMonitor::MonitoredDirectory::MonitoredDirectory(winrt::file_handle &&handle, std::string path, const HANDLE event)
	: handle{std::move(handle)}, path{std::move(path)}, overlapped{.hEvent = event}
{
	if (!ReadDirectoryChanges() && GetLastError() != ERROR_IO_PENDING)
	{
		winrt::throw_last_error();
	}
}

void C4FileMonitor::MonitoredDirectory::Execute()
{
	DWORD bytes;
	if (GetOverlappedResult(handle.get(), &overlapped, &bytes, false))
	{
		DWORD offset{0};
		auto *information = reinterpret_cast<FILE_NOTIFY_INFORMATION *>(buffer.data());

		for (;;)
		{
			const std::string filename{StdStringEncodingConverter::Utf16ToWinAcp({static_cast<wchar_t *>(information->FileName), information->FileNameLength / sizeof(wchar_t)})};
			Application.InteractiveThread.PushEvent(Ev_FileChange, path + DirectorySeparator + filename);

			if (!information->NextEntryOffset || offset + information->NextEntryOffset > std::min<std::size_t>(buffer.size(), bytes))
			{
				break;
			}

			information = reinterpret_cast<FILE_NOTIFY_INFORMATION *>(reinterpret_cast<char *>(information) + information->NextEntryOffset);
		}
	}

	do
	{
		ReadDirectoryChanges();
		bytes = 0;
	}
	while (GetOverlappedResult(handle.get(), &overlapped, &bytes, false));
}

bool C4FileMonitor::MonitoredDirectory::ReadDirectoryChanges()
{
	return ReadDirectoryChangesW(
		handle.get(),
		buffer.data(),
		buffer.size(),
		false,
		NotificationFilter,
		nullptr,
		&overlapped,
		nullptr
	);
}

C4FileMonitor::C4FileMonitor(ChangeNotifyCallback &&callback)
	: callback{std::move(callback)}
{
}

C4FileMonitor::~C4FileMonitor()
{
	if (started)
	{
		Application.InteractiveThread.RemoveProc(this);
		Application.InteractiveThread.ClearCallback(Ev_FileChange, this);
	}
}

void C4FileMonitor::StartMonitoring()
{
	Application.InteractiveThread.SetCallback(Ev_FileChange, this);
	Application.InteractiveThread.AddProc(this);
	started = true;
}

void C4FileMonitor::AddDirectory(const char *const path)
{
	winrt::file_handle directory{CreateFile(
		path,
		FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		nullptr,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
		nullptr
	)};

	if (!directory)
	{
		return;
	}

	try
	{
		directories.emplace_back(MonitoredDirectory{
			std::move(directory),
			path,
			event.GetEvent()
		});
	}
	catch (const winrt::hresult_error &)
	{
	}
}

bool C4FileMonitor::Execute(const int timeout)
{
	try
	{
		if (!event.WaitFor(static_cast<std::uint32_t>(timeout)))
		{
			return true;
		}
	}
	catch (const std::runtime_error &)
	{
		return true;
	}

	for (auto &directory : directories)
	{
		directory.Execute();
	}

	event.Reset();
	return true;
}

HANDLE C4FileMonitor::GetEvent()
{
	return event.GetEvent();
}

#elif defined(__APPLE__)

C4FileMonitor::C4FileMonitor(ChangeNotifyCallback &&callback)
	: callback{std::move(callback)}
{
}

C4FileMonitor::~C4FileMonitor()
{
	if (started)
	{
		FSEventStreamStop(eventStream);
		FSEventStreamSetDispatchQueue(eventStream, nullptr);
		FSEventStreamRelease(eventStream);

		Application.InteractiveThread.ClearCallback(Ev_FileChange, this);
	}
}

static void EventStreamCallback(ConstFSEventStreamRef streamRef, void *const clientCallbackInfo, const std::size_t numEvents, void *const eventPaths, const FSEventStreamEventFlags *const eventFlags, const FSEventStreamEventId *const eventIds)
{
	for (std::size_t i{0}; i < numEvents; ++i)
	{
		if (eventFlags[i] & (kFSEventStreamEventFlagUserDropped | kFSEventStreamEventFlagKernelDropped))
		{
			continue;
		}

		std::string path{reinterpret_cast<const char **>(eventPaths)[i]};
		if (path.ends_with(DirectorySeparator))
		{
			path.resize(path.size() - 1);
		}

		Application.InteractiveThread.PushEvent(Ev_FileChange, std::move(path));
	}
}

void C4FileMonitor::StartMonitoring()
{
	const CFUniquePtr<CFArrayRef> array{CFArrayCreate(nullptr, reinterpret_cast<const void **>(paths.data()), paths.size(), &kCFTypeArrayCallBacks)};

	FSEventStreamContext context{
		0,
		reinterpret_cast<void *>(this),
		nullptr,
		nullptr,
		[](const void *) { return CFSTR("C4FileMonitor"); }
	};

	eventStream = FSEventStreamCreate(nullptr, &EventStreamCallback, &context, array.get(), kFSEventStreamEventIdSinceNow, 1, 0);

	if (eventStream)
	{
		FSEventStreamSetDispatchQueue(eventStream, dispatch_get_global_queue(QOS_CLASS_BACKGROUND, 0));
		FSEventStreamStart(eventStream);

		Application.InteractiveThread.SetCallback(Ev_FileChange, this);
		started = true;
	}
}

void C4FileMonitor::AddDirectory(const char *const path)
{
	if (!started)
	{
		paths.emplace_back(CFStringCreateWithCString(nullptr, path, kCFStringEncodingUTF8));
	}
}

#endif

void C4FileMonitor::OnThreadEvent(const C4InteractiveEventType event, const std::any &eventData)
{
	if (event != Ev_FileChange) return;

	callback(std::any_cast<const std::string &>(eventData).c_str());
}
