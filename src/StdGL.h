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

/* OpenGL implementation of NewGfx */

#pragma once

#ifndef USE_CONSOLE

#include <GL/glew.h>

#ifdef USE_X11
// Xmd.h typedefs bool to CARD8, but we want int
#define bool _BOOL
#include <X11/Xmd.h>
#undef bool
#include <GL/glx.h>
#endif

#if defined(__APPLE__)
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif
#include <StdDDraw2.h>

class CStdWindow;

// one OpenGL context
class CStdGLCtx
{
public:
	CStdGLCtx();
	~CStdGLCtx() { Clear(); }

	void Clear(); // clear objects
#ifdef _WIN32
	bool Init(CStdWindow *pWindow, CStdApp *pApp, HWND hWindow = nullptr);
#else
	bool Init(CStdWindow *pWindow, CStdApp *pApp);
#endif

	bool Select(bool verbose = false, bool selectOnly = false); // select this context
	void Deselect(bool secondary = false); // select this context
	bool UpdateSize(); // get new size from hWnd

	bool PageFlip(); // present scene
	void Finish();

protected:
	void DoDeselect();
	// this handles are declared as pointers to structs
	CStdWindow *pWindow; // window to draw in
#ifdef _WIN32
	HGLRC hrc; // rendering context
	HWND hWindow; // used if pWindow==nullptr
	HDC hDC; // device context handle
#elif defined(USE_X11)
	GLXContext ctx;
#elif defined(USE_SDL_MAINLOOP)
	/*SDL_GLContext*/ void *ctx;
#endif
	int cx, cy; // context window size

	friend class CStdGL;
	friend class C4Surface;
};

// OpenGL encapsulation
class CStdGL : public CStdDDraw
{
public:
	CStdGL();
	~CStdGL();
	virtual bool PageFlip(RECT *pSrcRt = nullptr, RECT *pDstRt = nullptr, CStdWindow *pWindow = nullptr) override;

protected:
	GLenum sfcFmt; // texture surface format
	CStdGLCtx MainCtx; // main GL context
	CStdGLCtx *pCurrCtx; // current context
	// continously numbered shaders for ATI cards
	unsigned int shader;
	// shaders for the ARB extension
	GLuint shaders[6];

public:
	// General
	void Clear() override;
	void Default() override;
	virtual int GetEngine() override { return 1; } // get indexed engine
	virtual bool OnResolutionChanged() override; // reinit OpenGL and window for new resolution

	// Clipper
	bool UpdateClipper() override; // set current clipper to render target

	// Surface
	bool PrepareRendering(C4Surface *sfcToSurface) override; // check if/make rendering possible to given surface
	CStdGLCtx &GetMainCtx() { return MainCtx; }
	virtual CStdGLCtx *CreateContext(CStdWindow *pWindow, CStdApp *pApp) override;
#ifdef _WIN32
	virtual CStdGLCtx *CreateContext(HWND hWindow, CStdApp *pApp) override;
#endif

	// Blit
	void PerformBlt(CBltData &rBltData, C4TexRef *pTex, uint32_t dwModClr, bool fMod2, bool fExact) override;
	virtual void BlitLandscape(C4Surface *sfcSource, C4Surface *sfcSource2, C4Surface *sfcLiquidAnimation, int fx, int fy,
		C4Surface *sfcTarget, int tx, int ty, int wdt, int hgt) override;
	void FillBG(uint32_t dwClr = 0) override;

	// Drawing
	void DrawQuadDw(C4Surface *sfcTarget, int *ipVtx, uint32_t dwClr1, uint32_t dwClr2, uint32_t dwClr3, uint32_t dwClr4) override;
	void DrawLineDw(C4Surface *sfcTarget, float x1, float y1, float x2, float y2, uint32_t dwClr) override;
	void DrawPixInt(C4Surface *sfcDest, float tx, float ty, uint32_t dwCol) override;

	// Gamma
	virtual bool ApplyGammaRamp(CGammaControl &ramp, bool fForce) override;
	virtual bool SaveDefaultGammaRamp(CStdWindow *pWindow) override;

	// device objects
	bool RestoreDeviceObjects() override; // init/restore device dependent objects
	bool InvalidateDeviceObjects() override; // free device dependent objects
	void SetTexture() override;
	void ResetTexture() override;
#ifdef _WIN32
	bool DeviceReady() override { return !!MainCtx.hrc; }
#elif defined(USE_X11)
	bool DeviceReady() override { return !!MainCtx.ctx; }
#else
	bool DeviceReady() override { return true; } // SDL
#endif

protected:
	bool CreatePrimarySurfaces() override;
	bool CreateDirectDraw() override;

#ifdef USE_X11
	// Size of gamma ramps
	int gammasize{};
#endif

	friend class C4Surface;
	friend class C4TexRef;
	friend class CPattern;
	friend class CStdGLCtx;
	friend class C4StartupOptionsDlg;
	friend class C4FullScreen;
};

// Global access pointer
extern CStdGL *pGL;

#endif
