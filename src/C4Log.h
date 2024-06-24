/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
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

/* Log file handling */

#pragma once

#include "C4ResStrTable.h"

#include <StdBuf.h>
#include <StdCompiler.h>

#include <mutex>
#include <span>

#include <fmt/printf.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>

class C4LogSystem
{
public:
	class LogSink : public spdlog::sinks::base_sink<std::mutex>
	{
	public:
		LogSink();
		~LogSink();

		LogSink(const LogSink &) = delete;
		LogSink &operator=(const LogSink &) = delete;

		LogSink(LogSink &&) = delete;
		LogSink &operator=(LogSink &&) = delete;

	public:
		int GetFD() const noexcept { return fileno(file); }

	protected:
		void sink_it_(const spdlog::details::log_msg &msg) override;
		void flush_() override;

	private:
		FILE *file{nullptr};
	};

	class GuiSink : public spdlog::sinks::base_sink<spdlog::details::null_mutex>, public std::enable_shared_from_this<GuiSink>
	{
	protected:
		void sink_it_(const spdlog::details::log_msg &msg) override;
		void flush_() override {}

	private:
		void DoLog(const std::string &message);
	};

public:
	C4LogSystem();

	C4LogSystem(const C4LogSystem &) = delete;
	C4LogSystem &operator=(const C4LogSystem &) = delete;

	C4LogSystem(C4LogSystem &&) = delete;
	C4LogSystem &operator=(C4LogSystem &&) = delete;

public:
	void OpenLog();
	int GetLogFD() const noexcept { return clonkLogFD; }

public:
	const std::shared_ptr<spdlog::logger> &GetLogger() const noexcept { return logger; }
	const std::shared_ptr<spdlog::logger> &GetLoggerSilent() const noexcept { return loggerSilent; }
	const std::shared_ptr<spdlog::logger> &GetLoggerDebug() const noexcept { return loggerDebug; }

	void AddFatalError(std::string message);
	void ResetFatalErrors();
	std::string GetFatalErrorString();

	std::shared_ptr<spdlog::logger> CreateLogger(std::string name);

	void EnableDebugLog(bool enable);

private:
	std::shared_ptr<spdlog::logger> logger;
	std::shared_ptr<spdlog::logger> loggerSilent;
	std::shared_ptr<spdlog::logger> loggerDebug;
	std::shared_ptr<GuiSink> loggerDebugGuiSink;
	int clonkLogFD{-1};
	std::vector<std::string> fatalErrors;
};

void LogNTr(spdlog::level::level_enum level, std::string_view message);

inline void LogNTr(const std::string_view message)
{
	LogNTr(spdlog::level::info, message);
}

template<typename... Args>
void LogNTr(const spdlog::level::level_enum level, std::format_string<Args...> fmt, Args &&...args)
{
	LogNTr(level, std::format(fmt, std::forward<Args>(args)...));
}

template<typename... Args>
void LogNTr(const std::format_string<Args...> fmt, Args &&...args)
{
	LogNTr(spdlog::level::info, std::format(fmt, std::forward<Args>(args)...));
}

template<typename... Args>
void Log(const spdlog::level::level_enum level, const C4ResStrTableKeyFormat<std::type_identity_t<Args>...> id, Args &&...args)
{
	LogNTr(level, LoadResStr(id, std::forward<Args>(args)...));
}

template<typename... Args>
void Log(const C4ResStrTableKeyFormat<std::type_identity_t<Args>...> id, Args &&...args)
{
	LogNTr(spdlog::level::info, LoadResStr(id, std::forward<Args>(args)...));
}

bool DebugLog(spdlog::level::level_enum level, std::string_view message);

inline bool DebugLog(std::string_view message)
{
	return DebugLog(spdlog::level::info, message);
}

template<typename... Args>
bool DebugLog(const std::format_string<Args...> fmt, Args &&... args)
{
	return DebugLog(spdlog::level::info, std::format(fmt, std::forward<Args>(args)...));
}

template<typename... Args>
bool DebugLog(const spdlog::level::level_enum level, const std::format_string<Args...> fmt, Args &&... args)
{
	return DebugLog(level, std::format(fmt, std::forward<Args>(args)...));
}

void LogFatalNTr(std::string message); // log message and store it as a fatal error

template<typename... Args>
void LogFatalNTr(const std::format_string<Args...> fmt, Args &&...args)
{
	LogFatalNTr(std::format(fmt, std::forward<Args>(args)...));
}

template<typename... Args>
void LogFatal(const C4ResStrTableKeyFormat<std::type_identity_t<Args>...> id, Args &&...args)
{
	LogFatalNTr(LoadResStr(id, std::forward<Args>(args)...));
}

// Used to print a backtrace after a crash
int GetLogFD();
