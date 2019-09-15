/*
 * LegacyClonk
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2010-2016, The OpenClonk Team and contributors
 * Copyright (c) 2019, The LegacyClonk Team and contributors
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

#include "C4Include.h"
#ifndef _WIN32
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#endif

#include "C4Network2Address.h"

// *** C4Network2Address

void C4Network2Address::CompileFunc(StdCompiler *const comp)
{
	// Clear
	if (comp->isCompiler())
	{
		addr.Clear();
	}

	// Write protocol
	const StdEnumEntry<C4Network2IOProtocol> Protocols[]{
		{ "UDP",   P_UDP },
		{ "TCP",   P_TCP }
	};
	comp->Value(mkEnumAdaptT<std::uint8_t>(protocol, Protocols));
	comp->Separator(StdCompiler::SEP_PART2); // ':'

	comp->Value(mkDefaultAdapt(addr, C4NetIO::addr_t{}));
}

StdStrBuf C4Network2Address::toString() const
{
	switch (protocol)
	{
	case P_UDP: return FormatString("UDP:%s", addr.ToString().getData());
	case P_TCP: return FormatString("TCP:%s", addr.ToString().getData());
	default:    return StdStrBuf("INVALID");
	}
}

bool C4Network2Address::operator==(const C4Network2Address &addr2) const
{
	return protocol == addr2.GetProtocol() && addr == addr2.GetAddr();
}
