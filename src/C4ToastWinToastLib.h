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

#pragma once

#include "C4Toast.h"
#include "C4Windows.h"

#include "wintoastlib.h"

#include <vector>

class C4ToastSystemWinToastLib : public C4ToastSystem
{
public:
	C4ToastSystemWinToastLib();
};

class C4ToastImplWinToastLib : public C4ToastImpl
{
public:
	~C4ToastImplWinToastLib();

public:
	void AddAction(std::string_view action) override;
	void SetExpiration(std::uint32_t expiration) override;
	void SetText(std::string_view text) override;
	void SetTitle(std::string_view title) override;

	void Show() override;
	void Hide() override;

private:
	WinToastLib::WinToastTemplate toastTemplate{WinToastLib::WinToastTemplate::Text02};
	std::vector<std::string> actions;
	int64_t id{0};
};
