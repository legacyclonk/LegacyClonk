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

#include "C4EnumInfo.h"
#include "StdBuf.h"

#include <cstddef>
#include <cstdint>

#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
#else
	#define SOCKET int
	#define INVALID_SOCKET (-1)
	#include <arpa/inet.h>
	#include <sys/socket.h>
#endif

#ifndef SOCK_CLOEXEC
#define SOCK_CLOEXEC 0
#endif


class C4Network2HostAddress
{
public:
	enum AddressFamily
	{
		IPv6 = AF_INET6,
		IPv4 = AF_INET,
		UnknownFamily = 0
	};
	enum SpecialAddress
	{
		Loopback, // IPv6 localhost (::1)
		Any,      // IPv6 any-address (::)
		AnyIPv4   // IPv4 any-address (0.0.0.0)
	};

	enum ToStringFlags
	{
		TSF_SkipZoneId = 1,
		TSF_SkipPort = 2
	};

public:
	C4Network2HostAddress() { Clear(); }
	C4Network2HostAddress(const C4Network2HostAddress &other) { SetHost(other); }
	C4Network2HostAddress(const SpecialAddress addr) { SetHost(addr); }
	explicit C4Network2HostAddress(const std::uint32_t addr) { SetHost(addr); }
	C4Network2HostAddress(const StdStrBuf &addr) { SetHost(addr); }
	C4Network2HostAddress(const sockaddr *const addr) { SetHost(addr); }

public:
	AddressFamily GetFamily() const;
	std::size_t GetAddrLen() const;

	void SetScopeId(int scopeId);
	int GetScopeId() const;

	void Clear();
	void SetHost(const sockaddr *addr);
	void SetHost(const C4Network2HostAddress &host);
	void SetHost(SpecialAddress host);
	void SetHost(const StdStrBuf &host, AddressFamily family = UnknownFamily);
	void SetHost(std::uint32_t host);

	C4Network2HostAddress AsIPv6() const; // Convert an IPv4 address to an IPv6-mapped IPv4 address
	bool IsIPv6MappedIPv4() const;
	C4Network2HostAddress AsIPv4() const; // Try to convert an IPv6-mapped IPv4 address to an IPv4 address (returns unchanged address if not possible)

	// General categories
	bool IsNull() const;
	bool IsMulticast() const;
	bool IsLoopback() const;
	bool IsLocal() const; // IPv6 link-local address
	bool IsPrivate() const; // IPv6 ULA or IPv4 private address range

	std::string ToString(int flags = 0) const;

	bool operator ==(const C4Network2HostAddress &rhs) const;

protected:
	// Data
	union
	{
		sockaddr gen;
		sockaddr_in v4;
		sockaddr_in6 v6;
	};
};

class C4Network2EndpointAddress : public C4Network2HostAddress // Host and port
{
public:
	static constexpr std::uint16_t IPPORT_NONE{0};

public:
	C4Network2EndpointAddress() { Clear(); }
	C4Network2EndpointAddress(const C4Network2EndpointAddress &other) : C4Network2HostAddress{} { SetAddress(other); }
	C4Network2EndpointAddress(const C4Network2HostAddress &host, const std::uint16_t port = IPPORT_NONE) : C4Network2HostAddress{host} { SetPort(port); }
	C4Network2EndpointAddress(const C4Network2HostAddress::SpecialAddress addr, const std::uint16_t port = IPPORT_NONE) : C4Network2HostAddress{addr} { SetPort(port); }
	explicit C4Network2EndpointAddress(const StdStrBuf &addr) { SetAddress(addr); }

public:
	std::string ToString(int flags = 0) const;

	void Clear();

	void SetAddress(const sockaddr *addr);
	void SetAddress(const C4Network2EndpointAddress &other);
	void SetAddress(C4Network2HostAddress::SpecialAddress addr, std::uint16_t port = IPPORT_NONE);
	void SetAddress(const C4Network2HostAddress &host, std::uint16_t port = IPPORT_NONE);
	void SetAddress(const StdStrBuf &addr, AddressFamily family = UnknownFamily);
	void SetAddress(const std::string &addr, AddressFamily family = UnknownFamily);

	C4Network2HostAddress GetHost() const { return *this; } // HostAddress copy ctor slices off port information
	C4Network2EndpointAddress AsIPv6() const; // Convert an IPv4 address to an IPv6-mapped IPv4 address
	C4Network2EndpointAddress AsIPv4() const; // Try to convert an IPv6-mapped IPv4 address to an IPv4 address (returns unchanged address if not possible)

	void SetPort(std::uint16_t port);
	void SetDefaultPort(std::uint16_t port); // Set a port only if there is none
	std::uint16_t GetPort() const;

	bool IsNull() const;
	bool IsNullHost() const { return C4Network2HostAddress::IsNull(); }

	// Pointer wrapper to be able to implicitly convert to sockaddr*
	class EndpointAddressPtr;
	const EndpointAddressPtr operator&() const;
	EndpointAddressPtr operator&();

	class EndpointAddressPtr
	{
		C4Network2EndpointAddress *const p;
		friend EndpointAddressPtr C4Network2EndpointAddress::operator&();
		friend const EndpointAddressPtr C4Network2EndpointAddress::operator&() const;
		EndpointAddressPtr(C4Network2EndpointAddress *const p) : p(p) {}

	public:
		const C4Network2EndpointAddress &operator*() const { return *p; }
		C4Network2EndpointAddress &operator*() { return *p; }

		const C4Network2EndpointAddress &operator->() const { return *p; }
		C4Network2EndpointAddress &operator->() { return *p; }

		operator const C4Network2EndpointAddress *() const { return p; }
		operator C4Network2EndpointAddress *() { return p; }

		operator const sockaddr *() const { return &p->gen; }
		operator sockaddr *() { return &p->gen; }

		operator const sockaddr_in *() const { return &p->v4; }
		operator sockaddr_in *() { return &p->v4; }

		operator const sockaddr_in6 *() const { return &p->v6; }
		operator sockaddr_in6 *() { return &p->v6; }
	};

	bool operator==(const C4Network2EndpointAddress &rhs) const;

	// Conversions
	operator sockaddr() const { return gen; }
	operator sockaddr_in() const { assert(gen.sa_family == AF_INET); return v4; }
	operator sockaddr_in6() const { assert(gen.sa_family == AF_INET6); return v6; }

	// StdCompiler
	void CompileFunc(StdCompiler *comp);

	friend class EndpointAddressPtr;
};

enum C4Network2IOProtocol : std::int8_t
{
	P_UDP, P_TCP, P_NONE = -1
};

template<>
struct C4EnumInfo<C4Network2IOProtocol>
{
	static inline constexpr auto data = mkEnumInfo<C4Network2IOProtocol>("P_",
		{
			{ P_UDP, "UDP" },
			{ P_TCP, "TCP" }
		}
	);
};

class C4Network2Address
{
public:
	C4Network2Address()
		: protocol{P_NONE} {}

	C4Network2Address(const C4Network2EndpointAddress addr, const C4Network2IOProtocol protocol)
		: addr{addr.AsIPv4()}, protocol{protocol} {}

	C4Network2Address(const C4Network2Address &addr)
		: addr{addr.GetAddr()}, protocol{addr.GetProtocol()} {}

	void operator=(const C4Network2Address &addr)
	{
		SetAddr(addr.GetAddr());
		SetProtocol(addr.GetProtocol());
	}

	bool operator==(const C4Network2Address &addr) const;

	const C4Network2EndpointAddress &GetAddr() const { return addr; }
	C4Network2EndpointAddress &GetAddr() { return addr; }
	bool isIPNull() const { return addr.IsNull(); }
	std::uint16_t GetPort() const { return addr.GetPort(); }
	C4Network2IOProtocol GetProtocol() const { return protocol; }

	std::string ToString() const;

	void SetAddr(const C4Network2EndpointAddress naddr) { addr = naddr.AsIPv4(); }
	void SetIP(const C4Network2EndpointAddress ip) { addr.SetAddress(ip.AsIPv4()); }
	void SetPort(const std::uint16_t port) { addr.SetPort(port); }
	void SetProtocol(const C4Network2IOProtocol protocol) { this->protocol = protocol; }

	void CompileFunc(StdCompiler *comp);

protected:
	C4Network2EndpointAddress addr;
	C4Network2IOProtocol protocol;
};
