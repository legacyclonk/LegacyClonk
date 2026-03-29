/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2024, The LegacyClonk Team and contributors
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

/* Drawing tools dialog for landscape editing in console mode */

#include <C4Include.h>
#include <C4ToolsDlg.h>
#include <C4Console.h>
#include <C4Application.h>
#include <StdRegistry.h>
#ifndef USE_CONSOLE
#include <StdGL.h>
#endif

#include "C4Wrappers.h"

#include "imgui/imgui.h"
#include "res/DeveloperModeIcons.h"

C4ToolsDlg::C4ToolsDlg()
{
	Default();
}

C4ToolsDlg::~C4ToolsDlg()
{
	Clear();
}

void C4ToolsDlg::Open()
{
	Active = true;
}

void C4ToolsDlg::Default()
{
	Active = false;
	Tool = SelectedTool = ToolMode::Brush;
	Grade = GradeDefault;
	ModeIFT = true;
	SCopy("Earth", Material);
	SCopy("Rough", Texture);

	if(lpDDraw)
	{
		lpDDraw->LoadTextureFromMemory(DeveloperModeStaticImage, DeveloperModeStaticImageLength, &StaticImage);
		lpDDraw->LoadTextureFromMemory(DeveloperModeDynamicImage, DeveloperModeDynamicImageLength, &DynamicImage);
		lpDDraw->LoadTextureFromMemory(DeveloperModeExactImage, DeveloperModeExactImageLength, &ExactImage);

		lpDDraw->LoadTextureFromMemory(DeveloperModeBrushImage, DeveloperModeBrushImageLength, &BrushImage);
		lpDDraw->LoadTextureFromMemory(DeveloperModeLineImage, DeveloperModeLineImageLength, &LineImage);
		lpDDraw->LoadTextureFromMemory(DeveloperModeRectImage, DeveloperModeRectImageLength, &RectImage);
		lpDDraw->LoadTextureFromMemory(DeveloperModePickerImage, DeveloperModePickerImageLength, &PickerImage);
		lpDDraw->LoadTextureFromMemory(DeveloperModeFillImage, DeveloperModeFillImageLength, &FillImage);

		lpDDraw->LoadTextureFromMemory(DeveloperModeIftImage, DeveloperModeIftImageLength, &IftImage);
		lpDDraw->LoadTextureFromMemory(DeveloperModeNoIftImage, DeveloperModeNoIftImageLength, &NoIftImage);
	}
}

void C4ToolsDlg::Clear()
{
	Active = false;
}

void C4ToolsDlg::SetTool(const ToolMode tool, const bool temp)
{
	Tool = tool;
	if (!temp)
	{
		SelectedTool = Tool;
	}

	UpdatePreview();
}

bool C4ToolsDlg::ToggleTool()
{
	SetTool(static_cast<ToolMode>((static_cast<std::underlying_type_t<ToolMode>>(Tool) + 1) % 4), false);
	return true;
}

void C4ToolsDlg::SetMaterial(const char *szMaterial)
{
	SCopy(szMaterial, Material, C4M_MaxName);
	AssertValidTexture();
	UpdatePreview();
}

void C4ToolsDlg::SetTexture(const char *szTexture)
{
	// assert valid (for separator selection)
	if (!Game.TextureMap.GetTexture(szTexture))
	{
		return;
	}
	SCopy(szTexture, Texture, C4M_MaxName);
	UpdatePreview();
}

void C4ToolsDlg::SetIFT(bool fIFT)
{
	ModeIFT = fIFT;
	if (fIFT) ModeIFT = 1; else ModeIFT = 0;
	UpdatePreview();
}

void C4ToolsDlg::UpdatePreview()
{
	if (!Active) return;

	if (Game.Landscape.Mode < C4LSC_Static)
	{
		surfacePreview.Clear();
		return;
	}
	const auto surfacePreview = std::make_unique<C4Surface>(PreviewWidth, PreviewHeight);

	//Application.DDraw->DrawBox(purfacePreview.get(), 0, 0, previewWidth - 1, PreviewHeight - 1, CGray4);

	uint8_t bCol = 0;
	CPattern Pattern1;
	CPattern Pattern2;
	// Sky material: sky as pattern only
	if (SEqual(Material, C4TLS_MatSky))
	{
		Pattern1.SetColors(nullptr, nullptr);
		Pattern1.Set(Game.Landscape.Sky.Surface, 0, false);
	}
	// Material-Texture
	else
	{
		bCol = Mat2PixColDefault(Game.Material.Get(Material));
		// Get/Create TexMap entry
		uint8_t iTex = Game.TextureMap.GetIndex(Material, Texture, true);
		if (iTex)
		{
			// Define texture pattern
			const C4TexMapEntry *pTex = Game.TextureMap.GetEntry(iTex);
			// Security
			if (pTex)
			{
				// Set drawing pattern
				Pattern2 = pTex->getPattern();
				// get and set extended texture of material
				C4Material *pMat = pTex->GetMaterial();
				if (pMat && !(pMat->OverlayType & C4MatOv_None))
				{
					Pattern1 = pMat->MatPattern;
				}
			}
		}
	}

	Application.DDraw->DrawPatternedCircle(surfacePreview.get(),
		PreviewWidth / 2, PreviewHeight / 2,
		Grade,
		bCol, Pattern1, Pattern2, *Game.Landscape.GetPal());
}

void C4ToolsDlg::UpdateGrade()
{
	Grade = std::clamp(Grade, GradeMin, GradeMax);
	UpdatePreview();
}

bool C4ToolsDlg::ChangeGrade(int32_t iChange)
{
	Grade += iChange;
	UpdateGrade();
	return true;
}

bool C4ToolsDlg::SetLandscapeMode(int32_t iMode, bool fThroughControl)
{
	int32_t iLastMode = Game.Landscape.Mode;
	// send as control
	if (!fThroughControl)
	{
		Game.Control.DoInput(CID_EMDrawTool, new C4ControlEMDrawTool(EMDT_SetMode, iMode), CDT_Decide);
		return true;
	}
	// Set landscape mode
	Game.Landscape.SetMode(iMode);
	// Exact to static: redraw landscape from map
	if (iLastMode == C4LSC_Exact)
		if (iMode == C4LSC_Static)
			Game.Landscape.MapToLandscape();
	// Assert valid tool
	if (iMode != C4LSC_Exact)
		if (SelectedTool == ToolMode::Fill)
			SetTool(ToolMode::Brush, false);
	// Success
	return true;
}

void C4ToolsDlg::AssertValidTexture()
{
	// Static map mode only
	if (Game.Landscape.Mode != C4LSC_Static) return;
	// Ignore if sky
	if (SEqual(Material, C4TLS_MatSky)) return;
	// Current material-texture valid
	if (Game.TextureMap.GetIndex(Material, Texture, false)) return;
	// Find valid material-texture
	const char *szTexture;
	for (int32_t iTexture = 0; (szTexture = Game.TextureMap.GetTexture(iTexture)); iTexture++)
	{
		if (Game.TextureMap.GetIndex(Material, szTexture, false))
		{
			SetTexture(szTexture); return;
		}
	}
	// No valid texture found
}

void C4ToolsDlg::SetAlternateTool()
{
	// alternate tool is the picker in any mode
	SetTool(ToolMode::Picker, true);
}

void C4ToolsDlg::ResetAlternateTool()
{
	// reset tool to selected tool in case alternate tool was set
	SetTool(SelectedTool, true);
}

void C4ToolsDlg::Draw()
{
	if (!Active) return;
	ImGui::PushStyleColor(ImGuiCol_FrameBg, {0.06, 0.12, 0.18, 0.9});
	static ImVec2 PropertySize{210, 0};
	ImGui::SetNextWindowSize(PropertySize);

	ImGui::Begin(LoadResStr(C4ResStrTableKey::IDS_DLG_TOOLS), &Active, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

	auto CreateSelectedButton = [this] (std::uint32_t ImageId, bool IsDisabled, bool IsHighlighted, const char* Tooltip="") -> bool
	{
		const std::uint32_t ImageSize = 16;
		ImGui::BeginDisabled(IsDisabled);
		ImGui::PushID(ImageId);
		bool WasClicked = false;
		if(ImGui::ImageButton("", (ImTextureID)(intptr_t)ImageId, {ImageSize,ImageSize}))
		{
			WasClicked = true;
		}
		if(SLen(Tooltip) > 0)
		{
			ImGui::SetItemTooltip(Tooltip);
		}

		if(IsHighlighted)
		{
			ImGui::GetWindowDrawList()->AddRect(
			ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
			IM_COL32(30, 200, 247, 255),
			2.0f, 0, 3.0f);
		}

		ImGui::PopID();
		ImGui::EndDisabled();
		return WasClicked;
	};

	if (CreateSelectedButton(BrushImage, Game.Landscape.Mode < C4LSC_Static, ToolMode::Brush == SelectedTool, "Brush")) { SetTool(ToolMode::Brush,  false); }
	ImGui::SameLine();
	if (CreateSelectedButton(LineImage, Game.Landscape.Mode < C4LSC_Static, ToolMode::Line == SelectedTool, "Line")) { SetTool(ToolMode::Line,  false); }
	ImGui::SameLine();
	if (CreateSelectedButton(RectImage, Game.Landscape.Mode < C4LSC_Static, ToolMode::Rect == SelectedTool, "Rectangle")) { SetTool(ToolMode::Rect,  false); }
	ImGui::SameLine();
	if (CreateSelectedButton(PickerImage, Game.Landscape.Mode < C4LSC_Static, ToolMode::Picker == SelectedTool, "Material picker | Shortcut: Middle mouse button")) { SetTool(ToolMode::Picker,  false); }
	if(Game.Landscape.Mode == C4LSC_Exact)
	{
		ImGui::SameLine();
		if (CreateSelectedButton(FillImage, Game.IsPaused(), ToolMode::Fill == SelectedTool, "Dynamic fill | Only works when the game is running")) { SetTool(ToolMode::Fill,  false); }
	}

	if (CreateSelectedButton(IftImage, Game.Landscape.Mode < C4LSC_Static, ModeIFT, "With tunnel")) { SetIFT(true); }
	ImGui::SameLine();
	if (CreateSelectedButton(NoIftImage, Game.Landscape.Mode < C4LSC_Static, !ModeIFT, "Without tunnel")) { SetIFT(false); }

	ImGui::Text("Brush size");
	ImGui::SetNextItemWidth(190);
	if (ImGui::SliderInt("##Grade", &Grade, GradeMin, GradeMax, "%d", ImGuiSliderFlags_AlwaysClamp))
	{
		UpdateGrade();
	}

	ImGui::BeginGroup();
	ImGui::BeginDisabled(Game.Landscape.Mode < C4LSC_Static);
	if(ImGui::CollapsingHeader(LoadResStr(C4ResStrTableKey::IDS_CTL_MATERIAL), ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::BeginListBox("##MaterialBox", {200, 150}))
		{
			const auto addMaterialEntry = [this](const char *const material)
			{
				if (ImGui::Selectable(material, SEqual(Material, material))) SetMaterial(material);
			};

			addMaterialEntry(C4TLS_MatSky);

			for (std::int32_t i{0}; i < Game.Material.Num; ++i)
			{
				addMaterialEntry(Game.Material.Map[i].Name);
			}

			ImGui::EndListBox();
		}
	}

	if(ImGui::CollapsingHeader(LoadResStr(C4ResStrTableKey::IDS_CTL_TEXTURE), ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::BeginListBox("##TextureBox", {200, 150}))
		{
			const auto addTextureEntry = [this](const char *const texture)
			{
				if (ImGui::Selectable(texture, SEqual(Texture, texture)))
				{
					SetTexture(texture);
				}
			};

			// atop: valid textures
			const char *texture;
			for (std::int32_t i{0}; (texture = Game.TextureMap.GetTexture(i)); ++i)
			{
				// Current material-texture valid? Always valid for exact mode
				if (Game.TextureMap.GetIndex(Material, texture, false) || Game.Landscape.Mode == C4LSC_Exact)
				{
					addTextureEntry(texture);
				}
			}

			if (Game.Landscape.Mode != C4LSC_Exact)
			{
				ImGui::Separator();

				// bottom-most: any invalid textures
				for (std::int32_t i{0}; (texture = Game.TextureMap.GetTexture(i)); ++i)
				{
					if (!Game.TextureMap.GetIndex(Material, texture, false))
					{
						addTextureEntry(texture);
					}
				}
			}

			ImGui::EndListBox();
		}
	}
	ImGui::EndDisabled();
	ImGui::EndGroup();

	ImGui::Spacing();

	const std::int32_t oldLandscapeMode{Game.Landscape.Mode};
	// Static or exact landscapes can't be converted back to generated landscapes. So this button is merely to signalize state.
	if(Game.Landscape.Mode == C4LSC_Dynamic)
	{
		CreateSelectedButton(DynamicImage, false, oldLandscapeMode == C4LSC_Dynamic, "Dynamic: Generated");
	}
	ImGui::SameLine();
	// Static: enable only if map available
	if (CreateSelectedButton(StaticImage, !Game.Landscape.Map, oldLandscapeMode == C4LSC_Static, "Static: Grid based"))
	{
		// Exact to static: confirm data loss warning
		if (oldLandscapeMode == C4LSC_Exact)
		{
			ImGui::OpenPopup("Exact To Static");
		}
		else
		{
			SetLandscapeMode(C4LSC_Static);
		}
	}
	ImGui::SameLine();
	// Exact: enable always
	if (CreateSelectedButton(ExactImage, false, oldLandscapeMode == C4LSC_Exact, "Exact: Pixel-wise | Changes here are not transferable to static mode.")){ SetLandscapeMode(C4LSC_Exact); }

	ImGui::BeginDisabled(Game.Landscape.Mode < C4LSC_Static);
	ImGui::BeginGroup();
	const ImVec2 &initialCursorPos{ImGui::GetCursorPos()};
	const auto texSize = static_cast<float>(surfacePreview.iTexSize);

	for (std::int32_t texY{0}; texY < surfacePreview.iTexY; ++texY)
	{
		for (std::int32_t texX{0}; texX < surfacePreview.iTexX; ++texX)
		{
			ImGui::SetCursorPos({initialCursorPos.x + texX * texSize, initialCursorPos.y + texY * texSize});

			// TODO: Imgui Image
			//ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<std::uintptr_t>(surfacePreview.ppTex[texY][texX].texName)), {texSize, texSize});
		}
	}
	ImGui::EndGroup();
	ImGui::EndDisabled();


	ImGui::SetNextWindowSize({400, 200});
	if (ImGui::BeginPopupModal("Exact To Static", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
	{
		ImGui::TextWrapped("%s", LoadResStr(C4ResStrTableKey::IDS_CNS_EXACTTOSTATIC));

		ImGui::Spacing();

		if (ImGui::Button(LoadResStr(C4ResStrTableKey::IDS_DLG_YES)))
		{
			SetLandscapeMode(C4LSC_Static);
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine(0, 50);

		if (ImGui::Button(LoadResStr(C4ResStrTableKey::IDS_DLG_NO)))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	ImGui::End();

	ImGui::PopStyleColor(1);

	if (!Active)
	{
		Clear();
	}
}

#ifdef WITH_DEVELOPER_MODE

// GTK+ callbacks

void C4ToolsDlg::OnButtonModeDynamic(GtkWidget *widget, gpointer data)
{
	static_cast<C4ToolsDlg *>(data)->SetLandscapeMode(C4LSC_Dynamic);
}

void C4ToolsDlg::OnButtonModeStatic(GtkWidget *widget, gpointer data)
{
	static_cast<C4ToolsDlg *>(data)->SetLandscapeMode(C4LSC_Static);
}

void C4ToolsDlg::OnButtonModeExact(GtkWidget *widget, gpointer data)
{
	static_cast<C4ToolsDlg *>(data)->SetLandscapeMode(C4LSC_Exact);
}

void C4ToolsDlg::OnButtonBrush(GtkWidget *widget, gpointer data)
{
	static_cast<C4ToolsDlg *>(data)->SetTool(ToolMode::Brush, false);
}

void C4ToolsDlg::OnButtonLine(GtkWidget *widget, gpointer data)
{
	static_cast<C4ToolsDlg *>(data)->SetTool(ToolMode::Line, false);
}

void C4ToolsDlg::OnButtonRect(GtkWidget *widget, gpointer data)
{
	static_cast<C4ToolsDlg *>(data)->SetTool(ToolMode::Rect, false);
}

void C4ToolsDlg::OnButtonFill(GtkWidget *widget, gpointer data)
{
	static_cast<C4ToolsDlg *>(data)->SetTool(ToolMode::Fill, false);
}

void C4ToolsDlg::OnButtonPicker(GtkWidget *widget, gpointer data)
{
	static_cast<C4ToolsDlg *>(data)->SetTool(ToolMode::Picker, false);
}

void C4ToolsDlg::OnButtonIft(GtkWidget *widget, gpointer data)
{
	static_cast<C4ToolsDlg *>(data)->SetIFT(true);
}

void C4ToolsDlg::OnButtonNoIft(GtkWidget *widget, gpointer data)
{
	static_cast<C4ToolsDlg *>(data)->SetIFT(false);
}

void C4ToolsDlg::OnComboMaterial(GtkWidget *widget, gpointer data)
{
	gchar *text = gtk_combo_box_get_active_text(GTK_COMBO_BOX(widget));
	static_cast<C4ToolsDlg *>(data)->SetMaterial(text);
	g_free(text);
}

void C4ToolsDlg::OnComboTexture(GtkWidget *widget, gpointer data)
{
	gchar *text = gtk_combo_box_get_active_text(GTK_COMBO_BOX(widget));
	static_cast<C4ToolsDlg *>(data)->SetTexture(text);
	g_free(text);
}

void C4ToolsDlg::OnGrade(GtkWidget *widget, gpointer data)
{
	C4ToolsDlg *dlg = static_cast<C4ToolsDlg *>(data);
	int value = static_cast<int>(gtk_range_get_value(GTK_RANGE(dlg->scale)) + 0.5);
	dlg->SetGrade(ToolMode::GradeMax - value);
}

void C4ToolsDlg::OnWindowHide(GtkWidget *widget, gpointer data)
{
	static_cast<C4ToolsDlg *>(data)->Active = false;
}

#endif
