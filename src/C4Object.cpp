/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2022, The LegacyClonk Team and contributors
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

/* That which fills the world with life */

#include "C4Application.h"
#include "C4Game.h"
#include <C4Include.h>
#include <C4Object.h>
#include <C4Version.h>

#include <C4ObjectInfo.h>
#include <C4Physics.h>
#include <C4ObjectCom.h>
#include <C4Command.h>
#include <C4Viewport.h>
#ifdef DEBUGREC
#include <C4Record.h>
#endif
#include <C4SolidMask.h>
#include <C4Random.h>
#include <C4Wrappers.h>
#include <C4Player.h>
#include <C4ObjectMenu.h>

#include <cstring>
#include <format>
#include <limits>
#include <utility>

void DrawVertex(C4Facet &cgo, int32_t tx, int32_t ty, int32_t col, int32_t contact)
{
	if (Inside<int32_t>(tx, 1, cgo.Wdt - 2) && Inside<int32_t>(ty, 1, cgo.Hgt - 2))
	{
		Application.DDraw->DrawHorizontalLine(cgo.Surface, cgo.X + tx - 1, cgo.X + tx + 1, cgo.Y + ty, col);
		Application.DDraw->DrawVerticalLine(cgo.Surface, cgo.X + tx, cgo.Y + ty - 1, cgo.Y + ty + 1, col);
		if (contact) Application.DDraw->DrawFrame(cgo.Surface, cgo.X + tx - 2, cgo.Y + ty - 2, cgo.X + tx + 2, cgo.Y + ty + 2, CWhite);
	}
}

void C4Action::SetBridgeData(int32_t iBridgeTime, bool fMoveClonk, bool fWall, int32_t iBridgeMaterial)
{
	// validity
	iBridgeMaterial = iBridgeMaterial;
	if (iBridgeMaterial < 0) iBridgeMaterial = 0xff;
	iBridgeTime = BoundBy<int32_t>(iBridgeTime, 0, 0xffff);
	// mask in this->Data
	Data = (uint32_t(iBridgeTime) << 16) + (uint32_t(fMoveClonk) << 8) + (uint32_t(fWall) << 9) + iBridgeMaterial;
}

void C4Action::GetBridgeData(int32_t &riBridgeTime, bool &rfMoveClonk, bool &rfWall, int32_t &riBridgeMaterial)
{
	// mask from this->Data
	uint32_t uiData = Data;
	riBridgeTime = uiData >> 16;
	rfMoveClonk = !!(uiData & 0x100);
	rfWall = !!(uiData & 0x200);
	riBridgeMaterial = (uiData & 0xff);
	if (riBridgeMaterial == 0xff) riBridgeMaterial = -1;
}

C4Object::C4Object()
{
	Default();
}

void C4Object::Default()
{
	id = C4ID_None;
	Number = -1;
	Status = 1;
	nInfo.Clear();
	RemovalDelay = 0;
	Owner = NO_OWNER;
	Controller = NO_OWNER;
	LastEnergyLossCausePlayer = NO_OWNER;
	Category = 0;
	x = y = r = 0;
	motion_x = motion_y = 0;
	NoCollectDelay = 0;
	Base = NO_OWNER;
	Con = 0;
	Mass = OwnMass = 0;
	Damage = 0;
	Energy = 0;
	MagicEnergy = 0;
	Alive = 0;
	Breath = 0;
	FirePhase = 0;
	InMat = MNone;
	Color = 0;
	ViewEnergy = 0;
	Local.Reset();
	PlrViewRange = 0;
	fix_x = fix_y = fix_r = 0;
	xdir = ydir = rdir = 0;
	Mobile = 0;
	Select = 0;
	Unsorted = false;
	Initializing = false;
	OnFire = 0;
	InLiquid = 0;
	EntranceStatus = 0;
	Audible = -1;
	NeedEnergy = 0;
	Timer = 0;
	t_contact = 0;
	OCF = 0;
	Action.Default();
	Shape.Default();
	fOwnVertices = 0;
	Contents.Default();
	Component.Clear();
	SolidMask.Default();
	PictureRect.Default();
	Def = nullptr;
	Info = nullptr;
	Command = nullptr;
	Contained = nullptr;
	TopFace.Default();
	Menu = nullptr;
	PhysicalTemporary = false;
	TemporaryPhysical.Default();
	MaterialContents.fill(0);
	Visibility = VIS_All;
	LocalNamed.Reset();
	Marker = 0;
	ColorMod = BlitMode = 0;
	CrewDisabled = false;
	pLayer = nullptr;
	pSolidMaskData = nullptr;
	pGraphics = nullptr;
	pDrawTransform = nullptr;
	pEffects = nullptr;
	FirstRef = nullptr;
	pGfxOverlay = nullptr;
	iLastAttachMovementFrame = -1;
}

bool C4Object::Init(C4Def *pDef, C4Section &section, C4Object *pCreator,
	int32_t iOwner, C4ObjectInfo *pInfo,
	int32_t nx, int32_t ny, int32_t nr,
	C4Fixed nxdir, C4Fixed nydir, C4Fixed nrdir, int32_t iController)
{
	// currently initializing
	Initializing = true;

	// Def & basics
	id = pDef->id;
	Owner = iOwner;
	if (iController > NO_OWNER) Controller = iController; else Controller = iOwner;
	LastEnergyLossCausePlayer = NO_OWNER;
	Info = pInfo;
	Def = pDef;
	Category = Def->Category;
	Def->Count++;
	Section = &section;
	if (pCreator)
	{
		pLayer = pCreator->pLayer;
	}

	// graphics
	pGraphics = &Def->Graphics;
	BlitMode = Def->BlitMode;

	// Position
	if (!Def->Rotateable) { nr = 0; nrdir = 0; }
	x = nx; y = ny; r = nr;
	fix_x = itofix(x);
	fix_y = itofix(y);
	fix_r = itofix(r);
	xdir = nxdir; ydir = nydir; rdir = nrdir;

	// Initial mobility
	if (Category != C4D_StaticBack)
		if (!!xdir || !!ydir || !!rdir)
			Mobile = 1;

	// Mass
	Mass = std::max<int32_t>(Def->Mass * Con / FullCon, 1);

	// Life, energy, breath
	if (Category & C4D_Living) Alive = 1;
	if (Alive) Energy = GetPhysical()->Energy;
	Breath = GetPhysical()->Breath;

	// Components
	Component = Def->Component;
	ComponentConCutoff();

	// Color
	if (Def->ColorByOwner)
		if (ValidPlr(Owner))
			Color = Game.Players.Get(Owner)->ColorDw;

	// Shape & face
	Shape = Def->Shape;
	SolidMask = Def->SolidMask;
	CheckSolidMaskRect();
	UpdateGraphics(false);
	UpdateFace(true);

	// Initial audibility
	Audible = -1;

	// Initial OCF
	SetOCF();

	// local named vars
	LocalNamed.SetNameList(&pDef->Script.LocalNamed);

	// finished initializing
	Initializing = false;

	return true;
}

C4Object::~C4Object()
{
	Clear();

#ifndef NDEBUG
	// debug: mustn't be listed in any list now

	if (Section)
	{
		assert(!Section->Objects.ObjectNumber(this));
		assert(!Section->Objects.InactiveObjects.ObjectNumber(this));
		Section->Objects.Sectors.AssertObjectNotInList(this);
	}
#endif
}

void C4Object::AssignRemoval(bool fExitContents)
{
	// check status
	if (!Status) return;
#ifdef DEBUGREC
	C4RCCreateObj rc;
	rc.id = Def->id;
	rc.oei = Number;
	rc.x = x; rc.y = y; rc.ownr = Owner;
	AddDbgRec(RCT_DsObj, &rc, sizeof(rc));
#endif
	// Destruction call in container
	if (Contained)
	{
		Contained->Call(PSF_ContentsDestruction, {C4VObj(this)});
		if (!Status) return;
	}
	// Destruction call
	Call(PSF_Destruction);
	// Destruction-callback might have deleted the object already
	if (!Status) return;
	// remove all effects (extinguishes as well)
	if (pEffects)
	{
		pEffects->ClearAll(this, C4FxCall_RemoveClear);
		// Effect-callback might actually have deleted the object already
		if (!Status) return;
	}
	// ...or just deleted the effects
	delete pEffects;
	pEffects = nullptr;
	// remove particles
	if (FrontParticles) FrontParticles.Clear(Section->Particles.FreeParticles);
	if (BackParticles) BackParticles.Clear(Section->Particles.FreeParticles);
	// Action idle
	SetAction(ActIdle);
	// Object system operation
	if (Status == C4OS_INACTIVE)
	{
		// object was inactive: activate first, then delete
		Section->Objects.InactiveObjects.Remove(this);
		Status = C4OS_NORMAL;
		Section->Objects.Add(this);
	}
	Status = 0;
	// count decrease
	Def->Count--;
	// Kill contents
	C4Object *cobj; C4ObjectLink *clnk;
	while ((clnk = Contents.First) && (cobj = clnk->Obj))
	{
		if (fExitContents)
			cobj->Exit(x, y);
		else
		{
			Contents.Remove(cobj);
			cobj->AssignRemoval();
		}
	}
	// remove from container *after* contents have been removed!
	C4Object *pCont;
	if (pCont = Contained)
	{
		pCont->Contents.Remove(this);
		pCont->UpdateMass();
		pCont->SetOCF();
		Contained = nullptr;
	}
	// Object info
	if (Info) Info->Retire();
	Info = nullptr;
	// Object system operation
	while (FirstRef) FirstRef->Set0();
	Game.ClearPointers(this);
	ClearCommands();
	if (pSolidMaskData) pSolidMaskData->Remove(true, false);
	delete pSolidMaskData;
	pSolidMaskData = nullptr;
	SolidMask.Wdt = 0;
	RemovalDelay = 2;
}

void C4Object::UpdateShape(bool bUpdateVertices)
{
	// Line shape independent
	if (Def->Line) return;

	// Copy shape from def
	Shape.CopyFrom(Def->Shape, bUpdateVertices, !!fOwnVertices);

	// Construction zoom
	if (Con != FullCon)
		if (Def->GrowthType)
			Shape.Stretch(Con * 100 / FullCon, bUpdateVertices);
		else
			Shape.Jolt(Con * 100 / FullCon, bUpdateVertices);

	// Rotation
	if (Def->Rotateable)
		if (r != 0)
			Shape.Rotate(r, bUpdateVertices);

	// covered area changed? to be on the save side, update pos
	UpdatePos();
}

void C4Object::UpdatePos()
{
	// get new area covered
	// do *NOT* do this while initializing, because object cannot be sorted by main list
	if (!Initializing && Status == C4OS_NORMAL)
	{
		Section->Objects.UpdatePos(this);
		Audible = -1; // outdated, needs to be recalculated if needed
	}
}

void C4Object::UpdateFace(bool bUpdateShape, bool fTemp)
{
	// Update shape - NOT for temp call, because temnp calls are done in drawing routine
	// must not change sync relevant data here (although the shape and pos *should* be updated at that time anyway,
	// because a runtime join would desync otherwise)
	if (!fTemp) { if (bUpdateShape) UpdateShape(); else UpdatePos(); }

	// SolidMask
	if (!fTemp) UpdateSolidMask(false);

	// Null defaults
	TopFace.Default();

	// newgfx: TopFace only
	if (Con >= FullCon || Def->GrowthType)
		if (!Def->Rotateable || (r == 0))
			if (Def->TopFace.Wdt > 0) // Fullcon & no rotation
				TopFace.Set(GetGraphics()->GetBitmap(Color),
					Def->TopFace.x, Def->TopFace.y,
					Def->TopFace.Wdt, Def->TopFace.Hgt);

	// Active face
	UpdateActionFace();
}

void C4Object::UpdateGraphics(bool fGraphicsChanged, bool fTemp)
{
	// check color
	if (!fTemp) if (!pGraphics->IsColorByOwner()) Color = 0;
	// new grafics: update face + solidmask
	if (fGraphicsChanged)
	{
		// update solid
		if (pSolidMaskData && !fTemp)
		{
			// remove if put
			pSolidMaskData->Remove(true, false);
			// delete
			delete pSolidMaskData; pSolidMaskData = nullptr;
			// ensure SolidMask-rect lies within new graphics-rect
			CheckSolidMaskRect();
		}
		// update face - this also puts any SolidMask
		UpdateFace(false);
	}
}

void C4Object::UpdateFlipDir()
{
	int32_t iFlipDir;
	// We're active
	if (Action.Act > ActIdle)
		// Get flipdir value from action
		if (iFlipDir = Def->ActMap[Action.Act].FlipDir)
			// Action dir is in flipdir range
			if (Action.Dir >= iFlipDir)
			{
				// Calculate flipped drawing dir (from the flipdir direction going backwards)
				Action.DrawDir = (iFlipDir - 1 - (Action.Dir - iFlipDir));
				// Set draw transform, creating one if necessary
				if (pDrawTransform)
					pDrawTransform->SetFlipDir(-1);
				else
					pDrawTransform = new C4DrawTransform(-1);
				// Done setting flipdir
				return;
			}
	// No flipdir necessary
	Action.DrawDir = Action.Dir;
	// Draw transform present?
	if (pDrawTransform)
	{
		// reset flip dir
		pDrawTransform->SetFlipDir(1);
		// if it's identity now, remove the matrix
		if (pDrawTransform->IsIdentity())
		{
			delete pDrawTransform;
			pDrawTransform = nullptr;
		}
	}
}

void C4Object::DrawFace(C4FacetEx &cgo, int32_t cgoX, int32_t cgoY, int32_t iPhaseX, int32_t iPhaseY)
{
	const int swdt = Def->Shape.Wdt;
	const int shgt = Def->Shape.Hgt;
	// Grow Type Display
	float fx = static_cast<float>(swdt * iPhaseX);
	float fy = static_cast<float>(shgt * iPhaseY);
	float fwdt = static_cast<float>(swdt);
	float fhgt = static_cast<float>(shgt);

	float tx = static_cast<float>(cgoX + (Shape.Wdt - swdt * Con / FullCon) / 2);
	float ty = static_cast<float>(cgoY + (Shape.Hgt - shgt * Con / FullCon) / 2);
	float twdt = static_cast<float>(swdt * Con / FullCon);
	float thgt = static_cast<float>(shgt * Con / FullCon);

	// Construction Type Display
	if (!Def->GrowthType)
	{
		tx = static_cast<float>(cgoX) + (Shape.Wdt - swdt) / 2.0f;
		twdt = static_cast<float>(swdt);
		fy += static_cast<float>(shgt * std::max<int32_t>(FullCon - Con, 0) / FullCon);
		fhgt = static_cast<float>(std::min<int32_t>(shgt * Con / FullCon, shgt));
	}

	const float scale{GetGraphics()->pDef->Scale};

	fx *= scale;
	fy *= scale;
	fwdt *= scale;
	fhgt *= scale;

	// Straight
	if ((!Def->Rotateable || (r == 0)) && !pDrawTransform)
	{
		lpDDraw->Blit(GetGraphics()->GetBitmap(Color),
			fx, fy, fwdt, fhgt,
			cgo.Surface, tx, ty, twdt, thgt,
			true, nullptr);
	}
	// Rotated or transformed
	else
	{
		C4DrawTransform rot;
		if (pDrawTransform)
		{
			rot.SetTransformAt(*pDrawTransform, cgoX + Shape.Wdt / 2.0f, cgoY + Shape.Hgt / 2.0f);
			if (r) rot.Rotate(r * 100, cgoX + Shape.Wdt / 2.0f, cgoY + Shape.Hgt / 2.0f);
		}
		else
		{
			rot.SetRotate(r * 100, cgoX + Shape.Wdt / 2.0f, cgoY + Shape.Hgt / 2.0f);
		}
		lpDDraw->Blit(GetGraphics()->GetBitmap(Color),
			fx, fy, fwdt, fhgt,
			cgo.Surface, tx, ty, twdt, thgt,
			true, &rot);
	}
}

void C4Object::UpdateMass()
{
	Mass = std::max<int32_t>((Def->Mass + OwnMass) * Con / FullCon, 1);
	if (!Def->NoComponentMass) Mass += Contents.Mass;
	if (Contained)
	{
		Contained->Contents.MassCount();
		Contained->UpdateMass();
	}
}

void C4Object::ComponentConCutoff()
{
	// this is not ideal, since it does not know about custom builder components
	int32_t cnt;
	for (cnt = 0; Component.GetID(cnt); cnt++)
		Component.SetCount(cnt,
			std::min<int32_t>(Component.GetCount(cnt), Def->Component.GetCount(cnt) * Con / FullCon));
}

void C4Object::ComponentConGain()
{
	// this is not ideal, since it does not know about custom builder components
	int32_t cnt;
	for (cnt = 0; Component.GetID(cnt); cnt++)
		Component.SetCount(cnt,
			std::max<int32_t>(Component.GetCount(cnt), Def->Component.GetCount(cnt) * Con / FullCon));
}

void C4Object::SetOCF()
{
#ifdef DEBUGREC_OCF
	uint32_t dwOCFOld = OCF;
#endif
	// Update the object character flag according to the object's current situation
	C4Fixed cspeed = GetSpeed();
#ifndef NDEBUG
	if (Contained && !Section->Objects.ObjectNumber(Contained))
	{
		LogNTr(spdlog::level::warn, "Contained in wild object {}!", static_cast<void *>(Contained.Denumerated()));
	}
	else if (Contained && !Contained->Status)
	{
		LogNTr(spdlog::level::warn, "Warning: contained in deleted object {} ({})!", static_cast<void *>(Contained.Denumerated()), Contained->GetName());
	}
#endif
	if (Contained)
		InMat = Contained->Def->ClosedContainer ? MNone : Contained->InMat;
	else
		InMat = Section->Landscape.GetMat(x, y);
	// OCF_Normal: The OCF is never zero
	OCF = OCF_Normal;
	// OCF_Construct: Can be built outside
	if (Def->Constructable && (Con < FullCon)
		&& (r == 0) && !OnFire)
		OCF |= OCF_Construct;
	// OCF_Grab: Can be pushed
	if (Def->Grab && !(Category & C4D_StaticBack))
		OCF |= OCF_Grab;
	// OCF_Carryable: Can be picked up
	if (Def->Carryable)
		OCF |= OCF_Carryable;
	// OCF_OnFire: Is burning
	if (OnFire)
		OCF |= OCF_OnFire;
	// OCF_Inflammable: Is not burning and is inflammable
	if (!OnFire && Def->ContactIncinerate > 0)
		// Is not a dead living
		if (!(Category & C4D_Living) || Alive)
			OCF |= OCF_Inflammable;
	// OCF_FullCon: Is fully completed/grown
	if (Con >= FullCon)
		OCF |= OCF_FullCon;
	// OCF_Chop: Can be chopped
	uint32_t cocf = OCF_Exclusive;
	if (Def->Chopable)
		if (Category & C4D_StaticBack) // Must be static back: this excludes trees that have already been chopped
			if (!Section->Objects.AtObject(x, y, cocf)) // Can only be chopped if the center is not blocked by an exclusive object
				OCF |= OCF_Chop;
	// OCF_Rotate: Can be rotated
	if (Def->Rotateable)
		// Don't rotate minimum (invisible) construction sites
		if (Con > 100)
			OCF |= OCF_Rotate;
	// OCF_Exclusive: No action through this, no construction in front of this
	if (Def->Exclusive)
		OCF |= OCF_Exclusive;
	// OCF_Entrance: Can currently be entered/activated
	if ((Def->Entrance.Wdt > 0) && (Def->Entrance.Hgt > 0))
		if ((OCF & OCF_FullCon) && ((Def->RotatedEntrance == 1) || (r <= Def->RotatedEntrance)))
			OCF |= OCF_Entrance;
	// HitSpeeds
	if (cspeed >= HitSpeed1) OCF |= OCF_HitSpeed1;
	if (cspeed >= HitSpeed2) OCF |= OCF_HitSpeed2;
	if (cspeed >= HitSpeed3) OCF |= OCF_HitSpeed3;
	if (cspeed >= HitSpeed4) OCF |= OCF_HitSpeed4;
	// OCF_Collection
	if ((OCF & OCF_FullCon) || Def->IncompleteActivity)
		if ((Def->Collection.Wdt > 0) && (Def->Collection.Hgt > 0))
			if (!Def->CollectionLimit || (Contents.ObjectCount() < Def->CollectionLimit))
				if ((Action.Act <= ActIdle) || (!Def->ActMap[Action.Act].Disabled))
					if (NoCollectDelay == 0)
						OCF |= OCF_Collection;
	// OCF_Living
	if (Category & C4D_Living)
	{
		OCF |= OCF_Living;
		if (Alive) OCF |= OCF_Alive;
	}
	// OCF_FightReady
	if (OCF & OCF_Alive)
		if ((Action.Act <= ActIdle) || (!Def->ActMap[Action.Act].Disabled))
			if (!Def->NoFight)
				OCF |= OCF_FightReady;
	// OCF_LineConstruct
	if (OCF & OCF_FullCon)
		if (Def->LineConnect)
			OCF |= OCF_LineConstruct;
	// OCF_Prey
	if (Def->Prey)
		if (Alive)
			OCF |= OCF_Prey;
	// OCF_CrewMember
	if (Def->CrewMember)
		if (Alive)
			OCF |= OCF_CrewMember;
	// OCF_AttractLightning
	if (Def->AttractLightning)
		if (OCF & OCF_FullCon)
			OCF |= OCF_AttractLightning;
	// OCF_NotContained
	if (!Contained)
		OCF |= OCF_NotContained;
	// OCF_Edible
	if (Def->Edible)
		OCF |= OCF_Edible;
	// OCF_InLiquid
	if (InLiquid)
		if (!Contained)
			OCF |= OCF_InLiquid;
	// OCF_InSolid
	if (!Contained)
		if (Section->Landscape.GBackSolid(x, y))
			OCF |= OCF_InSolid;
	// OCF_InFree
	if (!Contained)
		if (!Section->Landscape.GBackSemiSolid(x, y - 1))
			OCF |= OCF_InFree;
	// OCF_Available
	if (!Contained || (Contained->Def->GrabPutGet & C4D_Grab_Get) || (Contained->OCF & OCF_Entrance))
		if (!Section->Landscape.GBackSemiSolid(x, y - 1) || (!Section->Landscape.GBackSolid(x, y - 1) && !Section->Landscape.GBackSemiSolid(x, y - 8)))
			OCF |= OCF_Available;
	// OCF_PowerConsumer
	if (Def->LineConnect & C4D_Power_Consumer)
		if (OCF & OCF_FullCon)
			OCF |= OCF_PowerConsumer;
	// OCF_PowerSupply
	if ((Def->LineConnect & C4D_Power_Generator)
		|| ((Def->LineConnect & C4D_Power_Output) && (Energy > 0)))
		if (OCF & OCF_FullCon)
			OCF |= OCF_PowerSupply;
	// OCF_Container
	if ((Def->GrabPutGet & C4D_Grab_Put) || (Def->GrabPutGet & C4D_Grab_Get) || (OCF & OCF_Entrance))
		OCF |= OCF_Container;
#ifdef DEBUGREC_OCF
	assert(!dwOCFOld || ((dwOCFOld & OCF_Carryable) == (OCF & OCF_Carryable)));
	C4RCOCF rc = { dwOCFOld, OCF, false };
	AddDbgRec(RCT_OCF, &rc, sizeof(rc));
#endif
}

void C4Object::UpdateOCF()
{
#ifdef DEBUGREC_OCF
	uint32_t dwOCFOld = OCF;
#endif
	// Update the object character flag according to the object's current situation
	C4Fixed cspeed = GetSpeed();
#ifndef NDEBUG
	if (Contained && !Section->Objects.ObjectNumber(Contained))
	{
		LogNTr(spdlog::level::warn, "contained in wild object {}!", static_cast<void *>(Contained.Denumerated()));
	}
	else if (Contained && !Contained->Status)
	{
		LogNTr(spdlog::level::warn, "contained in deleted object {} ({})!", static_cast<void *>(Contained.Denumerated()), Contained->GetName());
	}
#endif
	if (Contained)
		InMat = Contained->Def->ClosedContainer ? MNone : Contained->InMat;
	else
		InMat = Section->Landscape.GetMat(x, y);
	// Keep the bits that only have to be updated with SetOCF (def, category, con, alive, onfire)
	OCF = OCF & (OCF_Normal | OCF_Carryable | OCF_Exclusive | OCF_Edible | OCF_Grab | OCF_FullCon
		/*| OCF_Chop - now updated regularly, see below */
		| OCF_Rotate | OCF_OnFire | OCF_Inflammable | OCF_Living | OCF_Alive
		| OCF_LineConstruct | OCF_Prey | OCF_CrewMember | OCF_AttractLightning
		| OCF_PowerConsumer);
	// OCF_Construct: Can be built outside
	if (Def->Constructable && (Con < FullCon)
		&& (r == 0) && !OnFire)
		OCF |= OCF_Construct;
	// OCF_Entrance: Can currently be entered/activated
	if ((Def->Entrance.Wdt > 0) && (Def->Entrance.Hgt > 0))
		if ((OCF & OCF_FullCon) && ((Def->RotatedEntrance == 1) || (r <= Def->RotatedEntrance)))
			OCF |= OCF_Entrance;
	// OCF_Chop: Can be chopped
	uint32_t cocf = OCF_Exclusive;
	if (Def->Chopable)
		if (Category & C4D_StaticBack) // Must be static back: this excludes trees that have already been chopped
			if (!Section->Objects.AtObject(x, y, cocf)) // Can only be chopped if the center is not blocked by an exclusive object
				OCF |= OCF_Chop;
	// HitSpeeds
	if (cspeed >= HitSpeed1) OCF |= OCF_HitSpeed1;
	if (cspeed >= HitSpeed2) OCF |= OCF_HitSpeed2;
	if (cspeed >= HitSpeed3) OCF |= OCF_HitSpeed3;
	if (cspeed >= HitSpeed4) OCF |= OCF_HitSpeed4;
	// OCF_Collection
	if ((OCF & OCF_FullCon) || Def->IncompleteActivity)
		if ((Def->Collection.Wdt > 0) && (Def->Collection.Hgt > 0))
			if (!Def->CollectionLimit || (Contents.ObjectCount() < Def->CollectionLimit))
				if ((Action.Act <= ActIdle) || (!Def->ActMap[Action.Act].Disabled))
					if (NoCollectDelay == 0)
						OCF |= OCF_Collection;
	// OCF_FightReady
	if (OCF & OCF_Alive)
		if ((Action.Act <= ActIdle) || (!Def->ActMap[Action.Act].Disabled))
			if (!Def->NoFight)
				OCF |= OCF_FightReady;
	// OCF_NotContained
	if (!Contained)
		OCF |= OCF_NotContained;
	// OCF_InLiquid
	if (InLiquid)
		if (!Contained)
			OCF |= OCF_InLiquid;
	// OCF_InSolid
	if (!Contained)
		if (Section->Landscape.GBackSolid(x, y))
			OCF |= OCF_InSolid;
	// OCF_InFree
	if (!Contained)
		if (!Section->Landscape.GBackSemiSolid(x, y - 1))
			OCF |= OCF_InFree;
	// OCF_Available
	if (!Contained || (Contained->Def->GrabPutGet & C4D_Grab_Get) || (Contained->OCF & OCF_Entrance))
		if (!Section->Landscape.GBackSemiSolid(x, y - 1) || (!Section->Landscape.GBackSolid(x, y - 1) && !Section->Landscape.GBackSemiSolid(x, y - 8)))
			OCF |= OCF_Available;
	// OCF_PowerSupply
	if ((Def->LineConnect & C4D_Power_Generator)
		|| ((Def->LineConnect & C4D_Power_Output) && (Energy > 0)))
		if (OCF & OCF_FullCon)
			OCF |= OCF_PowerSupply;
	// OCF_Container
	if ((Def->GrabPutGet & C4D_Grab_Put) || (Def->GrabPutGet & C4D_Grab_Get) || (OCF & OCF_Entrance))
		OCF |= OCF_Container;
#ifdef DEBUGREC_OCF
	C4RCOCF rc = { dwOCFOld, OCF, true };
	AddDbgRec(RCT_OCF, &rc, sizeof(rc));
#endif
#ifndef NDEBUG
	DEBUGREC_OFF
		uint32_t updateOCF = OCF;
	SetOCF();
	assert(updateOCF == OCF);
	DEBUGREC_ON
#endif
}

bool C4Object::ExecFire(int32_t iFireNumber, int32_t iCausedByPlr)
{
	// Fire Phase
	FirePhase++; if (FirePhase >= MaxFirePhase) FirePhase = 0;
	// Extinguish in base
	if (!Tick5)
		if (Category & C4D_Living)
			if (Contained && ValidPlr(Contained->Base))
				if (Section->C4S.Game.Realism.BaseFunctionality & BASEFUNC_Extinguish)
					Extinguish(iFireNumber);
	// Decay
	if (!Def->NoBurnDecay)
		DoCon(-100);
	// Damage
	if (!Tick10) if (!Def->NoBurnDamage) DoDamage(+2, iCausedByPlr, C4FxCall_DmgFire);
	// Energy
	if (!Tick5) DoEnergy(-1, false, C4FxCall_EngFire, iCausedByPlr);
	// Effects
	int32_t smoke_level = 2 * Shape.Wdt / 3;
	int32_t smoke_rate = Def->SmokeRate;
	if (smoke_rate)
	{
		smoke_rate = 50 * smoke_level / smoke_rate;
		if (!((Game.FrameCounter + (Number * 7)) % std::max<int32_t>(smoke_rate, 3)) || (Abs(xdir) > 2))
			Smoke(*Section, x, y, smoke_level);
	}
	// Background Effects
	if (!Tick5)
	{
		int32_t mat;
		if (Section->MatValid(mat = Section->Landscape.GetMat(x, y)))
		{
			// Extinguish
			if (Section->Material.Map[mat].Extinguisher)
			{
				Extinguish(iFireNumber); if (Section->Landscape.GBackLiquid(x, y)) StartSoundEffect("Pshshsh", false, 100, this);
			}
			// Inflame
			if (!Random(3))
				Section->Landscape.Incinerate(x, y);
		}
	}

	return true;
}

bool C4Object::BuyEnergy()
{
	C4Player *pPlr = Game.Players.Get(Base); if (!pPlr) return false;
	if (!GetPhysical()->Energy) return false;
	if (pPlr->Eliminated) return false;
	if (pPlr->Wealth < Section->C4S.Game.Realism.BaseRegenerateEnergyPrice) return false;
	pPlr->DoWealth(-Section->C4S.Game.Realism.BaseRegenerateEnergyPrice);
	DoEnergy(+100, false, C4FxCall_EngBaseRefresh, Owner);
	return true;
}

bool C4Object::ExecLife()
{
	// Growth
	if (!Tick35)
		// Growth specified by definition
		if (Def->Growth)
			// Alive livings && trees only
			if (((Category & C4D_Living) && Alive)
				|| (Category & C4D_StaticBack))
				// Not burning
				if (!OnFire)
					// Not complete yet
					if (Con < FullCon)
						// Grow
						DoCon(Def->Growth * 100);

	// Energy reload in base
	int32_t transfer;
	if (!Tick3) if (Alive)
		if (Contained && ValidPlr(Contained->Base))
			if (!Hostile(Owner, Contained->Base))
				if (Energy < GetPhysical()->Energy)
					if (Section->C4S.Game.Realism.BaseFunctionality & BASEFUNC_RegenerateEnergy)
					{
						if (Contained->Energy <= 0) Contained->BuyEnergy();
						transfer = std::min<int32_t>(std::min<int32_t>(2 * C4MaxPhysical / 100, Contained->Energy), GetPhysical()->Energy - Energy);
						if (transfer)
						{
							Contained->DoEnergy(-transfer, true, C4FxCall_EngBaseRefresh, Contained->Owner);
							DoEnergy(transfer, true, C4FxCall_EngBaseRefresh, Contained->Owner);
						}
					}

	// Magic reload
	if (!Tick3) if (Alive)
		if (Contained)
			if (!Hostile(Owner, Contained->Owner))
				if (MagicEnergy < GetPhysical()->Magic)
				{
					transfer = std::min<int32_t>(std::min<int32_t>(2 * MagicPhysicalFactor, Contained->MagicEnergy), GetPhysical()->Magic - MagicEnergy) / MagicPhysicalFactor;
					if (transfer)
					{
						// do energy transfer via script, so it can be overloaded by No-Magic-Energy-rule
						// always use global func instead of local to save double search
						C4AulFunc *pMagicEnergyFn = Game.ScriptEngine.GetFuncRecursive(PSF_DoMagicEnergy);
						if (pMagicEnergyFn) // should always be true
						{
							if (pMagicEnergyFn->Exec(*Section, nullptr, {C4VInt(-transfer), C4VObj(Contained)}))
							{
								pMagicEnergyFn->Exec(*Section, nullptr, {C4VInt(+transfer), C4VObj(this)});
							}
						}
					}
				}

	// Breathing
	if (!Tick5)
		if (Alive && !Def->NoBreath)
		{
			// Supply check
			bool Breathe = false;
			// Forcefields are breathable.
			if (Section->Landscape.GetMat(x, y + Shape.y / 2) == Section->Landscape.MVehic)
			{
				Breathe = true;
			}
			else if (GetPhysical()->BreatheWater)
			{
				if (Section->Landscape.GetMat(x, y) == Section->Landscape.MWater) Breathe = true;
			}
			else
			{
				if (!Section->Landscape.GBackSemiSolid(x, y + Shape.y / 2)) Breathe = true;
			}
			if (Contained) Breathe = true;
			// No supply
			if (!Breathe)
			{
				// Reduce breath, then energy, bubble
				// Asphyxiation cause is last energy loss cause player, so kill tracing works when player is pushed into liquid
				if (Breath > 0) Breath = (std::max)(Breath - 2 * C4MaxPhysical / 100, 0);
				else DoEnergy(-1, false, C4FxCall_EngAsphyxiation, LastEnergyLossCausePlayer);
				BubbleOut(*Section, x + Random(5) - 2, y + Shape.y / 2);
				ViewEnergy = C4ViewDelay;
				// Physical training
				TrainPhysical(&C4PhysicalInfo::Breath, 2, C4MaxPhysical);
			}
			// Supply
			else
			{
				// Take breath
				int32_t takebreath = GetPhysical()->Breath - Breath;
				if (takebreath > GetPhysical()->Breath / 2)
					Call(PSF_DeepBreath);
				Breath += takebreath;
			}
		}

	// Corrosion energy loss
	if (!Tick10)
		if (Alive)
			if (InMat != MNone)
				if (Section->Material.Map[InMat].Corrosive)
					if (!GetPhysical()->CorrosionResist)
						// Inflict corrision damage by last energy loss cause player, so tumbling enemies into an acid lake attribute kills properly
						DoEnergy(-Section->Material.Map[InMat].Corrosive / 15, false, C4FxCall_EngCorrosion, LastEnergyLossCausePlayer);

	// InMat incineration
	if (!Tick10)
		if (InMat != MNone)
			if (Section->Material.Map[InMat].Incindiary)
				if (Def->ContactIncinerate)
					// Inflict fire by last energy loss cause player, so tumbling enemies into a lava lake attribute kills properly
					Incinerate(LastEnergyLossCausePlayer);

	// Nonlife normal energy loss
	if (!Tick10) if (Energy)
		if (!(Category & C4D_Living))
			if (!ValidPlr(Base) || (~Section->C4S.Game.Realism.BaseFunctionality & BASEFUNC_RegenerateEnergy))
				// don't loose if assigned as Energy-holder
				if (!(Def->LineConnect & C4D_EnergyHolder))
					DoEnergy(-1, false, C4FxCall_EngStruct, NO_OWNER);

	// birthday
	if (!Tick255)
		if (Alive)
			if (Info)
			{
				int32_t iPlayingTime = Info->TotalPlayingTime + (Game.Time - Info->InActionTime);

				int32_t iNewAge = iPlayingTime / 3600 / 5;

				if (Info->Age != iNewAge)
				{
					// message
					GameMsgObject(LoadResStr(C4ResStrTableKey::IDS_OBJ_BIRTHDAY, GetName(), iNewAge).c_str(), this);
					StartSoundEffect("Trumpet", false, 100, this);
				}

				Info->Age = iNewAge;
			}

	return true;
}

void C4Object::AutoSellContents()
{
	C4Player *pPlr = Game.Players.Get(Base); if (!pPlr) return;

	for (auto outerIt = std::begin(Contents); outerIt != std::end(Contents); )
	{
		const auto &obj = *outerIt;
		++outerIt;
		if (obj && obj->Status)
		{
			for (auto innerIt = std::begin(obj->Contents); innerIt != std::end(obj->Contents); )
			{
				const auto &contents = *innerIt;
				++innerIt;
				if (contents && contents->Status && contents->Def->BaseAutoSell && pPlr->CanSell(contents))
				{
					contents->Exit();
					pPlr->Sell2Home(contents);
				}
			}

			if (obj->Def->BaseAutoSell && pPlr->CanSell(obj))
			{
				obj->Exit();
				pPlr->Sell2Home(obj);
			}
		}
	}
}

void C4Object::ExecBase()
{
	C4Object *flag;

	// New base assignment by flag (no old base removal)
	if (!Tick10)
		if (Def->CanBeBase) if (!ValidPlr(Base))
			if (flag = Contents.Find(C4ID_Flag))
				if (ValidPlr(flag->Owner) && (flag->Owner != Base))
				{
					// Attach new flag
					flag->Exit();
					flag->SetActionByName("FlyBase", this);
					// Assign new base
					Base = flag->Owner;
					Contents.CloseMenus();
					StartSoundEffect("Trumpet", false, 100, this);
					SetOwner(flag->Owner);
				}

	// Base execution
	if (!Tick35)
		if (ValidPlr(Base))
		{
			// Auto sell contents
			if (Section->C4S.Game.Realism.BaseFunctionality & BASEFUNC_AutoSellContents) AutoSellContents();
			// Lost flag?
			if (!Section->FindObject(C4ID_Flag, 0, 0, 0, 0, OCF_All, "FlyBase", this))
			{
				Base = NO_OWNER;
				Contents.CloseMenus();
			}
		}

	// Environmental action
	if (!Tick35)
	{
		// Structures dig free snow
		if ((Category & C4D_Structure) && !(Game.Rules & C4RULE_StructuresSnowIn))
			if (r == 0)
			{
				Section->Landscape.DigFreeMat(x + Shape.x, y + Shape.y, Shape.Wdt, Shape.Hgt, Section->Landscape.MSnow);
				Section->Landscape.DigFreeMat(x + Shape.x, y + Shape.y, Shape.Wdt, Shape.Hgt, Section->Material.Get("FlyAshes"));
			}
	}
}

void C4Object::Execute()
{
#ifdef DEBUGREC
	// record debug
	C4RCExecObj rc;
	rc.Number = Number;
	rc.id = Def->id;
	rc.fx = fix_x;
	rc.fy = fix_y;
	rc.fr = fix_r;
	AddDbgRec(RCT_ExecObj, &rc, sizeof(rc));
#endif
	// OCF
	UpdateOCF();
	// Command
	ExecuteCommand();
	// Action
	// need not check status, because dead objects have lost their action
	ExecAction();
	// commands and actions are likely to have removed the object, and movement
	// *must not* be executed for dead objects (SolidMask-errors)
	if (!Status) return;
	// Movement
	ExecMovement();
	if (!Status) return;
	// particles
	if (BackParticles) BackParticles.Exec(*Section, this);
	if (FrontParticles) FrontParticles.Exec(*Section, this);
	// effects
	if (pEffects)
	{
		pEffects->Execute(this);
		if (!Status) return;
	}
	// Life
	ExecLife();
	// Base
	ExecBase();
	// Timer
	Timer++;
	if (Timer >= Def->Timer)
	{
		Timer = 0;
		// TimerCall
		if (Def->TimerCall) Def->TimerCall->Exec(*Section, this);
	}
	// Menu
	if (Menu) Menu->Execute();
	// View delays
	if (ViewEnergy > 0) ViewEnergy--;
}

bool C4Object::At(int32_t ctx, int32_t cty)
{
	if (Status) if (!Contained) if (Def)
		if (Inside<int32_t>(cty - (y + Shape.y - addtop()), 0, Shape.Hgt - 1 + addtop()))
			if (Inside<int32_t>(ctx - (x + Shape.x), 0, Shape.Wdt - 1))
				return true;
	return false;
}

bool C4Object::At(int32_t ctx, int32_t cty, uint32_t &ocf)
{
	if (Status) if (!Contained) if (Def)
		if (OCF & ocf)
			if (Inside<int32_t>(cty - (y + Shape.y - addtop()), 0, Shape.Hgt - 1 + addtop()))
				if (Inside<int32_t>(ctx - (x + Shape.x), 0, Shape.Wdt - 1))
				{
					// Set ocf return value
					GetOCFForPos(ctx, cty, ocf);
					return true;
				}

	return false;
}

void C4Object::GetOCFForPos(int32_t ctx, int32_t cty, uint32_t &ocf)
{
	uint32_t rocf = OCF;
	// Verify entrance area OCF return
	if (rocf & OCF_Entrance)
		if (!Inside<int32_t>(cty - (y + Def->Entrance.y), 0, Def->Entrance.Hgt - 1)
			|| !Inside<int32_t>(ctx - (x + Def->Entrance.x), 0, Def->Entrance.Wdt - 1))
			rocf &= (~OCF_Entrance);
	// Verify collection area OCF return
	if (rocf & OCF_Collection)
		if (!Inside<int32_t>(cty - (y + Def->Collection.y), 0, Def->Collection.Hgt - 1)
			|| !Inside<int32_t>(ctx - (x + Def->Collection.x), 0, Def->Collection.Wdt - 1))
			rocf &= (~OCF_Collection);
	ocf = rocf;
}

void C4Object::AssignDeath(bool fForced)
{
	C4Object *thing;
	// Alive objects only
	if (!Alive) return;
	// clear all effects
	// do not delete effects afterwards, because they might have denied removal
	// set alive-flag before, so objects know what's up
	// and prevent recursive death-calls this way
	// get death causing player before doing effect calls, because those might meddle around with the flags
	int32_t iDeathCausingPlayer = LastEnergyLossCausePlayer;
	Alive = 0;
	if (pEffects) pEffects->ClearAll(this, C4FxCall_RemoveDeath);
	// if the object is alive again, abort here if the kill is not forced
	if (Alive && !fForced) return;
	// Action
	SetActionByName("Dead");
	// Values
	Select = 0;
	Alive = 0;
	ClearCommands();
	if (Info)
	{
		Info->HasDied = true;
		++Info->DeathCount;
		Info->Retire();
	}
	// Lose contents
	while (thing = Contents.GetObject()) thing->Exit(thing->x, thing->y);
	// Remove from crew/cursor/view
	C4Player *pPlr = Game.Players.Get(Owner);
	if (pPlr) pPlr->ClearPointers(this, true);
	// ensure objects that won't be affected by dead-plrview-decay are handled properly
	if (!pPlr || !(Category & C4D_Living) || !pPlr->FoWViewObjs.IsContained(this))
		SetPlrViewRange(0);
	// Engine script call
	Call(PSF_Death, {C4VInt(iDeathCausingPlayer)});
	// Update OCF. Done here because previously it would have been done in the next frame
	// Whats worse: Having the OCF change because of some unrelated script-call like
	// SetCategory, or slightly breaking compatibility?
	SetOCF();
}

bool C4Object::ChangeDef(C4ID idNew)
{
	// Get new definition
	C4Def *pDef = Game.Defs.ID2Def(idNew);
	if (!pDef) return false;
	// Containment storage
	C4Object *pContainer = Contained;
	// Exit container (no Ejection/Departure)
	if (Contained) Exit(0, 0, 0, Fix0, Fix0, Fix0, false);
	// Pre change resets
	SetAction(ActIdle);
	Action.Act = ActIdle; // Enforce ActIdle because SetAction may have failed due to NoOtherAction
	SetDir(0); // will drop any outdated flipdir
	if (pSolidMaskData) pSolidMaskData->Remove(true, false);
	delete pSolidMaskData; pSolidMaskData = nullptr;
	Def->Count--;
	// Def change
	Def = pDef;
	id = pDef->id;
	Def->Count++;
	LocalNamed.SetNameList(&pDef->Script.LocalNamed);
	// new def: Needs to be resorted
	Unsorted = true;
	// graphics change
	pGraphics = &pDef->Graphics;
	// blit mode adjustment
	if (!(BlitMode & C4GFXBLIT_CUSTOM)) BlitMode = Def->BlitMode;
	// an object may have newly become an ColorByOwner-object
	// if it had been ColorByOwner, but is not now, this will be caught in UpdateGraphics()
	if (!Color && ValidPlr(Owner))
		Color = Game.Players.Get(Owner)->ColorDw;
	if (!Def->Rotateable) { r = 0; fix_r = rdir = Fix0; }
	// Reset solid mask
	SolidMask = Def->SolidMask;
	// Post change updates
	UpdateGraphics(true);
	UpdateMass();
	UpdateFace(true);
	SetOCF();
	// Any effect callbacks to this object might need to reinitialize their target functions
	// This is ugly, because every effect there is must be updated...
	Game.OnObjectChangedDef(this);

	for (C4Object *const obj : Game.GetAllObjects())
	{
		obj->pEffects->OnObjectChangedDef(this);
	}
	// Containment (no Entrance)
	if (pContainer) Enter(pContainer, false);
	// Done
	return true;
}

bool C4Object::Incinerate(int32_t iCausedBy, bool fBlasted, C4Object *pIncineratingObject)
{
	// Already on fire
	if (OnFire) return false;
	// Dead living don't burn
	if ((Category & C4D_Living) && !Alive) return false;
	// add effect
	int32_t iEffNumber;
	C4Value Par1 = C4VInt(iCausedBy), Par2 = C4VBool(!!fBlasted), Par3 = C4VObj(pIncineratingObject), Par4;
	new C4Effect(*Section, this, C4Fx_Fire, C4Fx_FirePriority, C4Fx_FireTimer, nullptr, 0, Par1, Par2, Par3, Par4, true, iEffNumber);
	return !!iEffNumber;
}

bool C4Object::Extinguish(int32_t iFireNumber)
{
	// any effects?
	if (!pEffects) return false;
	// fire number known: extinguish that fire
	C4Effect *pEffFire;
	if (iFireNumber)
	{
		pEffFire = pEffects->Get(iFireNumber, false);
		if (!pEffFire) return false;
		pEffFire->Kill(this);
	}
	else
	{
		// otherwise, kill all fires
		// (keep checking from beginning of pEffects, as Kill might delete or change effects)
		int32_t iFiresKilled = 0;
		while (pEffects && (pEffFire = pEffects->Get(C4Fx_AnyFire)))
		{
			while (pEffFire && WildcardMatch(C4Fx_Internal, pEffFire->Name))
			{
				pEffFire = pEffFire->pNext;
				if (pEffFire) pEffFire = pEffFire->Get(C4Fx_AnyFire);
			}
			if (!pEffFire) break;
			pEffFire->Kill(this);
			++iFiresKilled;
		}
		if (!iFiresKilled) return false;
	}
	// done, success
	return true;
}

void C4Object::DoDamage(int32_t iChange, int32_t iCausedBy, int32_t iCause)
{
	// non-living: ask effects first
	if (pEffects && !Alive)
	{
		pEffects->DoDamage(this, iChange, iCause, iCausedBy);
		if (!iChange) return;
	}
	// Change value
	Damage = std::max<int32_t>(Damage + iChange, 0);
	// Engine script call
	Call(PSF_Damage, {C4VInt(iChange), C4VInt(iCausedBy)});
}

// returns x * y, but returns std::numeric_limits<T>::min() or std::numeric_limits<T>::max() in case of a negative or positive overflow respectively
template<typename T>
constexpr T clampedMultiplication(T x, T y)
{
	if (y == 0) return 0;

	if (y < 0)
	{
		if (x < 0)
		{
			x *= -1;
			y *= -1;
		}
		else
		{
			std::swap(x, y);
		}
	}
	constexpr auto min = std::numeric_limits<T>::min();
	constexpr auto max = std::numeric_limits<T>::max();
	if (x < 0 && x < min / y) // would give negative overflow, so make it the most negative possible
	{
		return min;
	}
	else if (x > 0 && x > max / y) // would give positive overflow, so make it the most positive possible
	{
		return max;
	}
	else
	{
		return x * y;
	}
}

// same as above, but calculating x + y instead
template<typename T>
constexpr T clampedAddition(T x, T y)
{
	constexpr auto min = std::numeric_limits<T>::min();
	constexpr auto max = std::numeric_limits<T>::max();
	if (x < 0 && y < min - x) // would give negative overflow, so make it the most negative possible
	{
		return min;
	}
	else if (x > 0 && y > max - x) // would give positive overflow, so make it the most positive possible
	{
		return max;
	}
	else
	{
		return x + y;
	}
}

void C4Object::DoEnergy(int32_t iChange, bool fExact, int32_t iCause, int32_t iCausedByPlr)
{
	// iChange 100% = Physical 100000
	if (!fExact) iChange = clampedMultiplication(C4MaxPhysical / 100, iChange);
	// Was zero?
	bool fWasZero = (Energy == 0);
	// Mark last damage causing player to trace kills. Always update on C4FxCall_EngObjHit even if iChange==0
	// so low mass objects also correctly set the killer
	if (iChange < 0 || iCause == C4FxCall_EngObjHit) UpdatLastEnergyLossCause(iCausedByPlr);
	// Living things: ask effects for change first
	if (pEffects && Alive)
	{
		pEffects->DoDamage(this, iChange, iCause, iCausedByPlr);
		if (!iChange) return;
	}
	// Do change
	Energy = BoundBy<int32_t>(clampedAddition(Energy, iChange), 0, GetPhysical()->Energy);
	// Alive and energy reduced to zero: death
	if (Alive) if (Energy == 0) if (!fWasZero) AssignDeath(false);
	// View change
	ViewEnergy = C4ViewDelay;
}

void C4Object::UpdatLastEnergyLossCause(int32_t iNewCausePlr)
{
	// Mark last damage causing player to trace kills
	// do not regard self-administered damage if there was a previous damage causing player, because that would steal kills
	// if people tumble themselves via stop-stop-(left/right)-throw while falling into teh abyss
	if (iNewCausePlr != Controller || LastEnergyLossCausePlayer < 0)
	{
		LastEnergyLossCausePlayer = iNewCausePlr;
	}
}

void C4Object::DoBreath(int32_t iChange)
{
	// iChange 100% = Physical 100000
	iChange = clampedMultiplication(C4MaxPhysical / 100, iChange);
	// Do change
	Breath = BoundBy<int32_t>(clampedAddition(Breath, iChange), 0, GetPhysical()->Breath);
	// View change
	ViewEnergy = C4ViewDelay;
}

void C4Object::Blast(int32_t iLevel, int32_t iCausedBy)
{
	// Damage
	DoDamage(iLevel, iCausedBy, C4FxCall_DmgBlast);
	// Energy (alive objects)
	if (Alive) DoEnergy(-iLevel / 3, false, C4FxCall_EngBlast, iCausedBy);
	// Incinerate
	if (Def->BlastIncinerate)
		if (Damage >= Def->BlastIncinerate)
			Incinerate(iCausedBy, true);
}

void C4Object::DoCon(int32_t iChange, bool fInitial, bool fNoComponentChange)
{
	int32_t iStepSize = FullCon / 100;
	int32_t lRHgt = Shape.Hgt, lRy = Shape.y;
	int32_t iLastStep = Con / iStepSize;
	int32_t strgt_con_b = y + Shape.y + Shape.Hgt;
	bool fWasFull = (Con >= FullCon);

	// Change con
	if (Def->Oversize)
		Con = std::max<int32_t>(Con + iChange, 0);
	else
		Con = BoundBy<int32_t>(Con + iChange, 0, FullCon);
	int32_t iStepDiff = Con / iStepSize - iLastStep;

	// Update OCF
	SetOCF();

	// If step changed or limit reached or degraded from full: update mass, face, components, etc.
	if (iStepDiff || (Con >= FullCon) || (Con == 0) || (fWasFull && (Con < FullCon)))
	{
		// Mass
		UpdateMass();
		// Decay from full remove mask before face is changed
		if (fWasFull && (Con < FullCon))
			if (pSolidMaskData) pSolidMaskData->Remove(true, false);
		// Face
		UpdateFace(true);
		// component update
		if (!fNoComponentChange)
		{
			// Decay: reduce components
			if (iChange < 0)
				ComponentConCutoff();
			// Growth: gain components
			else
				ComponentConGain();
		}
		// Unfullcon
		if (Con < FullCon)
		{
			// Lose contents
			if (!Def->IncompleteActivity)
			{
				C4Object *cobj;
				while (cobj = Contents.GetObject())
					if (Contained) cobj->Enter(Contained);
					else cobj->Exit(cobj->x, cobj->y);
			}
			// No energy need
			NeedEnergy = 0;
		}
		// Decay from full stop action
		if (fWasFull && (Con < FullCon))
			if (!Def->IncompleteActivity)
				SetAction(ActIdle);
	}
	else
		// set first position
		if (fInitial) UpdatePos();

	// Straight Con bottom y-adjust
	if (!r || fInitial)
	{
		if ((Shape.Hgt != lRHgt) || (Shape.y != lRy))
		{
			y = strgt_con_b - Shape.Hgt - Shape.y;
			UpdatePos(); UpdateSolidMask(false);
		}
	}
	else if (Category & C4D_Structure) if (iStepDiff > 0)
	{
		// even rotated buildings need to be moved upwards
		// but by con difference, because with keep-bottom-method, they might still be sinking
		// besides, moving the building up may often stabilize it
		y -= ((iLastStep + iStepDiff) * Def->Shape.Hgt / 100) - (iLastStep * Def->Shape.Hgt / 100);
		UpdatePos(); UpdateSolidMask(false);
	}
	// Completion (after bottom y-adjust for correct position)
	if (!fWasFull && (Con >= FullCon))
	{
		Call(PSF_Completion);
		Call(PSF_Initialize);
	}

	// Con Zero Removal
	if (Con <= 0)
		AssignRemoval();
}

void C4Object::DoExperience(int32_t change)
{
	const int32_t MaxExperience = 100000000;

	if (!Info) return;

	Info->Experience = BoundBy<int32_t>(Info->Experience + change, 0, MaxExperience);

	// Promotion check
	if (Info->Experience < MaxExperience)
		if (Info->Experience >= Game.Rank.Experience(Info->Rank + 1))
			Promote(Info->Rank + 1, false);
}

bool C4Object::Exit(int32_t iX, int32_t iY, int32_t iR, C4Fixed iXDir, C4Fixed iYDir, C4Fixed iRDir, bool fCalls)
{
	// 1. Exit the current container.
	// 2. Update Contents of container object and set Contained to nullptr.
	// 3. Set offset position/motion if desired.
	// 4. Call Ejection for container and Departure for object.

	// Not contained
	C4Object *pContainer = Contained;
	if (!pContainer) return false;
	// Remove object from container
	pContainer->Contents.Remove(this);
	pContainer->UpdateMass();
	pContainer->SetOCF();
	// No container
	Contained = nullptr;
	// Position/motion
	BoundsCheck(iX, iY);
	x = iX; y = iY; r = iR;
	fix_x = itofix(x); fix_y = itofix(y); fix_r = itofix(r);
	xdir = iXDir; ydir = iYDir; rdir = iRDir;
	// Misc updates
	Mobile = 1;
	InLiquid = 0;
	CloseMenu(true);
	UpdateFace(true);
	SetOCF();
	// Engine calls
	if (fCalls) pContainer->Call(PSF_Ejection, {C4VObj(this)});
	if (fCalls) Call(PSF_Departure, {C4VObj(pContainer)});
	// Success (if the obj wasn't "re-entered" by script)
	return !Contained;
}

bool C4Object::Enter(C4Object *pTarget, bool fCalls, bool fCopyMotion, bool *pfRejectCollect)
{
	// 0. Query entrance and collection
	// 1. Exit if contained.
	// 2. Set new container.
	// 3. Update Contents and mass of the new container.
	// 4. Call collection for container
	// 5. Call entrance for object.

	// No target or target is self
	if (!pTarget || (pTarget == this)) return false;
	// check if entrance is allowed
	if (Call(PSF_RejectEntrance, {C4VObj(pTarget)})) return false;
	// check if we end up in an endless container-recursion
	for (C4Object *pCnt = pTarget->Contained; pCnt; pCnt = pCnt->Contained)
		if (pCnt == this) return false;
	// Check RejectCollect, if desired
	if (pfRejectCollect)
	{
		if (pTarget->Call(PSF_RejectCollection, {C4VID(Def->id), C4VObj(this)}))
		{
			*pfRejectCollect = true;
			return false;
		}
		*pfRejectCollect = false;
	}
	// Exit if contained
	if (Contained) if (!Exit(x, y)) return false;
	if (Contained || !Status || !pTarget->Status) return false;
	// Failsafe updates
	CloseMenu(true);
	SetOCF();
	// Set container
	Contained = pTarget;
	// Enter
	if (!Contained->Contents.Add(this, C4ObjectList::stContents))
	{
		Contained = nullptr;
		return false;
	}
	// Assume that the new container controls this object, if it cannot control itself (i.e.: Alive)
	// So it can be traced back who caused the damage, if a projectile hits its target
	if (!(Alive && (Category & C4D_Living)))
		Controller = pTarget->Controller;
	// Misc updates
	// motion must be copied immediately, so the position will be correct when OCF is set, and
	// OCF_Available will be set for newly bought items, even if 50/50 is solid in the landscape
	// however, the motion must be preserved sometimes to keep flags like OCF_HitSpeed upon collection
	if (fCopyMotion)
	{
		// remove any solidmask before copying the motion...
		UpdateSolidMask(false);
		CopyMotion(Contained);
	}
	// Set section
	MoveToSection(*Contained->Section, false);
	SetOCF();
	UpdateFace(true);
	// Update container
	Contained->UpdateMass();
	Contained->SetOCF();
	// Collection call
	if (fCalls) pTarget->Call(PSF_Collection2, {C4VObj(this)});
	if (!Contained || !Contained->Status || !pTarget->Status) return true;
	// Entrance call
	if (fCalls) Call(PSF_Entrance, {C4VObj(Contained)});
	if (!Contained || !Contained->Status || !pTarget->Status) return true;
	// Base auto sell contents
	if (ValidPlr(Contained->Base))
		if (Section->C4S.Game.Realism.BaseFunctionality & BASEFUNC_AutoSellContents)
			Contained->AutoSellContents();
	// Success
	return true;
}

void C4Object::Fling(C4Fixed txdir, C4Fixed tydir, bool fAddSpeed, int32_t iCausedBy)
{
	// trace indirect killers
	if (OCF & OCF_Alive) UpdatLastEnergyLossCause(iCausedBy); else if (!Contained) Controller = iCausedBy;
	// fling object
	if (fAddSpeed) { txdir += xdir / 2; tydir += ydir / 2; }
	if (!ObjectActionTumble(this, (txdir < 0), txdir, tydir))
		if (!ObjectActionJump(this, txdir, tydir, false))
		{
			xdir = txdir; ydir = tydir;
			Mobile = 1;
			Action.t_attach &= ~CNAT_Bottom;
		}
}

bool C4Object::ActivateEntrance(int32_t by_plr, C4Object *by_obj)
{
	// Hostile: no entrance to base
	if (Hostile(by_plr, Base) && (Section->C4S.Game.Realism.BaseFunctionality & BASEFUNC_RejectEntrance))
	{
		if (ValidPlr(Owner))
		{
			GameMsgObject(LoadResStr(C4ResStrTableKey::IDS_OBJ_HOSTILENOENTRANCE, Game.Players.Get(Owner)->GetName()).c_str(), this);
		}
		return false;
	}
	// Try entrance activation
	if (OCF & OCF_Entrance)
		if (Call(PSF_ActivateEntrance, {C4VObj(by_obj)}))
			return true;
	// Failure
	return false;
}

void C4Object::Explode(int32_t iLevel, C4ID idEffect, const char *szEffect)
{
	// Called by FnExplode only
	C4Object *pContainer = Contained;
	int32_t iCausedBy = Controller;
	AssignRemoval();
	Explosion(x, y, iLevel, pContainer, iCausedBy, this, idEffect, szEffect);
}

bool C4Object::Build(int32_t iLevel, C4Object *pBuilder)
{
	int32_t cnt;
	C4ID NeededMaterial;
	int32_t NeededMaterialCount = 0;
	C4Object *pMaterial;

	// Invalid or complete: no build
	if (!Status || !Def || (Con >= FullCon)) return false;

	// Material check (if rule set or any other than structure or castle-part)
	bool fNeedMaterial = (Game.Rules & C4RULE_ConstructionNeedsMaterial) || !(Category & (C4D_Structure | C4D_StaticBack));
	if (fNeedMaterial)
	{
		// Determine needed components (may be overloaded)
		C4IDList NeededComponents;
		Def->GetComponents(&NeededComponents, *Section, nullptr, pBuilder);

		// Grab any needed components from builder
		C4ID idMat;
		for (cnt = 0; idMat = NeededComponents.GetID(cnt); cnt++)
			if (Component.GetIDCount(idMat) < NeededComponents.GetCount(cnt))
				if ((pMaterial = pBuilder->Contents.Find(idMat)))
					if (!pMaterial->OnFire) if (pMaterial->OCF & OCF_FullCon)
					{
						Component.SetIDCount(idMat, Component.GetIDCount(idMat) + 1, true);
						pBuilder->Contents.Remove(pMaterial);
						pMaterial->AssignRemoval();
					}
		// Grab any needed components from container
		if (Contained)
			for (cnt = 0; idMat = NeededComponents.GetID(cnt); cnt++)
				if (Component.GetIDCount(idMat) < NeededComponents.GetCount(cnt))
					if ((pMaterial = Contained->Contents.Find(idMat)))
						if (!pMaterial->OnFire) if (pMaterial->OCF & OCF_FullCon)
						{
							Component.SetIDCount(idMat, Component.GetIDCount(idMat) + 1, true);
							Contained->Contents.Remove(pMaterial);
							pMaterial->AssignRemoval();
						}
		// Check for needed components at current con
		for (cnt = 0; idMat = NeededComponents.GetID(cnt); cnt++)
			if (NeededComponents.GetCount(cnt) != 0)
				if ((100 * Component.GetIDCount(idMat) / NeededComponents.GetCount(cnt)) < (100 * Con / FullCon))
				{
					NeededMaterial = NeededComponents.GetID(cnt);
					NeededMaterialCount = NeededComponents.GetCount(cnt) - Component.GetCount(cnt);
					break;
				}
	}

	// Needs components
	if (NeededMaterialCount)
	{
		// BuildNeedsMaterial call to builder script...
		if (!pBuilder->Call(PSF_BuildNeedsMaterial, {C4VID(NeededMaterial), C4VInt(NeededMaterialCount)}))
		{
			// Builder is a crew member...
			if (pBuilder->OCF & OCF_CrewMember)
				// ...tell builder to acquire the material
				pBuilder->AddCommand(C4CMD_Acquire, nullptr, 0, 0, 50, nullptr, true, NeededMaterial, false, 1);
			// ...game message if not overloaded
			Game.Messages.New(C4GM_Target, StdStrBuf{GetNeededMatStr(pBuilder).c_str()}, pBuilder->Section, pBuilder, pBuilder->Controller);
		}
		// Still in need: done/fail
		return false;
	}

	// Do con (mass- and builder-relative)
	int32_t iBuildSpeed = 100; C4PhysicalInfo *pPhys;
	if (pBuilder) if (pPhys = pBuilder->GetPhysical())
	{
		iBuildSpeed = pPhys->CanConstruct;
		if (!iBuildSpeed)
		{
			// shouldn't even have gotten here. Looks like the Clonk lost the ability to build recently
			return false;
		}
		if (iBuildSpeed <= 1) iBuildSpeed = 100;
	}
	DoCon(iLevel * iBuildSpeed * 150 / (Def->Mass ? Def->Mass : 1), false, fNeedMaterial);

	// TurnTo
	if (Def->BuildTurnTo != C4ID_None)
		ChangeDef(Def->BuildTurnTo);

	// Repair
	Damage = 0;

	// Done/success
	return true;
}

bool C4Object::Chop(C4Object *pByObject)
{
	// Valid check
	if (!Status || !Def || Contained || !(OCF & OCF_Chop))
		return false;
	// Chop
	if (!Tick10) DoDamage(+10, pByObject ? pByObject->Owner : NO_OWNER, C4FxCall_DmgChop);
	return true;
}

bool C4Object::Push(C4Fixed txdir, C4Fixed dforce, bool fStraighten)
{
	// Valid check
	if (!Status || !Def || Contained || !(OCF & OCF_Grab)) return false;
	// Grabbing okay, no pushing
	if (Def->Grab == 2) return true;
	// Mobilization check (pre-mobilization zero)
	if (!Mobile)
	{
		xdir = ydir = Fix0; fix_x = itofix(x); fix_y = itofix(y);
	}
	// General pushing force vs. object mass
	dforce = dforce * 100 / Mass;
	// Set dir
	if (xdir < 0) SetDir(DIR_Left);
	if (xdir > 0) SetDir(DIR_Right);
	// Work towards txdir
	if (Abs(xdir - txdir) <= dforce) // Close-enough-set
	{
		xdir = txdir;
	}
	else // Work towards
	{
		if (xdir < txdir) xdir += dforce;
		if (xdir > txdir) xdir -= dforce;
	}
	// Straighten
	if (fStraighten)
		if (Inside<int32_t>(r, -StableRange, +StableRange))
		{
			rdir = 0; // cheap way out
		}
		else
		{
			if (r > 0) { if (rdir > -RotateAccel) rdir -= dforce; }
			else { if (rdir < +RotateAccel) rdir += dforce; }
		}

	// Mobilization check
	if (!!xdir || !!ydir || !!rdir) Mobile = 1;

	// Stuck check
	if (!Tick35) if (txdir) if (!Def->NoHorizontalMove)
		if (ContactCheck(x, y)) // Resets t_contact
		{
			GameMsgObject(LoadResStr(C4ResStrTableKey::IDS_OBJ_STUCK, GetName()).c_str(), this);
			Call(PSF_Stuck);
		}

	return true;
}

bool C4Object::Lift(C4Fixed tydir, C4Fixed dforce)
{
	// Valid check
	if (!Status || !Def || Contained) return false;
	// Mobilization check
	if (!Mobile)
	{
		xdir = ydir = Fix0; fix_x = itofix(x); fix_y = itofix(y); Mobile = 1;
	}
	// General pushing force vs. object mass
	dforce = dforce * 100 / Mass;
	// If close enough, set tydir
	if (Abs(tydir - ydir) <= Abs(dforce))
		ydir = tydir;
	else // Work towards tydir
	{
		if (ydir < tydir) ydir += dforce;
		if (ydir > tydir) ydir -= dforce;
	}
	// Stuck check
	if (tydir != -Section->Landscape.Gravity)
		if (ContactCheck(x, y)) // Resets t_contact
		{
			GameMsgObject(LoadResStr(C4ResStrTableKey::IDS_OBJ_STUCK, GetName()).c_str(), this);
			Call(PSF_Stuck);
		}
	return true;
}

C4Object *C4Object::CreateContents(C4ID n_id)
{
	C4Object *nobj;
	if (!(nobj = Section->CreateObject(n_id, this, Owner))) return nullptr;
	if (!nobj->Enter(this)) { nobj->AssignRemoval(); return nullptr; }
	return nobj;
}

bool C4Object::CreateContentsByList(C4IDList &idlist)
{
	int32_t cnt, cnt2;
	for (cnt = 0; idlist.GetID(cnt); cnt++)
		for (cnt2 = 0; cnt2 < idlist.GetCount(cnt); cnt2++)
			if (!CreateContents(idlist.GetID(cnt)))
				return false;
	return true;
}

bool C4Object::ActivateMenu(int32_t iMenu, int32_t iMenuSelect,
	int32_t iMenuData, int32_t iMenuPosition,
	C4Object *pTarget)
{
	// Variables
	C4FacetExSurface fctSymbol;
	std::string caption;
	std::string command;
	int32_t cnt, iCount;
	C4Def *pDef;
	C4Player *pPlayer;
	C4IDList ListItems;
	// Close any other menu
	if (Menu && Menu->IsActive()) if (!Menu->TryClose(true, false)) return false;
	// Create menu
	if (!Menu) Menu = new C4ObjectMenu; else Menu->ClearItems(true);
	// Open menu
	switch (iMenu)
	{
	case C4MN_Activate:
		// No target specified: use own container as target
		if (!pTarget) if (!(pTarget = Contained)) break;
		// Opening contents menu blocked by RejectContents
		if (pTarget->Call(PSF_RejectContents)) return false;
		// Create symbol
		fctSymbol.Create(C4SymbolSize, C4SymbolSize);
		pTarget->Def->Draw(fctSymbol, false, pTarget->Color, pTarget);
		caption = LoadResStr(C4ResStrTableKey::IDS_OBJ_EMPTY, pTarget->GetName());
		// Init
		Menu->Init(fctSymbol, *Section, caption.c_str(), this, C4MN_Extra_None, 0, iMenu);
		Menu->SetPermanent(true);
		Menu->SetRefillObject(pTarget);
		// Success
		return true;

	case C4MN_Buy:
		// No target specified: container is base
		if (!pTarget) if (!(pTarget = Contained)) break;
		// Create symbol
		fctSymbol.Create(C4SymbolSize, C4SymbolSize);
		DrawMenuSymbol(C4MN_Buy, fctSymbol, pTarget->Owner, pTarget);
		// Init menu
		Menu->Init(fctSymbol, *Section, LoadResStr(C4ResStrTableKey::IDS_PLR_NOBUY), this, C4MN_Extra_Value, 0, iMenu);
		Menu->SetPermanent(true);
		Menu->SetRefillObject(pTarget);
		// Success
		return true;

	case C4MN_Sell:
		// No target specified: container is base
		if (!pTarget) if (!(pTarget = Contained)) break;
		// Create symbol & init
		fctSymbol.Create(C4SymbolSize, C4SymbolSize);
		DrawMenuSymbol(C4MN_Sell, fctSymbol, pTarget->Owner, pTarget);
		caption = LoadResStr(C4ResStrTableKey::IDS_OBJ_EMPTY, pTarget->GetName());
		Menu->Init(fctSymbol, *Section, caption.c_str(), this, C4MN_Extra_Value, 0, iMenu);
		Menu->SetPermanent(true);
		Menu->SetRefillObject(pTarget);
		// Success
		return true;

	case C4MN_Get:
	case C4MN_Contents:
		// No target specified
		if (!pTarget) break;
		// Opening contents menu blocked by RejectContents
		if (pTarget->Call(PSF_RejectContents)) return false;
		// Create symbol & init
		fctSymbol.Create(C4SymbolSize, C4SymbolSize);
		pTarget->Def->Draw(fctSymbol, false, pTarget->Color, pTarget);
		caption = LoadResStr(C4ResStrTableKey::IDS_OBJ_EMPTY, pTarget->GetName());
		Menu->Init(fctSymbol, *Section, caption.c_str(), this, C4MN_Extra_None, 0, iMenu);
		Menu->SetPermanent(true);
		Menu->SetRefillObject(pTarget);
		// Success
		return true;

	case C4MN_Context:
	{
		// Target by parameter
		if (!pTarget) break;

		// Create symbol & init menu
		pPlayer = Game.Players.Get(pTarget->Owner);
		fctSymbol.Create(C4SymbolSize, C4SymbolSize);
		pTarget->Def->Draw(fctSymbol, false, pTarget->Color, pTarget);
		Menu->Init(fctSymbol, *Section, pTarget->GetName(), this, C4MN_Extra_None, 0, iMenu, C4MN_Style_Context);

		Menu->SetPermanent(iMenuData);
		Menu->SetRefillObject(pTarget);

		// Preselect
		Menu->SetSelection(iMenuSelect, false, true);
		Menu->SetPosition(iMenuPosition);
	}
	// Success
	return true;

	case C4MN_Construction:
		// Check valid player
		if (!(pPlayer = Game.Players.Get(Owner))) break;
		// Create symbol
		fctSymbol.Create(C4SymbolSize, C4SymbolSize);
		DrawMenuSymbol(C4MN_Construction, fctSymbol, -1, nullptr);
		// Init menu
		Menu->Init(fctSymbol, *Section, LoadResStr(C4ResStrTableKey::IDS_PLR_NOBKNOW, pPlayer->GetName()).c_str(), this, C4MN_Extra_Components, 0, iMenu);
		// Add player's structure build knowledge
		for (cnt = 0; pDef = Game.Defs.ID2Def(pPlayer->Knowledge.GetID(Game.Defs, C4D_Structure, cnt, &iCount)); cnt++)
		{
			// Caption
			caption = LoadResStr(C4ResStrTableKey::IDS_MENU_CONSTRUCT, pDef->GetName());
			// Picture
			pDef->Picture2Facet(fctSymbol);
			// Command
			command = std::format("SetCommand(this, \"Construct\",,0,0,,{})", C4IdText(pDef->id));
			// Add menu item
			Menu->AddRefSym(caption.c_str(), fctSymbol, command.c_str(), C4MN_Item_NoCount, nullptr, pDef->GetDesc(), pDef->id);
		}
		// Preselect
		Menu->SetSelection(iMenuSelect, false, true);
		Menu->SetPosition(iMenuPosition);
		// Success
		return true;

	case C4MN_Info:
		// Target by parameter
		if (!pTarget) break;
		pPlayer = Game.Players.Get(pTarget->Owner);
		// Create symbol & init menu
		fctSymbol.Create(C4SymbolSize, C4SymbolSize); GfxR->fctOKCancel.Draw(fctSymbol, true, 0, 1);
		Menu->Init(fctSymbol, *Section, pTarget->GetName(), this, C4MN_Extra_None, 0, iMenu, C4MN_Style_Info);
		Menu->SetPermanent(true);
		Menu->SetAlignment(C4MN_Align_Free);
		C4Viewport *pViewport = Game.GraphicsSystem.GetViewport(Owner); // Hackhackhack!!!
		if (pViewport) Menu->SetLocation(pTarget->x + pTarget->Shape.x + pTarget->Shape.Wdt + 10 - pViewport->ViewX, pTarget->y + pTarget->Shape.y - pViewport->ViewY);

		const auto pictureSize = Application.GetScale() * C4PictureSize;

		// Add info item
		fctSymbol.Create(pictureSize, pictureSize); pTarget->Def->Draw(fctSymbol, false, pTarget->Color, pTarget);
		Menu->Add(pTarget->GetName(), fctSymbol, "", C4MN_Item_NoCount, nullptr, pTarget->GetInfoString().getData());
		fctSymbol.Default();
		// Success
		return true;
	}
	// Invalid menu identification
	CloseMenu(true);
	return false;
}

bool C4Object::CloseMenu(bool fForce)
{
	if (Menu)
	{
		if (Menu->IsActive()) if (!Menu->TryClose(fForce, false)) return false;
		if (!Menu->IsCloseQuerying()) { delete Menu; Menu = nullptr; } // protect menu deletion from recursive menu operation calls
	}
	return true;
}

void C4Object::AutoContextMenu(int32_t iMenuSelect)
{
	// Auto Context Menu - the "new structure menus"
	// No command set and no menu open
	if (!Command && !(Menu && Menu->IsActive()))
		// In a container with AutoContextMenu
		if (Contained && Contained->Def->AutoContextMenu)
			// Crew members only
			if (OCF & OCF_CrewMember)
			{
				// Player has AutoContextMenus enabled
				C4Player *pPlayer = Game.Players.Get(Controller);
				if (pPlayer && pPlayer->AutoContextMenu)
				{
					// Open context menu for structure
					ActivateMenu(C4MN_Context, iMenuSelect, 1, 0, Contained);
					// Closing the menu exits the building (all selected clonks)
					Menu->SetCloseCommand("PlayerObjectCommand(GetOwner(), \"Exit\") && ExecuteCommand()");
				}
			}
}

uint8_t C4Object::GetArea(int32_t &aX, int32_t &aY, int32_t &aWdt, int32_t &aHgt)
{
	if (!Status || !Def) return 0;
	aX = x + Shape.x; aY = y + Shape.y;
	aWdt = Shape.Wdt; aHgt = Shape.Hgt;
	return 1;
}

uint8_t C4Object::GetEntranceArea(int32_t &aX, int32_t &aY, int32_t &aWdt, int32_t &aHgt)
{
	if (!Status || !Def) return 0;
	// Return actual entrance
	if (OCF & OCF_Entrance)
	{
		aX = x + Def->Entrance.x;
		aY = y + Def->Entrance.y;
		aWdt = Def->Entrance.Wdt;
		aHgt = Def->Entrance.Hgt;
	}
	// Return object center
	else
	{
		aX = x; aY = y;
		aWdt = 0; aHgt = 0;
	}
	// Done
	return 1;
}

C4Fixed C4Object::GetSpeed()
{
	C4Fixed cobjspd = Fix0;
	if (xdir < 0) cobjspd -= xdir; else cobjspd += xdir;
	if (ydir < 0) cobjspd -= ydir; else cobjspd += ydir;
	return cobjspd;
}

const char *C4Object::GetName()
{
	if (!CustomName.empty()) return CustomName.c_str();
	if (Info) return Info->Name;
	return Def->Name.getData();
}

void C4Object::SetName(const char *NewName)
{
	if (!NewName)
		CustomName.clear();
	else
		CustomName = NewName;
}

int32_t C4Object::GetValue(C4Object *pInBase, int32_t iForPlayer)
{
	int32_t iValue;

	// value by script?
	if (C4AulScriptFunc *f = Def->Script.SFn_CalcValue)
		iValue = f->Exec(*Section, this, {C4VObj(pInBase), C4VInt(iForPlayer)}).getInt();
	else
	{
		// get value of def
		// Caution: Do not pass pInBase here, because the def base value is to be queried
		//  - and not the value if you had to buy the object in this particular base
		iValue = Def->GetValue(*Section, nullptr, iForPlayer);
	}
	// Con percentage
	iValue = iValue * Con / FullCon;
	// value adjustments buy base function
	if (pInBase)
	{
		C4AulFunc *pFn;
		if (pFn = pInBase->Def->Script.GetSFunc(PSF_CalcSellValue, AA_PROTECTED))
			iValue = pFn->Exec(*pInBase->Section, pInBase, {C4VObj(this), C4VInt(iValue)}).getInt();
	}
	// Return value
	return iValue;
}

C4PhysicalInfo *C4Object::GetPhysical(bool fPermanent)
{
	// Temporary physical
	if (PhysicalTemporary && !fPermanent) return &TemporaryPhysical;
	// Info physical: Available only if there's an info and it should be used
	if (Info)
		if (!Game.Parameters.UseFairCrew)
			return &(Info->Physical);
		else if (Info->pDef)
			return Info->pDef->GetFairCrewPhysicals(*Section);
		else
			// shouldn't really happen, but who knows.
			// Maybe some time it will be possible to have crew infos that aren't tied to a specific definition
			return Def->GetFairCrewPhysicals(*Section);
	// Definition physical
	return &(Def->Physical);
}

bool C4Object::TrainPhysical(C4PhysicalInfo::Offset mpiOffset, int32_t iTrainBy, int32_t iMaxTrain)
{
	int i = 0;
	// Train temp
	if (PhysicalTemporary) { TemporaryPhysical.Train(mpiOffset, iTrainBy, iMaxTrain); ++i; }
	// train permanent, if existent
	// this also trains if fair crew is used!
	if (Info) { Info->Physical.Train(mpiOffset, iTrainBy, iMaxTrain); ++i; }
	// return whether anything was trained
	return !!i;
}

bool C4Object::Promote(int32_t torank, bool fForceRankName)
{
	if (!Info) return false;
	// get rank system
	C4Def *pUseDef = Game.Defs.ID2Def(Info->id);
	C4RankSystem *pRankSys;
	if (pUseDef && pUseDef->pRankNames)
		pRankSys = pUseDef->pRankNames;
	else
		pRankSys = &Game.Rank;
	// always promote info
	Info->Promote(torank, *pRankSys, fForceRankName);
	// silent update?
	if (!pRankSys->GetRankName(torank, false)) return false;
	GameMsgObject(LoadResStr(C4ResStrTableKey::IDS_OBJ_PROMOTION, GetName(), Info->sRankName.getData()).c_str(), this);
	StartSoundEffect("Trumpet", 0, 100, this);
	return true;
}

void C4Object::ClearPointers(C4Object *pObj)
{
	// effects
	if (pEffects) pEffects->ClearPointers(pObj);
	// contents/contained: not necessary, because it's done in AssignRemoval and StatusDeactivate
	// Action targets
	if (Action.Target == pObj) Action.Target = nullptr;
	if (Action.Target2 == pObj) Action.Target2 = nullptr;
	// Commands
	C4Command *cCom;
	for (cCom = Command; cCom; cCom = cCom->Next)
		cCom->ClearPointers(pObj);
	// Menu
	if (Menu) Menu->ClearPointers(pObj);
	// Layer
	if (pLayer == pObj) pLayer = nullptr;
	// gfx overlays
	if (pGfxOverlay)
	{
		C4GraphicsOverlay *pNextGfxOvrl = pGfxOverlay, *pGfxOvrl;
		while (pGfxOvrl = pNextGfxOvrl)
		{
			pNextGfxOvrl = pGfxOvrl->GetNext();
			if (pGfxOvrl->GetOverlayObject() == pObj)
				// overlay relying on deleted object: Delete!
				RemoveGraphicsOverlay(pGfxOvrl->GetID());
		}
	}
}

void C4Object::OnSectionMove(C4Object *const obj, C4Section &newSection)
{
	if (Action.Target == obj || Action.Target2 == obj)
	{
		// attach target is moving to another section: move
		if ((GetProcedure() == DFA_ATTACH))
		{
			MoveToSection(newSection);
		}
		else
		{
			if (Action.Target == obj)
			{
				Action.Target = nullptr;
			}

			if (Action.Target2 == obj)
			{
				Action.Target2 = nullptr;
			}
		}
	}

	for (C4Command *command{Command}; command; command = command->Next)
	{
		command->OnSectionMove(obj);
	}
}

C4Value C4Object::Call(const char *szFunctionCall, const C4AulParSet &pPars, bool fPassError, bool convertNilToIntBool)
{
	if (!Status || !Def || !szFunctionCall[0]) return C4VNull;
	return Def->Script.ObjectCall(this, this, szFunctionCall, pPars, fPassError, convertNilToIntBool);
}

bool C4Object::SetPhase(int32_t iPhase)
{
	if (Action.Act <= ActIdle) return false;
	C4ActionDef *actdef = &(Def->ActMap[Action.Act]);
	Action.Phase = BoundBy<int32_t>(iPhase, 0, actdef->Length);
	return true;
}

void C4Object::Draw(C4FacetEx &cgo, int32_t iByPlayer, DrawMode eDrawMode)
{
	C4Facet ccgo;

	// Status
	if (!Status || !Def) return;

	// visible?
	if (Visibility || pLayer) if (!IsVisible(iByPlayer, !!eDrawMode)) return;

	// Line
	if (Def->Line) { DrawLine(cgo); return; }

	// background particles (bounds not checked)
	if (BackParticles && !Contained && eDrawMode != ODM_BaseOnly) BackParticles.Draw(cgo, this);

	// Object output position
	int32_t cotx = cgo.TargetX, coty = cgo.TargetY; if (eDrawMode != ODM_Overlay) TargetPos(cotx, coty, cgo);
	int32_t cox = x + Shape.x - cotx, coy = y + Shape.y - coty;

	bool fYStretchObject = false;
	if (Action.Act > ActIdle)
		if (Def->ActMap[Action.Act].FacetTargetStretch)
			fYStretchObject = true;

	// Set audibility - only for parallax objects, others calculate it on demand
	if (!eDrawMode && (Category & C4D_Parallax)) SetAudibilityAt(cgo, x, y);

	// Output boundary
	if (!fYStretchObject && !eDrawMode)
		if (Action.Act > ActIdle && !r && !Def->ActMap[Action.Act].FacetBase && Con <= FullCon)
		{
			// active
			if (!Inside(cox + Action.FacetX, 1 - Action.Facet.Wdt, cgo.Wdt)
				|| (!Inside(coy + Action.FacetY, 1 - Action.Facet.Hgt, cgo.Hgt)))
			{
				if (FrontParticles && !Contained) FrontParticles.Draw(cgo, this); return;
			}
		}
		else
			// idle
			if (!Inside(cox, 1 - Shape.Wdt, cgo.Wdt)
				|| (!Inside(coy, 1 - Shape.Hgt, cgo.Hgt)))
			{
				if (FrontParticles && !Contained) FrontParticles.Draw(cgo, this); return;
			}

	// ensure correct color is set
	if (GetGraphics()->BitmapClr) GetGraphics()->BitmapClr->SetClr(Color);

	// Debug Display
	if (Game.GraphicsSystem.ShowCommand && !eDrawMode)
	{
		C4Command *pCom;
		int32_t ccx = x, ccy = y;
		int32_t x1, y1, x2, y2;
		std::string command;
		std::string commands;
		int32_t iMoveTos = 0;
		for (pCom = Command; pCom; pCom = pCom->Next)
		{
			switch (pCom->Command)
			{
			case C4CMD_MoveTo:
				// Angle
				int32_t iAngle; iAngle = Angle(ccx, ccy, pCom->Tx._getInt(), pCom->Ty); while (iAngle > 180) iAngle -= 360;
				// Path
				x1 = ccx - cotx; y1 = ccy - coty;
				x2 = pCom->Tx._getInt() - cotx; y2 = pCom->Ty - coty;
				Application.DDraw->DrawLine(cgo.Surface, cgo.X + x1, cgo.Y + y1, cgo.X + x2, cgo.Y + y2, CRed);
				Application.DDraw->DrawFrame(cgo.Surface, cgo.X + x2 - 1, cgo.Y + y2 - 1, cgo.X + x2 + 1, cgo.Y + y2 + 1, CRed);
				if (x1 > x2) std::swap(x1, x2); if (y1 > y2) std::swap(y1, y2);
				ccx = pCom->Tx._getInt(); ccy = pCom->Ty;
				// Message
				iMoveTos++;
				command.clear();
				break;
			case C4CMD_Put:
				command = std::format("{} {} to {}", CommandName(pCom->Command), pCom->Target2 ? pCom->Target2->GetName() : pCom->Data ? C4IdText(pCom->Data) : "Content", pCom->Target ? pCom->Target->GetName() : "");
				break;
			case C4CMD_Buy: case C4CMD_Sell:
				command = std::format("{} {} at {}", CommandName(pCom->Command), C4IdText(pCom->Data), pCom->Target ? pCom->Target->GetName() : "closest base");
				break;
			case C4CMD_Acquire:
				command = std::format("{} {}", CommandName(pCom->Command), pCom->Target ? pCom->Target->GetName() : "");
				break;
			case C4CMD_Call:
				command = std::format("{} {} in {}", CommandName(pCom->Command), pCom->Text, pCom->Target ? pCom->Target->GetName() : "(null)");
				break;
			case C4CMD_Construct:
				C4Def *pDef; pDef = Game.Defs.ID2Def(pCom->Data);
				command = std::format("{} {}", CommandName(pCom->Command), pDef ? pDef->GetName() : "");
				break;
			case C4CMD_None:
				command.clear();
				break;
			case C4CMD_Transfer:
				// Path
				x1 = ccx - cotx; y1 = ccy - coty;
				x2 = pCom->Tx._getInt() - cotx; y2 = pCom->Ty - coty;
				Application.DDraw->DrawLine(cgo.Surface, cgo.X + x1, cgo.Y + y1, cgo.X + x2, cgo.Y + y2, CGreen);
				Application.DDraw->DrawFrame(cgo.Surface, cgo.X + x2 - 1, cgo.Y + y2 - 1, cgo.X + x2 + 1, cgo.Y + y2 + 1, CGreen);
				if (x1 > x2) std::swap(x1, x2); if (y1 > y2) std::swap(y1, y2);
				ccx = pCom->Tx._getInt(); ccy = pCom->Ty;
				// Message
				command = std::format("{} {}", CommandName(pCom->Command), pCom->Target ? pCom->Target->GetName() : "");
				break;
			default:
				command = std::format("{} {}", CommandName(pCom->Command), pCom->Target ? pCom->Target->GetName() : "");
				break;
			}
			// Compose command stack message
			if (!command.empty())
			{
				// End MoveTo stack first
				if (iMoveTos) { commands += std::format("|{}x MoveTo", std::exchange(iMoveTos, 0)); }
				// Current message
				commands += '|';
				if (pCom->Finished) commands += "<i>";
				commands += std::move(command);
				if (pCom->Finished) commands += "</i>";
			}
		}
		// Open MoveTo stack
		if (iMoveTos) { commands += std::format("|{}x MoveTo", std::exchange(iMoveTos, 0)); }
		// Draw message
		int32_t cmwdt, cmhgt; Game.GraphicsResource.FontRegular.GetTextExtent(commands.c_str(), cmwdt, cmhgt, true);
		Application.DDraw->TextOut(commands.c_str(), Game.GraphicsResource.FontRegular, 1.0, cgo.Surface, cgo.X + cox - Shape.x, cgo.Y + coy - 10 - cmhgt, CStdDDraw::DEFAULT_MESSAGE_COLOR, ACenter);
	}

	// Don't draw (show solidmask)
	if (Game.GraphicsSystem.ShowSolidMask)
		if (SolidMask.Wdt)
		{
			// no need to draw it, because the 8bit-surface will be shown
			return;
		}

	// Contained check
	if (Contained && !eDrawMode) return;

	// Visibility inside FoW
	bool fOldClrModEnabled = !!(Category & C4D_IgnoreFoW);
	if (fOldClrModEnabled)
	{
		fOldClrModEnabled = lpDDraw->GetClrModMapEnabled();
		lpDDraw->SetClrModMapEnabled(false);
	}

	// Fire facet - always draw, even if particles are drawn as well
	if (OnFire) if (eDrawMode != ODM_BaseOnly)
	{
		C4Facet fgo;
		// Straight: Full Shape.Rect on fire
		if (r == 0)
		{
			fgo.Set(cgo.Surface, cgo.X + cox, cgo.Y + coy,
				Shape.Wdt, Shape.Hgt - Shape.FireTop);
		}
		// Rotated: Reduced fire rect
		else
		{
			C4Rect fr;
			Shape.GetVertexOutline(fr);
			fgo.Set(cgo.Surface,
				cgo.X + cox - Shape.x + fr.x,
				cgo.Y + coy - Shape.y + fr.y,
				fr.Wdt, fr.Hgt);
		}
		Game.GraphicsResource.fctFire.Draw(fgo, false, FirePhase);
	}

	// color modulation (including construction sign...)
	if (ColorMod || BlitMode) if (!eDrawMode) PrepareDrawing();

	// Not active or rotated: BaseFace only
	if ((Action.Act <= ActIdle))
	{
		DrawFace(cgo, cgo.X + cox, cgo.Y + coy);
	}

	// Active
	else
	{
		// FacetBase
		if (Def->ActMap[Action.Act].FacetBase)
			DrawFace(cgo, cgo.X + cox, cgo.Y + coy, 0, Action.DrawDir);

		// Facet
		if (Action.Facet.Surface)
		{
			// Special: stretched action facet
			if (Def->ActMap[Action.Act].FacetTargetStretch)
			{
				if (Action.Target)
					Action.Facet.DrawX(cgo.Surface,
						cgo.X + cox + Action.FacetX,
						cgo.Y + coy + Action.FacetY,
						Action.Facet.Wdt,
						(Action.Target->y + Action.Target->Shape.y) - (y + Shape.y + Action.FacetY),
						0, 0, GetGraphics()->pDef->Scale);
			}
			// Regular action facet
			else
			{
				// Calculate graphics phase index
				int32_t iPhase = Action.Phase;
				if (Def->ActMap[Action.Act].Reverse) iPhase = Def->ActMap[Action.Act].Length - 1 - Action.Phase;
				if (r != 0 && Def->Rotateable)
				{
					// newgfx: draw rotated actions as well
					if (Def->GrowthType || Con == FullCon)
					{
						// rotate midpoint of action facet around center of shape
						// combine with existing transform if necessary
						C4DrawTransform rot;
						if (pDrawTransform)
						{
							rot.SetTransformAt(*pDrawTransform, cgo.X + cox + float(Shape.Wdt) / 2, cgo.Y + coy + float(Shape.Hgt) / 2);
							rot.Rotate(r * 100, float(cgo.X + cox + Shape.Wdt / 2), float(cgo.Y + coy + Shape.Hgt / 2));
						}
						else
							rot.SetRotate(r * 100, float(cgo.X + cox + Shape.Wdt / 2), float(cgo.Y + coy + Shape.Hgt / 2));
						// draw stretched towards shape center with transform
						Action.Facet.DrawXT(cgo.Surface,
							(Def->Shape.x + Action.FacetX) * Con / FullCon + cgo.X + cox - Shape.x,
							(Def->Shape.y + Action.FacetY) * Con / FullCon + cgo.Y + coy - Shape.y,
							Action.Facet.Wdt * Con / FullCon, Action.Facet.Hgt * Con / FullCon,
							iPhase, Action.DrawDir, &rot,
							false, GetGraphics()->pDef->Scale);
					}
					else
					{
						// incomplete constructions do not show actions
						DrawFace(cgo, cgo.X + cox, cgo.Y + coy);
					}
				}
				else
				{
					// Exact fullcon
					if (Con == FullCon)
						Action.Facet.DrawT(cgo.Surface,
							cgo.X + cox + Action.FacetX,
							cgo.Y + coy + Action.FacetY,
							iPhase, Action.DrawDir,
							pDrawTransform ? &C4DrawTransform(*pDrawTransform, static_cast<float>(Shape.Wdt) / 2 + cgo.X + cox, static_cast<float>(Shape.Hgt) / 2 + cgo.Y + coy) : nullptr,
							false, GetGraphics()->pDef->Scale);
					// Growth strechted
					else
						Action.Facet.DrawXT(cgo.Surface,
							cgo.X + cox, cgo.Y + coy,
							Shape.Wdt, Shape.Hgt,
							iPhase, Action.DrawDir,
							pDrawTransform ? &C4DrawTransform(*pDrawTransform, static_cast<float>(Shape.Wdt) / 2 + cgo.X + cox, static_cast<float>(Shape.Hgt) / 2 + cgo.Y + coy) : nullptr,
							false, GetGraphics()->pDef->Scale);
				}
			}
		}
	}

	// end of color modulation
	if (ColorMod || BlitMode) if (!eDrawMode) FinishedDrawing();

	// draw overlays - after blit mode changes, because overlay gfx set their own
	if (pGfxOverlay) if (eDrawMode != ODM_BaseOnly)
		for (C4GraphicsOverlay *pGfxOvrl = pGfxOverlay; pGfxOvrl; pGfxOvrl = pGfxOvrl->GetNext())
			if (!pGfxOvrl->IsPicture())
				pGfxOvrl->Draw(cgo, this, iByPlayer);

	// local particles in front of the object
	if (FrontParticles) if (eDrawMode != ODM_BaseOnly) FrontParticles.Draw(cgo, this);

	// Select Mark
	if (Select)
		if (eDrawMode != ODM_BaseOnly)
			if (ValidPlr(Owner))
				if (Owner == iByPlayer)
					if (Game.Players.Get(Owner)->SelectFlash)
						DrawSelectMark(cgo);

	// Energy shortage
	if (NeedEnergy) if (Tick35 > 12) if (eDrawMode != ODM_BaseOnly)
	{
		C4Facet &fctEnergy = Game.GraphicsResource.fctEnergy;
		int32_t tx = cox + Shape.Wdt / 2 - fctEnergy.Wdt / 2, ty = coy - fctEnergy.Hgt - 5;
		fctEnergy.Draw(cgo.Surface, cgo.X + tx, cgo.Y + ty);
	}

	// Debug Display
	if (Game.GraphicsSystem.ShowVertices) if (eDrawMode != ODM_BaseOnly)
	{
		int32_t cnt;
		if (Shape.VtxNum > 1)
			for (cnt = 0; cnt < Shape.VtxNum; cnt++)
			{
				DrawVertex(cgo,
					cox - Shape.x + Shape.VtxX[cnt],
					coy - Shape.y + Shape.VtxY[cnt],
					(Shape.VtxCNAT[cnt] & CNAT_NoCollision) ? CBlue : (Mobile ? CRed : CYellow),
					Shape.VtxContactCNAT[cnt]);
			}
	}

	if (Game.GraphicsSystem.ShowEntrance) if (eDrawMode != ODM_BaseOnly)
	{
		if (OCF & OCF_Entrance)
			Application.DDraw->DrawFrame(cgo.Surface, cgo.X + cox - Shape.x + Def->Entrance.x,
				cgo.Y + coy - Shape.y + Def->Entrance.y,
				cgo.X + cox - Shape.x + Def->Entrance.x + Def->Entrance.Wdt - 1,
				cgo.Y + coy - Shape.y + Def->Entrance.y + Def->Entrance.Hgt - 1,
				CBlue);
		if (OCF & OCF_Collection)
			Application.DDraw->DrawFrame(cgo.Surface, cgo.X + cox - Shape.x + Def->Collection.x,
				cgo.Y + coy - Shape.y + Def->Collection.y,
				cgo.X + cox - Shape.x + Def->Collection.x + Def->Collection.Wdt - 1,
				cgo.Y + coy - Shape.y + Def->Collection.y + Def->Collection.Hgt - 1,
				CRed);
	}

	if (Game.GraphicsSystem.ShowAction) if (eDrawMode != ODM_BaseOnly)
	{
		if (Action.Act > ActIdle)
		{
			const std::string message{std::format("{} ({})", Def->ActMap[Action.Act].Name, Action.Phase)};
			int32_t cmwdt, cmhgt;
			Game.GraphicsResource.FontRegular.GetTextExtent(message.c_str(), cmwdt, cmhgt, true);
			Application.DDraw->TextOut(message.c_str(), Game.GraphicsResource.FontRegular, 1.0, cgo.Surface, cgo.X + cox - Shape.x, cgo.Y + coy - cmhgt, InLiquid ? 0xfa0000FF : CStdDDraw::DEFAULT_MESSAGE_COLOR, ACenter);
		}
	}

	// Restore visibility inside FoW
	if (fOldClrModEnabled) lpDDraw->SetClrModMapEnabled(fOldClrModEnabled);
}

void C4Object::DrawTopFace(C4FacetEx &cgo, int32_t iByPlayer, DrawMode eDrawMode)
{
	// Status
	if (!Status || !Def) return;
	// visible?
	if (Visibility) if (!IsVisible(iByPlayer, eDrawMode == ODM_Overlay)) return;
	// target pos (parallax)
	int32_t cotx = cgo.TargetX, coty = cgo.TargetY; if (eDrawMode != ODM_Overlay) TargetPos(cotx, coty, cgo);
	// Clonk name
	// Name of Owner/Clonk (only when Crew Member; never in films)
	if (OCF & OCF_CrewMember) if ((Config.Graphics.ShowCrewNames || Config.Graphics.ShowCrewCNames) && (!Game.C4S.Head.Film || !Game.C4S.Head.Replay)) if (!eDrawMode)
		if (Owner != iByPlayer && !Contained)
		{
			// inside screen range?
			if (!Inside(x + Shape.x - cotx, 1 - Shape.Wdt, cgo.Wdt)
				|| !Inside(y + Shape.y - coty, 1 - Shape.Hgt, cgo.Hgt)) return;
			// get player
			C4Player *pOwner = Game.Players.Get(Owner);
			if (pOwner) if (!Hostile(Owner, iByPlayer)) if (!pOwner->IsInvisible())
			{
				int32_t X = x;
				int32_t Y = y - Def->Shape.Hgt / 2 - 20;
				// compose string
				char szText[C4GM_MaxText + 1];
				if (Config.Graphics.ShowCrewNames)
					if (Config.Graphics.ShowCrewCNames)
						FormatWithNull(szText, "{} ({})", GetName(), pOwner->GetName());
					else
						SCopy(pOwner->GetName(), szText);
				else
					SCopy(GetName(), szText);
				// Word wrap to cgo width
				int32_t iCharWdt, dummy; Game.GraphicsResource.FontRegular.GetTextExtent("m", iCharWdt, dummy, false);
				int32_t iMaxLine = std::max<int32_t>(cgo.Wdt / iCharWdt, 20);
				SWordWrap(szText, ' ', '|', iMaxLine);
				// Adjust position by output boundaries
				int32_t iTX, iTY, iTWdt, iTHgt;
				Game.GraphicsResource.FontRegular.GetTextExtent(szText, iTWdt, iTHgt, true);
				iTX = BoundBy<int32_t>(X - cotx, iTWdt / 2, cgo.Wdt - iTWdt / 2);
				iTY = BoundBy<int32_t>(Y - coty - iTHgt, 0, cgo.Hgt - iTHgt);
				// Draw
				Application.DDraw->TextOut(szText, Game.GraphicsResource.FontRegular, 1.0, cgo.Surface, cgo.X + iTX, cgo.Y + iTY,
					pOwner->ColorDw | 0x7f000000, ACenter);
			}
		}
	// TopFace
	if (!(TopFace.Surface || (OCF & OCF_Construct))) return;
	// Output position
	int32_t cox, coy;
	cox = x + Shape.x - cotx;
	coy = y + Shape.y - coty;
	// Output bounds check
	if (!Inside(cox, 1 - Shape.Wdt, cgo.Wdt)
		|| !Inside(coy, 1 - Shape.Hgt, cgo.Hgt))
		return;
	// Don't draw (show solidmask)
	if (Game.GraphicsSystem.ShowSolidMask)
		if (SolidMask.Wdt)
			return;
	// Contained
	if (Contained) if (eDrawMode != ODM_Overlay) return;
	// Construction sign
	if (OCF & OCF_Construct) if (r == 0) if (eDrawMode != ODM_BaseOnly)
	{
		C4Facet &fctConSign = Game.GraphicsResource.fctConstruction;
		fctConSign.Draw(cgo.Surface, cgo.X + cox, cgo.Y + coy + Shape.Hgt - fctConSign.Hgt);
	}
	// FacetTopFace: Override TopFace.x/y
	if ((Action.Act > ActIdle) && Def->ActMap[Action.Act].FacetTopFace)
	{
		C4ActionDef *actdef = &Def->ActMap[Action.Act];
		int32_t iPhase = Action.Phase;
		if (actdef->Reverse) iPhase = actdef->Length - 1 - Action.Phase;
		TopFace.X = actdef->Facet.x + Def->TopFace.x + actdef->Facet.Wdt * iPhase;
		TopFace.Y = actdef->Facet.y + Def->TopFace.y + actdef->Facet.Hgt * Action.DrawDir;
	}
	// ensure correct color is set
	if (GetGraphics()->BitmapClr) GetGraphics()->BitmapClr->SetClr(Color);
	// color modulation (including construction sign...)
	if (!eDrawMode) PrepareDrawing();
	// Draw top face bitmap
	if (Con != FullCon && Def->GrowthType)
		// stretched
		TopFace.DrawXT(cgo.Surface,
			cgo.X + cox + Def->TopFace.tx * Con / FullCon,
			cgo.Y + coy + Def->TopFace.ty * Con / FullCon,
			TopFace.Wdt * Con / FullCon,
			TopFace.Hgt * Con / FullCon,
			0, 0,
			pDrawTransform ? &C4DrawTransform(*pDrawTransform, cgo.X + cox + float(Shape.Wdt) / 2, cgo.Y + coy + float(Shape.Hgt) / 2) : nullptr,
			false, GetGraphics()->pDef->Scale);
	else
		// normal
		TopFace.DrawT(cgo.Surface,
			cgo.X + cox + Def->TopFace.tx,
			cgo.Y + coy + Def->TopFace.ty,
			0, 0,
			pDrawTransform ? &C4DrawTransform(*pDrawTransform, cgo.X + cox + float(Shape.Wdt) / 2, cgo.Y + coy + float(Shape.Hgt) / 2) : nullptr,
			false, GetGraphics()->pDef->Scale);
	// end of color modulation
	if (!eDrawMode) FinishedDrawing();
}

void C4Object::DrawLine(C4FacetEx &cgo)
{
	// Audibility
	SetAudibilityAt(cgo, Shape.VtxX[0], Shape.VtxY[0]);
	SetAudibilityAt(cgo, Shape.VtxX[Shape.VtxNum - 1], Shape.VtxY[Shape.VtxNum - 1]);
	// additive mode?
	PrepareDrawing();
	// Draw line segments
	for (int32_t vtx = 0; vtx + 1 < Shape.VtxNum; vtx++)
		switch (Def->Line)
		{
		case C4D_Line_Power:
			cgo.DrawLine(Shape.VtxX[vtx], Shape.VtxY[vtx],
				Shape.VtxX[vtx + 1], Shape.VtxY[vtx + 1],
				68, 26);
			break;
		case C4D_Line_Source: case C4D_Line_Drain:
			cgo.DrawLine(Shape.VtxX[vtx], Shape.VtxY[vtx],
				Shape.VtxX[vtx + 1], Shape.VtxY[vtx + 1],
				23, 26);
			break;
		case C4D_Line_Lightning:
			cgo.DrawBolt(Shape.VtxX[vtx], Shape.VtxY[vtx],
				Shape.VtxX[vtx + 1], Shape.VtxY[vtx + 1],
				CWhite, CWhite);
			break;
		case C4D_Line_Rope:
			cgo.DrawLine(Shape.VtxX[vtx], Shape.VtxY[vtx],
				Shape.VtxX[vtx + 1], Shape.VtxY[vtx + 1],
				65, 65);
			break;
		case C4D_Line_Vertex:
		case C4D_Line_Colored:
			cgo.DrawLine(Shape.VtxX[vtx], Shape.VtxY[vtx],
				Shape.VtxX[vtx + 1], Shape.VtxY[vtx + 1],
				uint8_t(Local[0].getInt()), uint8_t(Local[1].getInt()));
			break;
		}
	// reset blit mode
	FinishedDrawing();
}

void C4Object::DrawEnergy(C4Facet &cgo)
{
	cgo.DrawEnergyLevelEx(Energy, GetPhysical()->Energy, Game.GraphicsResource.fctEnergyBars, 0);
}

void C4Object::DrawMagicEnergy(C4Facet &cgo)
{
	// draw in units of MagicPhysicalFactor, so you can get a full magic energy bar by script even if partial magic energy training is not fulfilled
	cgo.DrawEnergyLevelEx(MagicEnergy / MagicPhysicalFactor, GetPhysical()->Magic / MagicPhysicalFactor, Game.GraphicsResource.fctEnergyBars, 1);
}

void C4Object::DrawBreath(C4Facet &cgo)
{
	cgo.DrawEnergyLevelEx(Breath, GetPhysical()->Breath, Game.GraphicsResource.fctEnergyBars, 2);
}

void C4Object::CompileFunc(StdCompiler *pComp)
{
	bool fCompiler = pComp->isCompiler();
	if (fCompiler)
		Clear();

	// Compile ID, search definition
	pComp->Value(mkNamingAdapt(mkC4IDAdapt(id), "id", C4ID_None));
	if (fCompiler)
	{
		Def = Game.Defs.ID2Def(id);
		if (!Def)
		{
			pComp->excNotFound(LoadResStr(C4ResStrTableKey::IDS_PRC_UNDEFINEDOBJECT, C4IdText(id))); return;
		}
	}

	// Write the name only if the object has an individual name, use def name as default for reading.
	// (Info may overwrite later, see C4Player::MakeCrewMember)
	if (pComp->isCompiler())
	{
		pComp->Value(mkNamingAdapt(CustomName, "Name", std::string{}));
	}
	else if (!CustomName.empty())
		// Write the name only if the object has an individual name
		// 2do: And what about binary compilers?
		pComp->Value(mkNamingAdapt(CustomName, "Name"));

	pComp->Value(mkNamingAdapt(Number,                                  "Number",             -1));
	pComp->Value(mkNamingAdapt(Status,                                  "Status",             1));
	pComp->Value(mkNamingAdapt(toC4CStrBuf(nInfo),                      "Info",               ""));
	pComp->Value(mkNamingAdapt(Owner,                                   "Owner",              NO_OWNER));
	pComp->Value(mkNamingAdapt(Timer,                                   "Timer",              0));
	pComp->Value(mkNamingAdapt(Controller,                              "Controller",         NO_OWNER));
	pComp->Value(mkNamingAdapt(LastEnergyLossCausePlayer,               "LastEngLossPlr",     NO_OWNER));
	pComp->Value(mkNamingAdapt(Category,                                "Category",           0));
	pComp->Value(mkNamingAdapt(x,                                       "X",                  0));
	pComp->Value(mkNamingAdapt(y,                                       "Y",                  0));
	pComp->Value(mkNamingAdapt(r,                                       "Rotation",           0));
	pComp->Value(mkNamingAdapt(motion_x,                                "MotionX",            0));
	pComp->Value(mkNamingAdapt(motion_y,                                "MotionY",            0));
	pComp->Value(mkNamingAdapt(iLastAttachMovementFrame,                "LastSolidAtchFrame", -1));
	pComp->Value(mkNamingAdapt(NoCollectDelay,                          "NoCollectDelay",     0));
	pComp->Value(mkNamingAdapt(Base,                                    "Base",               NO_OWNER));
	pComp->Value(mkNamingAdapt(Con,                                     "Size",               0));
	pComp->Value(mkNamingAdapt(OwnMass,                                 "OwnMass",            0));
	pComp->Value(mkNamingAdapt(Mass,                                    "Mass",               0));
	pComp->Value(mkNamingAdapt(Damage,                                  "Damage",             0));
	pComp->Value(mkNamingAdapt(Energy,                                  "Energy",             0));
	pComp->Value(mkNamingAdapt(MagicEnergy,                             "MagicEnergy",        0));
	pComp->Value(mkNamingAdapt(Alive,                                   "Alive",              false));
	pComp->Value(mkNamingAdapt(Breath,                                  "Breath",             0));
	pComp->Value(mkNamingAdapt(FirePhase,                               "FirePhase",          0));
	pComp->Value(mkNamingAdapt(Color,                                   "Color",              0u)); // TODO: Convert
	pComp->Value(mkNamingAdapt(Color,                                   "ColorDw",            0u));
	pComp->Value(mkNamingAdapt(Local,                                   "Locals",             C4ValueList()));
	pComp->Value(mkNamingAdapt(fix_x,                                   "FixX",               0));
	pComp->Value(mkNamingAdapt(fix_y,                                   "FixY",               0));
	pComp->Value(mkNamingAdapt(fix_r,                                   "FixR",               0));
	pComp->Value(mkNamingAdapt(xdir,                                    "XDir",               0));
	pComp->Value(mkNamingAdapt(ydir,                                    "YDir",               0));
	pComp->Value(mkNamingAdapt(rdir,                                    "RDir",               0));
	pComp->Value(mkParAdapt(Shape, true));
	pComp->Value(mkNamingAdapt(fOwnVertices,                            "OwnVertices",        false));
	pComp->Value(mkNamingAdapt(SolidMask,                               "SolidMask",          Def->SolidMask));
	pComp->Value(mkNamingAdapt(PictureRect,                             "Picture"));
	pComp->Value(mkNamingAdapt(Mobile,                                  "Mobile",             false));
	pComp->Value(mkNamingAdapt(Select,                                  "Selected",           false));
	pComp->Value(mkNamingAdapt(OnFire,                                  "OnFire",             false));
	pComp->Value(mkNamingAdapt(InLiquid,                                "InLiquid",           false));
	pComp->Value(mkNamingAdapt(EntranceStatus,                          "EntranceStatus",     false));
	pComp->Value(mkNamingAdapt(PhysicalTemporary,                       "PhysicalTemporary",  false));
	pComp->Value(mkNamingAdapt(NeedEnergy,                              "NeedEnergy",         false));
	pComp->Value(mkNamingAdapt(OCF,                                     "OCF",                0u));
	pComp->Value(Action);
	pComp->Value(mkNamingAdapt(Contained,                               "Contained",          C4EnumeratedObjectPtr{}));
	pComp->Value(mkNamingAdapt(Action.Target,                           "ActionTarget1",      C4EnumeratedObjectPtr{}));
	pComp->Value(mkNamingAdapt(Action.Target2,                          "ActionTarget2",      C4EnumeratedObjectPtr{}));
	pComp->Value(mkNamingAdapt(Component,                               "Component"));
	pComp->Value(mkNamingAdapt(Contents,                                "Contents"));
	pComp->Value(mkNamingAdapt(PlrViewRange,                            "PlrViewRange",       0));
	pComp->Value(mkNamingAdapt(Visibility,                              "Visibility",         VIS_All));
	pComp->Value(mkNamingAdapt(LocalNamed,                              "LocalNamed"));
	pComp->Value(mkNamingAdapt(ColorMod,                                "ColorMod",           0u));
	pComp->Value(mkNamingAdapt(BlitMode,                                "BlitMode",           0u));
	pComp->Value(mkNamingAdapt(CrewDisabled,                            "CrewDisabled",       false));
	pComp->Value(mkNamingAdapt(pLayer,                                  "Layer",              C4EnumeratedObjectPtr{}));
	pComp->Value(mkNamingAdapt(C4DefGraphicsAdapt(pGraphics),           "Graphics",           &Def->Graphics));
	pComp->Value(mkNamingPtrAdapt(pDrawTransform,                       "DrawTransform"));
	pComp->Value(mkNamingPtrAdapt(pEffects,                             "Effects"));
	pComp->Value(mkNamingAdapt(C4GraphicsOverlayListAdapt(pGfxOverlay), "GfxOverlay",         nullptr));

	if (PhysicalTemporary)
	{
		pComp->FollowName("Physical");
		pComp->Value(TemporaryPhysical);
	}

	// Commands
	if (pComp->FollowName("Commands"))
		if (fCompiler)
		{
			C4Command *pCmd = nullptr;
			for (int i = 1; ; i++)
			{
				// Every command has its own naming environment
				const std::string naming{std::format("Command{}", i)};
				pComp->Value(mkNamingPtrAdapt(pCmd ? pCmd->Next : Command, naming.c_str()));
				// Last command?
				pCmd = (pCmd ? pCmd->Next : Command);
				if (!pCmd)
					break;
				pCmd->cObj = this;
			}
		}
		else
		{
			C4Command *pCmd = Command;
			for (int i = 1; pCmd; i++, pCmd = pCmd->Next)
			{
				const std::string naming{std::format("Command{}", i)};
				pComp->Value(mkNamingAdapt(*pCmd, naming.c_str()));
			}
		}
}

void C4Object::PostCompileInit()
{
	// add to def count
	Def->Count++;

	// set local variable names
	LocalNamed.SetNameList(&Def->Script.LocalNamed);

	// Set action (override running data)
	int32_t iTime = Action.Time;
	int32_t iPhase = Action.Phase;
	int32_t iPhaseDelay = Action.PhaseDelay;
	if (SetActionByName(Action.Name, nullptr, nullptr, false))
	{
		Action.Time = iTime;
		Action.Phase = iPhase; // No checking for valid phase
		Action.PhaseDelay = iPhaseDelay;
	}

	// if on fire but no effect is present (old-style savegames), re-incinerate
	int32_t iFireNumber;
	C4Value Par1, Par2, Par3, Par4;
	if (OnFire && !pEffects) new C4Effect(*Section, this, C4Fx_Fire, C4Fx_FirePriority, C4Fx_FireTimer, nullptr, 0, Par1, Par2, Par3, Par4, false, iFireNumber);

	// blit mode not assigned? use definition default then
	if (!BlitMode) BlitMode = Def->BlitMode;

	// object needs to be resorted? May happen if there's unsorted objects in savegame
	if (Unsorted) Section->ResortAnyObject = true;

	// initial OCF update
	SetOCF();
}

void C4Object::EnumeratePointers()
{
	EnumerateObjectPtrs(Contained, Action.Target, Action.Target2, pLayer);

	// Info by name
	if (Info) nInfo = Info->Name; else nInfo.Clear();

	// Commands
	for (C4Command *pCom = Command; pCom; pCom = pCom->Next)
		pCom->EnumeratePointers();

	// effects
	if (pEffects) pEffects->EnumeratePointers();

	// gfx overlays
	if (pGfxOverlay)
		for (C4GraphicsOverlay *pGfxOvrl = pGfxOverlay; pGfxOvrl; pGfxOvrl = pGfxOvrl->GetNext())
			pGfxOvrl->EnumeratePointers();
}

void C4Object::DenumeratePointers(const bool onlyFromObjectSection)
{
	Contained.Denumerate(Section);
	Action.Target.Denumerate(Section);
	Action.Target2.Denumerate(Section);
	pLayer.Denumerate(Section);

	// Post-compile object list
	Contents.DenumerateRead(Section);

	// Local variable pointers
	Local.DenumeratePointers(Section);
	LocalNamed.DenumeratePointers(Section);

	// Commands
	for (C4Command *pCom = Command; pCom; pCom = pCom->Next)
		pCom->DenumeratePointers(*Section);

	// effects
	if (pEffects) pEffects->DenumeratePointers(onlyFromObjectSection);

	// gfx overlays
	if (pGfxOverlay)
		for (C4GraphicsOverlay *pGfxOvrl = pGfxOverlay; pGfxOvrl; pGfxOvrl = pGfxOvrl->GetNext())
			pGfxOvrl->DenumeratePointers(*Section);
}

bool DrawCommandQuery(int32_t controller, C4ScriptHost &scripthost, int32_t *mask, int com)
{
	int method = scripthost.GetControlMethod(com, mask[0], mask[1]);
	C4Player *player = Game.Players.Get(controller);
	if (!player) return false;

	switch (method)
	{
	case C4AUL_ControlMethod_All: return true;
	case C4AUL_ControlMethod_None: return false;
	case C4AUL_ControlMethod_Classic: return !player->ControlStyle;
	case C4AUL_ControlMethod_JumpAndRun: return !!player->ControlStyle;
	default: return false;
	}
}

void C4Object::DrawCommands(C4Facet &cgoBottom, C4Facet &cgoSide, C4RegionList *pRegions)
{
	int32_t cnt;
	C4Facet ccgo, ccgo2;
	C4Object *tObj;
	int32_t iDFA = GetProcedure();
	bool fContainedDownOverride = false;
	bool fContainedLeftOverride = false; // carlo
	bool fContainedRightOverride = false; // carlo
	bool fContentsActivationOverride = false;

	// Active menu (does not consider owner's active player menu)
	if (Menu && Menu->IsActive()) return;

	uint32_t ocf = OCF_Construct;
	if (Action.ComDir == COMD_Stop && iDFA == DFA_WALK && (tObj = Section->Objects.AtObject(x, y, ocf, this)))
	{
		int32_t com = COM_Down_D;
		if (Game.Players.Get(Controller)->ControlStyle) com = COM_Down;

		tObj->DrawCommand(cgoBottom, C4FCT_Right, nullptr, com, pRegions, Owner, LoadResStr(C4ResStrTableKey::IDS_CON_BUILD, tObj->GetName()).c_str(), &ccgo);
		tObj->Def->Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Right, C4FCT_Top), false, tObj->Color, tObj);
		Game.GraphicsResource.fctBuild.Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Left, C4FCT_Bottom), true);
	}

	// Grab target control (control flag)
	if (iDFA == DFA_PUSH && Action.Target)
	{
		for (cnt = ComOrderNum - 1; cnt >= 0; cnt--)
			if (DrawCommandQuery(Controller, Action.Target->Def->Script, Action.Target->Def->Script.ControlMethod, cnt))
			{
				Action.Target->DrawCommand(cgoBottom, C4FCT_Right, PSF_Control, ComOrder(cnt), pRegions, Owner);
			}
			else if (ComOrder(cnt) == COM_Down_D)
			{
				// Let Go
				Action.Target->DrawCommand(cgoBottom, C4FCT_Right, nullptr, COM_Down_D, pRegions, Owner, LoadResStr(C4ResStrTableKey::IDS_CON_UNGRAB, Action.Target->GetName()).c_str(), &ccgo);
				Action.Target->Def->Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Right, C4FCT_Top), false, Action.Target->Color, Action.Target);
				Game.GraphicsResource.fctHand.Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Left, C4FCT_Bottom), true, 6);
			}
			else if (ComOrder(cnt) == COM_Throw)
			{
				// Put
				if ((tObj = Contents.GetObject()) && (Action.Target->Def->GrabPutGet & C4D_Grab_Put))
				{
					Action.Target->DrawCommand(cgoBottom, C4FCT_Right, nullptr, COM_Throw, pRegions, Owner, LoadResStr(C4ResStrTableKey::IDS_CON_PUT, tObj->GetName(), Action.Target->GetName()).c_str(), &ccgo);
					tObj->Def->Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Right, C4FCT_Top), false, tObj->Color, tObj);
					Game.GraphicsResource.fctHand.Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Left, C4FCT_Bottom), true, 0);
				}
				// Get
				else if (Action.Target->Contents.ListIDCount(C4D_Get) && (Action.Target->Def->GrabPutGet & C4D_Grab_Get))
				{
					Action.Target->DrawCommand(cgoBottom, C4FCT_Right, nullptr, COM_Throw, pRegions, Owner, LoadResStr(C4ResStrTableKey::IDS_CON_GET, Action.Target->GetName()).c_str(), &ccgo);
					Action.Target->Def->Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Right, C4FCT_Top), false, Action.Target->Color, Action.Target);
					Game.GraphicsResource.fctHand.Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Left, C4FCT_Bottom), true, 1);
				}
			}
	}

	// Contained control (contained control flag)
	if (Contained)
	{
		for (cnt = ComOrderNum - 1; cnt >= 0; cnt--)
			if (DrawCommandQuery(Controller, Contained->Def->Script, Contained->Def->Script.ContainedControlMethod, cnt))
			{
				Contained->DrawCommand(cgoBottom, C4FCT_Right, PSF_ContainedControl, ComOrder(cnt), pRegions, Owner);
				// Contained control down overrides contained exit control
				if (Com2Control(ComOrder(cnt)) == CON_Down) fContainedDownOverride = true;
				// carlo - Contained controls left/right override contained Take, Take2 controls
				if (Com2Control(ComOrder(cnt)) == CON_Left)  fContainedLeftOverride  = true;
				if (Com2Control(ComOrder(cnt)) == CON_Right) fContainedRightOverride = true;
			}
		// Contained exit
		if (!fContainedDownOverride)
		{
			DrawCommand(cgoBottom, C4FCT_Right, nullptr, COM_Down, pRegions, Owner,
				LoadResStr(C4ResStrTableKey::IDS_CON_EXIT), &ccgo);
			Game.GraphicsResource.fctExit.Draw(ccgo);
		}
		// Contained base commands
		if (ValidPlr(Contained->Base))
		{
			// Sell
			if (Section->C4S.Game.Realism.BaseFunctionality & BASEFUNC_Sell)
			{
				Contained->DrawCommand(cgoBottom, C4FCT_Right, nullptr, COM_Dig, pRegions, Owner, LoadResStr(C4ResStrTableKey::IDS_CON_SELL), &ccgo);
				DrawMenuSymbol(C4MN_Sell, ccgo, Contained->Base, Contained);
			}
			// Buy
			if (Section->C4S.Game.Realism.BaseFunctionality & BASEFUNC_Buy)
			{
				Contained->DrawCommand(cgoBottom, C4FCT_Right, nullptr, COM_Up, pRegions, Owner, LoadResStr(C4ResStrTableKey::IDS_CON_BUY), &ccgo);
				DrawMenuSymbol(C4MN_Buy, ccgo, Contained->Base, Contained);
			}
		}
		// Contained put & activate
		// carlo
		int32_t nContents = Contained->Contents.ListIDCount(C4D_Get);
		if (nContents)
		{
			// carlo: Direct get ("Take2")
			if (!fContainedRightOverride)
			{
				Contained->DrawCommand(cgoBottom, C4FCT_Right, nullptr, COM_Right, pRegions, Owner, LoadResStr(C4ResStrTableKey::IDS_CON_GET, Contained->GetName()).c_str(), &ccgo);
				Contained->Def->Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Right, C4FCT_Top), false, Contained->Color, Contained);
				Game.GraphicsResource.fctHand.Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Left, C4FCT_Bottom), true, 1);
			}
			// carlo: Get ("Take")
			if (!fContainedLeftOverride)
			{
				Contained->DrawCommand(cgoBottom, C4FCT_Right, nullptr, COM_Left, pRegions, Owner, LoadResStr(C4ResStrTableKey::IDS_CON_ACTIVATEFROM, Contained->GetName()).c_str(), &ccgo);
				Contained->Def->Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Right, C4FCT_Top), false, Contained->Color, Contained);
				Game.GraphicsResource.fctHand.Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Left, C4FCT_Bottom), true, 0);
			}
		}
		if (tObj = Contents.GetObject())
		{
			// Put
			Contained->DrawCommand(cgoBottom, C4FCT_Right, nullptr, COM_Throw, pRegions, Owner, LoadResStr(C4ResStrTableKey::IDS_CON_PUT, tObj->GetName(), Contained->GetName()).c_str(), &ccgo);
			tObj->Def->Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Right, C4FCT_Top), false, tObj->Color, tObj);
			Game.GraphicsResource.fctHand.Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Left, C4FCT_Bottom), true, 0);
		}
		else if (nContents)
		{
			// Get
			Contained->DrawCommand(cgoBottom, C4FCT_Right, nullptr, COM_Throw, pRegions, Owner, LoadResStr(C4ResStrTableKey::IDS_CON_ACTIVATEFROM, Contained->GetName()).c_str(), &ccgo);
			Contained->Def->Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Right, C4FCT_Top), false, Contained->Color, Contained);
			Game.GraphicsResource.fctHand.Draw(ccgo2 = ccgo.GetFraction(85, 85, C4FCT_Left, C4FCT_Bottom), true, 0);
		}
	}

	// Contents activation (activation control flag)
	if (!Contained)
	{
		if ((iDFA == DFA_WALK) || (iDFA == DFA_SWIM) || (iDFA == DFA_DIG))
			if (tObj = Contents.GetObject())
				if (DrawCommandQuery(Controller, tObj->Def->Script, tObj->Def->Script.ActivationControlMethod, COM_Dig_D))
				{
					tObj->DrawCommand(cgoBottom, C4FCT_Right, PSF_Activate, COM_Dig_D, pRegions, Owner);
					// Flag override self-activation
					fContentsActivationOverride = true;
				}

		// Self activation (activation control flag)
		if (!fContentsActivationOverride)
			if ((iDFA == DFA_WALK) || (iDFA == DFA_SWIM) || (iDFA == DFA_FLOAT))
				if (DrawCommandQuery(Controller, Def->Script, Def->Script.ActivationControlMethod, COM_Dig_D))
					DrawCommand(cgoSide, C4FCT_Bottom | C4FCT_Half, PSF_Activate, COM_Dig_D, pRegions, Owner);
	}

	// Self special control (control flag)
	for (cnt = 6; cnt < ComOrderNum; cnt++)
	{
		// Hardcoded com order indexes for COM_Specials
		if (cnt == 8) cnt = 14; if (cnt == 16) cnt = 22;
		// Control in control flag?
		if (DrawCommandQuery(Controller, Def->Script, Def->Script.ControlMethod, cnt))
			DrawCommand(cgoSide, C4FCT_Bottom | C4FCT_Half, PSF_Control, ComOrder(cnt), pRegions, Owner);
	}
}

void C4Object::DrawPicture(C4Facet &cgo, bool fSelected, C4RegionList *pRegions)
{
	// Draw def picture with object color
	Def->Draw(cgo, fSelected, Color, this);
	// Region
	if (pRegions) pRegions->Add(cgo.X, cgo.Y, cgo.Wdt, cgo.Hgt, GetName(), COM_None, this);
}

void C4Object::Picture2Facet(C4FacetExSurface &cgo)
{
	// set picture rect to facet
	C4Rect fctPicRect = PictureRect;
	if (!fctPicRect.Wdt) fctPicRect = Def->PictureRect;
	const auto scaledRect = fctPicRect.Scaled(Def->Scale);
	C4Facet fctPicture{GetGraphics()->GetBitmap(Color), scaledRect.x, scaledRect.y, scaledRect.Wdt, scaledRect.Hgt};

	// use direct facet w/o own data if possible
	if (!ColorMod && BlitMode == C4GFXBLIT_NORMAL && !pGfxOverlay)
	{
		cgo.Set(fctPicture);
		return;
	}

	// otherwise, draw to picture facet
	if (!cgo.Create(cgo.Wdt, cgo.Hgt)) return;

	// specific object color?
	PrepareDrawing();

	// draw picture itself
	fctPicture.Draw(cgo, true);

	// draw overlays
	if (pGfxOverlay)
		for (C4GraphicsOverlay *pGfxOvrl = pGfxOverlay; pGfxOvrl; pGfxOvrl = pGfxOvrl->GetNext())
			if (pGfxOvrl->IsPicture())
				pGfxOvrl->DrawPicture(cgo, this);

	// done; reset drawing states
	FinishedDrawing();
}

bool C4Object::ValidateOwner()
{
	// Check owner and controller
	if (!ValidPlr(Owner)) Owner = NO_OWNER;
	if (!ValidPlr(Base)) Base = NO_OWNER;
	if (!ValidPlr(Controller)) Controller = NO_OWNER;
	// Color is not reset any more, because many scripts change colors to non-owner-colors these days
	// Additionally, player colors are now guarantueed to remain the same in savegame resumes
	return true;
}

bool C4Object::AssignInfo()
{
	if (Info || !ValidPlr(Owner)) return false;
	// In crew list?
	C4Player *pPlr = Game.Players.Get(Owner);
	if (pPlr->Crew.GetLink(this))
	{
		// Register with player
		if (!Game.Players.Get(Owner)->MakeCrewMember(this, true, false))
			pPlr->Crew.Remove(this);
		return true;
	}
	// Info set, but not in crew list, so
	//    a) The savegame is old-style (without crew list)
	// or b) The clonk is dead
	// or c) The clonk belongs to a script player that's restored without Game.txt
	else if (nInfo.getLength())
	{
		if (!Game.Players.Get(Owner)->MakeCrewMember(this, true, false))
			return false;
		// Dead and gone (info flags, remove from crew/cursor)
		if (!Alive)
		{
			Info->HasDied = true;
			if (ValidPlr(Owner)) Game.Players.Get(Owner)->ClearPointers(this, true);
		}
		return true;
	}
	return false;
}

bool C4Object::AssignPlrViewRange()
{
	// no range?
	if (!PlrViewRange) return true;
	// add to FoW-repellers
	PlrFoWActualize();
	// success
	return true;
}

void C4Object::ClearInfo(C4ObjectInfo *pInfo)
{
	if (Info == pInfo) Info = nullptr;
}

void C4Object::Clear()
{
	delete pEffects;         pEffects         = nullptr;
	if (FrontParticles) FrontParticles.Clear(Section->Particles.FreeParticles);
	if (BackParticles)   BackParticles.Clear(Section->Particles.FreeParticles);
	delete pSolidMaskData;   pSolidMaskData   = nullptr;
	delete Menu;             Menu             = nullptr;
	MaterialContents.fill(0);
	// clear commands!
	C4Command *pCom, *pNext;
	for (pCom = Command; pCom; pCom = pNext)
	{
		pNext = pCom->Next; delete pCom; pCom = pNext;
	}
	delete pDrawTransform;   pDrawTransform   = nullptr;
	delete pGfxOverlay;      pGfxOverlay      = nullptr;
	while (FirstRef) FirstRef->Set0();
}

bool C4Object::ContainedControl(uint8_t byCom)
{
	// Check
	if (!Contained) return false;
	// Check if object is about to exit; if so, return
	// dunno, maybe I should check all the commands, not just the first one?
	if ((byCom == COM_Left || byCom == COM_Right) && Command)
		if (Command->Command == C4CMD_Exit)
			// hack: in structures only; not in vehicles
			// they might have a pending Exit-command due to a down-control
			if (Contained->Category & C4D_Structure)
				return false; // or true? Currently it doesn't matter.
	// get script function if defined
	C4AulFunc *sf = Contained->Def->Script.GetSFunc(std::format(PSF_ContainedControl, ComName(byCom)).c_str());
	// in old versions, do hardcoded actions first (until gwe3)
	// new objects may overload them
	C4Def *pCDef = Contained->Def;
	bool fCallSfEarly = CompareVersion(pCDef->rC4XVer[0], pCDef->rC4XVer[1], pCDef->rC4XVer[2], pCDef->rC4XVer[3], 0, 4, 9, 1, 3) >= 0;
	bool result = false;
	C4Player *pPlr = Game.Players.Get(Controller);
	if (fCallSfEarly)
	{
		if (sf && sf->Exec(*Contained->Section, Contained, {C4VObj(this)})) result = true;
		// AutoStopControl: Also notify container about controlupdate
		// Note Contained may be nulled now due to ContainedControl call
		if (Contained && !(byCom & (COM_Single | COM_Double)) && pPlr->ControlStyle)
		{
			int32_t PressedComs = pPlr->PressedComs;
			Contained->Call(PSF_ContainedControlUpdate, {C4VObj(this), C4VInt(Coms2ComDir(PressedComs)),
				C4VBool(!!(PressedComs & (1 << COM_Dig))), C4VBool(!!(PressedComs & (1 << COM_Throw)))});
		}
	}
	if (result) return true;

	// hardcoded actions
	switch (byCom)
	{
	case COM_Down:
		PlayerObjectCommand(Owner, C4CMD_Exit);
		break;
	case COM_Throw_D:
		// avoid breaking objects with non-default behavior on ContainedThrow
		if (Contained->Def->Script.GetSFunc(std::format(PSF_ContainedControl, ComName(COM_Throw)).c_str()))
		{
			break;
		}
		[[fallthrough]];
	case COM_Throw:
		PlayerObjectCommand(Owner, C4CMD_Throw) && ExecuteCommand();
		break;
	case COM_Up:
		if (ValidPlr(Contained->Base))
			if (!Hostile(Owner, Contained->Base))
				if (Section->C4S.Game.Realism.BaseFunctionality & BASEFUNC_Buy)
					ActivateMenu(C4MN_Buy);
		break;
	case COM_Dig:
		if (ValidPlr(Contained->Base))
			if (!Hostile(Owner, Contained->Base))
				if (Section->C4S.Game.Realism.BaseFunctionality & BASEFUNC_Sell)
					ActivateMenu(C4MN_Sell);
		break;
	}
	// Call container script if defined for old versions
	if (!fCallSfEarly)
	{
		if (sf) sf->Exec(*Contained->Section, Contained, {C4VObj(this)});
		if (Contained && !(byCom & (COM_Single | COM_Double)) && pPlr->ControlStyle)
		{
			int32_t PressedComs = pPlr->PressedComs;
			Contained->Call(PSF_ContainedControlUpdate, {C4VObj(this), C4VInt(Coms2ComDir(PressedComs)),
				C4VBool(!!(PressedComs & (1 << COM_Dig))), C4VBool(!!(PressedComs & (1 << COM_Throw)))});
		}
	}
	// Take/Take2
	if (!sf || fCallSfEarly) switch (byCom)
	{
	case COM_Left:
		PlayerObjectCommand(Owner, C4CMD_Take);
		break;
	case COM_Right:
		PlayerObjectCommand(Owner, C4CMD_Take2);
		break;
	}
	// Success
	return true;
}

bool C4Object::CallControl(C4Player *pPlr, uint8_t byCom, const C4AulParSet &pPars)
{
	assert(pPlr);

	bool result = static_cast<bool>(Call(std::format(PSF_Control, ComName(byCom)).c_str(), pPars));

	// Call ControlUpdate when using Jump'n'Run control
	if (pPlr->ControlStyle)
	{
		int32_t PressedComs = pPlr->PressedComs;
		Call(PSF_ControlUpdate, {pPars[0]._getBool() ? pPars[0] : C4VObj(this),
			C4VInt(Coms2ComDir(PressedComs)),
			C4VBool(!!(PressedComs & (1 << COM_Dig))),
			C4VBool(!!(PressedComs & (1 << COM_Throw))),
			C4VBool(!!(PressedComs & (1 << COM_Special))),
			C4VBool(!!(PressedComs & (1 << COM_Special2)))});
	}
	return result;
}

void C4Object::DirectCom(uint8_t byCom, int32_t iData) // By player ObjectCom
{
#ifdef DEBUGREC_OBJCOM
	C4RCObjectCom rc = { byCom, iData, Number };
	AddDbgRec(RCT_ObjCom, &rc, sizeof(C4RCObjectCom));
#endif

	// Wether this is a KeyRelease-event
	bool IsRelease = Inside(byCom, COM_ReleaseFirst, COM_ReleaseLast);
	const auto plainCom = (IsRelease ? (byCom - 16) : (byCom & ~(COM_Single | COM_Double)));
	bool isCursor = Inside<int>(plainCom, COM_CursorFirst, COM_CursorLast);

	// we only want the script callbacks for cursor controls
	if (isCursor)
	{
		if (C4Player *pController = Game.Players.Get(Controller))
		{
			CallControl(pController, byCom);
		}
		return;
	}

	// COM_Special and COM_Contents specifically bypass the menu and always go to the object
	bool fBypassMenu = ((plainCom == COM_Special) || (byCom == COM_Contents));

	// Menu control
	if (!fBypassMenu)
		if (Menu && Menu->Control(byCom, iData)) return;

	// Ignore any menu com leftover in control queue from closed menu
	if (Inside(byCom, COM_MenuNavigation1, COM_MenuNavigation2)) return;

	// Decrease NoCollectDelay
	if (!(byCom & COM_Single) && !(byCom & COM_Double) && !IsRelease)
		if (NoCollectDelay > 0)
			NoCollectDelay--;

	// COM_Contents contents shift (data is target number (not ID!))
	// contents shift must always be done to container object, which is not necessarily this
	if (byCom == COM_Contents)
	{
		C4Object *pTarget = Section->Objects.SafeObjectPointer(iData);
		if (pTarget && pTarget->Contained)
			pTarget->Contained->DirectComContents(pTarget, true);
		return;
	}

	// Contained control (except specials)
	if (Contained)
		if (plainCom != COM_Special && plainCom != COM_Special2 && byCom != COM_WheelUp && byCom != COM_WheelDown)
		{
			Contained->Controller = Controller; ContainedControl(byCom); return;
		}

	// Regular DirectCom clears commands
	if (!(byCom & COM_Single) && !(byCom & COM_Double) && !IsRelease)
		ClearCommands();

	// Object script override
	C4Player *pController;
	if (pController = Game.Players.Get(Controller))
		if (CallControl(pController, byCom))
			return;

	// direct wheel control
	if (byCom == COM_WheelUp || byCom == COM_WheelDown)
		// scroll contents
	{
		ShiftContents(byCom == COM_WheelUp, true); return;
	}

	// The Player updates Controller before calling this, so trust Players.Get will return it
	if (pController && pController->ControlStyle)
	{
		AutoStopDirectCom(byCom, iData);
		return;
	}

	// Control by procedure
	switch (GetProcedure())
	{
	case DFA_WALK:
		switch (byCom)
		{
		case COM_Left:   ObjectComMovement(this, COMD_Left); break;
		case COM_Right:  ObjectComMovement(this, COMD_Right); break;
		case COM_Down:   ObjectComMovement(this, COMD_Stop); break;
		case COM_Up:     ObjectComUp(this); break;
		case COM_Down_D: ObjectComDownDouble(this); break;
		case COM_Dig_S:
			if (ObjectComDig(this))
			{
				Action.ComDir = (Action.Dir == DIR_Right) ? COMD_DownRight : COMD_DownLeft;
			}
			break;
		case COM_Dig_D:  ObjectComDigDouble(this); break;
		case COM_Throw:  PlayerObjectCommand(Owner, C4CMD_Throw); break;
		}
		break;

	case DFA_FLIGHT: case DFA_KNEEL: case DFA_THROW:
		switch (byCom)
		{
		case COM_Left:   ObjectComMovement(this, COMD_Left); break;
		case COM_Right:  ObjectComMovement(this, COMD_Right); break;
		case COM_Down:   ObjectComMovement(this, COMD_Stop); break;
		case COM_Throw:  PlayerObjectCommand(Owner, C4CMD_Throw); break;
		}
		break;

	case DFA_SCALE:
		switch (byCom)
		{
		case COM_Left:
			if (Action.Dir == DIR_Left) ObjectComMovement(this, COMD_Stop);
			else { ObjectComMovement(this, COMD_Left); ObjectComLetGo(this, -1); }
			break;
		case COM_Right:
			if (Action.Dir == DIR_Right) ObjectComMovement(this, COMD_Stop);
			else { ObjectComMovement(this, COMD_Right); ObjectComLetGo(this, +1); }
			break;
		case COM_Up:   ObjectComMovement(this, COMD_Up); break;
		case COM_Down: ObjectComMovement(this, COMD_Down); break;
		case COM_Throw: PlayerObjectCommand(Owner, C4CMD_Drop); break;
		}
		break;

	case DFA_HANGLE:
		switch (byCom)
		{
		case COM_Left:  ObjectComMovement(this, COMD_Left); break;
		case COM_Right: ObjectComMovement(this, COMD_Right); break;
		case COM_Up:    ObjectComMovement(this, COMD_Stop); break;
		case COM_Down:  ObjectComLetGo(this, 0); break;
		case COM_Throw: PlayerObjectCommand(Owner, C4CMD_Drop); break;
		}
		break;

	case DFA_DIG:
		switch (byCom)
		{
		case COM_Left:  if (Inside<int32_t>(Action.ComDir, COMD_UpRight, COMD_Left)) Action.ComDir++; break;
		case COM_Right: if (Inside<int32_t>(Action.ComDir, COMD_Right, COMD_UpLeft)) Action.ComDir--; break;
		case COM_Down:  ObjectComStop(this); break;
		case COM_Dig_D: ObjectComDigDouble(this); break;
		case COM_Dig_S: Action.Data = (!Action.Data); break; // Dig mat 2 object request
		}
		break;

	case DFA_SWIM:
		switch (byCom)
		{
		case COM_Left:  ObjectComMovement(this, COMD_Left); break;
		case COM_Right: ObjectComMovement(this, COMD_Right); break;
		case COM_Up:
			ObjectComMovement(this, COMD_Up);
			ObjectComUp(this); break;
		case COM_Down:  ObjectComMovement(this, COMD_Down); break;
		case COM_Throw: PlayerObjectCommand(Owner, C4CMD_Drop); break;
		case COM_Dig_D: ObjectComDigDouble(this); break;
		}
		break;

	case DFA_BRIDGE: case DFA_BUILD: case DFA_CHOP:
		switch (byCom)
		{
		case COM_Down: ObjectComStop(this); break;
		}
		break;

	case DFA_FIGHT:
		switch (byCom)
		{
		case COM_Left:  ObjectComMovement(this, COMD_Left); break;
		case COM_Right: ObjectComMovement(this, COMD_Right); break;
		case COM_Down:  ObjectComStop(this); break;
		}
		break;

	case DFA_PUSH:
	{
		bool fGrabControlOverload = false;
		if (Action.Target)
		{
			// Make sure controller is up-to-date, so if e.g. multiple people push a catapult the controller is correct for whoever issued the ControlThrow
			Action.Target->Controller = Controller;
			// New grab-control model: objects version 4.95 or higher (CE)
			// may overload control of grabbing clonks
			C4Def *pTDef = Action.Target->Def;
			if (CompareVersion(pTDef->rC4XVer[0], pTDef->rC4XVer[1], pTDef->rC4XVer[2], pTDef->rC4XVer[3], 0, 4, 9, 5, 0) >= 0)
				fGrabControlOverload = true;
		}
		// Call object control first in case it overloads
		if (fGrabControlOverload)
			if (Action.Target)
				if (Action.Target->CallControl(pController, byCom, {C4VObj(this)}))
					return;
		// Clonk direct control
		switch (byCom)
		{
		case COM_Left:  ObjectComMovement(this, COMD_Left); break;
		case COM_Right: ObjectComMovement(this, COMD_Right); break;
		case COM_Up:
			// Target -> enter
			if (ObjectComEnter(Action.Target))
				ObjectComMovement(this, COMD_Stop);
			// Else, comdir up for target straightening
			else
				ObjectComMovement(this, COMD_Up);
			break;
		case COM_Down: ObjectComMovement(this, COMD_Stop); break;
		case COM_Down_D: ObjectComUnGrab(this); break;
		case COM_Throw_D:
			// avoid breaking objects with non-default behavior on ControlThrow
			if (!fGrabControlOverload || !Action.Target || Action.Target->Def->Script.GetSFunc(std::format(PSF_Control, ComName(COM_Throw)).c_str()))
			{
				break;
			}
			[[fallthrough]];
		case COM_Throw:
			PlayerObjectCommand(Owner, C4CMD_Throw);
			break;
		}
		// Action target call control late for old objects
		if (!fGrabControlOverload)
			if (Action.Target)
				Action.Target->CallControl(pController, byCom, {C4VObj(this)});
		break;
	}
	}
}

void C4Object::AutoStopDirectCom(uint8_t byCom, int32_t iData) // By DirecCom
{
	C4Player *pPlayer = Game.Players.Get(Controller);
	// Control by procedure
	switch (GetProcedure())
	{
	case DFA_WALK:
		switch (byCom)
		{
		case COM_Up:    ObjectComUp(this); break;
		case COM_Down:
			// inhibit controldownsingle on freshly grabbed objects
			if (ObjectComDownDouble(this))
				pPlayer->LastCom = COM_None;
			break;
		case COM_Dig_S: ObjectComDig(this); break;
		case COM_Dig_D: ObjectComDigDouble(this); break;
		case COM_Throw: PlayerObjectCommand(Owner, C4CMD_Throw); break;
		default: AutoStopUpdateComDir();
		}
		break;

	case DFA_FLIGHT:
		switch (byCom)
		{
		case COM_Throw:
			// Drop when pressing left, right or down
			if (pPlayer->PressedComs & ((1 << COM_Left) | (1 << COM_Right) | (1 << COM_Down)))
				PlayerObjectCommand(Owner, C4CMD_Drop);
			else
				// This will fail, but whatever.
				PlayerObjectCommand(Owner, C4CMD_Throw);
			break;
		default: AutoStopUpdateComDir();
		}
		break;

	case DFA_KNEEL: case DFA_THROW:
		switch (byCom)
		{
		case COM_Throw: PlayerObjectCommand(Owner, C4CMD_Throw); break;
		default: AutoStopUpdateComDir();
		}
		break;

	case DFA_SCALE:
		switch (byCom)
		{
		case COM_Left:
			if (Action.Dir == DIR_Right) ObjectComLetGo(this, -1);
			else AutoStopUpdateComDir();
			break;
		case COM_Right:
			if (Action.Dir == DIR_Left) ObjectComLetGo(this, +1);
			else AutoStopUpdateComDir();
			break;
		case COM_Dig:    ObjectComLetGo(this, (Action.Dir == DIR_Left) ? +1 : -1);
		case COM_Throw:  PlayerObjectCommand(Owner, C4CMD_Drop); break;
		default: AutoStopUpdateComDir();
		}
		break;

	case DFA_HANGLE:
		switch (byCom)
		{
		case COM_Down:    ObjectComLetGo(this, 0); break;
		case COM_Dig:     ObjectComLetGo(this, 0); break;
		case COM_Throw:   PlayerObjectCommand(Owner, C4CMD_Drop); break;
		default: AutoStopUpdateComDir();
		}
		break;

	case DFA_DIG:
		switch (byCom)
		{
		// Dig mat 2 object request
		case COM_Throw: case COM_Dig: Action.Data = (!Action.Data); break;
		default: AutoStopUpdateComDir();
		}
		break;

	case DFA_SWIM:
		switch (byCom)
		{
		case COM_Up:
			AutoStopUpdateComDir();
			ObjectComUp(this);
			break;
		case COM_Throw: PlayerObjectCommand(Owner, C4CMD_Drop); break;
		case COM_Dig_D: ObjectComDigDouble(this); break;
		default: AutoStopUpdateComDir();
		}
		break;

	case DFA_BRIDGE: case DFA_BUILD: case DFA_CHOP:
		switch (byCom)
		{
		case COM_Down: ObjectComStop(this); break;
		}
		break;

	case DFA_FIGHT:
		switch (byCom)
		{
		case COM_Down: ObjectComStop(this); break;
		default: AutoStopUpdateComDir();
		}
		break;

	case DFA_PUSH:
	{
		bool fGrabControlOverload = false;
		if (Action.Target)
		{
			// New grab-control model: objects version 4.95 or higher (CE)
			// may overload control of grabbing clonks
			C4Def *pTDef = Action.Target->Def;
			if (CompareVersion(pTDef->rC4XVer[0], pTDef->rC4XVer[1], pTDef->rC4XVer[2], pTDef->rC4XVer[3], 0, 4, 9, 5, 0) >= 0)
				fGrabControlOverload = true;
			// Call object control first in case it overloads
			if (fGrabControlOverload)
			{
				if (Action.Target->CallControl(pPlayer, byCom, {C4VObj(this)}))
				{
					return;
				}
			}
		}
		// Clonk direct control
		switch (byCom)
		{
		case COM_Up:
			// Target -> enter
			if (ObjectComEnter(Action.Target))
				ObjectComMovement(this, COMD_Stop);
			// Else, comdir up for target straightening
			else
				AutoStopUpdateComDir();
			break;
		case COM_Down:
			// FIXME: replace constants
			// ComOrder(3) is COM_Down, ComOrder(11) is COM_Down_S and ComOrder(19) is COM_Down_D
			if (Action.Target
				&& !DrawCommandQuery(Controller, Action.Target->Def->Script, Action.Target->Def->Script.ControlMethod, 3)
				&& !DrawCommandQuery(Controller, Action.Target->Def->Script, Action.Target->Def->Script.ControlMethod, 11)
				&& !DrawCommandQuery(Controller, Action.Target->Def->Script, Action.Target->Def->Script.ControlMethod, 19))
			{
				ObjectComUnGrab(this);
			}
			break;
		case COM_Down_D:  ObjectComUnGrab(this); break;
		case COM_Throw_D:
			// avoid breaking objects with non-default behavior on ControlThrow
			if (!fGrabControlOverload || !Action.Target || Action.Target->Def->Script.GetSFunc(std::format(PSF_Control, ComName(COM_Throw)).c_str()))
			{
				break;
			}
			[[fallthrough]];
		case COM_Throw:   PlayerObjectCommand(Owner, C4CMD_Drop); break;
		default:
			AutoStopUpdateComDir();
		}
		// Action target call control late for old objects
		if (!fGrabControlOverload && Action.Target)
			Action.Target->CallControl(pPlayer, byCom, {C4VObj(this)});
		break;
	}
	}
}

void C4Object::AutoStopUpdateComDir()
{
	C4Player *pPlr = Game.Players.Get(Controller);
	if (!pPlr || pPlr->Cursor != this) return;
	int32_t NewComDir = Coms2ComDir(pPlr->PressedComs);
	if (Action.ComDir == NewComDir) return;
	if (NewComDir == COMD_Stop && GetProcedure() == DFA_DIG)
	{
		ObjectComStop(this);
		return;
	}
	ObjectComMovement(this, NewComDir);
}

bool C4Object::MenuCommand(const char *szCommand)
{
	// Native script execution
	if (!Def || !Status) return false;
	return static_cast<bool>(Def->Script.DirectExec(*Section, this, szCommand, "MenuCommand", false, Def->Script.Strict));
}

C4Object *C4Object::ComposeContents(C4ID id)
{
	int32_t cnt, cnt2;
	C4ID c_id;
	bool fInsufficient = false;
	C4Object *pObj;
	C4ID idNeeded = C4ID_None;
	int32_t iNeeded = 0;
	// Get def
	C4Def *pDef = Game.Defs.ID2Def(id); if (!pDef) return nullptr;
	// get needed contents
	C4IDList NeededComponents;
	pDef->GetComponents(&NeededComponents, *Section, nullptr, this);
	// Check for sufficient components
	std::string needs{LoadResStr(C4ResStrTableKey::IDS_CON_BUILDMATNEED, pDef->GetName())};
	for (cnt = 0; c_id = NeededComponents.GetID(cnt); cnt++)
		if (NeededComponents.GetCount(cnt) > Contents.ObjectCount(c_id))
		{
			needs += std::format("|{}x {}", NeededComponents.GetCount(cnt) - Contents.ObjectCount(c_id), Game.Defs.ID2Def(c_id) ? Game.Defs.ID2Def(c_id)->GetName() : C4IdText(c_id));
			if (!idNeeded) { idNeeded = c_id; iNeeded = NeededComponents.GetCount(cnt) - Contents.ObjectCount(c_id); }
			fInsufficient = true;
		}
	// Insufficient
	if (fInsufficient)
	{
		// BuildNeedsMaterial call to object...
		if (!Call(PSF_BuildNeedsMaterial, {C4VID(idNeeded), C4VInt(iNeeded)}))
			// ...game message if not overloaded
			GameMsgObject(needs.c_str(), this);
		// Return
		return nullptr;
	}
	// Remove components
	for (cnt = 0; c_id = NeededComponents.GetID(cnt); cnt++)
		for (cnt2 = 0; cnt2 < NeededComponents.GetCount(cnt); cnt2++)
			if (!(pObj = Contents.Find(c_id)))
				return nullptr;
			else
				pObj->AssignRemoval();
	// Create composed object
	// the object is created with default components instead of builder components
	// this is done because some objects (e.g. arrow packs) will set custom components during initialization, which should not be overriden
	return CreateContents(id);
}

void C4Object::SetSolidMask(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, int32_t iTX, int32_t iTY)
{
	// remove osld
	if (pSolidMaskData) pSolidMaskData->Remove(true, false);
	delete pSolidMaskData; pSolidMaskData = nullptr;
	// set new data
	SolidMask.Set(iX, iY, iWdt, iHgt, iTX, iTY);
	// re-put if valid
	if (CheckSolidMaskRect()) UpdateSolidMask(false);
}

bool C4Object::CheckSolidMaskRect()
{
	// check NewGfx only, because invalid SolidMask-rects are OK in OldGfx
	// the bounds-check is done in CStdDDraw::GetPixel()
	C4Surface *sfcGraphics = GetGraphics()->GetBitmap();
	SolidMask.Set(std::max<int32_t>(SolidMask.x, 0), std::max<int32_t>(SolidMask.y, 0), std::min<int32_t>(SolidMask.Wdt, sfcGraphics->Wdt - SolidMask.x), std::min<int32_t>(SolidMask.Hgt, sfcGraphics->Hgt - SolidMask.y), SolidMask.tx, SolidMask.ty);
	if (SolidMask.Hgt <= 0) SolidMask.Wdt = 0;
	return SolidMask.Wdt > 0;
}

void C4Object::SyncClearance()
{
	// Misc. no-save safeties
	Action.t_attach = CNAT_None;
	InMat = MNone;
	t_contact = 0;
	// Fixed point values - precision reduction
	fix_x = itofix(x);
	fix_y = itofix(y);
	fix_r = itofix(r);
	// Update OCF
	SetOCF();
	// Menu
	CloseMenu(true);
	// Material contents
	MaterialContents.fill(0);
	// reset speed of staticback-objects
	if (Category & C4D_StaticBack)
	{
		xdir = ydir = 0;
	}
}

void C4Object::DrawSelectMark(C4FacetEx &cgo)
{
	// Status
	if (!Status) return;
	// No select marks in film playback
	if (Game.C4S.Head.Film && Game.C4S.Head.Replay) return;
	// target pos (parallax)
	int32_t cotx = cgo.TargetX, coty = cgo.TargetY; TargetPos(cotx, coty, cgo);
	// Output boundary
	if (!Inside<int32_t>(x - cotx, 0, cgo.Wdt - 1)
		|| !Inside<int32_t>(y - coty, 0, cgo.Hgt - 1)) return;
	// Draw select marks
	int32_t cox = x + Shape.x - cotx + cgo.X - 2;
	int32_t coy = y + Shape.y - coty + cgo.Y - 2;
	GfxR->fctSelectMark.Draw(cgo.Surface, cox, coy, 0);
	GfxR->fctSelectMark.Draw(cgo.Surface, cox + Shape.Wdt, coy, 1);
	GfxR->fctSelectMark.Draw(cgo.Surface, cox, coy + Shape.Hgt, 2);
	GfxR->fctSelectMark.Draw(cgo.Surface, cox + Shape.Wdt, coy + Shape.Hgt, 3);
}

void C4Object::ClearCommands()
{
	C4Command *pNext;
	while (Command)
	{
		pNext = Command->Next;
		if (!Command->iExec)
			delete Command;
		else
			Command->iExec = 2;
		Command = pNext;
	}
}

void C4Object::ClearCommand(C4Command *pUntil)
{
	C4Command *pCom, *pNext;
	for (pCom = Command; pCom; pCom = pNext)
	{
		// Last one to clear
		if (pCom == pUntil) pNext = nullptr;
		// Next one to clear after this
		else pNext = pCom->Next;
		Command = pCom->Next;
		if (!pCom->iExec)
			delete pCom;
		else
			pCom->iExec = 2;
	}
}

bool C4Object::AddCommand(int32_t iCommand, C4Object *pTarget, C4Value iTx, int32_t iTy,
	int32_t iUpdateInterval, C4Object *pTarget2,
	bool fInitEvaluation, int32_t iData, bool fAppend,
	int32_t iRetries, const char *szText, int32_t iBaseMode)
{
	// Command stack size safety
	const int32_t MaxCommandStack = 35;
	C4Command *pCom, *pLast; int32_t iCommands;
	for (pCom = Command, iCommands = 0; pCom; pCom = pCom->Next, iCommands++);
	if (iCommands >= MaxCommandStack) return false;
	// Valid command safety
	if (!Inside(iCommand, C4CMD_First, C4CMD_Last)) return false;
	// Allocate and set new command
	pCom = new C4Command;
	pCom->Set(iCommand, this, pTarget, iTx, iTy, pTarget2, iData,
		iUpdateInterval, !fInitEvaluation, iRetries, szText, iBaseMode);
	// Append to bottom of stack
	if (fAppend)
	{
		for (pLast = Command; pLast && pLast->Next; pLast = pLast->Next);
		if (pLast) pLast->Next = pCom;
		else Command = pCom;
	}
	// Add to top of command stack
	else
	{
		pCom->Next = Command;
		Command = pCom;
	}
	// Success
	return true;
}

void C4Object::SetCommand(int32_t iCommand, C4Object *pTarget, C4Value iTx, int32_t iTy,
	C4Object *pTarget2, bool fControl, int32_t iData,
	int32_t iRetries, const char *szText)
{
	// Decrease NoCollectDelay
	if (NoCollectDelay > 0) NoCollectDelay--;
	// Clear stack
	ClearCommands();
	// Close menu
	if (fControl)
		if (!CloseMenu(false)) return;
	// Script overload
	if (fControl)
		if (Call(PSF_ControlCommand, {C4VString(CommandName(iCommand)),
			C4VObj(pTarget),
			iTx,
			C4VInt(iTy),
			C4VObj(pTarget2),
			C4VInt(iData)}))
			return;
	// Inside vehicle control overload
	if (Contained)
		if (Contained->Def->VehicleControl & C4D_VehicleControl_Inside)
		{
			Contained->Controller = Controller;
			if (Contained->Call(PSF_ControlCommand, {C4VString(CommandName(iCommand)),
				C4VObj(pTarget),
				iTx,
				C4VInt(iTy),
				C4VObj(pTarget2),
				C4VInt(iData),
				C4VObj(this)}))
				return;
		}
	// Outside vehicle control overload
	if (GetProcedure() == DFA_PUSH)
		if (Action.Target) if (Action.Target->Def->VehicleControl & C4D_VehicleControl_Outside)
		{
			Action.Target->Controller = Controller;
			if (Action.Target->Call(PSF_ControlCommand, {C4VString(CommandName(iCommand)),
				C4VObj(pTarget),
				iTx,
				C4VInt(iTy),
				C4VObj(pTarget2),
				C4VInt(iData)}))
				return;
		}
	// Add new command
	AddCommand(iCommand, pTarget, iTx, iTy, 0, pTarget2, true, iData, false, iRetries, szText, C4CMD_Mode_Base);
}

C4Command *C4Object::FindCommand(int32_t iCommandType)
{
	// seek all commands
	for (C4Command *pCom = Command; pCom; pCom = pCom->Next)
		if (pCom->Command == iCommandType) return pCom;
	// nothing found
	return nullptr;
}

bool C4Object::ExecuteCommand()
{
	// Execute first command
	if (Command) Command->Execute();
	// Command finished: engine call
	if (Command && Command->Finished)
		Call(PSF_ControlCommandFinished, {C4VString(CommandName(Command->Command)), C4VObj(Command->Target), Command->Tx, C4VInt(Command->Ty), C4VObj(Command->Target2), C4Value(Command->Data, C4V_Any)});
	// Clear finished commands
	while (Command && Command->Finished) ClearCommand(Command);
	// Done
	return true;
}

void C4Object::AddMaterialContents(int32_t iMaterial, int32_t iAmount)
{
	// Add amount
	if (!Inside<int32_t>(iMaterial, 0, C4MaxMaterial)) return;
	MaterialContents[iMaterial] += iAmount;
}

void C4Object::DigOutMaterialCast(bool fRequest)
{
	// Check material contents for sufficient object cast amounts
	for (int32_t iMaterial = 0; iMaterial < Section->Material.Num; iMaterial++)
		if (MaterialContents[iMaterial])
			if (Section->Material.Map[iMaterial].Dig2Object != C4ID_None)
				if (Section->Material.Map[iMaterial].Dig2ObjectRatio != 0)
					if (fRequest || !Section->Material.Map[iMaterial].Dig2ObjectOnRequestOnly)
						if (MaterialContents[iMaterial] >= Section->Material.Map[iMaterial].Dig2ObjectRatio)
						{
							Section->CreateObject(Section->Material.Map[iMaterial].Dig2Object, this, NO_OWNER, x, y + Shape.y + Shape.Hgt, Random(360));
							MaterialContents[iMaterial] = 0;
						}
}

void C4Object::DrawCommand(C4Facet &cgoBar, int32_t iAlign, const char *szFunctionFormat,
	int32_t iCom, C4RegionList *pRegions, int32_t iPlayer,
	const char *szDesc, C4Facet *pfctImage)
{
	const char *cpDesc = szDesc;
	C4ID idDescImage = id;
	C4Def *pDescImageDef = nullptr;
	int32_t iDescImagePhase = 0;
	C4Facet cgoLeft, cgoRight;
	bool fFlash = false;

	// Flash
	C4Player *pPlr;
	if (pPlr = Game.Players.Get(Owner))
		if (iCom == pPlr->FlashCom)
			fFlash = true;

	// Get desc from def script function desc
	if (szFunctionFormat)
		cpDesc = Def->Script.GetControlDesc(szFunctionFormat, iCom, &idDescImage, &iDescImagePhase);

	// Image def by id
	if (idDescImage && idDescImage != C4ID_Contents)
		pDescImageDef = Game.Defs.ID2Def(idDescImage);

	// Symbol sections
	cgoRight = cgoBar.TruncateSection(iAlign);
	if (!cgoRight.Wdt) return;
	if (iAlign & C4FCT_Bottom) cgoLeft = cgoRight.TruncateSection(C4FCT_Left);
	else cgoLeft = cgoBar.TruncateSection(iAlign);
	if (!cgoLeft.Wdt) return;

	// image drawn by caller
	if (pfctImage)
		*pfctImage = cgoRight;
	// Specified def
	else if (pDescImageDef)
		pDescImageDef->Draw(cgoRight, false, Color, nullptr, iDescImagePhase); // ...use specified color, but not object.
	// Contents image
	else if (idDescImage == C4ID_Contents)
	{
		// contents object
		C4Object *pContents = Contents.GetObject();
		if (pContents)
			pContents->DrawPicture(cgoRight);
		else
			DrawPicture(cgoRight);
	}
	// Picture
	else
		DrawPicture(cgoRight);

	// Command
	if (!fFlash || Tick35 > 15)
		DrawCommandKey(cgoLeft, iCom, false,
			Config.Graphics.ShowCommandKeys ? PlrControlKeyName(iPlayer, Com2Control(iCom), true).c_str() : nullptr);

	// Region (both symbols)
	if (pRegions)
		pRegions->Add(cgoLeft.X, cgoLeft.Y, cgoLeft.Wdt * 2, cgoLeft.Hgt, cpDesc ? cpDesc : GetName(), iCom);
}

void C4Object::Resort()
{
	// Flag resort
	Unsorted = true;
	Section->ResortAnyObject = true;
	// Must not immediately resort - link change/removal would crash C4Section::ExecObjects
}

bool C4Object::SetAction(int32_t iAct, C4Object *pTarget, C4Object *pTarget2, int32_t iCalls, bool fForce)
{
	int32_t iLastAction = Action.Act;
	int32_t iLastPhase = Action.Phase;
	C4ActionDef *pAction;

	// Def lost actmap: idle (safety)
	if (!Def->ActMap) iLastAction = ActIdle;

	// No other action
	if (iLastAction > ActIdle)
		if (Def->ActMap[iLastAction].NoOtherAction && !fForce)
			if (iAct != iLastAction)
				return false;

	// Invalid action
	if (Def && !Inside<int32_t>(iAct, ActIdle, Def->ActNum - 1))
		return false;

	// Stop previous act sound
	if (iLastAction > ActIdle)
		if (iAct != iLastAction)
			if (Def->ActMap[iLastAction].Sound[0])
				StopSoundEffect(Def->ActMap[iLastAction].Sound, this);

	// Unfullcon objects no action
	if (Con < FullCon)
		if (!Def->IncompleteActivity)
			iAct = ActIdle;

	// Reset action time on change
	if (iAct != iLastAction)
	{
		Action.Time = 0;
		// reset action data if procedure is changed
		if (((iAct > ActIdle) ? Def->ActMap[iAct].Procedure : DFA_NONE)
			!= ((iLastAction > ActIdle) ? Def->ActMap[iLastAction].Procedure : DFA_NONE))
			Action.Data = 0;
	}

	// Set new action
	Action.Act = iAct;
	std::fill(Action.Name, std::end(Action.Name), '\0');
	if (Action.Act > ActIdle) SCopy(Def->ActMap[Action.Act].Name, Action.Name);
	Action.Phase = Action.PhaseDelay = 0;

	// Set target if specified
	if (pTarget) Action.Target = pTarget;
	if (pTarget2) Action.Target2 = pTarget2;

	// Set Action Facet
	UpdateActionFace();

	// update flipdir
	if (((iLastAction > ActIdle) ? Def->ActMap[iLastAction].FlipDir : 0)
		!= ((iAct > ActIdle) ? Def->ActMap[iAct].FlipDir : 0)) UpdateFlipDir();

	// Start act sound
	if (Action.Act > ActIdle)
		if (Action.Act != iLastAction)
			if (Def->ActMap[Action.Act].Sound[0])
				StartSoundEffect(Def->ActMap[Action.Act].Sound, +1, 100, this);

	// Reset OCF
	SetOCF();

	// Reset fixed position...
	fix_x = itofix(x); fix_y = itofix(y);

	// issue calls

	// Execute StartCall
	if (iCalls & SAC_StartCall)
		if (Action.Act > ActIdle)
		{
			pAction = &(Def->ActMap[Action.Act]);
			if (pAction->StartCall)
			{
				C4Def *pOldDef = Def;
				pAction->StartCall->Exec(*Section, this);
				// abort exeution if def changed
				if (Def != pOldDef || !Status) return true;
			}
		}

	// Execute EndCall
	if (iCalls & SAC_EndCall && !fForce)
		if (iLastAction > ActIdle)
		{
			pAction = &(Def->ActMap[iLastAction]);
			if (pAction->EndCall)
			{
				C4Def *pOldDef = Def;
				pAction->EndCall->Exec(*Section, this);
				// abort exeution if def changed
				if (Def != pOldDef || !Status) return true;
			}
		}

	// Execute AbortCall
	if (iCalls & SAC_AbortCall && !fForce)
		if (iLastAction > ActIdle)
		{
			pAction = &(Def->ActMap[iLastAction]);
			if (pAction->AbortCall)
			{
				C4Def *pOldDef = Def;
				pAction->AbortCall->Exec(*Section, this, {C4VInt(iLastPhase)});
				// abort exeution if def changed
				if (Def != pOldDef || !Status) return true;
			}
		}

	return true;
}

void C4Object::UpdateActionFace()
{
	// Default: no action face
	Action.Facet.Default();
	// Active: get action facet from action definition
	if (Action.Act > ActIdle)
	{
		C4ActionDef *pAction = &(Def->ActMap[Action.Act]);
		if (pAction->Facet.Wdt > 0)
		{
			Action.Facet.Set(GetGraphics()->GetBitmap(Color), pAction->Facet.x, pAction->Facet.y, pAction->Facet.Wdt, pAction->Facet.Hgt);
			Action.FacetX = pAction->Facet.tx;
			Action.FacetY = pAction->Facet.ty;
		}
	}
}

bool C4Object::SetActionByName(const char *szActName,
	C4Object *pTarget, C4Object *pTarget2,
	int32_t iCalls, bool fForce)
{
	int32_t cnt;
	// Check for ActIdle passed by name
	if (SEqual(szActName, "ActIdle") || SEqual(szActName, "Idle"))
		return SetAction(ActIdle, nullptr, nullptr, SAC_StartCall | SAC_AbortCall, fForce);
	// Find act in ActMap of object
	for (cnt = 0; cnt < Def->ActNum; cnt++)
		if (SEqual(szActName, Def->ActMap[cnt].Name))
			return SetAction(cnt, pTarget, pTarget2, iCalls, fForce);
	return false;
}

void C4Object::SetDir(int32_t iDir)
{
	// Not active
	if (Action.Act <= ActIdle) return;
	// Invalid direction
	if (!Inside<int32_t>(iDir, 0, Def->ActMap[Action.Act].Directions - 1)) return;
	// Execute turn action
	C4ActionDef *pAction = &(Def->ActMap[Action.Act]);
	if (iDir != Action.Dir)
		if (pAction->TurnAction[0])
		{
			SetActionByName(pAction->TurnAction);
		}
	// Set dir
	Action.Dir = iDir;
	// update by flipdir?
	if (Def->ActMap[Action.Act].FlipDir)
		UpdateFlipDir();
	else
		Action.DrawDir = iDir;
}

int32_t C4Object::GetProcedure()
{
	if (Action.Act <= ActIdle) return DFA_NONE;
	return Def->ActMap[Action.Act].Procedure;
}

void GrabLost(C4Object *cObj)
{
	// Grab lost script call on target (quite hacky stuff...)
	cObj->Action.Target->Call(PSF_GrabLost);
	// Clear commands down to first PushTo (if any) in command stack
	for (C4Command *pCom = cObj->Command; pCom; pCom = pCom->Next)
		if (pCom->Next && pCom->Next->Command == C4CMD_PushTo)
		{
			cObj->ClearCommand(pCom);
			break;
		}
}

void DoGravity(C4Object *cobj, bool fFloatFriction = true);

void C4Object::NoAttachAction()
{
	// Active objects
	if (Action.Act > ActIdle)
	{
		int32_t iProcedure = GetProcedure();
		// Scaling upwards: corner scale
		if (iProcedure == DFA_SCALE && Action.ComDir != COMD_Stop && ComDirLike(Action.ComDir, COMD_Up))
			if (ObjectActionCornerScale(this)) return;
		if (iProcedure == DFA_SCALE && Action.ComDir == COMD_Left && Action.Dir == DIR_Left)
			if (ObjectActionCornerScale(this)) return;
		if (iProcedure == DFA_SCALE && Action.ComDir == COMD_Right && Action.Dir == DIR_Right)
			if (ObjectActionCornerScale(this)) return;
		// Scaling and stopped: fall off to side (avoid zuppel)
		if ((iProcedure == DFA_SCALE) && (Action.ComDir == COMD_Stop))
		{
			if (Action.Dir == DIR_Left)
			{
				if (ObjectActionJump(this, itofix(1), Fix0, false)) return;
			}
			else
			{
				if (ObjectActionJump(this, itofix(-1), Fix0, false)) return;
			}
		}
		// Pushing: grab loss
		if (iProcedure == DFA_PUSH) GrabLost(this);
		// Fighting: Set last energy loss player for kill tracing (pushing people off a cliff)
		else if (iProcedure == DFA_FIGHT && Action.Target) LastEnergyLossCausePlayer = Action.Target->Controller;
		// Else jump
		ObjectActionJump(this, xdir, ydir, false);
	}
	// Inactive objects, simple mobile natural gravity
	else
	{
		DoGravity(this);
		Mobile = 1;
	}
}

bool ContactVtxCNAT(C4Object *cobj, uint8_t cnat_dir);

void C4Object::ContactAction()
{
	// Take certain action on contact. Evaluate t_contact-CNAT and Procedure.
	C4Fixed last_xdir;

	int32_t iDir;
	C4PhysicalInfo *pPhysical = GetPhysical();

	// Determine Procedure
	if (Action.Act <= ActIdle) return;
	int32_t iProcedure = Def->ActMap[Action.Act].Procedure;
	int32_t fDisabled = Def->ActMap[Action.Act].Disabled;

	// Hit Bottom
	if (t_contact & CNAT_Bottom)
		switch (iProcedure)
		{
		case DFA_FLIGHT:
			if (ydir < 0) break;
			// Jump: FlatHit / HardHit / Walk
			if ((OCF & OCF_HitSpeed4) || fDisabled)
				if (ObjectActionFlat(this, Action.Dir)) return;
			if (OCF & OCF_HitSpeed3)
				if (ObjectActionKneel(this)) return;
			// Walk, but keep horizontal momentum (momentum is reset
			// by ObjectActionWalk) to avoid walk-jump-flipflop on
			// sideways corner hit if initial walk acceleration is
			// not enough to reach the next pixel for attachment.
			// Urks, all those special cases...
			last_xdir = xdir;
			ObjectActionWalk(this);
			xdir = last_xdir;
			return;
		case DFA_SCALE:
			// Scale up: try corner scale
			if (!ComDirLike(Action.ComDir, COMD_Down))
			{
				if (ObjectActionCornerScale(this)) return;
				return;
			}
			// Any other: Stand
			ObjectActionStand(this);
			return;
		case DFA_DIG:
			// Redirect downleft/downright
			if (Action.ComDir == COMD_DownLeft)
			{
				Action.ComDir = COMD_Left; break;
			}
			if (Action.ComDir == COMD_DownRight)
			{
				Action.ComDir = COMD_Right; break;
			}
			// Else stop
			ObjectComStopDig(this);
			return;
		case DFA_SWIM:
			// Try corner scale out
			if (!Section->Landscape.GBackLiquid(x, y - 1))
				if (ObjectActionCornerScale(this)) return;
			return;
		}

	// Hit Ceiling
	if (t_contact & CNAT_Top)
		switch (iProcedure)
		{
		case DFA_WALK:
			// Walk: Stop
			ObjectActionStand(this); return;
		case DFA_SCALE:
			// Scale: Try hangle, else stop if going upward
			if (ComDirLike(Action.ComDir, COMD_Up))
			{
				if (pPhysical->CanHangle)
				{
					iDir = DIR_Left;
					if (Action.Dir == DIR_Left) { iDir = DIR_Right; }
					ObjectActionHangle(this, iDir); return;
				}
				else
					Action.ComDir = COMD_Stop;
			}
			break;
		case DFA_FLIGHT:
			// Jump: Try hangle, else bounce off
			// High Speed Flight: Tumble
			if ((OCF & OCF_HitSpeed3) || fDisabled)
			{
				ObjectActionTumble(this, Action.Dir, Fix0, Fix0); break;
			}
			if (pPhysical->CanHangle)
			{
				ObjectActionHangle(this, Action.Dir); return;
			}
			break;
		case DFA_DIG:
			// Dig: Stop
			ObjectComStopDig(this); return;
		case DFA_HANGLE:
			Action.ComDir = COMD_Stop;
			break;
		}

	// Hit Left Wall
	if (t_contact & CNAT_Left)
	{
		switch (iProcedure)
		{
		case DFA_FLIGHT:
			// High Speed Flight: Tumble
			if ((OCF & OCF_HitSpeed3) || fDisabled)
			{
				ObjectActionTumble(this, DIR_Left, FIXED100(+150), Fix0); break;
			}
			// Else
			else if (pPhysical->CanScale)
			{
				ObjectActionScale(this, DIR_Left); return;
			}
			break;
		case DFA_WALK:
			// Walk: Try scale, else stop
			if (ComDirLike(Action.ComDir, COMD_Left))
			{
				if (pPhysical->CanScale)
				{
					ObjectActionScale(this, DIR_Left); return;
				}
				// Else stop
				Action.ComDir = COMD_Stop;
			}
			// Heading away from solid
			if (ComDirLike(Action.ComDir, COMD_Right))
			{
				// Slide off
				ObjectActionJump(this, xdir / 2, ydir, false);
			}
			return;
		case DFA_SWIM:
			// Try scale
			if (ComDirLike(Action.ComDir, COMD_Left))
				if (pPhysical->CanScale)
				{
					ObjectActionScale(this, DIR_Left); return;
				}
			// Try corner scale out
			if (ObjectActionCornerScale(this)) return;
			return;
		case DFA_HANGLE:
			// Hangle: Try scale, else stop
			if (pPhysical->CanScale)
				if (ObjectActionScale(this, DIR_Left))
					return;
			Action.ComDir = COMD_Stop;
			return;
		case DFA_DIG:
			// Dig: Stop
			ObjectComStopDig(this);
			return;
		}
	}

	// Hit Right Wall
	if (t_contact & CNAT_Right)
	{
		switch (iProcedure)
		{
		case DFA_FLIGHT:
			// High Speed Flight: Tumble
			if ((OCF & OCF_HitSpeed3) || fDisabled)
			{
				ObjectActionTumble(this, DIR_Right, FIXED100(-150), Fix0); break;
			}
			// Else Scale
			else if (pPhysical->CanScale)
			{
				ObjectActionScale(this, DIR_Right); return;
			}
			break;
		case DFA_WALK:
			// Walk: Try scale, else stop
			if (ComDirLike(Action.ComDir, COMD_Right))
			{
				if (pPhysical->CanScale)
				{
					ObjectActionScale(this, DIR_Right); return;
				}
				Action.ComDir = COMD_Stop;
			}
			// Heading away from solid
			if (ComDirLike(Action.ComDir, COMD_Left))
			{
				// Slide off
				ObjectActionJump(this, xdir / 2, ydir, false);
			}
			return;
		case DFA_SWIM:
			// Try scale
			if (ComDirLike(Action.ComDir, COMD_Right))
				if (pPhysical->CanScale)
				{
					ObjectActionScale(this, DIR_Right); return;
				}
			// Try corner scale out
			if (ObjectActionCornerScale(this)) return;
			// Skip to enable walk out
			return;
		case DFA_HANGLE:
			// Hangle: Try scale, else stop
			if (pPhysical->CanScale)
				if (ObjectActionScale(this, DIR_Right))
					return;
			Action.ComDir = COMD_Stop;
			return;
		case DFA_DIG:
			// Dig: Stop
			ObjectComStopDig(this);
			return;
		}
	}

	// Unresolved Cases

	// Flight stuck
	if (iProcedure == DFA_FLIGHT)
	{
		// Enforce slide free (might slide through tiny holes this way)
		if (!ydir)
		{
			bool fAllowDown = !(t_contact & CNAT_Bottom);
			if (t_contact & CNAT_Right)
			{
				ForcePosition(x - 1, y + fAllowDown);
				xdir = ydir = 0;
			}
			if (t_contact & CNAT_Left)
			{
				ForcePosition(x + 1, y + fAllowDown);
				xdir = ydir = 0;
			}
		}
		if (!xdir)
		{
			if (t_contact & CNAT_Top)
			{
				ForcePosition(x, y + 1);
				xdir = ydir = 0;
			}
		}
	}
}

void Towards(C4Fixed &val, C4Fixed target, C4Fixed step)
{
	if (val == target) return;
	if (Abs(val - target) <= step) { val = target; return; }
	if (val < target) val += step; else val -= step;
}

bool DoBridge(C4Object *clk)
{
	int32_t iBridgeTime; bool fMoveClonk, fWall; int32_t iBridgeMaterial;
	clk->Action.GetBridgeData(iBridgeTime, fMoveClonk, fWall, iBridgeMaterial);
	if (!iBridgeTime) iBridgeTime = 100; // default bridge time
	if (clk->Action.Time >= iBridgeTime) { ObjectActionStand(clk); return false; }
	// get bridge advancement
	int32_t dtp;
	if (fWall) switch (clk->Action.ComDir)
	{
	case COMD_Left: case COMD_Right: dtp = 4; fMoveClonk = false; break; // vertical wall: default 25 pixels
	case COMD_UpLeft: case COMD_UpRight: dtp = 5; fMoveClonk = false; break; // diagonal roof over Clonk: default 20 pixels up and 20 pixels side (28 pixels - optimized to close tunnels completely)
	case COMD_Up: dtp = 5; break; // horizontal roof over Clonk
	default: return true; // bridge procedure just for show
	}
	else switch (clk->Action.ComDir)
	{
	case COMD_Left: case COMD_Right: dtp = 5; break; // horizontal bridges: default 20 pixels
	case COMD_Up: dtp = 4; break; // vertical bridges: default 25 pixels (same as
	case COMD_UpLeft: case COMD_UpRight: dtp = 6; break; // diagonal bridges: default 16 pixels up and 16 pixels side (23 pixels)
	default: return true; // bridge procedure just for show
	}
	if (clk->Action.Time % dtp) return true; // no advancement in this frame
	// get target pos for Clonk and bridge
	int32_t cx = clk->x, cy = clk->y, cw = clk->Shape.Wdt, ch = clk->Shape.Hgt;
	int32_t tx = cx, ty = cy + ch / 2;
	int32_t dt;
	if (fMoveClonk) dt = 0; else dt = clk->Action.Time / dtp;
	if (fWall) switch (clk->Action.ComDir)
	{
	case COMD_Left: tx -= cw / 2; ty += -dt; break;
	case COMD_Right: tx += cw / 2; ty += -dt; break;
	case COMD_Up:
	{
		int32_t x0;
		if (fMoveClonk) x0 = -3; else x0 = (iBridgeTime / dtp) / -2;
		tx += (x0 + dt) * ((clk->Action.Dir == DIR_Right) * 2 - 1); cx += ((clk->Action.Dir == DIR_Right) * 2 - 1); ty -= ch + 3; break;
	}
	case COMD_UpLeft: tx -= -4 + dt; ty += -ch - 7 + dt; break;
	case COMD_UpRight: tx += -4 + dt; ty += -ch - 7 + dt; break;
	}
	else switch (clk->Action.ComDir)
	{
	case COMD_Left: tx += -2 - dt; --cx; break;
	case COMD_Right: tx += +2 + dt; ++cx; break;
	case COMD_Up: tx += (-cw / 2 + (cw - 1) * (clk->Action.Dir == DIR_Right)) * (!fMoveClonk); ty += -dt - fMoveClonk; --cy; break;
	case COMD_UpLeft: tx += -5 - dt + fMoveClonk * 3; ty += 2 - dt - fMoveClonk * 3; --cx; --cy; break;
	case COMD_UpRight: tx += +5 + dt - fMoveClonk * 2; ty += 2 - dt - fMoveClonk * 3; ++cx; --cy; break;
	}
	// check if Clonk movement is posible
	if (fMoveClonk)
	{
		int32_t cx2 = cx, cy2 = cy;
		if (clk->Shape.CheckContact(clk->Section->Landscape, cx2, cy2 - 1))
		{
			// Clonk would collide here: Change to nonmoving Clonk mode and redo bridging
			iBridgeTime -= clk->Action.Time;
			clk->Action.Time = 0;
			if (fWall && clk->Action.ComDir == COMD_Up)
			{
				// special for roof above Clonk: The nonmoving roof is started at bridgelength before the Clonkl
				// so, when interrupted, an action time halfway through the action must be set
				clk->Action.Time = iBridgeTime;
				iBridgeTime += iBridgeTime;
			}
			clk->Action.SetBridgeData(iBridgeTime, false, fWall, iBridgeMaterial);
			return DoBridge(clk);
		}
	}
	// draw bridge into landscape
	clk->Section->Landscape.DrawMaterialRect(iBridgeMaterial, tx - 2, ty, 4, 3);
	// Move Clonk
	if (fMoveClonk) clk->MovePosition(cx - clk->x, cy - clk->y);
	return true;
}

void DoGravity(C4Object *cobj, bool fFloatFriction)
{
	// Floatation in liquids
	if (cobj->InLiquid && cobj->Def->Float)
	{
		cobj->ydir -= FloatAccel;
		if (cobj->ydir < FloatAccel * -10) cobj->ydir = FloatAccel * -10;
		if (fFloatFriction)
		{
			if (cobj->xdir < -FloatFriction) cobj->xdir += FloatFriction;
			if (cobj->xdir > +FloatFriction) cobj->xdir -= FloatFriction;
			if (cobj->rdir < -FloatFriction) cobj->rdir += FloatFriction;
			if (cobj->rdir > +FloatFriction) cobj->rdir -= FloatFriction;
		}
		if (!cobj->Section->Landscape.GBackLiquid(cobj->x, cobj->y - 1 + cobj->Def->Float * cobj->GetCon() / FullCon - 1))
			if (cobj->ydir < 0) cobj->ydir = 0;
	}
	// Free fall gravity
	else if (~cobj->Category & C4D_StaticBack)
		cobj->ydir += cobj->Section->Landscape.Gravity;
}

void StopActionDelayCommand(C4Object *cobj)
{
	ObjectComStop(cobj);
	cobj->AddCommand(C4CMD_Wait, nullptr, 0, 0, 50);
}

bool ReduceLineSegments(C4Section &section, C4Shape &rShape, bool fAlternate)
{
	// try if line could go by a path directly when skipping on evertex. If fAlternate is true, try by skipping two vertices
	for (int32_t cnt = 0; cnt + 2 + fAlternate < rShape.VtxNum; cnt++)
		if (section.Landscape.PathFree(rShape.VtxX[cnt], rShape.VtxY[cnt],
			rShape.VtxX[cnt + 2 + fAlternate], rShape.VtxY[cnt + 2 + fAlternate]))
		{
			if (fAlternate) rShape.RemoveVertex(cnt + 2);
			rShape.RemoveVertex(cnt + 1);
			return true;
		}
	return false;
}

void C4Object::ExecAction()
{
	Action.t_attach = CNAT_None;
	uint32_t ocf;
	C4Fixed iTXDir;
	C4Fixed lftspeed, tydir;
	int32_t iTargetX;
	int32_t iPushRange, iPushDistance;

	// Standard phase advance
	int32_t iPhaseAdvance = 1;

	// Upright attachment check
	if (!Mobile)
		if (Def->UprightAttach)
			if (Inside<int32_t>(r, -StableRange, +StableRange))
			{
				Action.t_attach |= Def->UprightAttach;
				Mobile = 1;
			}

	// Idle objects do natural gravity only
	if (Action.Act <= ActIdle)
	{
		if (Mobile) DoGravity(this);
		return;
	}

	// No IncompleteActivity? Reset action
	if (!(OCF & OCF_FullCon) && !Def->IncompleteActivity)
	{
		SetAction(ActIdle); return;
	}

	// Determine ActDef & Physical Info
	C4ActionDef *pAction = &(Def->ActMap[Action.Act]);
	C4PhysicalInfo *pPhysical = GetPhysical();
	C4Fixed lLimit;
	C4Fixed fWalk, fMove;
	int32_t smpx, smpy;

	// Energy usage
	if (Game.Rules & C4RULE_StructuresNeedEnergy)
		if (pAction->EnergyUsage)
			if (pAction->EnergyUsage <= Energy)
			{
				Energy -= pAction->EnergyUsage;
				// No general DoEnergy-Process
				NeedEnergy = 0;
			}
			// Insufficient energy for action: same as idle
			else
			{
				NeedEnergy = 1;
				if (Mobile) DoGravity(this);
				return;
			}

	// Action time advance
	Action.Time++;

	// InLiquidAction check
	if (InLiquid)
		if (pAction->InLiquidAction[0])
		{
			SetActionByName(pAction->InLiquidAction); return;
		}

	// assign extra action attachment (CNAT_MultiAttach)
	// regular attachment values cannot be set for backwards compatibility reasons
	// this parameter had always been ignored for actions using an internal procedure,
	// but is for some obscure reasons set in the KneelDown-actions of the golems
	Action.t_attach |= (pAction->Attach & CNAT_MultiAttach);

	// if an object is in controllable state, so it can be assumed that if it dies later because of NO_OWNER's cause,
	// it has been its own fault and not the fault of the last one who threw a flint on it
	// do not reset for burning objects to make sure the killer is set correctly if they fall out of the map while burning
	// also keep state when swimming, so you get kills for pushing people into the lava/acid/shark lake
	if (!pAction->Disabled && pAction->Procedure != DFA_FLIGHT && pAction->Procedure != DFA_SWIM && !OnFire)
		LastEnergyLossCausePlayer = NO_OWNER;

	// Handle Default Action Procedure: evaluates Procedure and Action.ComDir
	// Update xdir,ydir,Action.Dir,attachment,iPhaseAdvance
	switch (pAction->Procedure)
	{
	case DFA_WALK:
		lLimit = ValByPhysical(280, pPhysical->Walk);
		switch (Action.ComDir)
		{
		case COMD_Left: case COMD_UpLeft: case COMD_DownLeft:
			xdir -= WalkAccel; if (xdir < -lLimit) xdir = -lLimit;
			break;
		case COMD_Right: case COMD_UpRight: case COMD_DownRight:
			xdir += WalkAccel; if (xdir > +lLimit) xdir = +lLimit;
			break;
		case COMD_Stop: case COMD_Up: case COMD_Down:
			if (xdir < 0) xdir += WalkAccel;
			if (xdir > 0) xdir -= WalkAccel;
			if ((xdir > -WalkAccel) && (xdir < +WalkAccel)) xdir = 0;
			break;
		}
		iPhaseAdvance = 0;
		if (xdir < 0) { iPhaseAdvance = -fixtoi(xdir * 10); SetDir(DIR_Left); }
		if (xdir > 0) { iPhaseAdvance = +fixtoi(xdir * 10); SetDir(DIR_Right); }
		Action.t_attach |= CNAT_Bottom;
		Mobile = 1;
		// object is rotateable? adjust to ground, if in horizontal movement or not attached to the center vertex
		if (Def->Rotateable && Shape.AttachMat != MNone && (!!xdir || Def->Shape.VtxX[Shape.iAttachVtx]))
			AdjustWalkRotation(20, 20, 100);
		else
			rdir = 0;
		break;

	case DFA_KNEEL:
		ydir = 0;
		Action.t_attach |= CNAT_Bottom;
		Mobile = 1;
		break;

	case DFA_SCALE:
	{
		lLimit = ValByPhysical(200, pPhysical->Scale);

		// Physical training
		if (!Tick5)
			if (Abs(ydir) == lLimit)
				TrainPhysical(&C4PhysicalInfo::Scale, 1, C4MaxPhysical);
		int ComDir = Action.ComDir;
		if (Action.Dir == DIR_Left && ComDir == COMD_Left)
			ComDir = COMD_Up;
		else if (Action.Dir == DIR_Right && ComDir == COMD_Right)
			ComDir = COMD_Up;
		switch (ComDir)
		{
		case COMD_Up: case COMD_UpRight: case COMD_UpLeft:
			ydir -= WalkAccel; if (ydir < -lLimit) ydir = -lLimit; break;
		case COMD_Down: case COMD_DownRight: case COMD_DownLeft:
			ydir += WalkAccel; if (ydir > +lLimit) ydir = +lLimit; break;
		case COMD_Left: case COMD_Right: case COMD_Stop:
			if (ydir < 0) ydir += WalkAccel;
			if (ydir > 0) ydir -= WalkAccel;
			if ((ydir > -WalkAccel) && (ydir < +WalkAccel)) ydir = 0;
			break;
		}
		iPhaseAdvance = 0;
		if (ydir < 0) iPhaseAdvance = -fixtoi(ydir * 14);
		if (ydir > 0) iPhaseAdvance = +fixtoi(ydir * 14);
		xdir = 0;
		if (Action.Dir == DIR_Left)  Action.t_attach |= CNAT_Left;
		if (Action.Dir == DIR_Right) Action.t_attach |= CNAT_Right;
		Mobile = 1;
		break;
	}

	case DFA_HANGLE:
		lLimit = ValByPhysical(160, pPhysical->Hangle);

		// Physical training
		if (!Tick5)
			if (Abs(xdir) == lLimit)
				TrainPhysical(&C4PhysicalInfo::Hangle, 1, C4MaxPhysical);

		switch (Action.ComDir)
		{
		case COMD_Left: case COMD_UpLeft: case COMD_DownLeft:
			xdir -= WalkAccel; if (xdir < -lLimit) xdir = -lLimit;
			break;
		case COMD_Right: case COMD_UpRight: case COMD_DownRight:
			xdir += WalkAccel; if (xdir > +lLimit) xdir = +lLimit;
			break;
		case COMD_Up:
			xdir += (Action.Dir == DIR_Left) ? -WalkAccel : WalkAccel;
			if (xdir < -lLimit) xdir = -lLimit;
			if (xdir > +lLimit) xdir = +lLimit;
			break;
		case COMD_Stop: case COMD_Down:
			if (xdir < 0) xdir += WalkAccel;
			if (xdir > 0) xdir -= WalkAccel;
			if ((xdir > -WalkAccel) && (xdir < +WalkAccel)) xdir = 0;
			break;
		}
		iPhaseAdvance = 0;
		if (xdir < 0) { iPhaseAdvance = -fixtoi(xdir * 10); SetDir(DIR_Left); }
		if (xdir > 0) { iPhaseAdvance = +fixtoi(xdir * 10); SetDir(DIR_Right); }
		ydir = 0;
		Action.t_attach |= CNAT_Top;
		Mobile = 1;
		break;

	case DFA_FLIGHT:
		// Contained: fall out (one try only)
		if (!Tick10)
			if (Contained)
			{
				StopActionDelayCommand(this);
				SetCommand(C4CMD_Exit);
			}
		// Gravity/mobile
		DoGravity(this);
		Mobile = 1;
		break;

	case DFA_DIG:
		smpx = x; smpy = y;
		if (!Shape.Attach(*Section, smpx, smpy, CNAT_Bottom))
		{
			ObjectComStopDig(this); return;
		}
		lLimit = ValByPhysical(125, pPhysical->Dig);
		iPhaseAdvance = fixtoi(lLimit * 40);
		switch (Action.ComDir)
		{
		case COMD_Up:
			ydir = -lLimit / 2;
			if (Action.Dir == DIR_Left) xdir = -lLimit;
			else xdir = +lLimit;
			break;
		case COMD_UpLeft:    xdir = -lLimit; ydir = -lLimit / 2; break;
		case COMD_Left:      xdir = -lLimit; ydir = 0;           break;
		case COMD_DownLeft:  xdir = -lLimit; ydir = +lLimit;     break;
		case COMD_Down:      xdir = 0;       ydir = +lLimit;     break;
		case COMD_DownRight: xdir = +lLimit; ydir = +lLimit;     break;
		case COMD_Right:     xdir = +lLimit; ydir = 0;           break;
		case COMD_UpRight:   xdir = +lLimit; ydir = -lLimit / 2; break;
		case COMD_Stop:
			xdir = 0; ydir = 0;
			iPhaseAdvance = 0;
			break;
		}
		if (xdir < 0) SetDir(DIR_Left); else if (xdir > 0) SetDir(DIR_Right);
		Action.t_attach = CNAT_None;
		Mobile = 1;
		break;

	case DFA_SWIM:
		lLimit = ValByPhysical(160, pPhysical->Swim);

		// Physical training
		if (!Tick10)
			if (Abs(xdir) == lLimit)
				TrainPhysical(&C4PhysicalInfo::Swim, 1, C4MaxPhysical);

		// ComDir changes xdir/ydir
		switch (Action.ComDir)
		{
		case COMD_Up:        ydir -= SwimAccel;                    break;
		case COMD_UpRight:   ydir -= SwimAccel; xdir += SwimAccel; break;
		case COMD_Right:                        xdir += SwimAccel; break;
		case COMD_DownRight: ydir += SwimAccel; xdir += SwimAccel; break;
		case COMD_Down:      ydir += SwimAccel;                    break;
		case COMD_DownLeft:  ydir += SwimAccel; xdir -= SwimAccel; break;
		case COMD_Left:                         xdir -= SwimAccel; break;
		case COMD_UpLeft:    ydir -= SwimAccel; xdir -= SwimAccel; break;
		case COMD_Stop:
			if (xdir < 0) xdir += SwimAccel;
			if (xdir > 0) xdir -= SwimAccel;
			if ((xdir > -SwimAccel) && (xdir < +SwimAccel)) xdir = 0;
			if (ydir < 0) ydir += SwimAccel;
			if (ydir > 0) ydir -= SwimAccel;
			if ((ydir > -SwimAccel) && (ydir < +SwimAccel)) ydir = 0;
			break;
		}

		// Out of liquid check
		if (!InLiquid)
		{
			// Just above liquid: move down
			if (Section->Landscape.GBackLiquid(x, y + 1 + Def->Float * Con / FullCon - 1)) ydir = +SwimAccel;
			// Free fall: walk
			else { ObjectActionWalk(this); return; }
		}

		// xdir/ydir bounds
		if (ydir < -lLimit) ydir = -lLimit; if (ydir > +lLimit) ydir = +lLimit;
		if (xdir > +lLimit) xdir = +lLimit; if (xdir < -lLimit) xdir = -lLimit;
		// Surface dir bound
		if (!Section->Landscape.GBackLiquid(x, y - 1 + Def->Float * Con / FullCon - 1)) if (ydir < 0) ydir = 0;
		// Dir, Phase, Attach
		if (xdir < 0) SetDir(DIR_Left);
		if (xdir > 0) SetDir(DIR_Right);
		iPhaseAdvance = fixtoi(lLimit * 10);
		Action.t_attach = CNAT_None;
		Mobile = 1;

		break;

	case DFA_THROW:
		ydir = 0; xdir = 0;
		Action.t_attach |= CNAT_Bottom;
		Mobile = 1;
		break;

	case DFA_BRIDGE:
	{
		if (!DoBridge(this)) return;
		switch (Action.ComDir)
		{
		case COMD_Left:  case COMD_UpLeft:  SetDir(DIR_Left); break;
		case COMD_Right: case COMD_UpRight: SetDir(DIR_Right); break;
		}
		ydir = 0; xdir = 0;
		Action.t_attach |= CNAT_Bottom;
		Mobile = 1;
	}
	break;

	case DFA_BUILD:
		// Woa, structures can build without target
		if ((Category & C4D_Structure) || (Category & C4D_StaticBack))
			if (!Action.Target) break;
		// No target
		if (!Action.Target) { ObjectComStop(this); return; }
		// Target internal: container needs to support by own DFA_BUILD
		if (Action.Target->Contained)
			if ((Action.Target->Contained->GetProcedure() != DFA_BUILD)
				|| (Action.Target->Contained->NeedEnergy))
				return;
		// Build speed
		int32_t iLevel;
		// Clonk-standard
		iLevel = 10;
		// Internal builds slower
		if (Action.Target->Contained) iLevel = 1;
		// Out of target area: stop
		if (!Inside<int32_t>(x - (Action.Target->x + Action.Target->Shape.x), 0, Action.Target->Shape.Wdt)
			|| !Inside<int32_t>(y - (Action.Target->y + Action.Target->Shape.y), -16, Action.Target->Shape.Hgt + 16))
		{
			ObjectComStop(this); return;
		}
		// Build target
		if (!Action.Target->Build(iLevel, this))
		{
			// Cannot build because target is complete (or removed, ugh): we're done
			if (!Action.Target || Action.Target->Con >= FullCon)
			{
				// Stop
				ObjectComStop(this);
				// Exit target if internal
				if (Action.Target) if (Action.Target->Contained == this)
					Action.Target->SetCommand(C4CMD_Exit);
			}
			// Cannot build because target needs material (assumeably)
			else
			{
				// Stop
				ObjectComStop(this);
			}
			return;
		}
		xdir = ydir = 0;
		Action.t_attach |= CNAT_Bottom;
		Mobile = 1;
		break;

	case DFA_PUSH:
		// No target
		if (!Action.Target) { StopActionDelayCommand(this); return; }
		// Inside target
		if (Contained == Action.Target) { StopActionDelayCommand(this); return; }
		// Target pushing force
		bool fStraighten;
		iTXDir = 0; fStraighten = false;
		lLimit = ValByPhysical(280, pPhysical->Walk);
		switch (Action.ComDir)
		{
		case COMD_Left: case COMD_DownLeft:   iTXDir = -lLimit; break;
		case COMD_UpLeft:  fStraighten = 1;   iTXDir = -lLimit; break;
		case COMD_Right: case COMD_DownRight: iTXDir = +lLimit; break;
		case COMD_UpRight: fStraighten = 1;   iTXDir = +lLimit; break;
		case COMD_Up:      fStraighten = 1;                     break;
		case COMD_Stop: case COMD_Down:       iTXDir = 0;       break;
		}
		// Push object
		if (!Action.Target->Push(iTXDir, ValByPhysical(250, pPhysical->Push), fStraighten))
		{
			StopActionDelayCommand(this); return;
		}
		// Set target controller
		Action.Target->Controller = Controller;
		// ObjectAction got hold check
		iPushDistance = (std::max)(Shape.Wdt / 2 - 8, 0);
		iPushRange = iPushDistance + 10;
		int32_t sax, say, sawdt, sahgt;
		Action.Target->GetArea(sax, say, sawdt, sahgt);
		// Object lost
		if (!Inside(x - sax, -iPushRange, sawdt - 1 + iPushRange)
			|| !Inside(y - say, -iPushRange, sahgt - 1 + iPushRange))
		{
			// Wait command (why, anyway?)
			StopActionDelayCommand(this);
			// Grab lost action
			GrabLost(this);
			// Done
			return;
		}
		// Follow object (full xdir reset)
		// Vertical follow: If object moves out at top, assume it's being pushed upwards and the Clonk must run after it
		if (y - iPushDistance > say + sahgt && iTXDir) { if (iTXDir > 0) sax += sawdt / 2; sawdt /= 2; }
		// Horizontal follow
		iTargetX = BoundBy(x, sax - iPushDistance, sax + sawdt - 1 + iPushDistance);
		if (x == iTargetX) xdir = 0;
		else { if (x < iTargetX) xdir = +lLimit; if (x > iTargetX) xdir = -lLimit; }
		// Phase by XDir
		if (xdir < 0) { iPhaseAdvance = -fixtoi(xdir * 10); SetDir(DIR_Left); }
		if (xdir > 0) { iPhaseAdvance = +fixtoi(xdir * 10); SetDir(DIR_Right); }
		// No YDir
		ydir = 0;
		// Attachment
		Action.t_attach |= CNAT_Bottom;
		// Mobile
		Mobile = 1;
		break;

	case DFA_PULL:
		// No target
		if (!Action.Target) { StopActionDelayCommand(this); return; }
		// Inside target
		if (Contained == Action.Target) { StopActionDelayCommand(this); return; }
		// Target contained
		if (Action.Target->Contained) { StopActionDelayCommand(this); return; }

		int32_t iPullDistance;
		int32_t iPullX;

		iPullDistance = Action.Target->Shape.Wdt / 2 + Shape.Wdt / 2;

		iTargetX = x;
		if (Action.ComDir == COMD_Right) iTargetX = Action.Target->x + iPullDistance;
		if (Action.ComDir == COMD_Left) iTargetX = Action.Target->x - iPullDistance;

		iPullX = Action.Target->x;
		if (Action.ComDir == COMD_Right) iPullX = x - iPullDistance;
		if (Action.ComDir == COMD_Left) iPullX = x + iPullDistance;

		fWalk = ValByPhysical(280, pPhysical->Walk);

		fMove = 0;
		if (Action.ComDir == COMD_Right) fMove = +fWalk;
		if (Action.ComDir == COMD_Left) fMove = -fWalk;

		iTXDir = fMove + fWalk * BoundBy<int32_t>(iPullX - Action.Target->x, -10, +10) / 10;

		// Push object
		if (!Action.Target->Push(iTXDir, ValByPhysical(250, pPhysical->Push), false))
		{
			StopActionDelayCommand(this); return;
		}
		// Set target controller
		Action.Target->Controller = Controller;

		// Train pulling: com dir transfer
		if ((Action.Target->GetProcedure() == DFA_WALK)
			|| (Action.Target->GetProcedure() == DFA_PULL))
		{
			Action.Target->Action.ComDir = COMD_Stop;
			if (iTXDir < 0) Action.Target->Action.ComDir = COMD_Left;
			if (iTXDir > 0) Action.Target->Action.ComDir = COMD_Right;
		}

		// Pulling range
		iPushDistance = (std::max)(Shape.Wdt / 2 - 8, 0);
		iPushRange = iPushDistance + 20;
		Action.Target->GetArea(sax, say, sawdt, sahgt);
		// Object lost
		if (!Inside(x - sax, -iPushRange, sawdt - 1 + iPushRange)
			|| !Inside(y - say, -iPushRange, sahgt - 1 + iPushRange))
		{
			// Wait command (why, anyway?)
			StopActionDelayCommand(this);
			// Grab lost action
			GrabLost(this);
			// Lose target
			Action.Target = nullptr;
			// Done
			return;
		}

		// Move to pulling position
		xdir = fMove + fWalk * BoundBy<int32_t>(iTargetX - x, -10, +10) / 10;

		// Phase by XDir
		iPhaseAdvance = 0;
		if (xdir < 0) { iPhaseAdvance = -fixtoi(xdir * 10); SetDir(DIR_Left); }
		if (xdir > 0) { iPhaseAdvance = +fixtoi(xdir * 10); SetDir(DIR_Right); }
		// No YDir
		ydir = 0;
		// Attachment
		Action.t_attach |= CNAT_Bottom;
		// Mobile
		Mobile = 1;

		break;

	case DFA_CHOP:
		// Valid check
		if (!Action.Target) { ObjectActionStand(this); return; }
		// Chop
		if (!Tick3)
			if (!Action.Target->Chop(this))
			{
				ObjectActionStand(this); return;
			}
		// Valid check (again, target might have been destroyed)
		if (!Action.Target) { ObjectActionStand(this); return; }
		// AtObject check
		ocf = OCF_Chop;
		if (!Action.Target->At(x, y, ocf)) { ObjectActionStand(this); return; }
		// Position
		SetDir((x > Action.Target->x) ? DIR_Left : DIR_Right);
		xdir = ydir = 0;
		Action.t_attach |= CNAT_Bottom;
		Mobile = 1;
		break;

	case DFA_FIGHT:
		// Valid check
		if (!Action.Target || (Action.Target->GetProcedure() != DFA_FIGHT))
		{
			ObjectActionStand(this); return;
		}

		// Fighting through doors only if doors open
		if (Action.Target->Contained != Contained)
			if ((Contained && !Contained->EntranceStatus) || (Action.Target->Contained && !Action.Target->Contained->EntranceStatus))
			{
				ObjectActionStand(this); return;
			}

		// Physical training
		if (!Tick5)
			TrainPhysical(&C4PhysicalInfo::Fight, 1, C4MaxPhysical);

		// Direction
		if (Action.Target->x > x) SetDir(DIR_Right);
		if (Action.Target->x < x) SetDir(DIR_Left);
		// Position
		iTargetX = x;
		if (Action.Dir == DIR_Left)  iTargetX = Action.Target->x + Action.Target->Shape.Wdt / 2 + 2;
		if (Action.Dir == DIR_Right) iTargetX = Action.Target->x - Action.Target->Shape.Wdt / 2 - 2;
		lLimit = ValByPhysical(95, pPhysical->Walk);
		if (x == iTargetX) Towards(xdir, Fix0, lLimit);
		if (x < iTargetX) Towards(xdir, +lLimit, lLimit);
		if (x > iTargetX) Towards(xdir, -lLimit, lLimit);
		// Distance check
		if ((Abs(x - Action.Target->x) > Shape.Wdt)
			|| (Abs(y - Action.Target->y) > Shape.Wdt))
		{
			ObjectActionStand(this); return;
		}
		// Other
		Action.t_attach |= CNAT_Bottom;
		ydir = 0;
		Mobile = 1;
		// Experience
		if (!Tick35) DoExperience(+2);
		break;

	case DFA_LIFT:
		// Valid check
		if (!Action.Target) { SetAction(ActIdle); return; }
		// Target lifting force
		lftspeed = itofix(2); tydir = 0;
		switch (Action.ComDir)
		{
		case COMD_Up:   tydir = -lftspeed; break;
		case COMD_Stop: tydir = -Section->Landscape.Gravity; break;
		case COMD_Down: tydir = +lftspeed; break;
		}
		// Lift object
		if (!Action.Target->Lift(tydir, FIXED100(50)))
		{
			SetAction(ActIdle); return;
		}
		// Check LiftTop
		if (Def->LiftTop)
			if (Action.Target->y <= (y + Def->LiftTop))
				if (Action.ComDir == COMD_Up)
					Call(PSF_LiftTop);
		// General
		DoGravity(this);
		break;

	case DFA_FLOAT:
		// Float speed
		lLimit = FIXED100(pPhysical->Float);
		// ComDir changes xdir/ydir
		switch (Action.ComDir)
		{
		case COMD_Up:    ydir -= FloatAccel; break;
		case COMD_Down:  ydir += FloatAccel; break;
		case COMD_Right: xdir += FloatAccel; break;
		case COMD_Left:  xdir -= FloatAccel; break;
		case COMD_UpRight:   ydir -= FloatAccel; xdir += FloatAccel; break;
		case COMD_DownRight: ydir += FloatAccel; xdir += FloatAccel; break;
		case COMD_DownLeft:  ydir += FloatAccel; xdir -= FloatAccel; break;
		case COMD_UpLeft:    ydir -= FloatAccel; xdir -= FloatAccel; break;
		}
		// xdir/ydir bounds
		if (ydir < -lLimit) ydir = -lLimit; if (ydir > +lLimit) ydir = +lLimit;
		if (xdir > +lLimit) xdir = +lLimit; if (xdir < -lLimit) xdir = -lLimit;
		Mobile = 1;
		break;

	// ATTACH: Force position to target object
	//   own vertex index is determined by high-order byte of action data
	//   target vertex index is determined by low-order byte of action data
	case DFA_ATTACH:
		// No target
		if (!Action.Target)
		{
			if (Status)
			{
				SetAction(ActIdle);
				Call(PSF_AttachTargetLost);
			}
			return;
		}

		// Target incomplete and no incomplete activity
		if (!(Action.Target->OCF & OCF_FullCon))
			if (!Action.Target->Def->IncompleteActivity)
			{
				SetAction(ActIdle); return;
			}

		// Force containment
		if (Action.Target->Contained != Contained)
		{
			if (Action.Target->Contained)
				Enter(Action.Target->Contained);
			else
				Exit(x, y, r);
			// Target might be lost in Enter/Exit callback
			if (!Action.Target)
			{
				if (Status)
				{
					SetAction(ActIdle);
					Call(PSF_AttachTargetLost);
				}
				return;
			}
		}

		// Force position
		ForcePosition(Action.Target->x + Action.Target->Shape.VtxX[Action.Data & 255]
			- Shape.VtxX[Action.Data >> 8],
			Action.Target->y + Action.Target->Shape.VtxY[Action.Data & 255]
			- Shape.VtxY[Action.Data >> 8]);
		// must zero motion...
		xdir = ydir = 0;

		break;

	case DFA_CONNECT:
		bool fBroke;
		fBroke = false;
		int32_t iConnectX, iConnectY;

		// Line destruction check: Target missing or incomplete
		if (!Action.Target  || (Action.Target->Con  < FullCon)) fBroke = true;
		if (!Action.Target2 || (Action.Target2->Con < FullCon)) fBroke = true;
		if (fBroke)
		{
			Call(PSF_LineBreak, {C4VBool(true)});
			AssignRemoval();
			return;
		}

		// Movement by Target
		if (Action.Target)
		{
			// Connect to vertex
			if (Def->Line == C4D_Line_Vertex)
			{
				iConnectX = Action.Target->x + Action.Target->Shape.GetVertexX(Local[2].getInt());
				iConnectY = Action.Target->y + Action.Target->Shape.GetVertexY(Local[2].getInt());
			}
			// Connect to bottom center
			else
			{
				iConnectX = Action.Target->x;
				iConnectY = Action.Target->y + Action.Target->Shape.Hgt / 4;
			}
			if ((iConnectX != Shape.VtxX[0]) || (iConnectY != Shape.VtxY[0]))
			{
				// Regular wrapping line
				if (Def->LineIntersect == 0)
					if (!Shape.LineConnect(Section->Landscape, iConnectX, iConnectY, 0, +1,
						Shape.VtxX[0], Shape.VtxY[0])) fBroke = true;
				// No-intersection line
				if (Def->LineIntersect == 1)
				{
					Shape.VtxX[0] = iConnectX; Shape.VtxY[0] = iConnectY;
				}
			}
		}
		// Movement by Target2
		if (Action.Target2)
		{
			// Connect to vertex
			if (Def->Line == C4D_Line_Vertex)
			{
				iConnectX = Action.Target2->x + Action.Target2->Shape.GetVertexX(Local[3].getInt());
				iConnectY = Action.Target2->y + Action.Target2->Shape.GetVertexY(Local[3].getInt());
			}
			// Connect to bottom center
			else
			{
				iConnectX = Action.Target2->x;
				iConnectY = Action.Target2->y + Action.Target2->Shape.Hgt / 4;
			}
			if ((iConnectX != Shape.VtxX[Shape.VtxNum - 1]) || (iConnectY != Shape.VtxY[Shape.VtxNum - 1]))
			{
				// Regular wrapping line
				if (Def->LineIntersect == 0)
					if (!Shape.LineConnect(Section->Landscape, iConnectX, iConnectY, Shape.VtxNum - 1, -1,
						Shape.VtxX[Shape.VtxNum - 1], Shape.VtxY[Shape.VtxNum - 1])) fBroke = true;
				// No-intersection line
				if (Def->LineIntersect == 1)
				{
					Shape.VtxX[Shape.VtxNum - 1] = iConnectX; Shape.VtxY[Shape.VtxNum - 1] = iConnectY;
				}
			}
		}

		// Line fBroke
		if (fBroke)
		{
			Call(PSF_LineBreak);
			AssignRemoval();
			return;
		}

		// Reduce line segments
		if (!Tick35)
			ReduceLineSegments(*Section, Shape, !Tick2);

		break;

	default:
		// Attach
		if (pAction->Attach)
		{
			Action.t_attach |= pAction->Attach;
			xdir = ydir = 0;
			Mobile = 1;
		}
		// Free gravity
		else
			DoGravity(this);
		break;
	}

	// Phase Advance (zero delay means no phase advance)
	if (pAction->Delay)
	{
		Action.PhaseDelay += iPhaseAdvance;
		if (Action.PhaseDelay >= pAction->Delay)
		{
			// Advance Phase
			Action.PhaseDelay = 0;
			Action.Phase += pAction->Step;
			// Phase call
			if (pAction->PhaseCall)
			{
				pAction->PhaseCall->Exec(*Section, this);
			}
			// Phase end
			if (Action.Phase >= pAction->Length)
			{
				// set new action if it's not Hold
				if (pAction->NextAction == ActHold)
					Action.Phase = pAction->Length - 1;
				else
					// Set new action
					SetAction(pAction->NextAction, nullptr, nullptr, SAC_StartCall | SAC_EndCall);
			}
		}
	}

	return;
}

bool C4Object::SetOwner(int32_t iOwner)
{
	C4Player *pPlr;
	// Check valid owner
	if (!(ValidPlr(iOwner) || iOwner == NO_OWNER)) return false;
	// always set color, even if no owner-change is done
	if (iOwner != NO_OWNER)
		if (GetGraphics()->IsColorByOwner())
		{
			Color = Game.Players.Get(iOwner)->ColorDw;
			UpdateFace(false);
		}
	// no change?
	if (Owner == iOwner) return true;
	// remove old owner view
	if (ValidPlr(Owner))
	{
		pPlr = Game.Players.Get(Owner);
		while (pPlr->FoWViewObjs.Remove(this));
	}
	else
		for (pPlr = Game.Players.First; pPlr; pPlr = pPlr->Next)
			while (pPlr->FoWViewObjs.Remove(this));
	// set new owner
	int32_t iOldOwner = Owner;
	Owner = iOwner;
	if (Owner != NO_OWNER)
		// add to plr view
		PlrFoWActualize();
	// this automatically updates controller
	Controller = Owner;
	// if this is a flag flying on a base, the base must be updated
	if (id == C4ID_Flag) if (SEqual(Action.Name, "FlyBase")) if (Action.Target && Action.Target->Status)
		if (Action.Target->Base == iOldOwner)
		{
			Action.Target->Base = Owner;
		}
	// script callback
	Call(PSF_OnOwnerChanged, {C4VInt(Owner), C4VInt(iOldOwner)});
	// done
	return true;
}

bool C4Object::SetPlrViewRange(int32_t iToRange)
{
	// set new range
	PlrViewRange = iToRange;
	// resort into player's FoW-repeller-list
	PlrFoWActualize();
	// success
	return true;
}

void C4Object::PlrFoWActualize()
{
	C4Player *pPlr;
	// single owner?
	if (ValidPlr(Owner))
	{
		// single player's FoW-list
		pPlr = Game.Players.Get(Owner);
		while (pPlr->FoWViewObjs.Remove(this));
		if (PlrViewRange) pPlr->FoWViewObjs.Add(this, C4ObjectList::stNone);
	}
	// no owner?
	else
	{
		// all players!
		for (pPlr = Game.Players.First; pPlr; pPlr = pPlr->Next)
		{
			while (pPlr->FoWViewObjs.Remove(this));
			if (PlrViewRange) pPlr->FoWViewObjs.Add(this, C4ObjectList::stNone);
		}
	}
}

void C4Object::SetAudibilityAt(C4FacetEx &cgo, int32_t iX, int32_t iY)
{
	if (Category & C4D_Parallax)
	{
		Audible = 0;
		// target pos (parallax)
		int32_t cotx = cgo.TargetX, coty = cgo.TargetY; TargetPos(cotx, coty, cgo);
		Audible = std::max<int32_t>(Audible, BoundBy(100 - 100 * Distance(cotx + cgo.Wdt / 2, coty + cgo.Hgt / 2, iX, iY) / C4SoundSystem::AudibilityRadius, 0, 100));
		AudiblePan = BoundBy(AudiblePan + (iX - (cotx + cgo.Wdt / 2)) / 5, -100, 100);
	}
	else
	{
		Audible = Game.GraphicsSystem.GetAudibility(*Section, iX, iY, &AudiblePan);
	}
}

int32_t C4Object::GetAudibility()
{
	if (Audible == -1)
	{
		Audible = Game.GraphicsSystem.GetAudibility(*Section, x, y, &AudiblePan);
	}
	return Audible;
}

int32_t C4Object::GetAudiblePan()
{
	GetAudibility();
	return AudiblePan;
}

bool C4Object::IsVisible(int32_t iForPlr, bool fAsOverlay)
{
	bool fDraw;
	// check overlay
	if (Visibility & VIS_OverlayOnly)
	{
		if (!fAsOverlay) return false;
		if (Visibility == VIS_OverlayOnly) return true;
	}
	// check layer
	if (pLayer && pLayer != this && !fAsOverlay)
	{
		fDraw = pLayer->IsVisible(iForPlr, false);
		if (pLayer->Visibility & VIS_LayerToggle) fDraw = !fDraw;
		if (!fDraw) return false;
	}
	// no flags set?
	if (!Visibility) return true;
	// check visibility
	fDraw = false;
	if (Visibility & VIS_Owner) fDraw = fDraw || (iForPlr == Owner);
	if (iForPlr != NO_OWNER)
	{
		// check all
		if (Visibility & VIS_Allies)  fDraw = fDraw || (iForPlr != Owner && !Hostile(iForPlr, Owner));
		if (Visibility & VIS_Enemies) fDraw = fDraw || (iForPlr != Owner && Hostile(iForPlr, Owner));
		if (Visibility & VIS_Local)   fDraw = fDraw || (Local[iForPlr / 32].getInt() & (1 << (iForPlr % 32)));
	}
	else fDraw = fDraw || (Visibility & VIS_God);
	return fDraw;
}

bool C4Object::IsInLiquidCheck()
{
	return Section->Landscape.GBackLiquid(x, y + Def->Float * Con / FullCon - 1);
}

void C4Object::SetRotation(int32_t nr)
{
	while (nr < 0) nr += 360; nr %= 360;
	// remove solid mask
	if (pSolidMaskData) pSolidMaskData->Remove(true, false);
	// set rotation
	r = nr;
	fix_r = itofix(nr);
	// Update face
	UpdateFace(true);
}

void C4Object::PrepareDrawing()
{
	// color modulation
	if (ColorMod || (BlitMode & (C4GFXBLIT_MOD2 | C4GFXBLIT_CLRSFC_MOD2))) Application.DDraw->ActivateBlitModulation(ColorMod);
	// other blit modes
	Application.DDraw->SetBlitMode(BlitMode);
}

void C4Object::FinishedDrawing()
{
	// color modulation
	Application.DDraw->DeactivateBlitModulation();
	// extra blitting flags
	Application.DDraw->ResetBlitMode();
}

void C4Object::UpdateSolidMask(bool fRestoreAttachedObjects)
{
	// solidmask doesn't make sense with non-existent objects
	// (the solidmask has already been destroyed in AssignRemoval -
	//  do not reset it!)
	if (!Status) return;
	// Determine necessity, update cSolidMask, put or remove mask
	// Mask if enabled, fullcon, no rotation, not contained
	if (SolidMask.Wdt > 0)
		if (Con >= FullCon)
			if (!Contained)
				if (!r || Def->RotatedSolidmasks)
				{
					// Recheck and put mask
					if (!pSolidMaskData)
					{
						pSolidMaskData = new C4SolidMask(this);
					}
					else
						pSolidMaskData->Remove(true, false);
					pSolidMaskData->Put(true, nullptr, fRestoreAttachedObjects);
					return;
				}
	// Otherwise, remove and destroy mask
	if (pSolidMaskData) pSolidMaskData->Remove(true, false);
	delete pSolidMaskData; pSolidMaskData = nullptr;
}

bool C4Object::Collect(C4Object *pObj)
{
	// Special: attached Flag may not be collectable
	if (pObj->Def->id == C4ID_Flag)
		if (!(Game.Rules & C4RULE_FlagRemoveable))
			if (pObj->Action.Act > ActIdle)
				if (SEqual(pObj->Def->ActMap[pObj->Action.Act].Name, "FlyBase"))
					return false;
	// Object enter container
	bool fRejectCollect;
	if (!pObj->Enter(this, true, false, &fRejectCollect))
		return false;
	// Cancel attach (hacky)
	ObjectComCancelAttach(pObj);
	// Container Collection call
	Call(PSF_Collection, {C4VObj(pObj)});
	// Object Hit call
	if (pObj->Status && pObj->OCF & OCF_HitSpeed1) pObj->Call(PSF_Hit);
	if (pObj->Status && pObj->OCF & OCF_HitSpeed2) pObj->Call(PSF_Hit2);
	if (pObj->Status && pObj->OCF & OCF_HitSpeed3) pObj->Call(PSF_Hit3);
	// post-copy the motion of the new container
	if (pObj->Contained == this) pObj->CopyMotion(this);
	// done, success
	return true;
}

bool C4Object::GrabInfo(C4Object *pFrom)
{
	// safety
	if (!pFrom) return false; if (!Status || !pFrom->Status) return false;
	// even more safety (own info: success)
	if (pFrom == this) return true;
	// only if other object has info
	if (!pFrom->Info) return false;
	// clear own info object
	if (Info)
	{
		Info->Retire();
		ClearInfo(Info);
	}
	// remove objects from any owning crews
	Game.Players.ClearPointers(pFrom);
	Game.Players.ClearPointers(this);
	// set info
	Info = pFrom->Info; pFrom->ClearInfo(pFrom->Info);
	// retire from old crew
	Info->Retire();
	// set death status
	Info->HasDied = !Alive;
	// if alive, recruit to new crew
	if (Alive) Info->Recruit();
	// make new crew member
	C4Player *pPlr = Game.Players.Get(Owner);
	if (pPlr) pPlr->MakeCrewMember(this);
	// done, success
	return true;
}

bool C4Object::ShiftContents(bool fShiftBack, bool fDoCalls)
{
	// get current object
	C4Object *c_obj = Contents.GetObject();
	if (!c_obj) return false;
	// get next/previous
	C4ObjectLink *pLnk = fShiftBack ? (Contents.Last) : (Contents.First->Next);
	for (;;)
	{
		// end reached without success
		if (!pLnk) return false;
		// check object
		C4Object *pObj = pLnk->Obj;
		if (pObj->Status)
			if (!c_obj->CanConcatPictureWith(pObj))
			{
				// object different: shift to this
				DirectComContents(pObj, !!fDoCalls);
				return true;
			}
		// next/prev item
		pLnk = fShiftBack ? (pLnk->Prev) : (pLnk->Next);
	}
	// not reached
}

void C4Object::DirectComContents(C4Object *pTarget, bool fDoCalls)
{
	// safety
	if (!pTarget || !pTarget->Status || pTarget->Contained != this) return;
	// Desired object already at front?
	if (Contents.GetObject() == pTarget) return;
	// select object via script?
	if (fDoCalls)
		if (Call("~ControlContents", {C4VID(pTarget->id)}))
			return;
	// default action
	if (!(Contents.ShiftContents(pTarget))) return;
	// Selection sound
	if (fDoCalls) if (!Contents.GetObject()->Call("~Selection", {C4VObj(this)})) StartSoundEffect("Grab", false, 100, this);
	// update menu with the new item in "put" entry
	if (Menu && Menu->IsActive() && Menu->IsContextMenu())
	{
		Menu->Refill();
	}
	// Done
	return;
}

void C4Object::ApplyParallaxity(int32_t &riTx, int32_t &riTy, const C4Facet &fctViewport)
{
	// parallaxity by locals
	// special: Negative positions with parallaxity 0 mean HUD elements positioned to the right/bottom
	int iParX = Local[0].getInt(), iParY = Local[1].getInt();
	if (!iParX && x < 0)
		riTx = -fctViewport.Wdt;
	else
		riTx = riTx * iParX / 100;
	if (!iParY && y < 0)
		riTy = -fctViewport.Hgt;
	else
		riTy = riTy * iParY / 100;
}

bool C4Object::DoSelect(bool fCursor)
{
	// selection allowed?
	if (CrewDisabled) return true;
	// select
	if (!fCursor) Select = 1;
	// do callback
	Call(PSF_CrewSelection, {C4VBool(false), C4VBool(fCursor)});
	// done
	return true;
}

void C4Object::UnSelect(bool fCursor)
{
	// unselect
	if (!fCursor) Select = 0;
	// do callback
	Call(PSF_CrewSelection, {C4VBool(true), C4VBool(fCursor)});
}

void C4Object::GetViewPos(int32_t &riX, int32_t &riY, int32_t tx, int32_t ty, const C4Facet &fctViewport) // get position this object is seen at (for given scroll)
{
	if (Category & C4D_Parallax)
	{
		GetViewPosPar(riX, riY, tx, ty, fctViewport);
	}
	else
	{
		riX = x; riY = y;
	}

	Section->PointToParentPoint(riX, riY);
}

void C4Object::GetViewPosPar(int32_t &riX, int32_t &riY, int32_t tx, int32_t ty, const C4Facet &fctViewport)
{
	int iParX = Local[0].getInt(), iParY = Local[1].getInt();
	// get drawing pos, then subtract original target pos to get drawing pos on landscape
	if (!iParX && x < 0)
		// HUD element at right viewport pos
		riX = x + tx + fctViewport.Wdt;
	else
		// regular parallaxity
		riX = x - (tx * (iParX - 100) / 100);
	if (!iParY && y < 0)
		// HUD element at bottom viewport pos
		riY = y + ty + fctViewport.Hgt;
	else
		// regular parallaxity
		riY = y - (ty * (iParY - 100) / 100);
}

bool C4Object::PutAwayUnusedObject(C4Object *pToMakeRoomForObject)
{
	// get unused object
	C4Object *pUnusedObject;
	C4AulFunc *pFnObj2Drop;
	if (pFnObj2Drop = Def->Script.GetSFunc(PSF_GetObject2Drop))
		pUnusedObject = pFnObj2Drop->Exec(*Section, this, pToMakeRoomForObject ? C4AulParSet{C4VObj(pToMakeRoomForObject)} : C4AulParSet{}).getObj();
	else
	{
		// is there any unused object to put away?
		if (!Contents.Last) return false;
		// defaultly, it's the last object in the list
		// (contents list cannot have invalid status-objects)
		pUnusedObject = Contents.Last->Obj;
	}
	// no object to put away? fail
	if (!pUnusedObject) return false;
	// grabbing something?
	bool fPushing = (GetProcedure() == DFA_PUSH);
	if (fPushing)
		// try to put it in there
		if (ObjectComPut(this, Action.Target, pUnusedObject))
			return true;
	// in container? put in there
	if (Contained)
	{
		// try to put it in directly
		// note that this works too, if an object is grabbed inside the container
		if (ObjectComPut(this, Contained, pUnusedObject))
			return true;
		// now putting didn't work - drop it outside
		AddCommand(C4CMD_Drop, pUnusedObject);
		AddCommand(C4CMD_Exit);
		return true;
	}
	else
		// if uncontained, simply try to drop it
		// if this doesn't work, it won't ever
		return !!ObjectComDrop(this, pUnusedObject);
}

bool C4Object::SetGraphics(const char *szGraphicsName, C4Def *pSourceDef)
{
	// safety
	if (!Status) return false;
	// default def
	if (!pSourceDef) pSourceDef = Def;
	// get graphics
	C4DefGraphics *pGrp = pSourceDef->Graphics.Get(szGraphicsName);
	if (!pGrp) return false;
	// no change? (no updates need to be done, then)
	if (pGraphics == pGrp) return true;
	// set new graphics
	pGraphics = pGrp;
	// update Color, SolidMask, etc.
	UpdateGraphics(true);
	// success
	return true;
}

bool C4Object::SetGraphics(C4DefGraphics *pNewGfx, bool fTemp)
{
	// safety
	if (!pNewGfx) return false;
	// set it and update related stuff
	pGraphics = pNewGfx;
	UpdateGraphics(true, fTemp);
	return true;
}

C4GraphicsOverlay *C4Object::GetGraphicsOverlay(int32_t iForID, bool fCreate)
{
	// search in list until ID is found or passed
	C4GraphicsOverlay *pOverlay = pGfxOverlay, *pPrevOverlay = nullptr;
	while (pOverlay && pOverlay->GetID() < iForID) { pPrevOverlay = pOverlay; pOverlay = pOverlay->GetNext(); }
	// exact match found?
	if (pOverlay && pOverlay->GetID() == iForID) return pOverlay;
	// ID has been passed: Create new if desired
	if (!fCreate) return nullptr;
	C4GraphicsOverlay *pNewOverlay = new C4GraphicsOverlay();
	pNewOverlay->SetID(iForID);
	pNewOverlay->SetNext(pOverlay);
	if (pPrevOverlay) pPrevOverlay->SetNext(pNewOverlay); else pGfxOverlay = pNewOverlay;
	// return newly created overlay
	return pNewOverlay;
}

bool C4Object::RemoveGraphicsOverlay(int32_t iOverlayID)
{
	// search in list until ID is found or passed
	C4GraphicsOverlay *pOverlay = pGfxOverlay, *pPrevOverlay = nullptr;
	while (pOverlay && pOverlay->GetID() < iOverlayID) { pPrevOverlay = pOverlay; pOverlay = pOverlay->GetNext(); }
	// exact match found?
	if (pOverlay && pOverlay->GetID() == iOverlayID)
	{
		// remove it
		if (pPrevOverlay) pPrevOverlay->SetNext(pOverlay->GetNext()); else pGfxOverlay = pOverlay->GetNext();
		pOverlay->SetNext(nullptr); // prevents deletion of following overlays
		delete pOverlay;
		// removed
		return true;
	}
	// no match found
	return false;
}

bool C4Object::HasGraphicsOverlayRecursion(const C4Object *pCheckObj) const
{
	C4Object *pGfxOvrlObj;
	if (pGfxOverlay)
		for (C4GraphicsOverlay *pGfxOvrl = pGfxOverlay; pGfxOvrl; pGfxOvrl = pGfxOvrl->GetNext())
			if (pGfxOvrlObj = pGfxOvrl->GetOverlayObject())
			{
				if (pGfxOvrlObj == pCheckObj) return true;
				if (pGfxOvrlObj->HasGraphicsOverlayRecursion(pCheckObj)) return true;
			}
	return false;
}

bool C4Object::StatusActivate()
{
	// readd to main list
	Section->Objects.InactiveObjects.Remove(this);
	Status = C4OS_NORMAL;
	Section->Objects.Add(this);
	// update some values
	UpdateGraphics(false);
	UpdateFace(true);
	UpdatePos();
	Call(PSF_UpdateTransferZone);
	// done, success
	return true;
}

bool C4Object::StatusDeactivate(bool fClearPointers)
{
	// clear particles
	if (FrontParticles) FrontParticles.Clear(Section->Particles.FreeParticles);
	if (BackParticles) BackParticles.Clear(Section->Particles.FreeParticles);
	// put into inactive list
	Section->Objects.Remove(this);
	Status = C4OS_INACTIVE;
	Section->Objects.InactiveObjects.Add(this, C4ObjectList::stMain);
	// if desired, clear game pointers
	if (fClearPointers)
	{
		// in this case, the object must also exit any container, and any contained objects must be exited
		ClearContentsAndContained();
		Game.ClearPointers(this);
	}
	else
	{
		for (const auto &section : Game.Sections)
		{
			section->TransferZones.ClearPointers(this);
		}
	}
	// done, success
	return true;
}

void C4Object::ClearContentsAndContained(bool fDoCalls)
{
	// exit contents from container
	for (auto it = Contents.begin(); it != std::default_sentinel; ++it)
	{
		(*it)->Exit(x, y, 0, Fix0, Fix0, Fix0, fDoCalls);
	}

	// remove from container *after* contents have been removed!
	if (Contained) Exit(x, y, 0, Fix0, Fix0, Fix0, fDoCalls);
}

bool C4Object::AdjustWalkRotation(int32_t iRangeX, int32_t iRangeY, int32_t iSpeed)
{
	int32_t iDestAngle;
	// attachment at middle (bottom) vertex?
	if (Shape.iAttachVtx < 0 || !Def->Shape.VtxX[Shape.iAttachVtx])
	{
		// evaluate floor around attachment pos
		int32_t iSolidLeft = 0, iSolidRight = 0;
		// left
		int32_t iXCheck = Shape.iAttachX - iRangeX;
		if (Section->Landscape.GBackSolid(iXCheck, Shape.iAttachY))
		{
			// up
			while (--iSolidLeft > -iRangeY)
				if (Section->Landscape.GBackSolid(iXCheck, Shape.iAttachY + iSolidLeft))
				{
					++iSolidLeft; break;
				}
		}
		else
			// down
			while (++iSolidLeft < iRangeY)
				if (Section->Landscape.GBackSolid(iXCheck, Shape.iAttachY + iSolidLeft))
				{
					--iSolidLeft; break;
				}
		// right
		iXCheck += 2 * iRangeX;
		if (Section->Landscape.GBackSolid(iXCheck, Shape.iAttachY))
		{
			// up
			while (--iSolidRight > -iRangeY)
				if (Section->Landscape.GBackSolid(iXCheck, Shape.iAttachY + iSolidRight))
				{
					++iSolidRight; break;
				}
		}
		else
			// down
			while (++iSolidRight < iRangeY)
				if (Section->Landscape.GBackSolid(iXCheck, Shape.iAttachY + iSolidRight))
				{
					--iSolidRight; break;
				}
		// calculate destination angle
		// 100% accurate for large values of Pi ;)
		iDestAngle = (iSolidRight - iSolidLeft) * (35 / std::max<int32_t>(iRangeX, 1));
	}
	else
	{
		// attachment at other than horizontal middle vertex: get your feet to the ground!
		// rotate target to large angle is OK, because rotation will stop once the real
		// bottom vertex hits solid ground
		if (Shape.VtxX[Shape.iAttachVtx] > 0)
			iDestAngle = -50;
		else
			iDestAngle = 50;
	}
	// move to destination angle
	if (Abs(iDestAngle - r) > 2)
	{
		rdir = itofix(BoundBy<int32_t>(iDestAngle - r, -15, +15));
		rdir /= (10000 / iSpeed);
	}
	else rdir = 0;
	// done, success
	return true;
}

void C4Object::UpdateInLiquid()
{
	// InLiquid check
	if (IsInLiquidCheck()) // In Liquid
	{
		if (!InLiquid) // Enter liquid
		{
			if (OCF & OCF_HitSpeed2) if (Mass > 3)
				Splash(x, y + 1, (std::min)(Shape.Wdt * Shape.Hgt / 10, 20), this);
			InLiquid = 1;
		}
	}
	else // Out of liquid
	{
		if (InLiquid) // Leave liquid
			InLiquid = 0;
	}
}

void C4Object::AddRef(C4Value *pRef)
{
	pRef->NextRef = FirstRef;
	FirstRef = pRef;
}

void C4Object::DelRef(const C4Value *pRef, C4Value *pNextRef)
{
	// References to objects never have HasBaseArray set
	if (pRef == FirstRef)
		FirstRef = pNextRef;
	else
	{
		C4Value *pVal = FirstRef;
		while (pVal->NextRef && pVal->NextRef != pRef)
			pVal = pVal->NextRef;
		assert(pVal->NextRef);
		pVal->NextRef = pNextRef;
	}
}

StdStrBuf C4Object::GetInfoString()
{
	StdStrBuf sResult;
	// no info for invalid objects
	if (!Status) return sResult;
	// first part always description
	sResult.Copy(Def->GetDesc());
	// go through all effects and add their desc
	for (C4Effect *pEff = pEffects; pEff; pEff = pEff->pNext)
	{
		C4Value vInfo = pEff->DoCall(this, PSFS_FxInfo);
		if (!vInfo) continue;
		// debug: warn for wrong return types
		if (vInfo.GetType() != C4V_String)
			DebugLog(spdlog::level::warn, "Effect {}({}) on object {} (#{}) returned wrong info type {}.", pEff->Name, pEff->iNumber, Def->GetName(), Number, std::to_underlying(vInfo.GetType()));
		// get string val
		C4String *psInfo = vInfo.getStr(); const char *szEffInfo;
		if (psInfo && (szEffInfo = psInfo->Data.getData()))
			if (*szEffInfo)
			{
				// OK; this effect has a desc. Add it!
				if (sResult.getLength()) sResult.AppendChar('|');
				sResult.Append(szEffInfo);
			}
	}
	// done
	return sResult;
}

void C4Object::GrabContents(C4Object *pFrom)
{
	// create a temp list of all objects and transfer it
	// this prevents nasty deadlocks caused by RejectEntrance-scripts
	C4ObjectList tmpList; tmpList.Copy(pFrom->Contents);
	C4ObjectLink *cLnk;
	for (cLnk = tmpList.First; cLnk; cLnk = cLnk->Next)
		if (cLnk->Obj->Status)
			cLnk->Obj->Enter(this);
}

bool C4Object::CanConcatPictureWith(C4Object *pOtherObject)
{
	// check current definition ID
	if (id != pOtherObject->id) return false;
	// def overwrite of stack conditions
	int32_t allow_picture_stack = Def->AllowPictureStack;
	if (!(allow_picture_stack & APS_Color))
	{
		// check color if ColorByOwner (flags)
		if (Color != pOtherObject->Color && Def->ColorByOwner) return false;
		// check modulation
		if (ColorMod != pOtherObject->ColorMod) return false;
		if (BlitMode != pOtherObject->BlitMode) return false;
	}
	if (!(allow_picture_stack & APS_Graphics))
	{
		// check graphics
		if (pGraphics != pOtherObject->pGraphics) return false;
		// check any own picture rect
		if (PictureRect != pOtherObject->PictureRect) return false;
	}
	if (!(allow_picture_stack & APS_Name))
	{
		// check name, so zagabar's sandwiches don't stack
		if (GetName() != pOtherObject->GetName() && std::strcmp(GetName(), pOtherObject->GetName()) != 0) return false;
	}
	if (!(allow_picture_stack & APS_Overlay))
	{
		// check overlay graphics
		for (C4GraphicsOverlay *pOwnOverlay = pGfxOverlay; pOwnOverlay; pOwnOverlay = pOwnOverlay->GetNext())
			if (pOwnOverlay->IsPicture())
			{
				C4GraphicsOverlay *pOtherOverlay = pOtherObject->GetGraphicsOverlay(pOwnOverlay->GetID(), false);
				if (!pOtherOverlay || !(*pOtherOverlay == *pOwnOverlay)) return false;
			}
		for (C4GraphicsOverlay *pOtherOverlay = pOtherObject->pGfxOverlay; pOtherOverlay; pOtherOverlay = pOtherOverlay->GetNext())
			if (pOtherOverlay->IsPicture())
				if (!GetGraphicsOverlay(pOtherOverlay->GetID(), false)) return false;
	}
	// concat OK
	return true;
}

int32_t C4Object::GetFireCausePlr()
{
	// get fire effect
	if (!pEffects) return NO_OWNER;
	C4Effect *pFire = pEffects->Get(C4Fx_Fire);
	if (!pFire) return NO_OWNER;
	// get causing player
	int32_t iFireCausePlr = FxFireVarCausedBy(pFire).getInt();
	// return if valid
	if (ValidPlr(iFireCausePlr)) return iFireCausePlr; else return NO_OWNER;
}

void C4Object::UpdateScriptPointers()
{
	if (pEffects)
		pEffects->ReAssignAllCallbackFunctions();
}

std::string C4Object::GetNeededMatStr(C4Object *pBuilder)
{
	C4Def *pComponent;
	int32_t cnt, ncnt;
	std::string neededMats;

	C4IDList NeededComponents;
	Def->GetComponents(&NeededComponents, *Section, nullptr, pBuilder);

	C4ID idComponent;

	for (cnt = 0; idComponent = NeededComponents.GetID(cnt); cnt++)
	{
		if (NeededComponents.GetCount(cnt) != 0)
		{
			ncnt = NeededComponents.GetCount(cnt) - Component.GetIDCount(idComponent);
			if (ncnt > 0)
			{
				neededMats += std::format("|{}x {}", ncnt, (pComponent = Game.Defs.ID2Def(idComponent)) ? pComponent->GetName() : C4IdText(idComponent));
			}
		}
	}

	if (!neededMats.empty())
	{
		return LoadResStr(C4ResStrTableKey::IDS_CON_BUILDMATNEED, GetName()).append(std::move(neededMats));
	}
	else
	{
		return LoadResStr(C4ResStrTableKey::IDS_CON_BUILDMATNONE, GetName());
	}
}

bool C4Object::IsPlayerObject(int32_t iPlayerNumber)
{
	bool fAnyPlr = (iPlayerNumber == NO_OWNER);
	// if an owner is specified: only owned objects
	if (fAnyPlr && !ValidPlr(Owner)) return false;
	// and crew objects
	if (fAnyPlr || Owner == iPlayerNumber)
	{
		// flags are player objects
		if (id == C4ID_Flag) return true;

		C4Player *pOwner = Game.Players.Get(Owner);
		if (pOwner)
		{
			if (pOwner && pOwner->Crew.IsContained(this)) return true;
		}
		else
		{
			// Do not force that the owner exists because the function must work for unjoined players (savegame resume)
			if (Category & C4D_CrewMember)
				return true;
		}
	}
	// otherwise, not a player object
	return false;
}

bool C4Object::IsUserPlayerObject()
{
	// must be a player object at all
	if (!IsPlayerObject()) return false;
	// and the owner must not be a script player
	C4Player *pOwner = Game.Players.Get(Owner);
	if (!pOwner || pOwner->GetType() != C4PT_User) return false;
	// otherwise, it's a user playeer object
	return true;
}

void C4Object::MoveToSection(C4Section &newSection, const bool checkContained)
{
	if (Section == &newSection) return;

	if (checkContained && Contained && Contained->Section != &newSection)
	{
		Exit(0, 0, 0, Fix0, Fix0, Fix0, false);
	}

	if (pLayer && pLayer->Section != &newSection)
	{
		pLayer = nullptr;
	}

	for (C4ObjectLink *link{Contents.First}; link; link = link->Next)
	{
		link->Obj->MoveToSection(newSection, false);
	}

	if (pSolidMaskData)
	{
		pSolidMaskData->Remove(true, true);
	}

	Section->Objects.OnSectionMove(this, newSection);

	Section->Objects.Remove(this);
	Section = &newSection;
	Section->Objects.Add(this);

	if (pSolidMaskData)
	{
		pSolidMaskData->Put(true, nullptr, true);
	}
}
