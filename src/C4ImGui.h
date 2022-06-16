/*
 * LegacyClonk
 *
 * Copyright (c) 2022, The LegacyClonk Team and contributors
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

#pragma once

#include "C4Windows.h"

struct ImGuiContext;

class C4ImGui
{
public:
#ifdef USE_SDL_MAINLOOP
#elif defined(_WIN32)
	using WindowHandle = HWND;
#endif

	struct Selected
	{
		Selected(ImGuiContext *newContext);
		~Selected();

	private:
		ImGuiContext *previousContext;
	};

public:
	C4ImGui(WindowHandle window);
	~C4ImGui();

public:
	bool IsVisible() const;
	bool WantsKeyboard() const;
	bool WantsMouse() const;
	void Select();
	[[nodiscard]] Selected SelectTemporary();
	void SetVisible(bool visible);
	void NewFrame();
	void Render();

#ifdef _WIN32
	bool HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

private:
	struct ImGuiContext *context{nullptr};
	bool isVisible{false};
};
