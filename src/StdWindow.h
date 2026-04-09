/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2005, Günther
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

/* A wrapper class to OS dependent event and window interfaces */

#pragma once

#include "C4Rect.h"
#include <Standard.h>

#include "C4Rect.h"
#include <StdBuf.h>

#include <tuple>

#ifdef _WIN32
#include <shobjidl.h>
#endif

#include "C4ImGui.h"
#include <optional>

class CStdApp;
struct SDL_Window;
union SDL_Event;
struct SDL_MouseButtonEvent;

enum class DisplayMode
{
	Fullscreen,
	Window
};

class CStdWindow
{
public:
	CStdWindow() = default;
	virtual ~CStdWindow();
	bool Active{false};
	virtual void Clear();
	// Only when the wm requests a close
	// For example, when the user clicks the little x in the corner or uses Alt-F4
	virtual void Close() = 0;
	// Keypress(es) translated to a char
	virtual void CharIn(const char *c) {}
	virtual bool Init(CStdApp *app, const char *title, const C4Rect &bounds = defaultBounds, CStdWindow *parent = nullptr, std::uint32_t additionalFlags = 0, std::int32_t minWidth = 250, std::int32_t minHeight = 250);
	void StorePosition();
	void RestorePosition();
	bool GetSize(C4Rect &rect);
	static constexpr C4Rect defaultBounds{0, 0, 100, 100};

	void InitImGui();
	std::optional<C4ImGui> imGui;

	void SetSize(unsigned int cx, unsigned int cy); // resize
	void SetTitle(const char *title);
	void FlashWindow();
	void SetDisplayMode(DisplayMode mode);
	void SetProgress(uint32_t progress); // progress 100 disables the progress bar
	void CenterMouseInWindow();

protected:
	virtual void Sec1Timer() {}

#if defined(USE_SDL_MAINLOOP)
public:
	static void sdlToC4MCBtn(const SDL_MouseButtonEvent &e, int32_t &button);
	float GetInputScale();

protected:
	SDL_Window *sdlWindow;
	virtual void HandleMessage(SDL_Event &) {}

private:
	int width, height;
	CStdApp *app;
	DisplayMode displayMode;

#endif

	friend class CStdDDraw;
	friend class CStdGL;
	friend class CStdGLCtx;
	friend class CStdApp;
	friend class CStdGtkWindow;
};
