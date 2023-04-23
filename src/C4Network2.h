/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2017, The OpenClonk Team and contributors
 * Copyright (c) 2017-2022, The LegacyClonk Team and contributors
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

#include "C4Application.h"
#include "C4Network2Client.h"
#include "C4Network2Res.h"
#include "C4Network2IO.h"
#include "C4Network2Players.h"
#include "C4GameParameters.h"

#include "C4PlayerInfo.h"
#include "C4Teams.h"
#include "C4Control.h"
#include "C4Gui.h"

#ifndef USE_CONSOLE
#include "C4ToastEventHandler.h"
#endif

#include <cstdint>
#include <optional>

// lobby predef - no need to include lobby in header just for the class ptr
namespace C4GameLobby { class MainDlg; class Countdown; }
class C4PacketJoinData;
class C4Record;

// standard ports
const std::uint16_t C4NetStdPortTCP = 11112,
                    C4NetStdPortUDP = 11113,
                    C4NetStdPortDiscovery = 11114,
                    C4NetStdPortRefServer = 11111,
                    C4NetStdPortPuncher = 11115,
                    C4NetStdPortHTTP = 80;

// ressource retrieve wait timeout
const int C4NetResRetrieveTimeout = 100000; // (ms)

// client (de)activation
const int C4NetActivationReqInterval = 5000, // (ms)
          C4NetMaxBehind4Activation = 20, // (ticks)
          C4NetDeactivationDelay = 500; // (ticks)

// client chase
const unsigned int C4NetChaseTargetUpdateInterval = 5; // (s)

// reference
const unsigned int C4NetReferenceUpdateInterval = 120; // (s)
const unsigned int C4NetMinLeagueUpdateInterval = 10; // (s)

// voting
const unsigned int C4NetVotingTimeout = 10; // (s)
const unsigned int C4NetMinVotingInterval = 120; // (s)

// streaming
const int C4NetStreamingMinBlockSize = 10 * 1024;
const int C4NetStreamingMaxBlockSize = 20 * 1024;
const int C4NetStreamingInterval = 30; // (s)

enum C4NetGameState
{
	GS_None,  // network not active
	GS_Init,  // connecting to host, waiting for join data
	GS_Lobby, // lobby mode
	GS_Pause, // game paused
	GS_Go,    // game running
};

class C4Network2Status : public C4PacketBase
{
public:
	C4Network2Status();

protected:
	C4NetGameState eState;
	int32_t iCtrlMode;
	int32_t iTargetCtrlTick;

public:
	C4NetGameState  getState()          const { return eState; }
	int32_t         getCtrlMode()       const { return iCtrlMode; }
	int32_t         getTargetCtrlTick() const { return iTargetCtrlTick; }
	const char     *getStateName()      const;
	const char     *getDescription()    const;

	bool isEnabled()     const { return eState != GS_None; }
	bool isLobbyActive() const { return eState == GS_Lobby; }
	bool isPastLobby()   const { return eState >  GS_Lobby; }
	bool isPaused()      const { return eState == GS_Pause; }
	bool isRunning()     const { return eState == GS_Go; }

	void Set(C4NetGameState eState, int32_t iTargetCtrlTick);
	void SetCtrlMode(int32_t iCtrlMode);
	void SetTargetTick(int32_t iTargetCtrlTick);
	void Clear();

	void CompileFunc(StdCompiler *pComp, bool fReference);
	virtual void CompileFunc(StdCompiler *pComp) override;
};

class C4Network2
{
	friend class C4Network2IO;
	friend class C4Sec1TimerCallback<C4Network2>;

#ifndef USE_CONSOLE
	class ReadyCheckDialog final : public C4GUI::TimedDialog, private C4ToastEventHandler
	{
	public:
		ReadyCheckDialog();
		~ReadyCheckDialog() override = default;

	public:
		void UpdateText() override;

	protected:
		void OnClosed(bool ok) override;
		const char *GetID() override { return "ReadyCheckDialog"; }

	private:
		void Activated() override;
		void OnAction(std::string_view action) override;

	private:
		std::unique_ptr<class C4Toast> toast;
	};
#endif

public:
	C4Network2();
	virtual ~C4Network2() { Clear(); }

public:
	// network i/o class
	C4Network2IO NetIO;

	// ressource list
	C4Network2ResList ResList;

	// client list
	C4Network2ClientList Clients;

	// player list
	C4Network2Players Players;

	// game status
	C4Network2Status Status;

protected:
	// role
	bool fHost;

	// options
	bool fAllowJoin;

	// join resource
	C4Network2ResCore ResDynamic;

	// ressources
	int32_t iDynamicTick;
	bool fDynamicNeeded;

	// game status flags
	bool fStatusAck, fStatusReached;
	bool fChasing;

	// control
	class C4GameControlNetwork *pControl;

	// lobby
	C4GameLobby::MainDlg *pLobby;
	bool fLobbyRunning;
	C4GameLobby::Countdown *pLobbyCountdown;
#ifndef USE_CONSOLE
	ReadyCheckDialog *readyCheckDialog;
#endif

	// master server used
	StdStrBuf MasterServerAddress;

	// timer
	C4Sec1TimerCallback<C4Network2> *pSec1Timer;

	// clients
	int32_t iNextClientID;

	// chase
	time_t iLastChaseTargetUpdate;

	// activation
	time_t iLastActivateRequest;

	// reference
	time_t iLastReferenceUpdate;
	time_t iLastLeagueUpdate, iLeagueUpdateDelay;
	bool fLeagueEndSent;

	// league
	class C4LeagueClient *pLeagueClient;

	// game password
	StdStrBuf sPassword;

	// connection failed because password needed?
	bool fWrongPassword;

	// delayed activation request?
	bool fDelayedActivateReq;

	// voting
	C4Control Votes;
	class C4VoteDialog *pVoteDialog;
	bool fPausedForVote;
	time_t iVoteStartTime, iLastOwnVoting;

	// streaming
	bool fStreaming;
	time_t iLastStreamAttempt;
	C4Record *pStreamedRecord;
	StdBuf StreamingBuf;
	z_stream StreamCompressor;

	class C4Network2HTTPClient *pStreamer;
	unsigned int iCurrentStreamAmount, iCurrentStreamPosition;

	// Puncher
	C4NetpuncherID NetpuncherGameID;
	StdStrBuf NetpuncherAddr;

public:
	// data access
	bool isEnabled()     const { return Status.isEnabled(); }
	bool isLobbyActive() const { return Status.isLobbyActive(); }
	bool isPastLobby()   const { return Status.isPastLobby(); }
	bool isRunning()     const { return Status.isRunning() && isStatusAck(); }
	bool isPaused()      const { return Status.isPaused() && isStatusAck(); }
	bool isHost()        const { return fHost; }
	bool isStatusAck()   const { return fStatusAck; }
	bool isFrozen()      const;

	bool isJoinAllowed()      const { return fAllowJoin; }

	class C4GameLobby::MainDlg *GetLobby() const { return pLobby; } // lobby publication
	const char *GetPassword()              const { return sPassword.getData(); } // Oh noez, now the password is public!
	bool isPassworded()                    const { return !sPassword.isNull(); }

	// initialization result type
	enum InitResult
	{
		IR_Success, IR_Error, IR_Fatal,
	};

	// initialization
	bool InitHost(bool fLobby);
	InitResult InitClient(const class C4Network2Reference &Ref, bool fObserver);
	InitResult InitClient(const std::vector<class C4Network2Address> &addrs, const class C4ClientCore &HostCore, const char *szPassword = nullptr);
	bool DoLobby();
	bool Start();
	bool Pause();
	bool Sync();
	bool FinalInit();

	bool RetrieveScenario(char *szScenario);
	C4Network2Res::Ref RetrieveRes(const C4Network2ResCore &Core, int32_t iTimeout, const char *szResName, bool fWaitForCore = false);

	// idle processes
	void OnSec1Timer();
	void Execute();

	// termination
	void Clear();

	// some options
	bool ToggleAllowJoin();
	bool ToggleClientListDlg();
	void AllowJoin(bool fAllow);
	void SetCtrlMode(int32_t iCtrlMode);
	void SetPassword(const char *szToPassword);
	StdStrBuf QueryClientPassword(); // ask client for a password; empty if user canceled

	// interface for C4Network2IO
	void OnConn(C4Network2IOConnection *pConn);
	void OnDisconn(C4Network2IOConnection *pConn);
	void HandlePacket(char cStatus, const C4PacketBase *pBasePkt, C4Network2IOConnection *pConn);
	void HandleLobbyPacket(char cStatus, const C4PacketBase *pBasePkt, C4Network2IOConnection *pConn);
	bool HandlePuncherPacket(C4NetpuncherPacket::uptr, C4NetIO::HostAddress::AddressFamily family);
	void OnPuncherConnect(C4NetIO::addr_t addr);

	// runtime join stuff
	void OnGameSynchronized();

	// status
	void DrawStatus(C4FacetEx &cgo);

	// client activation
	void RequestActivate();
	void DeactivateInactiveClients(); // host

	// league
	void LeagueGameEvaluate(const char *szRecordName = nullptr, const uint8_t *pRecordSHA = nullptr);
	void LeagueSignupDisable(); // if "internet game" button is switched off in lobby: Remove from league server
	bool LeagueSignupEnable();  // if "internet game" button is switched on in lobby: (re)Add to league server
	void InvalidateReference(); // forces a recreation and re-send of the game reference in the next execution cycle
	bool LeaguePlrAuth(C4PlayerInfo *pInfo); // client: get authentication for a player from the league server
	bool LeaguePlrAuthCheck(C4PlayerInfo *pInfo); // host: check AUID of player info with league server
	void LeagueNotifyDisconnect(int32_t iClientID, enum C4LeagueDisconnectReason eReason);
	void LeagueWaitNotBusy(); // block until league serveris no longer busy. Process update reply if last message was an update
	void LeagueSurrender(); // forfeit in league - just fake a disconnect
	void LeagueShowError(const char *szMsg); // show league error msg box in fullscreen; just log in console

	// voting
	void Vote(C4ControlVoteType eType, bool fApprove = true, int32_t iData = 0);
	void AddVote(const C4ControlVote &Vote);
	C4IDPacket *GetVote(int32_t iClientID, C4ControlVoteType eType, int32_t iData);
	void EndVote(C4ControlVoteType eType, bool fApprove, int32_t iData);
	void OpenVoteDialog();
	void OpenSurrenderDialog(C4ControlVoteType eType, int32_t iData);
	void OnVoteDialogClosed();

	// lobby countdown
	void StartLobbyCountdown(int32_t iCountdownTime);
	void AbortLobbyCountdown();

	// streaming
	size_t getPendingStreamData() const { return StreamingBuf.getSize() - StreamCompressor.avail_out; }
	bool isStreaming() const;
	bool StartStreaming(C4Record *pRecord);
	bool FinishStreaming();
	bool StopStreaming();

	// Netpuncher
	C4NetpuncherID::value &getNetpuncherGameID(C4NetIO::HostAddress::AddressFamily family);
	C4NetpuncherID getNetpuncherGameID() const { return NetpuncherGameID; }
	StdStrBuf getNetpuncherAddr() const { return NetpuncherAddr; }

protected:
	// net i/o initialization
	bool InitNetIO(bool fNoClientID, bool fHost);

	// handling of own packets
	void HandleConn(const class C4PacketConn &Pkt, C4Network2IOConnection *pConn, C4Network2Client *pClient);
	bool CheckConn(const C4ClientCore &CCore, C4Network2IOConnection *pConn, C4Network2Client *pClient, const char *&szReply);
	bool HostConnect(const C4ClientCore &CCore, C4Network2IOConnection *pConn, const char *&szReply);
	bool Join(C4ClientCore &CCore, C4Network2IOConnection *pConn, const char *&szReply);
	void HandleConnRe(const class C4PacketConnRe &Pkt, C4Network2IOConnection *pConn, C4Network2Client *pClient);
	void HandleStatus(const C4Network2Status &nStatus);
	void HandleStatusAck(const C4Network2Status &nStatus, C4Network2Client *pClient);
	void HandleActivateReq(int32_t iTick, C4Network2Client *pClient);
	void HandleJoinData(const class C4PacketJoinData &rPkt);
	void HandleReadyCheck(const class C4PacketReadyCheck &packet);

	// events
	void OnConnect(C4Network2Client *pClient, C4Network2IOConnection *pConn, const char *szMsg, bool fFirstConnection);
	void OnConnectFail(C4Network2IOConnection *pConn);
	void OnDisconnect(C4Network2Client *pClient, C4Network2IOConnection *pConn);
	void OnClientConnect(C4Network2Client *pClient, C4Network2IOConnection *pConn);
	void OnClientDisconnect(C4Network2Client *pClient);

	void SendJoinData(C4Network2Client *pClient);

	// ressource list
	bool CreateDynamic(bool fInit);
	void RemoveDynamic();

	// status changes
	bool ChangeGameStatus(C4NetGameState enState, int32_t iTargetCtrlTick, int32_t iCtrlMode = -1);
	void CheckStatusReached(bool fFromFinalInit = false);
	void CheckStatusAck();
	void OnStatusReached();
	void OnStatusAck();

	// chase
	void UpdateChaseTarget();

	// league
	bool InitLeague(bool *pCancel);
	void DeinitLeague();
	bool LeagueStart(bool *pCancel);
	bool LeagueUpdate();
	bool LeagueUpdateProcessReply();
	bool LeagueEnd(const char *szRecordName = nullptr, const uint8_t *pRecordSHA = nullptr);

	// streaming
	bool StreamIn(bool fFinish);
	bool StreamOut();

	// Puncher
	void InitPuncher();
};

class C4VoteDialog : public C4GUI::MessageDialog
{
public:
	C4VoteDialog(const char *szText, C4ControlVoteType eVoteType, int32_t iVoteData, bool fSurrender);

private:
	C4ControlVoteType eVoteType;
	int32_t iVoteData;
	bool fSurrender;

public:
	C4ControlVoteType getVoteType() const { return eVoteType; }
	int32_t getVoteData() const { return iVoteData; }

private:
	virtual bool OnEnter() override { UserClose(false); return true; } // the vote dialog defaults to "No" [MarkFra]
	virtual void OnClosed(bool fOK) override;

	// true for dialogs that receive full keyboard and mouse input even in shared mode
	virtual bool IsExclusiveDialog() override { return true; }
};

// Packets

class C4PacketJoinData : public C4PacketBase
{
public:
	C4PacketJoinData() {}

protected:
	// the client ID
	int32_t iClientID;

	// Dynamic data to boot
	C4Network2ResCore Dynamic;

	// network status
	C4Network2Status GameStatus;

	// control tick
	int32_t iStartCtrlTick;

public:
	// the game parameters
	C4GameParameters Parameters;

public:
	const int32_t           &getClientID()      const { return iClientID; }
	const C4Network2ResCore &getDynamicCore()   const { return Dynamic; }
	const C4Network2Status  &getStatus()        const { return GameStatus; }
	int32_t                  getStartCtrlTick() const { return iStartCtrlTick; }

	void SetClientID(int32_t inClientID)               { iClientID = inClientID; }
	void SetGameStatus(const C4Network2Status &Status) { GameStatus = Status; }
	void SetDynamicCore(const C4Network2ResCore &Core) { Dynamic = Core; }
	void SetStartCtrlTick(int32_t iTick)               { iStartCtrlTick = iTick; }

	virtual void CompileFunc(StdCompiler *pComp) override;
};

// shows ready check dialog or sends back information

class C4PacketReadyCheck : public C4PacketBase
{
public:
	enum Data : int32_t
	{
		Request = -1,
		NotReady = 0,
		Ready = 1
	};

	C4PacketReadyCheck() = default;
	C4PacketReadyCheck(int32_t client, Data data) : client{client}, data{data} {}
	void CompileFunc(StdCompiler *comp) override;

	int32_t GetClientID() const { return client; }
	auto GetData() const { return static_cast<Data>(data); }
	bool VoteRequested() const { return data == Request; }
	bool IsReady() const { return data == Ready; }

private:
	int32_t client;
	std::underlying_type_t<Data> data;
};

class C4PacketActivateReq : public C4PacketBase
{
public:
	C4PacketActivateReq(int32_t iTick = -1) : iTick(iTick) {}

protected:
	int32_t iTick;

public:
	int32_t getTick() const { return iTick; }

	virtual void CompileFunc(StdCompiler *pComp) override;
};
