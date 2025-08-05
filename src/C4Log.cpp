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

#include <C4Log.h>

#include <C4Console.h>
#include <C4GameLobby.h>
#include <C4LogBuf.h>
#include "C4TextEncoding.h"
#include "StdMarkup.h"

#ifdef _WIN32
#include <share.h>
#endif

#include <ranges>

#include <spdlog/cfg/helpers.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/ringbuffer_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#ifdef _WIN32
#include <spdlog/sinks/msvc_sink.h>
#endif

namespace
{
class LogLevelPrefixFormatterFlag : public spdlog::custom_flag_formatter
{
public:
	void format(const spdlog::details::log_msg &msg, const tm &, std::string &dest) override
	{
		switch (msg.level)
		{
		case spdlog::level::trace:
			dest += "TRACE: ";
			break;

		case spdlog::level::debug:
			dest += "DEBUG: ";
			break;

		case spdlog::level::info:
			break;

		case spdlog::level::warn:
			dest += "WARNING: ";
			break;

		case spdlog::level::err:
			dest += "ERROR: ";
			break;

		case spdlog::level::critical:
			dest += "CRITICAL: ";
			break;

		default:
			std::unreachable();
		}
	}

	std::unique_ptr<custom_flag_formatter> clone() const override
	{
		return std::make_unique<LogLevelPrefixFormatterFlag>();
	}
};


class LoggerNameIfExistsFormatterFlag : public spdlog::custom_flag_formatter
{
public:
	void format(const spdlog::details::log_msg &msg, const tm &, std::string &dest) override
	{
		if (!msg.logger_name.empty())
		{
			dest += std::format("[{}] ", msg.logger_name);
		}
	}

	std::unique_ptr<custom_flag_formatter> clone() const override
	{
		return std::make_unique<LoggerNameIfExistsFormatterFlag>();
	}
};

class StrippedTextFormatterFlag : public spdlog::custom_flag_formatter
{
public:
	void format(const spdlog::details::log_msg &msg, const tm &, std::string &dest) override
	{
		if (msg.payload.empty())
		{
			return;
		}

		dest.resize_and_overwrite(dest.size() + msg.payload.size(), [&msg, oldSize{dest.size()}](char *const ptr, const std::size_t size)
		{
			std::fill_n(ptr + oldSize, size - oldSize, 0xAB);
			auto oldSize2 = oldSize;
			auto &payload = msg.payload;
			auto copied = msg.payload.copy(ptr + oldSize, msg.payload.size());
			ptr[oldSize + copied] = '\0';

			CMarkup markup{false};
			if (markup.StripMarkup(ptr + oldSize))
			{
				return std::strlen(ptr);
			}
			else
			{
				return size;
			}
		});
	}

	std::unique_ptr<custom_flag_formatter> clone() const override
	{
		return std::make_unique<StrippedTextFormatterFlag>();
	}
};
}

C4LogSystem::LogSink::LogSink(std::unique_ptr<spdlog::formatter> formatter)
	: base_sink{std::move(formatter)}
{
	std::string logFileName{C4CFN_Log};
	// open
	int iLog = 2;
#ifdef _WIN32
	while (!(file = C4File{_fsopen(logFileName.c_str(), "wb", _SH_DENYWR)}))
#else
	while (!(file.Open(logFileName, "wb")))
#endif
	{
		if (errno == EACCES)
		{
#ifdef _WIN32
			if (GetLastError() != ERROR_SHARING_VIOLATION)
#endif
				throw CStdApp::StartupException{"Cannot create log file (Permission denied).\nPlease ensure that LegacyClonk is allowed to write in its installation directory."};
		}

		if (iLog == 100)
		{
			// it is extremely unlikely that someone is running 100 clonk instances at the same time in the same directory
			throw CStdApp::StartupException{std::string{"Cannot create log file ("} + strerror(errno) + ")"};
		}

		// try different name
		logFileName = std::format(C4CFN_LogEx, iLog++);
	}
}

void C4LogSystem::LogSink::sink_it_(const spdlog::details::log_msg &msg)
{
	std::string formatted;
	formatter_->format(msg, formatted);
	(void) file.WriteString(formatted);
}

void C4LogSystem::LogSink::flush_()
{
	(void) file.Flush();
}

C4LogSystem::GuiSink::GuiSink(const spdlog::level::level_enum level, const bool showLoggerNameInGui)
{
	set_level(level);

	auto guiFormatter = std::make_unique<spdlog::pattern_formatter>();
	guiFormatter->add_flag<LogLevelPrefixFormatterFlag>('*');
	guiFormatter->add_flag<StrippedTextFormatterFlag>('v');

	if (showLoggerNameInGui)
	{
		guiFormatter->add_flag<LoggerNameIfExistsFormatterFlag>('~').set_pattern("%~%*%v");
	}
	else
	{
		guiFormatter->set_pattern("%*%v");
	}

	set_formatter(std::move(guiFormatter));
}

void C4LogSystem::GuiSink::sink_it_(const spdlog::details::log_msg &msg)
{
	std::string formatted;
	formatter_->format(msg, formatted);

	if (Application.IsMainThread())
	{
		DoLog(formatted);
	}
	else
	{
		Application.InteractiveThread.ExecuteInMainThread([formatted{std::move(formatted)}, weak{weak_from_this()}]
		{
			if (auto strong = weak.lock())
			{
				strong->DoLog(formatted);
			}
		});
	}
}

void C4LogSystem::GuiSink::DoLog(const std::string &message)
{
	if (C4GameLobby::MainDlg *const lobby{Game.Network.GetLobby()}; lobby && Game.pGUI)
	{
		lobby->OnLog(message.c_str());
	}

	Console.Out(message);

	if (Game.GraphicsSystem.MessageBoard.Active)
	{
		Game.GraphicsSystem.MessageBoard.AddLog(message.c_str());
		Game.GraphicsSystem.MessageBoard.LogNotify();
	}
}

std::vector<std::string> C4LogSystem::RingbufferSink::TakeMessages()
{
	const std::lock_guard lock{mutex_};

	const std::size_t size{ringbuffer.size()};
	std::vector<std::string> result;
	result.reserve(size);

	std::ranges::generate_n(std::back_inserter(result), size, [this]
	{
		auto &msg = ringbuffer.front();
		std::string formatted;
		formatter_->format(msg, formatted);

		ringbuffer.pop_front();
		return formatted;
	});

	return result;
}

void C4LogSystem::RingbufferSink::Clear()
{
	const std::lock_guard lock{mutex_};
	ringbuffer = spdlog::details::circular_q<spdlog::details::log_msg_buffer>{size};
}

void C4LogSystem::RingbufferSink::sink_it_(const spdlog::details::log_msg &msg)
{
	ringbuffer.push_back(spdlog::details::log_msg_buffer{msg});
}

void C4LogSystem::ClonkToUtf8Sink::log(const spdlog::details::log_msg &msg)
{
	const std::string payloadAsUtf8{TextEncodingConverter.ClonkToUtf8<char>(msg.payload)};

	spdlog::details::log_msg utf8Msg{msg};
	utf8Msg.payload = payloadAsUtf8;

	for (const auto &sink : sinks)
	{
		sink->log(utf8Msg);
	}
}

void C4LogSystem::ClonkToUtf8Sink::flush()
{
	for (const auto &sink : sinks)
	{
		sink->flush();
	}
}

C4LogSystem::C4LogSystem()
{
	spdlog::set_automatic_registration(false);

	defaultPatternFormatter = std::make_unique<spdlog::pattern_formatter>();
	defaultPatternFormatter->add_flag<LoggerNameIfExistsFormatterFlag>('~');
	defaultPatternFormatter->add_flag<StrippedTextFormatterFlag>('v');
	defaultPatternFormatter->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %~%v");

	loggerSilentGuiSink = std::make_shared<GuiSink>(spdlog::level::warn, true);

#ifdef _WIN32
	debugSink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
	debugSink->set_formatter(defaultPatternFormatter->clone());

	loggerSilent = std::make_shared<spdlog::logger>("", std::initializer_list<spdlog::sink_ptr>{loggerSilentGuiSink, debugSink});
#else
	loggerSilent = std::make_shared<spdlog::logger>("", loggerSilentGuiSink);
#endif

	loggerSilent->set_level(spdlog::level::trace);
	loggerSilent->flush_on(spdlog::level::trace);

	spdlog::set_default_logger(loggerSilent);
	spdlog::flush_on(spdlog::level::debug);

	logger = loggerSilent;

#ifdef _WIN32
	loggerDebug = std::make_shared<spdlog::logger>("DebugLog", debugSink);
#else
	loggerDebug = std::make_shared<spdlog::logger>("DebugLog");
#endif
	loggerDebug->set_level(spdlog::level::trace);
	loggerDebug->flush_on(spdlog::level::trace);

	ringbufferSink = std::make_shared<RingbufferSink>(defaultPatternFormatter->clone(), 100);
	logger->sinks().emplace_back(ringbufferSink);
}

void C4LogSystem::OpenLog(const bool verbose)
{
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8);
#endif

	this->verbose = verbose;

	if (verbose)
	{
		loggerSilentGuiSink->set_level(spdlog::level::trace);
	}

	{
		auto stdoutSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		stdoutSink->set_formatter(defaultPatternFormatter->clone());
		stdoutSink->set_level(AdjustLevelIfVerbose(Config.Logging.LogLevelStdout));

		auto clonkLogSink = std::make_shared<LogSink>(defaultPatternFormatter->clone());
		clonkLogFD = clonkLogSink->GetFD();

		clonkToUtf8Sink = std::make_shared<ClonkToUtf8Sink>(
							  std::initializer_list<spdlog::sink_ptr>{
								  std::static_pointer_cast<spdlog::sinks::sink>(std::move(stdoutSink)),
								  std::static_pointer_cast<spdlog::sinks::sink>(std::move(clonkLogSink))
							  });
	}

	loggerSilent->sinks().emplace_back(clonkToUtf8Sink);

	logger = std::make_shared<spdlog::logger>("", loggerSilent->sinks().begin() + 1, loggerSilent->sinks().end());
	logger->set_level(spdlog::level::trace);
	logger->flush_on(spdlog::level::trace);

	auto guiSink = std::make_shared<GuiSink>(std::make_unique<spdlog::pattern_formatter>("%v"));

	logger->sinks().insert(logger->sinks().begin(), std::move(guiSink));

	loggerDebug->sinks().emplace_back(clonkToUtf8Sink);
	loggerDebug->sinks().emplace_back(ringbufferSink);

	loggerDebugGuiSink = std::make_shared<GuiSink>(spdlog::level::off, false);
	loggerDebug->sinks().insert(loggerDebug->sinks().begin(), loggerDebugGuiSink);
}

void C4LogSystem::EnableDebugLog(const bool enable)
{
	if (loggerDebugGuiSink)
	{
		loggerDebugGuiSink->set_level(enable ? spdlog::level::debug : spdlog::level::off);
	}
}

void C4LogSystem::AddFatalError(std::string message)
{
	logger->critical("{}", LoadResStr(C4ResStrTableKey::IDS_ERR_FATAL, message));

	if (!std::ranges::contains(fatalErrors, message))
	{
		fatalErrors.emplace_back(std::move(message));
	}
}

std::string C4LogSystem::GetFatalErrorString()
{
	std::string result;
	for (const auto &error : fatalErrors)
	{
		if (!result.empty())
		{
			result += '|';
		}

		result += error;
	}
	return result;
}

void C4LogSystem::ResetFatalErrors()
{
	fatalErrors.clear();
}

std::vector<std::string> C4LogSystem::GetRingbufferLogEntries()
{
	return ringbufferSink->TakeMessages();
}

void C4LogSystem::ClearRingbuffer()
{
	ringbufferSink->Clear();
}

#ifdef _WIN32

void C4LogSystem::SetConsoleInputCharset(const std::int32_t charset)
{
	SetConsoleCP(charset);
}

#endif

spdlog::level::level_enum C4LogSystem::AdjustLevelIfVerbose(const spdlog::level::level_enum level) const noexcept
{
	return verbose ? spdlog::level::trace : level;
}

std::shared_ptr<spdlog::logger> C4LogSystem::CreateLogger(std::string name, const C4LoggerConfig::ConfigBase config)
{
	auto newLogger = std::make_shared<spdlog::logger>(std::move(name));
	newLogger->set_level(AdjustLevelIfVerbose(config.LogLevel));
	newLogger->flush_on(spdlog::level::trace);

	spdlog::level::level_enum guiLogLevel;
	if (verbose)
	{
		guiLogLevel = spdlog::level::trace;
	}
	else if (config.GuiLogLevel == spdlog::level::n_levels)
	{
		guiLogLevel = loggerSilentGuiSink->level();
	}
	else
	{
		guiLogLevel = config.GuiLogLevel;
	}

	newLogger->sinks().emplace_back(std::make_shared<GuiSink>(guiLogLevel, config.ShowLoggerNameInGui));

#ifdef _WIN32
	newLogger->sinks().emplace_back(debugSink);
#endif

	newLogger->sinks().emplace_back(clonkToUtf8Sink);

	return newLogger;
}

std::shared_ptr<spdlog::logger> C4LogSystem::GetOrCreate(std::string name, C4LoggerConfig::ConfigBase config)
{
	const std::lock_guard lock{getOrCreateMutex};
	if (const auto logger = spdlog::get(name); logger)
	{
		return logger;
	}

	auto logger = CreateLogger(std::move(name), std::move(config));
	spdlog::register_logger(logger);
	return logger;
}

void LogNTr(const spdlog::level::level_enum level, const std::string_view message)
{
	Application.LogSystem.GetLogger()->log(level, message);
}

void LogFatalNTr(std::string message)
{
	Application.LogSystem.AddFatalError(std::move(message));
}

bool DebugLog(const spdlog::level::level_enum level, const std::string_view message)
{
	Application.LogSystem.GetLoggerDebug()->log(level, message);
	return true;
}

int GetLogFD()
{
	return Application.LogSystem.GetLogFD();
}
