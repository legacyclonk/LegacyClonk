/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2011-2018, The OpenClonk Team and contributors
 * Copyright (c) 2017-2021, The LegacyClonk Team and contributors
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

#include <C4Include.h>
#include <C4Network2Client.h>

#include <C4Log.h>
#include <C4Console.h>
#include <C4Network2.h>
#include <C4Network2IO.h>
#include <C4Network2Stats.h>
#include <C4GameLobby.h> // fullscreen network lobby

#include <algorithm>
#include <chrono>
#include <thread>
#include <utility>

// *** C4Network2Client

C4Network2Client::C4Network2Client(std::shared_ptr<spdlog::logger> logger, C4Client *pClient)
	: logger{std::move(logger)},
	pClient(pClient),
	eStatus(NCS_Ready),
	iLastActivity(0),
	pMsgConn(nullptr), pDataConn(nullptr),
	iNextConnAttempt(0),
	pNext(nullptr), pParent(nullptr), pstatPing(nullptr) {}

C4Network2Client::~C4Network2Client()
{
	ClearGraphs();
	if (pMsgConn) { pMsgConn->Close(); pMsgConn->DelRef(); } pMsgConn = nullptr;
	if (pDataConn) { pDataConn->Close(); pDataConn->DelRef(); } pDataConn = nullptr;
	if (pClient) pClient->UnlinkNetClient();
}

namespace
{
	template<typename Addrs, typename Property, typename Value>
	auto getClientAddress(Addrs &&addresses, Property property, Value &&value)
	{
		return std::find_if(addresses.begin(), addresses.end(), [property, value](const auto &ownAddr) { return ownAddr.*property == value; });
	}
}

bool C4Network2Client::hasConn(C4Network2IOConnection *pConn)
{
	return pMsgConn == pConn || pDataConn == pConn;
}

void C4Network2Client::SetMsgConn(C4Network2IOConnection *pConn)
{
	// security
	if (pConn != pMsgConn)
	{
		if (pMsgConn) pMsgConn->DelRef();
		pMsgConn = pConn;
		pMsgConn->AddRef();
	}
	if (!pDataConn) SetDataConn(pConn);
}

void C4Network2Client::SetDataConn(C4Network2IOConnection *pConn)
{
	// security
	if (pConn != pDataConn)
	{
		if (pDataConn) pDataConn->DelRef();
		pDataConn = pConn;
		pDataConn->AddRef();
	}
	if (!pMsgConn) SetMsgConn(pConn);
}

void C4Network2Client::RemoveConn(C4Network2IOConnection *pConn)
{
	if (pConn == pMsgConn)
	{
		pMsgConn->DelRef(); pMsgConn = nullptr;
	}
	if (pConn == pDataConn)
	{
		pDataConn->DelRef(); pDataConn = nullptr;
	}
	if (pMsgConn && !pDataConn) SetDataConn(pMsgConn);
	if (!pMsgConn && pDataConn) SetMsgConn(pDataConn);
}

void C4Network2Client::CloseConns(const char *szMsg)
{
	C4PacketConnRe Pkt(false, false, szMsg);
	C4Network2IOConnection *pConn;
	while (pConn = pMsgConn)
	{
		// send packet, close
		if (pConn->isOpen())
		{
			pConn->Send(MkC4NetIOPacket(PID_ConnRe, Pkt));
			pConn->Close();
		}
		// remove
		RemoveConn(pConn);
	}
}

bool C4Network2Client::SendMsg(C4NetIOPacket rPkt) const
{
	return getMsgConn() && getMsgConn()->Send(rPkt);
}

bool C4Network2Client::DoConnectAttempt(C4Network2IO *pIO)
{
	// local?
	if (isLocal()) { iNextConnAttempt = 0; return true; }
	// msg and data connected? Nothing to do
	if (getMsgConn() != getDataConn()) { iNextConnAttempt = time(nullptr) + 10; return true; }
	// too early?
	if (iNextConnAttempt && iNextConnAttempt > time(nullptr)) return true;
	// find address to try
	int32_t iBestAddress = -1;
	for (int32_t i = 0; const auto &addr : Addresses)
	{
		// valid address?
		if (!addr.Addr.GetAddr().IsNullHost()
			// no connection for this protocol?
			&& (!pDataConn || addr.Addr.GetProtocol() != pDataConn->getProtocol())
			&& (!pMsgConn || addr.Addr.GetProtocol() != pMsgConn->getProtocol())
			// protocol available?
			&& pIO->getNetIO(addr.Addr.GetProtocol()))
		{
			if (iBestAddress < 0 || addr.ConnectionAttempts < Addresses[iBestAddress].ConnectionAttempts)
			{
				iBestAddress = i;
			}
		}
		++i;
	}
	// too many attempts or nothing found?
	if (iBestAddress < 0 || Addresses[iBestAddress].ConnectionAttempts > C4NetClientConnectAttempts)
	{
		iNextConnAttempt = time(nullptr) + 10; return true;
	}
	// save attempt
	++Addresses[iBestAddress].ConnectionAttempts;
	iNextConnAttempt = time(nullptr) + C4NetClientConnectInterval;
	auto addr = Addresses[iBestAddress].Addr.GetAddr();
	const auto addrProtocol = Addresses[iBestAddress].Addr.GetProtocol();

	// Try TCP simultaneous open if the stars align right
	if (addr.GetFamily() == C4NetIO::addr_t::IPv6 && // Address needs to be IPv6...
		!addr.IsLocal() && !addr.IsPrivate() &&      // ...global unicast...
		addrProtocol == P_TCP && // ...TCP,
		!tcpSimOpenSocket &&                         // there is no previous request,
		pParent->GetLocal()->getID() < getID())      // and make sure that only one client per pair initiates a request.
	{
		DoTCPSimultaneousOpen(pIO, C4Network2Address{});
	}

	const std::set<int> DefaultInterfaceIDs{0};
	const auto &interfaceIDs = addr.IsLocal() ?
		Game.Network.Clients.GetLocal()->getInterfaceIDs() : DefaultInterfaceIDs;
	for (const auto &id : interfaceIDs)
	{
		addr.SetScopeId(id);
		logger->info("Network: connecting client {} on {}...", getName(), addr.ToString());
		if (pIO->Connect(addr, addrProtocol, pClient->getCore()))
			return true;
	}
	return false;
}

bool C4Network2Client::DoTCPSimultaneousOpen(C4Network2IO *const pIO, const C4Network2Address &addr)
{
	if (!pIO->getNetIO(P_TCP)) return false;

	// Did we already bind a socket?
	if (tcpSimOpenSocket)
	{
		logger->info("Network: connecting client {} on {} with TCP simultaneous open...", getName(), addr.GetAddr().ToString());
		return pIO->ConnectWithSocket(addr.GetAddr(), addr.GetProtocol(), pClient->getCore(), std::move(tcpSimOpenSocket));
	}
	else
	{
		// No - bind one, inform peer, and schedule a connection attempt.
		auto NetIOTCP = dynamic_cast<C4NetIOTCP *>(pIO->getNetIO(P_TCP));
		auto bindAddr = pParent->GetLocal()->IPv6AddrFromPuncher;
		// We need to know an address that works.
		if (bindAddr.IsNull()) return false;
		bindAddr.SetPort(0);
		tcpSimOpenSocket = NetIOTCP->Bind(bindAddr);
		if (!tcpSimOpenSocket) return false;
		const auto &boundAddr = tcpSimOpenSocket->GetAddress();
		logger->info("Network: {} TCP simultaneous open request for client {} from {}...",
			(addr.isIPNull() ? "initiating" : "responding to"),
			getName(), boundAddr.ToString());
		// Send address we bound to to the client.
		if (!SendMsg(MkC4NetIOPacket(PID_TCPSimOpen, C4PacketTCPSimOpen{
			pParent->GetLocal()->getID(), C4Network2Address{boundAddr, P_TCP}})))
		{
			return false;
		}
		if (!addr.isIPNull())
		{
			// We need to delay the connection attempt a bit. Unfortunately,
			// waiting for the next tick would usually take way too much time.
			// Instead, we block the main thread for a very short time and hope
			// that noone notices...
			const int ping{getMsgConn()->getLag()};
			std::this_thread::sleep_for(std::chrono::milliseconds(std::min(ping / 2, 10)));
			DoTCPSimultaneousOpen(pIO, addr);
		}
		return true;
	}
}

bool C4Network2Client::hasAddr(const C4Network2Address &addr) const
{
	// Note that the host only knows its own address as 0.0.0.0, so if the real address is being added, that can't be sorted out.
	return getClientAddress(Addresses, &ClientAddress::Addr, addr) != Addresses.end();
}

void C4Network2Client::AddAddrFromPuncher(const C4NetIO::addr_t &addr)
{
	AddAddr(C4Network2Address{addr, P_UDP}, true, true);
	// If the outside port matches the inside port, there is no port translation and the
	// TCP address will probably work as well.
	if (addr.GetPort() != Config.Network.PortUDP)
	{
		auto udpAddr = addr;
		udpAddr.SetPort(Config.Network.PortUDP);
		AddAddr(C4Network2Address{udpAddr, P_UDP}, true, true);
	}
	if (Config.Network.PortTCP > 0)
	{
		auto tcpAddr = addr;
		tcpAddr.SetPort(Config.Network.PortTCP);
		AddAddr(C4Network2Address{tcpAddr, P_TCP}, true, true);
	}
	// Save IPv6 address for TCP simultaneous connect.
	if (addr.GetFamily() == C4NetIO::addr_t::IPv6)
		IPv6AddrFromPuncher = addr;
}

bool C4Network2Client::AddAddr(const C4Network2Address &addr, bool fAnnounce, bool inFront)
{
	if (hasAddr(addr)) return true;

	if (inFront)
	{
		Addresses.emplace(Addresses.begin(), addr);
	}
	else
	{
		Addresses.emplace_back(addr);
	}
	// attempt to use this one
	if (!iNextConnAttempt) iNextConnAttempt = time(nullptr);
	// announce
	if (fAnnounce)
		if (!pParent->BroadcastMsgToConnClients(MkC4NetIOPacket(PID_Addr, C4PacketAddr(getID(), addr))))
			return false;
	// done
	return true;
}

void C4Network2Client::AddLocalAddrs(const std::uint16_t iPortTCP, const std::uint16_t iPortUDP)
{
	C4NetIO::addr_t addr{C4Network2HostAddress::AnyIPv4};

	if (iPortTCP != 0)
	{
		addr.SetPort(iPortTCP);
		AddAddr(C4Network2Address(addr, P_TCP), false);
	}

	if (iPortUDP != 0)
	{
		addr.SetPort(iPortUDP);
		AddAddr(C4Network2Address(addr, P_UDP), false);
	}

	for (const auto &ha : C4NetIO::GetLocalAddresses())
	{
		addr.SetAddress(ha);
		if (iPortTCP != 0)
		{
			addr.SetPort(iPortTCP);
			AddAddr(C4Network2Address(addr, P_TCP), false);
		}

		if (iPortUDP != 0)
		{
			addr.SetPort(iPortUDP);
			AddAddr(C4Network2Address(addr, P_UDP), false);
		}

		if (addr.GetScopeId())
		{
			InterfaceIDs.insert(addr.GetScopeId());
		}
	}
}

void C4Network2Client::SendAddresses(C4Network2IOConnection *pConn)
{
	// send all addresses
	for (const auto &clientAddr : Addresses)
	{
		auto addr = clientAddr.Addr;
		if (addr.GetAddr().GetScopeId() &&
			(!pConn || pConn->getPeerAddr().GetScopeId() != addr.GetAddr().GetScopeId()))
		{
			continue;
		}

		addr.GetAddr().SetScopeId(0);
		const C4NetIOPacket Pkt{MkC4NetIOPacket(PID_Addr, C4PacketAddr{getID(), addr})};
		if (pConn)
			pConn->Send(Pkt);
		else
			pParent->BroadcastMsgToConnClients(Pkt);
	}
}

void C4Network2Client::CreateGraphs()
{
	// del prev
	ClearGraphs();
	// get client color
	static const uint32_t ClientDefColors[] = { 0xff0000, 0x00ff00, 0xffff00, 0x7f7fff, 0xffffff, 0x00ffff, 0xff00ff, 0x7f7f7f, 0xff7f7f, 0x7fff7f, 0x0000ff };
	int32_t iClientColorNum = sizeof(ClientDefColors) / sizeof(uint32_t);
	uint32_t dwClientClr = ClientDefColors[std::max<int32_t>(getID(), 0) % iClientColorNum];
	// create graphs
	pstatPing = new C4TableGraph(C4TableGraph::DefaultBlockLength, Game.pNetworkStatistics ? Game.pNetworkStatistics->SecondCounter : 0);
	pstatPing->SetColorDw(dwClientClr);
	pstatPing->SetTitle(getName());
	// register into stat module
	if (Game.pNetworkStatistics) Game.pNetworkStatistics->statPings.AddGraph(pstatPing);
}

void C4Network2Client::ClearGraphs()
{
	// del all assigned graphs
	if (pstatPing && Game.pNetworkStatistics)
	{
		Game.pNetworkStatistics->statPings.RemoveGraph(pstatPing);
	}
	delete pstatPing;
	pstatPing = nullptr;
}

// *** C4Network2ClientList

C4Network2ClientList::C4Network2ClientList(C4Network2IO *pIO)
	: pFirst(nullptr), pLocal(nullptr), pIO(pIO) {}

C4Network2ClientList::~C4Network2ClientList()
{
	Clear();
}

C4Network2Client *C4Network2ClientList::GetClientByID(int32_t iID) const
{
	for (C4Network2Client *pClient = pFirst; pClient; pClient = pClient->pNext)
		if (pClient->getID() == iID)
			return pClient;
	return nullptr;
}

C4Network2Client *C4Network2ClientList::GetClient(const char *szName) const
{
	for (C4Network2Client *pClient = pFirst; pClient; pClient = pClient->pNext)
		if (SEqual(pClient->getName(), szName))
			return pClient;
	return nullptr;
}

C4Network2Client *C4Network2ClientList::GetClient(C4Network2IOConnection *pConn) const
{
	for (C4Network2Client *pClient = pFirst; pClient; pClient = pClient->pNext)
		if (pClient->hasConn(pConn))
			return pClient;
	return nullptr;
}

C4Network2Client *C4Network2ClientList::GetClient(const C4ClientCore &CCore, int32_t iMaxDiffLevel)
{
	for (C4Network2Client *pClient = pFirst; pClient; pClient = pClient->pNext)
		if (pClient->getCore().getDiffLevel(CCore) <= iMaxDiffLevel)
			return pClient;
	return nullptr;
}

C4Network2Client *C4Network2ClientList::GetHost()
{
	return GetClientByID(C4ClientIDHost);
}

C4Network2Client *C4Network2ClientList::GetNextClient(C4Network2Client *pClient)
{
	return pClient ? pClient->pNext : pFirst;
}

void C4Network2ClientList::Init(std::shared_ptr<spdlog::logger> logger, C4ClientList *pnClientList, bool fnHost)
{
	// save flag
	fHost = fnHost;
	// initialize
	pClientList = pnClientList;
	pClientList->InitNetwork(this);
	// set logger here, because InitNetwork() calls this->Clear()
	this->logger = std::move(logger);
}

C4Network2Client *C4Network2ClientList::RegClient(C4Client *pClient)
{
	// security
	if (pClient->getNetClient())
		return pClient->getNetClient();
	// find insert position
	C4Network2Client *pPos = pFirst, *pLast = nullptr;
	for (; pPos; pLast = pPos, pPos = pPos->getNext())
		if (pPos->getID() > pClient->getID())
			break;
	assert(!pLast || pLast->getID() != pClient->getID());
	// create new client
	C4Network2Client *pNetClient = new C4Network2Client(logger, pClient);
	// add to list
	pNetClient->pNext = pPos;
	(pLast ? pLast->pNext : pFirst) = pNetClient;
	pNetClient->pParent = this;
	// local?
	if (pClient->isLocal())
		pLocal = pNetClient;
	else
		// set auto-accept
		pIO->AddAutoAccept(pClient->getCore());
	// add
	return pNetClient;
}

void C4Network2ClientList::DeleteClient(C4Network2Client *pClient)
{
	// close connections
	pClient->CloseConns("removing client");
	// remove from list
	if (pClient == pFirst)
		pFirst = pClient->getNext();
	else
	{
		C4Network2Client *pPrev;
		for (pPrev = pFirst; pPrev && pPrev->getNext(); pPrev = pPrev->getNext())
			if (pPrev->getNext() == pClient)
				break;
		if (pPrev && pPrev->getNext() == pClient)
			pPrev->pNext = pClient->getNext();
	}
	// remove auto-accept
	pIO->RemoveAutoAccept(pClient->getCore());
	// delete
	delete pClient;
}

void C4Network2ClientList::Clear()
{
	// remove link to main client list
	if (pClientList)
	{
		C4ClientList *poClientList = pClientList;
		pClientList = nullptr;
		poClientList->ClearNetwork();
	}
	// delete clients
	while (pFirst)
	{
		DeleteClient(pFirst);
	}
	pLocal = nullptr;
	logger.reset();
}

bool C4Network2ClientList::BroadcastMsgToConnClients(const C4NetIOPacket &rPkt)
{
	// Send a msg to all clients that are currently directly reachable.

	// lock
	pIO->BeginBroadcast(false);
	// select connections for broadcast
	for (C4Network2Client *pClient = pFirst; pClient; pClient = pClient->getNext())
		if (pClient->isConnected())
			pClient->getMsgConn()->SetBroadcastTarget(true);
	// broadcast
	bool fSuccess = pIO->Broadcast(rPkt);
	// unlock
	pIO->EndBroadcast();
	// finished
	return fSuccess;
}

bool C4Network2ClientList::BroadcastMsgToClients(const C4NetIOPacket &rPkt, bool includeHost)
{
	// Send a msg to all clients, including clients that are not connected to
	// this computer (will get forwarded by host).

	C4PacketFwd Fwd; Fwd.SetListType(true);
	// lock
	pIO->BeginBroadcast(false);
	// select connections for broadcast
	for (C4Network2Client *pClient = pFirst; pClient; pClient = pClient->getNext())
		if (!pClient->isHost() || includeHost)
			if (pClient->isConnected())
			{
				pClient->getMsgConn()->SetBroadcastTarget(true);
				Fwd.AddClient(pClient->getID());
			}
	// broadcast
	bool fSuccess = pIO->Broadcast(rPkt);
	// unlock
	pIO->EndBroadcast();
	// clients: send forward request to host
	if (!fHost)
	{
		Fwd.SetData(rPkt);
		fSuccess &= SendMsgToHost(MkC4NetIOPacket(PID_FwdReq, Fwd));
	}
	return fSuccess;
}

bool C4Network2ClientList::SendMsgToHost(C4NetIOPacket rPkt)
{
	// find host
	C4Network2Client *pHost = GetHost();
	if (!pHost) return false;
	// send message
	if (!pHost->getMsgConn()) return false;
	return pHost->SendMsg(rPkt);
}

bool C4Network2ClientList::SendMsgToClient(int32_t iClient, C4NetIOPacket &&rPkt)
{
	// find client
	C4Network2Client *pClient = GetClientByID(iClient);
	if (!pClient) return false;
	// connected? send directly
	if (pClient->isConnected())
		return pClient->SendMsg(rPkt);
	// forward
	C4PacketFwd Fwd; Fwd.SetListType(false);
	Fwd.AddClient(iClient);
	Fwd.SetData(rPkt);
	return SendMsgToHost(MkC4NetIOPacket(PID_FwdReq, Fwd));
}

void C4Network2ClientList::HandlePacket(char cStatus, const C4PacketBase *pBasePkt, C4Network2IOConnection *pConn)
{
	// find associated client
	C4Network2Client *pClient = GetClient(pConn);
	if (!pClient) return;

#define GETPKT(type, name) \
	assert(pBasePkt); \
	const type &name = static_cast<const type &>(*pBasePkt);

	switch (cStatus)
	{
	case PID_Addr: // address propagation
	{
		GETPKT(C4PacketAddr, rPkt);
		// find client
		pClient = GetClientByID(rPkt.getClientID());
		if (pClient)
		{
			C4Network2Address addr = rPkt.getAddr();
			// IP zero? Set to IP from where the packet came
			if (addr.isIPNull())
			{
				addr.SetIP(pConn->getPeerAddr());
			}
			// add (no announce)
			if (pClient->AddAddr(addr, true))
				// new address? Try to connect
				pClient->DoConnectAttempt(pIO);
		}
	}
	break;

	case PID_TCPSimOpen:
	{
		GETPKT(C4PacketTCPSimOpen, rPkt);
		if (const auto &client = GetClientByID(rPkt.GetClientID()))
		{
			client->DoTCPSimultaneousOpen(pIO, rPkt.GetAddr());
		}
	}
	break;
	}

#undef GETPKT
}

void C4Network2ClientList::SendAddresses(C4Network2IOConnection *pConn)
{
	// send all client addresses known
	for (C4Network2Client *pClient = pFirst; pClient; pClient = pClient->getNext())
		pClient->SendAddresses(pConn);
}

void C4Network2ClientList::DoConnectAttempts()
{
	// check interval
	time_t t; time(&t);
	for (C4Network2Client *pClient = pFirst; pClient; pClient = pClient->getNext())
		if (!pClient->isLocal() && !pClient->isRemoved() && pClient->getNextConnAttempt() && pClient->getNextConnAttempt() <= t)
			// attempt connect
			pClient->DoConnectAttempt(pIO);
}

void C4Network2ClientList::ResetReady()
{
	for (C4Network2Client *pClient = pFirst; pClient; pClient = pClient->getNext())
		if (pClient->isWaitedFor())
			pClient->SetStatus(NCS_NotReady);
}

bool C4Network2ClientList::AllClientsReady() const
{
	for (C4Network2Client *pClient = pFirst; pClient; pClient = pClient->getNext())
		if (!pClient->isLocal() && pClient->isWaitedFor() && !pClient->isReady())
			return false;
	return true;
}

void C4Network2ClientList::UpdateClientActivity()
{
	for (C4Network2Client *pClient = pFirst; pClient; pClient = pClient->getNext())
		if (pClient->isActivated())
			if (Game.Players.GetAtClient(pClient->getID()))
				pClient->SetLastActivity(Game.FrameCounter);
}

// *** C4PacketAddr

void C4PacketAddr::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iClientID), "ClientID", C4ClientIDUnknown));
	pComp->Value(mkNamingAdapt(addr,                      "Addr"));
}

// *** C4PacketTCPSimOpen

void C4PacketTCPSimOpen::CompileFunc(StdCompiler *const comp)
{
	comp->Value(mkNamingAdapt(mkIntPackAdapt(clientID), "ClientID", C4ClientIDUnknown));
	comp->Value(mkNamingAdapt(addr, "Addr"));
}
