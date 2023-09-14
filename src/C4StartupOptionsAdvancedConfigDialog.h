/*
 * LegacyClonk
 *
 * Copyright (c) 2023, The LegacyClonk Team and contributors
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

#include "C4Gui.h"

#include <unordered_map>

class C4StartupOptionsAdvancedConfigDialog : public C4GUI::Dialog
{
public:
	C4StartupOptionsAdvancedConfigDialog(std::int32_t width, std::int32_t height);
	const char *GetID() override;
	void UpdateSize() override;
	template <std::integral T>
	void AddSetting(std::string_view name, T &setting) const;
	void AddSetting(std::string_view name, bool &setting) const;
	void AddSetting(std::string_view name, char &setting) const;
	void AddSetting(std::string_view name, std::string &text, std::size_t maxLength = std::string::npos) const;
	void AddStringSetting(std::string_view name, char *text, std::size_t maxLength) const;
	template <std::integral T>
	void GetSetting(std::string_view name, T &setting) const;
	void GetSetting(std::string_view name, bool &setting) const;
	void GetSetting(std::string_view name, char &setting) const;
	void GetSetting(std::string_view name, std::string &text) const;
	void AddText(std::string_view name, std::string_view text) const;
	void ChangeSection(const char *name);

	static bool ShowModal(C4GUI::Screen *screen);

protected:
	bool OnEnter() override;
	void UserClose(bool ok) override;

private:
	void AddSettingInternal(std::string_view name, C4GUI::Element *control) const;
	template <typename T>
	T *GetSettingInternal(std::string_view name) const;
	void OnSave(C4GUI::Control *control);
	void OnAbort(C4GUI::Control *control);
	bool IsBlocked(std::string_view name) const;

	C4GUI::Tabular *tabs;
	C4GUI::Button *cancelButton;
	C4GUI::Button *saveButton;

	class Setting : public Window
	{
	public:
		Setting(std::string_view label, C4GUI::Element *control);
		void UpdateSize() override;
		void UpdateSize(bool adjustHeight);
		C4GUI::Element *GetControl() const;

		static inline constexpr int32_t Margin{2};
		int32_t GetMarginTop() override { return Margin; }
		int32_t GetMarginBottom() override { return Margin; }
		int32_t GetMarginLeft() override { return Margin; }
		int32_t GetMarginRight() override { return Margin; }

	private:
		C4GUI::Label label;
		C4GUI::Element *control;
	};

	struct string_hash : std::hash<std::string_view>
	{
		using is_transparent = void;
	};

	struct Section;

	Section *currentSection{nullptr};

	std::unordered_map<std::string, Section, string_hash, std::equal_to<>> sections;
};
