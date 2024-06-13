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

#include <StdBuf.h>
#include <StdCompiler.h>

#include <span>

#include <fmt/printf.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>

class C4LogSystem
{
public:
	class LogSink : public spdlog::sinks::base_sink<spdlog::details::null_mutex>
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

public:
	C4LogSystem();

	C4LogSystem(const C4LogSystem &) = delete;
	C4LogSystem &operator=(const C4LogSystem &) = delete;

	C4LogSystem(C4LogSystem &&) = delete;
	C4LogSystem &operator=(C4LogSystem &&) = delete;

public:
	void OpenLog();
	int GetLogFD() const noexcept { return clonkLogSink ? clonkLogSink->GetFD() : -1; }

public:
	std::span<const spdlog::sink_ptr> GetDefaultLogSinks() const noexcept { return defaultLogSinks; }

	const std::shared_ptr<spdlog::logger> &GetLogger() const noexcept { return logger; }
	const std::shared_ptr<spdlog::logger> &GetLoggerSilent() const noexcept { return loggerSilent; }

private:
	std::vector<spdlog::sink_ptr> defaultLogSinks;
	std::shared_ptr<spdlog::logger> logger;
	std::shared_ptr<spdlog::logger> loggerSilent;
	std::shared_ptr<LogSink> clonkLogSink;
};

template<typename... Args>
bool LogF(const std::string_view message, Args... args)
{
	return Log(fmt::sprintf(message, args...));
}

template<typename... Args>
bool LogSilentF(const std::string_view message, Args... args)
{
	return LogSilent(fmt::sprintf(message, args...));
}

bool DebugLog(std::string_view message);

template<typename... Args>
bool DebugLogF(const std::string_view message, Args... args)
{
	return DebugLog(fmt::sprintf(message, args...));
}

bool LogFatal(const char *szMessage); // log message and store it as a fatal error
void ResetFatalError();               // clear any fatal error message
const char *GetFatalError();          // return message that was set as fatal error, if any

// Used to print a backtrace after a crash
int GetLogFD();
