/*
 * LegacyClonk
 *
 * Copyright (c) 2012-2016, The OpenClonk Team and contributors
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

#include "C4Network2IO.h"

#include <cstdint>
#include <memory>

class C4Network2UPnPImpl
{
public:
	virtual ~C4Network2UPnPImpl() = default;

public:
	virtual void AddMapping(C4Network2IOProtocol protocol, std::uint16_t internalPort, std::uint16_t externalPort) {}
	virtual void ClearMappings() {}
};

class C4Network2UPnP
{
public:
	C4Network2UPnP();
	~C4Network2UPnP();
	C4Network2UPnP(const C4Network2UPnP &) = delete;
	C4Network2UPnP &operator=(const C4Network2UPnP &) = delete;

public:
	void AddMapping(C4Network2IOProtocol protocol, std::uint16_t internalPort, std::uint16_t externalPort) { return impl->AddMapping(protocol, internalPort, externalPort); }
	void ClearMappings() { return impl->ClearMappings(); }

private:
	const std::unique_ptr<C4Network2UPnPImpl> impl;
};
