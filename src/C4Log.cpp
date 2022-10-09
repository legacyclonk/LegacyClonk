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

#include <C4Include.h>
#include <C4Log.h>

#include <C4Console.h>
#include <C4GameLobby.h>
#include <C4LogBuf.h>
#include <C4Language.h>

#ifdef _WIN32
#include <share.h>
#endif

FILE *C4LogFile = nullptr;
time_t C4LogStartTime;
StdStrBuf sLogFileName;

StdStrBuf sFatalError;

void OpenLog()
{
	// open
	sLogFileName = C4CFN_Log; int iLog = 2;
#ifdef _WIN32
	while (!(C4LogFile = _fsopen(sLogFileName.getData(), "wt", _SH_DENYWR)))
#else
	while (!(C4LogFile = fopen(sLogFileName.getData(), "wb")))
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
	// save start time
	time(&C4LogStartTime);
}

bool CloseLog()
{
	// close
	if (C4LogFile) fclose(C4LogFile); C4LogFile = nullptr;
	// ok
	return true;
}

int GetLogFD()
{
	if (C4LogFile)
		return fileno(C4LogFile);
	else
		return -1;
}

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

	// add timestamp
	StdStrBuf TimeMessage;
	TimeMessage.SetLength(11 + SLen(szMessage) + 1);
	strncpy(TimeMessage.getMData(), GetCurrentTimeStamp(false), 10);
	strncpy(TimeMessage.getMData() + 10, " ", 2);

	// output until all data is written
	const char *pSrc = szMessage;
	do
	{
		// timestamp will always be that length
		char *pDest = TimeMessage.getMData() + 11;

		// copy rest of message, skip tags
		CMarkup Markup(false);
		while (*pSrc)
		{
			Markup.SkipTags(&pSrc);
			// break on crlf
			while (*pSrc == '\r') pSrc++;
			if (*pSrc == '\n') { pSrc++; break; }
			// copy otherwise
			if (*pSrc) *pDest++ = *pSrc++;
		}
		*pDest++ = '\n'; *pDest = '\0';

#ifdef HAVE_ICONV
		StdStrBuf Line = Languages.IconvSystem(TimeMessage.getData());
#else
		StdStrBuf &Line = TimeMessage;
#endif

		// Save into log file
		if (C4LogFile)
		{
			fputs(Line.getData(), C4LogFile);
			fflush(C4LogFile);
		}

		// Write to console
		if (fConsole || Game.Verbose)
		{
#if !defined(NDEBUG) && defined(_WIN32)
			// debug: output to VC console
			OutputDebugString(Line.getData());
#endif
			fputs(Line.getData(), stdout);
			fflush(stdout);
		}
	} while (*pSrc);

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

	// Pass on to console
	Console.Out(szMessage);
	// pass on to lobby
	C4GameLobby::MainDlg *pLobby = Game.Network.GetLobby();
	if (pLobby && Game.pGUI) pLobby->OnLog(szMessage);

	// Add message to log buffer
	bool fNotifyMsgBoard = false;
	if (Game.GraphicsSystem.MessageBoard.Active)
	{
		Game.GraphicsSystem.MessageBoard.AddLog(szMessage);
		fNotifyMsgBoard = true;
	}

	// log
	LogSilent(szMessage, true);

	// Notify message board
	if (fNotifyMsgBoard) Game.GraphicsSystem.MessageBoard.LogNotify();

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
	return !!LogF(LoadResStr("IDS_ERR_FATAL"), szMessage);
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
	if (Game.DebugMode)
		return Log(strMessage);
	else
		return LogSilent(strMessage);
}

size_t GetLogPos()
{
	// get current log position
	return FileSize(sLogFileName.getData());
}

bool GetLogSection(size_t iStart, size_t iLength, StdStrBuf &rsOut)
{
	if (!iLength) { rsOut.Clear(); return true; }
	// read section from log file
	CStdFile LogFileRead;
	char *szBuf, *szBufOrig; size_t iSize; // size exclusing terminator
	if (!LogFileRead.Load(sLogFileName.getData(), reinterpret_cast<uint8_t **>(&szBuf), &iSize, 1)) return false;
	szBufOrig = szBuf;
	// reduce to desired buffer section
	if (iStart > iSize) iStart = iSize;
	if (iStart + iLength > iSize) iLength = iSize - iStart;
	szBuf += iStart; szBuf[iLength] = '\0';
	// strip timestamps; convert linebreaks to Clonk-linebreaks '|'
	char *szPosWrite = szBuf; const char *szPosRead = szBuf;
	while (*szPosRead)
	{
		// skip timestamp
		if (*szPosRead == '[')
			while (*szPosRead && *szPosRead != ']') { --iSize; ++szPosRead; }
		// skip whitespace behind timestamp
		if (!*szPosRead) break;
		szPosRead++;
		// copy data until linebreak
		size_t iLen = 0;
		while (*szPosRead && *szPosRead != 0x0d && *szPosRead != 0x0a)
		{
			++szPosRead; ++iLen;
		}
		if (iLen && szPosRead - iLen != szPosWrite) memmove(szPosWrite, szPosRead - iLen, iLen);
		szPosWrite += iLen;
		// skip additional linebreaks
		while (*szPosRead == 0x0d || *szPosRead == 0x0a) ++szPosRead;
		// write a Clonk-linebreak
		if (*szPosRead) *szPosWrite++ = '|';
	}
	// done; create string buffer from data
	rsOut.Copy(szBuf, szPosWrite - szBuf);
	// old buf no longer used
	delete[] szBufOrig;
	// done, success
	return true;
}
