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

/* Object motion, collision, friction */

#include <C4Include.h>
#include <C4Object.h>

#ifndef BIG_C4INCLUDE
#include <C4Physics.h>
#include <C4SolidMask.h>
#include <C4Wrappers.h>
#endif

/* Some physical constants */

const FIXED FRedirect = FIXED100(50);
const FIXED FFriction = FIXED100(30);
const FIXED FixFullCircle = itofix(360), FixHalfCircle = FixFullCircle / 2;
const FIXED FloatFriction = FIXED100(2);
const FIXED RotateAccel = FIXED100(20);
const FIXED FloatAccel = FIXED100(10);
const FIXED WalkAccel = FIXED100(50), SwimAccel = FIXED100(20);
const FIXED HitSpeed1 = FIXED100(150); // Hit Event
const FIXED HitSpeed2 = itofix(2); // Cross Check Hit
const FIXED HitSpeed3 = itofix(6); // Scale disable, kneel
const FIXED HitSpeed4 = itofix(8); // Flat

/* Some helper functions */

void RedirectForce(FIXED &from, FIXED &to, int32_t tdir)
{
	FIXED fred;
	fred = (std::min)(Abs(from), FRedirect);
	from -= fred * Sign(from);
	to += fred * tdir;
}

void ApplyFriction(FIXED &tval, int32_t percent)
{
	FIXED ffric = FFriction * percent / 100;
	if (tval > +ffric) { tval -= ffric; return; }
	if (tval < -ffric) { tval += ffric; return; }
	tval = 0;
}

// Compares all Shape.VtxContactCNAT[] CNAT flags to search flag.
// Returns true if CNAT match has been found.

bool ContactVtxCNAT(C4Object *cobj, uint8_t cnat_dir)
{
	int32_t cnt;
	bool fcontact = false;
	for (cnt = 0; cnt < cobj->Shape.VtxNum; cnt++)
		if (cobj->Shape.VtxContactCNAT[cnt] & cnat_dir)
			fcontact = true;
	return fcontact;
}

// Finds first vertex with contact flag set.
// Returns -1/0/+1 for relation on vertex to object center.

int32_t ContactVtxWeight(C4Object *cobj)
{
	int32_t cnt;
	for (cnt = 0; cnt < cobj->Shape.VtxNum; cnt++)
		if (cobj->Shape.VtxContactCNAT[cnt])
		{
			if (cobj->Shape.VtxX[cnt] < 0) return -1;
			if (cobj->Shape.VtxX[cnt] > 0) return +1;
		}
	return 0;
}

// ContactVtxFriction: Returns 0-100 friction value of first
//                     contacted vertex;

int32_t ContactVtxFriction(C4Object *cobj)
{
	int32_t cnt;
	for (cnt = 0; cnt < cobj->Shape.VtxNum; cnt++)
		if (cobj->Shape.VtxContactCNAT[cnt])
			return cobj->Shape.VtxFriction[cnt];
	return 0;
}

const char *CNATName(int32_t cnat)
{
	switch (cnat)
	{
	case CNAT_None:   return "None";
	case CNAT_Left:   return "Left";
	case CNAT_Right:  return "Right";
	case CNAT_Top:    return "Top";
	case CNAT_Bottom: return "Bottom";
	case CNAT_Center: return "Center";
	}
	return "Undefined";
}

bool C4Object::Contact(int32_t iCNAT)
{
	if (Def->ContactFunctionCalls)
	{
		sprintf(OSTR, PSF_Contact, CNATName(iCNAT));
		return !!Call(OSTR);
	}
	return false;
}

void C4Object::DoMotion(int32_t mx, int32_t my)
{
	if (pSolidMaskData) pSolidMaskData->Remove(true, true);
	x += mx; y += my;
	motion_x += mx; motion_y += my;
}

void C4Object::TargetBounds(int32_t &ctco, int32_t limit_low, int32_t limit_hi, int32_t cnat_low, int32_t cnat_hi)
{
	switch (ForceLimits(ctco, limit_low, limit_hi))
	{
	case -1:
		// stop
		if (cnat_low == CNAT_Left) xdir = 0; else ydir = 0;
		// do calls
		Contact(cnat_low);
		break;
	case +1:
		// stop
		if (cnat_hi == CNAT_Right) xdir = 0; else ydir = 0;
		// do calls
		Contact(cnat_hi);
		break;
	}
}

int32_t C4Object::ContactCheck(int32_t iAtX, int32_t iAtY)
{
	// Check shape contact at given position
	Shape.ContactCheck(iAtX, iAtY);

	// Store shape contact values in object t_contact
	t_contact = Shape.ContactCNAT;

	// Contact script call for the first contacted cnat
	if (Shape.ContactCNAT)
		for (int32_t ccnat = 0; ccnat < 4; ccnat++) // Left, right, top bottom
			if (Shape.ContactCNAT & (1 << ccnat))
				if (Contact(1 << ccnat))
					break; // Will stop on first positive return contact call!

	// Return shape contact count
	return Shape.ContactCount;
}

void C4Object::SideBounds(int32_t &ctcox)
{
	// layer bounds
	if (pLayer) if (pLayer->Def->BorderBound & C4D_Border_Layer)
		if (Action.Act <= ActIdle || Def->ActMap[Action.Act].Procedure != DFA_ATTACH)
			if (Category & C4D_StaticBack)
				TargetBounds(ctcox, pLayer->x + pLayer->Shape.x, pLayer->x + pLayer->Shape.x + pLayer->Shape.Wdt, CNAT_Left, CNAT_Right);
			else
				TargetBounds(ctcox, pLayer->x + pLayer->Shape.x - Shape.x, pLayer->x + pLayer->Shape.x + pLayer->Shape.Wdt + Shape.x, CNAT_Left, CNAT_Right);
	// landscape bounds
	if (Def->BorderBound & C4D_Border_Sides)
		TargetBounds(ctcox, 0 - Shape.x, GBackWdt + Shape.x, CNAT_Left, CNAT_Right);
}

void C4Object::VerticalBounds(int32_t &ctcoy)
{
	// layer bounds
	if (pLayer) if (pLayer->Def->BorderBound & C4D_Border_Layer)
		if (Action.Act <= ActIdle || Def->ActMap[Action.Act].Procedure != DFA_ATTACH)
			if (Category & C4D_StaticBack)
				TargetBounds(ctcoy, pLayer->y + pLayer->Shape.y, pLayer->y + pLayer->Shape.y + pLayer->Shape.Hgt, CNAT_Top, CNAT_Bottom);
			else
				TargetBounds(ctcoy, pLayer->y + pLayer->Shape.y - Shape.y, pLayer->y + pLayer->Shape.y + pLayer->Shape.Hgt + Shape.y, CNAT_Top, CNAT_Bottom);
	// landscape bounds
	if (Def->BorderBound & C4D_Border_Top)
		TargetBounds(ctcoy, 0 - Shape.y, +1000000, CNAT_Top, CNAT_Bottom);
	if (Def->BorderBound & C4D_Border_Bottom)
		TargetBounds(ctcoy, -1000000, GBackHgt + Shape.y, CNAT_Top, CNAT_Bottom);
}

void C4Object::DoMovement()
{
	int32_t ctcox, ctcoy, ctcor, ctx, cty, iContact = 0;
	bool fAnyContact = false; uint32_t iContacts = 0;
	bool fTurned = false, fRedirectYR = false, fNoAttach = false;

	// Reset motion for this frame
	motion_x = motion_y = 0;

	// Restrictions
	if (Def->NoHorizontalMove) xdir = 0;

	// Dig free target area
	if (Action.Act > ActIdle)
		if (Def->ActMap[Action.Act].DigFree)
		{
			// Shape size square
			if (Def->ActMap[Action.Act].DigFree == 1)
			{
				ctcox = fixtoi(fix_x + xdir); ctcoy = fixtoi(fix_y + ydir);
				Game.Landscape.DigFreeRect(ctcox + Shape.x, ctcoy + Shape.y, Shape.Wdt, Shape.Hgt, Action.Data, this);
			}
			// Free size round (variable size)
			else
			{
				ctcox = fixtoi(fix_x + xdir); ctcoy = fixtoi(fix_y + ydir);
				int32_t rad = Def->ActMap[Action.Act].DigFree;
				if (Con < FullCon) rad = rad * 6 * Con / 5 / FullCon;
				Game.Landscape.DigFree(ctcox, ctcoy - 1, rad, Action.Data, this);
			}
		}

	// store previous position
	int32_t ix0 = x; int32_t iy0 = y;

	// store previous movement and ocf
	FIXED oldxdir(xdir), oldydir(ydir);
	uint32_t old_ocf = OCF;

	if (!Action.t_attach) // Unattached movement

	{
		// Horizontal movement

		// Movement target
		fix_x += xdir; ctcox = fixtoi(fix_x);

		// Movement bounds (horiz)
		SideBounds(ctcox);

		// Move to target
		while (x != ctcox)
		{
			// Next step
			ctx = x + Sign(ctcox - x);

			if (iContact = ContactCheck(ctx, y))
			{
				fAnyContact = true; iContacts |= t_contact;
				// Abort horizontal movement
				ctcox = x; fix_x = itofix(x);
				// Vertical redirection (always)
				RedirectForce(xdir, ydir, -1);
				ApplyFriction(ydir, ContactVtxFriction(this));
			}
			else // Free horizontal movement
				DoMotion(ctx - x, 0);
		}

		// Vertical movement

		// Movement target
		fix_y += ydir; ctcoy = fixtoi(fix_y);

		// Movement bounds (vertical)
		VerticalBounds(ctcoy);

		// Move to target
		while (y != ctcoy)
		{
			// Next step
			cty = y + Sign(ctcoy - y);
			if (iContact = ContactCheck(x, cty))
			{
				fAnyContact = true; iContacts |= t_contact;
				ctcoy = y; fix_y = itofix(y);
				// Vertical contact horizontal friction
				ApplyFriction(xdir, ContactVtxFriction(this));
				// Redirection slide or rotate
				if (!ContactVtxCNAT(this, CNAT_Left))
					RedirectForce(ydir, xdir, -1);
				else if (!ContactVtxCNAT(this, CNAT_Right))
					RedirectForce(ydir, xdir, +1);
				else
				{
					// living things are always capable of keeping their rotation
					if (OCF & OCF_Rotate) if (iContact == 1) if (!Alive)
					{
						RedirectForce(ydir, rdir, -ContactVtxWeight(this));
						fRedirectYR = true;
					}
					ydir = 0;
				}
			}
			else // Free vertical movement
				DoMotion(0, cty - y);
		}
	}

	if (Action.t_attach) // Attached movement
	{
		uint8_t at_xovr, at_yovr;

		// Set target position by momentum
		fix_x += xdir; fix_y += ydir;

		// Movement target
		ctcox = fixtoi(fix_x); ctcoy = fixtoi(fix_y);

		// Movement bounds (horiz + verti)
		SideBounds(ctcox); VerticalBounds(ctcoy);

		// Move to target
		do
		{
			at_xovr = at_yovr = 0; // ctco Attachment override flags

			// Set next step target
			ctx = x + Sign(ctcox - x); cty = y + Sign(ctcoy - y);

			// Attachment check
			if (!Shape.Attach(ctx, cty, Action.t_attach))
				fNoAttach = true;
			else
			{
				// Attachment change to ctx/cty overrides ctco target
				if (cty != y + Sign(ctcoy - y)) at_yovr = 1;
				if (ctx != x + Sign(ctcox - x)) at_xovr = 1;
			}

			// Contact check & evaluation
			if (iContact = ContactCheck(ctx, cty))
			{
				fAnyContact = true; iContacts |= t_contact;
				// Abort movement
				ctcox = x; fix_x = itofix(x);
				ctcoy = y; fix_y = itofix(y);
			}
			else // Continue free movement
				DoMotion(ctx - x, cty - y);

			// ctco Attachment overrides
			if (at_xovr) { ctcox = x; xdir = Fix0; fix_x = itofix(x); }
			if (at_yovr) { ctcoy = y; ydir = Fix0; fix_y = itofix(y); }
		} while ((x != ctcox) || (y != ctcoy));
	}

	// Rotation
	if (OCF & OCF_Rotate && !!rdir)
	{
		// Set target
		fix_r += rdir * 5;
		// Rotation limit
		if (Def->Rotateable > 1)
		{
			if (fix_r > itofix(Def->Rotateable))
			{
				fix_r = itofix(Def->Rotateable); rdir = 0;
			}
			if (fix_r < itofix(-Def->Rotateable))
			{
				fix_r = itofix(-Def->Rotateable); rdir = 0;
			}
		}
		ctcor = fixtoi(fix_r);
		ctx = x; cty = y;
		// Move to target
		while (r != ctcor)
		{
			// Save step undos
			int32_t lcobjr = r; C4Shape lshape = Shape;
			// Try next step
			r += Sign(ctcor - r);
			UpdateShape();
			// attached rotation: rotate around attachment pos
			if (Action.t_attach && !fNoAttach)
			{
				// more accurately, attachment should be evaluated by a rotation around the attachment vertex
				// however, as long as this code is only used for some surfaces adjustment for large vehicles,
				// it's enough to assume rotation around the center
				ctx = x; cty = y;
				// evaulate attachment, but do not bother about attachment loss
				// that will then be done in next execution cycle
				Shape.Attach(ctx, cty, Action.t_attach);
			}
			// check for contact
			if (iContact = ContactCheck(ctx, cty)) // Contact
			{
				fAnyContact = true; iContacts |= t_contact;
				// Undo step and abort movement
				Shape = lshape;
				r = lcobjr;
				ctcor = r;
				fix_r = itofix(r);
				// last UpdateShape-call might have changed sector lists!
				UpdatePos();
				// Redirect to y
				if (iContact == 1) if (!fRedirectYR)
					RedirectForce(rdir, ydir, -1);
				// Stop rotation
				rdir = 0;
			}
			else
			{
				fTurned = true;
				x = ctx; y = cty;
			}
		}
		// Circle bounds
		if (fix_r < -FixHalfCircle) { fix_r += FixFullCircle; r = fixtoi(fix_r); }
		if (fix_r > +FixHalfCircle) { fix_r -= FixFullCircle; r = fixtoi(fix_r); }
	}

	// Reput solid mask: Might have been removed by motion or
	// motion might be out of date from last frame.
	UpdateSolidMask(true);

	// Misc checks

	// InLiquid check
	// this equals C4Object::UpdateLiquid, but the "fNoAttach=false;"-line
	if (IsInLiquidCheck()) // In Liquid
	{
		if (!InLiquid) // Enter liquid
		{
			if (OCF & OCF_HitSpeed2) if (Mass > 3)
				Splash(x, y + 1, (std::min)(Shape.Wdt * Shape.Hgt / 10, 20), this);
			fNoAttach = false;
			InLiquid = 1;
		}
	}
	else // Out of liquid
	{
		if (InLiquid) // Leave liquid
			InLiquid = 0;
	}

	// Contact Action
	if (fAnyContact)
	{
		t_contact = iContacts;
		ContactAction();
	}
	// Attachment Loss Action
	if (fNoAttach)
		NoAttachAction();
	// Movement Script Execution
	if (fAnyContact)
	{
		C4AulParSet pars(C4VInt(fixtoi(oldxdir, 100)), C4VInt(fixtoi(oldydir, 100)));
		if (old_ocf & OCF_HitSpeed1) Call(PSF_Hit,  &pars);
		if (old_ocf & OCF_HitSpeed2) Call(PSF_Hit2, &pars);
		if (old_ocf & OCF_HitSpeed3) Call(PSF_Hit3, &pars);
	}

	// Rotation gfx
	if (fTurned)
		UpdateFace(true);
	else
		// pos changed?
		if ((ix0 - x) | (iy0 - y)) UpdatePos();
}

void C4Object::Stabilize()
{
	// def allows stabilization?
	if (Def->NoStabilize) return;
	// normalize angle
	int32_t nr = r; while (nr < -180) nr += 360; while (nr > 180) nr -= 360;
	if (nr != 0)
		if (Inside<int32_t>(nr, -StableRange, +StableRange))
		{
			// Save step undos
			int32_t lcobjr = r;
			C4Shape lshape = Shape;
			// Try rotation
			r = 0;
			UpdateShape();
			if (ContactCheck(x, y))
			{
				// Undo rotation
				Shape = lshape;
				r = lcobjr;
			}
			else
			{
				// Stabilization okay
				fix_r = itofix(r);
				UpdateFace(true);
			}
		}
}

void C4Object::CopyMotion(C4Object *from)
{
	// Designed for contained objects, no static
	if (x != from->x || y != from->y)
	{
		x = from->x; y = from->y;
		// Resort into sectors
		UpdatePos();
	}
	fix_x = itofix(x); fix_y = itofix(y);
	xdir = from->xdir; ydir = from->ydir;
}

void C4Object::ForcePosition(int32_t tx, int32_t ty)
{
	// always reset fixed
	fix_x = itofix(tx); fix_y = itofix(ty);
	// no movement
	if (!((x - tx) | (y - ty))) return;
	x = tx; y = ty;
	UpdatePos();
	UpdateSolidMask(false);
}

void C4Object::MovePosition(int32_t dx, int32_t dy)
{
	// move object position; repositions SolidMask
	if (pSolidMaskData) pSolidMaskData->Remove(true, true);
	x += dx; y += dy;
	fix_x += dx;
	fix_y += dy;
	UpdatePos();
	UpdateSolidMask(true);
}

bool C4Object::ExecMovement() // Every Tick1 by Execute
{
	// Containment check
	if (Contained)
	{
		CopyMotion(Contained);

		return true;
	}

	// General mobility check
	if (Category & C4D_StaticBack) return false;

	// Movement execution
	if (Mobile) // Object is moving
	{
		// Move object
		DoMovement();
		// Demobilization check
		if ((xdir == 0) && (ydir == 0) && (rdir == 0)) Mobile = 0;
		// Check for stabilization
		if (rdir == 0) Stabilize();
	}
	else // Object is static
	{
		// Check for stabilization
		Stabilize();
		// Check for mobilization
		if (!Tick10)
		{
			// Gravity mobilization
			xdir = ydir = rdir = 0;
			fix_x = itofix(x); fix_y = itofix(y); fix_r = itofix(r);
			Mobile = 1;
		}
	}

	// Enforce zero rotation
	if (!Def->Rotateable) r = 0;

	// Out of bounds check
	if ((!Inside<int32_t>(x, 0, GBackWdt) && !(Def->BorderBound & C4D_Border_Sides)) || (y > GBackHgt && !(Def->BorderBound & C4D_Border_Bottom)))
		// Never remove attached objects: If they are truly outside landscape, their target will be removed,
		// and the attached objects follow one frame later
		if (Action.Act < 0 || !Action.Target || Def->ActMap[Action.Act].Procedure != DFA_ATTACH)
		{
			bool fRemove = true;
			// never remove HUD objects
			if (Category & C4D_Parallax)
			{
				fRemove = false;
				if (x > GBackWdt || y > GBackHgt) fRemove = true; // except if they are really out of the viewport to the right...
				else if (x < 0 && Local[0].Data) fRemove = true; // ...or it's not HUD horizontally and it's out to the left
				else if (!Local[0].Data && x < -GBackWdt) fRemove = true; // ...or it's HUD horizontally and it's out to the left
			}
			if (fRemove)
			{
				AssignDeath(true);
				AssignRemoval();
			}
		}

	return true;
}

bool SimFlight(FIXED &x, FIXED &y, FIXED &xdir, FIXED &ydir, int32_t iDensityMin, int32_t iDensityMax, int32_t iIter)
{
	bool fBreak = false;
	int32_t ctcox, ctcoy, cx, cy;
	cx = fixtoi(x); cy = fixtoi(y);
	do
	{
		if (!iIter--) return false;
		// Set target position by momentum
		x += xdir; y += ydir;
		// Movement to target
		ctcox = fixtoi(x); ctcoy = fixtoi(y);
		// Bounds
		if (!Inside<int32_t>(ctcox, 0, GBackWdt) || (ctcoy >= GBackHgt)) return false;
		// Move to target
		do
		{
			// Set next step target
			cx += Sign(ctcox - cx); cy += Sign(ctcoy - cy);
			// Contact check
			if (Inside(GBackDensity(cx, cy), iDensityMin, iDensityMax))
			{
				fBreak = true; break;
			}
		} while ((cx != ctcox) || (cy != ctcoy));
		// Adjust GravAccel once per frame
		ydir += GravAccel;
	} while (!fBreak);
	// write position back
	x = itofix(cx); y = itofix(cy);
	// ok
	return true;
}

bool SimFlightHitsLiquid(FIXED fcx, FIXED fcy, FIXED xdir, FIXED ydir)
{
	// Start in water?
	if (DensityLiquid(GBackDensity(fixtoi(fcx), fixtoi(fcy))))
		if (!SimFlight(fcx, fcy, xdir, ydir, 0, C4M_Liquid - 1, 10))
			return false;
	// Hits liquid?
	if (!SimFlight(fcx, fcy, xdir, ydir, C4M_Liquid, 100, -1))
		return false;
	// liquid & deep enough?
	return GBackLiquid(fixtoi(fcx), fixtoi(fcy)) && GBackLiquid(fixtoi(fcx), fixtoi(fcy) + 9);
}
