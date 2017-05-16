/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Some functions to help with loading and saving AVI files using Video for Windows */

#pragma once

#ifdef _WIN32

#include <vfw.h>

bool AVIOpenOutput(const char *szFilename,
	PAVIFILE *ppAviFile,
	PAVISTREAM *ppAviStream,
	int iWidth, int iHeight);

bool AVICloseOutput(PAVIFILE *ppAviFile,
	PAVISTREAM *ppAviStream);

bool AVIPutFrame(PAVISTREAM pAviStream,
	long lFrame,
	void *lpInfo, long lInfoSize,
	void *lpData, long lDataSize);

#endif // _WIN32
