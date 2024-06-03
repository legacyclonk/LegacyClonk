/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2013-2017, The OpenClonk Team and contributors
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

#include "C4Include.h"
#include "C4Game.h"
#include "C4Version.h"
#include "C4Network2Reference.h"

#include <stdexcept>

#include <fcntl.h>

// *** C4Network2Reference

C4Network2Reference::C4Network2Reference()
	: Icon(0), Time(0), Frame(0), StartTime(0), LeaguePerformance(0),
	JoinAllowed(true), PasswordNeeded(false), OfficialServer(false),
	NetpuncherGameID{} {}

C4Network2Reference::~C4Network2Reference() {}

void C4Network2Reference::SetSourceAddress(const C4Network2EndpointAddress &ip)
{
	source = ip;
	for (auto &addr : Addrs)
	{
		if (addr.GetAddr().IsNullHost())
		{
			addr.GetAddr().SetHost(ip);
		}
	}
}

void C4Network2Reference::InitLocal(C4Game *pGame)
{
	// Copy all game parameters
	Parameters = pGame->Parameters;

	// Discard player resources (we don't want these infos in the reference)
	// Add league performance (but only after game end)
	C4ClientPlayerInfos *pClientInfos; C4PlayerInfo *pPlayerInfo;
	int32_t i, j;
	for (i = 0; pClientInfos = Parameters.PlayerInfos.GetIndexedInfo(i); i++)
		for (j = 0; pPlayerInfo = pClientInfos->GetPlayerInfo(j); j++)
		{
			pPlayerInfo->DiscardResource();
			if (pGame->GameOver)
				pPlayerInfo->SetLeaguePerformance(
					pGame->RoundResults.GetLeaguePerformance(pPlayerInfo->GetID()));
		}

	// Special additional information in reference
	Icon = pGame->C4S.Head.Icon;
	GameStatus = pGame->Network.Status;
	Time = pGame->Time;
	Frame = pGame->FrameCounter;
	StartTime = pGame->StartTime;
	LeaguePerformance = pGame->RoundResults.GetLeaguePerformance();
	Comment = Config.Network.Comment;
	JoinAllowed = pGame->Network.isJoinAllowed();
	PasswordNeeded = pGame->Network.isPassworded();
	NetpuncherGameID = pGame->Network.getNetpuncherGameID();
	NetpuncherAddr = pGame->Network.getNetpuncherAddr();
	Game.Set();

	// Addresses
	for (const auto &addr : pGame->Clients.getLocal()->getNetClient()->getAddresses())
	{
		Addrs.emplace_back(addr.Addr);
	}
}

void C4Network2Reference::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(Icon,                                               "Icon",              0));
	pComp->Value(mkParAdapt(GameStatus, true));
	pComp->Value(mkNamingAdapt(Time,                                               "Time",              0));
	pComp->Value(mkNamingAdapt(Frame,                                              "Frame",             0));
	pComp->Value(mkNamingAdapt(StartTime,                                          "StartTime",         0));
	pComp->Value(mkNamingAdapt(LeaguePerformance,                                  "LeaguePerformance", 0));
	pComp->Value(mkNamingAdapt(Comment,                                            "Comment",           ""));
	pComp->Value(mkNamingAdapt(JoinAllowed,                                        "JoinAllowed",       true));
	pComp->Value(mkNamingAdapt(PasswordNeeded,                                     "PasswordNeeded",    false));
	pComp->Value(mkNamingAdapt(mkSTLContainerAdapt(Addrs), "Address"));
	pComp->Value(mkNamingAdapt(Game.sEngineName,                                   "Game",              "None"));
	pComp->Value(mkNamingAdapt(mkArrayAdapt(Game.iVer, 0),                         "Version"));
	pComp->Value(mkNamingAdapt(Game.iBuild,                                        "Build",             -1));
	pComp->Value(mkNamingAdapt(OfficialServer,                                     "OfficialServer",    false));

	pComp->Value(Parameters);

	pComp->Value(mkNamingAdapt(NetpuncherGameID,                                   "NetpuncherID",      C4NetpuncherID(), false, false));
	pComp->Value(mkNamingAdapt(NetpuncherAddr,                                     "NetpuncherAddr",    "", false, false));
}

int32_t C4Network2Reference::getSortOrder() const // Don't go over 100, because that's for the masterserver...
{
	C4GameVersion verThis;
	int iOrder = 0;
	// Official server
	if (isOfficialServer() && !Config.Network.UseAlternateServer) iOrder += 50;
	// Joinable
	if (isJoinAllowed() && (getGameVersion() == verThis)) iOrder += 25;
	// League game
	if (Parameters.isLeague()) iOrder += 5;
	// In lobby
	if (getGameStatus().isLobbyActive()) iOrder += 3;
	// No password needed
	if (!isPasswordNeeded()) iOrder += 1;
	// Done
	return iOrder;
}

// *** C4Network2RefServer

C4Network2RefServer::C4Network2RefServer()
	: pReference(nullptr) {}

C4Network2RefServer::~C4Network2RefServer()
{
	Clear();
}

void C4Network2RefServer::Clear()
{
	C4NetIOTCP::Close();
	delete pReference; pReference = nullptr;
}

void C4Network2RefServer::SetReference(C4Network2Reference *pNewReference)
{
	CStdLock RefLock(&RefCSec);
	delete pReference; pReference = pNewReference;
}

void C4Network2RefServer::PackPacket(const C4NetIOPacket &rPacket, StdBuf &rOutBuf)
{
	// Just append the packet
	rOutBuf.Append(rPacket);
}

size_t C4Network2RefServer::UnpackPacket(const StdBuf &rInBuf, const C4NetIO::addr_t &addr)
{
	const char *pData = rInBuf.getPtr<char>();
	// Check for complete header
	const char *pHeaderEnd = strstr(pData, "\r\n\r\n");
	if (!pHeaderEnd)
		return 0;
	// Check method (only GET is allowed for now)
	if (!SEqual2(pData, "GET "))
	{
		RespondMethodNotAllowed(addr);
		return rInBuf.getSize();
	}
	// Check target
	// TODO
	RespondReference(addr);
	return rInBuf.getSize();
}

void C4Network2RefServer::RespondMethodNotAllowed(const C4NetIO::addr_t &addr)
{
	// Send the message
	const std::string data{"HTTP/1.0 405 Method Not Allowed\r\n\r\n"};
	Send(C4NetIOPacket(data.c_str(), data.size(), false, addr));
	// Close the connection
	Close(addr);
}

void C4Network2RefServer::RespondReference(const C4NetIO::addr_t &addr)
{
	CStdLock RefLock(&RefCSec);
	// Pack
	StdStrBuf PacketData = DecompileToBuf<StdCompilerINIWrite>(mkNamingPtrAdapt(pReference, "Reference"));
	// Create header
	const char *szCharset = C4Config::GetCharsetCodeName(Config.General.LanguageCharset);
	StdStrBuf Header = FormatString(
		"HTTP/1.0 200 OK\r\n"
		"Content-Length: %zu\r\n"
		"Content-Type: text/plain; charset=%s\r\n"
		"Server: " C4ENGINENAME "/" C4VERSION "\r\n"
		"\r\n",
		PacketData.getLength(),
		szCharset);
	// Send back
	Send(C4NetIOPacket(Header.getData(), Header.getLength(), false, addr));
	Send(C4NetIOPacket(PacketData.getData(), PacketData.getLength(), false, addr));
	// Close the connection
	Close(addr);
}

// *** C4Network2HTTPClient

class C4Network2HTTPClient::Impl
{
public:
	virtual ~Impl() = default;

public:
	virtual bool Init() = 0;
	virtual bool Query(const StdBuf &Data, bool binary, C4HTTPClient::Headers headers) = 0;

	virtual bool isBusy() const = 0;
	virtual bool isSuccess() const = 0;
	virtual bool isConnected() const = 0;
	virtual std::size_t getTotalSize() const = 0;
	virtual std::size_t getDownloadedSize() const = 0;
	virtual const StdBuf &getResultBin() const = 0;
	virtual const StdStrBuf &getResultString() const = 0;
	virtual const char *getURL() const = 0;
	virtual const char *getServerName() const = 0;
	virtual const C4NetIO::addr_t &getServerAddress() const = 0;
	virtual const char *GetError() const = 0;
	virtual void ResetError() = 0;
	virtual bool Cancel(std::string_view reason) = 0;
	virtual void Clear() = 0;

	virtual bool SetServer(std::string_view serverAddress, std::uint16_t defaultPort) = 0;
	virtual void SetNotify(C4InteractiveThread *thread) = 0;

	virtual bool Execute(int maxTime) = 0;
	virtual int GetTimeout() = 0;

#ifdef _WIN32
	virtual HANDLE GetEvent() = 0;
#else
	virtual void GetFDs(std::vector<pollfd> &fds) = 0;
#endif

	virtual void SetError(std::string_view error) = 0;
};

class C4Network2HTTPClientImplCurl : public C4Network2HTTPClient::Impl
{
public:
	C4Network2HTTPClientImplCurl() = default;
	~C4Network2HTTPClientImplCurl() override = default;

public:
	bool Init() override;
	bool Query(const StdBuf &Data, bool binary, C4HTTPClient::Headers headers = {}) override;

	bool isBusy() const override { return busy.load(std::memory_order_acquire); }
	bool isSuccess() const override { return success.load(std::memory_order_acquire); }
	bool isConnected() const override { return downloadedSize.load(std::memory_order_acquire) + totalSize.load(std::memory_order_acquire) != 0; }
	std::size_t getTotalSize() const override { return totalSize.load(std::memory_order_acquire); }
	std::size_t getDownloadedSize() const override { return downloadedSize.load(std::memory_order_acquire); }
	const StdBuf &getResultBin() const override { assert(binary); return resultBin; }
	const StdStrBuf &getResultString() const override { assert(!binary); return resultString; }
	const char *getURL() const override { return url.c_str(); }
	const char *getServerName() const override { return serverName.c_str(); }
	const C4NetIO::addr_t &getServerAddress() const override { return serverAddress; }
	virtual const char *GetError() const override { return !error.empty() && *error.c_str() ? error.c_str() : nullptr; }
	void ResetError() override { error.clear(); }
	bool Cancel(std::string_view reason) override;
	void Clear() override;

	bool SetServer(std::string_view serverAddress, std::uint16_t defaultPort) override;
	void SetNotify(class C4InteractiveThread *thread) override { this->thread.store(thread, std::memory_order_release); }

	bool Execute(int iMaxTime = C4NetIO::TO_INF) override { return true; }
	int GetTimeout() override { return C4NetIO::TO_INF; }

#ifdef _WIN32
	HANDLE GetEvent() override { return nullptr; }
#else
	void GetFDs(std::vector<pollfd> &fds) override {}
#endif

protected:
	void SetError(const std::string_view error) override { this->error = error; }

private:
	C4Task::Hot<void> QueryAsync(C4Task::Hot<C4HTTPClient::Result> &&queryTask);

protected:
	StdBuf resultBin;
	StdStrBuf resultString;

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

	std::string error;
	C4NetIO::addr_t serverAddress;

	std::atomic_size_t totalSize;
	std::atomic_size_t downloadedSize;

	std::atomic<class C4InteractiveThread *> thread{nullptr};
};

// mini HTTP client
class C4Network2HTTPClientImplNetIO : public C4Network2HTTPClient::Impl, public C4NetIOTCP, private C4NetIO::CBClass
{
public:
	C4Network2HTTPClientImplNetIO();
	virtual ~C4Network2HTTPClientImplNetIO() override;

private:
	// Address information
	C4NetIO::addr_t ServerAddr, ServerAddrFallback, PeerAddr;
	StdStrBuf Server, RequestPath;

	bool fBinary;
	bool fBusy, fSuccess, fConnected;
	size_t iDataOffset;
	StdBuf Request;
	time_t iRequestTimeout;
	std::chrono::time_point<std::chrono::steady_clock> HappyEyeballsTimeout;

	// Response header data
	size_t iDownloadedSize, iTotalSize;
	bool fCompressed;

	// Event queue to use for notify when something happens
	class C4InteractiveThread *pNotify;

protected:
	StdBuf ResultBin; // set if fBinary
	StdStrBuf ResultString; // set if !fBinary

protected:
	// Overridden
	virtual void PackPacket(const C4NetIOPacket &rPacket, StdBuf &rOutBuf) override;
	virtual size_t UnpackPacket(const StdBuf &rInBuf, const C4NetIO::addr_t &addr) override;

	// Callbacks
	bool OnConn(const C4NetIO::addr_t &AddrPeer, const C4NetIO::addr_t &AddrConnect, const addr_t *pOwnAddr, C4NetIO *pNetIO) override;
	void OnDisconn(const C4NetIO::addr_t &AddrPeer, C4NetIO *pNetIO, const char *szReason) override;
	void OnPacket(const class C4NetIOPacket &rPacket, C4NetIO *pNetIO) override;

	void ResetRequestTimeout();
	virtual int32_t GetDefaultPort() { return 80; }

public:
	bool Init() override { return C4NetIOTCP::Init(); }
	bool Init(const std::uint16_t port) override { return C4NetIOTCP::Init(port); }
	bool Query(const StdBuf &Data, bool fBinary, C4HTTPClient::Headers headers) override;

	bool isBusy() const override { return fBusy; }
	bool isSuccess() const override { return fSuccess; }
	bool isConnected() const override { return fConnected; }
	size_t getTotalSize() const override { return iTotalSize; }
	size_t getDownloadedSize() const override { return iDownloadedSize; }
	const StdBuf &getResultBin() const override { assert(fBinary); return ResultBin; }
	const StdStrBuf &getResultString() const override { assert(!fBinary); return ResultString; }
	const char *getServerName() const override { return Server.getData(); }
	const char *getURL() const override { return ""; }
	const C4NetIO::addr_t &getServerAddress() const override { return ServerAddr; }
	const char *GetError() const override { return C4NetIOTCP::GetError(); }
	void ResetError() override { return C4NetIOTCP::ResetError(); }
	bool Cancel(std::string_view reason) override;
	void Clear() override;

	bool SetServer(std::string_view serverAddress, std::uint16_t defaultPort) override;

	void SetNotify(class C4InteractiveThread *pnNotify) override { pNotify = pnNotify; }

	// Overridden
	bool Execute(int iMaxTime = TO_INF) override;
	int GetTimeout() override;

#ifdef _WIN32
	HANDLE GetEvent() override { return C4NetIOTCP::GetEvent(); }
#else
	void GetFDs(std::vector<pollfd> &fds) { C4NetIOTCP::GetFDs(fds); }
#endif

protected:
	void SetError(const std::string_view error) override { C4NetIO::SetError(error.data()); }

private:
	bool ReadHeader(const StdStrBuf &Data);
	bool Decompress(StdBuf *pData);

private:
	static constexpr std::chrono::milliseconds C4Network2HTTPHappyEyeballsTimeout{300};
	static constexpr int C4Network2HTTPQueryTimeout{20}; // (s)
};

C4Network2HTTPClient::C4Network2HTTPClient()
	: impl{Config.Network.UseCurl
		   ? static_cast<std::unique_ptr<C4Network2HTTPClient::Impl>>(std::make_unique<C4Network2HTTPClientImplCurl>())
		   : static_cast<std::unique_ptr<C4Network2HTTPClient::Impl>>(std::make_unique<C4Network2HTTPClientImplNetIO>())
	  }
{
}

C4Network2HTTPClient::~C4Network2HTTPClient()
{
	Cancel({});
}

bool C4Network2HTTPClient::Init() { return impl->Init(); }
bool C4Network2HTTPClient::Query(const StdBuf &Data, const bool binary, C4HTTPClient::Headers headers) { return impl->Query(Data, binary, std::move(headers)); }

bool C4Network2HTTPClient::isBusy() const { return impl->isBusy(); }
bool C4Network2HTTPClient::isSuccess() const { return impl->isSuccess(); }
bool C4Network2HTTPClient::isConnected() const { return impl->isConnected(); }
std::size_t C4Network2HTTPClient::getTotalSize() const { return impl->getTotalSize(); }
std::size_t C4Network2HTTPClient::getDownloadedSize() const { return impl->getDownloadedSize(); }
const StdBuf &C4Network2HTTPClient::getResultBin() const { return impl->getResultBin(); }
const StdStrBuf &C4Network2HTTPClient::getResultString() const { return impl->getResultString(); }
const char *C4Network2HTTPClient::getURL() const { return impl->getURL(); }
const char *C4Network2HTTPClient::getServerName() const { return impl->getServerName(); }
const C4NetIO::addr_t &C4Network2HTTPClient::getServerAddress() const { return impl->getServerAddress(); }
const char *C4Network2HTTPClient::GetError() const { return impl->GetError(); }
void C4Network2HTTPClient::ResetError() { impl->ResetError(); }
bool C4Network2HTTPClient::Cancel(const std::string_view reason) { return impl->Cancel(reason); }
void C4Network2HTTPClient::Clear() { impl->Clear(); }

bool C4Network2HTTPClient::SetServer(const std::string_view serverAddress, const std::uint16_t defaultPort) { return impl->SetServer(serverAddress, defaultPort); }
void C4Network2HTTPClient::SetNotify(C4InteractiveThread *const thread) { impl->SetNotify(thread); }

bool C4Network2HTTPClient::Execute(const int maxTime) { return impl->Execute(maxTime); }
int C4Network2HTTPClient::GetTimeout() { return impl->GetTimeout(); }

#ifdef _WIN32
HANDLE C4Network2HTTPClient::GetEvent() { return impl->GetEvent(); }
#else
void C4Network2HTTPClient::GetFDs(std::vector<pollfd> &fds) { impl->GetFDs(fds); }
#endif

void C4Network2HTTPClient::SetError(const char *const error) { impl->SetError(error); }

bool C4Network2HTTPClientImplCurl::Init() try
{
	client.emplace();
	return true;
}
catch (const std::runtime_error &e)
{
	SetError(e.what());
	return false;
}

bool C4Network2HTTPClientImplCurl::Query(const StdBuf &Data, const bool binary, C4HTTPClient::Headers headers)
{
	if (!Cancel({}))
	{
		return false;
	}

	data = Data;
	this->binary.store(binary, std::memory_order_release);
	success.store(false, std::memory_order_release);

	downloadedSize.store(0, std::memory_order_release);
	totalSize.store(0, std::memory_order_release);

	try
	{
		C4HTTPClient::Request request{{url}, binary, {reinterpret_cast<const std::byte *>(this->data.getData()), this->data.getSize()}};

		const auto progressCallback = [this](const std::size_t totalSize, const std::size_t downloadedSize)
		{
			this->totalSize.store(totalSize, std::memory_order_release);
			this->downloadedSize.store(downloadedSize, std::memory_order_release);
			return true;
		};

		task = QueryAsync(this->data.isNull() ? client->GetAsync(std::move(request), progressCallback, std::move(headers)) : client->PostAsync(std::move(request), progressCallback, std::move(headers)));
		return true;
	}
	catch (const std::runtime_error &e)
	{
		SetError(e.what());
		return false;
	}
}

bool C4Network2HTTPClientImplCurl::Cancel(const std::string_view reason)
{
	if (task)
	{
		try
		{
			std::move(task).CancelAndWait();
		}
		catch (const std::runtime_error &)
		{
			SetError("Could not cancel");
			return false;
		}
	}

	SetError(reason);
	return true;
}

void C4Network2HTTPClientImplCurl::Clear()
{
	Cancel({});
	resultBin.Clear();
	resultString.Clear();
	ResetError();
	serverAddress.Clear();
	url.clear();
}

bool C4Network2HTTPClientImplCurl::SetServer(const std::string_view serverAddress, const std::uint16_t defaultPort) try
{
	const auto uri = C4HTTPClient::Uri::ParseOldStyle(std::string{serverAddress}, defaultPort);
	url = uri.GetPart(C4HTTPClient::Uri::Part::Uri);
	serverName = uri.GetPart(C4HTTPClient::Uri::Part::Host);
	return true;
}
catch (const std::runtime_error &e)
{
	SetError(e.what());
	return false;
}

C4Task::Hot<void> C4Network2HTTPClientImplCurl::QueryAsync(C4Task::Hot<C4HTTPClient::Result> &&queryTask)
{
	busy.store(true, std::memory_order_release);

	{
		const struct Cleanup
		{
			~Cleanup()
			{
				busy.store(false, std::memory_order_release);
			}

			std::atomic_bool &busy;
		} cleanup{busy};

		try
		{
			auto result = co_await std::move(queryTask);

			if (binary)
			{
				resultBin = std::move(result.Buffer);
			}
			else
			{
				resultString.Copy(reinterpret_cast<const char *>(result.Buffer.getData()), result.Buffer.getSize());
			}

			success.store(true, std::memory_order_release);
			serverAddress = result.ServerAddress;
		}
		catch (const std::runtime_error &e)
		{
			SetError(e.what());
			success.store(false, std::memory_order_release);
		}
	}

	if (auto *const thread = this->thread.load(std::memory_order_acquire); thread)
	{
		thread->PushEvent(Ev_HTTP_Response, this);
	}
}

C4Network2HTTPClientImplNetIO::C4Network2HTTPClientImplNetIO()
	: fBusy(false), fSuccess(false), fConnected(false), iDownloadedSize(0), iTotalSize(0), fBinary(false), iDataOffset(0),
	pNotify(nullptr)
{
	C4NetIOTCP::SetCallback(this);
}

C4Network2HTTPClientImplNetIO::~C4Network2HTTPClientImplNetIO() {}

void C4Network2HTTPClientImplNetIO::PackPacket(const C4NetIOPacket &rPacket, StdBuf &rOutBuf)
{
	// Just append the packet
	rOutBuf.Append(rPacket);
}

size_t C4Network2HTTPClientImplNetIO::UnpackPacket(const StdBuf &rInBuf, const C4NetIO::addr_t &addr)
{
	// since new data arrived, increase timeout time
	ResetRequestTimeout();
	// Check for complete header
	if (!iDataOffset)
	{
		// Copy data into string buffer (terminate)
		StdStrBuf Data; Data.Copy(rInBuf.getPtr<char>(), rInBuf.getSize());
		const char *pData = Data.getData();
		// Header complete?
		const char *pContent = SSearch(pData, "\r\n\r\n");
		if (!pContent)
			return 0;
		// Read the header
		if (!ReadHeader(Data))
		{
			fBusy = fSuccess = false;
			Close(addr);
			return rInBuf.getSize();
		}
	}
	iDownloadedSize = rInBuf.getSize() - iDataOffset;
	// Check if the packet is complete
	if (iTotalSize > iDownloadedSize)
	{
		return 0;
	}
	// Get data, uncompress it if needed
	StdBuf Data = rInBuf.getPart(iDataOffset, iTotalSize);
	if (fCompressed)
		if (!Decompress(&Data))
		{
			fBusy = fSuccess = false;
			Close(addr);
			return rInBuf.getSize();
		}
	// Save the result
	if (fBinary)
		ResultBin.Copy(Data);
	else
		ResultString.Copy(Data.getPtr<char>(), Data.getSize());
	fBusy = false; fSuccess = true;
	// Callback
	OnPacket(C4NetIOPacket(Data, addr), this);
	// Done
	Close(addr);
	return rInBuf.getSize();
}

bool C4Network2HTTPClientImplNetIO::ReadHeader(const StdStrBuf &Data)
{
	const char *pData = Data.getData();
	const char *pContent = SSearch(pData, "\r\n\r\n");
	if (!pContent)
		return 0;
	// Parse header line
	int iHTTPVer1, iHTTPVer2, iResponseCode, iStatusStringPtr;
	if (sscanf(pData, "HTTP/%d.%d %d %n", &iHTTPVer1, &iHTTPVer2, &iResponseCode, &iStatusStringPtr) != 3)
	{
		Error = "Invalid status line!";
		return false;
	}
	// Check HTTP version
	if (iHTTPVer1 != 1)
	{
		Error.Format("Unsupported HTTP version: %d.%d!", iHTTPVer1, iHTTPVer2);
		return false;
	}
	// Check code
	if (iResponseCode != 200)
	{
		// Get status string
		StdStrBuf StatusString; StatusString.CopyUntil(pData + iStatusStringPtr, '\r');
		// Create error message
		Error.Format("HTTP server responded %d: %s", iResponseCode, StatusString.getData());
		return false;
	}
	// Get content length
	const char *pContentLength = SSearch(pData, "\r\nContent-Length:");
	int iContentLength;
	if (!pContentLength || pContentLength > pContent ||
		sscanf(pContentLength, "%d", &iContentLength) != 1)
	{
		Error.Format("Invalid server response: Content-Length is missing!");
		return false;
	}
	iTotalSize = iContentLength;
	iDataOffset = (pContent - pData);
	// Get content encoding
	const char *pContentEncoding = SSearch(pData, "\r\nContent-Encoding:");
	if (pContentEncoding)
	{
		while (*pContentEncoding == ' ') pContentEncoding++;
		StdStrBuf Encoding; Encoding.CopyUntil(pContentEncoding, '\r');
		if (Encoding == "gzip")
			fCompressed = true;
		else
			fCompressed = false;
	}
	else
		fCompressed = false;
	// Okay
	return true;
}

bool C4Network2HTTPClientImplNetIO::Decompress(StdBuf *pData)
{
	size_t iSize = pData->getSize();
	// Create buffer
	uint32_t iOutSize = *pData->getPtr<uint32_t>(pData->getSize() - sizeof(uint32_t));
	iOutSize = std::min<uint32_t>(iOutSize, iSize * 1000);
	StdBuf Out; Out.New(iOutSize);
	// Prepare stream
	z_stream zstrm{};
	zstrm.next_in = const_cast<Byte *>(pData->getPtr<Byte>());
	zstrm.avail_in = pData->getSize();
	zstrm.next_out = Out.getMPtr<Byte>();
	zstrm.avail_out = Out.getSize();
	// Inflate...
	if (inflateInit2(&zstrm, 15 + 16) != Z_OK)
	{
		Error.Format("Could not decompress data!");
		return false;
	}
	// Inflate!
	if (inflate(&zstrm, Z_FINISH) != Z_STREAM_END)
	{
		inflateEnd(&zstrm);
		Error.Format("Could not decompress data!");
		return false;
	}
	// Return the buffer
	Out.SetSize(zstrm.total_out);
	pData->Take(Out);
	// Okay
	inflateEnd(&zstrm);
	return true;
}

bool C4Network2HTTPClientImplNetIO::OnConn(const C4NetIO::addr_t &AddrPeer, const C4NetIO::addr_t &AddrConnect, const C4NetIO::addr_t *pOwnAddr, C4NetIO *pNetIO)
{
	// Make sure we're actually waiting for this connection
	if (fConnected || (AddrConnect != ServerAddr && AddrConnect != ServerAddrFallback))
		return false;
	// Save pack peer address
	PeerAddr = AddrPeer;
	// Send the request
	if (!Send(C4NetIOPacket(Request, AddrPeer)))
	{
		Error.Format("Unable to send HTTP request: %s", Error.getData());
	}
	Request.Clear();
	fConnected = true;
	return true;
}

void C4Network2HTTPClientImplNetIO::OnDisconn(const C4NetIO::addr_t &AddrPeer, C4NetIO *pNetIO, const char *szReason)
{
	// Got no complete packet? Failure...
	if (!fSuccess && Error.isNull())
	{
		fBusy = false;
		Error.Format("Unexpected disconnect: %s", szReason);
	}
	fConnected = false;
	// Notify
	if (pNotify)
		pNotify->PushEvent(Ev_HTTP_Response, this);
}

void C4Network2HTTPClientImplNetIO::OnPacket(const class C4NetIOPacket &rPacket, C4NetIO *pNetIO)
{
	// Everything worthwhile was already done in UnpackPacket. Only do notify callback
	if (pNotify)
		pNotify->PushEvent(Ev_HTTP_Response, this);
}

bool C4Network2HTTPClientImplNetIO::Execute(int iMaxTime)
{
	// Check timeout
	if (fBusy)
	{
		if (std::chrono::steady_clock::now() > HappyEyeballsTimeout)
		{
			HappyEyeballsTimeout = decltype(HappyEyeballsTimeout)::max();
			Application.InteractiveThread.ThreadLogSF("HTTP: Starting fallback connection to %s (%s)", Server.getData(), ServerAddrFallback.ToString().getData());
			Connect(ServerAddrFallback);
		}

		if (time(nullptr) > iRequestTimeout)
		{
			Cancel("Request timeout");
			return true;
		}
	}
	// Execute normally
	return C4NetIOTCP::Execute(iMaxTime);
}

int C4Network2HTTPClientImplNetIO::GetTimeout()
{
	if (!fBusy)
		return C4NetIOTCP::GetTimeout();
	return MaxTimeout(C4NetIOTCP::GetTimeout(), static_cast<int>(1000 * std::max<time_t>(time(nullptr) - iRequestTimeout, 0)));
}

bool C4Network2HTTPClientImplNetIO::Query(const StdBuf &Data, bool fBinary, C4HTTPClient::Headers headers)
{
	if (Server.isNull()) return false;
	// Cancel previous request
	if (fBusy)
		Cancel("Cancelled");
	// No result known yet
	ResultString.Clear();
	// store mode
	this->fBinary = fBinary;
	// Create request
	const char *szCharset = C4Config::GetCharsetCodeName(Config.General.LanguageCharset);
	StdStrBuf Header;
	if (Data.getSize())
		Header.Format(
			"POST %s HTTP/1.0\r\n"
			"Host: %s\r\n"
			"Connection: Close\r\n"
			"Content-Length: %zu\r\n"
			"Content-Type: text/plain; encoding=%s\r\n"
			"Accept-Charset: %s\r\n"
			"Accept-Encoding: gzip\r\n"
			"Accept-Language: %s\r\n"
			"User-Agent: " C4ENGINENAME "/" C4VERSION "\r\n"
			"\r\n",
			RequestPath.getData(),
			Server.getData(),
			Data.getSize(),
			szCharset,
			szCharset,
			Config.General.LanguageEx);
	else
		Header.Format(
			"GET %s HTTP/1.0\r\n"
			"Host: %s\r\n"
			"Connection: Close\r\n"
			"Accept-Charset: %s\r\n"
			"Accept-Encoding: gzip\r\n"
			"Accept-Language: %s\r\n"
			"User-Agent: " C4ENGINENAME "/" C4VERSION "\r\n"
			"\r\n",
			RequestPath.getData(),
			Server.getData(),
			szCharset,
			Config.General.LanguageEx);

	for (const auto &[key, value] : headers)
	{
		Header.AppendFormat("%.*s: %.*s\r\n", key.size(), key.data(), value.size(), value.data());
	}

	// Compose query
	Request.Take(Header.GrabPointer(), Header.getLength());
	Request.Append(Data);

	bool enableFallback{!ServerAddrFallback.IsNull()};
	// Start connecting
	if (!Connect(ServerAddr))
	{
		if (!enableFallback)
		{
			return false;
		}

		std::swap(ServerAddr, ServerAddrFallback);
		enableFallback = false;
		if (!Connect(ServerAddr))
		{
			return false;
		}
	}
	if (enableFallback)
	{
		HappyEyeballsTimeout = std::chrono::steady_clock::now() + C4Network2HTTPHappyEyeballsTimeout;
	}
	else
	{
		HappyEyeballsTimeout = decltype(HappyEyeballsTimeout)::max();
	}

	// Okay, request will be performed when connection is complete
	fBusy = true;
	iDataOffset = 0;
	ResetRequestTimeout();
	ResetError();
	return true;
}

void C4Network2HTTPClientImplNetIO::ResetRequestTimeout()
{
	// timeout C4Network2HTTPQueryTimeout seconds from this point
	iRequestTimeout = time(nullptr) + C4Network2HTTPQueryTimeout;
}

bool C4Network2HTTPClientImplNetIO::Cancel(const std::string_view reason)
{
	// Close connection - and connection attempt
	Close(ServerAddr);
	Close(ServerAddrFallback);
	Close(PeerAddr);

	// Reset flags
	fBusy = fSuccess = fConnected = fBinary = false;
	iDownloadedSize = iTotalSize = iDataOffset = 0;
	Error.Copy(reason.data(), reason.size());
	return true;
}

void C4Network2HTTPClientImplNetIO::Clear()
{
	fBusy = fSuccess = fConnected = fBinary = false;
	iDownloadedSize = iTotalSize = iDataOffset = 0;
	ResultBin.Clear();
	ResultString.Clear();
	Error.Clear();
}

bool C4Network2HTTPClientImplNetIO::SetServer(const std::string_view serverAddress, std::uint16_t defaultPort)
{
	if (defaultPort == 0)
	{
		defaultPort = GetDefaultPort();
	}

	try
	{
		const auto uri = C4HTTPClient::Uri::ParseOldStyle(std::string{serverAddress}, defaultPort);

		Server = std::format("{}:{}", static_cast<std::string>(uri.GetPart(C4HTTPClient::Uri::Part::Host)), static_cast<std::string>(uri.GetPart(C4HTTPClient::Uri::Part::Port))).c_str();
		RequestPath = uri.GetPart(C4HTTPClient::Uri::Part::Path);
	}
	catch (const std::runtime_error &e)
	{
		SetError(e.what());
		return false;
	}

	// Resolve address
	ServerAddr.SetAddress(Server);
	if (ServerAddr.IsNull())
	{
		SetError(FormatString("Could not resolve server address %s!", Server.getData()).getData());
		return false;
	}
	ServerAddr.SetDefaultPort(defaultPort);

	if (ServerAddr.GetFamily() == C4Network2HostAddress::IPv6)
	{
		// Try to find a fallback IPv4 address for Happy Eyeballs.
		ServerAddrFallback.SetAddress(Server, C4Network2HostAddress::IPv4);
		ServerAddrFallback.SetDefaultPort(defaultPort);
	}
	else
	{
		ServerAddrFallback.Clear();
	}

	// Remove port
	const auto &firstColon = std::strchr(Server.getData(), ':');
	const auto &lastColon = std::strrchr(Server.getData(), ':');
	if (firstColon)
	{
		// Hostname/IPv4 address or IPv6 address with port (e.g. [::1]:1234)
		if (firstColon == lastColon || (Server[0] == '[' && lastColon[-1] == ']'))
			Server.SetLength(lastColon - Server.getData());
	}

	// Done
	ResetError();
	return true;
}

// *** C4Network2RefClient

bool C4Network2RefClient::QueryReferences()
{
	// invalidate version
	fVerSet = false;
	// Perform an Query query
	return Query(nullptr, false);
}

bool C4Network2RefClient::GetReferences(C4Network2Reference ** &rpReferences, int32_t &rRefCount)
{
	// Sanity check
	if (isBusy() || !isSuccess()) return false;
	// Parse response
	MasterVersion.Set("", 0, 0, 0, 0, 0);
	fVerSet = false;
	// local update test
	try
	{
		// Create compiler
		StdCompilerINIRead Comp;
		Comp.setInput(getResultString());
		Comp.Begin();
		// get current version
		Comp.Value(mkNamingAdapt(mkInsertAdapt(mkInsertAdapt(mkInsertAdapt(
																 mkNamingAdapt(mkParAdapt(MasterVersion, false), "Version"),
																 mkNamingAdapt(mkParAdapt(MessageOfTheDay, StdCompiler::RCT_All), "MOTD", "")),
															 mkNamingAdapt(mkParAdapt(MessageOfTheDayHyperlink, StdCompiler::RCT_All), "MOTDURL", "")),
											   mkNamingAdapt(mkParAdapt(LeagueServerRedirect, StdCompiler::RCT_All), "LeagueServerRedirect", "")),
								 C4ENGINENAME));
		// Read reference count
		Comp.Value(mkNamingCountAdapt(rRefCount, "Reference"));
		// Create reference array and initialize
		rpReferences = new C4Network2Reference *[rRefCount];
		for (int i = 0; i < rRefCount; i++)
			rpReferences[i] = nullptr;
		// Get references
		Comp.Value(mkNamingAdapt(mkArrayAdaptMapS(rpReferences, rRefCount, mkPtrAdaptNoNull<C4Network2Reference>), "Reference"));
		mkPtrAdaptNoNull<C4Network2Reference>(*rpReferences);
		// Done
		Comp.End();
	}
	catch (const StdCompiler::Exception &e)
	{
		SetError(e.what());
		return false;
	}
	// Set source ip
	for (int i = 0; i < rRefCount; i++)
		rpReferences[i]->SetSourceAddress(getServerAddress());
	// validate version
	if (MasterVersion.iVer[0]) fVerSet = true;
	// Done
	ResetError();
	return true;
}
