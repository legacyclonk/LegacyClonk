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

/* Lots of handler functions for object action */

#pragma once

#include "C4ForwardDeclarations.h"
#include <C4Id.h>
#include "Fixed.h"

void DrawCommandKey(C4Facet& cgo, int32_t iCom,
	int32_t iPlayer,
	bool fPressed = false,
	bool showText = true);

int32_t Control2Com(int32_t iControl, bool fUp);
int32_t Com2Control(int32_t iCom);
int32_t Coms2ComDir(int32_t iComs);
bool ComDirLike(int32_t iComDir, int32_t iSample);
const char *ComName(int32_t iCom);
int32_t ComOrder(int32_t iCom);
StdStrBuf PlrControlKeyName(int32_t iPlayer, int32_t iControl, bool fShort);

const int32_t ComOrderNum = 24;

bool PlayerObjectCommand(int32_t plr, int32_t cmdf, C4Object *pTarget = nullptr, int32_t tx = 0, int32_t ty = 0);

bool ObjectActionWalk(C4Object *cObj);
bool ObjectActionStand(C4Object *cObj);
bool ObjectActionJump(C4Object *cObj, C4Fixed xdir, C4Fixed ydir, bool fByCom);
bool ObjectActionDive(C4Object *cObj, C4Fixed xdir, C4Fixed ydir);
bool ObjectActionTumble(C4Object *cObj, int32_t dir, C4Fixed xdir, C4Fixed ydir);
bool ObjectActionGetPunched(C4Object *cObj, C4Fixed xdir, C4Fixed ydir);
bool ObjectActionKneel(C4Object *cObj);
bool ObjectActionFlat(C4Object *cObj, int32_t dir);
bool ObjectActionScale(C4Object *cObj, int32_t dir);
bool ObjectActionHangle(C4Object *cObj, int32_t dir);
bool ObjectActionThrow(C4Object *cObj, C4Object *pThing = nullptr);
bool ObjectActionDig(C4Object *cObj);
bool ObjectActionBuild(C4Object *cObj, C4Object *pTarget);
bool ObjectActionPush(C4Object *cObj, C4Object *pTarget);
bool ObjectActionChop(C4Object *cObj, C4Object *pTarget);
bool ObjectActionCornerScale(C4Object *cObj);
bool ObjectActionFight(C4Object *cObj, C4Object *pTarget);

bool ObjectComMovement(C4Object *cObj, int32_t iComDir);
bool ObjectComStop(C4Object *cObj);
bool ObjectComGrab(C4Object *cObj, C4Object *pTarget);
bool ObjectComPut(C4Object *cObj, C4Object *pTarget, C4Object *pThing = nullptr);
bool ObjectComThrow(C4Object *cObj, C4Object *pThing = nullptr);
bool ObjectComDrop(C4Object *cObj, C4Object *pThing = nullptr);
bool ObjectComUnGrab(C4Object *cObj);
bool ObjectComJump(C4Object *cObj);
bool ObjectComLetGo(C4Object *cObj, int32_t xdirf);
bool ObjectComUp(C4Object *cObj);
bool ObjectComDig(C4Object *cObj);
bool ObjectComChop(C4Object *cObj, C4Object *pTarget);
bool ObjectComBuild(C4Object *cObj, C4Object *pTarget);
bool ObjectComEnter(C4Object *cObj);
bool ObjectComDownDouble(C4Object *cObj);
bool ObjectComPutTake(C4Object *cObj, C4Object *pTarget, C4Object *pThing = nullptr);
bool ObjectComTake(C4Object *cObj); // carlo
bool ObjectComTake2(C4Object *cObj); // carlo
bool ObjectComPunch(C4Object *cObj, C4Object *pTarget, int32_t iPunch = 0);
bool ObjectComCancelAttach(C4Object *cObj);
void ObjectComDigDouble(C4Object *cObj);
void ObjectComStopDig(C4Object *cObj);

bool Buy2Base(int32_t iPlr, C4Object *pBase, C4ID id, bool fShowErrors = true);
bool SellFromBase(int32_t iPlr, C4Object *pBase, C4ID id, C4Object *pSellObj);
