/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2006, survivor/qualle
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

/* A wrapper class to OS dependent event and window interfaces, SDL version */

#include <Standard.h>
#include <StdWindow.h>
#include <StdGL.h>
#include <StdDDraw2.h>
#include <StdFile.h>
#include <StdBuf.h>

#include <stdexcept>

namespace {
	void ThrowIfFailed(const char *const funcName, const bool failed)
	{
		if (failed)
		{
			throw std::runtime_error(std::string{"SDL: "} +
			funcName + " failed: " + SDL_GetError());
		}
	}
}

/* CStdWindow */

CStdWindow::CStdWindow() :
	Active{false}, sdlWindow{} {}

CStdWindow::~CStdWindow()
{
	Clear();
}

// Only set title.
// FIXME: Read from application bundle on the Mac.

CStdWindow *CStdWindow::Init(CStdApp *pApp)
{
	return Init(pApp, STD_PRODUCT);
}

CStdWindow *CStdWindow::Init(CStdApp *pApp, const char *Title, CStdWindow *pParent, bool HideCursor)
{
	Active = true;
	SetTitle(Title);

	width = 100;
	height = 100;
	displayMode = DisplayMode::Window;
	app = pApp;

	sdlWindow = SDL_CreateWindow(Title, 0, 0, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	ThrowIfFailed("SDL_CreateWindow", !sdlWindow);

	return this;
}

void CStdWindow::Clear() {}

bool CStdWindow::RestorePosition(const char *, const char *, bool) { return true; }

// Window size is automatically managed by CStdApp's display mode management.
// Just remember the size for others to query.

bool CStdWindow::GetSize(RECT *pRect)
{
	SDL_GL_GetDrawableSize(sdlWindow, &width, &height);

	pRect->left = pRect->top = 0;
	pRect->right = width;
	pRect->bottom = height;
	return true;
}

void CStdWindow::SetSize(unsigned int X, unsigned int Y)
{
	const auto scale = GetInputScale();
	width = X / scale, height = Y / scale;
	SetDisplayMode(displayMode);
}

void CStdWindow::SetTitle(const char *Title)
{
	SDL_SetWindowTitle(sdlWindow, Title);
}

void CStdWindow::FlashWindow()
{
#ifdef __APPLE__
	void requestUserAttention();
	requestUserAttention();
#endif
}

void CStdWindow::SetDisplayMode(DisplayMode mode)
{
	if (mode == DisplayMode::Fullscreen)
	{
		ThrowIfFailed("SDL_SetWindowFullscreen", SDL_SetWindowFullscreen(sdlWindow, SDL_WINDOW_FULLSCREEN_DESKTOP) != 0);
	}
	else
	{
		if (displayMode == DisplayMode::Fullscreen)
		{
			const auto currentDisplay = SDL_GetWindowDisplayIndex(sdlWindow);
			ThrowIfFailed("SDL_GetWindowDisplayIndex", currentDisplay < 0);
			SDL_DisplayMode mode;
			ThrowIfFailed("SDL_GetCurrentDisplayMode", SDL_GetCurrentDisplayMode(currentDisplay, &mode) != 0);

			width = mode.w - 100;
			height = mode.h - 100;
		}

		ThrowIfFailed("SDL_SetWindowFullscreen", SDL_SetWindowFullscreen(sdlWindow, 0) != 0);
		SDL_SetWindowSize(sdlWindow, width, height);
	}

	displayMode = mode;
	ThrowIfFailed("SDL_ShowCursor", SDL_ShowCursor(SDL_DISABLE) < 0);
}

void CStdWindow::SetProgress(uint32_t) {} // stub

float CStdWindow::GetInputScale()
{
	int width, height;
	SDL_GetWindowSize(sdlWindow, &width, &height);

	int drawableWidth, drawableHeight;
	SDL_GL_GetDrawableSize(sdlWindow, &drawableWidth, &drawableHeight);

	return static_cast<float>(drawableWidth) / static_cast<float>(width);
}
