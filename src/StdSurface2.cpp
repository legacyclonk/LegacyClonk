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

CSurface::CSurface()
{
	Default();
}

CSurface::CSurface(uint32_t iWdt, uint32_t iHgt) : fIsBackground(false)
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
	pMainSfc = nullptr;
	ClrByOwnerClr = 0;
	Textures.clear();
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
	Textures = std::move(psfcFrom->Textures);
	pMainSfc = psfcFrom->pMainSfc;
	ClrByOwnerClr = psfcFrom->ClrByOwnerClr;
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
#ifdef _DEBUG
	delete dbg_idx;
	dbg_idx = nullptr;
#endif
}

bool CSurface::IsRenderTarget()
{
	// primary is always OK...
	return fPrimary || IsSingleSurface();
}

void CSurface::NoClip()
{
	ClipX = 0; ClipY = 0; ClipX2 = Wdt - 1; ClipY2 = Hgt - 1;
}

void CSurface::Clip(uint32_t iX, uint32_t iY, uint32_t iX2, uint32_t iY2)
{
	ClipX  = BoundBy(iX,  0u, Wdt - 1); ClipY  = BoundBy(iY,  0u, Hgt - 1);
	ClipX2 = BoundBy(iX2, 0u, Wdt - 1); ClipY2 = BoundBy(iY2, 0u, Hgt - 1);
}

bool CSurface::Create(int iWdt, int iHgt, bool fOwnPal)
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
	{
		Format = pGL->sfcFmt;
	}
#endif
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
	uint32_t maxTextureSize = 4096;
#ifndef USE_CONSOLE
	if (!pGL)
	{
		GLint m = 0;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m);
		maxTextureSize = static_cast<decltype(maxTextureSize)>(m);
	}
#endif

	uint32_t y = 0;

	auto tile = [&maxTextureSize, this](uint32_t &x, uint32_t y, uint32_t height)
	{
		uint32_t w = std::min(Wdt - x, maxTextureSize);
		uint32_t h = std::min(height - y, maxTextureSize);

		Textures.emplace_back(new CTexRef{x, y, w, h});
		if (fIsBackground)
		{
			Textures.back()->FillBlack();
		}

		x += w;
	};

	while (y < Hgt)
	{
		uint32_t height = std::min(Hgt - y, maxTextureSize);
		for (uint32_t x = 0; x < Wdt; )
		{
			tile(x, y, height);
		}

		y += height;
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
	Textures.clear();
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
	if (pBySurface->Textures.empty()) return false;
	// create in same size
	if (!Create(pBySurface->Wdt, pBySurface->Hgt, false)) return false;
	// set main surface
	pMainSfc = pBySurface;
	// lock it
	if (!pMainSfc->Lock()) return false;
	if (!Lock()) { pMainSfc->Unlock(); return false; }
	// set ColorByOwner-pixels
	for (uint32_t iY = 0; iY < Hgt; ++iY)
		for (uint32_t iX = 0; iX < Wdt; ++iX)
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
	if (Textures.empty()) return false;
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
			if (Textures.empty()) return false;
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
			for (const auto &texture : Textures)
			{
				texture->Unlock();
			}
		}
	}
	return true;
}

bool CSurface::GetTexAt(CTexRef **ppTexRef, uint32_t &rX, uint32_t &rY)
{
	// texture present?
	if (Textures.empty()) return false;
	// get pos
	for (const auto &texture : Textures)
	{
		if (Inside<uint32_t>(rX, texture->X, texture->X + texture->Width - 1) && Inside<uint32_t>(rY, texture->Y, texture->Y + texture->Height - 1))
		{
			*ppTexRef = texture.get();
			rX -= texture->X;
			rY -= texture->Y;

			return true;
		}
	}

	assert(false);

	return false;
}

bool CSurface::GetLockTexAt(CTexRef **ppTexRef, uint32_t &rX, uint32_t &rY)
{
	// texture present?
	if (!GetTexAt(ppTexRef, rX, rY)) return false;
	// Already partially locked
	if ((*ppTexRef)->lockData)
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

uint32_t CSurface::GetPixDw(uint32_t iX, uint32_t iY, bool fApplyModulation, float scale)
{
	uint32_t *buffer;
	uint32_t width;
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
		else
#endif
		{
			return 0;
		}
	}
	else
	{
		// get+lock affected texture
		if (Textures.empty()) return 0;
		CTexRef *pTexRef;
		if (!GetLockTexAt(&pTexRef, iX, iY)) return 0;
		buffer = pTexRef->lockData.get();
		width = pTexRef->LockSize.right - pTexRef->LockSize.left;
	}
	// get pix of surface
	uint32_t dwPix = *(buffer + iY * width + iX);
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

bool CSurface::SetPixDw(uint32_t iX, uint32_t iY, uint32_t dwClr)
{
	// clip
	if ((iX < ClipX) || (iX > ClipX2) || (iY < ClipY) || (iY > ClipY2)) return true;
	// get+lock affected texture
	if (Textures.empty()) return false;
	// if color is fully transparent, ensure it's black
	if (dwClr >> 24 == 0xff) dwClr = 0xff000000;
	CTexRef *pTexRef;
	if (!GetLockTexAt(&pTexRef, iX, iY)) return false;
	// ...and set in actual surface
	pTexRef->SetPix(iX, iY, dwClr);
	// success
	return true;
}

bool CSurface::BltPix(uint32_t iX, uint32_t iY, CSurface *sfcSource, int iSrcX, int iSrcY, bool fTransparency)
{
	// lock target
	CTexRef *pTexRef;
	if (!GetLockTexAt(&pTexRef, iX, iY)) return false;

	uint32_t *pPix = pTexRef->lockData.get() + iY * (pTexRef->LockSize.right - pTexRef->LockSize.left) + iX;
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

void CSurface::ClearBoxDw(uint32_t iX, uint32_t iY, uint32_t iWdt, uint32_t iHgt)
{
	// lock
	if (!Locked) return;
	// clip to target size
	if (iX < 0) { iWdt += iX; iX = 0; }
	if (iY < 0) { iHgt += iY; iY = 0; }
	int iOver;
	iOver = Wdt - (iX + iWdt); if (iOver < 0) iWdt += iOver;
	iOver = Hgt - (iY + iHgt); if (iOver < 0) iHgt += iOver;

	if (pMainSfc && !pMainSfc->Textures.empty())
	{
		pMainSfc->ClearBoxDw(iX, iY, iWdt, iHgt);
	}

	for (const auto &texture : Textures)
	{
		RECT rect;
		rect.left = std::max<float>(texture->X, iX) - texture->X;
		rect.top = std::max<float>(texture->Y, iY) - texture->Y;
		rect.right = std::min<float>(texture->X + texture->Width, iX + iWdt) - texture->X;
		rect.bottom = std::min<float>(texture->Y + texture->Height, iY + iHgt) - texture->Y;

		if (rect.right <= rect.left || rect.bottom <= rect.top)
		{
			continue;
		}

		texture->ClearRect(rect);
	}
}

CTexRef::CTexRef(uint32_t x, uint32_t y, uint32_t width, uint32_t height) : X{x}, Y{y}, Width{width}, Height{height}
{
	// zero fields
#ifndef USE_CONSOLE
	texName = 0;
#endif
	fIntLock = false;

	// create texture: check ddraw
	if (!lpDDraw) return;
	if (!lpDDraw->DeviceReady()) return;
	// create it!
#ifndef USE_CONSOLE
	if (pGL)
	{
		// OpenGL
		// create mem array for texture creation
		lockData.reset(new uint32_t[Width * Height]);
		std::fill_n(lockData.get(), Width * Height, 0xff000000);
		// turn mem array into texture
		Unlock();
	}
	else
#endif
		if (lpDDraw)
		{
			lockData.reset(new uint32_t[Width * Height]);
			std::fill_n(lockData.get(), Width * Height, 0xff000000);
			// Always locked
			LockSize.left = LockSize.top = 0;
			LockSize.right = LockSize.bottom = width;
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
	if (lpDDraw) lockData.release(); lockData = nullptr;
}

bool CTexRef::LockForUpdate(const RECT &rtUpdate)
{
	// already locked?
	if (lockData)
	{
		// fully locked
		if (LockSize.left == 0 && LockSize.right == Width && LockSize.top == 0 && LockSize.bottom == Height)
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
			lockData.reset(new uint32_t[(rtUpdate.right - rtUpdate.left) * (rtUpdate.bottom - rtUpdate.top)]);
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
	if (lockData)
	{
		assert(false);
		return true;
	}
	LockSize.right = Width;
	LockSize.bottom = Height;
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
			lockData.reset(new uint32_t[Width * Height]);
			glBindTexture(GL_TEXTURE_2D, texName);
			glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, lockData.get());
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
	if (!lockData || fIntLock) return;
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
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, lockData.get());
		}
		else
		{
			// reuse the existing texture
			glBindTexture(GL_TEXTURE_2D, texName);
			glTexSubImage2D(GL_TEXTURE_2D, 0,
				LockSize.left, LockSize.top, LockSize.right - LockSize.left, LockSize.bottom - LockSize.top,
				GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, lockData.get());
		}
		lockData.release();
		// switch back to original context
	}
	else
#endif
	{
		// nothing to do
	}
}

bool CTexRef::ClearRect(const RECT &rtClear)
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
	std::fill_n(lockData.get(), Width * Height, 0);
	// success
	return true;
}

void CTexRef::SetPix(int iX, int iY, uint32_t v)
{
	*reinterpret_cast<uint32_t *>(lockData.get() + (iY - LockSize.top) * (LockSize.right - LockSize.left) + (iX - LockSize.left)) = v;
}

const uint8_t FColors[] = { 31, 16, 39, 47, 55, 63, 71, 79, 87, 95, 23, 30, 99, 103 };
