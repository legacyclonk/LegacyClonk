/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2006, survivor/qualle
 * Copyright (c) 2017-2023, The LegacyClonk Team and contributors
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

void CStdWindow::sdlToC4MCBtn(const SDL_MouseButtonEvent &e,
	int32_t &button)
{
	button = C4MC_Button_None;

	switch (e.button)
	{
	case SDL_BUTTON_LEFT:
		if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
		{
			button = e.clicks % 2 == 0 ? C4MC_Button_LeftDouble : C4MC_Button_LeftDown;
		}
		else
		{
			button = C4MC_Button_LeftUp;
		}
		break;
	case SDL_BUTTON_RIGHT:
		if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
		{
			button = e.clicks % 2 == 0 ? C4MC_Button_RightDouble : C4MC_Button_RightDown;
		}
		else
		{
			button = C4MC_Button_RightUp;
		}
		break;
	case SDL_BUTTON_MIDDLE:
		if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
		{
			button = C4MC_Button_MiddleDown;
		}
		else
		{
			button = C4MC_Button_MiddleUp;
		}
		break;
	}
}
/* CStdWindow */

CStdWindow::~CStdWindow()
{
	Clear();
}

// FIXME: Read from application bundle on the Mac.
bool CStdWindow::Init(CStdApp *const app, const char *const title, const C4Rect &bounds, CStdWindow *const parent, const std::uint32_t additionalFlags, const std::int32_t minWidth, const std::int32_t minHeight)
{
	Active = true;

	width = bounds.Wdt;
	height = bounds.Hgt;
	displayMode = DisplayMode::Window;
	this->app = app;

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	std::uint32_t flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
	flags |= additionalFlags;
	sdlWindow = SDL_CreateWindow(title, width, height, flags);
	ThrowIfFailed("SDL_CreateWindow", !sdlWindow);
	SDL_SetWindowMinimumSize(sdlWindow, minWidth, minHeight);
	SDL_SetWindowPosition(sdlWindow, bounds.x, bounds.y);
	SDL_StartTextInput(sdlWindow);

	return true;
}

void CStdWindow::StorePosition()
{
	std::int32_t x, y;
	SDL_GetWindowPosition(sdlWindow, &x, &y);
	Config.Graphics.PositionX = x;
	Config.Graphics.PositionY = y;
}

void CStdWindow::RestorePosition()
{
	// Only has an effect in windowed mode.
	SDL_SetWindowPosition(sdlWindow, Config.Graphics.PositionX, Config.Graphics.PositionY);
}

void CStdWindow::InitImGui()
{
	imGui.emplace(sdlWindow);
}

void CStdWindow::Clear()
{
	if(sdlWindow)
	{
		SDL_StopTextInput(sdlWindow);
	}
}

// Window size is automatically managed by CStdApp's display mode management.
// Just remember the size for others to query.

bool CStdWindow::GetSize(C4Rect &rect)
{
	SDL_GetWindowSizeInPixels(sdlWindow, &width, &height);
	rect = {0, 0, width, height};
	return true;
}

void CStdWindow::SetSize(const unsigned int X, const unsigned int Y)
{
	const float scale{GetInputScale()};
	width = X / scale, height = Y / scale;
	SetDisplayMode(displayMode);
}

void CStdWindow::SetTitle(const char *const title)
{
	SDL_SetWindowTitle(sdlWindow, title);
}

void CStdWindow::FlashWindow()
{
	SDL_FlashWindow(sdlWindow, SDL_FlashOperation::SDL_FLASH_BRIEFLY);
}

void CStdWindow::SetDisplayMode(const DisplayMode mode)
{
	if (mode == DisplayMode::Fullscreen)
	{
		ThrowIfFailed("SDL_SetWindowFullscreen", !SDL_SetWindowFullscreen(sdlWindow, true));
	}
	else
	{
		if (displayMode == DisplayMode::Fullscreen)
		{
			const auto currentDisplay = SDL_GetDisplayForWindow(sdlWindow);
			ThrowIfFailed("SDL_GetWindowDisplayIndex", currentDisplay <= 0);
			const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(currentDisplay);
			ThrowIfFailed("SDL_GetCurrentDisplayMode", mode == nullptr);
			if(mode)
			{
				width = mode->w - 100;
				height = mode->h - 100;
			}
		}

		ThrowIfFailed("SDL_SetWindowFullscreen", !SDL_SetWindowFullscreen(sdlWindow, false));
		SDL_SetWindowSize(sdlWindow, width, height);
	}

	displayMode = mode;
	ThrowIfFailed("SDL_ShowCursor", !SDL_ShowCursor());
}

void CStdWindow::SetProgress(uint32_t) {} // stub

float CStdWindow::GetInputScale()
{
	int width, height;
	SDL_GetWindowSize(sdlWindow, &width, &height);

	int drawableWidth, drawableHeight;
	SDL_GetWindowSizeInPixels(sdlWindow, &drawableWidth, &drawableHeight);

	return static_cast<float>(drawableWidth) / static_cast<float>(width);
}
