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

#include "C4Windows.h"

#include <functional>

#include <unknwn.h>
#include <winrt/base.h>

template<typename Func, typename... Args>
decltype(auto) MapHResultError(Func &&func, Args &&...args)
{
	try
	{
		return std::invoke(func, std::forward<Args>(args)...);
	}
	catch (const winrt::hresult_error &e)
	{
		throw std::runtime_error{winrt::to_string(e.message()).c_str()};
	}
}
