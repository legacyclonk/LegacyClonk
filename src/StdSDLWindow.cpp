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

	SDL_DisplayMode mode;
	SDL_GetCurrentDisplayMode(0, &mode);

	resolutionX = mode.w;
	resolutionY = mode.h;
	width = height = 0;
	displayMode = DisplayMode::Window;
	app = pApp;

	sdlWindow = SDL_CreateWindow(Title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, resolutionX, resolutionY, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

	return this;
}

void CStdWindow::Clear() {}

bool CStdWindow::RestorePosition(const char *, const char *, bool) { return true; }

// Window size is automatically managed by CStdApp's display mode management.
// Just remember the size for others to query.

bool CStdWindow::GetSize(RECT *pRect)
{
	pRect->left = pRect->top = 0;
	pRect->right = width, pRect->bottom = height;
	return true;
}

void CStdWindow::SetSize(unsigned int X, unsigned int Y)
{
	width = X, height = Y;
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
	auto newWidth = width;
	auto newHeight = height;
	if (mode == DisplayMode::Fullscreen)
	{
		newWidth = resolutionX;
		newHeight = resolutionY;
		SDL_SetWindowFullscreen(sdlWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
	}
	else
	{
		SDL_SetWindowFullscreen(sdlWindow, 0);
		SDL_SetWindowSize(sdlWindow, newWidth, newHeight);
	}

	displayMode = mode;
	SDL_ShowCursor(SDL_DISABLE);
	UpdateSize(newWidth, newHeight);
}

void CStdWindow::SetProgress(uint32_t) {} // stub

void CStdWindow::UpdateSize(int newWidth, int newHeight)
{
	if (width != newWidth || height != newHeight)
	{
		width = newWidth;
		height = newHeight;
		SetDisplayMode(displayMode);
		SDL_Event e;
		e.type = SDL_WINDOWEVENT;
		e.window.event = SDL_WINDOWEVENT_RESIZED;
		e.window.data1 = width;
		e.window.data2 = height;
		app->HandleSDLEvent(e);
	}
}
