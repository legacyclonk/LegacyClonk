/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
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

/* control management */

#pragma once

#include "C4Control.h"
#include "C4GameControlNetwork.h"
#include "C4Log.h"
#include "C4Network2Client.h"
#include "C4Record.h"

enum C4ControlMode
{
	CM_None,
	CM_Local,   // control = input
	CM_Network, // control = input + network input
	CM_Replay,  // control = replay
};

#ifndef NDEBUG
const int32_t C4SyncCheckRate = 1,
#else
const int32_t C4SyncCheckRate = 100,
#endif
              C4SyncCheckMaxKeep = 50;

class C4GameControl
{
	friend class C4ControlSyncCheck;
	friend class C4GameControlNetwork;

public:
	C4GameControl();
	~C4GameControl();

public:
	C4Control Input;
	C4GameControlNetwork Network;

protected:
	C4ControlMode eMode;
	bool fPreInit, fInitComplete;
	bool fHost; // (set for local, too)
	bool fActivated;
	bool fRecordNeeded;
	int32_t iClientID;

	C4Record *pRecord;
	C4Playback *pPlayback;

	C4Control SyncChecks;

	C4GameControlClient *pClients;

	C4Control *pExecutingControl; // Control that is in the process of being executed - needed by non-initial records

private:
	std::shared_ptr<spdlog::logger> logger;

public:
	// ticks
	int32_t ControlRate;
	int32_t ControlTick;
	int32_t SyncRate;
	bool DoSync;

public:
	// configuration
	bool isLocal()     const { return eMode == CM_Local; }
	bool isNetwork()   const { return eMode == CM_Network; }
	bool isReplay()    const { return eMode == CM_Replay; }
	bool isCtrlHost()  const { return fHost; }
	bool isRecord()    const { return !!pRecord; }
	int32_t ClientID() const { return iClientID; }
	bool SyncMode()    const { return eMode != CM_Local || pRecord; }

	bool NoInput() const { return isReplay(); }

	// initialization
	void InitLogger();
	bool InitLocal(C4Client *pLocal);
	bool InitNetwork(C4Client *pLocal);
	bool InitReplay(C4Group &rGroup);

	void ChangeToLocal();

	void Clear();
	void Default();

	// records
	bool StartRecord(bool fInitial, bool fStreaming);
	void StopRecord(StdStrBuf *pRecordName = nullptr, uint8_t *pRecordSHA1 = nullptr);
	void RequestRuntimeRecord();
	bool IsRuntimeRecordPossible() const;
	bool RecAddFile(const char *szLocalFilename, const char *szAddAs);

	// execution
	bool Prepare();
	void Execute();
	void Ticks();

	// public helpers
	bool CtrlTickReached(int32_t iTick);
	int32_t getCtrlTick(int32_t iFrame) const;
	int32_t getNextControlTick() const;

	// control rate
	void AdjustControlRate(int32_t iBy);
	bool KeyAdjustControlRate(int32_t iBy)
	{
		AdjustControlRate(iBy); return true;
	}

	// activation
	void SetActivated(bool fActivated);

	// input
	void DoInput(C4PacketType eCtrlType, C4ControlPacket *pPkt, C4ControlDeliveryType eDelivery);
	void DbgRec(C4RecordChunkType eType, const uint8_t *pData = nullptr, size_t iSize = 0); // record debug stuff
	C4ControlDeliveryType DecideControlDelivery();

	// sync check
	void DoSyncCheck();

	// execute and record control (by self or C4GameControlNetwork)
	void ExecControl(const C4Control &rCtrl);
	void ExecControlPacket(C4PacketType eCtrlType, class C4ControlPacket *pPkt);
	void OnGameSynchronizing(); // start record if desired

	const std::shared_ptr<spdlog::logger> &GetLogger() const noexcept { return logger; }

protected:
	// sync checks
	C4ControlSyncCheck *GetSyncCheck(int32_t iTick);
	void RemoveOldSyncChecks();
};

C4LOGGERCONFIG_NAME_TYPE(C4GameControl);
