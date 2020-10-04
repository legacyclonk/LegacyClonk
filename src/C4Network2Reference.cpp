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
	const char *szCharset = GetCharsetCodeName(Config.General.LanguageCharset);
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

C4Network2HTTPClient::~C4Network2HTTPClient()
{
	Cancel({});
}

bool C4Network2HTTPClient::Init() try
{
	client.emplace();
	return true;
}
catch (const std::runtime_error &e)
{
	SetError(e.what());
	return false;
}

bool C4Network2HTTPClient::Query(const StdBuf &Data, const bool binary)
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

		const auto progressCallback = [this](const std::size_t downloadedSize, const std::size_t totalSize)
		{
			this->downloadedSize.store(downloadedSize, std::memory_order_release);
			this->totalSize.store(totalSize, std::memory_order_release);
			return true;
		};

		task = QueryAsync(this->data.isNull() ? client->GetAsync(std::move(request), progressCallback) : client->PostAsync(std::move(request), progressCallback));
		return true;
	}
	catch (const std::runtime_error &e)
	{
		SetError(e.what());
		return false;
	}
}

bool C4Network2HTTPClient::Cancel(const std::string_view reason)
{
	if (task)
	{
		task.Cancel();

		try
		{
			std::move(task).Get();
		}
		catch (const C4Task::CancelledException &)
		{
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

void C4Network2HTTPClient::Clear()
{
	Cancel({});
	resultBin.Clear();
	resultString.clear();
	ResetError();
	serverAddress.Clear();
	url.clear();
}

bool C4Network2HTTPClient::SetServer(const std::string_view serverAddress, const std::uint16_t defaultPort) try
{
	const C4HTTPClient::Uri uri{std::string{serverAddress}, defaultPort};
	url = uri.GetUriAsString();
	serverName = uri.GetServerAddress();
	return true;
}
catch (const std::runtime_error &e)
{
	SetError(e.what());
	return false;
}

C4Task::Hot<void> C4Network2HTTPClient::QueryAsync(C4Task::Hot<C4HTTPClient::Result> &&queryTask)
{
	busy.store(true, std::memory_order_release);

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

		const std::lock_guard lock{dataMutex};
		if (binary)
		{
			resultBin = std::move(result.Buffer);
		}
		else
		{
			resultString = {reinterpret_cast<const char *>(result.Buffer.getData()), result.Buffer.getSize()};
		}

		success.store(true, std::memory_order_release);
		serverAddress = result.ServerAddress;

		if (auto *const thread = this->thread.load(std::memory_order_acquire); thread)
		{
			thread->PushEvent(Ev_HTTP_Response, this);
		}
	}
	catch (const std::runtime_error &e)
	{
		const std::lock_guard lock{dataMutex};
		SetError(e.what());
		success.store(false, std::memory_order_release);
	}
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
		Comp.setInput(StdStrBuf::MakeRef(getResultString().c_str()));
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
