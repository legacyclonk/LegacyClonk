/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Log file handling */

#ifndef INC_C4Log
#define INC_C4Log

#include <StdBuf.h>
#include <StdCompiler.h>

BOOL OpenLog();
BOOL CloseLog();
BOOL DebugLog(const char *strMessage);
BOOL DebugLogF(const char *strMessage ...) GNUC_FORMAT_ATTRIBUTE;

bool LogFatal(const char *szMessage); // log message and store it as a fatal error
void ResetFatalError();               // clear any fatal error message
const char *GetFatalError();          // return message that was set as fatal error, if any

size_t GetLogPos(); // get current log position;
bool GetLogSection(size_t iStart, size_t iLength, StdStrBuf &rsOut); // re-read log data from file

// Used to print a backtrace after a crash
int GetLogFD();

#endif
