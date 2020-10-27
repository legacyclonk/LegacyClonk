/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2004, Sven2
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

// graphics used by object definitions (object and portraits)

#pragma once

#include <C4EnumeratedObjectPtr.h>
#include <C4FacetEx.h>
#include <C4Material.h>
#include <C4Surface.h>

#define C4Portrait_None   "none"
#define C4Portrait_Random "random"
#define C4Portrait_Custom "custom"

// defintion graphics
class C4AdditionalDefGraphics;
class C4DefGraphicsPtrBackup;
class C4PortraitGraphics;

class C4DefGraphics
{
public:
	C4Def *pDef; // underlying definition

protected:
	C4AdditionalDefGraphics *pNext; // next graphics

public:
	C4Surface *Bitmap, *BitmapClr;
	bool fColorBitmapAutoCreated; // if set, the color-by-owner-bitmap has been created automatically by all blue shades of the bitmap

	inline C4Surface *GetBitmap(uint32_t dwClr = 0) { if (BitmapClr) { BitmapClr->SetClr(dwClr); return BitmapClr; } else return Bitmap; }

	C4DefGraphics(C4Def *pOwnDef = nullptr);
	virtual ~C4DefGraphics() { Clear(); }

	bool LoadGraphics(C4Group &hGroup, const char *szFilename, const char *szFilenamePNG, const char *szOverlayPNG, bool fColorByOwner); // load specified graphics from group
	bool LoadAllGraphics(C4Group &hGroup, bool fColorByOwner); // load graphics from group
	bool ColorizeByMaterial(int32_t iMat, C4MaterialMap &rMats, uint8_t bGBM); // colorize all graphics by material
	C4DefGraphics *Get(const char *szGrpName); // get graphics by name
	void Clear(); // clear fields; delete additional graphics

	bool IsColorByOwner() // returns whether ColorByOwner-surfaces have been created
	{
		return !!BitmapClr;
	}

	bool CopyGraphicsFrom(C4DefGraphics &rSource); // copy bitmaps from source graphics

	virtual const char *GetName() { return nullptr; } // return name to be stored in safe game files

	C4AdditionalDefGraphics *GetNext() { return pNext; }
	virtual C4PortraitGraphics *IsPortrait() { return nullptr; }

	void DrawClr(C4Facet &cgo, bool fAspect = true, uint32_t dwClr = 0); // set surface color and draw

	friend class C4DefGraphicsPtrBackup;
};

// additional definition graphics
class C4AdditionalDefGraphics : public C4DefGraphics
{
protected:
	char Name[C4MaxName + 1]; // graphics name

public:
	C4AdditionalDefGraphics(C4Def *pOwnDef, const char *szName);
	virtual const char *GetName() override { return Name; }
};

// portrait graphics within object definition
class C4PortraitGraphics : public C4AdditionalDefGraphics
{
public:
	C4PortraitGraphics(C4Def *pOwnDef, const char *szName)
		: C4AdditionalDefGraphics(pOwnDef, szName) {}

	virtual C4PortraitGraphics *IsPortrait() override { return this; }
	C4PortraitGraphics *GetByIndex(int32_t iIndex);
	C4PortraitGraphics *Get(const char *szGrpName); // get portrait graphics by name
};

// backup class holding dead graphics pointers and names
class C4DefGraphicsPtrBackup
{
protected:
	C4DefGraphics *pGraphicsPtr; // dead graphics ptr
	C4Def *pDef; // definition of dead graphics
	char Name[C4MaxName + 1]; // name of graphics
	C4DefGraphicsPtrBackup *pNext; // next member of linked list

public:
	C4DefGraphicsPtrBackup(C4DefGraphics *pSourceGraphics);
	~C4DefGraphicsPtrBackup();

	void AssignUpdate(C4DefGraphics *pNewGraphics); // update all game objects with new graphics pointers
	void AssignRemoval(); // remove graphics of this def from all game objects
};

// Helper to compile C4DefGraphics-Pointer
class C4DefGraphicsAdapt
{
protected:
	C4DefGraphics *&pDefGraphics;

public:
	C4DefGraphicsAdapt(C4DefGraphics *&pDefGraphics) : pDefGraphics(pDefGraphics) {}
	void CompileFunc(StdCompiler *pComp);

	// Default checking / setting
	bool operator==(C4DefGraphics *pDef2) { return pDefGraphics == pDef2; }
	void operator=(C4DefGraphics *pDef2) { pDefGraphics = pDef2; }
};

// portrait link class
class C4Portrait
{
protected:
	C4DefGraphics *pGfxPortrait; // the portrait graphics
	bool fGraphicsOwned; // if true, the portrait graphics are owned (and deleted upon destruction)

public:
	C4Portrait() : pGfxPortrait(nullptr), fGraphicsOwned(false) {}
	~C4Portrait() { if (fGraphicsOwned) delete pGfxPortrait; }
	void Default() { pGfxPortrait = nullptr; fGraphicsOwned = false; }
	void Clear() { if (fGraphicsOwned) { delete pGfxPortrait; fGraphicsOwned = false; } pGfxPortrait = nullptr; }

	bool CopyFrom(C4DefGraphics &rCopyGfx); // copy portrait from graphics
	bool CopyFrom(C4Portrait &rCopy); // copy portrait

	bool Load(C4Group &rGrp, const char *szFilename, const char *szFilenamePNG, const char *szOverlayPNG); // load own portrait from group
	bool Link(C4DefGraphics *pGfxPortrait); // link with a present portrait surface
	bool SavePNG(C4Group &rGroup, const char *szFilename, const char *szOverlayFN); // store portrait gfx to file (including overlay)

	C4DefGraphics *GetGfx() { return pGfxPortrait; }
	bool IsOwnedGfx() { return fGraphicsOwned; } // return if it's a custom portrait

	static const char *EvaluatePortraitString(const char *szPortrait, C4ID &rIDOut, C4ID idDefaultID, uint32_t *pdwClrOut); // get portrait source and def from string
};

// graphics overlay used to attach additional graphics to objects
class C4GraphicsOverlay
{
public:
	enum Mode
	{
		MODE_None = 0,
		MODE_Base = 1,          // display base facet
		MODE_Action = 2,        // display action facet specified in Action
		MODE_Picture = 3,       // overlay picture to this picture only
		MODE_IngamePicture = 4, // draw picture of source def
		MODE_Object = 5,        // draw another object gfx
		MODE_ExtraGraphics = 6, // draw like this were a ClrByOwner-surface
	};

protected:
	Mode eMode; // overlay mode

	C4DefGraphics *pSourceGfx; // source graphics - used for savegame saving and comparisons in ReloadDef
	char Action[C4MaxName + 1]; // action used as overlay in source gfx
	C4FacetEx fctBlit; // current blit data
	uint32_t dwBlitMode; // extra parameters for additive blits, etc.
	uint32_t dwClrModulation; // colormod for this overlay
#ifdef C4ENGINE
	C4EnumeratedObjectPtr pOverlayObj; // object to be drawn as overlay in MODE_Object
	C4DrawTransform Transform; // drawing transformation: Rotation, zoom, etc.
#endif
	int32_t iPhase; // action face for MODE_Action
	bool fZoomToShape; // if true, overlay will be zoomed to match the target object shape

	int32_t iID; // identification number for Z-ordering and script identification

	C4GraphicsOverlay *pNext; // singly linked list

	void UpdateFacet(); // update fctBlit to reflect current data
	void Set(Mode aMode, C4DefGraphics *pGfx, const char *szAction, uint32_t dwBMode, C4Object *pOvrlObj);

public:
	C4GraphicsOverlay() : eMode(MODE_None), pSourceGfx(nullptr), fctBlit(), dwBlitMode(0), dwClrModulation(0xffffff),
#ifdef C4ENGINE
		Transform(+1),
#endif
		iPhase(0), fZoomToShape(false), iID(0), pNext(nullptr)
	{
		*Action = 0;
	}

	~C4GraphicsOverlay();

	void CompileFunc(StdCompiler *pComp);

	// object pointer management
	void EnumeratePointers();
	void DenumeratePointers();

	void SetAsBase(C4DefGraphics *pBaseGfx, uint32_t dwBMode) // set in MODE_Base
	{
		Set(MODE_Base, pBaseGfx, nullptr, dwBMode, nullptr);
	}

	void SetAsAction(C4DefGraphics *pBaseGfx, const char *szAction, uint32_t dwBMode)
	{
		Set(MODE_Action, pBaseGfx, szAction, dwBMode, nullptr);
	}

	void SetAsPicture(C4DefGraphics *pBaseGfx, uint32_t dwBMode)
	{
		Set(MODE_Picture, pBaseGfx, nullptr, dwBMode, nullptr);
	}

	void SetAsIngamePicture(C4DefGraphics *pBaseGfx, uint32_t dwBMode)
	{
		Set(MODE_IngamePicture, pBaseGfx, nullptr, dwBMode, nullptr);
	}

	void SetAsObject(C4Object *pOverlayObj, uint32_t dwBMode)
	{
		Set(MODE_Object, nullptr, nullptr, dwBMode, pOverlayObj);
	}

	void SetAsExtraGraphics(C4DefGraphics *pGfx, uint32_t dwBMode)
	{
		Set(MODE_ExtraGraphics, pGfx, nullptr, dwBMode, nullptr);
	}

	bool IsValid(const C4Object *pForObj) const;

#ifdef C4ENGINE
	C4DrawTransform *GetTransform() { return &Transform; }
	C4Object *GetOverlayObject() const { return pOverlayObj; }
#endif
	int32_t GetID() const { return iID; }
	void SetID(int32_t aID) { iID = aID; }
	C4GraphicsOverlay *GetNext() const { return pNext; }
	void SetNext(C4GraphicsOverlay *paNext) { pNext = paNext; }
	bool IsPicture() { return eMode == MODE_Picture; }
	C4DefGraphics *GetGfx() const { return pSourceGfx; }

	void Draw(C4FacetEx &cgo, C4Object *pForObj, int32_t iByPlayer);
	void DrawPicture(C4Facet &cgo, C4Object *pForObj);

	bool operator==(const C4GraphicsOverlay &rCmp) const; // comparison operator

	uint32_t GetClrModulation() const { return dwClrModulation; }
	void SetClrModulation(uint32_t dwToMod) { dwClrModulation = dwToMod; }

	uint32_t GetBlitMode() const { return dwBlitMode; }
	void SetBlitMode(uint32_t dwToMode) { dwBlitMode = dwToMode; }
};

// Helper to compile lists of C4GraphicsOverlay
class C4GraphicsOverlayListAdapt
{
protected:
	C4GraphicsOverlay *&pOverlay;

public:
	C4GraphicsOverlayListAdapt(C4GraphicsOverlay *&pOverlay) : pOverlay(pOverlay) {}
	void CompileFunc(StdCompiler *pComp);

	// Default checking / setting
	bool operator==(C4GraphicsOverlay *pDefault) { return pOverlay == pDefault; }
	void operator=(C4GraphicsOverlay *pDefault) { delete pOverlay; pOverlay = pDefault; }
};
