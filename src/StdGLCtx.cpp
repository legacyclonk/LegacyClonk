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

/* glx conversion by Guenther, 2005 */

/* OpenGL implementation of NewGfx, the context */

#include <Standard.h>
#include <StdGL.h>
#include <StdSurface2.h>
#include <StdWindow.h>

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
	if (!hDC) return !!pGL->Error("  gl: Error getting DC");

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
	if (pixelFormat == 0) return !!pGL->Error("  gl: Error getting pixel format");
	if (!SetPixelFormat(hDC, pixelFormat, &pfd)) pGL->Error("  gl: Error setting pixel format");

	// create context
	hrc = wglCreateContext(hDC); if (!hrc) return !!pGL->Error("  gl: Error creating gl context");

	// share textures
	wglMakeCurrent(nullptr, nullptr); pGL->pCurrCtx = nullptr;
	if (this != &pGL->MainCtx)
	{
		if (!wglShareLists(pGL->MainCtx.hrc, hrc)) pGL->Error("  gl: Textures for secondary context not available");
		return true;
	}

	// select
	if (!Select()) return !!pGL->Error("  gl: Unable to select context");

	// init extensions
	glewExperimental = true;
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		// Problem: glewInit failed, something is seriously wrong.
		pGL->Error(reinterpret_cast<const char *>(glewGetErrorString(err)));
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

bool CStdGL::SaveDefaultGammaRamp(CStdWindow *pWindow)
{
	HDC hDC = GetDC(pWindow->GetRenderWindow());
	if (hDC)
	{
		if (!GetDeviceGammaRamp(hDC, DefRamp.red))
		{
			DefRamp.Default();
			Log("  Error getting default gamma ramp; using standard");
		}
		ReleaseDC(pWindow->GetRenderWindow(), hDC);
		return true;
	}
	return false;
}

bool CStdGL::ApplyGammaRamp(CGammaControl &ramp, bool fForce)
{
	if (!MainCtx.hDC || (!Active && !fForce)) return false;
	if (!SetDeviceGammaRamp(MainCtx.hDC, ramp.red))
	{
		int i = ::GetLastError();
	}
	return true;
}

#elif defined(USE_X11)

#include <X11/extensions/xf86vmode.h>
#include <GL/gl.h>
#include <GL/glx.h>

CStdGLCtx::CStdGLCtx() : pWindow(nullptr), ctx(nullptr), cx(0), cy(0) {}

void CStdGLCtx::Clear()
{
	Deselect();
	Destroy();
	pWindow = nullptr;
	cx = cy = 0;
}

void CStdGLCtx::Destroy()
{
	if (ctx)
	{
		glXDestroyContext(pWindow->dpy, ctx);
		ctx = nullptr;
	}
}

bool CStdGLCtx::Init(CStdWindow *pWindow, CStdApp *)
{
	// safety
	if (!pGL) return false;
	// store window
	this->pWindow = pWindow;

	int contextAttributes[]
	{
		GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
		GLX_CONTEXT_MINOR_VERSION_ARB, 1,
		GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
		None
	};

	auto createContext = [this, &contextAttributes](int major, int minor) -> bool
	{
		contextAttributes[1] = major;
		contextAttributes[3] = minor;

		static bool error;
		error = false;

		auto *errorHandler = XSetErrorHandler([](Display *, XErrorEvent *) -> int { error = true; return 0; });

		// Create Context with sharing (if this is the main context, our ctx will be 0, so no sharing)
		// try direct rendering first
		if (!DDrawCfg.NoAcceleration)
		{
			ctx = glXCreateContextAttribsARB(this->pWindow->dpy, this->pWindow->FBConfig, pGL->MainCtx.ctx, True, contextAttributes);
		}

		// without, rendering will be unacceptable slow, but that's better than nothing at all
		if (!ctx)
		{
			ctx = glXCreateContextAttribsARB(this->pWindow->dpy, this->pWindow->FBConfig, pGL->MainCtx.ctx, False, contextAttributes);
		}

		XSync(this->pWindow->dpy, False);
		XSetErrorHandler(errorHandler);

		if (error || !ctx)
		{
			return pGL->Error(
						FormatString(
							"  gl: Unable to create a %d.%d (%s) context!",
							major,
							minor,
							contextAttributes[5] == GLX_CONTEXT_CORE_PROFILE_BIT_ARB ? "core" : "compat"
						).getData()
					);
		}
		if (!Select(true)) return pGL->Error("  gl: Unable to select context");
		// init extensions
		GLenum err = glewInit();
		if (GLEW_OK != err)
		{
			// Problem: glewInit failed, something is seriously wrong.
			pGL->Error(reinterpret_cast<const char *>(glewGetErrorString(err)));
		}

		return true;
	};

	auto checkExtension = [](const char *extension) -> bool
	{
		if (!glewIsSupported(extension))
		{
			return pGL->Error(FormatString("  gl: Extension(s) %s not supported", extension).getData());
		}

		return true;
	};

	assert(GLX_ARB_create_context);

	if (!createContext(3, 2))
	{
		if (!checkExtension("ARB_vertex_array_bgra")) return false;

		if (!createContext(3, 1))
		{
			if (!checkExtension("ARB_uniform_buffer_object")) return false;

			if (!createContext(3, 0))
			{
				/*if (!checkExtension("ARB_vertex_array_object"))*/ return false;
			}
		}
	}

	return true;
}

bool CStdGLCtx::Select(bool verbose, bool selectOnly)
{
	// safety
	if (!pGL || !ctx)
	{
		if (verbose) pGL->Error("  gl: pGL is zero");
		return false;
	}
	if (!pGL->lpPrimary)
	{
		if (verbose) pGL->Error("  gl: lpPrimary is zero");
		return false;
	}
	// make context current
	if (!pWindow->renderwnd || !glXMakeCurrent(pWindow->dpy, pWindow->renderwnd, ctx))
	{
		if (verbose) pGL->Error("  gl: glXMakeCurrent failed");
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
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDisable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		// update clipper - might have been done by UpdateSize
		// however, the wrong size might have been assumed
		if (!pGL->UpdateClipper())
		{
			if (verbose) pGL->Error("  gl: UpdateClipper failed");
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

bool CStdGL::ApplyGammaRamp(CGammaControl &ramp, bool fForce)
{
	if (!DeviceReady() || (!Active && !fForce)) return false;
	if (pApp->xf86vmode_major_version < 2) return false;
	if (gammasize != ramp.size || ramp.size == 0) return false;
	return XF86VidModeSetGammaRamp(pApp->dpy, DefaultScreen(pApp->dpy), ramp.size,
		ramp.red, ramp.green, ramp.blue);
}

bool CStdGL::SaveDefaultGammaRamp(CStdWindow *pWindow)
{
	if (pApp->xf86vmode_major_version < 2) return false;
	// Get the Display
	Display *const dpy = pWindow->dpy;
	XF86VidModeGetGammaRampSize(dpy, DefaultScreen(dpy), &gammasize);
	if (gammasize != 256)
	{
		DefRamp.Set(0x000000, 0x808080, 0xffffff, gammasize, nullptr);
		LogF("  Size of GammaRamp is %d, not 256", gammasize);
	}
	// store default gamma
	if (!XF86VidModeGetGammaRamp(pWindow->dpy, DefaultScreen(pWindow->dpy), DefRamp.size,
		DefRamp.red, DefRamp.green, DefRamp.blue))
	{
		DefRamp.Default();
		Log("  Error getting default gamma ramp; using standard");
	}
	Gamma.Set(0x000000, 0x808080, 0xffffff, gammasize, &DefRamp);
	return true;
}

#elif defined(USE_SDL_MAINLOOP)

CStdGLCtx::CStdGLCtx() : pWindow(nullptr), cx(0), cy(0) {}

void CStdGLCtx::Clear()
{
	pWindow = nullptr;
	cx = cy = 0;
}

bool CStdGLCtx::Init(CStdWindow *pWindow, CStdApp *)
{
	// safety
	if (!pGL) return false;
	// store window
	this->pWindow = pWindow;
	assert(!DDrawCfg.NoAcceleration);
	// No luck at all?
	if (!Select(true)) return pGL->Error("  gl: Unable to select context");
	// init extensions
	glewExperimental = true;
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		// Problem: glewInit failed, something is seriously wrong.
		pGL->Error(reinterpret_cast<const char *>(glewGetErrorString(err)));
	}
	return true;
}

bool CStdGLCtx::Select(bool verbose, bool selectOnly)
{
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
			if (verbose) pGL->Error("  gl: UpdateClipper failed");
			return false;
		}
	}
	// success
	return true;
}

void CStdGLCtx::DoDeselect()
{

}

bool CStdGLCtx::UpdateSize()
{
	// safety
	if (!pWindow) return false;
	// get size
	RECT rc;
	pWindow->GetSize(&rc);
	const auto scale = pGL->pApp->GetScale();
	int width = static_cast<int32_t>(ceilf((rc.right - rc.left) / scale)), height = static_cast<int32_t>(ceilf((rc.bottom - rc.top) / scale));
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
	SDL_GL_SwapBuffers();
	return true;
}

bool CStdGL::ApplyGammaRamp(CGammaControl &ramp, bool fForce)
{
	assert(ramp.size == 256);
	return SDL_SetGammaRamp(ramp.red, ramp.green, ramp.blue) != -1;
}

bool CStdGL::SaveDefaultGammaRamp(CStdWindow *pWindow)
{
	assert(DefRamp.size == 256);
	return SDL_GetGammaRamp(DefRamp.red, DefRamp.green, DefRamp.blue) != -1;
}

#endif // USE_X11/USE_SDL_MAINLOOP

#endif
