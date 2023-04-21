/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2006, survivor/qualle
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

/* A wrapper class to OS dependent event and window interfaces, SDL version */

#include "C4Shape.h"
#include <Standard.h>
#include <StdWindow.h>
#include <StdApp.h>
#include <StdGL.h>
#include <StdDDraw2.h>
#include <StdFile.h>
#include <StdBuf.h>

#include <stdexcept>

namespace
{
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

CStdWindow::~CStdWindow()
{
	Clear();
}

// Only set title.
// FIXME: Read from application bundle on the Mac.
bool CStdWindow::Init(CStdApp *const app, const char *const title, const C4Rect &bounds, CStdWindow *const parent)
{
	Active = true;
	SetTitle(title);

	width = bounds.Wdt;
	height = bounds.Hgt;
	displayMode = DisplayMode::Window;
	this->app = app;

	sdlWindow = SDL_CreateWindow(title, bounds.x, bounds.y, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	ThrowIfFailed("SDL_CreateWindow", !sdlWindow);

	return true;
}

void CStdWindow::Clear() {}

// Window size is automatically managed by CStdApp's display mode management.
// Just remember the size for others to query.

bool CStdWindow::GetSize(C4Rect &rect)
{
	SDL_GL_GetDrawableSize(sdlWindow, &width, &height);
	rect = {0, 0, width, height};
	return true;
}

void CStdWindow::SetSize(const unsigned int X, const unsigned int Y)
{
	const auto scale = GetInputScale();
	width = X / scale, height = Y / scale;
	SetDisplayMode(displayMode);
}

void CStdWindow::SetTitle(const char *const Title)
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

void CStdWindow::SetDisplayMode(const DisplayMode mode)
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
