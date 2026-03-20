/*
* LegacyClonk
 *
 * Copyright (c) 2026, The LegacyClonk Team and contributors
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

struct ImGuiContext;
class SDL_Window;

class C4ImGui
{
public:
	struct Selected
	{
		Selected(ImGuiContext *newContext);
		~Selected();

	private:
		ImGuiContext *previousContext;
	};

public:
	C4ImGui(SDL_Window* window);
	~C4ImGui();

public:
	bool IsVisible() const;
	bool WantsKeyboard() const;
	bool WantsMouse() const;
	void Select();
	[[nodiscard]] Selected SelectTemporary();
	void SetVisible(bool visible);
	bool NewFrame();
	void Render();

private:
	struct ImGuiContext *context{nullptr};
	bool isVisible{false};

	void SetStyle();
};
