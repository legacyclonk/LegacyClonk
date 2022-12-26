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

#include "C4Toast.h"
#include "C4WinRT.h"

#include <winrt/Windows.UI.Notifications.h>

class C4ToastSystemWinRT : public C4ToastSystem
{
public:
	C4ToastSystemWinRT();
	virtual ~C4ToastSystemWinRT() = default;
};

class C4ToastImplWinRT : public C4ToastImpl
{
private:
	class EventHandler : public winrt::implements<EventHandler, winrt::Windows::Foundation::IInspectable>
	{
	public:
		EventHandler(winrt::Windows::UI::Notifications::ToastNotification &notification, C4ToastEventHandler *const &eventHandler);

	private:
		C4ToastEventHandler *const &eventHandler;
	};

public:
	C4ToastImplWinRT();

protected:
	void AddAction(std::string_view action) override;
	void SetExpiration(uint32_t expiration) override;
	void SetText(std::string_view text) override;
	void SetTitle(std::string_view title) override;
	void Show() override;
	void Hide() override;

private:
	void SetTextNode(std::uint32_t index, std::wstring &&text);

private:
	winrt::Windows::UI::Notifications::ToastNotification notification;
	winrt::Windows::UI::Notifications::ToastNotifier notifier;
	winrt::com_ptr<EventHandler> eventHandler;
};
