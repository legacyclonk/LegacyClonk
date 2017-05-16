/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Some old constants and references */

#ifndef INC_C4Physics
#define INC_C4Physics

const int StableRange = 10;
const int AttachRange = 5;
const int CornerRange = AttachRange + 2;

#define GravAccel (Game.Landscape.Gravity)

extern const FIXED FloatAccel;
extern const FIXED HitSpeed1, HitSpeed2, HitSpeed3, HitSpeed4;
extern const FIXED WalkAccel, SwimAccel;
extern const FIXED FloatFriction;
extern const FIXED RotateAccel;

#endif
