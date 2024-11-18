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
#include "C4Log.h"
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
	class Impl;

public:
	C4Network2HTTPClient();
	~C4Network2HTTPClient() override;

public:
	bool Init();
	bool Query(const StdBuf &Data, bool binary, C4HTTPClient::Headers headers = {});
	bool Query(const std::string_view data, bool binary, C4HTTPClient::Headers headers = {}) { return Query(StdBuf::MakeRef(data.data(), data.size()), binary, std::move(headers)); }

	bool isBusy() const;
	bool isSuccess() const;
	bool isConnected() const;
	std::size_t getTotalSize() const;
	std::size_t getDownloadedSize() const;
	const StdBuf &getResultBin() const;
	const StdStrBuf &getResultString() const;
	const char *getURL() const;
	const char *getServerName() const;
	const C4NetIO::addr_t &getServerAddress() const;
	virtual const char *GetError() const;
	void ResetError();
	bool Cancel(std::string_view reason);
	void Clear();

	bool SetServer(std::string_view serverAddress, std::uint16_t defaultPort = 0);
	void SetNotify(class C4InteractiveThread *thread);

	bool Execute(int iMaxTime = C4NetIO::TO_INF) override;
	int GetTimeout() override;

#ifdef _WIN32
	HANDLE GetEvent() override;
#else
	void GetFDs(std::vector<pollfd> &fds) override;
#endif

protected:
	void SetError(const char *error);

private:
	std::unique_ptr<Impl> impl;
};

C4LOGGERCONFIG_NAME_TYPE(C4Network2HTTPClient);

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
