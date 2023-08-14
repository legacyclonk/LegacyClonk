#pragma once

#include "C4Facet.h"
#include "C4Gui.h"
#include "C4GuiEdit.h"
#include "C4GuiResource.h"
#include "C4MouseControl.h"
#include "C4NumberParsing.h"
#include "StdApp.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>

/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
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

namespace C4GUI
{
template <std::integral T>
class SpinBox : public Edit
{
	using limits = std::numeric_limits<T>;

	static constexpr std::int32_t ArrowHeight{8};
	static constexpr std::int32_t ArrowWidth{13};
	static constexpr std::int32_t ArrowMarginTop{2};
	static constexpr std::int32_t ArrowMarginRight{1};
	static constexpr std::int32_t ArrowMarginLeft{2};

public:
	SpinBox(const C4Rect &bounds, const bool focusEdit = false, const T minimum = limits::min(), const T maximum = limits::max()) : Edit{bounds, focusEdit}, minimum{minimum}, maximum{maximum}
	{
		UpdateSize();

		UpdateMaxText();
		SetValue(T{}, false);

		C4CustomKey::Priority keyPrio = focusEdit ? C4CustomKey::PRIO_FocusCtrl : C4CustomKey::PRIO_Ctrl;
		keyUp = std::make_unique<C4KeyBinding>(C4KeyCodeEx{K_UP}, "GUINumberEditUp", KEYSCOPE_Gui, new ControlKeyCB{*this, &SpinBox::OffsetValueCallback<+1>}, keyPrio);
		keyDown = std::make_unique<C4KeyBinding>(C4KeyCodeEx{K_DOWN}, "GUINumberEditDown", KEYSCOPE_Gui, new ControlKeyCB{*this, &SpinBox::OffsetValueCallback<-1>}, keyPrio);
		keyPageUp = std::make_unique<C4KeyBinding>(C4KeyCodeEx{K_PAGEUP}, "GUINumberEditPageUp", KEYSCOPE_Gui, new ControlKeyCB{*this, &SpinBox::OffsetValueCallback<+10>}, keyPrio);
		keyPageDown = std::make_unique<C4KeyBinding>(C4KeyCodeEx{K_PAGEDOWN}, "GUINumberEditPageDown", KEYSCOPE_Gui, new ControlKeyCB{*this, &SpinBox::OffsetValueCallback<-10>}, keyPrio);
	}

	void SetMinimum(const T newMinimum)
	{
		minimum = newMinimum;
		UpdateMaxText();
	}

	void SetMaximum(const T newMaximum)
	{
		maximum = newMaximum;
		UpdateMaxText();
	}

	T GetValue()
	{
		const auto result = [this]
		{
			try
			{
				const std::string_view text{GetText()};
				if (text.empty())
				{
					return T{};
				}
				return ParseNumber<T>(text);
			}
			catch (const NumberRangeError<T> &e)
			{
				return e.AtRangeLimit;
			}
		}();
		return std::clamp(result, minimum, maximum);
	}

	void SetValue(const T value, const bool user)
	{
		const auto clamped = std::clamp(value, minimum, maximum);
		SetText(std::to_string(clamped), user);
	}

protected:
	void OnTextChange() override
	{
		bool changed{false};
		std::string text{GetText()};
		for (std::size_t i = (text.starts_with('-') && minimum < 0) ? 1 : 0; i < text.size();)
		{
			const auto c = text[i];
			if (c < '0' || c > '9')
			{
				text.erase(i, 1);
				if (iCursorPos >= i)
				{
					--iCursorPos;
				}
				changed = true;
				continue;
			}
			++i;
		}
		if (changed)
		{
			const auto cursorPos = iCursorPos;
			SetText(text.c_str(), false);
			iCursorPos = cursorPos;
		}
	}

	InputResult OnFinishInput([[maybe_unused]] const bool pasting, [[maybe_unused]] const bool pastingMore) override
	{
		SetValue(GetValue(), true);
		return IR_None;
	}

	void MouseInput(CMouse &mouse, const std::int32_t button, const std::int32_t x, const std::int32_t y, const std::uint32_t keyParam) override
	{
		switch (button)
		{
			case C4MC_Button_Wheel:
			{
				const auto delta = static_cast<short>(keyParam >> 16);
				OffsetValue(delta < 0 ? -1 : +1, true);
				return;
			}
			case C4MC_Button_LeftDown:
			case C4MC_Button_LeftDouble:
				if (arrowsRect.Wdt <= 0 || !Inside(x, arrowsRect.x - rcBounds.x, rcBounds.Wdt))
				{
					break;
				}
				if (mouse.pDragElement)
				{
					break;
				}
				mouse.pDragElement = this;
				[[fallthrough]];
			case C4MC_Button_LeftUp:
			{
				const auto isDownButton = y > arrowsRect.GetMiddleY() - rcBounds.y;
				const auto prevPressed = upButtonPressed || downButtonPressed;
				if (button == C4MC_Button_LeftUp)
				{
					upButtonPressed = false;
					downButtonPressed = false;
				}
				else
				{
					OffsetValue(isDownButton ? -1 : +1, true);
					(isDownButton ? downButtonPressed : upButtonPressed) = true;
				}
				if ((upButtonPressed || downButtonPressed) != prevPressed)
				{
					GUISound("ArrowHit");
				}
				return;
			}
		}
		Edit::MouseInput(mouse, button, x, y, keyParam);
	}

	void DoDragging(CMouse &mouse, const std::int32_t x, const std::int32_t y, const std::uint32_t keyParam) override
	{
		if (!upButtonPressed && !downButtonPressed)
		{
			Edit::DoDragging(mouse, x, y, keyParam);
		}
	}

	void StopDragging(CMouse &mouse, const std::int32_t x, const std::int32_t y, const std::uint32_t keyParam) override
	{
		if (upButtonPressed || downButtonPressed)
		{
			MouseInput(mouse, C4MC_Button_LeftUp, x, y, keyParam);
		}
		else
		{
			Edit::StopDragging(mouse, x, y, keyParam);
		}
	}

	void DrawElement(C4FacetEx &cgo) override
	{
		Edit::DrawElement(cgo);
		if (arrowsRect.Wdt > 0)
		{
			static C4DrawTransform flipVerticalTransform = []{
				C4DrawTransform t;
				t.Set(1, 0, 0, 0, -1, 0, 0, 0, 1);
				return t;
			}();

			C4Facet& arrow = GetRes()->fctSpinBoxArrow;

			const auto x0 = cgo.TargetX + arrowsRect.x;
			const auto y0 = cgo.TargetY + arrowsRect.y;
			arrow.DrawT(cgo.Surface, x0 + upButtonPressed, - (y0 + upButtonPressed + arrow.Hgt), 0, 0, &flipVerticalTransform);
			arrow.Draw(cgo.Surface, x0 + downButtonPressed, y0 + arrowsRect.Hgt - arrow.Hgt + downButtonPressed);
		}
	}

	void UpdateSize() override
	{
		if (ResizeToIdealWidth())
		{
			return;
		}

		// reserve space for up-/down arrows
		auto& clientWidth = rcClientRect.Wdt;
		if (clientWidth > 30)
		{
			clientWidth -= ArrowWidth + ArrowMarginLeft + ArrowMarginRight;
			arrowsRect = {rcClientRect.x + clientWidth + ArrowMarginLeft, rcClientRect.y + ArrowMarginTop, ArrowWidth, rcClientRect.Hgt - ArrowMarginTop * 2};
		}
		else
		{
			arrowsRect = {0, 0, 0, 0};
		}
	}

	bool ResizeToIdealWidth()
	{
		const auto digits = std::max(GetNumberLength(minimum), GetNumberLength(maximum));
		const auto idealWidth = pFont->GetTextWidth(std::string(digits, '0').c_str(), false) + C4GUI_ScrollArrowWdt + 2;
		if (idealWidth < rcBounds.Wdt)
		{
			rcBounds.Wdt = idealWidth;
			SetBounds(rcBounds);
			return true;
		}

		return false;
	}

private:
	template <std::make_signed_t<T> change>
	bool OffsetValueCallback()
	{
		OffsetValue(change, true);
		return true;
	}

	void OffsetValue(const std::make_signed_t<T> change, const bool user)
	{
		const auto oldValue = GetValue();
		if (change < 0 && oldValue < limits::min() - change)
		{
			SetValue(limits::min(), user);
		}
		else if (change > 0 && oldValue > limits::max() - change)
		{
			SetValue(limits::max(), user);
		}
		else
		{
			SetValue(oldValue + change, user);
		}
		OnTextChange();
	}

	void UpdateMaxText()
	{
		SetMaxText(std::max(GetNumberLength(minimum), GetNumberLength(maximum)));
	}

	static unsigned short GetNumberLength(T number)
	{
		if (number < 0)
		{
			// avoid overflow
			if (number == limits::min())
			{
				number += 1;
			}
			number = -number;
		}

		if (number == 0)
		{
			return 1;
		}
		return checked_cast<unsigned short>(std::lround(std::log10(number))) + 1 + std::signed_integral<T>;
	}

	C4Rect arrowsRect;

	std::unique_ptr<C4KeyBinding> keyUp;
	std::unique_ptr<C4KeyBinding> keyDown;
	std::unique_ptr<C4KeyBinding> keyPageUp;
	std::unique_ptr<C4KeyBinding> keyPageDown;

	T minimum;
	T maximum;

	bool upButtonPressed{false};
	bool downButtonPressed{false};
};
}
