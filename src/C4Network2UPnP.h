/*
 * LegacyClonk
 *
 * Copyright (c) 2022, The LegacyClonk Team and contributors
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

#include "C4Network2IO.h"

#include <memory>

class C4Network2UPnP
{
private:
	struct Impl;

public:
	C4Network2UPnP();
	~C4Network2UPnP() noexcept;

public:
	void AddMapping(C4Network2IOProtocol protocol, std::uint16_t internalPort, std::uint16_t externalPort);
	void ClearMappings();

private:
	const std::unique_ptr<Impl> impl;
};
