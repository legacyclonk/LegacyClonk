/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Another C4Group bitmap-to-surface loader and saver */

#ifndef INC_C4SurfaceFile
#define INC_C4SurfaceFile

class CStdStream;
class C4Surface;

C4Surface *GroupReadSurface(CStdStream &hGroup, BYTE *bpPalette=NULL);
CSurface8 *GroupReadSurface8(CStdStream &hGroup);
C4Surface *GroupReadSurfacePNG(CStdStream &hGroup);
CSurface8 *GroupReadSurfaceOwnPal8(CStdStream &hGroup);

/*BOOL SaveSurface(const char *szFilename, SURFACE sfcSurface, BYTE *bpPalette);*/

#endif
