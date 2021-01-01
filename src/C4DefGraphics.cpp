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

#include <C4Include.h>
#include <C4DefGraphics.h>

#include <C4SurfaceFile.h>
#include <C4Object.h>
#include <C4ObjectInfo.h>
#include <C4Config.h>
#include <C4Components.h>
#include <C4Application.h>
#include <C4Game.h>
#include <C4Menu.h>
#include <C4ObjectMenu.h>
#include <C4Player.h>
#include <C4Log.h>

// C4DefGraphics

C4DefGraphics::C4DefGraphics(C4Def *pOwnDef)
{
	// store def
	pDef = pOwnDef;
	// zero fields
	Bitmap = BitmapClr = nullptr;
	pNext = nullptr;
	fColorBitmapAutoCreated = false;
}

void C4DefGraphics::Clear()
{
	// zero own fields
	delete BitmapClr; BitmapClr = nullptr;
	delete Bitmap;    Bitmap    = nullptr;
	// delete additonal graphics
	C4AdditionalDefGraphics *pGrp2N = pNext, *pGrp2;
	while (pGrp2 = pGrp2N) { pGrp2N = pGrp2->pNext; pGrp2->pNext = nullptr; delete pGrp2; }
	pNext = nullptr; fColorBitmapAutoCreated = false;
}

bool C4DefGraphics::LoadGraphics(C4Group &hGroup, const char *szFilename, const char *szFilenamePNG, const char *szOverlayPNG, bool fColorByOwner)
{
	// try png
	if (szFilenamePNG && hGroup.AccessEntry(szFilenamePNG))
	{
		Bitmap = new C4Surface();
		if (!Bitmap->ReadPNG(hGroup)) return false;
	}
	else
	{
		if (szFilename)
			if (!hGroup.AccessEntry(szFilename)
				|| !(Bitmap = GroupReadSurface(hGroup)))
				return false;
	}
	// Create owner color bitmaps
	if (fColorByOwner)
	{
		// Create additionmal bitmap
		BitmapClr = new C4Surface();
		// if overlay-surface is present, load from that
		if (szOverlayPNG && hGroup.AccessEntry(szOverlayPNG))
		{
			if (!BitmapClr->ReadPNG(hGroup))
				return false;
			// set as Clr-surface, also checking size
			if (!BitmapClr->SetAsClrByOwnerOf(Bitmap))
			{
				const char *szFn = szFilenamePNG ? szFilenamePNG : szFilename;
				if (!szFn) szFn = "???";
				DebugLogF("    Gfx loading error in %s: %s (%d x %d) doesn't match overlay %s (%d x %d) - invalid file or size mismatch",
					hGroup.GetFullName().getData(), szFn, Bitmap ? Bitmap->Wdt : -1, Bitmap ? Bitmap->Hgt : -1,
					szOverlayPNG, BitmapClr->Wdt, BitmapClr->Hgt);
				delete BitmapClr; BitmapClr = nullptr;
				return false;
			}
		}
		else
			// otherwise, create by all blue shades
			if (!BitmapClr->CreateColorByOwner(Bitmap)) return false;
		fColorBitmapAutoCreated = true;
	}
	// success
	return true;
}

bool C4DefGraphics::LoadAllGraphics(C4Group &hGroup, bool fColorByOwner)
{
	// load basic graphics
	if (!LoadGraphics(hGroup, C4CFN_DefGraphics, C4CFN_DefGraphicsPNG, C4CFN_ClrByOwnerPNG, fColorByOwner)) return false;
	// load additional graphics
	// first, search all png-graphics in NewGfx
	char Filename[_MAX_PATH + 1]; *Filename = 0;
	C4DefGraphics *pLastGraphics = this;
	int32_t iWildcardPos;
	iWildcardPos = SCharPos('*', C4CFN_DefGraphicsExPNG);
	int32_t iOverlayWildcardPos = SCharPos('*', C4CFN_ClrByOwnerExPNG);
	hGroup.ResetSearch();
	while (hGroup.FindNextEntry(C4CFN_DefGraphicsExPNG, Filename, nullptr, nullptr, !!*Filename))
	{
		// skip def graphics
		if (SEqualNoCase(Filename, C4CFN_DefGraphicsPNG)) continue;
		// get name
		char GrpName[_MAX_PATH + 1];
		SCopy(Filename + iWildcardPos, GrpName, _MAX_PATH);
		RemoveExtension(GrpName);
		// clip to max length
		GrpName[C4MaxName] = 0;
		// create new graphics
		pLastGraphics->pNext = new C4AdditionalDefGraphics(pDef, GrpName);
		pLastGraphics = pLastGraphics->pNext;
		// create overlay-filename
		char OverlayFn[_MAX_PATH + 1];
		if (fColorByOwner)
		{
			// GraphicsX.png -> OverlayX.png
			SCopy(C4CFN_ClrByOwnerExPNG, OverlayFn, _MAX_PATH);
			OverlayFn[iOverlayWildcardPos] = 0;
			SAppend(Filename + iWildcardPos, OverlayFn);
			EnforceExtension(OverlayFn, GetExtension(C4CFN_ClrByOwnerExPNG));
		}
		// load them
		if (!pLastGraphics->LoadGraphics(hGroup, nullptr, Filename, fColorByOwner ? OverlayFn : nullptr, fColorByOwner))
			return false;
	}
	// load bitmap-graphics
	iWildcardPos = SCharPos('*', C4CFN_DefGraphicsEx);
	hGroup.ResetSearch();
	*Filename = 0;
	while (hGroup.FindNextEntry(C4CFN_DefGraphicsEx, Filename, nullptr, nullptr, !!*Filename))
	{
		// skip def graphics
		if (SEqualNoCase(Filename, C4CFN_DefGraphics)) continue;
		// get graphics name
		char GrpName[_MAX_PATH + 1];
		SCopy(Filename + iWildcardPos, GrpName, _MAX_PATH);
		RemoveExtension(GrpName);
		// clip to max length
		GrpName[C4MaxName] = 0;
		// check if graphics already exist (-> loaded as PNG)
		if (Get(GrpName)) continue;
		// create new graphics
		pLastGraphics->pNext = new C4AdditionalDefGraphics(pDef, GrpName);
		pLastGraphics = pLastGraphics->pNext;
		// load them
		if (!pLastGraphics->LoadGraphics(hGroup, Filename, nullptr, nullptr, fColorByOwner))
			return false;
	}
	// load portrait graphics
	iWildcardPos = SCharPos('*', C4CFN_Portraits);
	hGroup.ResetSearch();
	*Filename = 0;
	while (hGroup.FindNextEntry(C4CFN_Portraits, Filename, nullptr, nullptr, !!*Filename))
	{
		// get graphics name
		char GrpName[_MAX_PATH + 1];
		SCopy(Filename + iWildcardPos, GrpName, _MAX_PATH);
		RemoveExtension(GrpName);
		// clip to max length
		GrpName[C4MaxName] = 0;
		// determine file type (bmp or png)
		char OverlayFn[_MAX_PATH + 1]; bool fBMP; *OverlayFn = 0;
		if (SEqualNoCase(GetExtension(Filename), "bmp"))
			fBMP = true;
		else
		{
			fBMP = false;
			if (!SEqualNoCase(GetExtension(Filename), "png")) continue;
			// create overlay-filename for PNGs
			if (fColorByOwner)
			{
				// PortraitX.png -> OverlayX.png
				SCopy(C4CFN_ClrByOwnerExPNG, OverlayFn, _MAX_PATH);
				OverlayFn[iOverlayWildcardPos] = 0;
				SAppend(Filename + iWildcardPos, OverlayFn);
				EnforceExtension(OverlayFn, GetExtension(C4CFN_ClrByOwnerExPNG));
			}
		}
		// create new graphics
		pLastGraphics->pNext = new C4PortraitGraphics(pDef, GrpName);
		pLastGraphics = pLastGraphics->pNext;
		// load them
		if (!pLastGraphics->LoadGraphics(hGroup, fBMP ? Filename : nullptr, fBMP ? nullptr : Filename, *OverlayFn ? OverlayFn : nullptr, fColorByOwner))
			return false;
	}
	// done, success
	return true;
}

bool C4DefGraphics::ColorizeByMaterial(int32_t iMat, C4MaterialMap &rMats, uint8_t bGBM)
{
	C4Surface *sfcBitmap = GetBitmap(); // first bitmap only
	if (sfcBitmap)
	{
		uint32_t dwMatColors[C4M_ColsPerMat];
		for (int32_t i = 0; i < C4M_ColsPerMat; ++i)
			dwMatColors[i] = rMats.Map[iMat].GetDWordColor(i);
		Application.DDraw->SurfaceAllowColor(sfcBitmap, dwMatColors, C4M_ColsPerMat, true);
	}
	// colorize other graphics
	if (pNext) return pNext->ColorizeByMaterial(iMat, rMats, bGBM); else return true;
}

C4DefGraphics *C4DefGraphics::Get(const char *szGrpName)
{
	// no group or empty string: base graphics
	if (!szGrpName || !szGrpName[0]) return this;
	// search additional graphics
	for (C4AdditionalDefGraphics *pGrp = pNext; pGrp; pGrp = pGrp->pNext)
		if (SEqualNoCase(pGrp->GetName(), szGrpName)) return pGrp;
	// nothing found
	return nullptr;
}

C4PortraitGraphics *C4PortraitGraphics::Get(const char *szGrpName)
{
	// no group or empty string: no gfx
	if (!szGrpName || !szGrpName[0]) return nullptr;
	// search self and additional graphics
	for (C4AdditionalDefGraphics *pGrp = this; pGrp; pGrp = pGrp->GetNext())
		if (SEqualNoCase(pGrp->GetName(), szGrpName)) return pGrp->IsPortrait();
	// nothing found
	return nullptr;
}

bool C4DefGraphics::CopyGraphicsFrom(C4DefGraphics &rSource)
{
	// clear previous
	delete BitmapClr; BitmapClr = nullptr;
	delete Bitmap;    Bitmap    = nullptr;
	// copy from source
	if (rSource.Bitmap)
	{
		Bitmap = new C4Surface();
		if (!Bitmap->Copy(*rSource.Bitmap))
		{
			delete Bitmap; Bitmap = nullptr; return false;
		}
	}
	if (rSource.BitmapClr)
	{
		BitmapClr = new C4Surface();
		if (!BitmapClr->Copy(*rSource.BitmapClr))
		{
			delete Bitmap;    Bitmap    = nullptr;
			delete BitmapClr; BitmapClr = nullptr; return false;
		}
		if (Bitmap) BitmapClr->SetAsClrByOwnerOf(Bitmap);
	}
	// done, success
	return true;
}

void C4DefGraphics::DrawClr(C4Facet &cgo, bool fAspect, uint32_t dwClr)
{
	// create facet and draw it
	C4Surface *pSfc = BitmapClr ? BitmapClr : Bitmap; if (!pSfc) return;
	C4Facet fct(pSfc, 0, 0, pSfc->Wdt, pSfc->Hgt);
	fct.DrawClr(cgo, fAspect, dwClr);
}

void C4DefGraphicsAdapt::CompileFunc(StdCompiler *pComp)
{
	bool fCompiler = pComp->isCompiler();
	// nothing?
	if (!fCompiler && !pDefGraphics) return;
	// definition
	C4ID id; if (!fCompiler) id = pDefGraphics->pDef->id;
	pComp->Value(mkC4IDAdapt(id));
	// go over two separators ("::"). Expect them if an id was found.
	if (!pComp->Separator(StdCompiler::SEP_PART2) || !pComp->Separator(StdCompiler::SEP_PART2))
		pComp->excCorrupt("DefGraphics: expected \"::\"");
	// compile name
	StdStrBuf Name; if (!fCompiler) Name.Ref(pDefGraphics->GetName());
	pComp->Value(mkDefaultAdapt(mkParAdapt(Name, StdCompiler::RCT_Idtf), ""));
	// reading: search def-graphics
	if (fCompiler)
	{
		// search definition, throw expection if not found
		C4Def *pDef = Game.Defs.ID2Def(id);
		// search def-graphics
		if (!pDef || !(pDefGraphics = pDef->Graphics.Get(Name.getData())))
			pComp->excCorrupt("DefGraphics: could not find graphics \"%s\" in %s(%s)!", Name.getData(), C4IdText(id), pDef ? pDef->Name.getData() : "def not found");
	}
}

C4AdditionalDefGraphics::C4AdditionalDefGraphics(C4Def *pOwnDef, const char *szName) : C4DefGraphics(pOwnDef)
{
	// store name
	SCopy(szName, Name, C4MaxName);
}

C4PortraitGraphics *C4PortraitGraphics::GetByIndex(int32_t iIndex)
{
	// start from this portrait
	C4DefGraphics *pResult = this;
	while (iIndex--)
	{
		// get next indexed
		pResult = pResult->GetNext(); if (!pResult) return nullptr;
		// skip non-portraits
		if (!pResult->IsPortrait()) ++iIndex;
	}
	// return portrait
	return pResult->IsPortrait();
}

C4DefGraphicsPtrBackup::C4DefGraphicsPtrBackup(C4DefGraphics *pSourceGraphics)
{
	// assign graphics + def
	pGraphicsPtr = pSourceGraphics;
	pDef = pSourceGraphics->pDef;
	// assign name
	const char *szName = pGraphicsPtr->GetName();
	if (szName) SCopy(szName, Name, C4MaxName); else *Name = 0;
	// create next graphics recursively
	C4DefGraphics *pNextGfx = pGraphicsPtr->pNext;
	if (pNextGfx)
		pNext = new C4DefGraphicsPtrBackup(pNextGfx);
	else
		pNext = nullptr;
}

C4DefGraphicsPtrBackup::~C4DefGraphicsPtrBackup()
{
	// graphics ptr still assigned? then remove dead graphics pointers from objects
	if (pGraphicsPtr) AssignRemoval();
	// delete following graphics recursively
	delete pNext;
}

void C4DefGraphicsPtrBackup::AssignUpdate(C4DefGraphics *pNewGraphics)
{
	// only if graphics are assigned
	if (pGraphicsPtr)
	{
		// check all objects
		C4Object *pObj;
		for (C4ObjectLink *pLnk = Game.Objects.First; pLnk; pLnk = pLnk->Next)
			if (pObj = pLnk->Obj) if (pObj->Status)
			{
				if (pObj->pGraphics == pGraphicsPtr)
				{
					// same graphics found: try to set them
					if (!pObj->SetGraphics(Name, pDef))
						if (!pObj->SetGraphics(Name, pObj->Def))
						{
							// shouldn't happen
							pObj->AssignRemoval(); pObj->pGraphics = nullptr;
						}
				}
				// remove any overlay graphics
				for (;;)
				{
					C4GraphicsOverlay *pGfxOverlay;
					for (pGfxOverlay = pObj->pGfxOverlay; pGfxOverlay; pGfxOverlay = pGfxOverlay->GetNext())
						if (pGfxOverlay->GetGfx() == pGraphicsPtr)
						{
							// then remove this overlay and redo the loop, because iterator has become invalid
							pObj->RemoveGraphicsOverlay(pGfxOverlay->GetID());
							break;
						}
					// looped through w/o removal?
					if (!pGfxOverlay) break;
				}
				// update menu frame decorations - may do multiple updates to the same deco if multiple menus share it...
				C4GUI::FrameDecoration *pDeco;
				if (pDef && pObj->Menu && (pDeco = pObj->Menu->GetFrameDecoration()))
					if (pDeco->idSourceDef == pDef->id)
						if (!pDeco->UpdateGfx())
							pObj->Menu->SetFrameDeco(nullptr);
			}
		// check all object infos for portraits
		for (C4Player *pPlr = Game.Players.First; pPlr; pPlr = pPlr->Next)
			for (C4ObjectInfo *pInfo = pPlr->CrewInfoList.GetFirst(); pInfo; pInfo = pInfo->Next)
			{
				if (pInfo->Portrait.GetGfx() == pGraphicsPtr)
				{
					// portrait found: try to re-set by new name
					if (!pInfo->SetPortrait(Name, pDef, false, false))
						// not found: no portrait then
						pInfo->ClearPortrait(false);
				}
				if (pInfo->pNewPortrait && pInfo->pNewPortrait->GetGfx() == pGraphicsPtr)
				{
					// portrait found as new portrait: simply reset (no complex handling for EM crew changes necessary)
					delete pInfo->pNewPortrait;
					pInfo->pNewPortrait = nullptr;
				}
			}
		// done; reset field to indicate finished update
		pGraphicsPtr = nullptr;
	}
	// check next graphics
	if (pNext) pNext->AssignUpdate(pNewGraphics);
}

void C4DefGraphicsPtrBackup::AssignRemoval()
{
	// only if graphics are assigned
	if (pGraphicsPtr)
	{
		// check all objects
		C4Object *pObj;
		for (C4ObjectLink *pLnk = Game.Objects.First; pLnk; pLnk = pLnk->Next)
			if (pObj = pLnk->Obj) if (pObj->Status)
			{
				if (pObj->pGraphics == pGraphicsPtr)
					// same graphics found: reset them
					if (!pObj->SetGraphics()) { pObj->AssignRemoval(); pObj->pGraphics = nullptr; }
				// remove any overlay graphics
				for (;;)
				{
					C4GraphicsOverlay *pGfxOverlay;
					for (pGfxOverlay = pObj->pGfxOverlay; pGfxOverlay; pGfxOverlay = pGfxOverlay->GetNext())
						if (pGfxOverlay->GetGfx() == pGraphicsPtr)
						{
							// then remove this overlay and redo the loop, because iterator has become invalid
							pObj->RemoveGraphicsOverlay(pGfxOverlay->GetID());
							break;
						}
					// looped through w/o removal?
					if (!pGfxOverlay) break;
				}
				// remove menu frame decorations
				C4GUI::FrameDecoration *pDeco;
				if (pDef && pObj->Menu && (pDeco = pObj->Menu->GetFrameDecoration()))
					if (pDeco->idSourceDef == pDef->id)
						pObj->Menu->SetFrameDeco(nullptr);
			}
		// done; reset field to indicate finished update
		pGraphicsPtr = nullptr;
	}
	// check next graphics
	if (pNext) pNext->AssignRemoval();
}

bool C4Portrait::Load(C4Group &rGrp, const char *szFilename, const char *szFilenamePNG, const char *szOverlayPNG)
{
	// clear previous
	Clear();
	// create new gfx
	pGfxPortrait = new C4DefGraphics();
	// load
	if (!pGfxPortrait->LoadGraphics(rGrp, szFilename, szFilenamePNG, szOverlayPNG, true))
	{
		// load failure
		delete pGfxPortrait; pGfxPortrait = nullptr;
		return false;
	}
	// assign owned gfx
	fGraphicsOwned = true;
	// done, success
	return true;
}

bool C4Portrait::Link(C4DefGraphics *pGfxPortrait)
{
	// clear previous
	Clear();
	// simply assign ptr
	this->pGfxPortrait = pGfxPortrait;
	// done, success
	return true;
}

bool C4Portrait::SavePNG(C4Group &rGroup, const char *szFilename, const char *szOverlayFN)
{
	// safety
	if (!pGfxPortrait || !szFilename || !pGfxPortrait->Bitmap) return false;
	// save files
	if (pGfxPortrait->fColorBitmapAutoCreated)
	{
		// auto-created ColorByOwner: save file with blue shades to be read by frontend
		if (!pGfxPortrait->GetBitmap(0xff)->SavePNG(rGroup, szFilename)) return false;
	}
	else
	{
		// save regular baseface
		if (!pGfxPortrait->Bitmap->SavePNG(rGroup, szFilename)) return false;
		// save Overlay
		if (pGfxPortrait->BitmapClr && szOverlayFN)
			if (!pGfxPortrait->BitmapClr->SavePNG(rGroup, szOverlayFN, true, false, true)) return false;
	}
	// done, success
	return true;
}

bool C4Portrait::CopyFrom(C4DefGraphics &rCopyGfx)
{
	// clear previous
	Clear();
	// gfx copy
	pGfxPortrait = new C4DefGraphics();
	if (!pGfxPortrait->CopyGraphicsFrom(rCopyGfx))
	{
		Clear(); return false;
	}
	// mark as own gfx
	fGraphicsOwned = true;
	// done, success
	return true;
}

bool C4Portrait::CopyFrom(C4Portrait &rCopy)
{
	// clear previous
	Clear();
	if (fGraphicsOwned = rCopy.fGraphicsOwned)
	{
		// gfx copy
		pGfxPortrait = new C4DefGraphics();
		if (!pGfxPortrait->CopyGraphicsFrom(*rCopy.GetGfx()))
		{
			Clear(); return false;
		}
	}
	else
	{
		// simple link
		pGfxPortrait = rCopy.GetGfx();
	}
	// done, success
	return true;
}

const char *C4Portrait::EvaluatePortraitString(const char *szPortrait, C4ID &rIDOut, C4ID idDefaultID, uint32_t *pdwClrOut)
{
	// examine portrait string
	if (SLen(szPortrait) > 6 && szPortrait[4] == ':' && szPortrait[5] == ':')
	{
		// C4ID::PortraitName or C4ID::dwClr::PortraitName
		rIDOut = C4Id(szPortrait);
		// color specified?
		szPortrait += 6;
		const char *szAfterQColon = SSearch(szPortrait, "::");
		if (szAfterQColon)
		{
			char buf[7];

			// szAfterQColon-szPortrait-2 results in long on 64bit,
			// so the template needs to be specialised
			SCopy(szPortrait, buf, std::min<ptrdiff_t>(6, szAfterQColon - szPortrait - 2));
			if (pdwClrOut) sscanf(buf, "%x", pdwClrOut);
			szPortrait = szAfterQColon;
		}
		// return last part of string
		return szPortrait;
	}
	else
	{
		// PortraitName. ID is info ID
		rIDOut = idDefaultID;
		return szPortrait;
	}
}

// C4GraphicsOverlay: graphics overlay used to attach additional graphics to objects

C4GraphicsOverlay::~C4GraphicsOverlay()
{
	// free any additional overlays
	C4GraphicsOverlay *pNextOther = pNext, *pOther;
	while (pOther = pNextOther)
	{
		pNextOther = pOther->pNext;
		pOther->pNext = nullptr;
		delete pOther;
	}
}

void C4GraphicsOverlay::UpdateFacet()
{
	// special: Nothing to update for object and pSourceGfx may be nullptr
	// If there will ever be something to init here, UpdateFacet() will also need to be called when objects have been loaded
	if (eMode == MODE_Object) return;
	// otherwise, source graphics must be specified
	if (!pSourceGfx) return;
	C4Def *pDef = pSourceGfx->pDef;
	assert(pDef);
	fZoomToShape = false;
	// update by mode
	switch (eMode)
	{
	case MODE_None:
		break;

	case MODE_Base: // def base graphics
		fctBlit.Set(pSourceGfx->GetBitmap(), 0, 0, pDef->Shape.Wdt, pDef->Shape.Hgt, pDef->Shape.x + pDef->Shape.Wdt / 2, pDef->Shape.y + pDef->Shape.Hgt / 2);
		break;

	case MODE_Action: // graphics of specified action
	{
		// Find act in ActMap of object
		int32_t cnt;
		for (cnt = 0; cnt < pDef->ActNum; cnt++)
			if (SEqual(Action, pDef->ActMap[cnt].Name))
				break;
		if (cnt == pDef->ActNum) { fctBlit.Default(); break; }
		// assign base gfx of action
		// doesn't catch any special action parameters (FacetBase, etc.)...
		C4ActionDef *pAct = pDef->ActMap + cnt;
		fctBlit.Set(pSourceGfx->GetBitmap(), pAct->Facet.x, pAct->Facet.y, pAct->Facet.Wdt, pAct->Facet.Hgt);
	}
	break;

	case MODE_IngamePicture:
	case MODE_Picture: // def picture
		fZoomToShape = true;
		fctBlit.Set(pSourceGfx->GetBitmap(), pDef->PictureRect.x, pDef->PictureRect.y, pDef->PictureRect.Wdt, pDef->PictureRect.Hgt);
		break;

	case MODE_ExtraGraphics: // like ColorByOwner-sfc
		// calculated at runtime
		break;

	case MODE_Object:
		// nothing to do
		break;
	}
}

void C4GraphicsOverlay::Set(Mode aMode, C4DefGraphics *pGfx, const char *szAction, uint32_t dwBMode, C4Object *pOvrlObj)
{
	// set values
	eMode = aMode;
	pSourceGfx = pGfx;
	if (szAction) SCopy(szAction, Action, C4MaxName); else *Action = 0;
	dwBlitMode = dwBMode;
	pOverlayObj = pOvrlObj;
	// (keep transform)
	// reset phase
	iPhase = 0;
	// update used facet
	UpdateFacet();
}

bool C4GraphicsOverlay::IsValid(const C4Object *pForObj) const
{
	assert(pForObj);
	if (eMode == MODE_Object)
	{
		if (!pOverlayObj || !pOverlayObj->Status) return false;
		return !pOverlayObj->HasGraphicsOverlayRecursion(pForObj);
	}
	else if (eMode == MODE_ExtraGraphics)
	{
		return !!pSourceGfx;
	}
	else
		return pSourceGfx && fctBlit.Surface;
}

void C4GraphicsOverlay::CompileFunc(StdCompiler *pComp)
{
	// read ID
	pComp->Value(iID); pComp->Separator();
	// read def-graphics
	pComp->Value(mkDefaultAdapt(C4DefGraphicsAdapt(pSourceGfx), nullptr));
	pComp->Separator();
	// read mode
	pComp->Value(mkIntAdapt(eMode)); pComp->Separator();
	// read action (identifier)
	pComp->Value(mkStringAdaptMIE(Action)); pComp->Separator();
	// read blit mode
	pComp->Value(dwBlitMode); pComp->Separator();
	// read phase
	pComp->Value(iPhase); pComp->Separator();
	// read transform
	pComp->Separator(StdCompiler::SEP_START);
	pComp->Value(Transform);
	pComp->Separator(StdCompiler::SEP_END);
	// read color-modulation
	if (pComp->Separator())
		pComp->Value(mkIntAdapt(dwClrModulation));
	else
		// default
		if (pComp->isCompiler()) dwClrModulation = 0xffffff;
	// read overlay target object
	if (pComp->Separator())
		pComp->Value(pOverlayObj);
	else
		// default
		if (pComp->isCompiler()) pOverlayObj.Reset();
	// update used facet according to read data
	if (pComp->isCompiler()) UpdateFacet();
}

void C4GraphicsOverlay::EnumeratePointers()
{
	pOverlayObj.Enumerate();
}

void C4GraphicsOverlay::DenumeratePointers()
{
	pOverlayObj.Denumerate();
}

void C4GraphicsOverlay::Draw(C4FacetEx &cgo, C4Object *pForObj, int32_t iByPlayer)
{
	assert(!IsPicture());
	assert(pForObj);
	// get target pos
	int32_t cotx = cgo.TargetX, coty = cgo.TargetY; pForObj->TargetPos(cotx, coty, cgo);
	int32_t iTx = pForObj->x - cotx + cgo.X,
		iTy = pForObj->y - coty + cgo.Y;
	// special blit mode
	if (dwBlitMode == C4GFXBLIT_PARENT)
		(pOverlayObj ? pOverlayObj : pForObj)->PrepareDrawing();
	else
	{
		Application.DDraw->SetBlitMode(dwBlitMode);
		if (dwClrModulation != 0xffffff) Application.DDraw->ActivateBlitModulation(dwClrModulation);
	}
	// drawing specific object?
	if (pOverlayObj)
	{
		// Draw specified object at target pos of this object; offset by transform.
		// This ignores any other transform than offset, and it doesn't work with parallax overlay objects yet
		// (But any parallaxity of pForObj is regarded in calculation of cotx/y!)
		int32_t oldTx = cgo.TargetX, oldTy = cgo.TargetY;
		cgo.TargetX = cotx - pForObj->x + pOverlayObj->x - Transform.GetXOffset();
		cgo.TargetY = coty - pForObj->y + pOverlayObj->y - Transform.GetYOffset();
		pOverlayObj->Draw(cgo, iByPlayer, C4Object::ODM_Overlay);
		pOverlayObj->DrawTopFace(cgo, iByPlayer, C4Object::ODM_Overlay);
		cgo.TargetX = oldTx;
		cgo.TargetY = oldTy;
	}
	else if (eMode == MODE_ExtraGraphics)
	{
		// draw self with specified gfx
		if (pSourceGfx)
		{
			C4DefGraphics *pPrevGfx = pForObj->GetGraphics();
			C4DrawTransform *pPrevTrf = pForObj->pDrawTransform;
			C4DrawTransform trf;
			if (pPrevTrf)
			{
				trf = *pPrevTrf;
				trf *= Transform;
			}
			else
			{
				trf = Transform;
			}
			pForObj->SetGraphics(pSourceGfx, true);
			pForObj->pDrawTransform = &trf;
			pForObj->Draw(cgo, iByPlayer, C4Object::ODM_BaseOnly);
			pForObj->DrawTopFace(cgo, iByPlayer, C4Object::ODM_BaseOnly);
			pForObj->SetGraphics(pPrevGfx, true);
			pForObj->pDrawTransform = pPrevTrf;
		}
	}
	else
	{
		// no object specified: Draw from fctBlit
		// update by object color
		if (fctBlit.Surface) fctBlit.Surface->SetClr(pForObj->Color);

		// draw there
		C4DrawTransform trf(Transform, float(iTx), float(iTy));
		if (fZoomToShape)
		{
			float fZoom = std::min<float>(static_cast<float>(pForObj->Shape.Wdt) / std::max<int>(fctBlit.Wdt, 1), static_cast<float>(pForObj->Shape.Hgt) / std::max<int>(fctBlit.Hgt, 1));
			trf.ScaleAt(fZoom, fZoom, float(iTx), float(iTy));
		}
		fctBlit.DrawT(cgo.Surface, iTx - fctBlit.Wdt / 2 + fctBlit.TargetX, iTy - fctBlit.Hgt / 2 + fctBlit.TargetY, iPhase, 0, &trf, false, pSourceGfx->pDef->Scale);
	}

	// cleanup
	if (dwBlitMode == C4GFXBLIT_PARENT)
		(pOverlayObj ? pOverlayObj : pForObj)->FinishedDrawing();
	else
	{
		Application.DDraw->ResetBlitMode();
		Application.DDraw->DeactivateBlitModulation();
	}
}

void C4GraphicsOverlay::DrawPicture(C4Facet &cgo, C4Object *pForObj)
{
	assert(IsPicture());
	// update object color
	if (pForObj && fctBlit.Surface) fctBlit.Surface->SetClr(pForObj->Color);
	// special blit mode
	if (dwBlitMode == C4GFXBLIT_PARENT)
	{
		if (pForObj) pForObj->PrepareDrawing();
	}
	else
	{
		Application.DDraw->SetBlitMode(dwBlitMode);
		if (dwClrModulation != 0xffffff) Application.DDraw->ActivateBlitModulation(dwClrModulation);
	}
	// draw at given rect
	fctBlit.DrawT(cgo, true, iPhase, 0, &C4DrawTransform(Transform, cgo.X + float(cgo.Wdt) / 2, cgo.Y + float(cgo.Hgt) / 2), pSourceGfx->pDef->Scale);
	// cleanup
	if (dwBlitMode == C4GFXBLIT_PARENT)
	{
		if (pForObj) pForObj->FinishedDrawing();
	}
	else
	{
		Application.DDraw->ResetBlitMode();
		Application.DDraw->DeactivateBlitModulation();
	}
}

bool C4GraphicsOverlay::operator==(const C4GraphicsOverlay &rCmp) const
{
	// compare relevant fields
	// ignoring phase, because different animation state may be concatenated in graphics display
	return (eMode == rCmp.eMode)
		&& (pSourceGfx == rCmp.pSourceGfx)
		&& SEqual(Action, rCmp.Action)
		&& (dwBlitMode == rCmp.dwBlitMode)
		&& (dwClrModulation == rCmp.dwClrModulation)
		&& (Transform == rCmp.Transform)
		&& (pOverlayObj == rCmp.pOverlayObj);
}

void C4GraphicsOverlayListAdapt::CompileFunc(StdCompiler *pComp)
{
	bool fNaming = pComp->hasNaming();
	if (pComp->isCompiler())
	{
		// clear list
		delete[] pOverlay; pOverlay = nullptr;
		// read the whole list
		C4GraphicsOverlay *pLast = nullptr;
		bool fContinue;
		do
		{
			C4GraphicsOverlay *pNext = new C4GraphicsOverlay();
			try
			{
				// read an overlay
				pComp->Value(*pNext);
			}
			catch (const StdCompiler::NotFoundException &)
			{
				// delete unused overlay
				delete pNext; pNext = nullptr;
				// clear up
				if (!pLast) pOverlay = nullptr;
				// done
				return;
			}
			// link it
			if (pLast)
				pLast->SetNext(pNext);
			else
				pOverlay = pNext;
			// step
			pLast = pNext;
			// continue?
			if (fNaming)
				fContinue = pComp->Separator(StdCompiler::SEP_SEP2) || pComp->Separator(StdCompiler::SEP_SEP);
			else
				pComp->Value(fContinue);
		} while (fContinue);
	}
	else
	{
		// write everything
		bool fContinue = true;
		for (C4GraphicsOverlay *pPos = pOverlay; pPos; pPos = pPos->GetNext())
		{
			// separate
			if (pPos != pOverlay)
				if (fNaming)
					pComp->Separator(StdCompiler::SEP_SEP2);
				else
					pComp->Value(fContinue);
			// write
			pComp->Value(*pPos);
		}
		// terminate
		if (!fNaming)
		{
			fContinue = false;
			pComp->Value(fContinue);
		}
	}
}
