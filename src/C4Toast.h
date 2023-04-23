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

#pragma once

#include "C4ToastEventHandler.h"

#include <cstdint>
#include <memory>
#include <string_view>

class C4Toast;

class C4ToastSystem
{
public:
	virtual ~C4ToastSystem() = default;

public:
	virtual std::unique_ptr<C4Toast> CreateToast() = 0;

public:
	static std::unique_ptr<C4ToastSystem> NewInstance();
};

class C4Toast
{
public:
	virtual ~C4Toast() = default;

public:
	virtual void AddAction(std::string_view action) = 0;
	virtual void SetEventHandler(C4ToastEventHandler *eventHandler);
	virtual void SetExpiration(std::uint32_t expiration) = 0;
	virtual void SetText(std::string_view text) = 0;
	virtual void SetTitle(std::string_view title) = 0;

	virtual void Show() = 0;
	virtual void Hide() = 0;

protected:
	C4ToastEventHandler *eventHandler;
};
