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

#ifdef USE_GL

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

void glColorDw(uint32_t dwClr);

// one OpenGL context
class CStdGLCtx
{
public:
	CStdGLCtx();
	~CStdGLCtx() { Clear(); };

	void Clear(); // clear objects
#ifdef _WIN32
	bool Init(CStdWindow *pWindow, CStdApp *pApp, HWND hWindow = nullptr);
#else
	bool Init(CStdWindow *pWindow, CStdApp *pApp);
#endif

	bool Select(bool verbose = false); // select this context
	void Deselect(); // select this context
	bool UpdateSize(); // get new size from hWnd

	bool PageFlip(); // present scene

protected:
	// this handles are declared as pointers to structs
	CStdWindow *pWindow; // window to draw in
#ifdef _WIN32
	HGLRC hrc; // rendering context
	HWND hWindow; // used if pWindow==nullptr
	HDC hDC; // device context handle
#elif defined(USE_X11)
	GLXContext ctx;
#endif
	int cx, cy; // context window size

	friend class CStdGL;
	friend class CSurface;
};

// OpenGL encapsulation
class CStdGL : public CStdDDraw
{
public:
	CStdGL();
	~CStdGL();
	virtual bool PageFlip(RECT *pSrcRt = nullptr, RECT *pDstRt = nullptr, CStdWindow *pWindow = nullptr);

protected:
	int iPixelFormat; // used pixel format

	GLenum sfcFmt; // texture surface format
	CStdGLCtx MainCtx; // main GL context
	CStdGLCtx *pCurrCtx; // current context (owned if fullscreen)
	bool fFullscreen; // fullscreen mode?
	int iClrDpt; // color depth
	// continously numbered shaders for ATI cards
	unsigned int shader;
	// shaders for the ARB extension
	GLuint shaders[6];

public:
	// General
	void Clear();
	void Default();
	virtual int GetEngine() { return 1; } // get indexed engine
	void TaskOut(); // user taskswitched the app away
	void TaskIn(); // user tasked back
	virtual bool IsOpenGL() { return true; }
	virtual bool OnResolutionChanged(); // reinit OpenGL and window for new resolution

	// Clipper
	bool UpdateClipper(); // set current clipper to render target

	// Surface
	bool PrepareRendering(SURFACE sfcToSurface); // check if/make rendering possible to given surface
	CStdGLCtx &GetMainCtx() { return MainCtx; }
	virtual CStdGLCtx *CreateContext(CStdWindow *pWindow, CStdApp *pApp);
#ifdef _WIN32
	virtual CStdGLCtx *CreateContext(HWND hWindow, CStdApp *pApp);
#endif

	// Blit
	void PerformBlt(CBltData &rBltData, CTexRef *pTex, uint32_t dwModClr, bool fMod2, bool fExact);
	virtual void BlitLandscape(SURFACE sfcSource, SURFACE sfcSource2, SURFACE sfcLiquidAnimation, int fx, int fy,
		SURFACE sfcTarget, int tx, int ty, int wdt, int hgt);
	void FillBG(uint32_t dwClr = 0);

	// Drawing
	void DrawQuadDw(SURFACE sfcTarget, int *ipVtx, uint32_t dwClr1, uint32_t dwClr2, uint32_t dwClr3, uint32_t dwClr4);
	void DrawLineDw(SURFACE sfcTarget, float x1, float y1, float x2, float y2, uint32_t dwClr);
	void DrawPixInt(SURFACE sfcDest, float tx, float ty, uint32_t dwCol);

	// Gamma
	virtual bool ApplyGammaRamp(CGammaControl &ramp, bool fForce);
	virtual bool SaveDefaultGammaRamp(CStdWindow *pWindow);

	// device objects
	bool InitDeviceObjects(); // init device dependent objects
	bool RestoreDeviceObjects(); // restore device dependent objects
	bool InvalidateDeviceObjects(); // free device dependent objects
	bool DeleteDeviceObjects(); // free device dependent objects
	bool StoreStateBlock();
	void SetTexture();
	void ResetTexture();
	bool RestoreStateBlock();
#ifdef _WIN32
	bool DeviceReady() { return !!MainCtx.hrc; }
#elif defined(USE_X11)
	bool DeviceReady() { return !!MainCtx.ctx; }
#else
	bool DeviceReady() { return true; } // SDL
#endif

protected:
	bool CreatePrimarySurfaces(bool Fullscreen, int iColorDepth, unsigned int iMonitor);
	bool CreateDirectDraw();
	virtual bool SetOutputAdapter(unsigned int iMonitor);

#ifdef USE_X11
	// Size of gamma ramps
	int gammasize;
#endif

	friend class CSurface;
	friend class CTexRef;
	friend class CPattern;
	friend class CStdGLCtx;
	friend class C4StartupOptionsDlg;
	friend class C4FullScreen;
};

// Global access pointer
extern CStdGL *pGL;

#endif // ifdef USE_GL
