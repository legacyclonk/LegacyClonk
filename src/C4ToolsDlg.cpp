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
	opened = true;
}

void C4ToolsDlg::Default()
{
	opened = false;
	tool = selectedTool = ToolMode::Brush;
	grade = GradeDefault;
	modeIft = true;
	SCopy("Earth", material);
	SCopy("Rough", texture);

	if(lpDDraw)
	{
		dynamicImage = lpDDraw->LoadPNGFromMemory(developerModeDynamicImage, developerModeDynamicImageLength);
		staticImage = lpDDraw->LoadPNGFromMemory(developerModeStaticImage, developerModeStaticImageLength);
		exactImage = lpDDraw->LoadPNGFromMemory(developerModeExactImage, developerModeExactImageLength);

		brushImage = lpDDraw->LoadPNGFromMemory(developerModeBrushImage, developerModeBrushImageLength);
		lineImage = lpDDraw->LoadPNGFromMemory(developerModeLineImage, developerModeLineImageLength);
		rectImage = lpDDraw->LoadPNGFromMemory(developerModeRectImage, developerModeRectImageLength);
		pickerImage = lpDDraw->LoadPNGFromMemory(developerModePickerImage, developerModePickerImageLength);
		fillImage = lpDDraw->LoadPNGFromMemory(developerModeFillImage, developerModeFillImageLength);
	}
}

void C4ToolsDlg::Clear()
{
	opened = false;
}

void C4ToolsDlg::SetTool(const ToolMode tool, const bool temp)
{
	this->tool = tool;
	if (!temp)
	{
		selectedTool = tool;
	}

	UpdatePreview();
}

bool C4ToolsDlg::ToggleTool()
{
	SetTool(static_cast<ToolMode>((static_cast<std::underlying_type_t<ToolMode>>(tool) + 1) % 4), false);
	return true;
}

void C4ToolsDlg::SetMaterial(const char *szMaterial)
{
	SCopy(szMaterial, material, C4M_MaxName);
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
	SCopy(szTexture, texture, C4M_MaxName);
	UpdatePreview();
}

void C4ToolsDlg::SetIFT(bool fIFT)
{
	modeIft = fIFT;
	if (fIFT)
	{
		modeIft = 1;
	}
	else
	{
		modeIft = 0;
	}
	UpdatePreview();
}

// TODO
void C4ToolsDlg::UpdatePreview()
{
	if (!opened)
	{
		return;
	}

	if (Game.Landscape.Mode < C4LSC_Static)
	{
		surfacePreview.Clear();
		return;
	}
	const auto surfacePreview = std::make_unique<C4Surface>(PreviewWidth, PreviewHeight);

	//Application.DDraw->DrawBox(purfacePreview.get(), 0, 0, previewWidth - 1, PreviewHeight - 1, CGray4);

	std::uint8_t bCol{0};
	CPattern pattern1;
	CPattern pattern2;
	// Sky material: sky as pattern only
	if (SEqual(material, C4TLS_MatSky))
	{
		pattern1.SetColors(nullptr, nullptr);
		pattern1.Set(Game.Landscape.Sky.Surface, 0, false);
	}
	// Material-Texture
	else
	{
		bCol = Mat2PixColDefault(Game.Material.Get(material));
		// Get/Create TexMap entry
		uint8_t iTex = Game.TextureMap.GetIndex(material, texture, true);
		if (iTex)
		{
			// Define texture pattern
			const C4TexMapEntry *pTex = Game.TextureMap.GetEntry(iTex);
			// Security
			if (pTex)
			{
				// Set drawing pattern
				pattern2 = pTex->getPattern();
				// get and set extended texture of material
				C4Material *pMat = pTex->GetMaterial();
				if (pMat && !(pMat->OverlayType & C4MatOv_None))
				{
					pattern1 = pMat->MatPattern;
				}
			}
		}
	}

	Application.DDraw->DrawPatternedCircle(surfacePreview.get(),
		PreviewWidth / 2, PreviewHeight / 2,
		grade,
		bCol, pattern1, pattern2, *Game.Landscape.GetPal());
}

void C4ToolsDlg::UpdateGrade()
{
	grade = std::clamp(grade, GradeMin, GradeMax);
	UpdatePreview();
}

bool C4ToolsDlg::ChangeGrade(int32_t iChange)
{
	grade += iChange;
	UpdateGrade();
	return true;
}

bool C4ToolsDlg::SetLandscapeMode(int32_t iMode, bool fThroughControl)
{
	int32_t iLastMode{Game.Landscape.Mode};
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
		if (selectedTool == ToolMode::Fill)
			SetTool(ToolMode::Brush, false);
	// Success
	return true;
}

void C4ToolsDlg::AssertValidTexture()
{
	// Static map mode only
	if (Game.Landscape.Mode != C4LSC_Static) return;
	// Ignore if sky
	if (SEqual(material, C4TLS_MatSky)) return;
	// Current material-texture valid
	if (Game.TextureMap.GetIndex(material, texture, false)) return;
	// Find valid material-texture
	const char *szTexture;
	for (int32_t iTexture = 0; (szTexture = Game.TextureMap.GetTexture(iTexture)); iTexture++)
	{
		if (Game.TextureMap.GetIndex(material, szTexture, false))
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
	SetTool(selectedTool, true);
}

void C4ToolsDlg::Draw()
{
	if (!opened)
	{
		return;
	}
	ImGui::PushStyleColor(ImGuiCol_FrameBg, {0.06, 0.12, 0.18, 0.9});
	static ImVec2 propertySize{210, 0};
	ImGui::SetNextWindowSize(propertySize);

	ImGui::Begin(LoadResStr(C4ResStrTableKey::IDS_DLG_TOOLS), &opened, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

	static ImVec2 ImageSize{16, 16};
	auto createSelectedButton = [this, ImageSize = ImageSize] (std::uint32_t imageId, bool isDisabled, bool isHighlighted, const char* tooltip = "") -> bool
	{
		ImGui::BeginDisabled(isDisabled);
		ImGui::PushID(imageId);
		bool wasClicked{false};
		if(ImGui::ImageButton("", {imageId}, ImageSize))
		{
			wasClicked = true;
		}
		if(SLen(tooltip) > 0)
		{
			ImGui::SetItemTooltip(tooltip);
		}

		if(isHighlighted)
		{
			ImGui::GetWindowDrawList()->AddRect(
			ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
			IM_COL32(30, 200, 247, 255),
			2.0f, 0, 3.0f);
		}

		ImGui::PopID();
		ImGui::EndDisabled();
		return wasClicked;
	};

	if (createSelectedButton(brushImage, Game.Landscape.Mode < C4LSC_Static, ToolMode::Brush == selectedTool, "Brush"))
	{
		SetTool(ToolMode::Brush,  false);
	}
	ImGui::SameLine();
	if (createSelectedButton(lineImage, Game.Landscape.Mode < C4LSC_Static, ToolMode::Line == selectedTool, "Line"))
	{
		SetTool(ToolMode::Line,  false);
	}
	ImGui::SameLine();
	if (createSelectedButton(rectImage, Game.Landscape.Mode < C4LSC_Static, ToolMode::Rect == selectedTool, "Filled rectangle"))
	{
		SetTool(ToolMode::Rect,  false);
	}
	ImGui::SameLine();
	if (createSelectedButton(pickerImage, Game.Landscape.Mode < C4LSC_Static, ToolMode::Picker == selectedTool, "Material picker | Shortcut: Middle mouse button"))
	{
		SetTool(ToolMode::Picker,  false);
	}
	if(Game.Landscape.Mode == C4LSC_Exact)
	{
		ImGui::SameLine();
		if (createSelectedButton(fillImage, Game.IsPaused(), ToolMode::Fill == selectedTool, "Rain brush | Only works when the game is running"))
		{
			SetTool(ToolMode::Fill,  false);
		}
	}

	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
	if (ImGui::SliderInt("##Grade", &grade, GradeMin, GradeMax, "%d", ImGuiSliderFlags_AlwaysClamp))
	{
		UpdateGrade();
	}
	ImGui::SetItemTooltip("Brush size");

	if(ImGui::Checkbox("Tunnel wall", &modeIft))
	{
		SetIFT(modeIft);
	}

	ImGui::BeginGroup();
	ImGui::BeginDisabled(Game.Landscape.Mode < C4LSC_Static);
	if(ImGui::CollapsingHeader(LoadResStr(C4ResStrTableKey::IDS_CTL_MATERIAL), ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::BeginListBox("##MaterialBox", {200, 150}))
		{
			const auto addMaterialEntry = [this](const char *const material)
			{
				if (ImGui::Selectable(material, SEqual(material, material)))
				{
					SetMaterial(material);
					ImGui::SetItemDefaultFocus();
				}
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
				if (ImGui::Selectable(texture, SEqual(texture, texture)))
				{
					SetTexture(texture);
					ImGui::SetItemDefaultFocus();
				}
			};

			// atop: valid textures
			const char *texture;
			for (std::int32_t i{0}; (texture = Game.TextureMap.GetTexture(i)); ++i)
			{
				// Current material-texture valid? Always valid for exact mode
				if (Game.TextureMap.GetIndex(material, texture, false) || Game.Landscape.Mode == C4LSC_Exact)
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
					if (!Game.TextureMap.GetIndex(material, texture, false))
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
		createSelectedButton(dynamicImage, false, oldLandscapeMode == C4LSC_Dynamic, "Dynamic: Generated");
	}
	ImGui::SameLine();
	// Static: enable only if map available
	if (createSelectedButton(staticImage, !Game.Landscape.Map, oldLandscapeMode == C4LSC_Static, "Static: Block-wise"))
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
	if (createSelectedButton(exactImage, false, oldLandscapeMode == C4LSC_Exact, "Exact: Pixel-wise | Changes here are not transferable to static mode."))
	{
		SetLandscapeMode(C4LSC_Exact);
	}

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

	if (!opened)
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
