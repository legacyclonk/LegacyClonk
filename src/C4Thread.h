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

#ifdef _WIN32
#include "C4Com.h"
#endif

#include <string_view>
#include <thread>

namespace C4Thread
{
	void SetCurrentThreadName(std::string_view name);

	struct CreateOptions
	{
		std::string_view Name;

#ifdef _WIN32
		winrt::apartment_type ApartmentType{winrt::apartment_type::multi_threaded};
#endif
	};

	template<typename Func, typename... Args>
	std::thread Create(CreateOptions options, Func &&func, Args &&...args)
	{
		return std::thread([](const CreateOptions &options, Func &&func, Args &&...args) mutable
		{
			SetCurrentThreadName(options.Name);

#ifdef _WIN32
			const C4Com com{options.ApartmentType};
#endif

			std::move(func)(std::forward<Args>(args)...);
		},
		std::move(options), std::move(func), std::forward<Args>(args)...);
	}
}
