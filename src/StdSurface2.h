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

#pragma once

#include <Standard.h>
#include <StdColors.h>

#ifndef USE_CONSOLE
#include <GL/glew.h>
#endif

#include <list>
#include <memory>

// config settings
#define C4GFXCFG_NO_ALPHA_ADD    1
#define C4GFXCFG_POINT_FILTERING 2
#define C4GFXCFG_NOADDITIVEBLTS  8
#define C4GFXCFG_NOBOXFADES      16
#define C4GFXCFG_GLCLAMP         128
#define C4GFXCFG_NOACCELERATION  512

// blitting modes
#define C4GFXBLIT_NORMAL          0 // regular blit
#define C4GFXBLIT_ADDITIVE        1 // all blits additive
#define C4GFXBLIT_MOD2            2 // additive color modulation
#define C4GFXBLIT_CLRSFC_OWNCLR   4 // do not apply global modulation to ColorByOwner-surface
#define C4GFXBLIT_CLRSFC_MOD2     8 // additive color modulation for ClrByOwner-surface

#define C4GFXBLIT_ALL            15 // bist mask covering all blit modes
#define C4GFXBLIT_NOADD          14 // bit mask covering all blit modes except additive

#define C4GFXBLIT_CUSTOM        128 // custom blitting mode - ignored by gfx system
#define C4GFXBLIT_PARENT        256 // blitting mode inherited by parent - ignored by gfx system

const int ALeft = 0, ACenter = 1, ARight = 2;

#ifndef USE_CONSOLE
class CStdGL;
extern CStdGL *pGL;
#endif

// config
class CDDrawCfg
{
public:
	bool NoAlphaAdd; // always modulate alpha values instead of assing them (->no custom modulated alpha)
	bool PointFiltering; // don't use linear filtering, because some crappy graphic cards can't handle it...
	bool PointFilteringStd; // backup value of PointFiltering
	bool AdditiveBlts; // enable additive blitting
	bool NoBoxFades; // map all DrawBoxFade-calls to DrawBoxDw
	bool GLClamp; // special texture clamping in OpenGL
	float fTexIndent; // texture indent
	float fBlitOff; // blit offsets
	uint32_t AllowedBlitModes; // bit mask for allowed blitting modes
	bool NoAcceleration; // wether direct rendering is used (X11)
	int Cfg;

	bool ColorAnimation = false; // whether to use color animation

	CDDrawCfg()
		// Let's end this silly bitmask business in the config.
	{
		Set(0, 0.01f, 0.0f);
	}

	void Set(int dwCfg, float fTexIndent, float fBlitOff) // set cfg
	{
		Cfg = dwCfg;
		NoAlphaAdd = !!(dwCfg & C4GFXCFG_NO_ALPHA_ADD);
		PointFiltering = PointFilteringStd = !!(dwCfg & C4GFXCFG_POINT_FILTERING);
		AdditiveBlts = !(dwCfg & C4GFXCFG_NOADDITIVEBLTS);
		NoBoxFades = !!(dwCfg & C4GFXCFG_NOBOXFADES);
		GLClamp = !!(dwCfg & C4GFXCFG_GLCLAMP);
		this->fTexIndent = fTexIndent;
		this->fBlitOff = fBlitOff;
		AllowedBlitModes = AdditiveBlts ? C4GFXBLIT_ALL : C4GFXBLIT_NOADD;
		NoAcceleration = !!(dwCfg & C4GFXCFG_NOACCELERATION);
	}

	void Get(int32_t &dwCfg, float &fTexIndent, float &fBlitOff)
	{
		dwCfg =
			(NoAlphaAdd ? C4GFXCFG_NO_ALPHA_ADD : 0) |
			(PointFiltering ? C4GFXCFG_POINT_FILTERING : 0) |
			(AdditiveBlts ? 0 : C4GFXCFG_NOADDITIVEBLTS) |
			(NoBoxFades ? C4GFXCFG_NOBOXFADES : 0) |
			(GLClamp ? C4GFXCFG_GLCLAMP : 0) |
			(NoAcceleration ? C4GFXCFG_NOACCELERATION : 0);
		fTexIndent = this->fTexIndent;
		fBlitOff = this->fBlitOff;
	}
};

extern CDDrawCfg DDrawCfg; // ddraw config

// class predefs
class CTexRef;
class CPattern;
class CStdDDraw;

extern CStdDDraw *lpDDraw;

class CSurface
{
private:
	CSurface(const CSurface &cpy); // do NOT copy
	CSurface &operator=(const CSurface &rCpy); // do NOT copy

public:
	CSurface();
	~CSurface();
	CSurface(uint32_t iWdt, uint32_t iHgt); // create new surface and init it

public:
	uint32_t Wdt, Hgt; // size of surface
	int PrimarySurfaceLockPitch; uint8_t *PrimarySurfaceLockBits; // lock data if primary surface is locked
	uint32_t ClipX, ClipY, ClipX2, ClipY2;
	bool fIsBackground; // background surfaces fill unused pixels with black, rather than transparency - must be set prior to loading
#ifdef _DEBUG
	int *dbg_idx;
#endif
#ifndef USE_CONSOLE
	GLenum Format; // used color format in textures
#endif
	std::vector<std::unique_ptr<CTexRef>> Textures;
	CSurface *pMainSfc; // main surface for simple ColorByOwner-surfaces
	uint32_t ClrByOwnerClr; // current color to be used for ColorByOwner-blits

	void MoveFrom(CSurface *psfcFrom); // grab data from other surface - invalidates other surface
	bool IsRenderTarget(); // surface can be used as a render target?

protected:
	int Locked;
	bool fPrimary;

	bool IsSingleSurface() { return Textures.size() == 1; } // return whether surface is not split

public:
	void SetBackground() { fIsBackground = true; }
	// Note: This uses partial locks, anything but SetPixDw and Unlock is undefined afterwards until unlock.
	void ClearBoxDw(uint32_t iX, uint32_t iY, uint32_t iWdt, uint32_t iHgt);
	bool Unlock();
	bool Lock();
	bool GetTexAt(CTexRef **ppTexRef, uint32_t &rX, uint32_t &rY); // get texture and adjust x/y
	bool GetLockTexAt(CTexRef **ppTexRef, uint32_t &rX, uint32_t &rY); // get texture; ensure it's locked and adjust x/y
	bool SetPix(int iX, int iY, uint8_t byCol); // set 8bit-px
	uint32_t GetPixDw(uint32_t iX, uint32_t iY, bool fApplyModulation, float scale = 1.0); // get 32bit-px
	bool IsPixTransparent(int iX, int iY); // is pixel's alpha value 0xff?
	bool SetPixDw(uint32_t iX, uint32_t iY, uint32_t dwCol); // set pix in surface only
	bool BltPix(uint32_t iX, uint32_t iY, CSurface *sfcSource, int iSrcX, int iSrcY, bool fTransparency); // blit pixel from source to this surface (assumes clipped coordinates!)
	bool Create(int iWdt, int iHgt, bool fOwnPal = false);
	bool CreateColorByOwner(CSurface *pBySurface); // create ColorByOwner-surface
	bool SetAsClrByOwnerOf(CSurface *pOfSurface); // assume that ColorByOwner-surface has been created, and just assign it; fails if the size doesn't match
	bool AttachSfc(void *sfcSurface);
	void Clear();
	void Default();
	void Clip(uint32_t iX, uint32_t iY, uint32_t iX2, uint32_t iY2);
	void NoClip();
	bool Read(class CStdStream &hGroup, bool fOwnPal = false);
	bool SavePNG(const char *szFilename, bool fSaveAlpha, bool fApplyGamma, bool fSaveOverlayOnly, float scale = 1.0f);
	bool Wipe(); // empty to transparent
	bool GetSurfaceSize(int &irX, int &irY); // get surface size
	void SetClr(uint32_t toClr) { ClrByOwnerClr = toClr ? toClr : 0xff; }
	uint32_t GetClr() { return ClrByOwnerClr; }

protected:
	bool CreateTextures();
	void FreeTextures();

	friend class CStdDDraw;
	friend class CPattern;
	friend class CStdGL;
};

// one texture encapsulation
class CTexRef
{
public:
	std::unique_ptr<uint32_t[]> lockData; // current lock-data
#ifndef USE_CONSOLE
	GLuint texName;
#endif
	uint32_t X, Y, Width, Height;
	bool fIntLock; // if set, texref is locked internally only
	RECT LockSize;

	CTexRef(uint32_t x, uint32_t y, uint32_t width, uint32_t height); // create texture with given size
	~CTexRef(); // release texture
	bool Lock(); // lock texture
	// Lock a part of the rect, discarding the content
	// Note: Calling Lock afterwards without an Unlock first is undefined
	bool LockForUpdate(const RECT &rtUpdate);
	void Unlock(); // unlock texture
	bool ClearRect(const RECT &rtClear); // clear rect in texture to transparent
	bool FillBlack(); // fill complete texture in black

	void SetPix(int iX, int iY, uint32_t v);
};
