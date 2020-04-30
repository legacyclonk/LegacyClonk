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

#pragma once

class CStdStream;
class C4Surface;

C4Surface *GroupReadSurface(CppC4Group &group, const std::string &path, uint8_t *bpPalette = nullptr);
CSurface8 *GroupReadSurface8(CppC4Group &group, const std::string &path);
C4Surface *GroupReadSurfacePNG(CppC4Group &group, const std::string &path);
CSurface8 *GroupReadSurfaceOwnPal8(CppC4Group &group, const std::string &path);
