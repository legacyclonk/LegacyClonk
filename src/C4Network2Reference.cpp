/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2013-2017, The OpenClonk Team and contributors
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
#ifdef C4ENGINE
#include "C4Game.h"
#endif
#include "C4Version.h"
#include "C4Network2Reference.h"

#define CURL_STRICTER
#include <curl/curl.h>

#include <fcntl.h>

#include <memory>
#include <stdexcept>

static constexpr long C4Network2HTTPQueryTimeout{20}; // (s)

// *** C4Network2Reference

C4Network2Reference::C4Network2Reference()
	: Icon(0), Time(0), Frame(0), StartTime(0), LeaguePerformance(0),
	JoinAllowed(true), ObservingAllowed(true), PasswordNeeded(false), OfficialServer(false),
	iAddrCnt(0), NetpuncherGameID{} {}

C4Network2Reference::~C4Network2Reference() {}

void C4Network2Reference::SetSourceAddress(const C4NetIO::EndpointAddress &ip)
{
	source = ip;
	for (int i = 0; i < iAddrCnt; i++)
	{
		if (Addrs[i].GetAddr().IsNullHost())
		{
			Addrs[i].GetAddr().SetHost(ip);
		}
	}
}

#ifdef C4ENGINE
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
	ObservingAllowed = pGame->Network.isObservingAllowed();
	PasswordNeeded = pGame->Network.isPassworded();
	NetpuncherGameID = pGame->Network.getNetpuncherGameID();
	NetpuncherAddr = pGame->Network.getNetpuncherAddr();
	Game.Set();

	// Addresses
	C4Network2Client *pLocalNetClient = pGame->Clients.getLocal()->getNetClient();
	iAddrCnt = pLocalNetClient->getAddrCnt();
	for (i = 0; i < iAddrCnt; i++)
		Addrs[i] = pLocalNetClient->getAddr(i);
}
#endif

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
	pComp->Value(mkNamingAdapt(ObservingAllowed,                                   "ObservingAllowed",  true));
	pComp->Value(mkNamingAdapt(PasswordNeeded,                                     "PasswordNeeded",    false));
	// Ignore RegJoinOnly
	bool RegJoinOnly = false;
	pComp->Value(mkNamingAdapt(RegJoinOnly,                                        "RegJoinOnly",       false));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iAddrCnt),                           "AddressCount",      0));
	iAddrCnt = std::min<uint8_t>(C4ClientMaxAddr, iAddrCnt);
	pComp->Value(mkNamingAdapt(mkArrayAdapt(Addrs, iAddrCnt, C4Network2Address()), "Address"));
	pComp->Value(mkNamingAdapt(Game.sEngineName,                                   "Game",              "None"));
	pComp->Value(mkNamingAdapt(mkArrayAdaptDM(Game.iVer, 0),                       "Version"));
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
	const char *pData = getBufPtr<char>(rInBuf);
	// Check for complete header
	const char *pHeaderEnd = strstr(pData, "\r\n\r\n");
	if (!pHeaderEnd)
		return 0;
	// Check method (only GET is allowed for now)
	if (!SEqual2(pData, "GET "))
	{
		RespondNotImplemented(addr, "Method not implemented");
		return rInBuf.getSize();
	}
	// Check target
	// TODO
	RespondReference(addr);
	return rInBuf.getSize();
}

void C4Network2RefServer::RespondNotImplemented(const C4NetIO::addr_t &addr, const char *szMessage)
{
	// Send the message
	StdStrBuf Data = FormatString("HTTP/1.0 501 %s\r\n\r\n", szMessage);
	Send(C4NetIOPacket(Data.getData(), Data.getLength(), false, addr));
	// Close the connection
	Close(addr);
}

void C4Network2RefServer::RespondReference(const C4NetIO::addr_t &addr)
{
	CStdLock RefLock(&RefCSec);
	// Pack
	StdStrBuf PacketData = DecompileToBuf<StdCompilerINIWrite>(mkNamingPtrAdapt(pReference, "Reference"));
	// Create header
	const char *szCharset = GetCharsetCodeName(Config.General.LanguageCharset);
	StdStrBuf Header = FormatString(
		"HTTP/1.1 200 Found\r\n"
		"Content-Length: %zu\r\n"
		"Content-Type: text/plain; charset=%s\r\n"
		"Server: " C4ENGINENAME "/" C4VERSION "\r\n"
		"\r\n",
		PacketData.getLength(),
		szCharset);
	// Send back
	Send(C4NetIOPacket(Header, Header.getLength(), false, addr));
	Send(C4NetIOPacket(PacketData, PacketData.getLength(), false, addr));
	// Close the connection
	Close(addr);
}

// *** C4Network2HTTPClient

C4Network2HTTPClient::~C4Network2HTTPClient()
{
	Cancel({});

	if (multiHandle)
	{
		curl_multi_cleanup(multiHandle);
	}

#ifdef STDSCHEDULER_USE_EVENTS
	if (event)
	{
		WSACloseEvent(event);
	}
#endif
}

bool C4Network2HTTPClient::Init()
{
	multiHandle = curl_multi_init();
	if (!multiHandle)
	{
		return false;
	}

#ifdef STDSCHEDULER_USE_EVENTS
	event = WSACreateEvent();
	if (event == WSA_INVALID_EVENT)
	{
		SetError("Could not create socket event");
		curl_multi_cleanup(multiHandle);
		multiHandle = nullptr;
		return false;
	}
#endif

	curl_multi_setopt(multiHandle, CURLMOPT_SOCKETFUNCTION, &C4Network2HTTPClient::SSocketCallback);
	curl_multi_setopt(multiHandle, CURLMOPT_SOCKETDATA, this);
	curl_multi_setopt(multiHandle, CURLMOPT_TIMERFUNCTION, &C4Network2HTTPClient::STimerCallback);
	curl_multi_setopt(multiHandle, CURLMOPT_TIMERDATA, this);

	return true;
}

bool C4Network2HTTPClient::Execute(int)
{
	int running{0};
#ifdef STDSCHEDULER_USE_EVENTS
	// On Windows, StdScheduler doesn't inform us about which fd triggered the
	// event, so we have to check manually.
	if (WaitForSingleObject(event, 0) == WAIT_OBJECT_0)
	{
		// copy map to prevent crashes
		auto tmp = sockets;
		for (const auto &[socket, what] : tmp)
		{
			if (WSANETWORKEVENTS networkEvents; !WSAEnumNetworkEvents(socket, event, &networkEvents))
			{
				int eventBitmask{0};
				if (networkEvents.lNetworkEvents & (FD_READ | FD_ACCEPT | FD_CLOSE)) eventBitmask |= CURL_CSELECT_IN;
				if (networkEvents.lNetworkEvents & (FD_WRITE | FD_CONNECT)) eventBitmask |= CURL_CSELECT_OUT;
				curl_multi_socket_action(multiHandle, socket, eventBitmask, &running);
			}
		}
	}
#else
	fd_set fds[2];

	// build socket sets
	int max{0};
	FD_ZERO(&fds[0]); FD_ZERO(&fds[1]);
	GetFDs(fds, &max);

	// build timeout value
	timeval to{0, 0};

	// wait for something to happen
	int ret{select(max + 1, &fds[0], &fds[1], nullptr, &to)};

	// error
	if (ret < 0)
	{
		SetError("select failed");
		return false;
	}

	else if (ret > 0)
	{
		// copy map to prevent crashes
		auto tmp = sockets;
		for (const auto &[socket, what] : tmp)
		{
			int eventBitmask{0};
			if (FD_ISSET(socket, &fds[0])) eventBitmask |= CURL_CSELECT_IN;
			if (FD_ISSET(socket, &fds[1])) eventBitmask |= CURL_CSELECT_OUT;

			curl_multi_socket_action(multiHandle, socket, eventBitmask, &running);
		}
	}
#endif
	else
	{
		curl_multi_socket_action(multiHandle, CURL_SOCKET_TIMEOUT, 0, &running);
	}

	CURLMsg *message;
	do
	{
		int messagesInQueue{0};
		message = curl_multi_info_read(multiHandle, &messagesInQueue);
		if (message && message->msg == CURLMSG_DONE)
		{
			CURL *const e{message->easy_handle};
			assert(e == curlHandle);
			curlHandle = nullptr;
			curl_multi_remove_handle(multiHandle, e);

			// Check for errors and notify listeners. Note that curl fills
			// the error buffer automatically.
			success = message->data.result == CURLE_OK;
			if (!success && error.empty())
			{
				SetError(curl_easy_strerror(message->data.result));
			}

			char *ip;
			curl_easy_getinfo(e, CURLINFO_PRIMARY_IP, &ip);
			serverAddress.SetHost(StdStrBuf{ip});

			if (thread)
			{
				thread->PushEvent(eventData.type, eventData.data);
			}

			curl_easy_cleanup(e);
		}
	}
	while (message);

	return true;
}

int C4Network2HTTPClient::GetTimeout()
{
	return timeout;
}

#ifndef STDSCHEDULER_USE_EVENTS
void C4Network2HTTPClient::GetFDs(fd_set *FDs, int *maxFD)
{
	for (const auto &[socket, what] : sockets)
	{
		switch (what)
		{
		case CURL_POLL_IN:
			FD_SET(socket, &FDs[0]); if (maxFD) *maxFD = std::max<SOCKET>(*maxFD, socket);
			break;

		case CURL_POLL_OUT:
			FD_SET(socket, &FDs[1]); if (maxFD) *maxFD = std::max<SOCKET>(*maxFD, socket);
			break;

		case CURL_POLL_INOUT:
			FD_SET(socket, &FDs[0]); FD_SET(socket, &FDs[1]); if (maxFD) *maxFD = std::max<SOCKET>(*maxFD, socket);
			break;

		default:
			break;;
		}
	}
}
#endif

bool C4Network2HTTPClient::Query(const StdBuf &Data, bool binary, Headers headers)
{
	if (serverName.empty()) return false;


	// Cancel previous request
	if (curlHandle)
		Cancel("Cancelled");

	// No result known yet
	resultBin.Clear();
	resultString.clear();

	this->binary = binary;

	CURL *const curl{curl_easy_init()};
	if (!curl)
	{
		return false;
	}

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
	curl_easy_setopt(curl, CURLOPT_USERAGENT, C4ENGINENAME "/" C4VERSION );
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, C4Network2HTTPQueryTimeout);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);

	// Create request
	const char *const charset{GetCharsetCodeName(Config.General.LanguageCharset)};

	headers["Accept-Charset"] = charset;
	headers["Accept-Language"] = Config.General.LanguageEx;
	headers.erase("User-Agent");

	curl_slist *headerList{nullptr};

	if (Data.getSize())
	{
		requestData.Copy(Data);
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestData.getData());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, requestData.getSize());

		if (!headers.count("Content-Type"))
		{
			static constexpr auto defaultType = "text/plain; encoding=";
			headerList = curl_slist_append(headerList, FormatString("Content-Type: %s%s", defaultType, charset).getData());
		}

		// Disable the Expect: 100-Continue header which curl automaticallY
		// adds for POST requests.
		headerList = curl_slist_append(headerList, "Expect:");
	}

	for (const auto &[key, value] : headers)
	{
		std::string header{key};
		header += ": ";
		header += value;
		headerList = curl_slist_append(headerList, header.c_str());
	}

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &C4Network2HTTPClient::SWriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
	curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, C4Network2HTTPClient::SProgressCallback);
	curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);

	ResetError();
	error.resize(CURL_ERROR_SIZE);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error.data());

	curl_multi_add_handle(multiHandle, curl);
	curlHandle = curl;
	downloadedSize = totalSize = 0;

	int running;
	curl_multi_socket_action(multiHandle, CURL_SOCKET_TIMEOUT, 0, &running);

	return true;
}

size_t C4Network2HTTPClient::SWriteCallback(char *data, size_t size, size_t nmemb, void *userData)
{
	return reinterpret_cast<C4Network2HTTPClient *>(userData)->WriteCallback(data, size * nmemb);
}

size_t C4Network2HTTPClient::WriteCallback(char *data, size_t size)
{
	if (binary)
	{
		resultBin.Append(data, size);
	}
	else
	{
		resultString.append(data, size);
	}

	return size;
}

int C4Network2HTTPClient::SProgressCallback(void *client, int64_t dltotal, int64_t dlnow, int64_t, int64_t)
{
	return reinterpret_cast<C4Network2HTTPClient *>(client)->ProgressCallback(dltotal, dlnow);
}

int C4Network2HTTPClient::ProgressCallback(int64_t downloadTotal, int64_t downloadNow)
{
	downloadedSize = static_cast<size_t>(downloadNow);
	totalSize = static_cast<size_t>(downloadTotal);
	return 0;
}

int C4Network2HTTPClient::SSocketCallback(CURL *, curl_socket_t s, int what, void *userData, void *)
{
	return reinterpret_cast<C4Network2HTTPClient *>(userData)->SocketCallback(s, what);
}

int C4Network2HTTPClient::SocketCallback(curl_socket_t s, int what)
{
#ifdef STDSCHEDULER_USE_EVENTS
	static constexpr long networkEventsIn{FD_READ | FD_ACCEPT | FD_CLOSE};
	static constexpr long networkEventsOut{FD_WRITE | FD_CONNECT};

	long networkEvents;
	switch (what)
	{
	case CURL_POLL_IN:
		networkEvents = networkEventsIn;
		break;

	case CURL_POLL_OUT:
		networkEvents = networkEventsOut;
		break;

	case CURL_POLL_INOUT:
		networkEvents = networkEventsIn | networkEventsOut;
		break;

	default:
		networkEvents = 0;
		break;
	}

	if (WSAEventSelect(s, event, networkEvents) == SOCKET_ERROR)
	{
		SetError("Could not set event");
		return 1;
	}
#endif
	if (what == CURL_POLL_REMOVE)
	{
		sockets.erase(s);
	}
	else
	{
		sockets[s] = what;
	}

	return 0;
}

int C4Network2HTTPClient::STimerCallback(CURLM *, long timeout, void *userData)
{
	reinterpret_cast<C4Network2HTTPClient *>(userData)->timeout = timeout >= 0 ? static_cast<int32_t>(timeout) : 1000;
	return 0;
}

void C4Network2HTTPClient::Cancel(std::string_view reason)
{
	if (curlHandle)
	{
		curl_multi_remove_handle(multiHandle, curlHandle);
		curl_easy_cleanup(curlHandle);
		curlHandle = nullptr;
	}

	binary = false;
	downloadedSize = totalSize = 0;
	SetError(reason);
}

void C4Network2HTTPClient::Clear()
{
	Cancel({});
	serverAddress.Clear();
	resultBin.Clear();
	resultString.clear();
}

bool C4Network2HTTPClient::SetServer(const std::string_view serverAddress, const std::uint16_t defaultPort)
{
	try
	{
		SetServer2(url, defaultPort);
		return true;
	}
	catch (const std::runtime_error &e)
	{
		SetError(e.what());
		return false;
	}
}

void C4Network2HTTPClient::SetServer2(const std::string_view serverAddress, const std::uint16_t defaultPort)
{
	// Convert string to CURLU
	std::unique_ptr<CURLU, decltype(&curl_url_cleanup)> url{curl_url(), curl_url_cleanup};
	if (!url)
		throw std::runtime_error{"curl_url failed"};
	if (curl_url_set(url.get(), CURLUPART_URL,
		std::string{serverAddress}.c_str(), CURLU_DEFAULT_SCHEME) != CURLUE_OK)
	{
		throw std::runtime_error{"Malformed URL"};
	}

	// Get host name part
	using UniqueCurlCharPtr = std::unique_ptr<char, decltype(&curl_free)>;
	char *host;
	if (curl_url_get(url.get(), CURLUPART_HOST, &host, 0) != CURLUE_OK)
		throw std::runtime_error{"curl_url_get HOST failed"};
	UniqueCurlCharPtr hostPtr{host, curl_free};

	// Use default port if URL has no port part
	char *port;
	const auto errGetPort = curl_url_get(url.get(), CURLUPART_PORT, &port, 0);
	curl_free(port);
	if (errGetPort == CURLUE_NO_PORT)
	{
		if (curl_url_set(url.get(), CURLUPART_PORT,
			std::to_string(defaultPort).c_str(), 0) != CURLUE_OK)
		{
			throw std::runtime_error{"curl_url_set PORT failed"};
		}
	}
	else if (errGetPort != CURLUE_OK)
	{
		throw std::runtime_error{"curl_url_get PORT failed"};
	}

	// Convert CURLU back to string
	char *urlStr;
	if (curl_url_get(url.get(), CURLUPART_URL, &urlStr, 0) != CURLUE_OK)
		throw std::runtime_error{"curl_url_get URL failed"};
	UniqueCurlCharPtr urlStrPtr{urlStr, curl_free};

	this->url = urlStr;
	serverName = host;
}

void C4Network2HTTPClient::SetNotify(class C4InteractiveThread *thread, const Notify &notify)
{
	this->thread = thread;
	eventData = notify ? decltype(eventData){Ev_MainThread, notify} : decltype(eventData){Ev_HTTP_Response, this};
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
		Comp.setInput(StdStrBuf::MakeRef(resultString.c_str()));
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
		Comp.Value(mkNamingAdapt(mkArrayAdaptMap(rpReferences, rRefCount, mkPtrAdaptNoNull<C4Network2Reference>), "Reference"));
		mkPtrAdaptNoNull<C4Network2Reference>(*rpReferences);
		// Done
		Comp.End();
	}
	catch (const StdCompiler::Exception &e)
	{
		SetError(e.Msg.getData());
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
