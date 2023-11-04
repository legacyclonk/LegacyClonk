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

#pragma once

#include "C4InteractiveThread.h"
#include "StdBuf.h"
#include "StdScheduler.h"
#include "StdSync.h"

#include <any>
#include <array>
#include <functional>
#include <map>

#ifdef _WIN32
#include "C4ThreadPool.h"
#include "C4WinRT.h"
#elif defined(__APPLE__)
#include <CoreServices/CoreServices.h>
#elif defined(__linux__)
#include "C4Coroutine.h"
#endif

class C4FileMonitor : public C4InteractiveThread::Callback
{
public:
	using ChangeNotifyCallback = std::function<void(const char *)>;

private:
	using TaskType = C4Task::Task<void, C4Task::TaskTraitsColdWaitOnDestruction, C4Task::PromiseTraitsTerminateOnException>;

#ifdef _WIN32
private:
	class MonitoredDirectory
	{
	public:
		MonitoredDirectory(winrt::file_handle &&handle, std::string path);

		MonitoredDirectory(MonitoredDirectory &&) = delete;
		MonitoredDirectory &operator=(MonitoredDirectory &&) = delete;

	public:
		void StartMonitoring();

	private:
		TaskType Execute();

	private:
		winrt::file_handle handle;
		std::string path;
		TaskType task;
		std::array<char, 1024> buffer{};

		static constexpr DWORD NotificationFilter{FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE};
	};
#elif defined(__APPLE__)
	template<typename T>
	struct CFRefDeleter
	{
		using pointer = T;

		void operator()(T ref)
		{
			CFRelease(ref);
		}
	};

	template<typename T>
	using CFUniquePtr = std::unique_ptr<T, CFRefDeleter<T>>;
#endif

public:
	C4FileMonitor(ChangeNotifyCallback &&callback);
	~C4FileMonitor();

	void StartMonitoring();
	void AddDirectory(const char *path);

	// C4InteractiveThread::Callback:
	virtual void OnThreadEvent(C4InteractiveEventType event, const std::any &eventData) override;

#ifdef __linux__
	TaskType Execute();
#endif

private:
	ChangeNotifyCallback callback;
#ifndef __linux__
	bool started{false};
#endif

#if defined(__linux__)
	int fd;
	TaskType task;
	std::map<int, std::string> watchDescriptors;
#elif defined(_WIN32)
	std::vector<std::unique_ptr<MonitoredDirectory>> directories;
#elif defined(__APPLE__)
	std::vector<CFUniquePtr<CFStringRef>> paths;
	FSEventStreamRef eventStream;
#endif
};
