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

#include <array>
#include <functional>
#include <map>

class C4FileMonitor : public StdSchedulerProc, public C4InteractiveThread::Callback
{
public:
	using ChangeNotifyCallback = std::function<void(const char *)>;

#ifdef _WIN32
private:
	class MonitoredDirectory
	{
	public:
		MonitoredDirectory(winrt::file_handle &&handle, std::string path, HANDLE event);

	public:
		void Execute();

	private:
		bool ReadDirectoryChanges();

	private:
		winrt::file_handle handle;
		std::string path;
		OVERLAPPED overlapped{};
		std::array<char, 1024> buffer{};

		static constexpr DWORD NotificationFilter{FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE};
	};
#endif

public:
	C4FileMonitor(ChangeNotifyCallback &&callback);
	~C4FileMonitor();

	void StartMonitoring();
	void AddDirectory(const char *path);

	// StdSchedulerProc:
	virtual bool Execute(int timeout = -1) override;

	// Signal for calling Execute()
#ifdef _WIN32
	virtual HANDLE GetEvent() override;
#else
	virtual void GetFDs(fd_set *pFDs, int *pMaxFD) override;
#endif

	// C4InteractiveThread::Callback:
	virtual void OnThreadEvent(C4InteractiveEventType event, const std::any &eventData) override;

private:
	ChangeNotifyCallback callback;
	bool started{false};

#if defined(__linux__)
	int fd;
	std::map<int, std::string> watchDescriptors;
#elif defined(_WIN32)
	CStdEvent event;
	std::vector<MonitoredDirectory> directories;
#endif
};
