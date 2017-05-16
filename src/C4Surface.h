/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Extension to CSurface that handles bitmaps in C4Group files */

#ifndef INC_C4Surface
#define INC_C4Surface

#include <StdSurface2.h>

class C4Group;

class C4Surface : public CSurface
{
private:
	C4Surface(const C4Surface &rCpy); // do NOT copy
	C4Surface &operator=(const C4Surface &rCpy); // do NOT copy

public:
	C4Surface() : CSurface() {};

	BOOL LoadAny(C4Group &hGroup, const char *szFilename, bool fOwnPal = false, bool fNoErrIfNotFound = false);
	BOOL LoadAny(C4GroupSet &hGroupset, const char *szFilename, bool fOwnPal = false, bool fNoErrIfNotFound = false);
	BOOL Load(C4Group &hGroup, const char *szFilename, bool fOwnPal = false, bool fNoErrIfNotFound = false);
	BOOL SavePNG(C4Group &hGroup, const char *szFilename, bool fSaveAlpha = true, bool fApplyGamma = false, bool fSaveOverlayOnly = false);
	BOOL Copy(C4Surface &fromSfc);
	BOOL ReadPNG(CStdStream &hGroup);
	bool ReadJPEG(CStdStream &hGroup);
};

#endif
