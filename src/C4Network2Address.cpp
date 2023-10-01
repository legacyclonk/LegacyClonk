/*
 * LegacyClonk
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2010-2016, The OpenClonk Team and contributors
 * Copyright (c) 2019-2021, The LegacyClonk Team and contributors
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
#include "StdAdaptors.h"
#include "StdCompiler.h"

namespace
{
	class AddrInfo
	{
	public:
		AddrInfo(const char *const node, const char *const service,
			const struct addrinfo *const hints)
		{
			if (::getaddrinfo(node, service, hints, &addrs) != 0) addrs = nullptr;
		}

		~AddrInfo() { if (addrs) ::freeaddrinfo(addrs); }
		explicit operator bool() const { return addrs != nullptr; }
		addrinfo *GetAddrs() const { return addrs; }

	private:
		addrinfo *addrs;
	};
}

// *** C4Network2HostAddress
void C4Network2HostAddress::Clear()
{
	v6.sin6_family = AF_INET6;
	v6.sin6_flowinfo = 0;
	v6.sin6_scope_id = 0;
	v6.sin6_addr = {};
}

// *** C4Network2EndpointAddress
const C4Network2EndpointAddress::EndpointAddressPtr C4Network2EndpointAddress::operator&() const { return EndpointAddressPtr{const_cast<C4Network2EndpointAddress *>(this)}; }
C4Network2EndpointAddress::EndpointAddressPtr C4Network2EndpointAddress::operator&() { return EndpointAddressPtr{this}; }

void C4Network2EndpointAddress::Clear()
{
	C4Network2HostAddress::Clear();
	SetPort(IPPORT_NONE);
}

void C4Network2HostAddress::SetHost(const C4Network2HostAddress &other)
{
	SetHost(&other.gen);
}

bool C4Network2HostAddress::IsMulticast() const
{
	if (gen.sa_family == AF_INET6)
		return IN6_IS_ADDR_MULTICAST(&v6.sin6_addr) != 0;
	if (gen.sa_family == AF_INET)
		return (ntohl(v4.sin_addr.s_addr) >> 24) == 239;
	return false;
}

bool C4Network2HostAddress::IsLoopback() const
{
	if (gen.sa_family == AF_INET6)
		return IN6_IS_ADDR_LOOPBACK(&v6.sin6_addr) != 0;
	if (gen.sa_family == AF_INET)
		return (ntohl(v4.sin_addr.s_addr) >> 24) == 127;
	return false;
}

bool C4Network2HostAddress::IsLocal() const
{
	if (gen.sa_family == AF_INET6)
		return IN6_IS_ADDR_LINKLOCAL(&v6.sin6_addr) != 0;
	if (gen.sa_family == AF_INET)
	{
		const std::uint32_t addr{ntohl(v4.sin_addr.s_addr)};
		return addr >> 24 == 169 && ((addr >> 16) & 0xff) == 254;
	}
	return false;
}

bool C4Network2HostAddress::IsPrivate() const
{
	// IPv6 unique local address
	if (gen.sa_family == AF_INET6)
		return (v6.sin6_addr.s6_addr[0] & 0xfe) == 0xfc;
	if (gen.sa_family == AF_INET)
	{
		const std::uint32_t addr{ntohl(v4.sin_addr.s_addr)};
		const std::uint32_t s{(addr >> 16) & 0xff};
		switch (addr >> 24)
		{
		case  10: return true;
		case 172: return s >= 16 && s <= 31;
		case 192: return s == 168;
		}
	}
	return false;
}

void C4Network2HostAddress::SetScopeId(const int scopeId)
{
	if (gen.sa_family != AF_INET6) return;
	if (IN6_IS_ADDR_LINKLOCAL(&v6.sin6_addr) != 0)
		v6.sin6_scope_id = scopeId;
}

int C4Network2HostAddress::GetScopeId() const
{
	if (gen.sa_family == AF_INET6)
		return v6.sin6_scope_id;
	return 0;
}

C4Network2HostAddress C4Network2HostAddress::AsIPv6() const
{
	static constexpr unsigned char v6_mapped_v4_prefix[12]{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff };

	C4Network2HostAddress nrv{*this};
	switch (gen.sa_family)
	{
	case AF_INET6:
		// That was easy
		break;
	case AF_INET:
		nrv.v6.sin6_family = AF_INET6;
		std::memcpy(&nrv.v6.sin6_addr.s6_addr[0], v6_mapped_v4_prefix, sizeof(v6_mapped_v4_prefix));
		std::memcpy(&nrv.v6.sin6_addr.s6_addr[sizeof(v6_mapped_v4_prefix)], &v4.sin_addr, sizeof(v4.sin_addr));
		nrv.v6.sin6_flowinfo = 0;
		nrv.v6.sin6_scope_id = 0;
		break;
	default:
		assert(!"Shouldn't reach this");
		break;
	}
	return nrv;
}

bool C4Network2HostAddress::IsIPv6MappedIPv4() const
{
	return gen.sa_family == AF_INET6 && IN6_IS_ADDR_V4MAPPED(&v6.sin6_addr);
}

C4Network2HostAddress C4Network2HostAddress::AsIPv4() const
{
	C4Network2HostAddress nrv{*this};
	if (IsIPv6MappedIPv4())
	{
		nrv.v4.sin_family = AF_INET;
		std::memcpy(&nrv.v4.sin_addr, &v6.sin6_addr.s6_addr[12], sizeof(v4.sin_addr));
	}
	return nrv;
}

C4Network2EndpointAddress C4Network2EndpointAddress::AsIPv6() const
{
	return {C4Network2HostAddress::AsIPv6(), GetPort()};
}

C4Network2EndpointAddress C4Network2EndpointAddress::AsIPv4() const
{
	return {C4Network2HostAddress::AsIPv4(), GetPort()};
}

void C4Network2HostAddress::SetHost(const sockaddr *const addr)
{
	// Copy all but port number
	if (addr->sa_family == AF_INET6)
	{
		const auto addr6 = reinterpret_cast<const sockaddr_in6 *>(addr);
		v6.sin6_family = addr6->sin6_family;
		v6.sin6_flowinfo = addr6->sin6_flowinfo;
		v6.sin6_addr = addr6->sin6_addr;
		v6.sin6_scope_id = addr6->sin6_scope_id;
	}
	else if (addr->sa_family == AF_INET)
	{
		const auto addr4 = reinterpret_cast<const sockaddr_in *>(addr);
		v4.sin_family = addr4->sin_family;
		v4.sin_addr.s_addr = addr4->sin_addr.s_addr;
		std::memset(&v4.sin_zero, 0, sizeof(v4.sin_zero));
	}
}

void C4Network2EndpointAddress::SetAddress(const sockaddr *const addr)
{
	switch (addr->sa_family)
	{
	case AF_INET: std::memcpy(&v4, addr, sizeof(v4)); break;
	case AF_INET6: std::memcpy(&v6, addr, sizeof(v6)); break;
	default:
		assert(!"Unexpected address family");
		std::memcpy(&gen, addr, sizeof(gen));
		break;
	}
}

void C4Network2HostAddress::SetHost(const SpecialAddress addr)
{
	switch (addr)
	{
	case Any:
		v6.sin6_family = AF_INET6;
		std::memset(&v6.sin6_addr, 0, sizeof(v6.sin6_addr));
		v6.sin6_flowinfo = 0;
		v6.sin6_scope_id = 0;
		break;
	case AnyIPv4:
		v4.sin_family = AF_INET;
		v4.sin_addr.s_addr = 0;
		std::memset(&v4.sin_zero, 0, sizeof(v4.sin_zero));
		break;
	case Loopback:
		v6.sin6_family = AF_INET6;
		std::memset(&v6.sin6_addr, 0, sizeof(v6.sin6_addr));
		v6.sin6_addr.s6_addr[15] = 1;
		v6.sin6_flowinfo = 0;
		v6.sin6_scope_id = 0;
		break;
	}
}

void C4Network2HostAddress::SetHost(const std::uint32_t v4addr)
{
	v4.sin_family = AF_INET;
	v4.sin_addr.s_addr = v4addr;
	std::memset(&v4.sin_zero, 0, sizeof(v4.sin_zero));
}

void C4Network2HostAddress::SetHost(const StdStrBuf &addr, const AddressFamily family)
{
	addrinfo hints{};
	hints.ai_family = family;

	if (const AddrInfo ai{addr.getData(), nullptr, &hints})
	{
		SetHost(ai.GetAddrs()->ai_addr);
	}
}

void C4Network2EndpointAddress::SetAddress(const StdStrBuf &addr, const AddressFamily family)
{
	Clear();

	if (addr.isNull()) return;

	const auto begin = addr.getData();
	const auto end = begin + addr.getLength();

	auto ab = begin;
	auto ae = end;

	auto pb = end;
	const auto pe = end;

	bool isIPv6{false};

	// If addr begins with [, it's an IPv6 address
	if (ab[0] == '[')
	{
		++ab; // Skip bracket
		auto cbracket = std::find(ab, ae, ']');
		if (cbracket == ae)
			// No closing bracket found: invalid
			return;
		ae = cbracket++;
		if (cbracket != end && cbracket[0] == ':')
		{
			// Port number given
			pb = ++cbracket;
			if (pb == end)
				// Trailing colon: invalid
				return;
		}
		isIPv6 = true;
	}
	// If there's more than 1 colon in the address, it's IPv6
	else if (std::count(ab, ae, ':') > 1)
	{
		isIPv6 = true;
	}
	// It's probably not IPv6, but look for a port specification
	else
	{
		const auto colon = std::find(ab, ae, ':');
		if (colon != ae)
		{
			ae = colon;
			pb = colon + 1;
			if (pb == end)
				// Trailing colon: invalid
				return;
		}
	}

	addrinfo hints{};
	hints.ai_family = family;
	if (const AddrInfo ai{std::string{ab, ae}.c_str(),
		(pb != end ? std::string{pb, pe}.c_str() : nullptr), &hints})
	{
		SetAddress(ai.GetAddrs()->ai_addr);
	}
}

void C4Network2EndpointAddress::SetAddress(const C4Network2EndpointAddress &addr)
{
	SetHost(addr);
	SetPort(addr.GetPort());
}

void C4Network2EndpointAddress::SetAddress(
	const C4Network2HostAddress::SpecialAddress host, const std::uint16_t port)
{
	SetHost(host);
	SetPort(port);
}

void C4Network2EndpointAddress::SetAddress(const C4Network2HostAddress &host, const std::uint16_t port)
{
	SetHost(host);
	SetPort(port);
}

bool C4Network2EndpointAddress::IsNull() const
{
	return IsNullHost() && GetPort() == IPPORT_NONE;
}

bool C4Network2HostAddress::IsNull() const
{
	switch (gen.sa_family)
	{
	case AF_INET: return v4.sin_addr.s_addr == 0;
	case AF_INET6: return IN6_IS_ADDR_UNSPECIFIED(&v6.sin6_addr);
	}
	assert(!"Shouldn't reach this");
	return false;
}

C4Network2HostAddress::AddressFamily C4Network2HostAddress::GetFamily() const
{
	return
		gen.sa_family == AF_INET ? IPv4 :
		gen.sa_family == AF_INET6 ? IPv6 :
		UnknownFamily;
}

std::size_t C4Network2HostAddress::GetAddrLen() const
{
	return GetFamily() == IPv4 ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
}

void C4Network2EndpointAddress::SetPort(const std::uint16_t port)
{
	switch (gen.sa_family)
	{
	case AF_INET: v4.sin_port = htons(port); break;
	case AF_INET6: v6.sin6_port = htons(port); break;
	default: assert(!"Shouldn't reach this"); break;
	}
}

void C4Network2EndpointAddress::SetDefaultPort(const std::uint16_t port)
{
	if (GetPort() == IPPORT_NONE)
		SetPort(port);
}

std::uint16_t C4Network2EndpointAddress::GetPort() const
{
	switch (gen.sa_family)
	{
	case AF_INET: return ntohs(v4.sin_port);
	case AF_INET6: return ntohs(v6.sin6_port);
	}
	assert(!"Shouldn't reach this");
	return IPPORT_NONE;
}

bool C4Network2HostAddress::operator==(const C4Network2HostAddress &rhs) const
{
	// Check for IPv4-mapped IPv6 addresses.
	if (gen.sa_family != rhs.gen.sa_family)
		return AsIPv6() == rhs.AsIPv6();
	if (gen.sa_family == AF_INET)
		return v4.sin_addr.s_addr == rhs.v4.sin_addr.s_addr;
	if (gen.sa_family == AF_INET6)
		return std::memcmp(&v6.sin6_addr, &rhs.v6.sin6_addr, sizeof(v6.sin6_addr)) == 0 &&
			v6.sin6_scope_id == rhs.v6.sin6_scope_id;
	assert(!"Shouldn't reach this");
	return false;
}

bool C4Network2EndpointAddress::operator==(const C4Network2EndpointAddress &rhs) const
{
	if (!C4Network2HostAddress::operator==(rhs)) return false;
	if (gen.sa_family == AF_INET)
	{
		return v4.sin_port == rhs.v4.sin_port;
	}
	else if (gen.sa_family == AF_INET6)
	{
		return v6.sin6_port == rhs.v6.sin6_port &&
			v6.sin6_scope_id == rhs.v6.sin6_scope_id;
	}
	assert(!"Shouldn't reach this");
	return false;
}

StdStrBuf C4Network2HostAddress::ToString(const int flags) const
{
	if (IsIPv6MappedIPv4()) return AsIPv4().ToString(flags);

	if (gen.sa_family == AF_INET6 && v6.sin6_scope_id != 0 && (flags & TSF_SkipZoneId))
	{
		auto addr = *this;
		addr.v6.sin6_scope_id = 0;
		return addr.ToString(flags);
	}

	char buf[INET6_ADDRSTRLEN];
	if (::getnameinfo(&gen, GetAddrLen(), buf, sizeof(buf), nullptr, 0, NI_NUMERICHOST) != 0)
		return {};

	return StdStrBuf{buf, true};
}

StdStrBuf C4Network2EndpointAddress::ToString(const int flags) const
{
	if (IsIPv6MappedIPv4()) return AsIPv4().ToString(flags);

	if (flags & TSF_SkipPort)
		return C4Network2HostAddress::ToString(flags);

	switch (GetFamily())
	{
	case IPv4: return FormatString("%s:%d", C4Network2HostAddress::ToString(flags).getData(), GetPort());
	case IPv6: return FormatString("[%s]:%d", C4Network2HostAddress::ToString(flags).getData(), GetPort());
	case UnknownFamily: ; // fallthrough
	}
	assert(!"Shouldn't reach this");
	return {};
}

void C4Network2EndpointAddress::CompileFunc(StdCompiler *const comp)
{
	if (!comp->isCompiler())
	{
		StdStrBuf val{ToString(TSF_SkipZoneId)};
		comp->Value(val);
	}
	else
	{
		StdStrBuf val;
		comp->Value(val);
		SetAddress(val);
	}
}

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

	comp->Value(mkDefaultAdapt(addr, C4Network2EndpointAddress{}));
}

StdStrBuf C4Network2Address::toString() const
{
	switch (protocol)
	{
	case P_UDP: return FormatString("UDP:%s", addr.ToString().getData());
	case P_TCP: return FormatString("TCP:%s", addr.ToString().getData());
	case P_NONE: ; // fallthrough
	}
	return StdStrBuf("INVALID");
}

bool C4Network2Address::operator==(const C4Network2Address &addr2) const
{
	return protocol == addr2.GetProtocol() && addr == addr2.GetAddr();
}
