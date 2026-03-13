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

#include "C4ImGui.h"

#include <array>
#include <stdexcept>

#include "imgui/imgui.h"

#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_opengl2.h"
#include <SDL2/SDL.h>

#include <gl/gl.h>

C4ImGui::Selected::Selected(ImGuiContext *newContext)
	: previousContext{ImGui::GetCurrentContext()}
{
	ImGui::SetCurrentContext(newContext);
}

C4ImGui::Selected::~Selected()
{
	ImGui::SetCurrentContext(previousContext);
}

static void ThrowIfFailed(const bool result, const char *const message)
{
	if (!result)
	{
		throw std::runtime_error{message};
	}
}

C4ImGui::C4ImGui(SDL_Window* window)
{
	context = ImGui::CreateContext();
	Select();
	ImGuiIO& io{ImGui::GetIO()};
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	ImGui::StyleColorsDark();
	SetStyle();

	ThrowIfFailed(ImGui_ImplSDL2_InitForOpenGL(window, nullptr), "ImGui SDL2 init");
	ThrowIfFailed(ImGui_ImplOpenGL2_Init(), "ImGui OpenGL2 init");
}

C4ImGui::~C4ImGui()
{
	Select();
	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplSDL2_Shutdown();

	ImGui::DestroyContext(context);
}

bool C4ImGui::IsVisible() const
{
	return isVisible;
}

bool C4ImGui::WantsKeyboard() const
{
	return ImGui::GetIO().WantCaptureKeyboard;
}

bool C4ImGui::WantsMouse() const
{
	return ImGui::GetIO().WantCaptureMouse;
}

void C4ImGui::Select()
{
	ImGui::SetCurrentContext(context);
}

C4ImGui::Selected C4ImGui::SelectTemporary()
{
	return {context};
}

void C4ImGui::SetVisible(bool visible)
{
	isVisible = visible;
	ShowCursor(visible);
}

void C4ImGui::NewFrame()
{
	Select();
	ImGui_ImplOpenGL2_NewFrame();
	ImGui::NewFrame();
}

void C4ImGui::Render()
{
	Select();
	ImGui::Render();

	std::array<GLfloat, 4 * 4> textureMatrix;
	glGetFloatv(GL_TEXTURE_MATRIX, textureMatrix.data());
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

	glLoadMatrixf(textureMatrix.data());
}

void C4ImGui::SetStyle()
{
	ImGuiStyle& style = ImGui::GetStyle();
	style.FrameRounding = 2.0f;
	// TODO: Add more styling
}

// TODO: Handle sdl messages on class instances
/*
#ifdef _WIN32
bool C4ImGui::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	const auto select = SelectTemporary();
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
	{
		return true;
	}

	if (msg >= WM_KEYFIRST && msg <= WM_KEYLAST && WantsKeyboard())
	{
		return true;
	}

	if (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST && WantsMouse())
	{
		return true;
	}

	return false;
}
#endif
*/
