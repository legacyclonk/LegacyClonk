/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2017-2020, The LegacyClonk Team and contributors
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

#include "C4Control.h"
#include "C4PacketBase.h"
#include "C4Network2.h"

#include <atomic>

// constants
const int32_t C4ControlBacklog = 100, // (ctrl ticks)
              C4ClientIDAll = C4ClientIDUnknown,
              C4ControlOverflowLimit = 3, // (ctrl ticks)
              C4MaxPreSend = 15; // (frames) - must be smaller than C4ControlBacklog!

const uint32_t C4ControlRequestInterval = 2000; // (ms)

enum C4ControlDeliveryType
{
	CDT_Queue = 0,   // Send in control queue (sync)
	CDT_Sync = 1,    // Send, delay execution until net is sync (sync)
	CDT_Direct = 2,  // Send directly to all clients (not sync)
	CDT_Private = 3, // Send only to some clients (not sync, obviously)

	CDT_Decide,      // Use whatever sync mode seems fastest atm (sync)
};

// Additional notes / requirements:
// * Direct control always reaches all clients faster than queue control.
// * Sync is the only synchronous control mode were it's garantueed that
//   the control is actually executed within a fixed time.
// * CDT_Decide is guesswork, control isn't garantueed to be faster.

enum C4GameControlNetworkMode
{
	CNM_Decentral = 0, // 0 is the standard mode set in config
	CNM_Central = 1,
	CNM_Async = 2,
};

// declarations
class C4GameControlPacket; class C4GameControlClient;
class C4PacketControlReq; class C4ClientList;

// main class
class C4GameControlNetwork // run by network thread
{
public:
	static constexpr int32_t DefaultTargetFPS = 38;

	C4GameControlNetwork(class C4GameControl *pParent);
	~C4GameControlNetwork();

protected:
	volatile bool fEnabled, fRunning;

	// local status
	int32_t iClientID;
	bool fHost, fActivated;
	C4GameControlNetworkMode eMode;
	int32_t iTargetTick;

	// options
	volatile int32_t iControlPreSend;

	// statistics
	int32_t iWaitStart;
	int32_t iAvgControlSendTime;
	int32_t iTargetFPS; // used for PreSend-colculation

	// control send / recv status
	std::atomic<std::int32_t> iControlSent, iControlReady;

	// control list
	C4GameControlPacket *pCtrlStack;
	CStdCSec CtrlCSec;

	// list of clients (activated only!)
	C4GameControlClient *pClients;
	CStdCSec ClientsCSec;

	// holds control that needs to be executed synchronized (main thread only)
	C4Control SyncControl;
	C4GameControlPacket *pSyncCtrlQueue;

	// control request timing
	uint32_t iNextControlReqeust;

	// links
	C4GameControl *const pParent;
	C4Network2 *pNetwork;

public:
	bool IsEnabled()   const { return fEnabled; }
	bool IsRunning()   const { return fRunning; }
	bool IsActivated() const { return fActivated; }

	int32_t getControlPreSend() const { return iControlPreSend; }
	void setControlPreSend(int32_t iToVal) { iControlPreSend = (std::min)(iToVal, C4MaxPreSend); }
	int32_t getAvgControlSendTime() const { return iAvgControlSendTime; }
	void setTargetFPS(int32_t iToVal) { iTargetFPS = iToVal; }

	// main thread communication
	bool Init(int32_t iClientID, bool fHost, int32_t iStartTick, bool fActivated, C4Network2 *pNetwork); // by main thread
	void Clear(); // by main thread

	void Execute(); // by main thread
	bool CtrlReady(int32_t iTick); // by main thread
	bool CtrlOverflow(int32_t iTick) const { return fRunning && iControlReady >= iTick + C4ControlOverflowLimit; } // by main thread
	int32_t GetBehind(int32_t iTick) const { return iControlReady - iTick + 1; } // by main thread
	bool GetControl(C4Control *pCtrl, int32_t iTick); // by main thread
	bool ClientReady(int32_t iClientID, int32_t iTick); // by main thread
	int32_t ClientPerfStat(int32_t iClientID); // by main thread
	int32_t ClientNextControl(int32_t iClientID); // by main thread

	bool CtrlNeeded(int32_t iTick) const; // by main thread
	void DoInput(const C4Control &Input); // by main thread
	void DoInput(C4PacketType eCtrlType, C4ControlPacket *pPkt, enum C4ControlDeliveryType eType); // by main thread

	// sync control
	C4ControlDeliveryType DecideControlDelivery() const; // by main thread
	void ExecSyncControl(); // by main thread
	void ExecSyncControl(int32_t iControlTick); // by main thread

	// clients
	void CopyClientList(const C4ClientList &rClients);

	// pausing
	void SetRunning(bool fnRunning, int32_t inTargetTick = -1); // by main thread
	void SetActivated(bool fnActivated); // by main thread
	void SetCtrlMode(C4GameControlNetworkMode enMode); // by main thread
	C4GameControlNetworkMode GetCtrlMode() const { return eMode; } // by main thread

	// performance
	void CalcPerformance(int32_t iCtrlTick); // by main thread

	// interfaces
	void HandlePacket(char cStatus, const C4PacketBase *pPacket, C4Network2IOConnection *pConn);
	void OnResComplete(C4Network2Res *pRes);

protected:
	// clients
	void AddClient(int32_t iClientID, const char *szName); // by main thread
	void ClearClients(); // by main thread

	// packet handling
	void HandleControl(int32_t iByClientID, const C4GameControlPacket &rPkt);
	void HandleControlReq(const C4PacketControlReq &rPkt, C4Network2IOConnection *pConn);
	void HandleControlPkt(C4PacketType eCtrlType, C4ControlPacket *pPkt, enum C4ControlDeliveryType eType);

	// client list
	C4GameControlClient *getClient(int32_t iID);
	void AddClient(C4GameControlClient *pClient);
	void RemoveClient(C4GameControlClient *pClient);

	// control stack
	C4GameControlPacket *getCtrl(int32_t iClientID, int32_t iCtrlTick); // by both
	void AddCtrl(C4GameControlPacket *pCtrl);
	void ClearCtrl(int32_t iBeforeTick = -1);
	void CheckCompleteCtrl(bool fSetEvent); // by both
	C4GameControlPacket *PackCompleteCtrl(int32_t iTick); // by main thread

	// sync control
	void AddSyncCtrlToQueue(const C4Control &Ctrl, int32_t iTick); // by main thread
	void ExecQueuedSyncCtrl(); // by main thread
};

class C4GameControlPacket : public C4PacketBase
{
	friend class C4GameControlNetwork;

public:
	C4GameControlPacket();

	// needed as C4Control doesn't seem to implement correct copying behavior
	C4GameControlPacket(const C4GameControlPacket &Pkt2);
	C4GameControlPacket &operator=(const C4GameControlPacket &) = delete;

protected:
	// header
	int32_t iClientID, iCtrlTick;
	int32_t iTime;

	// data
	C4Control Ctrl;

	// list (C4GameControlNetwork)
	C4GameControlPacket *pNext;

public:
	int32_t          getClientID() const { return iClientID; }
	int32_t          getCtrlTick() const { return iCtrlTick; }
	int32_t          getTime()     const { return iTime; }
	const C4Control &getControl()  const { return Ctrl; }

	void Set(int32_t iClientID, int32_t iCtrlTick);
	void Set(int32_t iClientID, int32_t iCtrlTick, const C4Control &Ctrl);
	void Add(const C4GameControlPacket &Ctrl);

	virtual void CompileFunc(StdCompiler *pComp) override;
};

class C4GameControlClient
{
	friend class C4GameControlNetwork;

public:
	C4GameControlClient();

protected:
	// core data
	int32_t iClientID;
	char szName[C4MaxName + 1];

	// next expected control for this client
	int32_t iNextControl;

	// performance data
	int32_t iPerformance;

	// list (C4GameControl)
	C4GameControlClient *pNext;

public:
	int32_t     getClientID()    const { return iClientID; }
	const char *getName()        const { return szName; }
	int32_t     getNextControl() const { return iNextControl; }
	int32_t     getPerfStat()    const;

	void Set(int32_t iClientID, const char *szName);
	void SetNextControl(int32_t inNextControl) { iNextControl = inNextControl; }
	void AddPerf(int32_t iTime);
};

// * Packet classes *

class C4PacketControlReq : public C4PacketBase
{
public:
	C4PacketControlReq(int32_t iCtrlTick = -1);

protected:
	int32_t iCtrlTick;

public:
	int32_t getCtrlTick() const { return iCtrlTick; }

	virtual void CompileFunc(StdCompiler *pComp) override;
};

class C4PacketControlPkt : public C4PacketBase
{
public:
	C4PacketControlPkt() {}
	C4PacketControlPkt(enum C4ControlDeliveryType eDelivery, const C4IDPacket &Ctrl)
		: eDelivery(eDelivery), Ctrl(Ctrl) {}

protected:
	enum C4ControlDeliveryType eDelivery;
	C4IDPacket Ctrl;

public:
	C4ControlDeliveryType getDelivery() const { return eDelivery; }
	const C4IDPacket &getCtrl() const { return Ctrl; }

	virtual void CompileFunc(StdCompiler *pComp) override;
};

class C4PacketExecSyncCtrl : public C4PacketBase
{
public:
	C4PacketExecSyncCtrl(int32_t iControlTick = ~0) : iControlTick(iControlTick) {}

protected:
	int32_t iControlTick;

public:
	int32_t getControlTick() const { return iControlTick; }

	virtual void CompileFunc(StdCompiler *pComp) override { pComp->Value(mkNamingAdapt(iControlTick, "ControlTick", -1)); }
};
