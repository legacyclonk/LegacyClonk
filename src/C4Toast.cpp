/*
 * LegacyClonk
 *
 * Copyright (c) 2020-2021, The LegacyClonk Team and contributors
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
#include "C4Toast.h"

#ifdef USE_LIBNOTIFY
#include "C4ToastLibNotify.h"
#elif defined(_WIN32)
#include "C4ToastWinToastLib.h"
#endif

C4ToastSystem *C4ToastSystem::NewInstance()
{
	try
	{
#ifdef USE_LIBNOTIFY
		return new C4ToastSystemLibNotify{};
#elif defined(_WIN32)
		return new C4ToastSystemWinToastLib{};
#else
		return nullptr;
#endif
	}
	catch (const std::runtime_error &e)
	{
		LogSilentF("%s", e.what());
	}

	return nullptr;
}

void C4ToastImpl::SetEventHandler(C4ToastEventHandler *const eventHandler)
{
	this->eventHandler = eventHandler;
}

C4Toast::C4Toast() : impl{
#ifdef USE_LIBNOTIFY
						 new C4ToastImplLibNotify{}
#elif defined(_WIN32)
						 new C4ToastImplWinToastLib{}
#else
						 new C4ToastImpl{}
#endif
						 }
{
}

void C4Toast::ShowToast(std::string_view title, std::string_view text, C4ToastEventHandler *const eventHandler, uint32_t expiration, std::initializer_list<std::string_view> actions)
{
	C4Toast toast;
	toast.SetTitle(title);
	toast.SetText(text);
	toast.SetEventHandler(eventHandler);
	toast.SetExpiration(expiration);

	for (const auto &action : actions)
	{
		toast.AddAction(action);
	}

	toast.Show();
}
