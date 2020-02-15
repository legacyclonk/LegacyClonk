/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
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

// a wrapper class to DirectDraw surfaces

#include <Standard.h>
#include <StdFile.h>
#include <CStdFile.h>
#include <StdGL.h>
#include <StdWindow.h>
#include <StdRegistry.h>
#include <StdConfig.h>
#include <StdSurface2.h>
#include <StdFacet.h>
#include <StdDDraw2.h>
#include <Bitmap256.h>
#include <StdBitmap.h>
#include <StdPNG.h>

#ifdef HAVE_IO_H
#include <io.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <list>
#include <stdexcept>

CDDrawCfg DDrawCfg; // ddraw config

CSurface::CSurface() : fIsBackground(false)
{
	Default();
}

CSurface::CSurface(int iWdt, int iHgt) : fIsBackground(false)
{
	Default();
	// create
	Create(iWdt, iHgt);
}

CSurface::~CSurface()
{
	Clear();
}

void CSurface::Default()
{
	Wdt = Hgt = 0;
	PrimarySurfaceLockPitch = 0; PrimarySurfaceLockBits = nullptr;
	ClipX = ClipY = ClipX2 = ClipY2 = 0;
	Locked = 0;
	fPrimary = false;
	ppTex = nullptr;
	pMainSfc = nullptr;
	ClrByOwnerClr = 0;
	iTexSize = iTexX = iTexY = 0;
	fIsRenderTarget = false;
	fIsBackground = false;
#ifdef _DEBUG
	dbg_idx = nullptr;
#endif
}

void CSurface::MoveFrom(CSurface *psfcFrom)
{
	// clear own
	Clear();
	// safety
	if (!psfcFrom) return;
	// grab data from other sfc
#ifdef _DEBUG
	dbg_idx = psfcFrom->dbg_idx;
#endif
	Wdt = psfcFrom->Wdt; Hgt = psfcFrom->Hgt;
	PrimarySurfaceLockPitch = psfcFrom->PrimarySurfaceLockPitch;
	PrimarySurfaceLockBits = psfcFrom->PrimarySurfaceLockBits;
	psfcFrom->PrimarySurfaceLockBits = nullptr;
	ClipX = psfcFrom->ClipX; ClipY = psfcFrom->ClipY;
	ClipX2 = psfcFrom->ClipX2; ClipY2 = psfcFrom->ClipY2;
	Locked = psfcFrom->Locked;
	fPrimary = psfcFrom->fPrimary; // shouldn't be true!
	ppTex = psfcFrom->ppTex;
	pMainSfc = psfcFrom->pMainSfc;
	ClrByOwnerClr = psfcFrom->ClrByOwnerClr;
	iTexSize = psfcFrom->iTexSize;
	iTexX = psfcFrom->iTexX; iTexY = psfcFrom->iTexY;
#ifndef USE_CONSOLE
	Format = psfcFrom->Format;
#endif
	fIsBackground = psfcFrom->fIsBackground;
	// default other sfc
	psfcFrom->Default();
}

void CSurface::Clear()
{
	// Undo all locks
	while (Locked) Unlock();
	// release surface
	FreeTextures();
	ppTex = nullptr;
#ifdef _DEBUG
	delete dbg_idx;
	dbg_idx = nullptr;
#endif
}

bool CSurface::IsRenderTarget()
{
	// primary is always OK...
	return fPrimary;
}

void CSurface::NoClip()
{
	ClipX = 0; ClipY = 0; ClipX2 = Wdt - 1; ClipY2 = Hgt - 1;
}

void CSurface::Clip(int iX, int iY, int iX2, int iY2)
{
	ClipX  = BoundBy(iX,  0, Wdt - 1); ClipY  = BoundBy(iY,  0, Hgt - 1);
	ClipX2 = BoundBy(iX2, 0, Wdt - 1); ClipY2 = BoundBy(iY2, 0, Hgt - 1);
}

bool CSurface::Create(int iWdt, int iHgt, bool fOwnPal, bool fIsRenderTarget)
{
	Clear(); Default();
	// check size
	if (!iWdt || !iHgt) return false;
	Wdt = iWdt; Hgt = iHgt;
	// create texture: check gfx system
	if (!lpDDraw) return false;
	if (!lpDDraw->DeviceReady()) return false;

	// store color format that will be used
#ifndef USE_CONSOLE
	if (pGL)
		Format = pGL->sfcFmt;
	else
#endif
		/* nothing to do */;
	this->fIsRenderTarget = fIsRenderTarget;
	// create textures
	if (!CreateTextures()) { Clear(); return false; }
	// update clipping
	NoClip();
	// success
	return true;
}

bool CSurface::CreateTextures()
{
	// free previous
	FreeTextures();
	// get max texture size
	int iMaxTexSize = 4096;
#ifndef USE_CONSOLE
	if (pGL)
	{
		GLint iMaxTexSize2 = 0;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &iMaxTexSize2);
		if (iMaxTexSize2 > 0) if (iMaxTexSize2 < iMaxTexSize) iMaxTexSize = iMaxTexSize2;
	}
	else
#endif
		/* keep standard texture size */;
	// get needed tex size - begin with smaller value of wdt/hgt, so there won't be too much space wasted
	int iNeedSize = (std::min)(Wdt, Hgt); int n = 0; while ((1 << ++n) < iNeedSize); iNeedSize = 1 << n;
	// adjust to available texture size
	iTexSize = (std::min)(iNeedSize, iMaxTexSize);
	// get the number of textures needed for this size
	iTexX = (Wdt - 1) / iTexSize + 1;
	iTexY = (Hgt - 1) / iTexSize + 1;
	// get mem for texture array
	ppTex = new CTexRef *[iTexX * iTexY]{};
	// cvan't be render target if it's not a single surface
	if (!IsSingleSurface()) fIsRenderTarget = false;
	// create textures
	CTexRef **ppCTex = ppTex;
	for (int i = iTexX * iTexY; i; --i, ++ppCTex)
	{
		// regular textures or if last texture fits exactly into the space by Wdt or Hgt
		if (i - 1 || !(Wdt % iTexSize) || !(Hgt % iTexSize))
			*ppCTex = new CTexRef(iTexSize, fIsRenderTarget);
		else
		{
			// last texture might be smaller
			iNeedSize = (std::max)(Wdt % iTexSize, Hgt % iTexSize);
			int n = 0; while ((1 << ++n) < iNeedSize); iNeedSize = 1 << n;
			*ppCTex = new CTexRef(iNeedSize, fIsRenderTarget);
		}
		if (fIsBackground && ppCTex)(*ppCTex)->FillBlack();
	}
#ifdef _DEBUG
	static int dbg_counter = 0;
	dbg_idx = new int;
	*dbg_idx = dbg_counter++;
#endif
	// success
	return true;
}

void CSurface::FreeTextures()
{
	if (ppTex)
	{
		// clear all textures
		CTexRef **ppTx = ppTex;
		for (int i = 0; i < iTexX * iTexY; ++i, ++ppTx)
			delete *ppTx;
		// clear texture list
		delete[] ppTex;
		ppTex = nullptr;
	}
}

#define RANGE 255
#define HLSMAX RANGE
#define RGBMAX 255

bool ClrByOwner(uint32_t &dwClr) // new style, based on Microsoft Knowledge Base Article - 29240
{
	int H, L, S;
	uint16_t R, G, B;
	uint8_t cMax, cMin;
	uint16_t Rdelta, Gdelta, Bdelta;
	// get RGB (from BGR...?)
	R = GetBValue(dwClr);
	G = GetGValue(dwClr);
	B = GetRValue(dwClr);
	// calculate lightness
	cMax = std::max<int>(std::max<int>(R, G), B);
	cMin = std::min<int>(std::min<int>(R, G), B);
	L = (((cMax + cMin) * HLSMAX) + RGBMAX) / (2 * RGBMAX);
	// achromatic case
	if (cMax == cMin)
	{
		S = 0;
		H = (HLSMAX * 2 / 3);
	}
	// chromatic case
	else
	{
		// saturation
		if (L <= (HLSMAX / 2))
			S = (((cMax - cMin) * HLSMAX) + ((cMax + cMin) / 2)) / (cMax + cMin);
		else
			S = (((cMax - cMin) * HLSMAX) + ((2 * RGBMAX - cMax - cMin) / 2))
			/ (2 * RGBMAX - cMax - cMin);
		// hue
		Rdelta = (((cMax - R) * (HLSMAX / 6)) + ((cMax - cMin) / 2)) / (cMax - cMin);
		Gdelta = (((cMax - G) * (HLSMAX / 6)) + ((cMax - cMin) / 2)) / (cMax - cMin);
		Bdelta = (((cMax - B) * (HLSMAX / 6)) + ((cMax - cMin) / 2)) / (cMax - cMin);
		if (R == cMax)
			H = Bdelta - Gdelta;
		else if (G == cMax)
			H = (HLSMAX / 3) + Rdelta - Bdelta;
		else
			H = ((2 * HLSMAX) / 3) + Gdelta - Rdelta;
		if (H < 0)
			H += HLSMAX;
		if (H > HLSMAX)
			H -= HLSMAX;
	}
	// Not blue
	if (!(Inside(H, 145, 175) && (S > 100))) return false;
	// It's blue: make it gray
	uint8_t b = GetRValue(dwClr);
	dwClr = RGB(b, b, b) | (dwClr & 0xff000000);
	return true;
}

bool CSurface::CreateColorByOwner(CSurface *pBySurface)
{
	// safety
	if (!pBySurface) return false;
	if (!pBySurface->ppTex) return false;
	// create in same size
	if (!Create(pBySurface->Wdt, pBySurface->Hgt, false)) return false;
	// set main surface
	pMainSfc = pBySurface;
	// lock it
	if (!pMainSfc->Lock()) return false;
	if (!Lock()) { pMainSfc->Unlock(); return false; }
	// set ColorByOwner-pixels
	for (int iY = 0; iY < Hgt; ++iY)
		for (int iX = 0; iX < Wdt; ++iX)
		{
			// get pixel
			uint32_t dwPix = pMainSfc->GetPixDw(iX, iY, false);
			// is it a ClrByOwner-px?
			if (!ClrByOwner(dwPix)) continue;
			// set in this surface
			SetPixDw(iX, iY, dwPix);
			// clear in the other
			pMainSfc->SetPixDw(iX, iY, 0xffffffff);
		}
	// unlock
	Unlock();
	pMainSfc->Unlock();
	// success
	return true;
}

bool CSurface::SetAsClrByOwnerOf(CSurface *pOfSurface)
{
	// safety
	if (!pOfSurface) return false;
	if (Wdt != pOfSurface->Wdt || Hgt != pOfSurface->Hgt)
		return false;
	// set main surface
	pMainSfc = pOfSurface;
	// success
	return true;
}

bool CSurface::AttachSfc(void *sfcSurface)
{
	Clear(); Default();
	fPrimary = true;
	if (lpDDraw && lpDDraw->pApp)
	{
		// use application size
		Wdt = lpDDraw->pApp->ScreenWidth();
		Hgt = lpDDraw->pApp->ScreenHeight();
	}
	// reset clipping
	NoClip();
	return true;
}

bool CSurface::Read(CStdStream &hGroup, bool fOwnPal)
{
	int lcnt, iLineRest;
	CBitmap256Info BitmapInfo;
	// read bmpinfo-header
	if (!hGroup.Read(&BitmapInfo, sizeof(CBitmapInfo))) return false;
	// is it 8bpp?
	if (BitmapInfo.Info.biBitCount == 8)
	{
		if (!hGroup.Read(((uint8_t *)&BitmapInfo) + sizeof(CBitmapInfo),
			(std::min)(sizeof(BitmapInfo) - sizeof(CBitmapInfo), sizeof(BitmapInfo) - sizeof(CBitmapInfo) + BitmapInfo.FileBitsOffset())))
			return false;
		if (!hGroup.Advance(BitmapInfo.FileBitsOffset())) return false;
	}
	else
	{
		// read 24bpp
		if (BitmapInfo.Info.biBitCount != 24) return false;
		if (!hGroup.Advance(((CBitmapInfo)BitmapInfo).FileBitsOffset())) return false;
	}

	// Create and lock surface
	if (!Create(BitmapInfo.Info.biWidth, BitmapInfo.Info.biHeight, fOwnPal)) return false;
	if (!Lock()) { Clear(); return false; }

	// create line buffer
	int iBufSize = DWordAligned(BitmapInfo.Info.biWidth * BitmapInfo.Info.biBitCount / 8);
	uint8_t *pBuf = new uint8_t[iBufSize];
	// Read lines
	iLineRest = DWordAligned(BitmapInfo.Info.biWidth) - BitmapInfo.Info.biWidth;
	for (lcnt = Hgt - 1; lcnt >= 0; lcnt--)
	{
		if (!hGroup.Read(pBuf, iBufSize))
		{
			Clear(); delete[] pBuf; return false;
		}
		uint8_t *pPix = pBuf;
		for (int x = 0; x < BitmapInfo.Info.biWidth; ++x)
			switch (BitmapInfo.Info.biBitCount)
			{
			case 8:
				if (fOwnPal)
					SetPixDw(x, lcnt, C4RGB(
						BitmapInfo.Colors[*pPix].rgbRed,
						BitmapInfo.Colors[*pPix].rgbGreen,
						BitmapInfo.Colors[*pPix].rgbBlue));
				else
					SetPix(x, lcnt, *pPix);
				++pPix;
				break;
			case 24:
				SetPixDw(x, lcnt, C4RGB(pPix[0], pPix[1], pPix[2]));
				pPix += 3;
				break;
			}
	}
	// free buffer again
	delete[] pBuf;

	Unlock();

	return true;
}

bool CSurface::SavePNG(const char *szFilename, bool fSaveAlpha, bool fApplyGamma, bool fSaveOverlayOnly, float scale)
{
	// Lock - WARNING - maybe locking primary surface here...
	if (!Lock()) return false;

	if (lpDDraw->Gamma.GetSize() == 0)
		fApplyGamma = false;

	int realWdt = static_cast<int32_t>(ceilf(Wdt * scale));
	int realHgt = static_cast<int32_t>(ceilf(Hgt * scale));

	// Create bitmap
	StdBitmap bmp(realWdt, realHgt, fSaveAlpha);

	// reset overlay if desired
	CSurface *pMainSfcBackup;
	if (fSaveOverlayOnly) { pMainSfcBackup = pMainSfc; pMainSfc = nullptr; }

#ifndef USE_CONSOLE
	if (fPrimary && pGL)
	{
		// Take shortcut. FIXME: Check Endian
		for (int y = 0; y < realHgt; ++y)
			glReadPixels(0, realHgt - y, realWdt, 1, fSaveAlpha ? GL_BGRA : GL_BGR, GL_UNSIGNED_BYTE, bmp.GetPixelAddr(0, y));
	}
	else
#endif
	{
		// write pixel values
		for (int y = 0; y < realHgt; ++y)
			for (int x = 0; x < realWdt; ++x)
			{
				uint32_t dwClr = GetPixDw(x, y, false, scale);
				if (fApplyGamma) dwClr = lpDDraw->Gamma.ApplyTo(dwClr);
				bmp.SetPixel(x, y, dwClr);
			}
	}

	// reset overlay
	if (fSaveOverlayOnly) pMainSfc = pMainSfcBackup;

	// Unlock
	Unlock();

	// Save bitmap to PNG file
	try
	{
		CPNGFile(szFilename, realWdt, realHgt, fSaveAlpha).Encode(bmp.GetBytes());
	}
	catch (const std::runtime_error &)
	{
		return false;
	}

	// Success
	return true;
}

bool CSurface::Wipe()
{
	if (!ppTex) return false;
	// simply clear it (currently slow...)
	if (!Lock()) return false;
	for (int i = 0; i < Wdt * Hgt; ++i)
		if (!fIsBackground)
			SetPix(i % Wdt, i / Wdt, 0);
		else
			SetPixDw(i % Wdt, i / Wdt, 0x00000000);
	Unlock();
	// success
	return true;
}

bool CSurface::GetSurfaceSize(int &irX, int &irY)
{
	// simply assign stored values
	irX = Wdt;
	irY = Hgt;
	// success
	return true;
}

bool CSurface::Lock()
{
	// lock main sfc
	if (pMainSfc) if (!pMainSfc->Lock()) return false;
	// not yet locked?
	if (!Locked)
	{
		if (fPrimary)
		{
			// OpenGL:
			// cannot really lock primary surface, but Get/SetPix will emulate it
		}
		else
		{
			if (!ppTex) return false;
			// lock texture
			// textures will be locked when needed
		}
	}
	// count lock
	Locked++; return true;
}

bool CSurface::Unlock()
{
	// unlock main sfc
	if (pMainSfc) pMainSfc->Unlock();
	// locked?
	if (!Locked) return false;
	// decrease lock counter; check if zeroed
	Locked--;
	if (!Locked)
	{
		// zeroed: unlock
		if (fPrimary)
		{
			{
				// emulated primary locks in OpenGL
				delete[] static_cast<unsigned char*>(PrimarySurfaceLockBits);
				PrimarySurfaceLockBits = nullptr;
				return true;
			}
		}
		else
		{
			// non-primary unlock: unlock all texture surfaces (if locked)
			CTexRef **ppTx = ppTex;
			for (int i = 0; i < iTexX * iTexY; ++i, ++ppTx)
				(*ppTx)->Unlock();
		}
	}
	return true;
}

bool CSurface::GetTexAt(CTexRef **ppTexRef, int &rX, int &rY)
{
	// texture present?
	if (!ppTex) return false;
	// get pos
	int iX = rX / iTexSize;
	int iY = rY / iTexSize;
	// clip
	if (iX < 0 || iY < 0 || iX >= iTexX || iY >= iTexY) return false;
	// get texture by pos
	*ppTexRef = *(ppTex + iY * iTexX + iX);
	// adjust pos
	rX -= iX * iTexSize;
	rY -= iY * iTexSize;
	// success
	return true;
}

bool CSurface::GetLockTexAt(CTexRef **ppTexRef, int &rX, int &rY)
{
	// texture present?
	if (!GetTexAt(ppTexRef, rX, rY)) return false;
	// Already partially locked
	if ((*ppTexRef)->texLock.pBits)
	{
		// But not for the requested pixel
		RECT &r = (*ppTexRef)->LockSize;
		if (r.left > rX || r.top > rY || r.right < rX || r.bottom < rY)
			// Unlock, then relock the whole thing
			(*ppTexRef)->Unlock();
		else return true;
	}
	// ensure it's locked
	if (!(*ppTexRef)->Lock()) return false;
	// success
	return true;
}

bool CSurface::SetPix(int iX, int iY, uint8_t byCol)
{
	return SetPixDw(iX, iY, lpDDrawPal->GetClr(byCol));
}

uint32_t CSurface::GetPixDw(int iX, int iY, bool fApplyModulation, float scale)
{
	uint8_t *pBuf; int iPitch;
	// backup pos
	int iX2 = iX; int iY2 = iY;
	// primary?
	if (fPrimary)
	{
#ifndef USE_CONSOLE
		// OpenGL?
		if (pGL)
		{
			int hgt = static_cast<int32_t>(ceilf(Hgt * scale));
			if (!PrimarySurfaceLockBits)
			{
				int wdt = static_cast<int32_t>(ceilf(Wdt * scale));
				wdt = ((wdt + 3) / 4) * 4; // round up to the next multiple of 4
				PrimarySurfaceLockBits = new unsigned char[wdt * hgt * 3];
				glReadPixels(0, 0, wdt, hgt, GL_BGR, GL_UNSIGNED_BYTE, PrimarySurfaceLockBits);
				PrimarySurfaceLockPitch = wdt * 3;
			}
			return *(uint32_t *)(PrimarySurfaceLockBits + (hgt - iY - 1) * PrimarySurfaceLockPitch + iX * 3);
		}
#endif
	}
	else
	{
		// get+lock affected texture
		if (!ppTex) return 0;
		CTexRef *pTexRef;
		if (!GetLockTexAt(&pTexRef, iX, iY)) return 0;
		pBuf = (uint8_t *)pTexRef->texLock.pBits;
		iPitch = pTexRef->texLock.Pitch;
	}
	// get pix of surface
	uint32_t dwPix = *(uint32_t *)(pBuf + iY * iPitch + iX * 4);
	// this is a ColorByOwner-surface?
	if (pMainSfc)
	{
		uint8_t byAlpha = uint8_t(dwPix >> 24);
		// pix is fully transparent?
		if (byAlpha == 0xff)
			// then get the main surfaces's pixel
			dwPix = pMainSfc->GetPixDw(iX2, iY2, fApplyModulation);
		else
		{
			// otherwise, it's a ColorByOwner-pixel: adjust the color
			if (fApplyModulation)
			{
				if (lpDDraw->dwBlitMode & C4GFXBLIT_CLRSFC_MOD2)
					ModulateClrMOD2(dwPix, ClrByOwnerClr);
				else
					ModulateClr(dwPix, ClrByOwnerClr);
				if (lpDDraw->BlitModulated && !(lpDDraw->dwBlitMode & C4GFXBLIT_CLRSFC_OWNCLR))
					ModulateClr(dwPix, lpDDraw->BlitModulateClr);
			}
			else
				ModulateClr(dwPix, ClrByOwnerClr);
			// does it contain transparency? then blit on main sfc
			if (byAlpha)
			{
				uint32_t dwMainPix = pMainSfc->GetPixDw(iX2, iY2, fApplyModulation);
				BltAlpha(dwMainPix, dwPix); dwPix = dwMainPix;
			}
		}
	}
	else
	{
		// single main surface
		// apply color modulation if desired
		if (fApplyModulation && lpDDraw->BlitModulated)
		{
			if (lpDDraw->dwBlitMode & C4GFXBLIT_MOD2)
				ModulateClrMOD2(dwPix, lpDDraw->BlitModulateClr);
			else
				ModulateClr(dwPix, lpDDraw->BlitModulateClr);
		}
	}
	// return pixel value
	return dwPix;
}

bool CSurface::IsPixTransparent(int iX, int iY)
{
	// get pixel value
	uint32_t dwPix = GetPixDw(iX, iY, false);
	// get alpha value
	return (dwPix >> 24) >= 128;
}

bool CSurface::SetPixDw(int iX, int iY, uint32_t dwClr)
{
	// clip
	if ((iX < ClipX) || (iX > ClipX2) || (iY < ClipY) || (iY > ClipY2)) return true;
	// get+lock affected texture
	if (!ppTex) return false;
	// if color is fully transparent, ensure it's black
	if (dwClr >> 24 == 0xff) dwClr = 0xff000000;
	CTexRef *pTexRef;
	if (!GetLockTexAt(&pTexRef, iX, iY)) return false;
	// ...and set in actual surface
	pTexRef->SetPix(iX, iY, dwClr);
	// success
	return true;
}

bool CSurface::BltPix(int iX, int iY, CSurface *sfcSource, int iSrcX, int iSrcY, bool fTransparency)
{
	// lock target
	CTexRef *pTexRef;
	if (!GetLockTexAt(&pTexRef, iX, iY)) return false;

	uint32_t *pPix = (uint32_t *)(((uint8_t *)pTexRef->texLock.pBits) + iY * pTexRef->texLock.Pitch + iX * 4);
	// get source pix as dword
	uint32_t srcPix = sfcSource->GetPixDw(iSrcX, iSrcY, true);
	// merge
	if (!fTransparency)
	{
		// set it
		*pPix = srcPix;
	}
	else
	{
		if (lpDDraw->dwBlitMode & C4GFXBLIT_ADDITIVE)
			BltAlphaAdd(*pPix, srcPix);
		else
			BltAlpha(*pPix, srcPix);
	}
	// done
	return true;
}

void CSurface::ClearBoxDw(int iX, int iY, int iWdt, int iHgt)
{
	// lock
	if (!Locked) return;
	// clip to target size
	if (iX < 0) { iWdt += iX; iX = 0; }
	if (iY < 0) { iHgt += iY; iY = 0; }
	int iOver;
	iOver = Wdt - (iX + iWdt); if (iOver < 0) iWdt += iOver;
	iOver = Hgt - (iY + iHgt); if (iOver < 0) iHgt += iOver;
	// get textures involved
	int iTexX1 = iX / iTexSize;
	int iTexY1 = iY / iTexSize;
	int iTexX2 = (std::min)((iX + iWdt - 1) / iTexSize + 1, iTexX);
	int iTexY2 = (std::min)((iY + iHgt - 1) / iTexSize + 1, iTexY);
	// clear basesfc?
	bool fBaseSfc = false;
	if (pMainSfc) if (pMainSfc->ppTex) fBaseSfc = true;
	// clear all these textures
	for (int y = iTexY1; y < iTexY2; ++y)
	{
		for (int x = iTexX1; x < iTexX2; ++x)
		{
			CTexRef *pTex = *(ppTex + y * iTexX + x);
			// get current offset in texture
			int iBlitX = iTexSize * x;
			int iBlitY = iTexSize * y;
			// get clearing bounds in texture
			RECT rtClear;
			rtClear.left = (std::max)(iX - iBlitX, 0);
			rtClear.top = (std::max)(iY - iBlitY, 0);
			rtClear.right = (std::min)(iX + iWdt - iBlitX, iTexSize);
			rtClear.bottom = (std::min)(iY + iHgt - iBlitY, iTexSize);
			// is there a base-surface to be cleared first?
			if (fBaseSfc)
			{
				// then get this surface as same offset as from other surface
				// assuming this is only valid as long as there's no texture management,
				// organizing partially used textures together!
				CTexRef *pBaseTex = *(pMainSfc->ppTex + y * iTexX + x);
				pBaseTex->ClearRect(rtClear);
			}
			// clear this texture
			pTex->ClearRect(rtClear);
		}
	}
}

bool CSurface::CopyBytes(uint8_t *pImageData)
{
	// copy image data directly into textures
	CTexRef **ppCurrTex = ppTex, *pTex = *ppTex;
	int iSrcPitch = Wdt * 4; int iLineTotal = 0;
	for (int iY = 0; iY < iTexY; ++iY)
	{
		uint8_t *pSource = pImageData + iSrcPitch * iLineTotal;
		int iLastHeight = pTex->iSize; int iXImgPos = 0;
		for (int iX = 0; iX < iTexX; ++iX)
		{
			pTex = *ppCurrTex++;
			if (!pTex->Lock()) return false;
			uint8_t *pTarget = (uint8_t *)pTex->texLock.pBits;
			int iCpyNum = (std::min)(pTex->iSize, Wdt - iXImgPos) * 4;
			int iYMax = (std::min)(pTex->iSize, Hgt - iLineTotal);
			for (int iLine = 0; iLine < iYMax; ++iLine)
			{
				memcpy(pTarget, pSource, iCpyNum);
				pSource += iSrcPitch;
				pTarget += pTex->iSize * 4;
			}
			pSource += iCpyNum - iSrcPitch * iYMax;
			iXImgPos += pTex->iSize;
		}
		iLineTotal += iLastHeight;
	}
	return true;
}

CTexRef::CTexRef(int iSize, bool fSingle)
{
	// zero fields
#ifndef USE_CONSOLE
	texName = 0;
#endif
	texLock.pBits = nullptr; fIntLock = false;
	// store size
	this->iSize = iSize;
	// add to texture manager
	if (!pTexMgr) pTexMgr = new CTexMgr();
	pTexMgr->RegTex(this);
	// create texture: check ddraw
	if (!lpDDraw) return;
	if (!lpDDraw->DeviceReady()) return;
	// create it!
#ifndef USE_CONSOLE
	if (pGL)
	{
		// OpenGL
		// create mem array for texture creation
		texLock.pBits = new unsigned char[iSize * iSize * 4];
		texLock.Pitch = iSize * 4;
		memset(texLock.pBits, 0xff, texLock.Pitch * iSize);
		// turn mem array into texture
		Unlock();
	}
	else
#endif
		if (lpDDraw)
		{
			texLock.pBits = new unsigned char[iSize * iSize * 4];
			texLock.Pitch = iSize * 4;
			memset(texLock.pBits, 0xff, texLock.Pitch * iSize);
			// Always locked
			LockSize.left = LockSize.top = 0;
			LockSize.right = LockSize.bottom = iSize;
		}
}

CTexRef::~CTexRef()
{
	fIntLock = false;
	// free texture
#ifndef USE_CONSOLE
	if (pGL)
	{
		if (texName && pGL->pCurrCtx) glDeleteTextures(1, &texName);
	}
#endif
	if (lpDDraw) delete[] texLock.pBits; texLock.pBits = nullptr;
	// remove from texture manager
	pTexMgr->UnregTex(this);
}

bool CTexRef::LockForUpdate(RECT &rtUpdate)
{
	// already locked?
	if (texLock.pBits)
	{
		// fully locked
		if (LockSize.left == 0 && LockSize.right == iSize && LockSize.top == 0 && LockSize.bottom == iSize)
		{
			return true;
		}
		else
		{
			// Commit previous changes to the texture
			Unlock();
		}
	}
	// lock
#ifndef USE_CONSOLE
	if (pGL)
	{
		if (texName)
		{
			// prepare texture data
			texLock.pBits = new unsigned char[
				(rtUpdate.right - rtUpdate.left) * (rtUpdate.bottom - rtUpdate.top) * 4];
			texLock.Pitch = (rtUpdate.right - rtUpdate.left) * 4;
			LockSize = rtUpdate;
			return true;
		}
	}
	else
#endif
	{
		// nothing to do
	}
	// failure
	return false;
}

bool CTexRef::Lock()
{
	// already locked?
	if (texLock.pBits) return true;
	LockSize.right = LockSize.bottom = iSize;
	LockSize.top = LockSize.left = 0;
	// lock
#ifndef USE_CONSOLE
	if (pGL)
	{
		if (texName)
		{
			// select context, if not already done
			if (!pGL->pCurrCtx) if (!pGL->MainCtx.Select()) return false;
			// get texture
			texLock.pBits = new unsigned char[iSize * iSize * 4];
			texLock.Pitch = iSize * 4;
			glBindTexture(GL_TEXTURE_2D, texName);
			glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, texLock.pBits);
			return true;
		}
	}
	else
#endif
	{
		// nothing to do
	}
	// failure
	return false;
}

void CTexRef::Unlock()
{
	// locked?
	if (!texLock.pBits || fIntLock) return;
#ifndef USE_CONSOLE
	if (pGL)
	{
		// select context, if not already done
		if (!pGL->pCurrCtx) if (!pGL->MainCtx.Select()) return;
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		if (!texName)
		{
			// create a new texture
			glGenTextures(1, &texName);
			glBindTexture(GL_TEXTURE_2D, texName);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			// Default, changed in PerformBlt if necessary
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, iSize, iSize, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, texLock.pBits);
		}
		else
		{
			// reuse the existing texture
			glBindTexture(GL_TEXTURE_2D, texName);
			glTexSubImage2D(GL_TEXTURE_2D, 0,
				LockSize.left, LockSize.top, LockSize.right - LockSize.left, LockSize.bottom - LockSize.top,
				GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, texLock.pBits);
		}
		delete[] texLock.pBits; texLock.pBits = nullptr;
		// switch back to original context
	}
	else
#endif
	{
		// nothing to do
	}
}

bool CTexRef::ClearRect(RECT &rtClear)
{
	// ensure locked
	if (!LockForUpdate(rtClear)) return false;
	// clear pixels
	for (int y = rtClear.top; y < rtClear.bottom; ++y)
	{
		for (int x = rtClear.left; x < rtClear.right; ++x)
			SetPix(x, y, 0xff000000);
	}
	// success
	return true;
}

bool CTexRef::FillBlack()
{
	// ensure locked
	if (!Lock()) return false;
	// clear pixels
	std::fill_n(reinterpret_cast<std::uint32_t *>(texLock.pBits), iSize * iSize, 0);
	// success
	return true;
}

// texture manager

CTexMgr::CTexMgr()
{
	// clear textures
	Textures.clear();
}

CTexMgr::~CTexMgr()
{
	// unlock all textures
	IntUnlock();
}

void CTexMgr::RegTex(CTexRef *pTex)
{
	// add texture to list
	Textures.push_front(pTex);
}

void CTexMgr::UnregTex(CTexRef *pTex)
{
	// remove texture from list
	Textures.remove(pTex);
	// if list is empty, remove self
	if (Textures.empty()) { delete this; pTexMgr = nullptr; }
}

void CTexMgr::IntLock()
{
	// lock all textures
	int j = Textures.size();
	for (std::list<CTexRef *>::iterator i = Textures.begin(); j--; ++i)
	{
		CTexRef *pRef = *i;
		if (pRef->Lock() && !pRef->texLock.pBits) pRef->fIntLock = true;
	}
}

void CTexMgr::IntUnlock()
{
	// unlock all internally locked textures
	int j = Textures.size();
	for (std::list<CTexRef *>::iterator i = Textures.begin(); j--; ++i)
	{
		CTexRef *pRef = *i;
		if (pRef->fIntLock) { pRef->fIntLock = false; pRef->Unlock(); }
	}
}

CTexMgr *pTexMgr;
const uint8_t FColors[] = { 31, 16, 39, 47, 55, 63, 71, 79, 87, 95, 23, 30, 99, 103 };
