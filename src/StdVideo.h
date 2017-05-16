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
