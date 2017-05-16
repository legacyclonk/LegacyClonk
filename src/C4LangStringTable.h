// Loads StringTbl* and replaces $..$-strings by localized versions

#pragma once

#include "C4ComponentHost.h"

class C4LangStringTable : public C4ComponentHost
{
public:
	// do replacement in buffer
	// if any replacement is done, the buffer will be realloced
	void ReplaceStrings(StdStrBuf &rBuf);
	void ReplaceStrings(const StdStrBuf &rBuf, StdStrBuf &rTarget, const char *szParentFilePath = NULL);
};
