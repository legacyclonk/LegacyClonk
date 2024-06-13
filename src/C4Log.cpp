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

#include <spdlog/sinks/stdout_color_sinks.h>

#ifdef _WIN32
#include <spdlog/sinks/msvc_sink.h>
#endif

C4LogSystem::LogSink::LogSink()
{
	StdStrBuf sLogFileName{C4CFN_Log};
	// open
	int iLog = 2;
#ifdef _WIN32
	while (!(file = _fsopen(sLogFileName.getData(), "wt", _SH_DENYWR)))
#else
	while (!(file = fopen(sLogFileName.getData(), "wb")))
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
		sLogFileName.Format(C4CFN_LogEx, iLog++);
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

class C4InternalLogSink : public spdlog::sinks::base_sink<spdlog::details::null_mutex>, public std::enable_shared_from_this<C4InternalLogSink>
{
protected:
	void sink_it_(const spdlog::details::log_msg &msg) override
	{
		if (!Application.IsMainThread())
		{
			Application.InteractiveThread.ExecuteInMainThread([msg, weak{weak_from_this()}]
			{
				if (auto strong = weak.lock())
				{
					strong->sink_it_(msg);
				}
			});

			return;
		}

		std::string formatted;
		formatter_->format(msg, formatted);

		if (C4GameLobby::MainDlg *const lobby{Game.Network.GetLobby()}; lobby && Game.pGUI)
		{
			lobby->OnLog(formatted.c_str());
		}

		Console.Out(formatted.c_str());

		if (Game.GraphicsSystem.MessageBoard.Active)
		{
			Game.GraphicsSystem.MessageBoard.AddLog(formatted.c_str());
			Game.GraphicsSystem.MessageBoard.LogNotify();
		}
	}

	void flush_() override {}
};

C4LogSystem::C4LogSystem()
	: defaultLogSinks{
		std::make_shared<C4InternalLogSink>(),
		std::make_shared<spdlog::sinks::stdout_color_sink_mt>()
	}
{
	const auto logLevel = Game.Verbose ? spdlog::level::debug : spdlog::level::info;

	defaultLogSinks[0]->set_level(logLevel);
	defaultLogSinks[0]->set_pattern("%v");
	defaultLogSinks[1]->set_level(logLevel);

#ifdef _WIN32
	defaultLogSinks.emplace_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
	defaultLogSinks.back()->set_level(spdlog::level::trace);
#endif

	logger = std::make_shared<spdlog::logger>("", defaultLogSinks.begin(), defaultLogSinks.end());
	logger->set_level(spdlog::level::trace);
	spdlog::set_default_logger(logger);
	spdlog::flush_on(spdlog::level::debug);

	loggerSilent = logger;
}

void C4LogSystem::OpenLog()
{
	if (!(Game.Verbose || Game.DebugMode))
	{
		loggerSilent = logger->clone("silent");
	}

	clonkLogSink = std::make_shared<LogSink>();
	defaultLogSinks.emplace_back(clonkLogSink);
	logger->sinks().emplace_back(clonkLogSink);

	if (Game.Verbose || Game.DebugMode)
	{
		loggerSilent = logger->clone("silent");
	}
}

StdStrBuf sFatalError;

bool LogSilent(const char *szMessage, bool fConsole)
{
	// security
	if (!szMessage) return false;

	if (!Application.IsMainThread())
	{
		if (fConsole)
		{
			return Application.InteractiveThread.ThreadLog(szMessage);
		}
		else
		{
			return Application.InteractiveThread.ThreadLogS(szMessage);
		}
	}

	(fConsole ? Application.LogSystem.GetLogger() : Application.LogSystem.GetLoggerSilent())->info(szMessage);

	return true;
}

bool LogSilent(const char *szMessage)
{
	return LogSilent(szMessage, false);
}

int iDisableLog = 0;

bool Log(const char *szMessage)
{
	if (iDisableLog) return true;
	// security
	if (!szMessage) return false;

	if (!Application.IsMainThread())
	{
		return Application.InteractiveThread.ThreadLog(szMessage);
	}

	spdlog::info("{}", szMessage);

	return true;
}

bool LogFatal(const char *szMessage)
{
	if (!szMessage) szMessage = "(null)";
	// add to fatal error message stack - if not already in there (avoid duplication)
	if (!SSearch(sFatalError.getData(), szMessage))
	{
		if (!sFatalError.isNull()) sFatalError.AppendChar('|');
		sFatalError.Append(szMessage);
	}
	// write to log - note that Log might overwrite a static buffer also used in szMessage
	spdlog::critical("{}", FormatString(LoadResStr(C4ResStrTableKey::IDS_ERR_FATAL), szMessage).getData());
	return true;
}

void ResetFatalError()
{
	sFatalError.Clear();
}

const char *GetFatalError()
{
	return sFatalError.getData();
}

bool DebugLog(const char *strMessage)
{
	spdlog::debug("{}", strMessage);
	return true;
}

int GetLogFD()
{
	return Application.LogSystem.GetLogFD();
}
