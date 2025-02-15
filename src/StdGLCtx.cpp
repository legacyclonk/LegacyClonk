/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
 * Copyright (c) 2017-2021, The LegacyClonk Team and contributors
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

/* glx conversion by Guenther, 2005 */

/* OpenGL implementation of NewGfx, the context */

#include <Standard.h>
#include "StdApp.h"
#include <StdGL.h>
#include <C4Log.h>
#include "C4Rect.h"
#include <C4Surface.h>
#include <StdWindow.h>

#ifndef _WIN32
#include "C4Config.h"
#endif

#ifndef USE_CONSOLE

void CStdGLCtx::Deselect(bool secondary)
{
	if (pGL && pGL->pCurrCtx == this)
	{
		DoDeselect();
		pGL->pCurrCtx = nullptr;
	}
	else if (secondary)
	{
		DoDeselect();
	}
}

void CStdGLCtx::Finish()
{
	glFinish();
}

#ifdef _WIN32

CStdGLCtx::CStdGLCtx() : hrc(nullptr), pWindow(nullptr), hDC(nullptr), cx(0), cy(0) {}

void CStdGLCtx::Clear()
{
	if (hrc)
	{
		Deselect();
		wglDeleteContext(hrc); hrc = nullptr;
	}
	if (hDC)
	{
		ReleaseDC(pWindow ? pWindow->GetRenderWindow() : hWindow, hDC);
		hDC = nullptr;
	}
	pWindow = 0; cx = cy = 0; hWindow = nullptr;
}

bool CStdGLCtx::Init(CStdWindow *pWindow, CStdApp *pApp, HWND hWindow)
{
	// safety
	if (!pGL) return false;

	// store window
	this->pWindow = pWindow;
	// default HWND
	if (pWindow) hWindow = pWindow->GetRenderWindow(); else this->hWindow = hWindow;

	// get DC
	hDC = GetDC(hWindow);
	if (!hDC)
	{
		pGL->logger->error("Error getting DC");
		return false;
	}

	// pixel format
	PIXELFORMATDESCRIPTOR pfd{};
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 0;
	pfd.iLayerType = PFD_MAIN_PLANE;
	const auto pixelFormat = ChoosePixelFormat(hDC, &pfd);
	if (pixelFormat == 0)
	{
		pGL->logger->error("Error getting pixel format");
		return false;
	}

	if (!SetPixelFormat(hDC, pixelFormat, &pfd))
	{
		pGL->logger->error("Error setting pixel format");
		return false;
	}

	// create context
	hrc = wglCreateContext(hDC);
	if (!hrc)
	{
		pGL->logger->error("Error creating context");
		return false;
	}

	// share textures
	if (this != &pGL->MainCtx)
	{
		if (!wglShareLists(pGL->MainCtx.hrc, hrc))
		{
			pGL->logger->error("Textures for secondary context not available");
			return false;
		}
		return true;
	}

	// select
	if (!Select())
	{
		pGL->logger->error("Unable to select context");
		return false;
	}

	// init extensions
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		// Problem: glewInit failed, something is seriously wrong.
		pGL->logger->error(reinterpret_cast<const char *>(glewGetErrorString(err)));
	}
	// success
	return true;
}

bool CStdGLCtx::Select(bool verbose, bool selectOnly)
{
	// safety
	if (!pGL || !hrc) return false; if (!pGL->lpPrimary) return false;
	// make context current
	if (!wglMakeCurrent(hDC, hrc)) return false;

	if (!selectOnly)
	{
		pGL->pCurrCtx = this;
		// update size
		UpdateSize();
		// assign size
		pGL->lpPrimary->Wdt = cx; pGL->lpPrimary->Hgt = cy;
		// set some default states
		glDisable(GL_DEPTH_TEST);
		glShadeModel(GL_FLAT);
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		// update clipper - might have been done by UpdateSize
		// however, the wrong size might have been assumed
		if (!pGL->UpdateClipper()) return false;
	}
	// success
	return true;
}

void CStdGLCtx::DoDeselect()
{
	wglMakeCurrent(nullptr, nullptr);
}

bool CStdGLCtx::UpdateSize()
{
	// safety
	if (!pWindow && !hWindow) return false;
	// get size
	RECT rt; if (!GetClientRect(pWindow ? pWindow->GetRenderWindow() : hWindow, &rt)) return false;
	const auto scale = pGL->pApp->GetScale();
	int cx2 = static_cast<int32_t>(ceilf((rt.right - rt.left) / scale)), cy2 = static_cast<int32_t>(ceilf((rt.bottom - rt.top) / scale));
	// assign if different
	if (cx != cx2 || cy != cy2)
	{
		cx = cx2; cy = cy2;
		if (pGL) pGL->UpdateClipper();
	}
	// success
	return true;
}

bool CStdGLCtx::PageFlip()
{
	// flush GL buffer
	glFlush();
	SwapBuffers(hDC);
	return true;
}

bool CStdGL::SaveDefaultGammaRampToMonitor(CStdWindow *pWindow)
{
	HDC hDC = GetDC(pWindow->GetRenderWindow());
	if (hDC)
	{
		if (!GetDeviceGammaRamp(hDC, DefRamp.red))
		{
			DefRamp.Default();
			logger->error("Error getting default gamma ramp; using standard");
		}
		ReleaseDC(pWindow->GetRenderWindow(), hDC);
		return true;
	}
	return false;
}

bool CStdGL::ApplyGammaRampToMonitor(CGammaControl &ramp, bool fForce)
{
	if (!MainCtx.hDC || (!Active && !fForce)) return false;
	SetDeviceGammaRamp(MainCtx.hDC, ramp.red);
	return true;
}

#elif defined(USE_X11)

#include <GL/glx.h>
#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>

CStdGLCtx::CStdGLCtx() : pWindow(nullptr), ctx(nullptr), cx(0), cy(0) {}

void CStdGLCtx::Clear()
{
	Deselect();
	if (ctx)
	{
		glXDestroyContext(pWindow->dpy, ctx);
		ctx = nullptr;
	}
	pWindow = nullptr;
	cx = cy = 0;
}

bool CStdGLCtx::Init(CStdWindow *pWindow, CStdApp *)
{
	// safety
	if (!pGL) return false;
	// store window
	this->pWindow = pWindow;
	// Create Context with sharing (if this is the main context, our ctx will be 0, so no sharing)
	// try direct rendering first
	if (!Config.Graphics.NoAcceleration)
		ctx = glXCreateContext(pWindow->dpy, reinterpret_cast<XVisualInfo *>(pWindow->Info), pGL->MainCtx.ctx, True);
	// without, rendering will be unacceptable slow, but that's better than nothing at all
	if (!ctx)
		ctx = glXCreateContext(pWindow->dpy, reinterpret_cast<XVisualInfo *>(pWindow->Info), pGL->MainCtx.ctx, False);
	// No luck at all?
	if (!ctx)
	{
		pGL->logger->error("Unable to create context");
		return false;
	}

	if (this == &pGL->MainCtx)
	{
		if (!Select(true))
		{
			pGL->logger->error("Unable to select context");
			return false;
		}
		// init extensions
		GLenum err = glewInit();
		if (GLEW_OK != err)
		{
			// Problem: glewInit failed, something is seriously wrong.
			pGL->logger->error(reinterpret_cast<const char *>(glewGetErrorString(err)));
		}
	}
	return true;
}

bool CStdGLCtx::Select(bool verbose, bool selectOnly)
{
	// safety
	if (!pGL || !ctx)
	{
		return false;
	}
	if (!pGL->lpPrimary)
	{
		if (verbose) pGL->logger->error("lpPrimary is zero");
		return false;
	}
	// make context current
	if (!pWindow->renderwnd || !glXMakeCurrent(pWindow->dpy, pWindow->renderwnd, ctx))
	{
		if (verbose) pGL->logger->error("glXMakeCurrent failed");
		return false;
	}

	if (!selectOnly)
	{
		pGL->pCurrCtx = this;
		// update size FIXME: Don't call this every frame
		UpdateSize();
		// assign size
		pGL->lpPrimary->Wdt = cx; pGL->lpPrimary->Hgt = cy;
		// set some default states
		glDisable(GL_DEPTH_TEST);
		glShadeModel(GL_FLAT);
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		// update clipper - might have been done by UpdateSize
		// however, the wrong size might have been assumed
		if (!pGL->UpdateClipper())
		{
			if (verbose) pGL->logger->error("UpdateClipper failed");
			return false;
		}
	}
	// success
	return true;
}

void CStdGLCtx::DoDeselect()
{
	glXMakeCurrent(pWindow->dpy, None, nullptr);
}

bool CStdGLCtx::UpdateSize()
{
	// safety
	if (!pWindow) return false;
	// get size
	Window winDummy;
	unsigned int borderDummy;
	int x, y;
	unsigned int width, height;
	unsigned int depth;
	XGetGeometry(pWindow->dpy, pWindow->renderwnd, &winDummy, &x, &y,
		&width, &height, &borderDummy, &depth);
	// assign if different
	const auto scale = pGL->pApp->GetScale();
	auto newWidth = static_cast<int32_t>(ceilf(width / scale));
	auto newHeight = static_cast<int32_t>(ceilf(height / scale));
	if (cx != newWidth || cy != newHeight)
	{
		cx = newWidth; cy = newHeight;
		if (pGL) pGL->UpdateClipper();
	}
	// success
	return true;
}

bool CStdGLCtx::PageFlip()
{
	// flush GL buffer
	glFlush();
	if (!pWindow || !pWindow->renderwnd) return false;
	glXSwapBuffers(pWindow->dpy, pWindow->renderwnd);
	return true;
}

bool CStdGL::ApplyGammaRampToMonitor(CGammaControl &ramp, bool fForce)
{
	if (!DeviceReady() || (!Active && !fForce)) return false;
	if (pApp->xf86vmode_major_version < 2) return false;
	if (gammasize != ramp.size || gammasize == 0) return false;
	return XF86VidModeSetGammaRamp(pApp->dpy, DefaultScreen(pApp->dpy), ramp.size,
		ramp.red, ramp.green, ramp.blue);
}

bool CStdGL::SaveDefaultGammaRampToMonitor(CStdWindow *pWindow)
{
	if (pApp->xf86vmode_major_version < 2) return false;
	// Get the Display
	Display *const dpy = pWindow->dpy;
	XF86VidModeGetGammaRampSize(dpy, DefaultScreen(dpy), &gammasize);

	if (gammasize != 0)
	{
		if (gammasize != 256)
		{
			DefRamp.Set(0x000000, 0x808080, 0xffffff, gammasize, nullptr);
			logger->warn("Size of GammaRamp is {}, not 256", gammasize);
		}

		// store default gamma
		if (!XF86VidModeGetGammaRamp(pWindow->dpy, DefaultScreen(pWindow->dpy), DefRamp.size,
			DefRamp.red, DefRamp.green, DefRamp.blue))
		{
			DefRamp.Default();
			logger->error("Error getting default gamma ramp; using standard");
		}
	}
	else
	{
		DefRamp.Default();
		logger->warn("Size of GammaRamp is 0, using default ramp");
	}

	Gamma.Set(0x000000, 0x808080, 0xffffff, DefRamp.GetSize(), &DefRamp);
	return true;
}

#elif defined(USE_SDL_MAINLOOP)

#include <stdexcept>
#include <string>

CStdGLCtx::CStdGLCtx() : pWindow{}, cx{}, cy{}, ctx{} {}

void CStdGLCtx::Clear()
{
	Deselect();
	if (ctx)
	{
		SDL_GL_DeleteContext(ctx);
		ctx = nullptr;
	}
	pWindow = nullptr;
	cx = cy = 0;
}

bool CStdGLCtx::Init(CStdWindow *pWindow, CStdApp *)
{
	// safety
	if (!pGL) return false;
	ctx = SDL_GL_CreateContext(pWindow->sdlWindow);
	if (!ctx)
	{
		pGL->logger->error("Unable to create context: {}", SDL_GetError());
		return false;
	}
	// store window
	this->pWindow = pWindow;
	assert(!Config.Graphics.NoAcceleration);

	if (this == &pGL->MainCtx)
	{
		// No luck at all?
		if (!Select(true))
		{
			pGL->logger->error("Unable to select context");
			return false;
		}
		// init extensions
		GLenum err = glewInit();
		if (GLEW_OK != err)
		{
			// Problem: glewInit failed, something is seriously wrong.
			pGL->logger->error(reinterpret_cast<const char *>(glewGetErrorString(err)));
		}
	}
	return true;
}

bool CStdGLCtx::Select(bool verbose, bool selectOnly)
{
	SDL_GL_MakeCurrent(this->pWindow->sdlWindow, ctx);
	if (!selectOnly)
	{
		pGL->pCurrCtx = this;
		// update size FIXME: Don't call this every frame
		UpdateSize();
		// assign size
		pGL->lpPrimary->Wdt = cx; pGL->lpPrimary->Hgt = cy;
		// set some default states
		glDisable(GL_DEPTH_TEST);
		glShadeModel(GL_FLAT);
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		// update clipper - might have been done by UpdateSize
		// however, the wrong size might have been assumed
		if (!pGL->UpdateClipper())
		{
			if (verbose) pGL->logger->error("UpdateClipper failed");
			return false;
		}
	}
	// success
	return true;
}

void CStdGLCtx::DoDeselect()
{
	if (SDL_GL_MakeCurrent(this->pWindow->sdlWindow, nullptr) != 0)
	{
		throw std::runtime_error{std::string{"SDL_GL_MakeCurrent failed: "} + SDL_GetError()};
	}
}

bool CStdGLCtx::UpdateSize()
{
	// safety
	if (!pWindow) return false;
	// get size
	C4Rect rc;
	if (!pWindow->GetSize(rc)) return false;
	const auto scale = pGL->pApp->GetScale();
	int width = static_cast<int32_t>(ceilf(rc.Wdt / scale)), height = static_cast<int32_t>(ceilf(rc.Hgt / scale));
	// assign if different
	if (cx != width || cy != height)
	{
		cx = width; cy = height;
		if (pGL) pGL->UpdateClipper();
	}
	// success
	return true;
}

bool CStdGLCtx::PageFlip()
{
	// flush GL buffer
	glFlush();
	if (!pWindow) return false;
	SDL_GL_SwapWindow(this->pWindow->sdlWindow);
	return true;
}

bool CStdGL::ApplyGammaRampToMonitor(CGammaControl &ramp, bool fForce)
{
	assert(ramp.size == 256);
	return SDL_SetWindowGammaRamp(MainCtx.pWindow->sdlWindow, ramp.red, ramp.green, ramp.blue) == 0;
}

bool CStdGL::SaveDefaultGammaRampToMonitor(CStdWindow *pWindow)
{
	assert(DefRamp.size == 256);
	return SDL_GetWindowGammaRamp(MainCtx.pWindow->sdlWindow, DefRamp.red, DefRamp.green, DefRamp.blue) == 0;
}

#endif // USE_X11/USE_SDL_MAINLOOP

#endif
