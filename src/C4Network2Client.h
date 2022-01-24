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

#pragma once

#include "C4NetIO.h"
#include "C4Network2IO.h"
#include "C4PacketBase.h"
#include "C4Client.h"
#include "C4Network2Address.h"

#include <cstddef>
#include <cstdint>
#include <set>
#include <vector>

class C4Network2; class C4Network2IOConnection;

// retry count and interval for connecting a client
const int32_t C4NetClientConnectAttempts = 3,
              C4NetClientConnectInterval = 6; // s

// network client status (host only)
enum C4Network2ClientStatus
{
	NCS_Joining,  // waiting for join data
	NCS_Chasing,  // client is behind (status not acknowledged, isn't waited for)
	NCS_NotReady, // client is behind (status not acknowledged)
	NCS_Ready,    // client acknowledged network status
	NCS_Remove,   // client is to be removed
};

class C4Network2Client
{
	friend class C4Network2ClientList;

public:
	C4Network2Client(C4Client *pClient);
	~C4Network2Client();

protected:
	struct ClientAddress
	{
		C4Network2Address Addr;
		int32_t ConnectionAttempts{0};

		ClientAddress(const C4Network2Address &addr) noexcept : Addr{addr} {}
	};

	// client data
	C4Client *pClient;

	std::vector<ClientAddress> Addresses;
	// addresses
	C4NetIO::addr_t IPv6AddrFromPuncher;

	// Interface ids
	std::set<int> InterfaceIDs;

	// status
	C4Network2ClientStatus eStatus;

	// frame of last activity
	int32_t iLastActivity;

	// connections
	C4Network2IOConnection *pMsgConn, *pDataConn;
	time_t iNextConnAttempt;
	std::unique_ptr<C4NetIOTCP::Socket> tcpSimOpenSocket;

	// part of client list
	C4Network2Client *pNext;
	class C4Network2ClientList *pParent;

	// statistics
	class C4TableGraph *pstatPing;

public:
	C4Client           *getClient()   const { return pClient; }
	const C4ClientCore &getCore()     const { return getClient()->getCore(); }
	int32_t             getID()       const { return getCore().getID(); }
	bool                isLocal()     const { return pClient->isLocal(); }
	bool                isHost()      const { return getID() == C4ClientIDHost; }
	const char         *getName()     const { return getCore().getName(); }
	bool                isActivated() const { return getCore().isActivated(); }
	bool                isObserver()  const { return getCore().isObserver(); }

	const std::vector<ClientAddress> &getAddresses() const { return Addresses; }
	std::size_t getAddrCnt() const { return Addresses.size(); }
	const C4Network2Address &getAddr(std::size_t i) const { return Addresses[i].Addr; }

	const std::set<int> &getInterfaceIDs() const { return InterfaceIDs; }

	C4Network2ClientStatus getStatus()   const { return eStatus; }
	bool                   hasJoinData() const { return getStatus() != NCS_Joining; }
	bool                   isChasing()   const { return getStatus() == NCS_Chasing; }
	bool                   isReady()     const { return getStatus() == NCS_Ready; }
	bool                   isWaitedFor() const { return getStatus() == NCS_NotReady || getStatus() == NCS_Ready; }
	bool                   isRemoved()   const { return getStatus() == NCS_Remove; }

	bool                isConnected()        const { return !!pMsgConn; }
	time_t              getNextConnAttempt() const { return iNextConnAttempt; }
	int32_t             getLastActivity()    const { return iLastActivity; }
	class C4TableGraph *getStatPing()        const { return pstatPing; }

	C4Network2Client *getNext() const { return pNext; }

	void SetStatus(C4Network2ClientStatus enStatus) { eStatus = enStatus; }
	void SetLastActivity(int32_t iTick) { iLastActivity = iTick; }

	C4Network2IOConnection *getMsgConn() const { return pMsgConn; }
	C4Network2IOConnection *getDataConn() const { return pDataConn; }
	bool hasConn(C4Network2IOConnection *pConn);
	void SetMsgConn(C4Network2IOConnection *pConn);
	void SetDataConn(C4Network2IOConnection *pConn);
	void RemoveConn(C4Network2IOConnection *pConn);
	void CloseConns(const char *szMsg);

	bool SendMsg(C4NetIOPacket rPkt) const;

	bool DoConnectAttempt(class C4Network2IO *pIO);
	bool DoTCPSimultaneousOpen(class C4Network2IO *pIO, const C4Network2Address &addr);

	// addresses
	bool hasAddr(const C4Network2Address &addr) const;
	void AddAddrFromPuncher(const C4NetIO::addr_t &addr);
	bool AddAddr(const C4Network2Address &addr, bool fAnnounce, bool inFront = false);
	void AddLocalAddrs(std::uint16_t iPortTCP, std::uint16_t iPortUDP);

	void SendAddresses(C4Network2IOConnection *pConn);

	// runtime statistics
	void CreateGraphs();
	void ClearGraphs();
};

class C4Network2ClientList
{
public:
	C4Network2ClientList(C4Network2IO *pIO);
	~C4Network2ClientList();

protected:
	C4Network2IO *pIO;
	C4Network2Client *pFirst;
	C4Network2Client *pLocal;
	C4ClientList *pClientList;
	bool fHost;

public:
	C4Network2Client *GetClientByID(int32_t iID) const;
	C4Network2Client *GetClient(const char *szName) const;
	C4Network2Client *GetClient(const C4ClientCore &CCore, int32_t iMaxDiffLevel = C4ClientCoreDL_IDMatch);
	C4Network2Client *GetClient(C4Network2IOConnection *pConn) const;
	C4Network2Client *GetLocal() const { return pLocal; }
	C4Network2Client *GetHost();
	C4Network2Client *GetNextClient(C4Network2Client *pClient);

	void Init(C4ClientList *pClientList, bool fHost);
	C4Network2Client *RegClient(C4Client *pClient);
	void DeleteClient(C4Network2Client *pClient);
	void Clear();

	// messages
	bool BroadcastMsgToConnClients(const C4NetIOPacket &rPkt);
	bool BroadcastMsgToClients(const C4NetIOPacket &rPkt, bool includeHost = false);
	bool SendMsgToHost(C4NetIOPacket rPkt);
	bool SendMsgToClient(int32_t iClient, C4NetIOPacket &&rPkt);

	// packet handling
	void HandlePacket(char cStatus, const C4PacketBase *pBasePkt, C4Network2IOConnection *pConn);

	// addresses
	void SendAddresses(C4Network2IOConnection *pConn);

	// connecting
	void DoConnectAttempts();

	// ready-ness
	void ResetReady();
	bool AllClientsReady() const;

	// activity
	void UpdateClientActivity();
};

// Packets

class C4PacketAddr : public C4PacketBase
{
public:
	C4PacketAddr() {}
	C4PacketAddr(int32_t iClientID, const C4Network2Address &addr)
		: iClientID(iClientID), addr(addr) {}

protected:
	int32_t iClientID;
	C4Network2Address addr;

public:
	int32_t                  getClientID() const { return iClientID; }
	const C4Network2Address &getAddr()     const { return addr; }

	virtual void CompileFunc(StdCompiler *pComp) override;
};

class C4PacketTCPSimOpen : public C4PacketBase
{
public:
	C4PacketTCPSimOpen() = default;
	C4PacketTCPSimOpen(const std::int32_t clientID, const C4Network2Address &addr)
		: clientID{clientID}, addr{addr} {}

	std::int32_t GetClientID() const { return clientID; }
	const C4Network2Address &GetAddr() const { return addr; }

	void CompileFunc(StdCompiler *comp) override;

private:
	std::int32_t clientID;
	C4Network2Address addr;
};
