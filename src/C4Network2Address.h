/*
 * LegacyClonk
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013-2017, The OpenClonk Team and contributors
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

#pragma once

#include "C4Network2IO.h"

class C4Network2Address
{
public:
	C4Network2Address()
		: protocol{P_NONE} {}

	C4Network2Address(const C4NetIO::addr_t addr, const C4Network2IOProtocol protocol)
		: addr{addr.AsIPv4()}, protocol{protocol} {}

	C4Network2Address(const C4Network2Address &addr)
		: addr{addr.GetAddr()}, protocol{addr.GetProtocol()} {}

	void operator=(const C4Network2Address &addr)
	{
		SetAddr(addr.GetAddr());
		SetProtocol(addr.GetProtocol());
	}

	bool operator==(const C4Network2Address &addr) const;

	const C4NetIO::addr_t &GetAddr() const { return addr; }
	C4NetIO::addr_t &GetAddr() { return addr; }
	bool isIPNull() const { return addr.IsNull(); }
	std::uint16_t GetPort() const { return addr.GetPort(); }
	C4Network2IOProtocol GetProtocol() const { return protocol; }

	StdStrBuf toString() const;

	void SetAddr(const C4NetIO::addr_t naddr) { addr = naddr.AsIPv4(); }
	void SetIP(const C4NetIO::addr_t ip) { addr.SetAddress(ip.AsIPv4()); }
	void SetPort(const std::uint16_t port) { addr.SetPort(port); }
	void SetProtocol(const C4Network2IOProtocol protocol) { this->protocol = protocol; }

	void CompileFunc(StdCompiler *comp);

protected:
	C4NetIO::addr_t addr;
	C4Network2IOProtocol protocol;
};
