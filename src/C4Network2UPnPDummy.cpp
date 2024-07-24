/*
 * LegacyClonk
 *
 * Copyright (c) 2012-2016, The OpenClonk Team and contributors
 * Copyright (c) 2024, The LegacyClonk Team and contributors
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

#include "C4Network2UPnP.h"

struct C4Network2UPnP::Impl {};

C4Network2UPnP::C4Network2UPnP() = default;
C4Network2UPnP::~C4Network2UPnP() noexcept = default;

void C4Network2UPnP::AddMapping(C4Network2IOProtocol, std::uint16_t, std::uint16_t) {}
void C4Network2UPnP::ClearMappings() {}
