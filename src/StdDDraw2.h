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

/* A wrapper class to DirectDraw */

#pragma once

#include <StdSurface2.h>
#include <StdSurface8.h>
#include <StdFont.h>
#include <StdBuf.h>

#include <array>

// texref-predef
class CStdDDraw;
class CTexRef;
class CSurface;
struct CStdPalette;
class CStdGLCtx;
class CStdApp;
class CStdWindow;

// engines
#define GFXENGN_OPENGL   0
#define GFXENGN_NOGFX    3

// Global DDraw access pointer
extern CStdDDraw *lpDDraw;
extern CStdPalette *lpDDrawPal;

extern int iGfxEngine;

// rotation info class
class CBltTransform
{
public:
	float mat[9]; // transformation matrix

public:
	CBltTransform() {} // default: don't init fields

	void Set(float fA, float fB, float fC, float fD, float fE, float fF, float fG, float fH, float fI)
	{
		mat[0] = fA; mat[1] = fB; mat[2] = fC; mat[3] = fD; mat[4] = fE; mat[5] = fF; mat[6] = fG; mat[7] = fH; mat[8] = fI;
	}

	void SetRotate(int iAngle, float fOffX, float fOffY); // set by angle and rotation offset
	bool SetAsInv(CBltTransform &rOfTransform);

	void Rotate(int iAngle, float fOffX, float fOffY) // rotate by angle around rotation offset
	{
		// multiply matrix as seen in SetRotate by own matrix
		CBltTransform rot; rot.SetRotate(iAngle, fOffX, fOffY);
		(*this) *= rot;
	}

	void SetMoveScale(float dx, float dy, float sx, float sy)
	{
		mat[0] = sx; mat[1] = 0;  mat[2] = dx;
		mat[3] = 0;  mat[4] = sy; mat[5] = dy;
		mat[6] = 0;  mat[7] = 0;  mat[8] = 1;
	}

	void MoveScale(float dx, float dy, float sx, float sy)
	{
		// multiply matrix by movescale matrix
		CBltTransform move; move.SetMoveScale(dx, dy, sx, sy);
		(*this) *= move;
	}

	void ScaleAt(float sx, float sy, float tx, float ty)
	{
		// scale and move back so tx and ty remain fixpoints
		MoveScale(-tx * (sx - 1), -ty * (sy - 1), sx, sy);
	}

	CBltTransform &operator*=(CBltTransform &r)
	{
		// transform transformation
		Set(
			mat[0] * r.mat[0] + mat[3] * r.mat[1] + mat[6] * r.mat[2],
			mat[1] * r.mat[0] + mat[4] * r.mat[1] + mat[7] * r.mat[2],
			mat[2] * r.mat[0] + mat[5] * r.mat[1] + mat[8] * r.mat[2],
			mat[0] * r.mat[3] + mat[3] * r.mat[4] + mat[6] * r.mat[5],
			mat[1] * r.mat[3] + mat[4] * r.mat[4] + mat[7] * r.mat[5],
			mat[2] * r.mat[3] + mat[5] * r.mat[4] + mat[8] * r.mat[5],
			mat[0] * r.mat[6] + mat[3] * r.mat[7] + mat[6] * r.mat[8],
			mat[1] * r.mat[6] + mat[4] * r.mat[7] + mat[7] * r.mat[8],
			mat[2] * r.mat[6] + mat[5] * r.mat[7] + mat[8] * r.mat[8]);
		return *this;
	}

	void TransformPoint(float &rX, float &rY); // rotate point by angle
};

// pattern
class CPattern
{
private:
	// pattern surface for new-style patterns
	class CSurface *sfcPattern32;
	class CSurface8 *sfcPattern8;
	// Faster access
	uint32_t *CachedPattern; int Wdt; int Hgt;
	// pattern zoom factor; 0 means no zoom
	int Zoom;
	// pattern is to be applied monochromatic
	bool Monochrome;
	// color array for old-style patterns
	uint32_t *pClrs;
	// alpha array for old-style patterns
	uint32_t *pAlpha;

public:
	CPattern &operator=(const CPattern &);
	bool PatternClr(int iX, int iY, uint8_t &byClr, uint32_t &dwClr, CStdPalette &rPal) const; // apply pattern to color
	bool Set(class CSurface *sfcSource, int iZoom = 0, bool fMonochrome = false); // set and enable pattern
	bool Set(class CSurface8 *sfcSource, int iZoom = 0, bool fMonochrome = false); // set and enable pattern
	void SetColors(uint32_t *pClrs, uint32_t *pAlpha) { this->pClrs = pClrs; this->pAlpha = pAlpha; } // set color triplet for old-style textures
	void SetZoom(int iZoom) { Zoom = iZoom; }
	void Clear(); // clear pattern
	CPattern();
	~CPattern() { Clear(); }
};

// blit position on screen
struct CBltVertex
{
	float ftx, fty; // blit positions
	uint32_t dwModClr; // color modulation
};

// blit data for PerformBlt
struct CBltData
{
	std::array<CBltVertex, 4> vtVtx; // vertices for triangle fan
	CBltTransform TexPos; // texture mapping matrix
	CBltTransform *pTransform; // Vertex transformation
};

// gamma ramp control
class CGammaControl
{
private:
	void SetClrChannel(uint16_t *pBuf, uint8_t c1, uint8_t c2, int c3, uint16_t *ref); // set color channel ramp

protected:
	uint16_t *red, *green, *blue; int size;

public:
	CGammaControl() : red(nullptr), green(nullptr), blue(nullptr), size(0) { Default(); }
	~CGammaControl();
	void Default() { Set(0x000000, 0x808080, 0xffffff, 256, 0); } // set default ramp

	void Set(uint32_t dwClr1, uint32_t dwClr2, uint32_t dwClr3, int size, CGammaControl *ref); // set color ramp
	int GetSize() const;

	uint32_t ApplyTo(uint32_t dwClr); // apply gamma to color value

	friend class CStdDDraw;
	friend class CStdGL;
};

// helper struct
struct FLOAT_RECT { float left, right, top, bottom; };

// direct draw encapsulation
class CStdDDraw
{
public:
	CStdDDraw() : Saturation(255) { lpDDrawPal = &Pal; }
	virtual ~CStdDDraw() { lpDDraw = nullptr; }

public:
	CStdApp *pApp; // the application
	CSurface *lpPrimary; // primary and back surface (emulation...)
	CSurface *lpBack;
	CStdPalette Pal; // 8bit-pal
	bool Active; // set if device is ready to render, etc.
	CGammaControl Gamma; // gamma
	CGammaControl DefRamp; // default gamma ramp
	StdStrBuf sLastError;

protected:
	int ClipX1, ClipY1, ClipX2, ClipY2;
	int StClipX1, StClipY1, StClipX2, StClipY2;
	bool ClipAll; // set if clipper clips everything away
	CSurface *RenderTarget; // current render target
	bool BlitModulated; // set if blits should be modulated with BlitModulateClr
	uint32_t BlitModulateClr; // modulation color for blitting
	uint32_t dwBlitMode; // extra flags for blit
	CClrModAddMap *pClrModMap; // map to be used for global color modulation (invalid if !fUseClrModMap)
	bool fUseClrModMap; // if set, pClrModMap will be checked for color modulations
	unsigned char Saturation; // if < 255, an extra filter is used to reduce the saturation

public:
	// General
	bool Init(CStdApp *pApp);
	virtual void Clear();
	virtual void Default();
	virtual CStdGLCtx *CreateContext(CStdWindow *, CStdApp *) { return nullptr; }
#ifdef _WIN32
	virtual CStdGLCtx *CreateContext(HWND, CStdApp *) { return nullptr; }
#endif
	virtual bool PageFlip(RECT *pSrcRt = nullptr, RECT *pDstRt = nullptr, CStdWindow *pWindow = nullptr) = 0;
	virtual int GetEngine() = 0; // get indexed engine
	virtual bool OnResolutionChanged() = 0; // reinit window for new resolution
	const char *GetLastError() { return sLastError.getData(); }

	// Palette
	bool SetPrimaryPalette(uint8_t *pBuf, uint8_t *pAlphaBuf = nullptr);
	bool AttachPrimaryPalette(CSurface *sfcSurface);

	// Clipper
	bool GetPrimaryClipper(int &rX1, int &rY1, int &rX2, int &rY2);
	bool SetPrimaryClipper(int iX1, int iY1, int iX2, int iY2);
	bool SubPrimaryClipper(int iX1, int iY1, int iX2, int iY2);
	bool StorePrimaryClipper();
	bool RestorePrimaryClipper();
	bool NoPrimaryClipper();
	virtual bool UpdateClipper() = 0; // set current clipper to render target

	// Surface
	bool GetSurfaceSize(CSurface *sfcSurface, int &iWdt, int &iHgt);
	bool WipeSurface(CSurface *sfcSurface);
	void SurfaceAllowColor(CSurface *sfcSfc, uint32_t *pdwColors, int iNumColors, bool fAllowZero = false);
	void Grayscale(CSurface *sfcSfc, int32_t iOffset = 0);
	virtual bool PrepareRendering(CSurface *sfcToSurface) = 0; // check if/make rendering possible to given surface

	// Blit
	virtual void BlitLandscape(CSurface *sfcSource, CSurface *sfcSource2, CSurface *sfcLiquidAnimation, int fx, int fy,
		CSurface *sfcTarget, int tx, int ty, int wdt, int hgt);
	void Blit8Fast(CSurface8 *sfcSource, int fx, int fy,
		CSurface *sfcTarget, int tx, int ty, int wdt, int hgt);
	bool Blit(CSurface *sfcSource, float fx, float fy, float fwdt, float fhgt,
		CSurface *sfcTarget, int tx, int ty, int twdt, int thgt,
		bool fSrcColKey = false, CBltTransform *pTransform = nullptr, bool noScalingCorrection = false);
	bool Blit(CSurface *sfcSource, float fx, float fy, float fwdt, float fhgt,
		CSurface *sfcTarget, float tx, float ty, float twdt, float thgt,
		bool fSrcColKey = false, CBltTransform *pTransform = nullptr, bool noScalingCorrection = false);
	virtual void PerformBlt(CBltData &rBltData, CTexRef *pTex, uint32_t dwModClr, bool fMod2, bool fExact) = 0;
	bool Blit8(CSurface *sfcSource, int fx, int fy, int fwdt, int fhgt, // force 8bit-blit (inline)
		CSurface *sfcTarget, int tx, int ty, int twdt, int thgt,
		bool fSrcColKey = false, CBltTransform *pTransform = nullptr);
	bool BlitRotate(CSurface *sfcSource, int fx, int fy, int fwdt, int fhgt,
		CSurface *sfcTarget, int tx, int ty, int twdt, int thgt,
		int iAngle, bool fTransparency = true);
	bool BlitSurface(CSurface *sfcSurface, CSurface *sfcTarget, int tx, int ty, bool fBlitBase);
	bool BlitSurfaceTile(CSurface *sfcSurface, CSurface *sfcTarget, int iToX, int iToY, int iToWdt, int iToHgt, int iOffsetX = 0, int iOffsetY = 0, bool fSrcColKey = false, float scale = 1.f);
	bool BlitSurfaceTile2(CSurface *sfcSurface, CSurface *sfcTarget, int iToX, int iToY, int iToWdt, int iToHgt, int iOffsetX = 0, int iOffsetY = 0, bool fSrcColKey = false);
	virtual void FillBG(uint32_t dwClr = 0) = 0;

	// Text
	enum { DEFAULT_MESSAGE_COLOR = 0xffffffff };
	bool TextOut(const char *szText, CStdFont &rFont, float fZoom, CSurface *sfcDest, int iTx, int iTy, uint32_t dwFCol = 0xffffffff, uint8_t byForm = ALeft, bool fDoMarkup = true);
	bool StringOut(const char *szText, CStdFont &rFont, float fZoom, CSurface *sfcDest, int iTx, int iTy, uint32_t dwFCol = 0xffffffff, uint8_t byForm = ALeft, bool fDoMarkup = true);

	// Drawing
	virtual void DrawPix(CSurface *sfcDest, float tx, float ty, uint32_t dwCol);
	void DrawBox(CSurface *sfcDest, int iX1, int iY1, int iX2, int iY2, uint8_t byCol); // calls DrawBoxDw
	void DrawBoxDw(CSurface *sfcDest, int iX1, int iY1, int iX2, int iY2, uint32_t dwClr); // calls DrawBoxFade
	void DrawBoxFade(CSurface *sfcDest, int iX, int iY, int iWdt, int iHgt, uint32_t dwClr1, uint32_t dwClr2, uint32_t dwClr3, uint32_t dwClr4, int iBoxOffX, int iBoxOffY); // calls DrawQuadDw
	void DrawPatternedCircle(CSurface *sfcDest, int x, int y, int r, uint8_t col, CPattern &Pattern1, CPattern &Pattern2, CStdPalette &rPal);
	void DrawHorizontalLine(CSurface *sfcDest, int x1, int x2, int y, uint8_t col);
	void DrawVerticalLine(CSurface *sfcDest, int x, int y1, int y2, uint8_t col);
	void DrawFrame(CSurface *sfcDest, int x1, int y1, int x2, int y2, uint8_t col);
	void DrawFrameDw(CSurface *sfcDest, int x1, int y1, int x2, int y2, uint32_t dwClr);

	virtual void DrawLine(CSurface *sfcTarget, int x1, int y1, int x2, int y2, uint8_t byCol)
	{
		DrawLineDw(sfcTarget, (float)x1, (float)y1, (float)x2, (float)y2, Pal.GetClr(byCol));
	}

	virtual void DrawLineDw(CSurface *sfcTarget, float x1, float y1, float x2, float y2, uint32_t dwClr) = 0;
	virtual void DrawQuadDw(CSurface *sfcTarget, int *ipVtx, uint32_t dwClr1, uint32_t dwClr2, uint32_t dwClr3, uint32_t dwClr4) = 0;

	// gamma
	void SetGamma(uint32_t dwClr1, uint32_t dwClr2, uint32_t dwClr3); // set gamma ramp
	void DisableGamma(); // reset gamma ramp to default
	void EnableGamma(); // set current gamma ramp
	uint32_t ApplyGammaTo(uint32_t dwClr); // apply gamma to given color
	virtual bool ApplyGammaRamp(CGammaControl &ramp, bool fForce) = 0; // really apply gamma ramp
	virtual bool SaveDefaultGammaRamp(CStdWindow *pWindow) = 0;

	// blit states
	void ActivateBlitModulation(uint32_t dwWithClr) { BlitModulated = true; BlitModulateClr = dwWithClr; } // modulate following blits with a given color
	void DeactivateBlitModulation() { BlitModulated = false; } // stop color modulation of blits
	bool GetBlitModulation(uint32_t &rdwColor) { rdwColor = BlitModulateClr; return BlitModulated; }
	void SetBlitMode(uint32_t dwBlitMode) { this->dwBlitMode = dwBlitMode & DDrawCfg.AllowedBlitModes; } // set blit mode extra flags (additive blits, mod2-modulation, etc.)
	void ResetBlitMode() { dwBlitMode = 0; }

	void ClrByCurrentBlitMod(uint32_t &rdwClr)
	{
		// apply modulation if activated
		if (BlitModulated) ModulateClr(rdwClr, BlitModulateClr);
	}

	void SetClrModMap(CClrModAddMap *pClrModMap) { this->pClrModMap = pClrModMap; }
	void SetClrModMapEnabled(bool fToVal) { fUseClrModMap = fToVal; }
	bool GetClrModMapEnabled() const { return fUseClrModMap; }
	virtual void SetTexture() = 0;
	virtual void ResetTexture() = 0;

	// device objects
	virtual bool RestoreDeviceObjects() = 0; // init/restore device dependent objects
	virtual bool InvalidateDeviceObjects() = 0; // free device dependent objects
	virtual bool DeviceReady() = 0; // return whether device exists

protected:
	bool StringOut(const char *szText, CSurface *sfcDest, int iTx, int iTy, uint32_t dwFCol, uint8_t byForm, bool fDoMarkup, CMarkup &Markup, CStdFont *pFont, float fZoom);
	virtual void DrawPixInt(CSurface *sfcDest, float tx, float ty, uint32_t dwCol) = 0; // without ClrModMap
	bool CreatePrimaryClipper();
	virtual bool CreatePrimarySurfaces() = 0;
	bool Error(const char *szMsg);
	virtual bool CreateDirectDraw() = 0;
	bool CalculateClipper(int *iX, int *iY, int *iWdt, int *iHgt);

	void DebugLog(const char *szMsg)
	{
#ifdef _DEBUG
#ifdef C4ENGINE
		Log(szMsg);
#endif
#endif
	}

	friend class CSurface;
	friend class CTexRef;
	friend class CPattern;
};

CStdDDraw *DDrawInit(CStdApp *pApp, int Engine);
