/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
 * Copyright (c) 2017-2020, The LegacyClonk Team and contributors
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

/* NewGfx interfaces */

#include <Standard.h>
#include <StdFacet.h>
#include <StdDDraw2.h>
#include <StdGL.h>
#include <StdNoGfx.h>
#include <StdMarkup.h>
#include <StdFont.h>
#include <StdWindow.h>

#include <stdio.h>
#include <limits.h>
#include <cmath>

// Global access pointer
CStdDDraw *lpDDraw = nullptr;
CStdPalette *lpDDrawPal = nullptr;
int iGfxEngine = -1;

void CBltTransform::SetRotate(int iAngle, float fOffX, float fOffY) // set by angle and rotation offset
{
	// iAngle is in 1/100-degrees (cycling from 0 to 36000)
	// determine sine and cos of reversed angle in radians
	// fAngle = -iAngle/100 * pi/180 = iAngle * -pi/18000
	float fAngle = (float)iAngle * (-1.7453292519943295769236907684886e-4f);
	float fsin = (float)sin(fAngle); float fcos = (float)cos(fAngle);
	// set matrix values
	mat[0] = +fcos; mat[1] = +fsin; mat[2] = (1 - fcos) * fOffX - fsin * fOffY;
	mat[3] = -fsin; mat[4] = +fcos; mat[5] = (1 - fcos) * fOffY + fsin * fOffX;
	mat[6] = 0; mat[7] = 0; mat[8] = 1;
	/* calculation of rotation matrix:
		x2 = fcos*(x1-fOffX) + fsin*(y1-fOffY) + fOffX
		   = fcos*x1 - fcos*fOffX + fsin*y1 - fsin*fOffY + fOffX
		   = x1*fcos + y1*fsin + (1-fcos)*fOffX - fsin*fOffY

		y2 = -fsin*(x1-fOffX) + fcos*(y1-fOffY) + fOffY
		   = x1*-fsin + fsin*fOffX + y1*fcos - fcos*fOffY + fOffY
		   = x1*-fsin + y1*fcos + fsin*fOffX + (1-fcos)*fOffY */
}

bool CBltTransform::SetAsInv(CBltTransform &r)
{
	// calc inverse of matrix
	float det = r.mat[0] * r.mat[4] * r.mat[8] + r.mat[1] * r.mat[5] * r.mat[6]
		+ r.mat[2] * r.mat[3] * r.mat[7] - r.mat[2] * r.mat[4] * r.mat[6]
		- r.mat[0] * r.mat[5] * r.mat[7] - r.mat[1] * r.mat[3] * r.mat[8];
	if (!det) { Set(1, 0, 0, 0, 1, 0, 0, 0, 1); return false; }
	mat[0] = (r.mat[4] * r.mat[8] - r.mat[5] * r.mat[7]) / det;
	mat[1] = (r.mat[2] * r.mat[7] - r.mat[1] * r.mat[8]) / det;
	mat[2] = (r.mat[1] * r.mat[5] - r.mat[2] * r.mat[4]) / det;
	mat[3] = (r.mat[5] * r.mat[6] - r.mat[3] * r.mat[8]) / det;
	mat[4] = (r.mat[0] * r.mat[8] - r.mat[2] * r.mat[6]) / det;
	mat[5] = (r.mat[2] * r.mat[3] - r.mat[0] * r.mat[5]) / det;
	mat[6] = (r.mat[3] * r.mat[7] - r.mat[4] * r.mat[6]) / det;
	mat[7] = (r.mat[1] * r.mat[6] - r.mat[0] * r.mat[7]) / det;
	mat[8] = (r.mat[0] * r.mat[4] - r.mat[1] * r.mat[3]) / det;
	return true;
}

void CBltTransform::TransformPoint(float &rX, float &rY)
{
	// apply matrix
	float fW = mat[6] * rX + mat[7] * rY + mat[8];
	// store in temp, so original rX is used for calculation of rY
	float fX = (mat[0] * rX + mat[1] * rY + mat[2]) / fW;
	rY = (mat[3] * rX + mat[4] * rY + mat[5]) / fW;
	rX = fX; // apply temp
}

CPattern &CPattern::operator=(const CPattern &nPattern)
{
	pClrs        = nPattern.pClrs;
	pAlpha       = nPattern.pAlpha;
	sfcPattern8  = nPattern.sfcPattern8;
	sfcPattern32 = nPattern.sfcPattern32;
	if (sfcPattern32) sfcPattern32->Lock();
	delete[] CachedPattern;
	if (nPattern.CachedPattern)
	{
		CachedPattern = new uint32_t[sfcPattern32->Wdt * sfcPattern32->Hgt];
		memcpy(CachedPattern, nPattern.CachedPattern, sfcPattern32->Wdt * sfcPattern32->Hgt * 4);
	}
	else
	{
		CachedPattern = nullptr;
	}
	Wdt        = nPattern.Wdt;
	Hgt        = nPattern.Hgt;
	Zoom       = nPattern.Zoom;
	Monochrome = nPattern.Monochrome;
	return *this;
}

bool CPattern::Set(CSurface *sfcSource, int iZoom, bool fMonochrome)
{
	// Safety
	if (!sfcSource) return false;
	// Clear existing pattern
	Clear();
	// new style: simply store pattern for modulation or shifting, which will be decided upon use
	sfcPattern32 = sfcSource;
	sfcPattern32->Lock();
	Wdt = sfcPattern32->Wdt;
	Hgt = sfcPattern32->Hgt;
	// set zoom
	Zoom = iZoom;
	// set flags
	Monochrome = fMonochrome;
	CachedPattern = new uint32_t[Wdt * Hgt];
	for (int y = 0; y < Hgt; ++y)
		for (int x = 0; x < Wdt; ++x)
		{
			CachedPattern[y * Wdt + x] = sfcPattern32->GetPixDw(x, y, false);
		}
	return true;
}

bool CPattern::Set(CSurface8 *sfcSource, int iZoom, bool fMonochrome)
{
	// Safety
	if (!sfcSource) return false;
	// Clear existing pattern
	Clear();
	// new style: simply store pattern for modulation or shifting, which will be decided upon use
	sfcPattern8 = sfcSource;
	Wdt = sfcPattern8->Wdt;
	Hgt = sfcPattern8->Hgt;
	// set zoom
	Zoom = iZoom;
	// set flags
	Monochrome = fMonochrome;
	CachedPattern = nullptr;
	return true;
}

CPattern::CPattern()
{
	// disable
	sfcPattern32 = nullptr;
	sfcPattern8 = nullptr;
	CachedPattern = nullptr;
	Zoom = 0;
	Monochrome = false;
	pClrs = nullptr; pAlpha = nullptr;
}

void CPattern::Clear()
{
	// pattern assigned
	if (sfcPattern32)
	{
		// unlock it
		sfcPattern32->Unlock();
		// clear field
		sfcPattern32 = nullptr;
	}
	sfcPattern8 = nullptr;
	delete[] CachedPattern; CachedPattern = nullptr;
}

bool CPattern::PatternClr(int iX, int iY, uint8_t &byClr, uint32_t &dwClr, CStdPalette &rPal) const
{
	// pattern assigned?
	if (!sfcPattern32 && !sfcPattern8) return false;
	// position zoomed?
	if (Zoom) { iX /= Zoom; iY /= Zoom; }
	// modulate position
	((unsigned int &)iX) %= Wdt; ((unsigned int &)iY) %= Hgt;
	// new style: modulate clr
	if (CachedPattern)
	{
		uint32_t dwPix = CachedPattern[iY * Wdt + iX];
		if (byClr)
		{
			if (Monochrome)
				ModulateClrMonoA(dwClr, uint8_t(dwPix), uint8_t(dwPix >> 24));
			else
				ModulateClrA(dwClr, dwPix);
			LightenClr(dwClr);
		}
		else dwClr = dwPix;
	}
	// old style?
	else if (sfcPattern8)
	{
		// if color triplet is given, use it
		uint8_t byShift = sfcPattern8->GetPix(iX, iY);
		if (pClrs)
		{
			// IFT (alpha only)
			int iAShift = 0; if (byClr & 0xf0) iAShift = 3;
			// compose color
			dwClr = RGB(pClrs[byShift * 3 + 2], pClrs[byShift * 3 + 1], pClrs[byShift * 3]) + (pAlpha[byShift + iAShift] << 24);
		}
		else
		{
			// shift color index and return indexed color
			byClr += byShift;
			dwClr = rPal.GetClr(byClr);
		}
	}
	// success
	return true;
}

CGammaControl::~CGammaControl()
{
	delete[] red;;
}

void CGammaControl::SetClrChannel(uint16_t *pBuf, uint8_t c1, uint8_t c2, int c3, uint16_t *ref)
{
	// Using this minimum value, gamma ramp errors on some cards can be avoided
	int MinGamma = 0x100;
	// adjust clr3-value
	++c3;
	// calc ramp
	uint16_t *pBuf1 = pBuf;
	uint16_t *pBuf2 = pBuf + size / 2;
	for (int i = 0; i < size / 2; ++i)
	{
		int i2 = size / 2 - i;
		int size1 = size - 1;
		// interpolate ramps between the three points
		*pBuf1++ = BoundBy(((c1 * i2 + c2 * i) * size1 + (2 * c2 - c1 - c3) * 2 * i * i2) / (size1 * size1 / 512), 0, 0xffff);
		*pBuf2++ = BoundBy(((c2 * i2 + c3 * i) * size1 + (2 * c2 - c1 - c3) * 2 * i * i2) / (size1 * size1 / 512), 0, 0xffff);
	}
	if (ref)
	{
		for (int i = 0; i < size; ++i)
		{
			int c = 0x10000 * i / (size - 1);
			*pBuf = BoundBy(*pBuf + ref[i] - c, MinGamma, 0xffff);
			++pBuf;
		}
	}
	else
	{
		for (int i = 0; i < size; ++i)
		{
			*pBuf = BoundBy<int>(*pBuf, MinGamma, 0xffff);
			++pBuf;
		}
	}
}

void CGammaControl::Set(uint32_t dwClr1, uint32_t dwClr2, uint32_t dwClr3, int nsize, CGammaControl *ref)
{
	if (nsize != size)
	{
		delete[] red;
		red   = new uint16_t[nsize * 3];
		green = &red[nsize];
		blue  = &red[nsize * 2];
		size = nsize;
	}
	// set red, green and blue channel
	SetClrChannel(red,   GetBValue(dwClr1), GetBValue(dwClr2), GetBValue(dwClr3), ref ? ref->red   : 0);
	SetClrChannel(green, GetGValue(dwClr1), GetGValue(dwClr2), GetGValue(dwClr3), ref ? ref->green : 0);
	SetClrChannel(blue,  GetRValue(dwClr1), GetRValue(dwClr2), GetRValue(dwClr3), ref ? ref->blue  : 0);
}

int CGammaControl::GetSize() const
{
	return size;
}

uint32_t CGammaControl::ApplyTo(uint32_t dwClr)
{
	// apply to reg, green and blue color component
	return RGBA(red[GetBValue(dwClr) * 256 / size] >> 8, green[GetGValue(dwClr) * 256 / size] >> 8, blue[GetRValue(dwClr) * 256 / size] >> 8, dwClr >> 24);
}

void CClrModAddMap::Reset(int iResX, int iResY, int iWdtPx, int iHgtPx, int iOffX, int iOffY, uint32_t dwModClr, uint32_t dwAddClr, int x0, int y0, uint32_t dwBackClr, class CSurface *backsfc)
{
	// set values
	iResolutionX = iResX; iResolutionY = iResY;
	this->iOffX = -((iOffX) % iResolutionX);
	this->iOffY = -((iOffY) % iResolutionY);
	// calc w/h required for map
	iWdt = (iWdtPx - this->iOffX + iResolutionX - 1) / iResolutionX + 1;
	iHgt = (iHgtPx - this->iOffY + iResolutionY - 1) / iResolutionY + 1;
	this->iOffX += x0;
	this->iOffY += y0;
	size_t iNewMapSize = iWdt * iHgt;
	if (iNewMapSize > iMapSize || iNewMapSize < iMapSize / 2)
	{
		delete[] pMap;
		pMap = new CClrModAdd[iMapSize = iNewMapSize];
	}
	// is a background color desired?
	if (dwBackClr && backsfc)
	{
		// then draw a background now and fade against transparent later
		lpDDraw->DrawBoxDw(backsfc, x0, y0, x0 + iWdtPx - 1, y0 + iHgtPx - 1, dwBackClr);
		dwModClr = 0xffffffff;
		fFadeTransparent = true;
	}
	else
		fFadeTransparent = false;
	// reset all of map to given values
	int i = iMapSize;
	for (CClrModAdd *pCurr = pMap; i--; ++pCurr)
	{
		pCurr->dwModClr = dwModClr;
		pCurr->dwAddClr = dwAddClr;
	}
}

void CClrModAddMap::ReduceModulation(int cx, int cy, int iRadius1, int iRadius2)
{
	// reveal all within iRadius1; fade off squared until iRadius2
	int i = iMapSize, x = iOffX, y = iOffY, xe = iWdt * iResolutionX + iOffX;
	int iRadius1Sq = iRadius1 * iRadius1, iRadius2Sq = iRadius2 * iRadius2;
	for (CClrModAdd *pCurr = pMap; i--; ++pCurr)
	{
		int d = (x - cx) * (x - cx) + (y - cy) * (y - cy);
		if (d < iRadius2Sq)
		{
			if (d < iRadius1Sq)
				pCurr->dwModClr = 0xffffff; // full visibility
			else
			{
				// partly visible
				int iVis = (iRadius2Sq - d) * 255 / (iRadius2Sq - iRadius1Sq);
				pCurr->dwModClr = fFadeTransparent ? (0xffffff + (std::min<uint32_t>(pCurr->dwModClr >> 24, 255 - iVis) << 24))
					: std::max<uint32_t>(pCurr->dwModClr, RGB(iVis, iVis, iVis));
			}
		}
		// next pos
		x += iResolutionX;
		if (x >= xe) { x = iOffX; y += iResolutionY; }
	}
}

void CClrModAddMap::AddModulation(int cx, int cy, int iRadius1, int iRadius2, uint8_t byTransparency)
{
	// hide all within iRadius1; fade off squared until iRadius2
	int i = iMapSize, x = iOffX, y = iOffY, xe = iWdt * iResolutionX + iOffX;
	int iRadius1Sq = iRadius1 * iRadius1, iRadius2Sq = iRadius2 * iRadius2;
	for (CClrModAdd *pCurr = pMap; i--; ++pCurr)
	{
		int d = (x - cx) * (x - cx) + (y - cy) * (y - cy);
		if (d < iRadius2Sq)
		{
			if (d < iRadius1Sq && !byTransparency)
				pCurr->dwModClr = 0x000000; // full invisibility
			else
			{
				// partly visible
				int iVis = std::min<int>(255 - std::min<int>((iRadius2Sq - d) * 255 / (iRadius2Sq - iRadius1Sq), 255) + byTransparency, 255);
				pCurr->dwModClr = fFadeTransparent ? (0xffffff + (std::max<uint32_t>(pCurr->dwModClr >> 24, 255 - iVis) << 24))
					: std::min<uint32_t>(pCurr->dwModClr, RGB(iVis, iVis, iVis));
			}
		}
		// next pos
		x += iResolutionX;
		if (x >= xe) { x = iOffX; y += iResolutionY; }
	}
}

uint32_t CClrModAddMap::GetModAt(int x, int y) const
{
	// slower, more accurate method: Interpolate between 4 neighboured modulations
	x -= iOffX;
	y -= iOffY;
	int tx = BoundBy(x / iResolutionX, 0, iWdt - 1);
	int ty = BoundBy(y / iResolutionY, 0, iHgt - 1);
	int tx2 = (std::min)(tx + 1, iWdt - 1);
	int ty2 = (std::min)(ty + 1, iHgt - 1);
	CColorFadeMatrix clrs(tx * iResolutionX, ty * iResolutionY, iResolutionX, iResolutionY, pMap[ty * iWdt + tx].dwModClr, pMap[ty * iWdt + tx2].dwModClr, pMap[ty2 * iWdt + tx].dwModClr, pMap[ty2 * iWdt + tx2].dwModClr);
	return clrs.GetColorAt(x, y);
}

CColorFadeMatrix::CColorFadeMatrix(int iX, int iY, int iWdt, int iHgt, uint32_t dwClr1, uint32_t dwClr2, uint32_t dwClr3, uint32_t dwClr4)
	: ox(iX), oy(iY), w(iWdt), h(iHgt)
{
	uint32_t dwMask = 0xff;
	for (int iChan = 0; iChan < 4; (++iChan), (dwMask <<= 8))
	{
		int c0 = (dwClr1 & dwMask) >> (iChan * 8);
		int cx = (dwClr2 & dwMask) >> (iChan * 8);
		int cy = (dwClr3 & dwMask) >> (iChan * 8);
		int ce = (dwClr4 & dwMask) >> (iChan * 8);
		clrs[iChan].c0 = c0;
		clrs[iChan].cx = cx - c0;
		clrs[iChan].cy = cy - c0;
		clrs[iChan].ce = ce;
	}
}

uint32_t CColorFadeMatrix::GetColorAt(int iX, int iY)
{
	iX -= ox; iY -= oy;
	uint32_t dwResult = 0x00;
	for (int iChan = 0; iChan < 4; ++iChan)
	{
		int clr = clrs[iChan].c0 + clrs[iChan].cx * iX / w + clrs[iChan].cy * iY / h;
		clr += iX * iY * (clrs[iChan].ce - clr) / (w * h);
		dwResult |= (BoundBy(clr, 0, 255) << (iChan * 8));
	}
	return dwResult;
}

void CStdShader::SetMacro(const std::string &key, const std::string &value)
{
	macros[key] = value;
}

void CStdShader::UnsetMacro(const std::string &key)
{
	macros.erase(key);
}

void CStdShader::SetSource(const std::string &source)
{
	this->source = source;
}

void CStdShader::SetType(Type type)
{
	this->type = type;
}

void CStdShader::Clear()
{
	source.clear();
	macros.clear();
	errorMessage.clear();
}

bool CStdShaderProgram::AddShader(CStdShader *shader)
{
	EnsureProgram();
	if (std::find(shaders.cbegin(), shaders.cend(), shader) != shaders.cend())
	{
		return true;
	}

	if (AddShaderInt(shader))
	{
		shaders.push_back(shader);
		return true;
	}

	return false;
}

void CStdShaderProgram::Clear()
{
	shaders.clear();
}

void CStdDDraw::Default()
{
	RenderTarget = nullptr;
	ClipAll = false;
	Active = false;
	BlitModulated = false;
	dwBlitMode = 0;
	Gamma.Default();
	DefRamp.Default();
	currentShaderProgram = nullptr;
	lpPrimary = lpBack = nullptr;
	fUseClrModMap = false;
}

void CStdDDraw::Clear()
{
	sLastError.Clear();
	DisableGamma();
	Active = BlitModulated = fUseClrModMap = false;
	dwBlitMode = 0;
	while (!modes.empty())
	{
		modes.pop();
	}
	currentShaderProgram = nullptr;
}

bool CStdDDraw::WipeSurface(CSurface *sfcSurface)
{
	if (!sfcSurface) return false;
	return sfcSurface->Wipe();
}

bool CStdDDraw::GetSurfaceSize(CSurface *sfcSurface, int &iWdt, int &iHgt)
{
	return sfcSurface->GetSurfaceSize(iWdt, iHgt);
}

void CStdDDraw::PushDrawMode(DrawMode mode)
{
	DrawMode old = modes.empty() ? DrawMode::Other : modes.top();
	modes.push(mode);
	DrawModeChanged(old, mode);
}

void CStdDDraw::PopDrawMode()
{
	assert(!modes.empty());

	DrawMode old = modes.top();
	modes.pop();
	DrawModeChanged(old, modes.empty() ? DrawMode::Other : modes.top());
}

bool CStdDDraw::SetPrimaryPalette(uint8_t *pBuf, uint8_t *pAlphaBuf)
{
	// store into loaded pal
	memcpy(&Pal.Colors, pBuf, 768);
	// store alpha pal
	if (pAlphaBuf) memcpy(Pal.Alpha, pAlphaBuf, 256);
	// success
	return true;
}

bool CStdDDraw::SubPrimaryClipper(int iX1, int iY1, int iX2, int iY2)
{
	// Set sub primary clipper
	SetPrimaryClipper((std::max)(iX1, ClipX1), (std::max)(iY1, ClipY1), (std::min)(iX2, ClipX2), (std::min)(iY2, ClipY2));
	return true;
}

bool CStdDDraw::StorePrimaryClipper()
{
	// Store current primary clipper
	StClipX1 = ClipX1; StClipY1 = ClipY1; StClipX2 = ClipX2; StClipY2 = ClipY2;
	return true;
}

bool CStdDDraw::RestorePrimaryClipper()
{
	// Restore primary clipper
	SetPrimaryClipper(StClipX1, StClipY1, StClipX2, StClipY2);
	return true;
}

bool CStdDDraw::SetPrimaryClipper(int iX1, int iY1, int iX2, int iY2)
{
	// set clipper
	ClipX1 = iX1; ClipY1 = iY1; ClipX2 = iX2; ClipY2 = iY2;
	UpdateClipper();
	// Done
	return true;
}

bool CStdDDraw::NoPrimaryClipper()
{
	// apply maximum clipper
	SetPrimaryClipper(0, 0, 439832, 439832);
	// Done
	return true;
}

bool CStdDDraw::CalculateClipper(int *const iX, int *const iY, int *const iWdt, int *const iHgt)
{
	// no render target? do nothing
	if (!RenderTarget || !Active) return false;
	// negative/zero?
	*iWdt = (std::min<uint32_t>)(ClipX2, RenderTarget->Wdt - 1) - ClipX1 + 1;
	*iHgt = (std::min<uint32_t>)(ClipY2, RenderTarget->Hgt - 1) - ClipY1 + 1;
	*iX = ClipX1; if (*iX < 0) { *iWdt += *iX; *iX = 0; }
	*iY = ClipY1; if (*iY < 0) { *iHgt += *iY; *iY = 0; }
	if (*iWdt <= 0 || *iHgt <= 0)
	{
		ClipAll = true;
		return false;
	}
	ClipAll = false;

	return true;
}

void CStdDDraw::BlitLandscape(CSurface *sfcSource, CSurface *sfcSource2, CSurface *sfcSource3, int fx, int fy,
	CSurface *sfcTarget, int tx, int ty, int wdt, int hgt)
{
	(void) sfcSource2;
	(void) sfcSource3;
	Blit(sfcSource, fx, fy, wdt, hgt, sfcTarget, tx, ty, wdt, hgt, false);
}

void CStdDDraw::Blit8Fast(CSurface8 *sfcSource, int fx, int fy,
	CSurface *sfcTarget, int tx, int ty, int wdt, int hgt)
{
	// blit 8bit-sfc
	// lock surfaces
	assert(sfcTarget->fPrimary);
	bool fRender = sfcTarget->IsRenderTarget();
	if (!fRender) if (!sfcTarget->Lock())
	{
		return;
	}
	// blit 8 bit pix
	for (int ycnt = 0; ycnt < hgt; ++ycnt)
		for (int xcnt = 0; xcnt < wdt; ++xcnt)
		{
			uint8_t byPix = sfcSource->GetPix(fx + xcnt, fy + ycnt);
			if (byPix) DrawPixInt(sfcTarget, (float)(tx + xcnt), (float)(ty + ycnt), sfcSource->pPal->GetClr(byPix));
		}
	// unlock
	if (!fRender) sfcTarget->Unlock();
}

bool CStdDDraw::Blit(CSurface *sfcSource, float fx, float fy, float fwdt, float fhgt,
	CSurface *sfcTarget, float tx, float ty, float twdt, float thgt,
	bool fSrcColKey, CBltTransform *pTransform, bool noScalingCorrection)
{
	// safety
	if (!sfcSource || !sfcTarget || !twdt || !thgt || !fwdt || !fhgt) return false;
	// emulated blit?
	if (!sfcTarget->IsRenderTarget())
	{
		return Blit8(sfcSource, static_cast<int>(fx), static_cast<int>(fy), static_cast<int>(fwdt), static_cast<int>(fhgt), sfcTarget, static_cast<int>(tx), static_cast<int>(ty), static_cast<int>(twdt), static_cast<int>(thgt), fSrcColKey, pTransform);
	}
	// calc stretch

	const auto scalingCorrection = (pApp->GetScale() != 1.f && !noScalingCorrection ? 0.5f : 0.f);
	if (scalingCorrection != 0)
	{
		if (fwdt > 1)
		{
			fx += scalingCorrection;
			fwdt -= 2 * scalingCorrection;
		}
		if (fhgt > 1)
		{
			fy += scalingCorrection;
			fhgt -= 2 * scalingCorrection;
		}
	}


	float scaleX = twdt / fwdt;
	float scaleY = thgt / fhgt;
	// bound
	if (ClipAll) return true;
	// check exact
	bool fExact = !pTransform && fwdt == twdt && fhgt == thgt;
	// inside screen?
	if (twdt <= 0 || thgt <= 0) return false;
	// prepare rendering to surface
	if (!PrepareRendering(sfcTarget)) return false;
	// texture present?
	if (sfcSource->Textures.empty())
	{
		// primary surface?
		if (sfcSource->fPrimary)
		{
			// blit emulated
			return Blit8(sfcSource, static_cast<int>(fx), int(fy), int(fwdt), int(fhgt), sfcTarget, static_cast<int>(tx), static_cast<int>(ty), static_cast<int>(twdt), static_cast<int>(thgt), fSrcColKey);
		}
		return false;
	}
	// create blitting struct
	CBltData BltData;
	// pass down pTransform
	BltData.pTransform = pTransform;
	// blit with basesfc?
	bool fBaseSfc = false;
	if (sfcSource->pMainSfc) if (!sfcSource->pMainSfc->Textures.empty()) fBaseSfc = true;
	// set blitting state - done by PerformBlt
	// blit from all these textures
	SetTexture();

	for (const auto &texture : sfcSource->Textures)
	{
		float scaleX2 = scaleX * (texture->Width + DDrawCfg.fTexIndent * 2);
		float scaleY2 = scaleY * (texture->Height + DDrawCfg.fTexIndent * 2);

		FLOAT_RECT vertices;
		vertices.left = std::max<float>(texture->X, fx);
		vertices.top = std::max<float>(texture->Y, fy);
		vertices.right = std::min<float>(texture->X + texture->Width, fx + fwdt);
		vertices.bottom = std::min<float>(texture->Y + texture->Height, fy + fhgt);

		if (vertices.right <= vertices.left || vertices.bottom <= vertices.top)
		{
			continue;
		}

		if (!modes.empty() && modes.top() == DrawMode::Text)
		{
			asm("nop");
		}

		FLOAT_RECT target;
		target.left   = (vertices.left - fx) * scaleX + tx;
		target.top    = (vertices.top - fy) * scaleY + ty;
		target.right  = (vertices.right - fx) * scaleX + tx;
		target.bottom = (vertices.bottom - fy) * scaleY + ty;

		CBltTransform texPos;
		texPos.SetMoveScale(
			(vertices.left + DDrawCfg.fTexIndent) / texture->Width - (target.left + DDrawCfg.fBlitOff) / scaleX2,
			(vertices.top  + DDrawCfg.fTexIndent) / texture->Height - (target.top  + DDrawCfg.fBlitOff) / scaleY2,
			1 / scaleX2,
			1 / scaleY2);
		// set up blit data as rect
		BltData.vtVtx[0].coordinates[0] = target.left  + DDrawCfg.fBlitOff; BltData.vtVtx[0].coordinates[1] = target.top    + DDrawCfg.fBlitOff;
		BltData.vtVtx[1].coordinates[0] = target.right + DDrawCfg.fBlitOff; BltData.vtVtx[1].coordinates[1] = target.top    + DDrawCfg.fBlitOff;
		BltData.vtVtx[2].coordinates[0] = target.right + DDrawCfg.fBlitOff; BltData.vtVtx[2].coordinates[1] = target.bottom + DDrawCfg.fBlitOff;
		BltData.vtVtx[3].coordinates[0] = target.left  + DDrawCfg.fBlitOff; BltData.vtVtx[3].coordinates[1] = target.bottom + DDrawCfg.fBlitOff;

		for (auto &vertex : BltData.vtVtx)
		{
			vertex.textureCoordinates[0] = vertex.coordinates[0];
			vertex.textureCoordinates[1] = vertex.coordinates[1];

			texPos.TransformPoint(vertex.textureCoordinates[0], vertex.textureCoordinates[1]);
		}

		// is there a base-surface to be blitted first?
		if (fBaseSfc)
		{
			// then get this surface as same offset as from other surface
			// assuming this is only valid as long as there's no texture management,
			// organizing partially used textures together!
			Blit(sfcSource->pMainSfc, fx, fy, fwdt, fhgt, sfcTarget, tx, ty, twdt, thgt, fSrcColKey, pTransform, noScalingCorrection);
		}
		// base blit
		PerformBlt(BltData, texture.get(), BlitModulated ? BlitModulateClr : 0xffffff, !!(dwBlitMode & C4GFXBLIT_MOD2), fExact);
		// overlay
		if (fBaseSfc)
		{
			uint32_t dwModClr = sfcSource->ClrByOwnerClr;
			// apply global modulation to overlay surfaces only if desired
			if (BlitModulated && !(dwBlitMode & C4GFXBLIT_CLRSFC_OWNCLR))
				ModulateClr(dwModClr, BlitModulateClr);
			PerformBlt(BltData, texture.get(), dwModClr, !!(dwBlitMode & C4GFXBLIT_CLRSFC_MOD2), fExact);
		}
	}
	// reset texture
	ResetTexture();
	// success
	return true;
}

bool CStdDDraw::Blit8(CSurface *sfcSource, int fx, int fy, int fwdt, int fhgt,
	CSurface *sfcTarget, int tx, int ty, int twdt, int thgt,
	bool fSrcColKey, CBltTransform *pTransform)
{
	if (!pTransform) return BlitRotate(sfcSource, fx, fy, fwdt, fhgt, sfcTarget, tx, ty, twdt, thgt, 0, fSrcColKey != false);
	// safety
	if (!fwdt || !fhgt) return true;
	// Lock the surfaces
	if (!sfcSource->Lock())
		return false;
	if (!sfcTarget->Lock())
	{
		sfcSource->Unlock(); return false;
	}
	// transformed, emulated blit
	// Calculate transform target rect
	CBltTransform Transform;
	Transform.SetMoveScale(tx - (float)fx * twdt / fwdt, ty - (float)fy * thgt / fhgt, (float)twdt / fwdt, (float)thgt / fhgt);
	Transform *= *pTransform;
	CBltTransform TransformBack;
	TransformBack.SetAsInv(Transform);
	float ttx0 = (float)tx, tty0 = (float)ty, ttx1 = (float)(tx + twdt), tty1 = (float)(ty + thgt);
	float ttx2 = (float)ttx0, tty2 = (float)tty1, ttx3 = (float)ttx1, tty3 = (float)tty0;
	pTransform->TransformPoint(ttx0, tty0);
	pTransform->TransformPoint(ttx1, tty1);
	pTransform->TransformPoint(ttx2, tty2);
	pTransform->TransformPoint(ttx3, tty3);
	int ttxMin = std::max<int>((int)floor((std::min)((std::min)(ttx0, ttx1), (std::min)(ttx2, ttx3))), 0);
	int ttxMax = std::min<int>((int)ceil((std::max)((std::max)(ttx0, ttx1), (std::max)(ttx2, ttx3))), sfcTarget->Wdt);
	int ttyMin = std::max<int>((int)floor((std::min)((std::min)(tty0, tty1), (std::min)(tty2, tty3))), 0);
	int ttyMax = std::min<int>((int)ceil((std::max)((std::max)(tty0, tty1), (std::max)(tty2, tty3))), sfcTarget->Hgt);
	// blit within target rect
	for (int y = ttyMin; y < ttyMax; ++y)
		for (int x = ttxMin; x < ttxMax; ++x)
		{
			float ffx = (float)x, ffy = (float)y;
			TransformBack.TransformPoint(ffx, ffy);
			int ifx = static_cast<int>(ffx), ify = static_cast<int>(ffy);
			if (ifx < fx || ify < fy || ifx >= fx + fwdt || ify >= fy + fhgt) continue;
			sfcTarget->BltPix(x, y, sfcSource, ifx, ify, !!fSrcColKey);
		}
	// Unlock the surfaces
	sfcSource->Unlock();
	sfcTarget->Unlock();
	return true;
}

bool CStdDDraw::BlitRotate(CSurface *sfcSource, int fx, int fy, int fwdt, int fhgt,
	CSurface *sfcTarget, int tx, int ty, int twdt, int thgt,
	int iAngle, bool fTransparency)
{
	// rendertarget?
	if (sfcTarget->IsRenderTarget())
	{
		CBltTransform rot;
		rot.SetRotate(iAngle, (float)(tx + tx + twdt) / 2, (float)(ty + ty + thgt) / 2);
		return Blit(sfcSource, float(fx), float(fy), float(fwdt), float(fhgt), sfcTarget, tx, ty, twdt, thgt, true, &rot);
	}
	// Object is first stretched to dest rect, then rotated at place.
	int xcnt, ycnt, fcx, fcy, tcx, tcy, cpcx, cpcy;
	int npcx, npcy;
	double mtx[4], dang;
	if (!fwdt || !fhgt || !twdt || !thgt) return false;
	// Lock the surfaces
	if (!sfcSource->Lock())
		return false;
	if (!sfcTarget->Lock())
	{
		sfcSource->Unlock(); return false;
	}
	// Rectangle centers
	fcx = static_cast<int>(fwdt / 2); fcy = static_cast<int>(fhgt / 2);
	tcx = twdt / 2; tcy = thgt / 2;
	// Adjust angle range
	while (iAngle < 0) iAngle += 36000; while (iAngle > 35999) iAngle -= 36000;
	// Exact/free rotation
	switch (iAngle)
	{
	case 0:
		for (ycnt = 0; ycnt < thgt; ycnt++)
			if (Inside<int32_t>(cpcy = ty + tcy - thgt / 2 + ycnt, 0, sfcTarget->Hgt - 1))
				for (xcnt = 0; xcnt < twdt; xcnt++)
					if (Inside<int32_t>(cpcx = tx + tcx - twdt / 2 + xcnt, 0, sfcTarget->Wdt - 1))
						sfcTarget->BltPix(cpcx, cpcy, sfcSource, xcnt * fwdt / twdt + fx, ycnt * fhgt / thgt + fy, fTransparency);
		break;

	case 9000:
		for (ycnt = 0; ycnt < thgt; ycnt++)
			if (Inside<int32_t>(cpcx = ty + tcy + thgt / 2 - ycnt, 0, sfcTarget->Wdt - 1))
				for (xcnt = 0; xcnt < twdt; xcnt++)
					if (Inside<int32_t>(cpcy = tx + tcx - twdt / 2 + xcnt, 0, sfcTarget->Hgt - 1))
						sfcTarget->BltPix(cpcx, cpcy, sfcSource, xcnt * fwdt / twdt + fx, ycnt * fhgt / thgt + fy, fTransparency);
		break;

	case 18000:
		for (ycnt = 0; ycnt < thgt; ycnt++)
			if (Inside<int32_t>(cpcy = ty + tcy + thgt / 2 - ycnt, 0, sfcTarget->Hgt - 1))
				for (xcnt = 0; xcnt < twdt; xcnt++)
					if (Inside<int32_t>(cpcx = tx + tcx + twdt / 2 - xcnt, 0, sfcTarget->Wdt - 1))
						sfcTarget->BltPix(cpcx, cpcy, sfcSource, xcnt * fwdt / twdt + fx, ycnt * fhgt / thgt + fy, fTransparency);
		break;

	case 27000:
		for (ycnt = 0; ycnt < thgt; ycnt++)
			if (Inside<int32_t>(cpcx = ty + tcy - thgt / 2 + ycnt, 0, sfcTarget->Wdt - 1))
				for (xcnt = 0; xcnt < twdt; xcnt++)
					if (Inside<int32_t>(cpcy = tx + tcx + twdt / 2 - xcnt, 0, sfcTarget->Hgt - 1))
						sfcTarget->BltPix(cpcx, cpcy, sfcSource, xcnt * fwdt / twdt + fx, ycnt * fhgt / thgt + fy, fTransparency);
		break;

	default:
		// Calculate rotation matrix
		dang = M_PI * iAngle / 18000.0;
		mtx[0] = cos(dang); mtx[1] = -sin(dang);
		mtx[2] = sin(dang); mtx[3] = cos(dang);
		// Blit source rect
		for (ycnt = 0; ycnt < fhgt; ycnt++)
		{
			// Source line start
			for (xcnt = 0; xcnt < fwdt; xcnt++)
			{
				// Current pixel coordinate as from source
				cpcx = xcnt - fcx; cpcy = ycnt - fcy;
				// Convert to coordinate as in dest
				cpcx = cpcx * twdt / fwdt; cpcy = cpcy * thgt / fhgt;
				// Rotate current pixel coordinate
				npcx = (int)(mtx[0] * cpcx + mtx[1] * cpcy);
				npcy = (int)(mtx[2] * cpcx + mtx[3] * cpcy);
				// Place in dest
				sfcTarget->BltPix(tx + tcx + npcx,     ty + tcy + npcy, sfcSource, xcnt + fx, ycnt + fy, fTransparency);
				sfcTarget->BltPix(tx + tcx + npcx + 1, ty + tcy + npcy, sfcSource, xcnt + fx, ycnt + fy, fTransparency);
			}
		}
		break;
	}

	// Unlock the surfaces
	sfcSource->Unlock();
	sfcTarget->Unlock();
	return true;
}

#ifdef C4ENGINE

bool CStdDDraw::Error(const char *szMsg)
{
	sLastError.Copy(szMsg);
	Log(szMsg); return false;
}

#else

bool CStdDDraw::Error(const char *szMsg)
{
	sLastError.Copy(szMsg);
	return false;
}

#endif

bool CStdDDraw::CreatePrimaryClipper()
{
	// simply setup primary viewport
	SetPrimaryClipper(0, 0, pApp->ScreenWidth() - 1, pApp->ScreenHeight() - 1);
	StClipX1 = ClipX1; StClipY1 = ClipY1; StClipX2 = ClipX2; StClipY2 = ClipY2;
	return true;
}

bool CStdDDraw::AttachPrimaryPalette(CSurface *sfcSurface)
{
	(void) sfcSurface;
	return true;
}

bool CStdDDraw::BlitSurface(CSurface *sfcSurface, CSurface *sfcTarget, int tx, int ty, bool fBlitBase)
{
	if (fBlitBase)
	{
		Blit(sfcSurface, 0.0f, 0.0f, (float)sfcSurface->Wdt, (float)sfcSurface->Hgt, sfcTarget, tx, ty, sfcSurface->Wdt, sfcSurface->Hgt, false);
		return true;
	}
	else
	{
		if (!sfcSurface) return false;
		CSurface *pSfcBase = sfcSurface->pMainSfc;
		sfcSurface->pMainSfc = nullptr;
		Blit(sfcSurface, 0.0f, 0.0f, (float)sfcSurface->Wdt, (float)sfcSurface->Hgt, sfcTarget, tx, ty, sfcSurface->Wdt, sfcSurface->Hgt, false);
		sfcSurface->pMainSfc = pSfcBase;
		return true;
	}
}

bool CStdDDraw::BlitSurfaceTile(CSurface *sfcSurface, CSurface *sfcTarget, int iToX, int iToY, int iToWdt, int iToHgt, int iOffsetX, int iOffsetY, bool fSrcColKey, float scale)
{
	iToHgt /= scale;
	iToWdt /= scale;

	int iSourceWdt, iSourceHgt, iX, iY, iBlitX, iBlitY, iBlitWdt, iBlitHgt;
	// Get source surface size
	if (!GetSurfaceSize(sfcSurface, iSourceWdt, iSourceHgt)) return false;
	// reduce offset to needed size
	iOffsetX %= iSourceWdt;
	iOffsetY %= iSourceHgt;

	CBltTransform transform;
	transform.SetMoveScale(0.f, 0.f, scale, scale);
	// Vertical blits
	for (iY = iToY + iOffsetY; iY < iToY + iToHgt; iY += iSourceHgt)
	{
		// Vertical blit size
		iBlitY = (std::max)(static_cast<int>(std::floor((iToY - iY) / scale)), 0); iBlitHgt = (std::min)(iSourceHgt, iToY + iToHgt - iY) - iBlitY;
		// Horizontal blits
		for (iX = iToX + iOffsetX; iX < iToX + iToWdt; iX += iSourceWdt)
		{
			// Horizontal blit size
			iBlitX = (std::max)(static_cast<int>(std::floor((iToX - iX) / scale)), 0); iBlitWdt = (std::min)(iSourceWdt, iToX + iToWdt - iX) - iBlitX;
			// Blit
			if (!Blit(sfcSurface, float(iBlitX), float(iBlitY), float(iBlitWdt), float(iBlitHgt), sfcTarget, iX + iBlitX, iY + iBlitY, iBlitWdt, iBlitHgt, fSrcColKey, &transform)) return false;
		}
	}
	return true;
}

bool CStdDDraw::BlitSurfaceTile2(CSurface *sfcSurface, CSurface *sfcTarget, int iToX, int iToY, int iToWdt, int iToHgt, int iOffsetX, int iOffsetY, bool fSrcColKey)
{
	int tx, ty, iBlitX, iBlitY, iBlitWdt, iBlitHgt;
	// get tile size
	int iTileWdt = sfcSurface->Wdt;
	int iTileHgt = sfcSurface->Hgt;
	// adjust size of offsets
	iOffsetX %= iTileWdt;
	iOffsetY %= iTileHgt;
	if (iOffsetX < 0) iOffsetX += iTileWdt;
	if (iOffsetY < 0) iOffsetY += iTileHgt;
	// get start pos for blitting
	int iStartX = iToX - iOffsetX;
	int iStartY = iToY - iOffsetY;
	ty = 0;
	// blit vertical
	for (int iY = iStartY; ty < iToHgt; iY += iTileHgt)
	{
		// get vertical blit bounds
		iBlitY = 0; iBlitHgt = iTileHgt;
		if (iY < iToY) { iBlitY = iToY - iY; iBlitHgt += iY - iToY; }
		int iOver = ty + iBlitHgt - iToHgt; if (iOver > 0) iBlitHgt -= iOver;
		// blit horizontal
		tx = 0;
		for (int iX = iStartX; tx < iToWdt; iX += iTileWdt)
		{
			// get horizontal blit bounds
			iBlitX = 0; iBlitWdt = iTileWdt;
			if (iX < iToX) { iBlitX = iToX - iX; iBlitWdt += iX - iToX; }
			iOver = tx + iBlitWdt - iToWdt; if (iOver > 0) iBlitWdt -= iOver;
			// blit
			if (!Blit(sfcSurface, float(iBlitX), float(iBlitY), float(iBlitWdt), float(iBlitHgt), sfcTarget, tx + iToX, ty + iToY, iBlitWdt, iBlitHgt, fSrcColKey)) return false;
			// next col
			tx += iBlitWdt;
		}
		// next line
		ty += iBlitHgt;
	}
	// success
	return true;
}

bool CStdDDraw::TextOut(const char *szText, CStdFont &rFont, float fZoom, CSurface *sfcDest, int iTx, int iTy, uint32_t dwFCol, uint8_t byForm, bool fDoMarkup)
{
	CMarkup Markup(true);
	static char szLinebuf[2500 + 1];
	for (int cnt = 0; SCopySegmentEx(szText, cnt, szLinebuf, fDoMarkup ? '|' : '\n', '\n', 2500); cnt++, iTy += int(fZoom * rFont.GetLineHeight()))
		if (!StringOut(szLinebuf, sfcDest, iTx, iTy, dwFCol, byForm, fDoMarkup, Markup, &rFont, fZoom)) return false;
	return true;
}

bool CStdDDraw::StringOut(const char *szText, CStdFont &rFont, float fZoom, CSurface *sfcDest, int iTx, int iTy, uint32_t dwFCol, uint8_t byForm, bool fDoMarkup)
{
	// init markup
	CMarkup Markup(true);
	// output string
	return StringOut(szText, sfcDest, iTx, iTy, dwFCol, byForm, fDoMarkup, Markup, &rFont, fZoom);
}

bool CStdDDraw::StringOut(const char *szText, CSurface *sfcDest, int iTx, int iTy, uint32_t dwFCol, uint8_t byForm, bool fDoMarkup, CMarkup &Markup, CStdFont *pFont, float fZoom)
{
	// clip
	if (ClipAll) return true;
	// safety
	if (!PrepareRendering(sfcDest)) return false;
	PushDrawMode(DrawMode::Text);
	// convert align
	int iFlags = 0;
	switch (byForm)
	{
	case ACenter: iFlags |= STDFONT_CENTERED; break;
	case ARight: iFlags |= STDFONT_RIGHTALGN; break;
	}
	if (!fDoMarkup) iFlags |= STDFONT_NOMARKUP;
	// draw text
#ifdef C4ENGINE
	pFont->DrawText(sfcDest, iTx, iTy, dwFCol, szText, iFlags, Markup, fZoom);
#endif
	PopDrawMode();
	// done, success
	return true;
}

void CStdDDraw::DrawPix(CSurface *sfcDest, float tx, float ty, uint32_t dwClr)
{
	// apply global modulation
	ClrByCurrentBlitMod(dwClr);
	// apply modulation map
	if (fUseClrModMap)
		ModulateClr(dwClr, pClrModMap->GetModAt((int)tx, (int)ty));
	// Draw
	DrawPixInt(sfcDest, tx, ty, dwClr);
}

void CStdDDraw::DrawBox(CSurface *sfcDest, int iX1, int iY1, int iX2, int iY2, uint8_t byCol)
{
	// get color
	uint32_t dwClr = Pal.GetClr(byCol);
	// offscreen emulation?
	if (!sfcDest->IsRenderTarget())
	{
		int iSfcWdt = sfcDest->Wdt, iSfcHgt = sfcDest->Hgt, xcnt, ycnt;
		// Lock surface
		if (!sfcDest->Lock()) return;
		// Outside of surface/clip
		if ((iX2 < (std::max)(0, ClipX1)) || (iX1 > (std::min)(iSfcWdt - 1, ClipX2))
			|| (iY2 < (std::max)(0, ClipY1)) || (iY1 > (std::min)(iSfcHgt - 1, ClipY2)))
		{
			sfcDest->Unlock(); return;
		}
		// Clip to surface/clip
		if (iX1 < (std::max)(0, ClipX1)) iX1 = (std::max)(0, ClipX1); if (iX2 > (std::min)(iSfcWdt - 1, ClipX2)) iX2 = (std::min)(iSfcWdt - 1, ClipX2);
		if (iY1 < (std::max)(0, ClipY1)) iY1 = (std::max)(0, ClipY1); if (iY2 > (std::min)(iSfcHgt - 1, ClipY2)) iY2 = (std::min)(iSfcHgt - 1, ClipY2);
		// Set lines
		for (ycnt = iY2 - iY1; ycnt >= 0; ycnt--)
			for (xcnt = iX2 - iX1; xcnt >= 0; xcnt--)
				sfcDest->SetPixDw(iX1 + xcnt, iY1 + ycnt, dwClr);
		// Unlock surface
		sfcDest->Unlock();
	}
	// draw as primitives
	DrawBoxDw(sfcDest, iX1, iY1, iX2, iY2, dwClr);
}

void CStdDDraw::DrawHorizontalLine(CSurface *sfcDest, int x1, int x2, int y, uint8_t col)
{
	// if this is a render target: draw it as a box
	if (sfcDest->IsRenderTarget())
	{
		DrawLine(sfcDest, x1, y, x2, y, col);
		return;
	}
	int lWdt = sfcDest->Wdt, lHgt = sfcDest->Hgt, xcnt;
	// Lock surface
	if (!sfcDest->Lock()) return;
	// Fix coordinates
	if (x1 > x2) std::swap(x1, x2);
	// Determine clip
	int clpx1, clpx2, clpy1, clpy2;
	clpx1 = (std::max)(0, ClipX1); clpy1 = (std::max)(0, ClipY1);
	clpx2 = (std::min)(lWdt - 1, ClipX2); clpy2 = (std::min)(lHgt - 1, ClipY2);
	// Outside clip check
	if ((x2 < clpx1) || (x1 > clpx2) || (y < clpy1) || (y > clpy2))
	{
		sfcDest->Unlock(); return;
	}
	// Clip to clip
	if (x1 < clpx1) x1 = clpx1; if (x2 > clpx2) x2 = clpx2;
	// Set line
	for (xcnt = x2 - x1; xcnt >= 0; xcnt--) sfcDest->SetPix(x1 + xcnt, y, col);
	// Unlock surface
	sfcDest->Unlock();
}

void CStdDDraw::DrawVerticalLine(CSurface *sfcDest, int x, int y1, int y2, uint8_t col)
{
	// if this is a render target: draw it as a box
	if (sfcDest->IsRenderTarget())
	{
		DrawLine(sfcDest, x, y1, x, y2, col);
		return;
	}
	int lWdt = sfcDest->Wdt, lHgt = sfcDest->Hgt, ycnt;
	// Lock surface
	if (!sfcDest->Lock()) return;
	// Fix coordinates
	if (y1 > y2) std::swap(y1, y2);
	// Determine clip
	int clpx1, clpx2, clpy1, clpy2;
	clpx1 = (std::max)(0, ClipX1); clpy1 = (std::max)(0, ClipY1);
	clpx2 = (std::min)(lWdt - 1, ClipX2); clpy2 = (std::min)(lHgt - 1, ClipY2);
	// Outside clip check
	if ((x < clpx1) || (x > clpx2) || (y2 < clpy1) || (y1 > clpy2))
	{
		sfcDest->Unlock(); return;
	}
	// Clip to clip
	if (y1 < clpy1) y1 = clpy1; if (y2 > clpy2) y2 = clpy2;
	// Set line
	for (ycnt = y1; ycnt <= y2; ycnt++) sfcDest->SetPix(x, ycnt, col);
	// Unlock surface
	sfcDest->Unlock();
}

void CStdDDraw::DrawFrame(CSurface *sfcDest, int x1, int y1, int x2, int y2, uint8_t col)
{
	DrawHorizontalLine(sfcDest, x1, x2, y1, col);
	DrawHorizontalLine(sfcDest, x1, x2, y2, col);
	DrawVerticalLine(sfcDest, x1, y1, y2, col);
	DrawVerticalLine(sfcDest, x2, y1, y2, col);
}

void CStdDDraw::DrawFrameDw(CSurface *sfcDest, int x1, int y1, int x2, int y2, uint32_t dwClr) // make these parameters float...?
{
	DrawLineDw(sfcDest, (float)x1, (float)y1, (float)x2, (float)y1, dwClr);
	DrawLineDw(sfcDest, (float)x2, (float)y1, (float)x2, (float)y2, dwClr);
	DrawLineDw(sfcDest, (float)x2, (float)y2, (float)x1, (float)y2, dwClr);
	DrawLineDw(sfcDest, (float)x1, (float)y2, (float)x1, (float)y1, dwClr);
}

// Globally locked surface variables - for DrawLine callback crap

void CStdDDraw::DrawPatternedCircle(CSurface *sfcDest, int x, int y, int r, uint8_t col, CPattern &Pattern1, CPattern &Pattern2, CStdPalette &rPal)
{
	if (!sfcDest->Lock()) return;
	for (int ycnt = -r; ycnt < r; ycnt++)
	{
		int lwdt = (int)sqrt(float(r * r - ycnt * ycnt));
		// Set line
		for (int xcnt = x - lwdt; xcnt < x + lwdt; ++xcnt)
		{
			// apply both patterns
			uint32_t dwClr = rPal.GetClr(col);
			Pattern1.PatternClr(xcnt, y + ycnt, col, dwClr, rPal);
			Pattern2.PatternClr(xcnt, y + ycnt, col, dwClr, rPal);
			sfcDest->SetPixDw(xcnt, y + ycnt, dwClr);
		}
	}
	sfcDest->Unlock();
}

void CStdDDraw::SurfaceAllowColor(CSurface *sfcSfc, uint32_t *pdwColors, int iNumColors, bool fAllowZero)
{
	// safety
	if (!sfcSfc) return;
	if (!pdwColors || !iNumColors) return;
	// change colors
	int xcnt, ycnt, wdt = sfcSfc->Wdt, hgt = sfcSfc->Hgt;
	// Lock surface
	if (!sfcSfc->Lock()) return;
	for (ycnt = 0; ycnt < hgt; ycnt++)
	{
		for (xcnt = 0; xcnt < wdt; xcnt++)
		{
			uint32_t dwColor = sfcSfc->GetPixDw(xcnt, ycnt, false);
			uint8_t px = 0;
			for (int i = 0; i < 256; ++i)
				if (lpDDrawPal->GetClr(i) == dwColor)
				{
					px = i; break;
				}
			if (fAllowZero)
			{
				if (!px) continue;
				--px;
			}
			sfcSfc->SetPixDw(xcnt, ycnt, pdwColors[px % iNumColors]);
		}
	}
	sfcSfc->Unlock();
}

void CStdDDraw::Grayscale(CSurface *sfcSfc, int32_t iOffset)
{
	// safety
	if (!sfcSfc) return;
	// change colors
	int xcnt, ycnt, wdt = sfcSfc->Wdt, hgt = sfcSfc->Hgt;
	// Lock surface
	if (!sfcSfc->Lock()) return;
	for (ycnt = 0; ycnt < hgt; ycnt++)
	{
		for (xcnt = 0; xcnt < wdt; xcnt++)
		{
			uint32_t dwColor = sfcSfc->GetPixDw(xcnt, ycnt, false);
			uint32_t r = GetRValue(dwColor), g = GetGValue(dwColor), b = GetBValue(dwColor), a = dwColor >> 24;
			int32_t gray = BoundBy<int32_t>((r + g + b) / 3 + iOffset, 0, 255);
			sfcSfc->SetPixDw(xcnt, ycnt, RGBA(gray, gray, gray, a));
		}
	}
	sfcSfc->Unlock();
}

bool CStdDDraw::GetPrimaryClipper(int &rX1, int &rY1, int &rX2, int &rY2)
{
	// Store drawing clip values
	rX1 = ClipX1; rY1 = ClipY1; rX2 = ClipX2; rY2 = ClipY2;
	// Done
	return true;
}

void CStdDDraw::SetGamma(uint32_t dwClr1, uint32_t dwClr2, uint32_t dwClr3)
{
	// calc ramp
	Gamma.Set(dwClr1, dwClr2, dwClr3, Gamma.size, &DefRamp);
	// set it
	ApplyGammaRamp(Gamma, false);
}

void CStdDDraw::DisableGamma()
{
	// set it
	ApplyGammaRamp(DefRamp, true);
}

void CStdDDraw::EnableGamma()
{
	// set it
	ApplyGammaRamp(Gamma, false);
}

uint32_t CStdDDraw::ApplyGammaTo(uint32_t dwClr)
{
	return Gamma.ApplyTo(dwClr);
}

CStdDDraw *DDrawInit(CStdApp *pApp, int Engine)
{
	// create engine
	switch (iGfxEngine = Engine)
	{
	default: // Use the first engine possible if none selected
#ifndef USE_CONSOLE
	case GFXENGN_OPENGL: lpDDraw = new CStdGL(); break;
#endif
	case GFXENGN_NOGFX: lpDDraw = new CStdNoGfx(); break;
	}
	if (!lpDDraw) return nullptr;
	// init it
	if (!lpDDraw->Init(pApp))
	{
		delete lpDDraw;
		return nullptr;
	}
	// done, success
	return lpDDraw;
}

bool CStdDDraw::Init(CStdApp *pApp)
{
	// set cfg again, as engine has been decided
	DDrawCfg.Set(DDrawCfg.Cfg, DDrawCfg.fTexIndent, DDrawCfg.fBlitOff);

	this->pApp = pApp;

	// store default gamma
	SaveDefaultGammaRamp(pApp->pWindow);

	DebugLog("Init DX");
	DebugLog("  Create DirectDraw...");

	if (!CreateDirectDraw())
		return Error("  CreateDirectDraw failure.");

	DebugLog("  Create Device...");

	if (!CreatePrimarySurfaces())
		return Error("  CreateDevice failure.");

	DebugLog("  Create Clipper");

	if (!CreatePrimaryClipper())
		return Error("  Clipper failure.");

	DrawModeChanged(DrawMode::Other, DrawMode::Other);

	return true;
}

void CStdDDraw::DrawBoxFade(CSurface *sfcDest, int iX, int iY, int iWdt, int iHgt, uint32_t dwClr1, uint32_t dwClr2, uint32_t dwClr3, uint32_t dwClr4, int iBoxOffX, int iBoxOffY)
{
	// clipping not performed - this fn should be called for clipped rects only
	// apply modulation map: Must sectionize blit
	if (fUseClrModMap)
	{
		int iModResX = pClrModMap ? pClrModMap->GetResolutionX() : CClrModAddMap::iDefResolutionX;
		int iModResY = pClrModMap ? pClrModMap->GetResolutionY() : CClrModAddMap::iDefResolutionY;
		iBoxOffX %= iModResX;
		iBoxOffY %= iModResY;
		if (iWdt + iBoxOffX > iModResX || iHgt + iBoxOffY > iModResY)
		{
			if (iWdt <= 0 || iHgt <= 0) return;
			CColorFadeMatrix clrs(iX, iY, iWdt, iHgt, dwClr1, dwClr2, dwClr3, dwClr4);
			int iMaxH = iModResY - iBoxOffY;
			int w, h;
			for (int y = iY, H = iHgt; H > 0; (y += h), (H -= h), (iMaxH = iModResY))
			{
				h = std::min<int>(H, iMaxH);
				int iMaxW = iModResX - iBoxOffX;
				for (int x = iX, W = iWdt; W > 0; (x += w), (W -= w), (iMaxW = iModResX))
				{
					w = std::min<int>(W, iMaxW);
					DrawBoxFade(sfcDest, x, y, w, h, clrs.GetColorAt(x, y), clrs.GetColorAt(x + w, y), clrs.GetColorAt(x, y + h), clrs.GetColorAt(x + w, y + h), 0, 0);
				}
			}
			return;
		}
	}
	// set vertex buffer data
	// vertex order:
	// 0=upper left   dwClr1
	// 1=lower left   dwClr3
	// 2=lower right  dwClr4
	// 3=upper right  dwClr2
	int vtx[8];
	vtx[0] = iX; vtx[1] = iY;
	vtx[2] = iX; vtx[3] = iY + iHgt;
	vtx[4] = iX + iWdt; vtx[5] = iY + iHgt;
	vtx[6] = iX + iWdt; vtx[7] = iY;
	DrawQuadDw(sfcDest, vtx, dwClr1, dwClr3, dwClr4, dwClr2);
}

void CStdDDraw::DrawBoxDw(CSurface *sfcDest, int iX1, int iY1, int iX2, int iY2, uint32_t dwClr)
{
	DrawBoxFade(sfcDest, iX1, iY1, iX2 - iX1 + 1, iY2 - iY1 + 1, dwClr, dwClr, dwClr, dwClr, 0, 0);
}

void CStdDDraw::DrawModeChanged(DrawMode oldMode, DrawMode newMode)
{
	(void) oldMode;
	if (auto it = modeShaders.find(newMode); it != modeShaders.end())
	{
		assert(it->second);
		it->second->Select();
		currentShaderProgram = it->second;
	}
	else
	{
		currentShaderProgram = nullptr;
	}
}

void CStdDDraw::SetShaderProgramForMode(DrawMode mode, CStdShaderProgram *shader)
{
	if (!shader)
	{
		if (auto it = modeShaders.find(mode); it != modeShaders.end())
		{
			modeShaders.erase(it);
		}
	}

	else
	{
		modeShaders[mode] = shader;
	}

	ShaderProgramSet(mode, shader);
}

void CStdDDraw::ClearModeShaderPrograms()
{
	// we don't delete the programs here, the caller has to take care of that
	modeShaders.clear();
}
