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

#ifdef USE_GL

#ifdef USE_X11
#include <X11/extensions/xf86vmode.h>
#include <GL/gl.h>
#include <GL/glx.h>
#endif

bool CStdGLCtx::Select(bool verbose, bool selectOnly)
{
	// safety
	if (!pGL || !hrc) return false; if (!pGL->lpPrimary) return false;

	// make context current
	if (!MakeCurrent()) return false;

	if (!selectOnly)
	{
		pGL->pCurrCtx = this;
		// update size
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
		if (!pGL->UpdateClipper()) return false;
	}
	// success
	return true;
}

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

bool CStdGLCtx::InitGLEW()
{
	glewExperimental = true;
	GLenum err = glewInit();
	return err == GLEW_OK || pGL->Error(FormatString("  gl: glewInit(): %s", reinterpret_cast<const char *>(glewGetErrorString(err))).getData());
}

bool CStdGLCtx::CheckExtension(const char *extension)
{
	return glewIsSupported(extension) || pGL->Error(FormatString("  gl: Extension(s) %s not supported", extension).getData());
}

bool CStdGLCtx::ApplyVAOWorkaround()
{
	if (CheckExtension("APPLE_vertex_array_objects"))
	{
		pGL->DebugLog("  gl: Using APPLE_vertex_array_objects");

		glGenVertexArrays = reinterpret_cast<PFNGLGENVERTEXARRAYSPROC>(glGenVertexArraysAPPLE);
		glBindVertexArray = reinterpret_cast<PFNGLBINDVERTEXARRAYPROC>(glBindVertexArrayAPPLE);
		glDeleteVertexArrays = reinterpret_cast<PFNGLDELETEVERTEXARRAYSPROC>(glDeleteVertexArraysAPPLE);
		glIsVertexArray = reinterpret_cast<PFNGLISVERTEXARRAYPROC>(glIsVertexArrayAPPLE);

		return true;
	}

	/*else if (CheckExtension("SGIX_vertex_array_objects"))
	{
		pGL->DebugLog("  gl: Using SGIX_vertex_array_objects");

		glGenVertexArrays = reinterpret_cast<PFNGLGENVERTEXARRAYSPROC>(glGenVertexArraysSGIX);
		glBindVertexArray = reinterpret_cast<PFNGLBINDVERTEXARRAYPROC>(glBindVertexArraySGIX);
		glDeleteVertexArrays = reinterpret_cast<PFNGLDELETEVERTEXARRAYSPROC>(glDeleteVertexArraysSGIX);
		glIsVertexArray = reinterpret_cast<PFNGLISVERTEXARRAYPROC>(glIsVertexArraySGIX);

		return true;
	}*/

	else
	{
		return pGL->Error("  gl: Could not apply vertex array object workaround");
	}
}

void CStdGLCtx::Clear()
{
	Deselect();
	Destroy();
	pWindow = nullptr;
	cx = cy = 0;
}

#ifdef _WIN32
namespace
{
	using wglCreateContextAttribsARB_t = HGLRC WINAPI (HDC, HGLRC, const int *);
	using wglChoosePixelFormatARB_t = BOOL WINAPI (HDC, const int *, const FLOAT *, UINT, int *, UINT *);

	wglCreateContextAttribsARB_t *wglCreateContextAttribsARB = nullptr;
	wglChoosePixelFormatARB_t *wglChoosePixelFormatARB = nullptr;

	constexpr int WGL_CONTEXT_MAJOR_VERSION_ARB = 0x2091;
	constexpr int WGL_CONTEXT_MINOR_VERSION_ARB = 0x2092;
	constexpr int WGL_CONTEXT_PROFILE_MASK_ARB = 0x9126;
	constexpr int WGL_CONTEXT_CORE_PROFILE_BIT_ARB = 0x00000001;
	constexpr int WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB = 0x00000002;
	constexpr int WGL_CONTEXT_DEBUG_BIT_ARB = 0x0001;
	constexpr int WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB = 0x0002;

	constexpr int WGL_DRAW_TO_WINDOW_ARB = 0x2001;
	constexpr int WGL_ACCELERATION_ARB = 0x2003;
	constexpr int WGL_SUPPORT_OPENGL_ARB = 0x2010;
	constexpr int WGL_DOUBLE_BUFFER_ARB = 0x2011;
	constexpr int WGL_PIXEL_TYPE_ARB = 0x2013;
	constexpr int WGL_COLOR_BITS_ARB = 0x2014;
	constexpr int WGL_DEPTH_BITS_ARB = 0x2022;
	constexpr int WGL_STENCIL_BITS_ARB = 0x2023;
	constexpr int WGL_FULL_ACCELERATION_ARB = 0x2027;
	constexpr int WGL_TYPE_RGBA_ARB = 0x2028;
}

bool CStdGLCtx::Init(CStdWindow *pWindow, CStdApp *pApp, HWND hWindow)

#else
bool CStdGLCtx::Init(CStdWindow *pWIndow, CStdApp *pApp)
#endif
{
	// safety
	if (!pGL) return false;

	// store window
	this->pWindow = pWindow;

#ifdef _WIN32
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
	int pixelFormat = ChoosePixelFormat(hDC, &pfd);
	if (pixelFormat == 0) return !!pGL->Error("  gl: Error getting dummy pixel format");
	if (!SetPixelFormat(hDC, pixelFormat, &pfd)) pGL->Error("  gl: Error setting dummy pixel format");

	// create dummy context
	hrc = wglCreateContext(hDC); if (!hrc) return !!pGL->Error("  gl: Error creating dummy gl context");

	MakeCurrent();

	wglCreateContextAttribsARB = reinterpret_cast<wglCreateContextAttribsARB_t *>(wglGetProcAddress("wglCreateContextAttribsARB"));
	wglChoosePixelFormatARB = reinterpret_cast<wglChoosePixelFormatARB_t *>(wglGetProcAddress("wglChoosePixelFormatARB"));

	constexpr int pixelFormatAttributes[]
	{
		WGL_DRAW_TO_WINDOW_ARB, TRUE,
		WGL_SUPPORT_OPENGL_ARB, TRUE,
		WGL_DOUBLE_BUFFER_ARB, TRUE,
		WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB, 32,
		WGL_DEPTH_BITS_ARB, 24,
		WGL_STENCIL_BITS_ARB, 8,
		0
	};

	UINT formatCount;
	wglChoosePixelFormatARB(hDC, pixelFormatAttributes, nullptr, 1, &pixelFormat, &formatCount);
	DescribePixelFormat(hDC, pixelFormat, sizeof(pfd), &pfd);

	if (!SetPixelFormat(hDC, pixelFormat, &pfd))
	{
		return pGL->Error("  gl: Error setting pixel format");
	}

	wglMakeCurrent(nullptr, nullptr); pGL->pCurrCtx = nullptr;
#endif

	if (!CreateContext(3, 2))
	{
		if (!CheckExtension("ARB_vertex_array_bgra") && !CheckExtension("EXT_vertex_array_bgra")) return false;

		if (!CreateContext(3, 1))
		{
			if (!CheckExtension("ARB_uniform_buffer_object")) return false;

			if (!CreateContext(3, 0))
			{
				if (!CreateContext(2, 1) || !ApplyVAOWorkaround())
				{
					return false;
				}
			}
		}
	}

	// success
	return true;
}

bool CStdGLCtx::CreateContext(int major, int minor, bool core)
{
#ifdef _WIN32
	static int contextAttributes[]
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 1,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0
	};

	contextAttributes[1] = major;
	contextAttributes[3] = minor;
	contextAttributes[5] = (core ? WGL_CONTEXT_CORE_PROFILE_BIT_ARB : WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB);// | WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;

#ifdef _DEBUG
	contextAttributes[5] |= WGL_CONTEXT_DEBUG_BIT_ARB;
#endif

	hrc = wglCreateContextAttribsARB(hDC, 0, contextAttributes);

	// select
	if (!Select()) return !!pGL->Error(FormatString("  gl: Unable to select context: 0x%x", GetLastError() & 0xFFFF).getData());

	if (this != &pGL->MainCtx)
	{
		if (!wglShareLists(pGL->MainCtx.hrc, hrc)) pGL->Error("  gl: Textures for secondary context not available");
		return true;
	}

	InitGLEW();
	return hrc;

#elif defined(USE_X11)
	static int contextAttributes[]
	{
		GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
		GLX_CONTEXT_MINOR_VERSION_ARB, 1,
		GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
		None
	};

	contextAttributes[1] = major;
	contextAttributes[3] = minor;
	contextAttributes[5] = (core ? GLX_CONTEXT_CORE_PROFILE_BIT_ARB : GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB) | GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;

#ifdef _DEBUG
	contextAttributes[5] |= GLX_CONTEXT_DEBUG_BIT_ARB;
#endif

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

	// select
	if (!Select()) return pGL->Error("  gl: Unable to select context");

	return !error && ctx && InitGLEW();
#elif defined(USE_SDL_MAINLOOP)
	if (!Select(true)) return pGL->Error("  gl: Unable to select context");
	return InitGLEW();
#endif
}

void CStdGLCtx::DoDeselect()
{
	if (pGL && pGL->pCurrCtx == this)
	{
#ifdef _WIN32
		wglMakeCurrent(nullptr, nullptr);
#elif defined(USE_X11)
		glXMakeCurrent(pWindow->dpy, None, nullptr);
#endif
	}
}

bool CStdGLCtx::PageFlip()
{
	// flush GL buffer
	glFlush();
#ifdef _WIN32
	SwapBuffers(hDC);
#elif defined(USE_X11)
	if (!pWindow || !pWindow->renderwnd) return false;
	glXSwapBuffers(pWindow->dpy, pWindow->renderwnd);
#elif defined(USE_SDL_MAINLOOP)
	if (!pWindow) return false;
	SDL_GL_SwapBuffers();
#endif
	return true;
}

#ifdef _WIN32

CStdGLCtx::CStdGLCtx() : hrc(nullptr), pWindow(nullptr), hDC(nullptr), cx(0), cy(0) {}

void CStdGLCtx::Destroy()
{
	if (hrc)
	{
		wglDeleteContext(hrc); hrc = nullptr;
	}

	if (hDC)
	{
		ReleaseDC(pWindow ? pWindow->GetRenderWindow() : hWindow, hDC);
		hDC = nullptr;
	}

	hWindow = nullptr;
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

bool CStdGLCtx::MakeCurrent()
{
	return wglMakeCurrent(hDC, hrc);
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

CStdGLCtx::CStdGLCtx() : pWindow(nullptr), cx(0), cy(0), ctx(0) {}

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

bool CStdGLCtx::MakeCurrent()
{
	return glXMakeCurrent(pWindow->dpy, pWindow->renderwnd, ctx);
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

bool CStdGLCtx::MakeCurrent()
{
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
