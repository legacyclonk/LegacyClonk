/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2013-2016, The OpenClonk Team and contributors
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

#include "C4Coroutine.h"
#include "C4ForwardDeclarations.h"
#include "C4HTTPClient.h"
#include "C4Network2.h"
#include "C4Network2Client.h"

#include "C4GameParameters.h"
#include "C4Version.h"
#include "C4GameVersion.h"
#include "C4InputValidation.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

// Session data
class C4Network2Reference
{
public:
	C4Network2Reference();
	~C4Network2Reference();

	// Game parameters
	C4GameParameters Parameters;

private:
	// General information
	int32_t Icon;
	C4Network2Status GameStatus;
	int32_t Time;
	int32_t Frame;
	int32_t StartTime;
	int32_t LeaguePerformance; // custom settlement league performance if scenario doesn't use elapsed frames
	ValidatedStdStrBuf<C4InVal::VAL_Comment> Comment;
	bool JoinAllowed;
	bool PasswordNeeded;
	bool OfficialServer;
	C4NetpuncherID NetpuncherGameID;
	StdStrBuf NetpuncherAddr;

	// Engine information
	C4GameVersion Game;

	// Network addresses
	std::vector<C4Network2Address> Addrs;
	C4Network2EndpointAddress source;

public:
	const std::vector<C4Network2Address> &getAddresses() const { return Addrs; }
	const char *getTitle() const { return Parameters.ScenarioTitle.getData(); }
	int32_t getIcon() const { return Icon; }
	C4Network2Status getGameStatus() const { return GameStatus; }
	const char *getComment() const { return Comment.getData(); }
	const C4GameVersion &getGameVersion() const { return Game; }
	bool isPasswordNeeded() const { return PasswordNeeded; }
	bool isJoinAllowed() const { return JoinAllowed; }
	bool isOfficialServer() const { return OfficialServer; }
	int32_t getSortOrder() const;
	int32_t getTime() const { return Time; }
	int32_t getStartTime() const { return StartTime; }
	C4NetpuncherID getNetpuncherGameID() const { return NetpuncherGameID; }
	StdStrBuf getNetpuncherAddr() const { return NetpuncherAddr; }

	void SetSourceAddress(const C4Network2EndpointAddress &ip);
	const C4Network2EndpointAddress &GetSourceAddress() const { return source; }

	void InitLocal(C4Game *pGame);

	void CompileFunc(StdCompiler *pComp);
};

// Serves references (mini-HTTP-server)
class C4Network2RefServer : public C4NetIOTCP
{
public:
	C4Network2RefServer();
	virtual ~C4Network2RefServer();

private:
	CStdCSec RefCSec;
	C4Network2Reference *pReference;

public:
	void Clear();
	void SetReference(C4Network2Reference *pReference);

protected:
	// Overridden
	virtual void PackPacket(const C4NetIOPacket &rPacket, StdBuf &rOutBuf) override;
	virtual size_t UnpackPacket(const StdBuf &rInBuf, const C4NetIO::addr_t &addr) override;

private:
	// Responses
	void RespondMethodNotAllowed(const C4NetIO::addr_t &addr);
	void RespondReference(const C4NetIO::addr_t &addr);
};

class C4Network2HTTPClient : public StdSchedulerProc
{
public:
	C4Network2HTTPClient() = default;
	~C4Network2HTTPClient() override;

public:
	bool Init();
	bool Query(const StdBuf &Data, bool binary);
	bool Query(const char *szData, bool binary) { return Query(StdBuf::MakeRef(szData, SLen(szData)), binary); }

	bool isBusy() const { return busy.load(std::memory_order_acquire); }
	bool isSuccess() const { return success.load(std::memory_order_acquire); }
	bool isConnected() const { return downloadedSize.load(std::memory_order_acquire) + totalSize.load(std::memory_order_acquire) != 0; }
	std::size_t getTotalSize() const { return totalSize.load(std::memory_order_acquire); }
	std::size_t getDownloadedSize() const { return downloadedSize.load(std::memory_order_acquire); }
	const auto &getResultBin() const { assert(binary); return resultBin; }
	const auto &getResultString() const { assert(!binary); return resultString; }
	const char *getURL() const { return url.c_str(); }
	const char *getServerName() const { return serverName.c_str(); }
	const C4NetIO::addr_t &getServerAddress() const { return serverAddress; }
	virtual const char *GetError() const { return !error.empty() && *error.c_str() ? error.c_str() : nullptr; }
	void ResetError() { error.clear(); }
	bool Cancel(std::string_view reason);
	void Clear();

	bool SetServer(std::string_view serverAddress, std::uint16_t defaultPort = 0);
	void SetNotify(class C4InteractiveThread *thread) { this->thread.store(thread, std::memory_order_release); }

	bool Execute(int iMaxTime = C4NetIO::TO_INF) override final { return true; }
	int GetTimeout() override final { return C4NetIO::TO_INF; }

#ifdef _WIN32
	HANDLE GetEvent() override final { return nullptr; }
#else
	void GetFDs(std::vector<pollfd> &fds) override final {}
#endif

protected:
	void SetError(std::string_view error) { this->error = error; }

private:
	C4Task::Hot<void> QueryAsync(C4Task::Hot<C4HTTPClient::Result> &&queryTask);

protected:
	StdBuf resultBin;
	std::string resultString;

private:
	std::optional<C4HTTPClient> client;
	C4Task::Hot<void> task;
	StdBuf data;

private:
	std::string url;
	std::string serverName;
	std::atomic_bool binary{false};

	std::atomic_bool busy{false};
	std::atomic_bool success{false};

	std::mutex dataMutex;
	std::string error;
	C4NetIO::addr_t serverAddress;

	std::atomic_size_t totalSize;
	std::atomic_size_t downloadedSize;

	std::atomic<class C4InteractiveThread *> thread{nullptr};
};

// Loads references (mini-HTTP-client)
class C4Network2RefClient : public C4Network2HTTPClient
{
	C4GameVersion MasterVersion;
	StdStrBuf MessageOfTheDay, MessageOfTheDayHyperlink;
	StdStrBuf LeagueServerRedirect;
	bool fVerSet;

public:
	C4Network2RefClient() : fVerSet(false), C4Network2HTTPClient() {}

	bool QueryReferences();
	bool GetReferences(C4Network2Reference ** &rpReferences, int32_t &rRefCount);
	const char *GetMessageOfTheDay() const { return MessageOfTheDay.getData(); }
	const char *GetMessageOfTheDayHyperlink() const { return MessageOfTheDayHyperlink.getData(); }
	const char *GetLeagueServerRedirect() const { return LeagueServerRedirect.getLength() ? LeagueServerRedirect.getData() : nullptr; }
};
