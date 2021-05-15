/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2019, The LegacyClonk Team and contributors
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

void OpenLog();
bool CloseLog();

template<typename... Args>
bool LogF(const char *strMessage, Args... args)
{
	return Log(FormatString(strMessage, args...).getData());
}

template<typename... Args>
bool LogSilentF(const char *strMessage, Args... args)
{
	return LogSilent(FormatString(strMessage, args...).getData());
}

bool DebugLog(const char *strMessage);

template<typename... Args>
bool DebugLogF(const char *strMessage, Args... args)
{
	return DebugLog(FormatString(strMessage, args...).getData());
}

bool LogFatal(const char *szMessage); // log message and store it as a fatal error
void ResetFatalError();               // clear any fatal error message
const char *GetFatalError();          // return message that was set as fatal error, if any

size_t GetLogPos(); // get current log position;
bool GetLogSection(size_t iStart, size_t iLength, StdStrBuf &rsOut); // re-read log data from file

// Used to print a backtrace after a crash
int GetLogFD();
