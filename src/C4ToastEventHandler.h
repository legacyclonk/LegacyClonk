/*
 * LegacyClonk
 *
 * Copyright (c) 2020-2023, The LegacyClonk Team and contributors
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

#include <string_view>

class C4ToastEventHandler
{
public:
	virtual ~C4ToastEventHandler() = default;

public:
	virtual void Activated() {}
	virtual void Dismissed() {}
	virtual void Failed() { }
	virtual void OnAction(std::string_view action) { (void) action; }
};
