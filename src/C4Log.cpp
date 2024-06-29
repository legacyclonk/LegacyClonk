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
#include "StdMarkup.h"
#include <C4Language.h>

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

C4LogSystem::LogSink::LogSink()
{
	std::string logFileName{C4CFN_Log};
	// open
	int iLog = 2;
#ifdef _WIN32
	while (!(file = _fsopen(logFileName.c_str(), "wt", _SH_DENYWR)))
#else
	while (!(file = fopen(logFileName.c_str(), "wb")))
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

C4LogSystem::LogSink::~LogSink()
{
	std::fclose(file);
}

void C4LogSystem::LogSink::sink_it_(const spdlog::details::log_msg &msg)
{
	std::string formatted;
	formatter_->format(msg, formatted);
	std::fwrite(formatted.data(), sizeof(char), formatted.size(), file);
}

void C4LogSystem::LogSink::flush_()
{
	std::fflush(file);
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

	Console.Out(message.c_str());

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
		if (!msg.logger_name.empty() && msg.logger_name != "DebugLog")
		{
			dest += std::format("[{}] ", msg.logger_name);
		}
	}

	std::unique_ptr<custom_flag_formatter> clone() const override
	{
		return std::make_unique<LoggerNameIfExistsFormatterFlag>();
	}
};

}

C4LogSystem::C4LogSystem()
{
	spdlog::set_automatic_registration(false);

	const auto logLevel = Game.Verbose ? spdlog::level::debug : spdlog::level::warn;

	auto guiSink = std::make_shared<GuiSink>();

	guiSink->set_level(logLevel);

	auto guiFormatter = std::make_unique<spdlog::pattern_formatter>();
	guiFormatter
			->add_flag<LoggerNameIfExistsFormatterFlag>('~').
			add_flag<LogLevelPrefixFormatterFlag>('*')
			.set_pattern("%~%*%v");
	guiSink->set_formatter(std::move(guiFormatter));

#ifdef _WIN32
	auto debugSink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
	loggerSilent = std::make_shared<spdlog::logger>("", std::initializer_list<spdlog::sink_ptr>{guiSink, debugSink});
#else
	loggerSilent = std::make_shared<spdlog::logger>("", guiSink);
#endif

	loggerSilent->set_level(spdlog::level::trace);

	spdlog::set_default_logger(loggerSilent);
	spdlog::flush_on(spdlog::level::debug);

	logger = loggerSilent;

#ifdef _WIN32
	loggerDebug = std::make_shared<spdlog::logger>("DebugLog", std::move(debugSink));
#else
	loggerDebug = std::make_shared<spdlog::logger>("DebugLog");
#endif
	loggerDebug->set_level(spdlog::level::trace);

	ringbufferSink = std::make_shared<RingbufferSink>(100);
	logger->sinks().emplace_back(ringbufferSink);
}

void C4LogSystem::OpenLog()
{
	auto stdoutColorSink = std::static_pointer_cast<spdlog::sinks::sink>(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	stdoutColorSink->set_level(Game.Verbose ? spdlog::level::debug : spdlog::level::info);

	auto clonkLogSink = std::make_shared<LogSink>();
	clonkLogFD = clonkLogSink->GetFD();

	loggerSilent->sinks().emplace_back(stdoutColorSink);
	loggerSilent->sinks().emplace_back(clonkLogSink);

	logger = std::make_shared<spdlog::logger>("", loggerSilent->sinks().begin() + 1, loggerSilent->sinks().end());
	logger->set_level(spdlog::level::trace);

	auto guiSink = std::make_shared<GuiSink>();
	guiSink->set_pattern("%v");

	logger->sinks().insert(logger->sinks().begin(), std::move(guiSink));

	loggerDebug->sinks().emplace_back(std::move(stdoutColorSink));
	loggerDebug->sinks().emplace_back(clonkLogSink);
	loggerDebug->sinks().emplace_back(ringbufferSink);

	loggerDebugGuiSink = std::make_shared<GuiSink>();

	auto debugFormatter = std::make_unique<spdlog::pattern_formatter>();
	debugFormatter
			->add_flag<LoggerNameIfExistsFormatterFlag>('~').
			add_flag<LogLevelPrefixFormatterFlag>('*')
			.set_pattern("%~%*%v");

	loggerDebugGuiSink->set_formatter(std::move(debugFormatter));
	loggerDebugGuiSink->set_level(spdlog::level::off);

	loggerDebug->sinks().insert(loggerDebug->sinks().begin(), loggerDebugGuiSink);
}

std::shared_ptr<spdlog::logger> C4LogSystem::CreateLogger(std::string name)
{
	return loggerSilent->clone(std::move(name));
}

void C4LogSystem::EnableDebugLog(const bool enable)
{
	loggerDebugGuiSink->set_level(enable ? spdlog::level::debug : spdlog::level::off);
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

std::shared_ptr<spdlog::logger> CreateLogger(std::string name)
{
	return Application.LogSystem.CreateLogger(std::move(name));
}

int GetLogFD()
{
	return Application.LogSystem.GetLogFD();
}
