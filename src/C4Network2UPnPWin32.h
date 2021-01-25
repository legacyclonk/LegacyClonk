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

#include "C4Network2UPnP.h"
#include "C4Windows.h"

#include <set>

#include <natupnp.h>

class C4Network2UPnPImplWin32 : public C4Network2UPnPImpl
{
public:
	C4Network2UPnPImplWin32();

public:
	void AddMapping(C4Network2IOProtocol protocol, std::uint16_t internalPort, std::uint16_t externalPort) override;
	void ClearMappings() override;

private:
	IStaticPortMappingCollection *mappings{nullptr};
	std::set<IStaticPortMapping *> addedMappings;
};
