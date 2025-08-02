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

#ifndef C4ENGINE
#error C4Log.h must not be included in non-C4ENGINE builds
#endif

#include "C4File.h"
#include "C4ResStrTable.h"

#include "StdAdaptors.h"
#include <StdBuf.h>
#include <StdCompiler.h>

#include <mutex>
#include <span>

#include <fmt/printf.h>
#include <spdlog/spdlog.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/base_sink.h>

class C4ConfigLogging;

namespace C4LoggerConfig
{
	template<typename T>
	struct Defaults
	{
	};

	template<typename T>
	struct Name;

#define C4LOGGERCONFIG_NAME_TYPE(type) template<> struct C4LoggerConfig::Name<type> { static constexpr auto Value = #type; }

	template<typename T>
	concept HasName = requires { Name<T>::Value; };

	struct ConfigBase
	{
		spdlog::level::level_enum LogLevel;
		spdlog::level::level_enum GuiLogLevel;
		bool ShowLoggerNameInGui;

		constexpr bool operator==(const ConfigBase &other) const noexcept = default;
	};

	template<typename T>
	struct Config : ConfigBase
	{
	private:
		struct Marker
		{
			explicit Marker() = default;
		};

		struct ConfigDefaults
		{
			static constexpr spdlog::level::level_enum LogLevel{spdlog::level::trace};
			static constexpr spdlog::level::level_enum GuiLogLevel{spdlog::level::n_levels};
			static constexpr bool ShowLoggerNameInGui{true};
		};

	private:
		Config() = default;
		Config(const spdlog::level::level_enum logLevel, const spdlog::level::level_enum guiLogLevel, const bool showLoggerNameInGui)
			: ConfigBase{logLevel, guiLogLevel, showLoggerNameInGui}
		{
		}

	public:
		void CompileFunc(StdCompiler *const comp) requires HasName<T>
		{
			comp->Value(mkNamingAdapt(mkParAdapt(*this, Marker{}), C4LoggerConfig::Name<T>::Value, Default()));
		}

		void CompileFunc(StdCompiler *const comp, Marker) requires HasName<T>
		{
			comp->Value(mkNamingAdapt(LogLevel, "LogLevel", DefaultLogLevel()));
			comp->Value(mkNamingAdapt(GuiLogLevel, "GuiLogLevel", DefaultGuiLogLevel()));
			comp->Value(mkNamingAdapt(ShowLoggerNameInGui, "ShowLoggerNameInGui", DefaultShowLoggerNameInGui()));
		}

		constexpr bool operator==(const Config &other) const noexcept = default;

	private:
		static consteval spdlog::level::level_enum DefaultLogLevel() noexcept
		{
			if constexpr (requires { Defaults<T>::LogLevel; })
			{
				return Defaults<T>::LogLevel;
			}
			else
			{
				return ConfigDefaults::LogLevel;
			}
		}

		static consteval spdlog::level::level_enum DefaultGuiLogLevel() noexcept
		{
			if constexpr (requires { Defaults<T>::GuiLogLevel; })
			{
				return Defaults<T>::GuiLogLevel;
			}
			else
			{
				return ConfigDefaults::GuiLogLevel;
			}
		}

		static consteval bool DefaultShowLoggerNameInGui() noexcept
		{
			if constexpr (requires { Defaults<T>::ShowLoggerNameInGui; })
			{
				return Defaults<T>::ShowLoggerNameInGui;
			}
			else
			{
				return ConfigDefaults::ShowLoggerNameInGui;
			}
		}

		static constexpr Config<T> Default() noexcept
		{
			return {DefaultLogLevel(), DefaultGuiLogLevel(), DefaultShowLoggerNameInGui()};
		}

		friend class ::C4ConfigLogging;
	};
};

struct C4LogSystemCreateLoggerOptions
{
	spdlog::level::level_enum GuiLogLevel{spdlog::level::n_levels};
	bool ShowLoggerNameInGui{false};
};

class C4LogSystem
{
	class LogSink : public spdlog::sinks::base_sink<std::mutex>
	{
	public:
		LogSink(std::unique_ptr<spdlog::formatter> formatter);

		LogSink(const LogSink &) = delete;
		LogSink &operator=(const LogSink &) = delete;

		LogSink(LogSink &&) = delete;
		LogSink &operator=(LogSink &&) = delete;

	public:
		int GetFD() const noexcept { return fileno(file.GetHandle()); }

	protected:
		void sink_it_(const spdlog::details::log_msg &msg) override;
		void flush_() override;

	private:
		C4File file;
	};

	class GuiSink : public spdlog::sinks::base_sink<spdlog::details::null_mutex>, public std::enable_shared_from_this<GuiSink>
	{
	public:
		GuiSink(std::unique_ptr<spdlog::formatter> formatter) : base_sink{std::move(formatter)} {}
		GuiSink(spdlog::level::level_enum level, bool showLoggerNameInGui);

	protected:
		void sink_it_(const spdlog::details::log_msg &msg) override;
		void flush_() override {}

	private:
		void DoLog(const std::string &message);
	};

	class RingbufferSink : public spdlog::sinks::base_sink<std::mutex>
	{
	public:
		RingbufferSink(std::unique_ptr<spdlog::formatter> formatter, const std::size_t size) : base_sink{std::move(formatter)}, size{size}, ringbuffer{size} {}

	public:
		std::vector<std::string> TakeMessages();
		void Clear();

	protected:
		void sink_it_(const spdlog::details::log_msg &msg) override;
		void flush_() override {}

	private:
		std::size_t size;
		spdlog::details::circular_q<spdlog::details::log_msg_buffer> ringbuffer;
	};

	class ClonkToUtf8Sink : public spdlog::sinks::sink // Not inheriting from base_sink as we don't need the extra formatter
	{
	public:
		ClonkToUtf8Sink(std::initializer_list<spdlog::sink_ptr> sinks) : sinks{sinks} {}

	protected:
		void log(const spdlog::details::log_msg &msg) override;
		void flush() override;
		void set_pattern(const std::string &) override {}
		void set_formatter(std::unique_ptr<spdlog::formatter>) override {}

	private:
		std::vector<spdlog::sink_ptr> sinks;
	};

public:
	C4LogSystem();

	C4LogSystem(const C4LogSystem &) = delete;
	C4LogSystem &operator=(const C4LogSystem &) = delete;

	C4LogSystem(C4LogSystem &&) = delete;
	C4LogSystem &operator=(C4LogSystem &&) = delete;

public:
	void OpenLog(bool verbose);
	int GetLogFD() const noexcept { return clonkLogFD; }

public:
	const std::shared_ptr<spdlog::logger> &GetLogger() const noexcept { return logger; }
	const std::shared_ptr<spdlog::logger> &GetLoggerSilent() const noexcept { return loggerSilent; }
	const std::shared_ptr<spdlog::logger> &GetLoggerDebug() const noexcept { return loggerDebug; }

	void AddFatalError(std::string message);
	void ResetFatalErrors();
	std::string GetFatalErrorString();
	std::vector<std::string> GetRingbufferLogEntries();
	void ClearRingbuffer();

	template<typename T>
	std::shared_ptr<spdlog::logger> CreateLogger(C4LoggerConfig::Config<T> &config)
	{
		return CreateLogger(C4LoggerConfig::Name<T>::Value, config);
	}


	template<typename T>
	std::shared_ptr<spdlog::logger> CreateLoggerWithDifferentName(C4LoggerConfig::Config<T> &config, std::string name)
	{
		return CreateLogger(std::move(name), config);
	}

	template<typename T>
	std::shared_ptr<spdlog::logger> GetOrCreate(C4LoggerConfig::Config<T> &config)
	{
		return GetOrCreate(C4LoggerConfig::Name<T>::Value, config);
	}

	void EnableDebugLog(bool enable);

#ifdef _WIN32
	void SetConsoleInputCharset(std::int32_t charset);
#endif

private:
	spdlog::level::level_enum AdjustLevelIfVerbose(const spdlog::level::level_enum level) const noexcept;
	std::shared_ptr<spdlog::logger> CreateLogger(std::string name, C4LoggerConfig::ConfigBase config);
	std::shared_ptr<spdlog::logger> GetOrCreate(std::string name, C4LoggerConfig::ConfigBase config);

private:
	std::unique_ptr<spdlog::pattern_formatter> defaultPatternFormatter;
	std::shared_ptr<spdlog::logger> logger;
	std::shared_ptr<spdlog::logger> loggerSilent;
	std::shared_ptr<spdlog::logger> loggerDebug;

#ifdef _WIN32
	spdlog::sink_ptr debugSink;
#endif
	std::shared_ptr<ClonkToUtf8Sink> clonkToUtf8Sink;
	std::shared_ptr<GuiSink> loggerSilentGuiSink;

	std::shared_ptr<GuiSink> loggerDebugGuiSink;
	std::shared_ptr<RingbufferSink> ringbufferSink;

	int clonkLogFD{-1};
	bool verbose{false};
	std::vector<std::string> fatalErrors;
	std::mutex getOrCreateMutex;
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

inline void CompileFunc(spdlog::level::level_enum &level, StdCompiler *const comp)
{
	constexpr StdEnumEntry<spdlog::level::level_enum> Values[]
	{
		{"trace", spdlog::level::trace},
		{"debug", spdlog::level::debug},
		{"info", spdlog::level::info},
		{"warn", spdlog::level::warn},
		{"error", spdlog::level::err},
		{"critical", spdlog::level::critical},
		{"off", spdlog::level::off},
	};

	comp->Value(mkEnumAdaptT<std::int32_t>(level, Values));
}
