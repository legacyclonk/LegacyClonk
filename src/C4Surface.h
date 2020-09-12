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

/* Extension to CSurface that handles bitmaps in C4Group files */

#pragma once

#include <StdSurface2.h>

class C4Group;
class C4GroupSet;

class C4Surface : public CSurface
{
private:
	C4Surface(const C4Surface &rCpy); // do NOT copy
	C4Surface &operator=(const C4Surface &rCpy); // do NOT copy

public:
	C4Surface() : CSurface() {};

	bool LoadAny(C4Group &hGroup, const char *szFilename, bool fOwnPal = false, bool fNoErrIfNotFound = false);
	bool LoadAny(C4GroupSet &hGroupset, const char *szFilename, bool fOwnPal = false, bool fNoErrIfNotFound = false);
	bool Load(C4Group &hGroup, const char *szFilename, bool fOwnPal = false, bool fNoErrIfNotFound = false);
	bool SavePNG(C4Group &hGroup, const char *szFilename, bool fSaveAlpha = true, bool fApplyGamma = false, bool fSaveOverlayOnly = false);
	bool Copy(C4Surface &fromSfc);
	bool ReadPNG(CStdStream &hGroup);
	bool ReadJPEG(CStdStream &hGroup);
};
