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

#pragma once

#include "C4Surface.h"

#ifdef WITH_DEVELOPER_MODE
#endif

#define C4TLS_MatSky "Sky"

enum class ToolMode
{
	Brush = 0,
	Line = 1,
	Rect = 2,
	Fill = 3,
	Picker
};

class C4ToolsDlg
{
public:


	static constexpr auto GradeMin = 1;
	static constexpr auto GradeMax = 50;
	static constexpr auto GradeDefault = 5;

public:
	C4ToolsDlg();
	~C4ToolsDlg();

public:
	bool Active;

	ToolMode Tool, SelectedTool;
	std::int32_t Grade;
	bool ModeIFT;
	char Material[C4M_MaxName + 1];
	char Texture[C4M_MaxName + 1];

protected:
	static constexpr auto PreviewWidth = 64;
	static constexpr auto PreviewHeight = 64;
	C4Surface surfacePreview;

public:
	void Default();
	void Clear();
	//bool PopTextures();
	//bool PopMaterial();
	bool ChangeGrade(int32_t iChange);
	void UpdatePreview();
	void Open();
	void UpdateGrade();
	void SetTool(ToolMode tool, bool temp);
	bool ToggleTool();
	bool SetLandscapeMode(int32_t iMode, bool fThroughControl = false);
	void SetIFT(bool fIFT);
	bool ToggleIFT() { SetIFT(!ModeIFT); return true; }
	void SetTexture(const char *szTexture);
	void SetMaterial(const char *szMaterial);
	void SetAlternateTool();
	void ResetAlternateTool();
	void Draw();

	std::uint32_t DynamicImage;
	std::uint32_t StaticImage;
	std::uint32_t ExactImage;

	std::uint32_t BrushImage;
	std::uint32_t LineImage;
	std::uint32_t RectImage;
	std::uint32_t PickerImage;
	std::uint32_t FillImage;

protected:
	void AssertValidTexture();
};
