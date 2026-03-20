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
	ImGuiContext* prev_ctx = ImGui::GetCurrentContext();
	context = ImGui::CreateContext();
	Select(); // Set explicitly since ImGui::CreateContext won't change the global context if there was already one assigned.
	ImGuiIO& io{ImGui::GetIO()};
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	ImGui::StyleColorsDark();
	SetStyle();

	ThrowIfFailed(ImGui_ImplSDL2_InitForOpenGL(window, nullptr), "ImGui SDL2 init");
	ThrowIfFailed(ImGui_ImplOpenGL2_Init(), "ImGui OpenGL2 init");

	// Reset outside context so we don't run into errors if this constructor was called via another ImGui UI (e.g When creating a new viewport).
	if(prev_ctx)
	{
		ImGui::SetCurrentContext(prev_ctx);
	}
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

bool C4ImGui::NewFrame()
{
	if(!context)
	{
		return false;
	}
	Select();

	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
	return true;
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
	style.WindowRounding = 2.0f;
	style.ChildRounding = 2.0f;
	style.PopupRounding = 2.0f;
	style.GrabRounding = 2.0f;
	style.FontScaleMain = 1.0f;
	style.FontSizeBase = 16;
	style.WindowTitleAlign.x = 0.5f;
	ImVec4* colors = style.Colors;

	colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg]               = ImVec4(0.07f, 0.12f, 0.18f, 0.70f);
	colors[ImGuiCol_ChildBg]                = ImVec4(0.04f, 0.04f, 0.04f, 0.50f);
	colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
	colors[ImGuiCol_Border]                 = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg]                = ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
	colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	colors[ImGuiCol_FrameBgActive]          = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_TitleBg]                = ImVec4(0.07f, 0.12f, 0.18f, 0.60f);
	colors[ImGuiCol_TitleBgActive]          = ImVec4(0.07f, 0.12f, 0.18f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	colors[ImGuiCol_MenuBarBg]              = ImVec4(0.07f, 0.12f, 0.18f, 1.00f);
	colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.04f, 0.04f, 0.70f);
	colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.27f, 0.3f, 0.34f, 1.00f);
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
