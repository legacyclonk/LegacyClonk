/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Extension to CSurface that handles bitmaps in C4Group files */

#pragma once

#include <StdSurface2.h>

class C4Group;

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
