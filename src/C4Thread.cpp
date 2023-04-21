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

#include "C4Thread.h"

#ifdef _WIN32
#include "C4Windows.h"
#include "StdStringEncodingConverter.h"
#else
#include <pthread.h>
#endif

#include <string>

void C4Thread::SetCurrentThreadName(const std::string_view name)
{
#ifdef _WIN32
	static auto *const setThreadDescription = reinterpret_cast<HRESULT(__stdcall *)(HANDLE, PCWSTR)>(GetProcAddress(GetModuleHandle("KernelBase.dll"), "SetThreadDescription"));

	if (setThreadDescription)
	{
		setThreadDescription(GetCurrentThread(), StdStringEncodingConverter::WinAcpToUtf16(name).c_str());
	}
#elif defined(__linux__)
	pthread_setname_np(pthread_self(), std::string{name, 0, 15}.c_str());
#elif defined(__APPLE__)
	pthread_setname_np(std::string{name}.c_str());
#endif
}
