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

/* Another C4Group bitmap-to-surface loader and saver */

#ifndef INC_C4SurfaceFile
#define INC_C4SurfaceFile

class CStdStream;
class C4Surface;

C4Surface *GroupReadSurface(CStdStream &hGroup, BYTE *bpPalette = NULL);
CSurface8 *GroupReadSurface8(CStdStream &hGroup);
C4Surface *GroupReadSurfacePNG(CStdStream &hGroup);
CSurface8 *GroupReadSurfaceOwnPal8(CStdStream &hGroup);

#endif
