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

#ifndef INC_STDDDRAW2
#define INC_STDDDRAW2

#include <StdSurface2.h>
#include <StdSurface8.h>
#include <StdFont.h>
#include <StdBuf.h>

// texref-predef
class CStdDDraw;
class CTexRef;
class CSurface;
struct CStdPalette;
class CStdGLCtx;
class CStdApp;
class CStdWindow;

// engines
#define GFXENGN_DIRECTX  0
#define GFXENGN_OPENGL   1
#define GFXENGN_DIRECTXS 2
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
	bool PatternClr(int iX, int iY, BYTE &byClr, DWORD &dwClr, CStdPalette &rPal) const; // apply pattern to color
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

// blit bounds polygon - note that blitting procedures are not designed for inner angles (>pi)
struct CBltData
{
	BYTE byNumVertices;  // number of valid vertices
	CBltVertex vtVtx[8]; // vertices for polygon - up to eight vertices may be needed
	CBltTransform TexPos; // texture mapping matrix
	CBltTransform *pTransform; // Vertex transformation

	// clip poly, so that for any point (x,y) is: (fX*x + fY*y <= fMax)
	// assumes a valid poly!
	bool ClipBy(float fX, float fY, float fMax);
};

// gamma ramp control
class CGammaControl
{
private:
	void SetClrChannel(uint16_t *pBuf, BYTE c1, BYTE c2, int c3, uint16_t *ref); // set color channel ramp

protected:
	uint16_t *red, *green, *blue; int size;

public:
	CGammaControl() : red(0), green(0), blue(0), size(0) { Default(); }
	~CGammaControl();
	void Default() { Set(0x000000, 0x808080, 0xffffff, 256, 0); } // set default ramp

	void Set(DWORD dwClr1, DWORD dwClr2, DWORD dwClr3, int size, CGammaControl *ref); // set color ramp

	DWORD ApplyTo(DWORD dwClr); // apply gamma to color value

	friend class CStdDDraw;
	friend class CStdD3D;
	friend class CStdGL;
};

// helper struct
struct FLOAT_RECT { float left, right, top, bottom; };

// direct draw encapsulation
class CStdDDraw
{
public:
	CStdDDraw() : Saturation(255) { lpDDrawPal = &Pal; }
	virtual ~CStdDDraw() { lpDDraw = NULL; }

public:
	CStdApp *pApp; // the application
	SURFACE lpPrimary; // primary and back surface (emulation...)
	SURFACE lpBack;
	CStdPalette Pal; // 8bit-pal
	bool Active; // set if device is ready to render, etc.
	CGammaControl Gamma; // gamma
	CGammaControl DefRamp; // default gamma ramp
	StdStrBuf sLastError;

protected:
	BYTE byByteCnt; // bytes per pixel (2 or 4)
	BOOL fFullscreen;
	int ClipX1, ClipY1, ClipX2, ClipY2;
	int StClipX1, StClipY1, StClipX2, StClipY2;
	bool ClipAll; // set if clipper clips everything away
	CSurface *RenderTarget; // current render target
	bool BlitModulated; // set if blits should be modulated with BlitModulateClr
	DWORD BlitModulateClr; // modulation color for blitting
	DWORD dwBlitMode; // extra flags for blit
	CClrModAddMap *pClrModMap; // map to be used for global color modulation (invalid if !fUseClrModMap)
	bool fUseClrModMap; // if set, pClrModMap will be checked for color modulations
	unsigned char Saturation; // if < 255, an extra filter is used to reduce the saturation

public:
	// General
	bool Init(CStdApp *pApp, BOOL Fullscreen, BOOL fUsePageLock, int iBitDepth, unsigned int iMonitor);
	virtual void Clear();
	virtual void Default();
	virtual CStdGLCtx *CreateContext(CStdWindow *, CStdApp *) { return NULL; }
#ifdef _WIN32
	virtual CStdGLCtx *CreateContext(HWND, CStdApp *) { return NULL; }
#endif
	virtual bool PageFlip(RECT *pSrcRt = NULL, RECT *pDstRt = NULL, CStdWindow *pWindow = NULL) = 0;
	virtual int GetEngine() = 0; // get indexed engine
	virtual void TaskOut() = 0; // user taskswitched the app away
	virtual void TaskIn() = 0; // user tasked back
	virtual bool IsOpenGL() { return false; }
	virtual bool OnResolutionChanged() = 0; // reinit window for new resolution
	const char *GetLastError() { return sLastError.getData(); }

	// Palette
	BOOL SetPrimaryPalette(BYTE *pBuf, BYTE *pAlphaBuf = NULL);
	BOOL AttachPrimaryPalette(SURFACE sfcSurface);

	// Clipper
	BOOL GetPrimaryClipper(int &rX1, int &rY1, int &rX2, int &rY2);
	BOOL SetPrimaryClipper(int iX1, int iY1, int iX2, int iY2);
	BOOL SubPrimaryClipper(int iX1, int iY1, int iX2, int iY2);
	BOOL StorePrimaryClipper();
	BOOL RestorePrimaryClipper();
	BOOL NoPrimaryClipper();
	virtual bool UpdateClipper() = 0; // set current clipper to render target
	bool ClipPoly(CBltData &rBltData); // clip polygon to clipper; return whether completely clipped out

	// Surface
	BOOL GetSurfaceSize(SURFACE sfcSurface, int &iWdt, int &iHgt);
	BOOL WipeSurface(SURFACE sfcSurface);
	void SurfaceAllowColor(SURFACE sfcSfc, DWORD *pdwColors, int iNumColors, BOOL fAllowZero = FALSE);
	void Grayscale(SURFACE sfcSfc, int32_t iOffset = 0);
	virtual bool PrepareRendering(SURFACE sfcToSurface) = 0; // check if/make rendering possible to given surface

	// Blit
	virtual void BlitLandscape(SURFACE sfcSource, SURFACE sfcSource2, SURFACE sfcLiquidAnimation, int fx, int fy,
		SURFACE sfcTarget, int tx, int ty, int wdt, int hgt);
	void Blit8Fast(CSurface8 *sfcSource, int fx, int fy,
		SURFACE sfcTarget, int tx, int ty, int wdt, int hgt);
	BOOL Blit(SURFACE sfcSource, float fx, float fy, float fwdt, float fhgt,
		SURFACE sfcTarget, int tx, int ty, int twdt, int thgt,
		BOOL fSrcColKey = FALSE, CBltTransform *pTransform = NULL);
	virtual void PerformBlt(CBltData &rBltData, CTexRef *pTex, DWORD dwModClr, bool fMod2, bool fExact) = 0;
	BOOL Blit8(SURFACE sfcSource, int fx, int fy, int fwdt, int fhgt, // force 8bit-blit (inline)
		SURFACE sfcTarget, int tx, int ty, int twdt, int thgt,
		BOOL fSrcColKey = FALSE, CBltTransform *pTransform = NULL);
	BOOL BlitRotate(SURFACE sfcSource, int fx, int fy, int fwdt, int fhgt,
		SURFACE sfcTarget, int tx, int ty, int twdt, int thgt,
		int iAngle, bool fTransparency = true);
	BOOL BlitSurface(SURFACE sfcSurface, SURFACE sfcTarget, int tx, int ty, bool fBlitBase);
	BOOL BlitSurfaceTile(SURFACE sfcSurface, SURFACE sfcTarget, int iToX, int iToY, int iToWdt, int iToHgt, int iOffsetX = 0, int iOffsetY = 0, BOOL fSrcColKey = FALSE);
	BOOL BlitSurfaceTile2(SURFACE sfcSurface, SURFACE sfcTarget, int iToX, int iToY, int iToWdt, int iToHgt, int iOffsetX = 0, int iOffsetY = 0, BOOL fSrcColKey = FALSE);
	virtual void FillBG(DWORD dwClr = 0) = 0;

	// Text
	enum { DEFAULT_MESSAGE_COLOR = 0xffffffff };
	BOOL TextOut(const char *szText, CStdFont &rFont, float fZoom, SURFACE sfcDest, int iTx, int iTy, DWORD dwFCol = 0xffffffff, BYTE byForm = ALeft, bool fDoMarkup = true);
	BOOL StringOut(const char *szText, CStdFont &rFont, float fZoom, SURFACE sfcDest, int iTx, int iTy, DWORD dwFCol = 0xffffffff, BYTE byForm = ALeft, bool fDoMarkup = true);

	// Drawing
	virtual void DrawPix(SURFACE sfcDest, float tx, float ty, DWORD dwCol);
	void DrawBox(SURFACE sfcDest, int iX1, int iY1, int iX2, int iY2, BYTE byCol); // calls DrawBoxDw
	void DrawBoxDw(SURFACE sfcDest, int iX1, int iY1, int iX2, int iY2, DWORD dwClr); // calls DrawBoxFade
	void DrawBoxFade(SURFACE sfcDest, int iX, int iY, int iWdt, int iHgt, DWORD dwClr1, DWORD dwClr2, DWORD dwClr3, DWORD dwClr4, int iBoxOffX, int iBoxOffY); // calls DrawQuadDw
	void DrawPatternedCircle(SURFACE sfcDest, int x, int y, int r, BYTE col, CPattern &Pattern1, CPattern &Pattern2, CStdPalette &rPal);
	void DrawHorizontalLine(SURFACE sfcDest, int x1, int x2, int y, BYTE col);
	void DrawVerticalLine(SURFACE sfcDest, int x, int y1, int y2, BYTE col);
	void DrawFrame(SURFACE sfcDest, int x1, int y1, int x2, int y2, BYTE col);
	void DrawFrameDw(SURFACE sfcDest, int x1, int y1, int x2, int y2, DWORD dwClr);

	virtual void DrawLine(SURFACE sfcTarget, int x1, int y1, int x2, int y2, BYTE byCol)
	{
		DrawLineDw(sfcTarget, (float)x1, (float)y1, (float)x2, (float)y2, Pal.GetClr(byCol));
	}

	virtual void DrawLineDw(SURFACE sfcTarget, float x1, float y1, float x2, float y2, DWORD dwClr) = 0;
	virtual void DrawQuadDw(SURFACE sfcTarget, int *ipVtx, DWORD dwClr1, DWORD dwClr2, DWORD dwClr3, DWORD dwClr4) = 0;

	// gamma
	void SetGamma(DWORD dwClr1, DWORD dwClr2, DWORD dwClr3); // set gamma ramp
	void DisableGamma(); // reset gamma ramp to default
	void EnableGamma(); // set current gamma ramp
	DWORD ApplyGammaTo(DWORD dwClr); // apply gamma to given color
	virtual bool ApplyGammaRamp(CGammaControl &ramp, bool fForce) = 0; // really apply gamma ramp
	virtual bool SaveDefaultGammaRamp(CStdWindow *pWindow) = 0;

	// blit states
	void ActivateBlitModulation(DWORD dwWithClr) { BlitModulated = true; BlitModulateClr = dwWithClr; } // modulate following blits with a given color
	void DeactivateBlitModulation() { BlitModulated = false; } // stop color modulation of blits
	bool GetBlitModulation(DWORD &rdwColor) { rdwColor = BlitModulateClr; return BlitModulated; }
	void SetBlitMode(DWORD dwBlitMode) { this->dwBlitMode = dwBlitMode & DDrawCfg.AllowedBlitModes; } // set blit mode extra flags (additive blits, mod2-modulation, etc.)
	void ResetBlitMode() { dwBlitMode = 0; }

	void ClrByCurrentBlitMod(DWORD &rdwClr)
	{
		// apply modulation if activated
		if (BlitModulated) ModulateClr(rdwClr, BlitModulateClr);
	}

	void SetClrModMap(CClrModAddMap *pClrModMap) { this->pClrModMap = pClrModMap; }
	void SetClrModMapEnabled(bool fToVal) { fUseClrModMap = fToVal; }
	bool GetClrModMapEnabled() const { return fUseClrModMap; }
	virtual bool StoreStateBlock() = 0;
	virtual void SetTexture() = 0;
	virtual void ResetTexture() = 0;
	virtual bool RestoreStateBlock() = 0;

	// device objects
	virtual bool InitDeviceObjects() = 0; // init device dependant objects
	virtual bool RestoreDeviceObjects() = 0; // restore device dependant objects
	virtual bool InvalidateDeviceObjects() = 0; // free device dependant objects
	virtual bool DeleteDeviceObjects() = 0; // free device dependant objects
	virtual bool DeviceReady() = 0; // return whether device exists

	int GetByteCnt() { return byByteCnt; } // return bytes per pixel

protected:
	bool StringOut(const char *szText, SURFACE sfcDest, int iTx, int iTy, DWORD dwFCol, BYTE byForm, bool fDoMarkup, CMarkup &Markup, CStdFont *pFont, float fZoom);
	virtual void DrawPixInt(SURFACE sfcDest, float tx, float ty, DWORD dwCol) = 0; // without ClrModMap
	BOOL CreatePrimaryClipper();
	virtual bool CreatePrimarySurfaces(BOOL Fullscreen, int iColorDepth, unsigned int iMonitor) = 0;
	virtual bool SetOutputAdapter(unsigned int iMonitor) = 0;
	bool Error(const char *szMsg);
	virtual BOOL CreateDirectDraw() = 0;

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

bool LockSurfaceGlobal(SURFACE sfcTarget);
bool UnLockSurfaceGlobal(SURFACE sfcTarget);
bool DLineSPix(int32_t x, int32_t y, int32_t col);
bool DLineSPixDw(int32_t x, int32_t y, int32_t dwClr);

CStdDDraw *DDrawInit(CStdApp *pApp, BOOL Fullscreen, BOOL fUsePageLock, int iBitDepth, int Engine, unsigned int iMonitor);

#endif // INC_STDDDRAW2
