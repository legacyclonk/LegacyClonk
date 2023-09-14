/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2005, Sven2
 * Copyright (c) 2017-2021, The LegacyClonk Team and contributors
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

#include "C4StartupOptionsAdvancedConfigDialog.h"
#include "C4Config.h"
#include "C4Gui.h"
#include "C4GuiEdit.h"
#include "C4GuiListBox.h"
#include "C4GuiSpinBox.h"
#include "C4GuiResource.h"
#include "C4GuiTabular.h"
#include "StdCompiler.h"
#include "StdResStr2.h"

#include <algorithm>
#include <format>
#include <ranges>
#include <stdexcept>

using ConfigDialog = C4StartupOptionsAdvancedConfigDialog;

namespace
{
template <typename Impl>
class StdCompilerConfigGuiBase : public StdCompiler
{
public:
	StdCompilerConfigGuiBase(ConfigDialog *dialog) : dialog{dialog} {}

	bool hasNaming() override { return true; }

	void QWord(int64_t &rInt) override { return HandleSettingInternal(rInt); }
	void QWord(uint64_t &rInt) override { return HandleSettingInternal(rInt); }
	void DWord(int32_t &rInt) override { return HandleSettingInternal(rInt); }
	void DWord(uint32_t &rInt) override { return HandleSettingInternal(rInt); }
	void Word(int16_t &rShort) override { return HandleSettingInternal(rShort); }
	void Word(uint16_t &rShort) override { return HandleSettingInternal(rShort); }
	void Byte(int8_t &rByte) override { return HandleSettingInternal(rByte); }
	void Byte(uint8_t &rByte) override { return HandleSettingInternal(rByte); }
	void Boolean(bool &rBool) override { return HandleSettingInternal(rBool); }
	void Character(char &rChar) override { return HandleSettingInternal(rChar); }

	void String(char *szString, size_t iMaxLength, RawCompileType type = RCT_Escaped) override { return HandleSettingInternal(szString, iMaxLength, type); }
	void String(std::string &str, RawCompileType type = RCT_Escaped) override { return HandleSettingInternal(str, type); }
	void Raw(void *pData, size_t iSize, RawCompileType type = RCT_Escaped) override { return HandleSettingInternal(pData, iSize, type); }

	bool Default(const char *name) override { return false; }

	NameGuard Name(const char *name) override
	{
		lastName = name;
		if (++level == 1)
		{
			dialog->ChangeSection(name);
		}
		assert(level <= 2);
		return StdCompiler::Name(name);
	}
	void NameEnd(bool breaking = false) override
	{
		assert(level > 0);
		--level;
		return StdCompiler::NameEnd(breaking);
	}
protected:
	ConfigDialog *dialog;

private:
	auto &self() { return static_cast<Impl&>(*this); }

	template <typename T, typename... Args>
	void HandleSettingInternal(T &setting, Args &&... args)
	{
		try
		{
			self().HandleSetting(lastName, setting, std::forward<Args>(args)...);
		}
		catch (const std::out_of_range &e)
		{
			excNotFound(e.what());
		}
	}

	std::string lastName;
	std::size_t level{0};
};

class StdCompilerConfigGuiRead : public StdCompilerConfigGuiBase<StdCompilerConfigGuiRead>
{
	using Base = StdCompilerConfigGuiBase<StdCompilerConfigGuiRead>;

public:
	using Base::Base;

	bool isCompiler() override { return false; }

	template <typename T>
	void HandleSetting(std::string_view name, T &number)
	{
		dialog->AddSetting(name, number);
	}

	void HandleSetting(std::string_view name, std::string &str, RawCompileType type)
	{
		dialog->AddSetting(name, str);
	}

	void HandleSetting(std::string_view name, char *str, std::size_t maxLength, RawCompileType type)
	{
		dialog->AddStringSetting(name, str, maxLength);
	}

	void HandleSetting(std::string_view name, void *data, std::size_t length, RawCompileType type)
	{
		dialog->AddText(name, "Raw data");
	}
};

class StdCompilerConfigGuiWrite : public StdCompilerConfigGuiBase<StdCompilerConfigGuiWrite>
{
	using Base = StdCompilerConfigGuiBase<StdCompilerConfigGuiWrite>;

public:
	using Base::Base;

	bool isCompiler() override { return true; }

	template <typename T>
	void HandleSetting(std::string_view name, T &number)
	{
		dialog->GetSetting(name, number);
	}

	void HandleSetting(std::string_view name, std::string &str, RawCompileType type)
	{
		std::string val;
		dialog->GetSetting(name, val);
		if (type == RCT_Idtf || type == RCT_IdtfAllowEmpty || type == RCT_ID)
		{
			const auto wrongChar = std::ranges::find_if_not(val, IsIdentifierChar);
			if (wrongChar != std::ranges::end(val))
			{
				val.erase(wrongChar, val.end());
			}
		}

		if (type == RCT_Idtf || type == RCT_ID)
		{
			if (val.empty())
			{
				excNotFound("String");
			}
		}

		str = std::move(val);
	}

	void HandleSetting(std::string_view name, char *str, std::size_t maxLength, RawCompileType type)
	{
		std::string val;
		HandleSetting(name, val, type);

		if (val.size() > maxLength)
		{
			std::strncpy(str, val.c_str(), maxLength);
		}
		else
		{
			std::strcpy(str, val.c_str());
		}
	}

	void HandleSetting(std::string_view name, void *data, std::size_t length,  RawCompileType type)
	{

	}
};

struct BlockedSetting {
	std::string_view section;
	std::string_view name;

	std::strong_ordering operator<=>(const BlockedSetting &other) const = default;
};

constexpr BlockedSetting blockedSettings[]
{
	{"General", "Version"},
	{"General", "ConfigResetSafety"},
};

class SectionSheet : public C4GUI::Tabular::Sheet
{
public:
	SectionSheet(const char *title, const C4Rect &bounds, Element *mainContent) : Sheet{title, bounds}, mainContent{mainContent} {}

	void RemoveElement(Element *element) override
	{
		if (element == mainContent)
		{
			mainContent = nullptr;
		}
	}

	void UpdateSize() override
	{
		if (mainContent)
		{
			mainContent->SetBounds(GetContainedClientRect());
		}
	}

private:
	Element *mainContent;
};
}

struct ConfigDialog::Section
{
	std::string name;
	C4GUI::Tabular::Sheet *tab;
	C4GUI::ListBox *list;
	std::unordered_map<std::string, Setting, string_hash, std::equal_to<>> settings;
};

ConfigDialog::C4StartupOptionsAdvancedConfigDialog(std::int32_t width, std::int32_t height) : C4GUI::Dialog{width, height, LoadResStr("IDS_DLG_ADVANCED_SETTINGS"), false}
{
	StdCompilerConfigGuiRead comp{this};
	saveButton = new C4GUI::CallbackButton<ConfigDialog>{LoadResStr("IDS_BTN_SAVE"), {0, 0, 1, 1}, &ConfigDialog::OnSave, this};
	cancelButton = new C4GUI::CallbackButton<ConfigDialog>{LoadResStr("IDS_BTN_CANCEL"), {0, 0, 1, 1}, &ConfigDialog::OnAbort, this};
	tabs = new C4GUI::Tabular{C4Rect{0, 0, 0, 0}, C4GUI::Tabular::tbLeft};
	AddElement(tabs);
	AddElement(saveButton);
	AddElement(cancelButton);
	UpdateSize();
	comp.Decompile(Config);
}

const char *ConfigDialog::GetID()
{
	return "OptionsAdvancedConfigDialog";
}

void ConfigDialog::UpdateSize()
{
	Dialog::UpdateSize();

	static constexpr std::int32_t buttonMargin{10};
	static constexpr std::int32_t buttonWidth{200};

	C4GUI::ComponentAligner ca{GetContainedClientRect(), 0, 0};
	C4GUI::ComponentAligner caBottom{ca.GetFromBottom(C4GUI_ButtonHgt + 2 * buttonMargin), 0, buttonMargin};
	tabs->SetBounds(ca.GetAll());
	if (caBottom.GetWidth() > 2 * buttonWidth + 4 * buttonMargin)
	{
		caBottom.GetFromLeft(tabs->GetTabButtonWidth());
	}
	C4GUI::ComponentAligner caButtons{caBottom.GetCentered(std::min(600, caBottom.GetWidth()), caBottom.GetHeight()), 0, buttonMargin};
	const auto actualButtonWidth = std::min(buttonWidth, caButtons.GetInnerWidth() / 2);
	saveButton->SetBounds(caButtons.GetFromLeft(actualButtonWidth));
	cancelButton->SetBounds(caButtons.GetFromRight(actualButtonWidth));
}

void ConfigDialog::AddText(std::string_view name, std::string_view value) const
{
	const auto label = new C4GUI::Label{value, 0, 0};
	AddSettingInternal(name, label);
}

void ConfigDialog::AddSettingInternal(std::string_view name, C4GUI::Element *control) const
{
	auto &setting = currentSection->settings.emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(name, control)).first->second;
	setting.SetBounds({0, 0, currentSection->list->GetItemWidth(), setting.GetHeight()});
	currentSection->list->AddElement(&setting);
}

void ConfigDialog::ChangeSection(const char *name)
{
	if (sections.contains(name))
	{
		currentSection = &sections.find(name)->second;
	}
	else
	{
		auto listBox = new C4GUI::ListBox{tabs->GetContainedClientRect()};
		const auto tab = new SectionSheet{name, tabs->GetContainedClientRect(), listBox};
		tabs->AddCustomSheet(tab);
		currentSection = &sections.emplace(name, Section{name, tab, listBox}).first->second;
		tab->AddElement(listBox);
		UpdateSize();
	}
}

bool ConfigDialog::ShowModal(C4GUI::Screen *screen)
{
	auto warningDialog = new C4GUI::MessageDialog{LoadResStr("IDS_MSG_ADVANCED_SETTINGS_WARNING"), LoadResStr("IDS_DLG_WARNING"), C4GUI::MessageDialog::btnOKAbort, C4GUI::Ico_None, C4GUI::MessageDialog::dsRegular, nullptr, true};
	if (!screen->ShowModalDlg(warningDialog))
	{
		return false;
	}
	return screen->ShowModalDlg(new C4StartupOptionsAdvancedConfigDialog{screen->GetWidth() * 3 / 4, screen->GetHeight() * 3 / 4});
}

bool ConfigDialog::OnEnter()
{
	return false;
}

void ConfigDialog::UserClose(bool ok)
{
	// don’t close on enter
	if (!ok)
	{
		Dialog::UserClose(ok);
	}
}

ConfigDialog::Setting::Setting(std::string_view label, C4GUI::Element *control) : label{std::format("{}: ", label).c_str(), 0, 0}, control{control}
{
	AddElement(&this->label);
	AddElement(control);
	UpdateSize(true);
}

void ConfigDialog::Setting::UpdateSize()
{
	Window::UpdateSize();

	return UpdateSize(false);
}

void ConfigDialog::Setting::UpdateSize(bool adjustHeight)
{
	if (adjustHeight)
	{
		SetBounds({rcBounds.x, rcBounds.y, rcBounds.Wdt, control->GetHeight() + GetMarginTop() + GetMarginBottom()});
	}
	else
	{
		C4GUI::ComponentAligner ca{GetContainedClientRect(), Margin, 0};
		const auto labelWidth = C4GUI::GetRes()->TextFont.GetTextWidth(label.GetText(), false);
		label.SetBounds(ca.GetFromLeft(labelWidth));
		control->SetBounds(ca.GetAll());
	}
}

C4GUI::Element *ConfigDialog::Setting::GetControl() const { return control; }

template <std::integral T>
void ConfigDialog::AddSetting(std::string_view name, T &setting) const
{
	if (IsBlocked(name))
	{
		return AddText(name, std::format("{}", setting));
	}

	const auto edit = new C4GUI::SpinBox<T>{C4Rect{0, 0, 0, C4GUI::Edit::GetDefaultEditHeight()}};
	edit->SetValue(setting, false);
	return AddSettingInternal(name, edit);
}

void ConfigDialog::AddSetting(std::string_view name, bool &setting) const
{
	if (IsBlocked(name))
	{
		return AddText(name, std::format("{}", setting));
	}

	const auto edit = new C4GUI::CheckBox{C4Rect{0, 0, 0, C4GUI::Edit::GetDefaultEditHeight()}, "", setting};
	return AddSettingInternal(name, edit);
}

void ConfigDialog::AddSetting(std::string_view name, char &setting) const
{
	if (IsBlocked(name))
	{
		return AddText(name, std::format("{}", setting));
	}

	char text[]{setting};
	return AddStringSetting(name, text, 1);
}

void ConfigDialog::AddStringSetting(std::string_view name, char *text, std::size_t maxLength) const
{
	if (IsBlocked(name))
	{
		return AddText(name, text);
	}

	std::string str{text};
	return AddSetting(name, str);
}

void ConfigDialog::AddSetting(std::string_view name, std::string &text, std::size_t maxLength) const
{
	if (IsBlocked(name))
	{
		AddText(name, text);
	}

	const auto edit = new C4GUI::Edit{C4Rect{0, 0, 0, C4GUI::Edit::GetDefaultEditHeight()}};
	edit->SetText(text, false);
	if (maxLength != std::string::npos)
	{
		edit->SetMaxText(maxLength);
	}
	return AddSettingInternal(name, edit);
}

template <std::integral T>
void ConfigDialog::GetSetting(std::string_view name, T &setting) const
{
	if (IsBlocked(name))
	{
		return;
	}

	setting = GetSettingInternal<C4GUI::SpinBox<T>>(name)->GetValue();
}

void ConfigDialog::GetSetting(std::string_view name, bool &setting) const
{
	if (IsBlocked(name))
	{
		return;
	}

	setting = GetSettingInternal<C4GUI::CheckBox>(name)->GetChecked();
}

void ConfigDialog::GetSetting(std::string_view name, char &setting) const
{
	if (IsBlocked(name))
	{
		return;
	}

	const auto text = GetSettingInternal<C4GUI::Edit>(name)->GetText();
	setting = text[0];
}

void ConfigDialog::GetSetting(std::string_view name, std::string &text) const
{
	if (IsBlocked(name))
	{
		return;
	}

	text = GetSettingInternal<C4GUI::Edit>(name)->GetText();
}

template <typename T>
T *ConfigDialog::GetSettingInternal(std::string_view name) const
{
	const auto it = currentSection->settings.find(name);
	if (it == currentSection->settings.end())
	{
		throw std::out_of_range{std::format("Unknown setting: [{}]/{}", currentSection->name, name)};
	}

	const auto control = dynamic_cast<T *>(it->second.GetControl());
	if (!control)
	{
		throw std::out_of_range{std::format("Setting type mismatch: [{}]/{}", currentSection->name, name)};
	}
	return control;
}

void ConfigDialog::OnSave(C4GUI::Control *)
{
	StdCompilerConfigGuiWrite comp{this};
	comp.Compile(Config);
	Close(true);
}

void ConfigDialog::OnAbort(C4GUI::Control *)
{
	Close(false);
}

bool ConfigDialog::IsBlocked(std::string_view name) const
{
	return std::ranges::find(blockedSettings, BlockedSetting{currentSection->name, name}) != std::ranges::end(blockedSettings);
}
