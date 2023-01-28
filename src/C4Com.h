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

#include "C4WinRT.h"

#include <utility>

class C4Com
{
public:
	C4Com() = default;

	explicit C4Com(const winrt::apartment_type apartmentType)
	{
		MapHResultError(&winrt::init_apartment, apartmentType);
		initialized = true;
	}

	~C4Com()
	{
		if (initialized)
		{
			winrt::uninit_apartment();
		}
	}

	C4Com(const C4Com &) = delete;
	C4Com &operator=(const C4Com &) = delete;

	C4Com(C4Com &&other) noexcept
		: initialized{std::exchange(other.initialized, false)}
	{
	}

	C4Com &operator=(C4Com &&other) noexcept
	{
		initialized = std::exchange(other.initialized, false);
		return *this;
	}

private:
	bool initialized{false};
};
