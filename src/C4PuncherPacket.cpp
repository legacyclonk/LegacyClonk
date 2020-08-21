/*
 * LegacyClonk
 *
 * Copyright (c) 2016-2017, The OpenClonk Team and contributors
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
#include "C4PuncherPacket.h"

#include "C4Network2Address.h"

#include <stdexcept>

static constexpr char C4NetpuncherProtocolVersion{1};
// Netpuncher packet header: (1 byte type "Status"), 1 byte version
static constexpr std::size_t HeaderSize{2}, HeaderPSize{1};

void C4NetpuncherID::CompileFunc(StdCompiler *const comp)
{
	comp->Value(mkNamingAdapt(v4, "IPv4", 0u));
	comp->Value(mkNamingAdapt(v6, "IPv6", 0u));
}

std::unique_ptr<C4NetpuncherPacket> C4NetpuncherPacket::Construct(const C4NetIOPacket &pkt)
{
	if (!pkt.getPData() || *pkt.getPData() != C4NetpuncherProtocolVersion) return nullptr;
	try
	{
		switch (pkt.getStatus())
		{
			case PID_Puncher_AssID: return uptr{new C4NetpuncherPacketAssID{pkt}};
			case PID_Puncher_SReq:  return uptr{new C4NetpuncherPacketSReq{pkt}};
			case PID_Puncher_CReq:  return uptr{new C4NetpuncherPacketCReq{pkt}};
			case PID_Puncher_IDReq: return uptr{new C4NetpuncherPacketIDReq{pkt}};
			default: return nullptr;
		}
	}
	catch (const StdCompiler::Exception &)
	{
		return nullptr;
	}
	catch (const std::runtime_error &)
	{
		return nullptr;
	}
}

C4NetIOPacket C4NetpuncherPacket::PackTo(const C4NetIO::addr_t &addr) const
{
	C4NetIOPacket pkt;
	pkt.SetAddr(addr);
	StdBuf content{PackInto()};
	const auto &type = GetType();
	pkt.New(sizeof(type) + sizeof(C4NetpuncherProtocolVersion) + content.getSize());
	std::size_t offset{0};
	pkt.Write(&type, sizeof(type), offset);
	offset += sizeof(type);
	pkt.Write(&C4NetpuncherProtocolVersion, sizeof(C4NetpuncherProtocolVersion), offset);
	offset += sizeof(C4NetpuncherProtocolVersion);
	pkt.Write(content, offset);
	return pkt;
}

C4NetpuncherPacketCReq::C4NetpuncherPacketCReq(const C4NetIOPacket &pkt)
{
	if (pkt.getPSize() < HeaderPSize + 2 + 16) throw std::runtime_error{"invalid size"};
	const std::uint16_t port{*getBufPtr<std::uint16_t>(pkt, HeaderSize)};
	addr.SetAddress(C4NetIO::addr_t::Any, port);
	std::memcpy(&static_cast<sockaddr_in6 *>(&addr)->sin6_addr, getBufPtr<char>(pkt, HeaderSize + sizeof(port)), 16);
}

StdBuf C4NetpuncherPacketCReq::PackInto() const
{
	StdBuf buf;
	const auto sin6 = static_cast<sockaddr_in6>(addr.AsIPv6());
	const auto port = addr.GetPort();
	buf.New(sizeof(port) + sizeof(sin6.sin6_addr));
	std::size_t offset{0};
	buf.Write(&port, sizeof(port), offset);
	offset += sizeof(port);
	buf.Write(&sin6.sin6_addr, sizeof(sin6.sin6_addr), offset);
	static_assert(sizeof(sin6.sin6_addr) == 16, "expected sin6_addr to be 16 bytes");
	return buf;
}

template<C4NetpuncherPacketType Type>
C4NetpuncherPacketID<Type>::C4NetpuncherPacketID(const C4NetIOPacket &pkt)
{
	if (pkt.getPSize() < HeaderPSize + sizeof(id)) throw std::runtime_error{"invalid size"};
	id = *getBufPtr<CID>(pkt, HeaderSize);
}

template<C4NetpuncherPacketType Type>
StdBuf C4NetpuncherPacketID<Type>::PackInto() const
{
	StdBuf buf;
	const auto &id = GetID();
	buf.New(sizeof(id));
	buf.Write(&id, sizeof(id));
	return buf;
}
