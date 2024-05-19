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

#include <C4Include.h>
#include <C4ObjectCom.h>

#include <C4Object.h>
#include <C4Physics.h>
#include <C4Command.h>
#include <C4Random.h>
#include <C4Game.h>
#include <C4Wrappers.h>
#include <C4ObjectMenu.h>
#include <C4Player.h>
#include <C4SoundSystem.h>

bool SimFlightHitsLiquid(C4Fixed fcx, C4Fixed fcy, C4Fixed xdir, C4Fixed ydir);

bool ObjectActionWalk(C4Object *cObj)
{
	if (!cObj->SetActionByName("Walk")) return false;
	cObj->xdir = cObj->ydir = 0;
	return true;
}

bool ObjectActionStand(C4Object *cObj)
{
	cObj->Action.ComDir = COMD_Stop;
	if (!ObjectActionWalk(cObj)) return false;
	return true;
}

bool ObjectActionJump(C4Object *cObj, C4Fixed xdir, C4Fixed ydir, bool fByCom)
{
	// scripted jump?
	assert(cObj);
	if (cObj->Call(PSF_OnActionJump, {C4VInt(fixtoi(xdir, 100)), C4VInt(fixtoi(ydir, 100)), C4VBool(fByCom)})) return true;
	// hardcoded jump by action
	if (!cObj->SetActionByName("Jump")) return false;
	cObj->xdir = xdir; cObj->ydir = ydir;
	cObj->Mobile = 1;
	// unstick from ground, because jump command may be issued in an Action-callback,
	// where attach-values have already been determined for that frame
	cObj->Action.t_attach &= ~CNAT_Bottom;
	return true;
}

bool ObjectActionDive(C4Object *cObj, C4Fixed xdir, C4Fixed ydir)
{
	if (!cObj->SetActionByName("Dive")) return false;
	cObj->xdir = xdir; cObj->ydir = ydir;
	cObj->Mobile = 1;
	// unstick from ground, because jump command may be issued in an Action-callback,
	// where attach-values have already been determined for that frame
	cObj->Action.t_attach &= ~CNAT_Bottom;
	return true;
}

bool ObjectActionTumble(C4Object *cObj, int32_t dir, C4Fixed xdir, C4Fixed ydir)
{
	if (!cObj->SetActionByName("Tumble")) return false;
	cObj->SetDir(dir);
	cObj->xdir = xdir; cObj->ydir = ydir;
	return true;
}

bool ObjectActionGetPunched(C4Object *cObj, C4Fixed xdir, C4Fixed ydir)
{
	if (!cObj->SetActionByName("GetPunched")) return false;
	cObj->xdir = xdir; cObj->ydir = ydir;
	return true;
}

bool ObjectActionKneel(C4Object *cObj)
{
	if (!cObj->SetActionByName("KneelDown")) return false;
	cObj->xdir = cObj->ydir = 0;
	return true;
}

bool ObjectActionFlat(C4Object *cObj, int32_t dir)
{
	if (!cObj->SetActionByName("FlatUp")) return false;
	cObj->xdir = cObj->ydir = 0;
	cObj->SetDir(dir);
	return true;
}

bool ObjectActionScale(C4Object *cObj, int32_t dir)
{
	if (!cObj->SetActionByName("Scale")) return false;
	cObj->xdir = cObj->ydir = 0;
	cObj->SetDir(dir);
	return true;
}

bool ObjectActionHangle(C4Object *cObj, int32_t dir)
{
	if (!cObj->SetActionByName("Hangle")) return false;
	cObj->xdir = cObj->ydir = 0;
	cObj->SetDir(dir);
	return true;
}

bool ObjectActionThrow(C4Object *cObj, C4Object *pThing)
{
	// No object specified, first from contents
	if (!pThing) pThing = cObj->Contents.GetObject();
	// Nothing to throw
	if (!pThing) return false;
	// Force and direction
	C4Fixed pthrow = ValByPhysical(400, cObj->GetPhysical()->Throw);
	int32_t iDir = 1; if (cObj->Action.Dir == DIR_Left) iDir = -1;
	// Set action
	if (!cObj->SetActionByName("Throw")) return false;
	// Exit object
	pThing->Exit(cObj->x,
		cObj->y + cObj->Shape.y - 1,
		Random(360),
		pthrow * iDir, -pthrow, pthrow * iDir);
	// Success
	return true;
}

bool ObjectActionDig(C4Object *cObj)
{
	if (!cObj->SetActionByName("Dig")) return false;
	cObj->Action.Data = 0; // Material Dig2Object request
	return true;
}

bool ObjectActionBuild(C4Object *cObj, C4Object *target)
{
	return cObj->SetActionByName("Build", target);
}

bool ObjectActionPush(C4Object *cObj, C4Object *target)
{
	return cObj->SetActionByName("Push", target);
}

bool ObjectActionFight(C4Object *cObj, C4Object *target)
{
	return cObj->SetActionByName("Fight", target);
}

bool ObjectActionChop(C4Object *cObj, C4Object *target)
{
	return cObj->SetActionByName("Chop", target);
}

bool CornerScaleOkay(C4Object *cObj, int32_t iRangeX, int32_t iRangeY)
{
	int32_t ctx, cty;
	cty = cObj->y - iRangeY;
	if (cObj->Action.Dir == DIR_Left) ctx = cObj->x - iRangeX;
	else ctx = cObj->x + iRangeX;
	cObj->ContactCheck(ctx, cty); // (resets VtxContact & t_contact)
	if (!(cObj->t_contact & CNAT_Top))
		if (!(cObj->t_contact & CNAT_Left))
			if (!(cObj->t_contact & CNAT_Right))
				if (!(cObj->t_contact & CNAT_Bottom))
					return true;
	return false;
}

bool CheckCornerScale(C4Object *cObj, int32_t &iRangeX, int32_t &iRangeY)
{
	for (iRangeX = CornerRange; iRangeX >= 1; iRangeX--)
		for (iRangeY = CornerRange; iRangeY >= 1; iRangeY--)
			if (CornerScaleOkay(cObj, iRangeX, iRangeY))
				return true;
	return false;
}

bool ObjectActionCornerScale(C4Object *cObj)
{
	int32_t iRangeX, iRangeY;
	// Scaling: check range max to min
	if (cObj->GetProcedure() == DFA_SCALE)
	{
		if (!CheckCornerScale(cObj, iRangeX, iRangeY)) return false;
	}
	// Swimming: check range min to max
	else
	{
		iRangeY = 2;
		while (!CornerScaleOkay(cObj, iRangeY, iRangeY))
		{
			iRangeY++; if (iRangeY > CornerRange) return false;
		}
		iRangeX = iRangeY;
	}
	// Do corner scale
	if (!cObj->SetActionByName("KneelUp"))
		cObj->SetActionByName("Walk");
	cObj->xdir = cObj->ydir = 0;
	if (cObj->Action.Dir == DIR_Left) cObj->fix_x -= itofix(iRangeX);
	else cObj->fix_x += itofix(iRangeX);
	cObj->fix_y -= itofix(iRangeY);
	cObj->x = fixtoi(cObj->fix_x); cObj->y = fixtoi(cObj->fix_y);
	return true;
}

bool ObjectComMovement(C4Object *cObj, int32_t comdir)
{
	cObj->Action.ComDir = comdir;

	PlayerObjectCommand(cObj->Owner, C4CMD_Follow, cObj);
	// direkt turnaround if standing still
	if (!cObj->xdir && (cObj->GetProcedure() == DFA_WALK || cObj->GetProcedure() == DFA_HANGLE))
		switch (comdir)
		{
		case COMD_Left: case COMD_UpLeft: case COMD_DownLeft:
			cObj->SetDir(DIR_Left);
			break;
		case COMD_Right: case COMD_UpRight: case COMD_DownRight:
			cObj->SetDir(DIR_Right);
			break;
		}
	return true;
}

bool ObjectComStop(C4Object *cObj)
{
	// Cease current action
	cObj->SetActionByName("Idle");
	// Action walk if possible
	return ObjectActionStand(cObj);
}

bool ObjectComGrab(C4Object *cObj, C4Object *pTarget)
{
	if (!pTarget) return false;
	if (cObj->GetProcedure() != DFA_WALK) return false;
	if (!ObjectActionPush(cObj, pTarget)) return false;
	cObj->Call(PSF_Grab, {C4VObj(pTarget), C4VBool(true)});
	if (pTarget->Status && cObj->Status)
	{
		pTarget->Controller = cObj->Controller;
		pTarget->Call(PSF_Grabbed, {C4VObj(cObj), C4VBool(true)});
	}
	return true;
}

bool ObjectComUnGrab(C4Object *cObj)
{
	// Only if pushing, -> stand
	if (cObj->GetProcedure() == DFA_PUSH)
	{
		C4Object *pTarget = cObj->Action.Target;
		if (ObjectActionStand(cObj))
		{
			if (!cObj->CloseMenu(false)) return false;
			cObj->Call(PSF_Grab, {C4VObj(pTarget), C4VBool(false)});
			if (pTarget && pTarget->Status && cObj->Status)
				pTarget->Call(PSF_Grabbed, {C4VObj(cObj), C4VBool(false)});
			return true;
		}
	}

	return false;
}

bool ObjectComJump(C4Object *cObj) // by ObjectComUp, ExecCMDFMoveTo, FnJump
{
	// Only if walking
	if (cObj->GetProcedure() != DFA_WALK) return false;
	// Calculate direction & forces
	C4Fixed TXDir = Fix0;
	C4PhysicalInfo *pPhysical = cObj->GetPhysical();
	C4Fixed iPhysicalWalk = ValByPhysical(280, pPhysical->Walk) * itofix(cObj->GetCon(), FullCon);
	C4Fixed iPhysicalJump = ValByPhysical(1000, pPhysical->Jump) * itofix(cObj->GetCon(), FullCon);

	if      (cObj->Action.ComDir == COMD_Left  || cObj->Action.ComDir == COMD_UpLeft)  TXDir = -iPhysicalWalk;
	else if (cObj->Action.ComDir == COMD_Right || cObj->Action.ComDir == COMD_UpRight) TXDir = +iPhysicalWalk;
	else
	{
		if (cObj->Action.Dir == DIR_Left)  TXDir = -iPhysicalWalk;
		if (cObj->Action.Dir == DIR_Right) TXDir = +iPhysicalWalk;
	}
	C4Fixed x = cObj->fix_x, y = cObj->fix_y;
	// find bottom-most vertex, correct starting position for simulation
	int32_t iBtmVtx = cObj->Shape.GetBottomVertex();
	if (iBtmVtx != -1) { x += cObj->Shape.GetVertexX(iBtmVtx); y += cObj->Shape.GetVertexY(iBtmVtx); }
	// Try dive
	if (cObj->Shape.ContactDensity > C4M_Liquid)
		if (SimFlightHitsLiquid(x, y, TXDir, -iPhysicalJump))
			if (ObjectActionDive(cObj, TXDir, -iPhysicalJump))
				return true;
	// Regular jump
	return ObjectActionJump(cObj, TXDir, -iPhysicalJump, true);
}

bool ObjectComLetGo(C4Object *cObj, int32_t xdirf)
{
	// by ACTSCALE, ACTHANGLE or ExecCMDFMoveTo
	return ObjectActionJump(cObj, itofix(xdirf), Fix0, true);
}

bool ObjectComEnter(C4Object *cObj) // by pusher
{
	if (!cObj) return false;

	// NoPushEnter
	if (cObj->Def->NoPushEnter) return false;

	// Check object entrance, try command enter
	C4Object *pTarget;
	uint32_t ocf = OCF_Entrance;
	if ((pTarget = Game.Objects.AtObject(cObj->x, cObj->y, ocf, cObj)))
		if (ocf & OCF_Entrance)
		{
			cObj->SetCommand(C4CMD_Enter, pTarget); return true;
		}

	return false;
}

bool ObjectComUp(C4Object *cObj) // by DFA_WALK or DFA_SWIM
{
	if (!cObj) return false;

	// Check object entrance, try command enter
	C4Object *pTarget;
	uint32_t ocf = OCF_Entrance;
	if ((pTarget = Game.Objects.AtObject(cObj->x, cObj->y, ocf, cObj)))
		if (ocf & OCF_Entrance)
			return PlayerObjectCommand(cObj->Owner, C4CMD_Enter, pTarget);

	// Try jump
	if (cObj->GetProcedure() == DFA_WALK) {
		// Don't jump on the COM_Up key if the controlling player has configured a COM_Jump button
		C4Player* pPlr = Game.Players.Get(cObj->Controller);
		if (pPlr->HasExplicitJumpKey) return false;

		return PlayerObjectCommand(cObj->Owner, C4CMD_Jump);
	}

	return false;
}

bool ObjectComDig(C4Object *cObj) // by DFA_WALK
{
	C4PhysicalInfo *phys = cObj->GetPhysical();
	if (!phys->CanDig || !ObjectActionDig(cObj))
	{
		GameMsgObject(FormatString(LoadResStr("IDS_OBJ_NODIG"), cObj->GetName()).getData(), cObj);
		return false;
	}
	return true;
}

C4Object *CreateLine(C4ID idType, int32_t iOwner, C4Object *pFrom, C4Object *pTo)
{
	C4Object *pLine;
	if (!pFrom || !pTo) return nullptr;
	if (!(pLine = Game.CreateObject(idType, pFrom, iOwner, 0, 0))) return nullptr;
	pLine->Shape.VtxNum = 2;
	pLine->Shape.VtxX[0] = pFrom->x;
	pLine->Shape.VtxY[0] = pFrom->y + pFrom->Shape.Hgt / 4;
	pLine->Shape.VtxX[1] = pTo->x;
	pLine->Shape.VtxY[1] = pTo->y + pTo->Shape.Hgt / 4;
	pLine->Action.Target = pFrom;
	pLine->Action.Target2 = pTo;
	return pLine;
}

bool ObjectComLineConstruction(C4Object *cObj)
{
	C4Object *linekit, *tstruct, *cline;
	uint32_t ocf;

	ObjectActionStand(cObj);

	// Check physical
	if (!cObj->GetPhysical()->CanConstruct)
	{
		GameMsgObject(FormatString(LoadResStr("IDS_OBJ_NOLINECONSTRUCT"), cObj->GetName()).getData(), cObj); return false;
	}

	// Line pickup

	// Check for linekit
	if (!(linekit = cObj->Contents.Find(C4ID_Linekit)))
	{
		// Check for collection limit
		if (cObj->Def->CollectionLimit && (cObj->Contents.ObjectCount() >= cObj->Def->CollectionLimit)) return false;
		// Check line pickup
		ocf = OCF_LineConstruct;
		tstruct = Game.Objects.AtObject(cObj->x, cObj->y, ocf, cObj);
		if (!tstruct || !(ocf & OCF_LineConstruct)) return false;
		if (!(cline = Game.FindObject(C4ID_None, 0, 0, 0, 0, OCF_All, "Connect", tstruct))) return false;
		// Check line connected to linekit at other end
		if ((cline->Action.Target && (cline->Action.Target->Def->id == C4ID_Linekit))
			|| (cline->Action.Target2 && (cline->Action.Target2->Def->id == C4ID_Linekit)))
		{
			StartSoundEffect("Error", false, 100, cObj);
			GameMsgObject(FormatString(LoadResStr("IDS_OBJ_NODOUBLEKIT"), cline->GetName()).getData(), cObj); return false;
		}
		// Create new linekit
		if (!(linekit = Game.CreateObject(C4ID_Linekit, cObj, cline->Owner))) return false;
		// Enter linekit into clonk
		bool fRejectCollect;
		if (!linekit->Enter(cObj, true, true, &fRejectCollect))
		{
			// Enter failed: abort operation
			linekit->AssignRemoval(); return false;
		}
		// Attach line to collected linekit
		StartSoundEffect("Connect", false, 100, cObj);
		if (cline->Action.Target  == tstruct) cline->Action.Target  = linekit;
		if (cline->Action.Target2 == tstruct) cline->Action.Target2 = linekit;
		// Message
		GameMsgObject(FormatString(LoadResStr("IDS_OBJ_DISCONNECT"), cline->GetName(), tstruct->GetName()).getData(), tstruct);
		return true;
	}

	// Active construction

	// Active line construction
	if (cline = Game.FindObject(C4ID_None, 0, 0, 0, 0, OCF_All, "Connect", linekit))
	{
		// Check for structure connection
		ocf = OCF_LineConstruct;
		tstruct = Game.Objects.AtObject(cObj->x, cObj->y, ocf, cObj);
		// No structure
		if (!tstruct || !(ocf & OCF_LineConstruct))
		{
			// No connect
			StartSoundEffect("Error", false, 100, cObj);
			GameMsgObject(LoadResStr("IDS_OBJ_NOCONNECT"), cObj); return false;
		}

		// Check short circuit -> removal
		if ((cline->Action.Target == tstruct)
			|| (cline->Action.Target2 == tstruct))
		{
			StartSoundEffect("Connect", false, 100, cObj);
			GameMsgObject(FormatString(LoadResStr("IDS_OBJ_LINEREMOVAL"), cline->GetName()).getData(), tstruct);
			cline->AssignRemoval();
			return true;
		}

		// Check for correct connection type
		bool connect_okay = false;
		switch (cline->Def->Line)
		{
		case C4D_Line_Power:
			if (tstruct->Def->LineConnect & C4D_Power_Input)  connect_okay = true;
			if (tstruct->Def->LineConnect & C4D_Power_Output) connect_okay = true;
			break;
		case C4D_Line_Source:
			if (tstruct->Def->LineConnect & C4D_Liquid_Output) connect_okay = true; break;
		case C4D_Line_Drain:
			if (tstruct->Def->LineConnect & C4D_Liquid_Input)  connect_okay = true; break;
		default: return false; // Undefined line type
		}
		if (!connect_okay)
		{
			StartSoundEffect("Error", false, 100, cObj);
			GameMsgObject(FormatString(LoadResStr("IDS_OBJ_NOCONNECTTYPE"), cline->GetName(), tstruct->GetName()).getData(), tstruct); return false;
		}

		// Connect line to structure
		StartSoundEffect("Connect", false, 100, cObj);
		if (cline->Action.Target == linekit) cline->Action.Target = tstruct;
		if (cline->Action.Target2 == linekit) cline->Action.Target2 = tstruct;
		linekit->Exit();
		linekit->AssignRemoval();

		GameMsgObject(FormatString(LoadResStr("IDS_OBJ_CONNECT"), cline->GetName(), tstruct->GetName()).getData(), tstruct);

		return true;
	}

	// New line

	// Check for new structure connection
	ocf = OCF_LineConstruct;
	tstruct = Game.Objects.AtObject(cObj->x, cObj->y, ocf, cObj);
	if (!tstruct || !(ocf & OCF_LineConstruct))
	{
		StartSoundEffect("Error", false, 100, cObj);
		GameMsgObject(LoadResStr("IDS_OBJ_NONEWLINE"), cObj); return false;
	}

	// Determine new line type
	C4ID linetype = C4ID_None;
	// Check source pipe
	if (linetype == C4ID_None)
		if (tstruct->Def->LineConnect & C4D_Liquid_Pump)
			if (!Game.FindObject(C4ID_SourcePipe, 0, 0, 0, 0, OCF_All, "Connect", tstruct))
				linetype = C4ID_SourcePipe;
	// Check drain pipe
	if (linetype == C4ID_None)
		if (tstruct->Def->LineConnect & C4D_Liquid_Output)
			if (!Game.FindObject(C4ID_DrainPipe, 0, 0, 0, 0, OCF_All, "Connect", tstruct))
				linetype = C4ID_DrainPipe;
	// Check power
	if (linetype == C4ID_None)
		if (tstruct->Def->LineConnect & C4D_Power_Output)
			linetype = C4ID_PowerLine;
	// No good
	if (linetype == C4ID_None)
	{
		StartSoundEffect("Error", false, 100, cObj);
		GameMsgObject(LoadResStr("IDS_OBJ_NONEWLINE"), cObj); return false;
	}

	// Create new line
	C4Object *newline = CreateLine(linetype, cObj->Owner,
		tstruct, linekit);
	if (!newline) return false;
	StartSoundEffect("Connect", false, 100, cObj);
	GameMsgObject(FormatString(LoadResStr("IDS_OBJ_NEWLINE"), newline->GetName()).getData(), tstruct);

	return true;
}

void ObjectComDigDouble(C4Object *cObj) // "Activation" by DFA_WALK, DFA_DIG, DFA_SWIM
{
	C4Object *pTarget;
	uint32_t ocf;
	C4PhysicalInfo *phys = cObj->GetPhysical();

	// Contents activation (first contents object only)
	if (cObj->Contents.GetObject())
		if (cObj->Contents.GetObject()->Call(PSF_Activate, {C4VObj(cObj)}))
			return;

	// Linekit: Line construction (move to linekit script...)
	if (cObj->Contents.GetObject() && (cObj->Contents.GetObject()->id == C4ID_Linekit))
	{
		ObjectComLineConstruction(cObj);
		return;
	}

	// Chop
	ocf = OCF_Chop;
	if (phys->CanChop)
		if (cObj->GetProcedure() != DFA_SWIM)
			if ((pTarget = Game.Objects.AtObject(cObj->x, cObj->y, ocf, cObj)))
				if (ocf & OCF_Chop)
				{
					PlayerObjectCommand(cObj->Owner, C4CMD_Chop, pTarget);
					return;
				}

	// Line construction pick up
	ocf = OCF_LineConstruct;
	if (phys->CanConstruct)
		if (!cObj->Contents.GetObject())
			if ((pTarget = Game.Objects.AtObject(cObj->x, cObj->y, ocf, cObj)))
				if (ocf & OCF_LineConstruct)
					if (ObjectComLineConstruction(cObj))
						return;

	// Own activation call
	if (cObj->Call(PSF_Activate, {C4VObj(cObj)})) return;
}

bool ObjectComDownDouble(C4Object *cObj) // by DFA_WALK
{
	C4Object *pTarget;
	uint32_t ocf = OCF_Construct | OCF_Grab;
	if ((pTarget = Game.Objects.AtObject(cObj->x, cObj->y, ocf, cObj)))
	{
		if (ocf & OCF_Construct)
		{
			PlayerObjectCommand(cObj->Owner, C4CMD_Build, pTarget); return true;
		}
		if (ocf & OCF_Grab)
		{
			PlayerObjectCommand(cObj->Owner, C4CMD_Grab, pTarget); return true;
		}
	}
	return false;
}

bool ObjectComPut(C4Object *cObj, C4Object *pTarget, C4Object *pThing)
{
	// No object specified, first from contents
	if (!pThing) pThing = cObj->Contents.GetObject();
	// Nothing to put
	if (!pThing) return false;
	// No target
	if (!pTarget) return false;
	// Grabbing: check C4D_Grab_Put
	if (pTarget != cObj->Contained)
		if (!(pTarget->Def->GrabPutGet & C4D_Grab_Put))
		{
			// Was meant to be a drop anyway
			if (ValidPlr(cObj->Owner))
				if (Game.Players.Get(cObj->Owner)->LastComDownDouble)
					return ObjectComDrop(cObj, pThing);
			// No grab put: fail
			return false;
		}
	// Target no fullcon
	if (!(pTarget->OCF & OCF_FullCon)) return false;
	// Check target collection limit
	if (pTarget->Def->CollectionLimit && (pTarget->Contents.ObjectCount() >= pTarget->Def->CollectionLimit)) return false;
	// Transfer thing
	bool fRejectCollect;
	if (!pThing->Enter(pTarget, true, true, &fRejectCollect)) return false;
	// Put call to object script
	cObj->Call(PSF_Put);
	// Target collection call
	pTarget->Call(PSF_Collection, {C4VObj(pThing), C4VBool(true)});
	// Success
	return true;
}

bool ObjectComThrow(C4Object *cObj, C4Object *pThing)
{
	// No object specified, first from contents
	if (!pThing) pThing = cObj->Contents.GetObject();
	// Nothing to throw
	if (!pThing) return false;
	// Throw com
	switch (cObj->GetProcedure())
	{
	case DFA_WALK: return ObjectActionThrow(cObj, pThing);
	}
	// Failure
	return false;
}

bool ObjectComDrop(C4Object *cObj, C4Object *pThing)
{
	// No object specified, first from contents
	if (!pThing) pThing = cObj->Contents.GetObject();
	// Nothing to throw
	if (!pThing) return false;
	// Force and direction
	// When dropping diagonally, drop from edge of shape
	// When doing a diagonal forward drop during flight, exit a bit closer to the Clonk to allow planned tumbling
	// Except when hangling, so you can mine effectively form the ceiling, and when swimming because you cannot tumble then
	C4Fixed pthrow = ValByPhysical(400, cObj->GetPhysical()->Throw);
	int32_t tdir = 0; int right = 0;
	bool isHanglingOrSwimming = false;
	int32_t iProc = DFA_NONE;
	if (cObj->Action.Act >= 0)
	{
		iProc = cObj->Def->ActMap[cObj->Action.Act].Procedure;
		if (iProc == DFA_HANGLE || iProc == DFA_SWIM) isHanglingOrSwimming = true;
	}
	int32_t iOutposReduction = 1; // don't exit object too far forward during jump
	if (iProc != DFA_SCALE) // never diagonal during scaling (can have com into wall during scaling!)
	{
		if (ComDirLike(cObj->Action.ComDir, COMD_Left))  { tdir = -1; right = 0; if (cObj->xdir < FIXED10(15)  && !isHanglingOrSwimming) --iOutposReduction; }
		if (ComDirLike(cObj->Action.ComDir, COMD_Right)) { tdir = +1; right = 1; if (cObj->xdir > FIXED10(-15) && !isHanglingOrSwimming) --iOutposReduction; }
	}
	// Exit object
	pThing->Exit(cObj->x + (cObj->Shape.x + cObj->Shape.Wdt * right) * !!tdir * iOutposReduction,
		cObj->y + cObj->Shape.y + cObj->Shape.Hgt - (pThing->Shape.y + pThing->Shape.Hgt), 0, pthrow * tdir, Fix0, Fix0);
	// NoCollectDelay
	cObj->NoCollectDelay = 2;
	// Update OCF
	cObj->SetOCF();
	// Ungrab
	ObjectComUnGrab(cObj);
	// Done
	return true;
}

bool ObjectComChop(C4Object *cObj, C4Object *pTarget)
{
	if (!pTarget) return false;
	if (!cObj->GetPhysical()->CanChop)
	{
		GameMsgObject(FormatString(LoadResStr("IDS_OBJ_NOCHOP"), cObj->GetName()).getData(), cObj);
		return false;
	}
	if (cObj->GetProcedure() != DFA_WALK) return false;
	return ObjectActionChop(cObj, pTarget);
}

bool ObjectComBuild(C4Object *cObj, C4Object *pTarget)
{
	if (!pTarget) return false;
	// Needs to be idle or walking
	if (cObj->Action.Act != ActIdle)
		if (cObj->GetProcedure() != DFA_WALK)
			return false;
	return ObjectActionBuild(cObj, pTarget);
}

bool ObjectComPutTake(C4Object *cObj, C4Object *pTarget, C4Object *pThing) // by C4CMD_Throw
{
	// by C4CMD_Drop
	// Valid checks
	if (!pTarget) return false;
	// No object specified, first from contents
	if (!pThing) pThing = cObj->Contents.GetObject();
	// Has thing, put to target
	if (pThing)
		return ObjectComPut(cObj, pTarget, pThing);
	// If target is own container, activate activation menu
	if (pTarget == cObj->Contained)
		return ObjectComTake(cObj); // carlo
	// Assuming target is grabbed, check for grab get
	if (pTarget->Def->GrabPutGet & C4D_Grab_Get)
	{
		// Activate get menu
		return cObj->ActivateMenu(C4MN_Get, 0, 0, 0, pTarget);
	}
	// Failure
	return false;
}

// carlo
bool ObjectComTake(C4Object *cObj) // by C4CMD_Take
{
	return cObj->ActivateMenu(C4MN_Activate);
}

// carlo
bool ObjectComTake2(C4Object *cObj) // by C4CMD_Take2
{
	return cObj->ActivateMenu(C4MN_Get, 0, 0, 0, cObj->Contained);
}

bool ObjectComPunch(C4Object *cObj, C4Object *pTarget, int32_t punch)
{
	if (!cObj || !pTarget) return false;
	if (!punch)
		if (pTarget->GetPhysical()->Fight)
			punch = BoundBy<int32_t>(5 * cObj->GetPhysical()->Fight / pTarget->GetPhysical()->Fight, 0, 10);
	if (!punch) return true;
	bool fBlowStopped = static_cast<bool>(pTarget->Call(PSF_QueryCatchBlow, {C4VObj(cObj)}));
	if (fBlowStopped && punch > 1) punch = punch / 2; // half damage for caught blow, so shield+armor help in fistfight and vs monsters
	pTarget->DoEnergy(-punch, false, C4FxCall_EngGetPunched, cObj->Controller);
	int32_t tdir = +1; if (cObj->Action.Dir == DIR_Left) tdir = -1;
	pTarget->Action.ComDir = COMD_Stop;
	// No tumbles when blow was caught
	if (fBlowStopped) return false;
	// Hard punch
	if (punch >= 10)
		if (ObjectActionTumble(pTarget, pTarget->Action.Dir, FIXED100(150) * tdir, itofix(-2)))
		{
			pTarget->LastEnergyLossCausePlayer = cObj->Controller; // for kill tracing when pushing enemies off a cliff
			pTarget->Call(PSF_CatchBlow, {C4VInt(punch), C4VObj(cObj)});
			return true;
		}

	// Regular punch
	if (ObjectActionGetPunched(pTarget, FIXED100(250) * tdir, Fix0))
	{
		pTarget->LastEnergyLossCausePlayer = cObj->Controller; // for kill tracing when pushing enemies off a cliff
		pTarget->Call(PSF_CatchBlow, {C4VInt(punch), C4VObj(cObj)});
		return true;
	}

	return false;
}

bool ObjectComCancelAttach(C4Object *cObj)
{
	if (cObj->GetProcedure() == DFA_ATTACH)
		return cObj->SetAction(ActIdle);
	return false;
}

void ObjectComStopDig(C4Object *cObj)
{
	// Stand
	ObjectActionStand(cObj);
	// Clear digging command
	if (cObj->Command)
		if (cObj->Command->Command == C4CMD_Dig)
			cObj->ClearCommand(cObj->Command);
}

int32_t ComOrder(int32_t iIndex)
{
	static uint8_t bComOrder[ComOrderNum] =
	{
		COM_Left,   COM_Right,   COM_Up,   COM_Down,   COM_Throw,   COM_Dig,   COM_Special,   COM_Special2,
		COM_Left_S, COM_Right_S, COM_Up_S, COM_Down_S, COM_Throw_S, COM_Dig_S, COM_Special_S, COM_Special2_S,
		COM_Left_D, COM_Right_D, COM_Up_D, COM_Down_D, COM_Throw_D, COM_Dig_D, COM_Special_D, COM_Special2_D
	};

	if (Inside<int32_t>(iIndex, 0, ComOrderNum - 1)) return bComOrder[iIndex];

	return COM_None;
}

const char *ComName(int32_t iCom)
{
	switch (iCom)
	{
	case COM_Up:         return "Up";
	case COM_Up_S:       return "UpSingle";
	case COM_Up_D:       return "UpDouble";
	case COM_Up_R:       return "UpReleased";
	case COM_Down:       return "Down";
	case COM_Down_S:     return "DownSingle";
	case COM_Down_D:     return "DownDouble";
	case COM_Down_R:     return "DownReleased";
	case COM_Left:       return "Left";
	case COM_Left_S:     return "LeftSingle";
	case COM_Left_D:     return "LeftDouble";
	case COM_Left_R:     return "LeftReleased";
	case COM_Right:      return "Right";
	case COM_Right_S:    return "RightSingle";
	case COM_Right_D:    return "RightDouble";
	case COM_Right_R:    return "RightReleased";
	case COM_Dig:        return "Dig";
	case COM_Dig_S:      return "DigSingle";
	case COM_Dig_D:      return "DigDouble";
	case COM_Dig_R:      return "DigReleased";
	case COM_Throw:      return "Throw";
	case COM_Throw_S:    return "ThrowSingle";
	case COM_Throw_D:    return "ThrowDouble";
	case COM_Throw_R:    return "ThrowReleased";
	case COM_Special:    return "Special";
	case COM_Special_S:  return "SpecialSingle";
	case COM_Special_D:  return "SpecialDouble";
	case COM_Special_R:  return "SpecialReleased";
	case COM_Special2:   return "Special2";
	case COM_Special2_S: return "Special2Single";
	case COM_Special2_D: return "Special2Double";
	case COM_Special2_R: return "Special2Released";
	case COM_WheelUp:    return "WheelUp";
	case COM_WheelDown:  return "WheelDown";
	case COM_CursorLeft:     return "CursorLeft";
	case COM_CursorLeft_S:   return "CursorLeftSingle";
	case COM_CursorLeft_D:   return "CursorLeftDouble";
	case COM_CursorLeft_R:   return "CursorLeftReleased";
	case COM_CursorToggle:   return "CursorToggle";
	case COM_CursorToggle_S: return "CursorToggleSingle";
	case COM_CursorToggle_D: return "CursorToggleDouble";
	case COM_CursorToggle_R: return "CursorToggleReleased";
	case COM_CursorRight:    return "CursorRight";
	case COM_CursorRight_S:  return "CursorRightSingle";
	case COM_CursorRight_D:  return "CursorRightDouble";
	case COM_CursorRight_R:  return "CursorRightReleased";
	}
	return "Undefined";
}

int32_t Com2Control(int32_t iCom)
{
	iCom = iCom & ~(COM_Double | COM_Single);
	switch (iCom)
	{
	case COM_CursorLeft:   return CON_CursorLeft;
	case COM_CursorToggle: return CON_CursorToggle;
	case COM_CursorRight:  return CON_CursorRight;
	case COM_Throw:        return CON_Throw;
	case COM_Up:           return CON_Up;
	case COM_Dig:          return CON_Dig;
	case COM_Left:         return CON_Left;
	case COM_Down:         return CON_Down;
	case COM_Right:        return CON_Right;
	case COM_Special:      return CON_Special;
	case COM_Special2:     return CON_Special2;
	}
	return CON_Menu;
}

int32_t Control2Com(int32_t iControl, bool fUp)
{
	static const char con2com[C4MaxKey] =
	{
		COM_CursorLeft, COM_CursorToggle, COM_CursorRight,
		COM_Throw,      COM_Up,           COM_Dig,
		COM_Left,       COM_Down,         COM_Right,
		COM_PlayerMenu, COM_Special,      COM_Special2
	};
	static const char con2com_r[C4MaxKey] =
	{
		COM_CursorLeft_R, COM_CursorToggle_R, COM_CursorRight_R,
		COM_Throw_R, COM_Up_R,      COM_Dig_R,
		COM_Left_R,  COM_Down_R,    COM_Right_R,
		COM_None,    COM_Special_R, COM_Special2_R
	};
	if (fUp)
	{
		if (Inside<int32_t>(iControl, 0, C4MaxKey - 1))
			return con2com_r[iControl];
	}
	else
	{
		if (Inside<int32_t>(iControl, 0, C4MaxKey - 1))
			return con2com[iControl];
	}
	return COM_None;
}

int32_t Coms2ComDir(int32_t iComs, bool fJumpIsUp)
{
	// This is possible because COM_Left - COM_Down are < 32
	static_assert((COM_Left | COM_Right | COM_Up | COM_Down) < 32);
	static int32_t DirComs = (1 << COM_Left) | (1 << COM_Right) | (1 << COM_Up) | (1 << COM_Down);
	// Allow swimming up with the jump key
	if (fJumpIsUp && (iComs & (1 << COM_Jump))) iComs |= (1 << COM_Up);
	switch (iComs & DirComs)
	{
	case (1 << COM_Up):                      return COMD_Up;
	case (1 << COM_Up)   | (1 << COM_Right): return COMD_UpRight;
	case (1 << COM_Right):                   return COMD_Right;
	case (1 << COM_Down) | (1 << COM_Right): return COMD_DownRight;
	case (1 << COM_Down):                    return COMD_Down;
	case (1 << COM_Down) | (1 << COM_Left):  return COMD_DownLeft;
	case (1 << COM_Left):                    return COMD_Left;
	case (1 << COM_Up)   | (1 << COM_Left):  return COMD_UpLeft;
	// up, right and left could be interpreted as COMD_Up etc., but that's too complicated for now
	default:                                 return COMD_Stop;
	}
}

bool ComDirLike(int32_t iComDir, int32_t iSample)
{
	if (iComDir == iSample) return true;
	if (iComDir % 8 + 1 == iSample) return true;
	if (iComDir == iSample % 8 + 1) return true;
	return false;
}

// Convert menu controls (not COMs) to the configured key name for a given player
static int32_t GetXInputControlForCommand(int32_t iPlayer, int32_t iCom)
{
	C4Player* pPlr = Game.Players.Get(iPlayer);
	if (!pPlr || !Inside(pPlr->Control, C4P_Control_GamePad1, C4P_Control_GamePadMax)) {
		return -1;
	}

	int32_t iGamepad = pPlr->Control - C4P_Control_GamePad1;
	auto& conf = Config.Gamepads[iGamepad];
	uint8_t buttonSearchOrder[]{
		// A command might have multiple buttons. Search A/B/X/Y buttons first, then in reverse order, so D-Pad is searched last
		XINPUT_BUTTON_A, XINPUT_BUTTON_B, XINPUT_BUTTON_X, XINPUT_BUTTON_Y,
		11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
	};
	for (const auto& iButton : buttonSearchOrder) {
		if (conf.ButtonCommand[iButton] == iCom) return iButton;
		if (conf.ButtonMenuCommand[iButton] == iCom) return iButton;
	}
	for (int iAxis = 0; iAxis < C4MaxGamePadAxis; iAxis++) {
		if (conf.AxisMinCommand[iAxis] == iCom) return C4MaxGamePadButtons + iAxis * 2 + 1;
		if (conf.AxisMaxCommand[iAxis] == iCom) return C4MaxGamePadButtons + iAxis * 2 + 0;
	}

	return -1;
}

static void DrawOffsetScaled(C4Facet &img, C4Facet& cgo, int32_t iPhaseX, int32_t iPhaseY, float dx, float dy, float scale) {
	C4DrawTransform t;
	t.SetMoveScale(cgo.X * (1 - scale) + dx * cgo.Wdt, cgo.Y * (1 - scale) + dy * cgo.Hgt, scale, scale);
	img.DrawT(cgo, true, iPhaseX, iPhaseY, &t);
}

void DrawCommandKey(C4Facet &cgo, int32_t iCom, int32_t iPlayer, bool fPressed, bool showText)
{
	bool isDouble = iCom & COM_Double;
	iCom = iCom & ~(COM_Double | COM_Single);
	int32_t iButton = GetXInputControlForCommand(iPlayer, iCom);
	// Convert menu commands to their respective base command key for displaying the respective symbol
	int32_t iBaseCom = C4Menu::ConvertMenuComToCom(iCom);
	if (iButton >= 0) {
		// For gamepad buttons, don't draw the "Key" icon and the "Control type" icon
		if (isDouble) {
			// Draw the icon twice, a bit shifted and overlapping
			DrawOffsetScaled(Game.GraphicsResource.fctXinputButtons, cgo, iButton % 4, iButton / 4, 0, 0, 0.75);
			DrawOffsetScaled(Game.GraphicsResource.fctXinputButtons, cgo, iButton % 4, iButton / 4, 0.1, 0.1, 0.9);
		}
		else {
			Game.GraphicsResource.fctXinputButtons.Draw(cgo, true, iButton % 4, iButton / 4);
		}
		if (iCom == COM_PlayerMenu) {
			DrawOffsetScaled(Game.GraphicsResource.fctOKCancel, cgo, 1, 1, 0.0, 0.5, 0.5);
		}
		else {
			// Show the action symbol on top of the button, in the bottom left corner. Really necessary?
			//DrawOffsetScaled(Game.GraphicsResource.fctCommand, cgo, Com2Control(iBaseCom), isDouble, -0.05, 0.45, 0.6); // This slightly protrudes, but it looks better
		}
		return;
	}

	// Draw key
	Game.GraphicsResource.fctKey.Draw(cgo, false, fPressed);
	// Draw control symbol
	if (iBaseCom == COM_PlayerMenu)
		Game.GraphicsResource.fctOKCancel.Draw(cgo, true, 1, 1);
	else
		Game.GraphicsResource.fctCommand.Draw(cgo, true, Com2Control(iBaseCom), isDouble);
	// Draw text
	if (Config.Graphics.ShowCommandKeys && showText) {
		// Use smaller font on smaller buttons
		CStdFont &rFont = (cgo.Hgt <= C4MN_SymbolSize) ? Game.GraphicsResource.FontTiny : Game.GraphicsResource.FontRegular;
		StdStrBuf text = PlrControlKeyName(iPlayer, Com2Control(iBaseCom), true);
		Application.DDraw->TextOut(text.getData(), rFont, 1.0, cgo.Surface, cgo.X + cgo.Wdt / 2, cgo.Y + cgo.Hgt - rFont.GetLineHeight() - 2, CStdDDraw::DEFAULT_MESSAGE_COLOR, ACenter);
	}
}

bool SellFromBase(int32_t iPlr, C4Object *pBaseObj, C4ID id, C4Object *pSellObj)
{
	C4Object *pThing;
	// Valid checks
	if (!ValidPlr(iPlr)) return false;
	if (!pBaseObj || !ValidPlr(pBaseObj->Base)) return false;
	if (~Game.C4S.Game.Realism.BaseFunctionality & BASEFUNC_Sell) return false;
	// Base owner eliminated
	if (Game.Players.Get(pBaseObj->Base)->Eliminated)
	{
		StartSoundEffect("Error", false, 100, pBaseObj);
		GameMsgPlayer(FormatString(LoadResStr("IDS_PLR_ELIMINATED"), Game.Players.Get(pBaseObj->Base)->GetName()).getData(), iPlr); return false;
	}
	// Base owner hostile
	if (Hostile(iPlr, pBaseObj->Base))
	{
		StartSoundEffect("Error", false, 100, pBaseObj);
		GameMsgPlayer(FormatString(LoadResStr("IDS_PLR_HOSTILE"), Game.Players.Get(pBaseObj->Base)->GetName()).getData(), iPlr); return false;
	}
	// check validity of sell object, if specified
	if (pThing = pSellObj)
		if (!pThing->Status || pThing->Contained != pBaseObj)
			pThing = nullptr;
	// Get object from home pBaseObj via selected id, if no or an anvalid thing has been specified
	if (!pThing)
		if (!(pThing = pBaseObj->Contents.Find(id))) return false;
	// check definition NoSell
	if (pThing->Def->NoSell) return false;
	// Sell object (pBaseObj owner gets the money)
	return Game.Players.Get(pBaseObj->Base)->Sell2Home(pThing);
}

bool Buy2Base(int32_t iPlr, C4Object *pBase, C4ID id, bool fShowErrors)
{
	C4Object *pThing;
	// Validity
	if (!ValidPlr(iPlr)) return false;
	if (!pBase || !ValidPlr(pBase->Base)) return false;
	if (~Game.C4S.Game.Realism.BaseFunctionality & BASEFUNC_Buy) return false;
	// Base owner hostile
	if (Hostile(iPlr, pBase->Base))
	{
		if (!fShowErrors) return false;
		StartSoundEffect("Error", false, 100, pBase);
		GameMsgPlayer(FormatString(LoadResStr("IDS_PLR_HOSTILE"), Game.Players.Get(pBase->Base)->GetName()).getData(), iPlr); return false;
	}
	// buy
	if (!(pThing = Game.Players.Get(pBase->Base)->Buy(id, fShowErrors, iPlr, pBase))) return false;
	// Object enter target object
	pThing->Enter(pBase);
	// Success
	return true;
}

bool PlayerObjectCommand(int32_t plr, int32_t cmdf, C4Object *pTarget, int32_t tx, int32_t ty)
{
	C4Player *pPlr = Game.Players.Get(plr);
	if (!pPlr) return false;
	int32_t iAddMode = C4P_Command_Set;
	// Adjust for old-style keyboard throw/drop control: add & in range
	if (cmdf == C4CMD_Throw || cmdf == C4CMD_Drop) iAddMode = C4P_Command_Add | C4P_Command_Range;
	if (cmdf == C4CMD_Throw)
	{
		bool fConvertToDrop = false;
		// Drop on down-down-throw (classic)
		if (pPlr->LastComDownDouble)
		{
			fConvertToDrop = true;
			// Dropping one object automatically reenables LastComDownDouble to
			// allow easy dropping of multiple objects. Also set LastCom for
			// script compatibility (custom scripted dropping) and to prevent
			// unwanted ThrowDouble for mass dropping
			pPlr->LastCom = COM_Down | COM_Double;
			pPlr->LastComDownDouble = C4DoubleClick;
		}
		// Jump'n'Run: Drop on combined Down/Left/Right+Throw
		if (pPlr->ControlStyle && (pPlr->PressedComs & (1 << COM_Down))) fConvertToDrop = true;
		if (fConvertToDrop) return pPlr->ObjectCommand(C4CMD_Drop, pTarget, tx, ty, nullptr, 0, iAddMode);
	}
	// Route to player
	return pPlr->ObjectCommand(cmdf, pTarget, tx, ty, nullptr, 0, iAddMode);
}
