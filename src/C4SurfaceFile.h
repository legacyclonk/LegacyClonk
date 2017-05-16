/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Another C4Group bitmap-to-surface loader and saver */

#pragma once

class CStdStream;
class C4Surface;

C4Surface *GroupReadSurface(CStdStream &hGroup, BYTE *bpPalette = nullptr);
CSurface8 *GroupReadSurface8(CStdStream &hGroup);
C4Surface *GroupReadSurfacePNG(CStdStream &hGroup);
CSurface8 *GroupReadSurfaceOwnPal8(CStdStream &hGroup);
