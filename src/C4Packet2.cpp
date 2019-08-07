/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2018, The OpenClonk Team and contributors
 * Copyright (c) 2017-2019, The LegacyClonk Team and contributors
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

#include "C4Game.h"
#include "C4Network2Res.h"
#include "C4Version.h"
#include "C4GameLobby.h"

// constants

// workaround
template <class T> struct unpack_class
{
	static C4PacketBase *unpack(StdCompiler *pComp)
	{
		assert(pComp->isCompiler());
		T *pPkt = new T();
		try
		{
			pComp->Value(*pPkt);
		}
		catch (...)
		{
			delete pPkt;
			throw;
		}
		return pPkt;
	}
};

#define PKT_UNPACK(T) unpack_class<T>::unpack

const C4PktHandlingData PktHandlingData[] =
{
	// C4Network2IO (network thread)
	{ PID_Conn,               PC_Network, "Connection Request",          true,  true,  PH_C4Network2IO,         PKT_UNPACK(C4PacketConn) },
	{ PID_ConnRe,             PC_Network, "Connection Request Reply",    true,  true,  PH_C4Network2IO,         PKT_UNPACK(C4PacketConnRe) },

	{ PID_Ping,               PC_Network, "Ping",                        true,  true,  PH_C4Network2IO,         PKT_UNPACK(C4PacketPing) },
	{ PID_Pong,               PC_Network, "Pong",                        true,  true,  PH_C4Network2IO,         PKT_UNPACK(C4PacketPing) },

	{ PID_FwdReq,             PC_Network, "Forward Request",             false, true,  PH_C4Network2IO,         PKT_UNPACK(C4PacketFwd) },
	{ PID_Fwd,                PC_Network, "Forward",                     false, true,  PH_C4Network2IO,         PKT_UNPACK(C4PacketFwd) },

	{ PID_PostMortem,         PC_Network, "Post Mortem",                 false, true,  PH_C4Network2IO,         PKT_UNPACK(C4PacketPostMortem) },

	// C4Network2 (main thread)
	{ PID_Conn,               PC_Network, "Connection Request",          true,  false, PH_C4Network2,           PKT_UNPACK(C4PacketConn) },
	{ PID_ConnRe,             PC_Network, "Connection Request Reply",    true,  false, PH_C4Network2,           PKT_UNPACK(C4PacketConnRe) },

	{ PID_Status,             PC_Network, "Game Status",                 true,  false, PH_C4Network2,           PKT_UNPACK(C4Network2Status) },
	{ PID_StatusAck,          PC_Network, "Game Status Acknowledgement", true,  false, PH_C4Network2,           PKT_UNPACK(C4Network2Status) },

	{ PID_ClientActReq,       PC_Network, "Client Activation Request",   false, false, PH_C4Network2,           PKT_UNPACK(C4PacketActivateReq) },

	{ PID_JoinData,           PC_Network, "Join Data",                   false, false, PH_C4Network2,           PKT_UNPACK(C4PacketJoinData) },

	// C4Network2PlayerList (main thread)
	{ PID_PlayerInfoUpdReq,   PC_Network, "Player info update request",  true,  false, PH_C4Network2Players,    PKT_UNPACK(C4PacketPlayerInfoUpdRequest) },

	{ PID_LeagueRoundResults, PC_Network, "League round results",        true,  false, PH_C4Network2Players,    PKT_UNPACK(C4PacketLeagueRoundResults) },

	// C4GameLobby (main thread)
	{ PID_LobbyCountdown,     PC_Network, "Lobby countdown",             false, false, PH_C4GUIMainDlg,         PKT_UNPACK(C4GameLobby::C4PacketCountdown) },

	// C4Network2ClientList (main thread)
	{ PID_Addr,               PC_Network, "Client Address",              false, false, PH_C4Network2ClientList, PKT_UNPACK(C4PacketAddr) },
	{ PID_TCPSimOpen,         PC_Network, "TCP simultaneous open req",   false, false, PH_C4Network2ClientList, PKT_UNPACK(C4PacketTCPSimOpen) },

	// C4Network2ResList (network thread)
	{ PID_NetResDis,          PC_Network, "Resource Discover",           true,  true,  PH_C4Network2ResList,    PKT_UNPACK(C4PacketResDiscover) },
	{ PID_NetResStat,         PC_Network, "Resource Status",             false, true,  PH_C4Network2ResList,    PKT_UNPACK(C4PacketResStatus) },
	{ PID_NetResDerive,       PC_Network, "Resource Derive",             false, true,  PH_C4Network2ResList,    PKT_UNPACK(C4Network2ResCore) },
	{ PID_NetResReq,          PC_Network, "Resource Request",            false, true,  PH_C4Network2ResList,    PKT_UNPACK(C4PacketResRequest) },
	{ PID_NetResData,         PC_Network, "Resource Data",               false, true,  PH_C4Network2ResList,    PKT_UNPACK(C4Network2ResChunk) },

	// C4GameControlNetwork (network thread)
	{ PID_Control,            PC_Network, "Control",                     false, true,  PH_C4GameControlNetwork, PKT_UNPACK(C4GameControlPacket) },
	{ PID_ControlReq,         PC_Network, "Control Request",             false, true,  PH_C4GameControlNetwork, PKT_UNPACK(C4PacketControlReq) },
	//                       main thread
	{ PID_ControlPkt,         PC_Network, "Control Paket",               false, false, PH_C4GameControlNetwork, PKT_UNPACK(C4PacketControlPkt) },
	{ PID_ExecSyncCtrl,       PC_Network, "Execute Sync Control",        false, false, PH_C4GameControlNetwork, PKT_UNPACK(C4PacketExecSyncCtrl) },

	// Control (Isn't send over network, handled only as part of a control list)
	{ CID_ClientJoin,         PC_Control, "Client Join",                 false, true,  0,                       PKT_UNPACK(C4ControlClientJoin) },
	{ CID_ClientUpdate,       PC_Control, "Client Update",               false, true,  0,                       PKT_UNPACK(C4ControlClientUpdate) },
	{ CID_ClientRemove,       PC_Control, "Client Remove",               false, true,  0,                       PKT_UNPACK(C4ControlClientRemove) },
	{ CID_Vote,               PC_Control, "Voting",                      false, true,  0,                       PKT_UNPACK(C4ControlVote) },
	{ CID_VoteEnd,            PC_Control, "Voting End",                  false, true,  0,                       PKT_UNPACK(C4ControlVoteEnd) },
	{ CID_SyncCheck,          PC_Control, "Sync Check",                  false, true,  0,                       PKT_UNPACK(C4ControlSyncCheck) },
	{ CID_Synchronize,        PC_Control, "Synchronize",                 false, true,  0,                       PKT_UNPACK(C4ControlSynchronize) },
	{ CID_Set,                PC_Control, "Set",                         false, true,  0,                       PKT_UNPACK(C4ControlSet) },
	{ CID_Script,             PC_Control, "Script",                      false, true,  0,                       PKT_UNPACK(C4ControlScript) },
	{ CID_PlrInfo,            PC_Control, "Player Info",                 false, true,  0,                       PKT_UNPACK(C4ControlPlayerInfo) },
	{ CID_JoinPlr,            PC_Control, "Join Player",                 false, true,  0,                       PKT_UNPACK(C4ControlJoinPlayer) },
	{ CID_RemovePlr,          PC_Control, "Remove Player",               false, true,  0,                       PKT_UNPACK(C4ControlRemovePlr) },
	{ CID_PlrSelect,          PC_Control, "Player Select",               false, true,  0,                       PKT_UNPACK(C4ControlPlayerSelect) },
	{ CID_PlrControl,         PC_Control, "Player Control",              false, true,  0,                       PKT_UNPACK(C4ControlPlayerControl) },
	{ CID_PlrCommand,         PC_Control, "Player Command",              false, true,  0,                       PKT_UNPACK(C4ControlPlayerCommand) },
	{ CID_Message,            PC_Control, "Message",                     false, true,  0,                       PKT_UNPACK(C4ControlMessage) },
	{ CID_EMMoveObj,          PC_Control, "EM Move Obj",                 false, true,  0,                       PKT_UNPACK(C4ControlEMMoveObject) },
	{ CID_EMDrawTool,         PC_Control, "EM Draw Tool",                false, true,  0,                       PKT_UNPACK(C4ControlEMDrawTool) },
	{ CID_EMDropDef,          PC_Control, "EM Drop Def",                 false, true,  0,                       PKT_UNPACK(C4ControlEMDropDef) },

	{ CID_MessageBoardAnswer,   PC_Control, "Message Board Answer",      false, true,  0,                       PKT_UNPACK(C4ControlMessageBoardAnswer) },
	{ CID_CustomCommand,        PC_Control, "Custom Command",            false, true,  0,                       PKT_UNPACK(C4ControlCustomCommand) },
	{ CID_InitScenarioPlayer,   PC_Control, "Init Scenario Player",      false, true,  0,                       PKT_UNPACK(C4ControlInitScenarioPlayer) },
	{ CID_ActivateGameGoalMenu, PC_Control, "Activate Game Goal Menu",   false, true,  0,                       PKT_UNPACK(C4ControlActivateGameGoalMenu) },
	{ CID_ToggleHostility,      PC_Control, "Toggle Hostility",          false, true,  0,                       PKT_UNPACK(C4ControlToggleHostility) },
	{ CID_SurrenderPlayer,      PC_Control, "Surrender Player",          false, true,  0,                       PKT_UNPACK(C4ControlSurrenderPlayer) },
	{ CID_ActivateGameGoalRule, PC_Control, "Activate Game Goal/Rule",   false, true,  0,                       PKT_UNPACK(C4ControlActivateGameGoalRule) },
	{ CID_SetPlayerTeam,        PC_Control, "Set Player Team",           false, true,  0,                       PKT_UNPACK(C4ControlSetPlayerTeam) },
	{ CID_EliminatePlayer,      PC_Control, "Eliminate Player",          false, true,  0,                       PKT_UNPACK(C4ControlEliminatePlayer) },

	{ CID_DebugRec,           PC_Control, "Debug Rec",                   false, true,  0,                       PKT_UNPACK(C4ControlDebugRec) },

	// EOL
	{ PID_None,               PC_Network, nullptr,                          false, true,  0,                       nullptr }
};

// C4PacketBase

C4NetIOPacket C4PacketBase::pack(uint8_t cStatus, const C4NetIO::addr_t &addr) const
{
	return C4NetIOPacket(DecompileToBuf<StdCompilerBinWrite>(mkInsertAdapt(mkDecompileAdapt(*this), cStatus)), addr);
}

void C4PacketBase::unpack(const C4NetIOPacket &Pkt, char *pStatus)
{
	if (pStatus) *pStatus = Pkt.getStatus();
	CompileFromBuf<StdCompilerBinRead>(*this, pStatus ? Pkt.getPBuf() : Pkt.getRef());
}

// C4IDPacket

C4IDPacket::C4IDPacket()
	: eID(PID_None), pPkt(nullptr), fOwnPkt(true), pNext(nullptr) {}

C4IDPacket::C4IDPacket(C4PacketType eID, C4PacketBase *pPkt, bool fTakePkt)
	: eID(eID), pPkt(pPkt), fOwnPkt(fTakePkt), pNext(nullptr) {}

C4IDPacket::C4IDPacket(const C4IDPacket &Packet2)
	: C4PacketBase(Packet2),
	eID(PID_None), pPkt(nullptr), fOwnPkt(true), pNext(nullptr)
{
	// kinda hacky (note this might throw an uncaught exception)
	CompileFromBuf<StdCompilerBinRead>(*this,
		DecompileToBuf<StdCompilerBinWrite>(Packet2));
}

C4IDPacket::~C4IDPacket()
{
	Clear();
}

const char *C4IDPacket::getPktName() const
{
	// Use map
	for (const C4PktHandlingData *pPData = PktHandlingData; pPData->ID != PID_None; pPData++)
		if (pPData->ID == eID && pPData->Name)
			return pPData->Name;
	return "Unknown Packet Type";
}

void C4IDPacket::Default()
{
	eID = PID_None; pPkt = nullptr;
}

void C4IDPacket::Clear()
{
	if (fOwnPkt) delete pPkt; pPkt = nullptr;
	eID = PID_None;
}

void C4IDPacket::CompileFunc(StdCompiler *pComp)
{
	// Packet ID
	pComp->Value(mkNamingAdapt(mkIntAdaptT<uint8_t>(eID), "ID", PID_None));
	// Compiling or Decompiling?
	if (pComp->isCompiler())
	{
		if (!pComp->Name(getPktName()))
		{
			pComp->excCorrupt("C4IDPacket: Data value needed! Packet data missing!"); return;
		}
		// Delete old packet
		if (fOwnPkt) delete pPkt; pPkt = nullptr;
		if (eID == PID_None) return;
		// Search unpacking function
		for (const C4PktHandlingData *pPData = PktHandlingData; pPData->ID != PID_None; pPData++)
			if (pPData->ID == eID && pPData->FnUnpack)
			{
				pPkt = pPData->FnUnpack(pComp);
				break;
			}
		if (!pPkt)
			pComp->excCorrupt("C4IDPacket: could not unpack packet id %02x!", eID);
		pComp->NameEnd();
	}
	else if (eID != PID_None)
		// Just write
		pComp->Value(mkNamingAdapt(*pPkt, getPktName()));
}

// C4PacketList

C4PacketList::C4PacketList()
	: pFirst(nullptr), pLast(nullptr) {}

C4PacketList::C4PacketList(const C4PacketList &List2)
	: C4PacketBase(List2),
	pFirst(nullptr), pLast(nullptr)
{
	Append(List2);
}

C4PacketList::~C4PacketList()
{
	Clear();
}

void C4PacketList::Add(C4IDPacket *pPkt)
{
	assert(!pPkt->pNext);
	(pLast ? pLast->pNext : pFirst) = pPkt;
	pLast = pPkt;
}

void C4PacketList::Add(C4PacketType eType, C4PacketBase *pPkt)
{
	Add(new C4IDPacket(eType, pPkt));
}

void C4PacketList::Take(C4PacketList &List)
{
	pFirst = List.pFirst;
	pLast = List.pLast;
	List.pFirst = List.pLast = nullptr;
}

void C4PacketList::Append(const C4PacketList &List)
{
	for (C4IDPacket *pPkt = List.firstPkt(); pPkt; pPkt = List.nextPkt(pPkt))
		Add(new C4IDPacket(*pPkt));
}

void C4PacketList::Clear()
{
	while (pFirst)
		Delete(pFirst);
}

void C4PacketList::Remove(C4IDPacket *pPkt)
{
	if (pPkt == pFirst)
	{
		pFirst = pPkt->pNext;
		if (pPkt == pLast)
			pLast = nullptr;
	}
	else
	{
		C4IDPacket *pPrev;
		for (pPrev = pFirst; pPrev && pPrev->pNext != pPkt; pPrev = pPrev->pNext);
		if (pPrev)
		{
			pPrev->pNext = pPkt->pNext;
			if (pPkt == pLast)
				pLast = pPrev;
		}
	}
}

void C4PacketList::Delete(C4IDPacket *pPkt)
{
	Remove(pPkt);
	delete pPkt;
}

void C4PacketList::CompileFunc(StdCompiler *pComp)
{
	// unpack packets
	if (pComp->isCompiler())
	{
		// Read until no further sections available
		while (pComp->Name("IDPacket"))
		{
			// Read the packet
			C4IDPacket *pPkt = new C4IDPacket();
			try
			{
				pComp->Value(*pPkt);
				pComp->NameEnd();
			}
			catch (...)
			{
				delete pPkt;
				throw;
			}
			// Terminator?
			if (!pPkt->getPkt()) { delete pPkt; break; }
			// Add to list
			Add(pPkt);
		}
		pComp->NameEnd();
	}
	else
	{
		// Write all packets
		for (C4IDPacket *pPkt = pFirst; pPkt; pPkt = pPkt->pNext)
			pComp->Value(mkNamingAdapt(*pPkt, "IDPacket"));
		// Terminate, if no naming is available
		if (!pComp->hasNaming())
		{
			C4IDPacket Pkt;
			pComp->Value(mkNamingAdapt(Pkt, "IDPacket"));
		}
	}
}
