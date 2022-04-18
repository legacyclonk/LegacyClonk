/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2021, The LegacyClonk Team and contributors
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

/* Some old constants and references */

#pragma once

const int StableRange = 10;
const int AttachRange = 5;
const int CornerRange = AttachRange + 2;

#define GravAccel (Game.Landscape.Gravity)

extern const C4Fixed FloatAccel;
extern const C4Fixed HitSpeed1, HitSpeed2, HitSpeed3, HitSpeed4;
extern const C4Fixed WalkAccel, SwimAccel;
extern const C4Fixed FloatFriction;
extern const C4Fixed RotateAccel;
