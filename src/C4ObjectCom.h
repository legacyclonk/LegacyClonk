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

/* Lots of handler functions for object action */

#pragma once

#include <C4Id.h>

void DrawCommandKey(C4Facet &cgo, int32_t iCom,
	BOOL fPressed = FALSE,
	const char *szText = nullptr);

void DrawControlKey(C4Facet &cgo, int32_t iControl,
	BOOL fPressed = FALSE,
	const char *szText = nullptr);

int32_t Control2Com(int32_t iControl, bool fUp);
int32_t Com2Control(int32_t iCom);
int32_t Coms2ComDir(int32_t iComs);
bool ComDirLike(int32_t iComDir, int32_t iSample);
const char *ComName(int32_t iCom);
int32_t ComOrder(int32_t iCom);
StdStrBuf PlrControlKeyName(int32_t iPlayer, int32_t iControl, bool fShort);

const int32_t ComOrderNum = 24;

BOOL PlayerObjectCommand(int32_t plr, int32_t cmdf, C4Object *pTarget = nullptr, int32_t tx = 0, int32_t ty = 0);

BOOL ObjectActionWalk(C4Object *cObj);
BOOL ObjectActionStand(C4Object *cObj);
BOOL ObjectActionJump(C4Object *cObj, FIXED xdir, FIXED ydir, bool fByCom);
BOOL ObjectActionDive(C4Object *cObj, FIXED xdir, FIXED ydir);
BOOL ObjectActionTumble(C4Object *cObj, int32_t dir, FIXED xdir, FIXED ydir);
BOOL ObjectActionGetPunched(C4Object *cObj, FIXED xdir, FIXED ydir);
BOOL ObjectActionKneel(C4Object *cObj);
BOOL ObjectActionFlat(C4Object *cObj, int32_t dir);
BOOL ObjectActionScale(C4Object *cObj, int32_t dir);
BOOL ObjectActionHangle(C4Object *cObj, int32_t dir);
BOOL ObjectActionThrow(C4Object *cObj, C4Object *pThing = nullptr);
BOOL ObjectActionDig(C4Object *cObj);
BOOL ObjectActionBuild(C4Object *cObj, C4Object *pTarget);
BOOL ObjectActionPush(C4Object *cObj, C4Object *pTarget);
BOOL ObjectActionChop(C4Object *cObj, C4Object *pTarget);
BOOL ObjectActionCornerScale(C4Object *cObj);
BOOL ObjectActionFight(C4Object *cObj, C4Object *pTarget);

BOOL ObjectComMovement(C4Object *cObj, int32_t iComDir);
BOOL ObjectComStop(C4Object *cObj);
BOOL ObjectComGrab(C4Object *cObj, C4Object *pTarget);
BOOL ObjectComPut(C4Object *cObj, C4Object *pTarget, C4Object *pThing = nullptr);
BOOL ObjectComThrow(C4Object *cObj, C4Object *pThing = nullptr);
BOOL ObjectComDrop(C4Object *cObj, C4Object *pThing = nullptr);
BOOL ObjectComUnGrab(C4Object *cObj);
BOOL ObjectComJump(C4Object *cObj);
BOOL ObjectComLetGo(C4Object *cObj, int32_t xdirf);
BOOL ObjectComUp(C4Object *cObj);
BOOL ObjectComDig(C4Object *cObj);
BOOL ObjectComChop(C4Object *cObj, C4Object *pTarget);
BOOL ObjectComBuild(C4Object *cObj, C4Object *pTarget);
BOOL ObjectComEnter(C4Object *cObj);
BOOL ObjectComDownDouble(C4Object *cObj);
BOOL ObjectComPutTake(C4Object *cObj, C4Object *pTarget, C4Object *pThing = nullptr);
BOOL ObjectComTake(C4Object *cObj); // carlo
BOOL ObjectComTake2(C4Object *cObj); // carlo
BOOL ObjectComPunch(C4Object *cObj, C4Object *pTarget, int32_t iPunch = 0);
BOOL ObjectComCancelAttach(C4Object *cObj);
void ObjectComDigDouble(C4Object *cObj);
void ObjectComStopDig(C4Object *cObj);

BOOL Buy2Base(int32_t iPlr, C4Object *pBase, C4ID id, BOOL fShowErrors = TRUE);
BOOL SellFromBase(int32_t iPlr, C4Object *pBase, C4ID id, C4Object *pSellObj);
