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

C4LogSystem::C4LogSystem()
{
	spdlog::set_automatic_registration(false);

	const auto logLevel = Game.Verbose ? spdlog::level::debug : spdlog::level::warn;

	auto guiSink = std::make_shared<GuiSink>();

	guiSink->set_level(logLevel);
	guiSink->set_pattern("[%L] %v");

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
	loggerDebug = std::make_shared<spdlog::logger>("");
#endif
	loggerDebug->set_level(spdlog::level::trace);
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

	loggerDebugGuiSink = std::make_shared<GuiSink>();
	loggerDebugGuiSink->set_pattern("%v");
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

void C4LogSystem::ResetFatalErrors()
{
	fatalErrors.clear();
}

std::string C4LogSystem::GetFatalErrorString()
{
	return fatalErrors | std::views::join_with('|') | std::ranges::to<std::string>();
}

int iDisableLog = 0;

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
