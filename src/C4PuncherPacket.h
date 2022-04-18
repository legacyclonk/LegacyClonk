/*
 * LegacyClonk
 *
 * Copyright (c) 2016-2017, The OpenClonk Team and contributors
 * Copyright (c) 2019-2020, The LegacyClonk Team and contributors
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

/* I'm cooking up a seperate solution for the netpuncher because using C4Packet2.cpp would require introducing half a ton of stubs, and it seems like overkill */

#include "C4NetIO.h"
#include <memory>

enum C4NetpuncherPacketType : char
{
	PID_Puncher_AssID = 0x51, // Puncher announcing ID to host
	PID_Puncher_SReq  = 0x52, // Client requesting to be served with punching (for an ID)
	PID_Puncher_CReq  = 0x53, // Puncher requesting clients to punch (towards an address)
	PID_Puncher_IDReq = 0x54, // Host requesting an ID
	// Extend this with exchanging ICE parameters, some day?
};

struct C4NetpuncherID
{
	using value = std::uint32_t;

	value v4 = 0, v6 = 0;

	void CompileFunc(StdCompiler *comp);
	bool operator==(const C4NetpuncherID &other) const { return v4 == other.v4 && v6 == other.v6; }
};

class C4NetpuncherPacket
{
public:
	using uptr = std::unique_ptr<C4NetpuncherPacket>;
	static std::unique_ptr<C4NetpuncherPacket> Construct(const C4NetIOPacket &pkt);
	virtual ~C4NetpuncherPacket() = default;
	virtual C4NetpuncherPacketType GetType() const = 0;
	C4NetIOPacket PackTo(const C4NetIO::addr_t &) const;

protected:
	virtual StdBuf PackInto() const = 0;
	using CID = C4NetpuncherID::value;
};

class C4NetpuncherPacketIDReq : public C4NetpuncherPacket
{
public:
	C4NetpuncherPacketIDReq() = default;
	C4NetpuncherPacketIDReq(const C4NetIOPacket &pkt) {}
	C4NetpuncherPacketType GetType() const override final { return PID_Puncher_IDReq; }

private:
	StdBuf PackInto() const override { return StdBuf{}; }
};

template<C4NetpuncherPacketType Type>
class C4NetpuncherPacketID : public C4NetpuncherPacket
{
public:
	C4NetpuncherPacketType GetType() const override final { return Type; }
	CID GetID() const { return id; }
	virtual ~C4NetpuncherPacketID() = default;

protected:
	C4NetpuncherPacketID(const C4NetIOPacket &pkt);
	C4NetpuncherPacketID(const CID id) : id{id} {}

private:
	CID id;
	StdBuf PackInto() const override;
};

#pragma push_macro("PIDC")
#undef PIDC
#define PIDC(n) \
	class C4NetpuncherPacket##n : public C4NetpuncherPacketID<PID_Puncher_##n> \
	{ \
	public: \
		explicit C4NetpuncherPacket##n(const C4NetIOPacket &p) : C4NetpuncherPacketID<PID_Puncher_##n>{p} {} \
		explicit C4NetpuncherPacket##n(const CID id) : C4NetpuncherPacketID<PID_Puncher_##n>{id} {} \
	}
PIDC(AssID); PIDC(SReq);
#pragma pop_macro("PIDC")

class C4NetpuncherPacketCReq : public C4NetpuncherPacket
{
public:
	explicit C4NetpuncherPacketCReq(const C4NetIOPacket &pkt);
	explicit C4NetpuncherPacketCReq(const C4NetIO::addr_t &addr) : addr{addr} {}
	C4NetpuncherPacketType GetType() const override final { return PID_Puncher_CReq; }
	const C4NetIO::addr_t &GetAddr() { return addr; }

private:
	C4NetIO::addr_t addr;
	StdBuf PackInto() const override;
};
