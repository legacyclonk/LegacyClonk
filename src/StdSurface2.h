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
class CTexMgr;
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
	CSurface(int iWdt, int iHgt); // create new surface and init it

public:
	int Wdt, Hgt; // size of surface
	int PrimarySurfaceLockPitch; uint8_t *PrimarySurfaceLockBits; // lock data if primary surface is locked
	int iTexSize; // size of textures
	int iTexX, iTexY; // number of textures in x/y-direction
	int ClipX, ClipY, ClipX2, ClipY2;
	bool fIsRenderTarget; // set for surfaces to be used as offscreen render targets
	bool fIsBackground; // background surfaces fill unused pixels with black, rather than transparency - must be set prior to loading
#ifdef _DEBUG
	int *dbg_idx;
#endif
#ifndef USE_CONSOLE
	GLenum Format; // used color format in textures
#endif
	CTexRef **ppTex; // textures
	CSurface *pMainSfc; // main surface for simple ColorByOwner-surfaces
	uint32_t ClrByOwnerClr; // current color to be used for ColorByOwner-blits

	void MoveFrom(CSurface *psfcFrom); // grab data from other surface - invalidates other surface
	bool IsRenderTarget(); // surface can be used as a render target?

protected:
	int Locked;
	bool fPrimary;

	bool IsSingleSurface() { return iTexX * iTexY == 1; } // return whether surface is not split

public:
	void SetBackground() { fIsBackground = true; }
	// Note: This uses partial locks, anything but SetPixDw and Unlock is undefined afterwards until unlock.
	void ClearBoxDw(int iX, int iY, int iWdt, int iHgt);
	bool Unlock();
	bool Lock();
	bool GetTexAt(CTexRef **ppTexRef, int &rX, int &rY); // get texture and adjust x/y
	bool GetLockTexAt(CTexRef **ppTexRef, int &rX, int &rY); // get texture; ensure it's locked and adjust x/y
	bool SetPix(int iX, int iY, uint8_t byCol); // set 8bit-px
	uint32_t GetPixDw(int iX, int iY, bool fApplyModulation, float scale = 1.0); // get 32bit-px
	bool IsPixTransparent(int iX, int iY); // is pixel's alpha value 0xff?
	bool SetPixDw(int iX, int iY, uint32_t dwCol); // set pix in surface only
	bool BltPix(int iX, int iY, CSurface *sfcSource, int iSrcX, int iSrcY, bool fTransparency); // blit pixel from source to this surface (assumes clipped coordinates!)
	bool Create(int iWdt, int iHgt, bool fOwnPal = false, bool fIsRenderTarget = false);
	bool CreateColorByOwner(CSurface *pBySurface); // create ColorByOwner-surface
	bool SetAsClrByOwnerOf(CSurface *pOfSurface); // assume that ColorByOwner-surface has been created, and just assign it; fails if the size doesn't match
	bool AttachSfc(void *sfcSurface);
	void Clear();
	void Default();
	void Clip(int iX, int iY, int iX2, int iY2);
	void NoClip();
	bool Read(class CStdStream &hGroup, bool fOwnPal = false);
	bool SavePNG(const char *szFilename, bool fSaveAlpha, bool fApplyGamma, bool fSaveOverlayOnly, float scale = 1.0f);
	bool Wipe(); // empty to transparent
	bool GetSurfaceSize(int &irX, int &irY); // get surface size
	void SetClr(uint32_t toClr) { ClrByOwnerClr = toClr ? toClr : 0xff; }
	uint32_t GetClr() { return ClrByOwnerClr; }
	bool CopyBytes(uint8_t *pImageData); // assumes an array of wdt*hgt*4 and copies data directly from it

protected:
	bool CreateTextures(); // create ppTex-array
	void FreeTextures(); // free ppTex-array if existent

	friend class CStdDDraw;
	friend class CPattern;
	friend class CStdGL;
};

typedef struct _D3DLOCKED_RECT
{
	int Pitch;
	unsigned char *pBits;
} D3DLOCKED_RECT;

// one texture encapsulation
class CTexRef
{
public:
	D3DLOCKED_RECT texLock; // current lock-data
#ifndef USE_CONSOLE
	GLuint texName;
#endif
	int iSize;
	bool fIntLock; // if set, texref is locked internally only
	RECT LockSize;

	CTexRef(int iSize, bool fAsRenderTarget); // create texture with given size
	~CTexRef(); // release texture
	bool Lock(); // lock texture
	// Lock a part of the rect, discarding the content
	// Note: Calling Lock afterwards without an Unlock first is undefined
	bool LockForUpdate(RECT &rtUpdate);
	void Unlock(); // unlock texture
	bool ClearRect(RECT &rtClear); // clear rect in texture to transparent
	bool FillBlack(); // fill complete texture in black

	void SetPix(int iX, int iY, uint32_t v)
	{
		*((uint32_t *)(((uint8_t *)texLock.pBits) + (iY - LockSize.top) * texLock.Pitch + (iX - LockSize.left) * 4)) = v;
	}
};

// texture management
class CTexMgr
{
public:
	std::list<CTexRef *> Textures;

public:
	CTexMgr();
	~CTexMgr();

	void RegTex(CTexRef *pTex);
	void UnregTex(CTexRef *pTex);

	void IntLock(); // do an internal lock
	void IntUnlock(); // undo internal lock
};

extern CTexMgr *pTexMgr;
