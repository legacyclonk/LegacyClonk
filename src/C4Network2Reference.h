/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2013-2016, The OpenClonk Team and contributors
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

#pragma once

#ifdef C4ENGINE
#include "C4Network2.h"
#include "C4Network2Client.h"
#else
#include "C4NetIO.h"
#endif

#include "C4GameParameters.h"
#include "C4Version.h"
#include "C4GameVersion.h"
#include "C4InputValidation.h"
#include "C4TimeMilliseconds.h"

#include <functional>
#include <unordered_map>

const int C4Network2HTTPQueryTimeout = 20; // (s)

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
	bool ObservingAllowed;
	bool PasswordNeeded;
	bool OfficialServer;
	C4NetpuncherID NetpuncherGameID;
	StdStrBuf NetpuncherAddr;

	// Engine information
	C4GameVersion Game;

	// Network addresses
	uint8_t iAddrCnt;
	C4Network2Address Addrs[C4ClientMaxAddr];
	C4NetIO::EndpointAddress source;

public:
	const C4Network2Address &getAddr(int i) const { return Addrs[i]; }
	C4Network2Address &getAddr(const int i) { return Addrs[i]; }
	int getAddrCnt() const { return iAddrCnt; }
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

	void SetSourceAddress(const C4NetIO::EndpointAddress &ip);
	const C4NetIO::EndpointAddress &GetSourceAddress() const { return source; }

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
	void RespondNotImplemented(const C4NetIO::addr_t &addr, const char *szMessage);
	void RespondReference(const C4NetIO::addr_t &addr);
};

using CURLM = struct Curl_multi;
using CURL = struct Curl_easy;
using curl_socket_t = SOCKET;

// mini HTTP client
class C4Network2HTTPClient : public StdSchedulerProc
{
public:
	using Headers = std::unordered_map<std::string_view, std::string_view>;
	using Notify = std::function<void(C4Network2HTTPClient *)>;

public:
	C4Network2HTTPClient() = default;
	~C4Network2HTTPClient() override;

public:
	bool Init();

private:

	CURLM *multiHandle{nullptr};
	CURL *curlHandle{nullptr};

#ifdef STDSCHEDULER_USE_EVENTS
	// event indicating network activity
	HANDLE event{nullptr};
#endif
	std::unordered_map<SOCKET, int> sockets;

	std::string url;
	std::string serverName;
	C4NetIO::addr_t serverAddress;
	StdBuf requestData;

	bool binary{false};
	bool success{false};

	// Response header data
	size_t downloadedSize{0};
	size_t totalSize{0};
	bool compressed{false};

	// Callback to call when something happens
	Notify notify;

protected:
	StdBuf resultBin;
	std::string resultString;
	std::string error;
	void SetError(std::string_view error) { this->error = error; }

	void ResetRequestTimeout();
	virtual int32_t GetDefaultPort() { return 80; }

public:
	bool Query(const StdBuf &Data, bool binary, Headers headers = {});
	bool Query(const char *szData, bool binary, Headers headers = {}) { return Query(StdBuf::MakeRef(szData, SLen(szData)), binary, headers); }

	bool isBusy() const { return curlHandle; }
	bool isSuccess() const { return success; }
	bool isConnected() const { return downloadedSize + totalSize != 0; }
	size_t getTotalSize() const { return totalSize; }
	size_t getDownloadedSize() const { return downloadedSize; }
	const auto &getResultBin() const { assert(binary); return resultBin; }
	const auto &getResultString() const { assert(!binary); return resultString; }
	const char *getURL() const { return url.c_str(); }
	const char *getServerName() const { return serverName.c_str(); }
	const C4NetIO::addr_t &getServerAddress() const { return serverAddress; }
	virtual const char *GetError() const { return !error.empty() && *error.c_str() ? error.c_str() : nullptr; }
	void ResetError() { error.clear(); }

	void Cancel(std::string_view reason);
	void Clear();

	bool SetServer(std::string_view serverAddress);

	void SetNotify(const Notify &notify = {}, class C4InteractiveThread *thread = nullptr);

	// Overridden
	virtual bool Execute(int iMaxTime = C4NetIO::TO_INF) override;
	virtual int GetTimeout() override;

#ifdef STDSCHEDULER_USE_EVENTS
	HANDLE GetEvent() override { return event; }
#else
	void GetFDs(fd_set *FDs, int *maxFD) override;
#endif

private:
	static size_t SWriteCallback(char *data, size_t size, size_t nmemb, void *userData);
	size_t WriteCallback(char *data, size_t size);
	static int SProgressCallback(void *client, int64_t dltotal, int64_t dlnow, int64_t ultotal, int64_t ulnow);
	int ProgressCallback(int64_t dltotal, int64_t dlnow);
	static int SSocketCallback(CURL *easy, curl_socket_t s, int what, void *userData, void *socketPointer);
	int SocketCallback(curl_socket_t s, int what);
};

// Loads references (mini-HTTP-client)
class C4Network2RefClient : public C4Network2HTTPClient
{
	C4GameVersion MasterVersion;
	StdStrBuf MessageOfTheDay, MessageOfTheDayHyperlink;
	StdStrBuf LeagueServerRedirect;
	bool fVerSet;

protected:
	virtual int32_t GetDefaultPort() override { return C4NetStdPortRefServer; }

public:
	C4Network2RefClient() : fVerSet(false), C4Network2HTTPClient() {}

	bool QueryReferences();
	bool GetReferences(C4Network2Reference ** &rpReferences, int32_t &rRefCount);
	const char *GetMessageOfTheDay() const { return MessageOfTheDay.getData(); }
	const char *GetMessageOfTheDayHyperlink() const { return MessageOfTheDayHyperlink.getData(); }
	const char *GetLeagueServerRedirect() const { return LeagueServerRedirect.getLength() ? LeagueServerRedirect.getData() : nullptr; }
};
