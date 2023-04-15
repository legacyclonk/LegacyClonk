/*
 * LegacyClonk
 *
 * Copyright (c) 2020-2022, The LegacyClonk Team and contributors
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
#elif defined(USE_WINDOWS_RUNTIME)
#include "C4ToastWinRT.h"
#endif

std::unique_ptr<C4ToastSystem> C4ToastSystem::NewInstance()
{
#ifdef USE_LIBNOTIFY
	return std::make_unique<C4ToastSystemLibNotify>();
#elif defined(USE_WINDOWS_RUNTIME)
	return std::make_unique<C4ToastSystemWinRT>();
#else
	return nullptr;
#endif
}

void C4Toast::SetEventHandler(C4ToastEventHandler *const eventHandler)
{
	this->eventHandler = eventHandler;
}
