/*
 * LegacyClonk
 *
 * Copyright (c) 2020, The LegacyClonk Team and contributors
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

#include "C4Log.h"
#include "C4ToastWinToastLib.h"
#include "C4Version.h"
#include "StdStringEncodingConverter.h"

#include <functional>
#include <memory>
#include <stdexcept>

using namespace WinToastLib;

C4ToastSystemWinToastLib::C4ToastSystemWinToastLib()
{
	if (!WinToast::isCompatible())
	{
		throw std::runtime_error{"WinToastLib is not compatible with this system"};
	}

	const StdStringEncodingConverter converter;

	const std::wstring appName{converter.WinAcpToUtf16(C4ENGINENAME)};
	WinToast::instance()->setAppName(appName);
	WinToast::instance()->setAppUserModelId(appName);

	if (WinToast::WinToastError error; !WinToast::instance()->initialize(&error))
	{
		throw std::runtime_error{"Failed to initialize WinToastLib: " + converter.Utf16ToWinAcp(WinToast::strerror(error).c_str())};
	}
}

C4ToastImplWinToastLib::~C4ToastImplWinToastLib()
{
	C4ToastImplWinToastLib::SetEventHandler(nullptr);
}

void C4ToastImplWinToastLib::AddAction(const std::string_view action)
{
	toastTemplate.addAction(StdStringEncodingConverter{}.WinAcpToUtf16(action.data()));
	actions.emplace_back(action);
}

void C4ToastImplWinToastLib::SetExpiration(const std::uint32_t expiration)
{
	toastTemplate.setExpiration(expiration);
}

void C4ToastImplWinToastLib::SetEventHandler(C4ToastEventHandler *const eventHandler)
{
	C4ToastImpl::SetEventHandler(eventHandler);
	if (eventHandler)
	{
		eventHandlerGuard = std::shared_ptr<C4ToastEventHandler>{eventHandler, [](auto) {}};
	}
	else
	{
		eventHandlerGuard.reset();
	}
}

void C4ToastImplWinToastLib::SetText(const std::string_view text)
{
	toastTemplate.setSecondLine(StdStringEncodingConverter{}.WinAcpToUtf16(text.data()));
}

void C4ToastImplWinToastLib::SetTitle(const std::string_view text)
{
	toastTemplate.setFirstLine(StdStringEncodingConverter{}.WinAcpToUtf16(text.data()));
}

namespace
{
	class C4WinToastEventHandler final : public IWinToastHandler
	{
	public:
		C4WinToastEventHandler(std::weak_ptr<C4ToastEventHandler> eventHandler, std::vector<std::string> actions) : eventHandler{eventHandler}, actions{actions} {}

	public:
		void toastActivated() const override
		{
			if (const auto lock = eventHandler.lock(); lock)
			{
				lock->Activated();
			}
		}

		void toastActivated(int actionIndex) const override
		{
			if (const auto lock = eventHandler.lock(); lock)
			{
				lock->OnAction(actions.at(actionIndex)); // yes, this throws out_of_range if the index is invalid
			}
		}

		void toastDismissed(WinToastDismissalReason) const override
		{
			if (const auto lock = eventHandler.lock(); lock)
			{
				lock->Dismissed();
			}
		}

		void toastFailed() const override
		{
			if (const auto lock = eventHandler.lock(); lock)
			{
				lock->Failed();
			}
		}

	private:
		std::weak_ptr<C4ToastEventHandler> eventHandler;
		std::vector<std::string> actions;
	};
}

void C4ToastImplWinToastLib::Show()
{
	Hide();
	id = std::max(WinToast::instance()->showToast(toastTemplate, new C4WinToastEventHandler{eventHandlerGuard, actions}), 0LL);
}

void C4ToastImplWinToastLib::Hide()
{
	if (id)
	{
		WinToast::instance()->hideToast(id);
		id = 0;
	}
}
