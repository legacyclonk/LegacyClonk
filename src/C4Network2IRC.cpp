/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2013-2017, The OpenClonk Team and contributors
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

#include "C4Include.h"
#include "C4Network2IRC.h"
#include "C4Config.h"
#include "C4Version.h"
#include "C4InteractiveThread.h"

#include "C4Gui.h" // for clearly visible error message

#include <cctype> // for isdigit
#include <format>

// Helper for IRC command parameter parsing
StdStrBuf ircExtractPar(const char **ppPar)
{
	// No parameter left?
	if (!ppPar || !*ppPar || !**ppPar)
		return StdStrBuf("");
	// Last parameter?
	StdStrBuf Result;
	if (**ppPar == ':')
	{
		// Reference everything after the double-colon
		Result.Ref(*ppPar + 1);
		*ppPar = nullptr;
	}
	else
	{
		// Copy until next space (or end of string)
		Result.CopyUntil(*ppPar, ' ');
		// Go over parameters
		*ppPar += Result.getLength();
		if (**ppPar == ' ')
			(*ppPar)++;
		else
			*ppPar = nullptr;
	}
	// Done
	return Result;
}

// *** C4Network2IRCUser

C4Network2IRCUser::C4Network2IRCUser(const char *szName)
	: Name(szName) {}

// *** C4Network2IRCChannel

C4Network2IRCChannel::C4Network2IRCChannel(const char *szName)
	: Name(szName), pUsers(nullptr), fReceivingUsers(false) {}

C4Network2IRCChannel::~C4Network2IRCChannel()
{
	ClearUsers();
}

C4Network2IRCUser *C4Network2IRCChannel::getUser(const char *szName) const
{
	for (C4Network2IRCUser *pUser = pUsers; pUser; pUser = pUser->Next)
		if (SEqual(pUser->getName(), szName))
			return pUser;
	return nullptr;
}

void C4Network2IRCChannel::OnUsers(const char *szUsers, const char *szPrefixes)
{
	// Find actual prefixes
	szPrefixes = SSearch(szPrefixes, ")");
	// Reconstructs the list
	if (!fReceivingUsers)
		ClearUsers();
	while (szUsers && *szUsers)
	{
		// Get user name
		StdStrBuf PrefixedName = ircExtractPar(&szUsers);
		// Remove prefix(es)
		const char *szName = PrefixedName.getData();
		if (szPrefixes)
			while (strchr(szPrefixes, *szName))
				szName++;
		// Copy prefix
		StdStrBuf Prefix;
		Prefix.Copy(PrefixedName.getData(), szName - PrefixedName.getData());
		// Add user
		AddUser(szName)->SetPrefix(Prefix.getData());
	}
	// Set flag the user list won't get cleared again until OnUsersEnd is called
	fReceivingUsers = true;
}

void C4Network2IRCChannel::OnUsersEnd()
{
	// Reset flag
	fReceivingUsers = false;
}

void C4Network2IRCChannel::OnTopic(const char *szTopic)
{
	Topic = szTopic;
}

void C4Network2IRCChannel::OnKick(const char *szUser, const char *szComment)
{
	// Remove named user from channel list
	C4Network2IRCUser *pUser = getUser(szUser);
	if (pUser) DeleteUser(pUser);
}

void C4Network2IRCChannel::OnPart(const char *szUser, const char *szComment)
{
	// Remove named user from channel list
	C4Network2IRCUser *pUser = getUser(szUser);
	if (pUser) DeleteUser(pUser);
}

void C4Network2IRCChannel::OnJoin(const char *szUser)
{
	// Add user (do not set prefix)
	if (!getUser(szUser))
		AddUser(szUser);
}

C4Network2IRCUser *C4Network2IRCChannel::AddUser(const char *szName)
{
	// Check if the user already exists
	C4Network2IRCUser *pUser = getUser(szName);
	if (pUser) return pUser;
	// Add to list
	pUser = new C4Network2IRCUser(szName);
	pUser->Next = pUsers;
	pUsers = pUser;
	return pUser;
}

void C4Network2IRCChannel::DeleteUser(C4Network2IRCUser *pUser)
{
	// Unlink
	if (pUser == pUsers)
		pUsers = pUser->Next;
	else
	{
		C4Network2IRCUser *pPrev = pUsers;
		while (pPrev && pPrev->Next != pUser)
			pPrev = pPrev->Next;
		if (pPrev)
			pPrev->Next = pUser->Next;
	}
	// Delete
	delete pUser;
}

void C4Network2IRCChannel::ClearUsers()
{
	while (pUsers)
		DeleteUser(pUsers);
}

// *** C4Network2IRCClient

C4Network2IRCClient::C4Network2IRCClient()
	: fConnecting(false), fConnected(false),
	pChannels(nullptr),
	pLog(nullptr), pLogEnd(nullptr), iLogLength(0), iUnreadLogLength(0),
	pNotify(nullptr) {}

C4Network2IRCClient::~C4Network2IRCClient()
{
	Close();
}

void C4Network2IRCClient::PackPacket(const C4NetIOPacket &rPacket, StdBuf &rOutBuf)
{
	// Enlarge buffer
	int iSize = rPacket.getSize(),
		iPos = rOutBuf.getSize();
	rOutBuf.Grow(iSize + 2);
	// Write packet
	rOutBuf.Write(rPacket, iPos);
	// Terminate
	uint8_t *pPos = rOutBuf.getMPtr<uint8_t>(iPos + iSize);
	*pPos = '\r'; *(pPos + 1) = '\n';
}

size_t C4Network2IRCClient::UnpackPacket(const StdBuf &rInBuf, const C4NetIO::addr_t &addr)
{
	// Find line separation
	const char *pSep = reinterpret_cast<const char *>(memchr(rInBuf.getData(), '\n', rInBuf.getSize()));
	if (!pSep)
		return 0;
	// Check if it's actually correct separation (rarely the case)
	int iSize = pSep - rInBuf.getPtr<char>() + 1,
		iLength = iSize - 1;
	if (iLength && *(pSep - 1) == '\r')
		iLength--;
	// Copy the line
	StdStrBuf Buf; Buf.Copy(rInBuf.getPtr<char>(), iLength);
	// Ignore prefix
	const char *pMsg = Buf.getData();
	StdStrBuf Prefix;
	if (*pMsg == ':')
	{
		Prefix.CopyUntil(pMsg + 1, ' ');
		pMsg += Prefix.getLength() + 1;
	}
	// Strip whitespace
	while (*pMsg == ' ')
		pMsg++;
	// Ignore empty message
	if (!*pMsg)
		return iSize;
	// Get command
	StdStrBuf Cmd; Cmd.CopyUntil(pMsg, ' ');
	// Precess command
	const char *szParameters = SSearch(pMsg, " ");
	OnCommand(Prefix.getData(), Cmd.getData(), szParameters ? szParameters : "");
	// Consume the line
	return iSize;
}

bool C4Network2IRCClient::OnConn(const C4NetIO::addr_t &AddrPeer, const C4NetIO::addr_t &AddrConnect, const addr_t *pOwnAddr, C4NetIO *pNetIO)
{
	// Security checks
	if (!fConnecting || fConnected || AddrConnect != ServerAddr) return false;
	CStdLock Lock(&CSec);
	// Save connection data
	fConnected = true;
	fConnecting = false;
	C4Network2IRCClient::PeerAddr = AddrPeer;
	// Send welcome message
	if (!Password.isNull())
		Send("PASS", Password.getData());
	Send("NICK", Nick.getData());
	Send("USER", std::format("clonk x x :{}", RealName.getData()).c_str());
	// Okay
	return true;
}

void C4Network2IRCClient::OnDisconn(const C4NetIO::addr_t &AddrPeer, C4NetIO *pNetIO, const char *szReason)
{
	fConnected = false;
	// Show a message with the reason
	PushMessage(MSG_Status, "", Nick.getData(), LoadResStr(C4ResStrTableKey::IDS_MSG_DISCONNECTEDFROMSERVER, szReason).c_str());
}

void C4Network2IRCClient::OnPacket(const class C4NetIOPacket &rPacket, C4NetIO *pNetIO)
{
	// Won't get called
}

C4Network2IRCChannel *C4Network2IRCClient::getFirstChannel() const
{
	return pChannels;
}

C4Network2IRCChannel *C4Network2IRCClient::getNextChannel(C4Network2IRCChannel *pPrevChan) const
{
	return pPrevChan ? pPrevChan->Next : pChannels;
}

C4Network2IRCChannel *C4Network2IRCClient::getChannel(const char *szName) const
{
	for (C4Network2IRCChannel *pChan = pChannels; pChan; pChan = pChan->Next)
		if (SEqualNoCase(pChan->getName(), szName))
			return pChan;
	return nullptr;
}

void C4Network2IRCClient::ClearMessageLog()
{
	// Clear log
	while (iLogLength)
		PopMessage();
}

void C4Network2IRCClient::MarkMessageLogRead()
{
	// set read marker to last message
	pLogLastRead = pLogEnd;
	iUnreadLogLength = 0;
	// message buffer is smaller for messages already read: Remove old ones
	while (iLogLength > C4NetIRCMaxReadLogLength)
		PopMessage();
}

bool C4Network2IRCClient::Connect(const char *szServer, const char *szNick, const char *szRealName, const char *szPassword, const char *szAutoJoin)
{
	// Already connected? Close connection
	if (fConnecting || fConnected)
		Close();
	// Initialize
	C4NetIOTCP::SetCallback(this);
	if (!Init())
		return false;

	// Resolve address
	ServerAddr.SetAddress(StdStrBuf(szServer));
	if (ServerAddr.IsNull())
	{
		SetError("Could no resolve server address!"); return false;
	}
	ServerAddr.SetDefaultPort(6666);

	// Set connection data
	Nick = szNick; RealName = szRealName;
	Password = szPassword; AutoJoin = szAutoJoin;
	// Truncate password
	if (Password.getLength() > 31)
		Password.SetLength(31);
	// Start connecting
	if (!C4NetIOTCP::Connect(ServerAddr))
		return false;
	// Reset status data
	Prefixes = "(ov)@+";
	// Okay, let's wait for the connection.
	fConnecting = true;
	return true;
}

bool C4Network2IRCClient::Close()
{
	// Close network
	C4NetIOTCP::Close();
	// Clear channels
	while (pChannels)
		DeleteChannel(pChannels);
	// Clear log
	ClearMessageLog();
	// Reset flags
	fConnected = fConnecting = false;
	return true;
}

bool C4Network2IRCClient::Send(const char *szCommand, const char *szParameters)
{
	if (!fConnected)
	{
		SetError("not connected"); return false;
	}
	// Create message
	std::string msg;
	if (szParameters)
		msg = std::format("{} {}", szCommand, szParameters);
	else
		msg = szCommand;
	// Send
	return C4NetIOTCP::Send(C4NetIOPacket(msg.c_str(), msg.size(), false, PeerAddr));
}

bool C4Network2IRCClient::Quit(const char *szReason)
{
	if (!Send("QUIT", std::format(":{}", szReason).c_str()))
		return false;
	// Must be last message
	return Close();
}

bool C4Network2IRCClient::Join(const char *szChannel)
{
	return Send("JOIN", szChannel);
}

bool C4Network2IRCClient::Part(const char *szChannel)
{
	return Send("PART", szChannel);
}

bool C4Network2IRCClient::Message(const char *szTarget, const char *szText)
{
	if (!Send("PRIVMSG", std::format("{} :{}", szTarget, szText).c_str()))
		return false;
	PushMessage(MSG_Message, Nick.getData(), szTarget, szText);
	return true;
}

bool C4Network2IRCClient::Notice(const char *szTarget, const char *szText)
{
	if (!Send("NOTICE", std::format("{} :{}", szTarget, szText).c_str()))
		return false;
	PushMessage(MSG_Notice, Nick.getData(), szTarget, szText);
	return true;
}

bool C4Network2IRCClient::Action(const char *szTarget, const char *szText)
{
	if (!Send("PRIVMSG", std::format("{} :\1ACTION {}\1", szTarget, szText).c_str()))
		return false;
	PushMessage(MSG_Action, Nick.getData(), szTarget, szText);
	return true;
}

bool C4Network2IRCClient::ChangeNick(const char *szNewNick)
{
	return Send("NICK", szNewNick);
}

void C4Network2IRCClient::OnCommand(const char *szSender, const char *szCommand, const char *szParameters)
{
	CStdLock Lock(&CSec);
	// Numeric command?
	if (isdigit(static_cast<unsigned char>(*szCommand)) && SLen(szCommand) == 3)
	{
		OnNumericCommand(szSender, atoi(szCommand), szParameters);
		return;
	}
	// Sender's nick
	StdStrBuf SenderNick;
	if (szSender) SenderNick.CopyUntil(szSender, '!');
	// Ping?
	if (SEqualNoCase(szCommand, "PING"))
		Send("PONG", szParameters);
	// Message?
	if (SEqualNoCase(szCommand, "NOTICE") || SEqualNoCase(szCommand, "PRIVMSG"))
	{
		// Get target
		StdStrBuf Target = ircExtractPar(&szParameters);
		// Get text
		StdStrBuf Text = ircExtractPar(&szParameters);
		// Process message
		OnMessage(SEqualNoCase(szCommand, "NOTICE"), szSender, Target.getData(), Text.getData());
	}
	// Channel join?
	if (SEqualNoCase(szCommand, "JOIN"))
	{
		// Get channel
		StdStrBuf Channel = ircExtractPar(&szParameters);
		C4Network2IRCChannel *pChan = AddChannel(Channel.getData());
		// Add user
		pChan->OnJoin(SenderNick.getData());
		// Myself?
		if (SenderNick == Nick)
			PushMessage(MSG_Status, szSender, Channel.getData(), LoadResStr(C4ResStrTableKey::IDS_MSG_YOUHAVEJOINEDCHANNEL, Channel.getData()).c_str());
		else
			PushMessage(MSG_Status, szSender, Channel.getData(), LoadResStr(C4ResStrTableKey::IDS_MSG_HASJOINEDTHECHANNEL, SenderNick.getData()).c_str());
	}
	// Channel part?
	if (SEqualNoCase(szCommand, "PART"))
	{
		// Get channel
		StdStrBuf Channel = ircExtractPar(&szParameters);
		C4Network2IRCChannel *pChan = AddChannel(Channel.getData());
		// Get message
		StdStrBuf Comment = ircExtractPar(&szParameters);
		// Remove user
		pChan->OnPart(SenderNick.getData(), Comment.getData());
		// Myself?
		if (SenderNick == Nick)
		{
			DeleteChannel(pChan);
			PushMessage(MSG_Status, szSender, Nick.getData(), LoadResStr(C4ResStrTableKey::IDS_MSG_YOUHAVELEFTCHANNEL, Channel.getData(), Comment.getData()).c_str());
		}
		else
			PushMessage(MSG_Status, szSender, Channel.getData(), LoadResStr(C4ResStrTableKey::IDS_MSG_HASLEFTTHECHANNEL, SenderNick.getData(), Comment.getData()).c_str());
	}
	// Kick?
	if (SEqualNoCase(szCommand, "KICK"))
	{
		// Get channel
		StdStrBuf Channel = ircExtractPar(&szParameters);
		C4Network2IRCChannel *pChan = AddChannel(Channel.getData());
		// Get kicked user
		StdStrBuf Kicked = ircExtractPar(&szParameters);
		// Get message
		StdStrBuf Comment = ircExtractPar(&szParameters);
		// Remove user
		pChan->OnKick(Kicked.getData(), Comment.getData());
		// Myself?
		if (Kicked == Nick)
		{
			DeleteChannel(pChan);
			PushMessage(MSG_Status, szSender, Nick.getData(), LoadResStr(C4ResStrTableKey::IDS_MSG_YOUWEREKICKEDFROMCHANNEL, Channel.getData(), Comment.getData()).c_str());
		}
		else
			PushMessage(MSG_Status, szSender, Channel.getData(), LoadResStr(C4ResStrTableKey::IDS_MSG_WASKICKEDFROMTHECHANNEL, Kicked.getData(), Comment.getData()).c_str());
	}
	// Quit?
	if (SEqualNoCase(szCommand, "QUIT"))
	{
		// Get comment
		StdStrBuf Comment = ircExtractPar(&szParameters);
		// Format status message
		const std::string message{LoadResStr(C4ResStrTableKey::IDS_MSG_HASDISCONNECTED, SenderNick.getData(), Comment.getData())};
		// Remove him from all channels
		for (C4Network2IRCChannel *pChan = pChannels; pChan; pChan = pChan->Next)
			if (pChan->getUser(SenderNick.getData()))
			{
				pChan->OnPart(SenderNick.getData(), "Quit");
				PushMessage(MSG_Status, szSender, pChan->getName(), message.c_str());
			}
	}
	// Topic change?
	if (SEqualNoCase(szCommand, "TOPIC"))
	{
		// Get channel and topic
		StdStrBuf Channel = ircExtractPar(&szParameters);
		StdStrBuf Topic = ircExtractPar(&szParameters);
		// Set topic
		AddChannel(Channel.getData())->OnTopic(Topic.getData());
		// Message
		PushMessage(MSG_Status, szSender, Channel.getData(), LoadResStr(C4ResStrTableKey::IDS_MSG_CHANGESTHETOPICTO, SenderNick.getData(), Topic.getData()).c_str());
	}
	// Mode?
	if (SEqualNoCase(szCommand, "MODE"))
	{
		// Get all data
		StdStrBuf Channel = ircExtractPar(&szParameters);
		StdStrBuf Flags = ircExtractPar(&szParameters);
		StdStrBuf What = ircExtractPar(&szParameters);
		// Make sure it's a channel
		C4Network2IRCChannel *pChan = getChannel(Channel.getData());
		if (pChan)
			// Ask for names, because user prefixes might be out of sync
			Send("NAMES", Channel.getData());
		// Show Message
		PushMessage(MSG_Status, szSender, Channel.getData(), LoadResStr(C4ResStrTableKey::IDS_MSG_SETSMODE, SenderNick.getData(), Flags.getData(), What.getData()).c_str());
	}
	// Error?
	if (SEqualNoCase(szCommand, "ERROR"))
	{
		// Get message
		StdStrBuf Message = ircExtractPar(&szParameters);
		// Push it
		PushMessage(MSG_Server, szSender, Nick.getData(), Message.getData());
	}
	// Nickchange?
	if (SEqualNoCase(szCommand, "NICK"))
	{
		// Get new nick
		StdStrBuf NewNick = ircExtractPar(&szParameters);
		// Format status message
		const std::string message{LoadResStr(C4ResStrTableKey::IDS_MSG_ISNOWKNOWNAS, SenderNick.getData(), NewNick.getData())};
		// Rename on all channels
		for (C4Network2IRCChannel *pChan = pChannels; pChan; pChan = pChan->Next)
			if (pChan->getUser(SenderNick.getData()))
			{
				pChan->OnPart(SenderNick.getData(), "Nickchange");
				pChan->OnJoin(NewNick.getData());
				PushMessage(MSG_Status, szSender, pChan->getName(), message.c_str());
			}
		// Self?
		if (SenderNick == Nick)
			Nick = NewNick;
	}
}

void C4Network2IRCClient::OnNumericCommand(const char *szSender, int iCommand, const char *szParameters)
{
	bool fShowMessage = true;
	// Get target
	StdStrBuf Target = ircExtractPar(&szParameters);
	// Handle command
	switch (iCommand)
	{
	case 433: // Nickname already in use
	{
		StdStrBuf DesiredNick = ircExtractPar(&szParameters);
		// Automatically try to choose a new one
		DesiredNick.AppendChar('_');
		Send("NICK", DesiredNick.getData());
		break;
	}

	case 376: // End of MOTD
	case 422: // MOTD missing
		// Let's take this a sign that the connection is established.
		OnConnected();
		break;

	case 331: // No topic set
	case 332: // Topic notify / change
	{
		// Get Channel name and topic
		StdStrBuf Channel = ircExtractPar(&szParameters);
		StdStrBuf Topic = (iCommand == 332 ? ircExtractPar(&szParameters) : StdStrBuf(""));
		// Set it
		AddChannel(Channel.getData())->OnTopic(Topic.getData());
		// Log
		if (Topic.getLength())
			PushMessage(MSG_Status, szSender, Channel.getData(), LoadResStr(C4ResStrTableKey::IDS_MSG_TOPICIN, Channel.getData(), Topic.getData()).c_str());
	}
	break;

	case 333: // Last topic change
		fShowMessage = false; // ignore
		break;

	case 353: // Names in channel
	{
		// Get Channel name and name list
		StdStrBuf Junk = ircExtractPar(&szParameters); // ??!
		StdStrBuf Channel = ircExtractPar(&szParameters);
		StdStrBuf Names = ircExtractPar(&szParameters);
		// Set it
		AddChannel(Channel.getData())->OnUsers(Names.getData(), Prefixes.getData());
		fShowMessage = false;
	}
	break;

	case 366: // End of names list
	{
		// Get Channel name
		StdStrBuf Channel = ircExtractPar(&szParameters);
		// Finish
		AddChannel(Channel.getData())->OnUsersEnd();
		fShowMessage = false;
		// Notify
		if (pNotify) pNotify->PushEvent(Ev_IRC_Message, this);
	}
	break;

	case 4: // Server version
		fShowMessage = false; // ignore
		break;

	case 5: // Server support string
	{
		while (szParameters && *szParameters)
		{
			// Get support-token.
			StdStrBuf Token = ircExtractPar(&szParameters);
			StdStrBuf Parameter; Parameter.CopyUntil(Token.getData(), '=');
			// Check if it's interesting and safe data if so.
			if (SEqualNoCase(Parameter.getData(), "PREFIX"))
				Prefixes.Copy(SSearch(Token.getData(), "="));
		}
		fShowMessage = false;
	}
	break;
	}
	// Show embedded message, if any?
	if (fShowMessage)
	{
		// Check if first parameter is some sort of channel name
		C4Network2IRCChannel *pChannel = nullptr;
		if (szParameters && *szParameters && *szParameters != ':')
			pChannel = getChannel(ircExtractPar(&szParameters).getData());
		// Go over other parameters
		const char *pMsg = szParameters;
		while (pMsg && *pMsg && *pMsg != ':')
			pMsg = SSearch(pMsg, " ");
		// Show it
		if (pMsg && *pMsg)
			if (!pChannel)
				PushMessage(MSG_Server, szSender, Nick.getData(), pMsg + 1);
			else
				PushMessage(MSG_Status, szSender, pChannel->getName(), pMsg + 1);
	}
}

void C4Network2IRCClient::OnConnected()
{
	// Set flag
	fConnected = true;

	// Try to join channel(s)
	if (AutoJoin.getLength())
		Join(AutoJoin.getData());
}

void C4Network2IRCClient::OnMessage(bool fNotice, const char *szSender, const char *szTarget, const char *szText)
{
	// Find channel, if not private.
	C4Network2IRCChannel *pChan = nullptr;
	if (!SEqualNoCase(szTarget, Nick.getData()))
		pChan = getChannel(szTarget);

	// CTCP tagged data?
	const char X_DELIM = '\001';
	if (szText[0] == X_DELIM)
	{
		// Process messages (it's very rarely more than one, but the spec allows it)
		const char *pMsg = szText + 1;
		while (*pMsg)
		{
			// Find end
			const char *pEnd = strchr(pMsg, X_DELIM);
			if (!pEnd) pEnd = pMsg + SLen(pMsg);
			// Copy CTCP query/reply, get tag
			StdStrBuf CTCP; CTCP.Copy(pMsg, pEnd - pMsg);
			StdStrBuf Tag; Tag.CopyUntil(CTCP.getData(), ' ');
			const char *szData = SSearch(CTCP.getData(), " ");
			if (!szData) szData = "";
			StdStrBuf Sender; Sender.CopyUntil(szSender, '!');
			// Process
			if (SEqualNoCase(Tag.getData(), "ACTION"))
				PushMessage(MSG_Action, szSender, szTarget, szData);
			if (SEqualNoCase(Tag.getData(), "VERSION") && !fNotice)
				Send("NOTICE", std::format("{} :{}VERSION " C4ENGINECAPTION ":" C4VERSION ":" C4_OS "{}",
					Sender.getData(), X_DELIM, X_DELIM).c_str());
			if (SEqualNoCase(Tag.getData(), "PING") && !fNotice)
				Send("NOTICE", std::format("{} :{}PING {}{}",
					Sender.getData(), X_DELIM, szData, X_DELIM).c_str());
			// Get next message
			pMsg = pEnd;
			if (*pMsg == X_DELIM) pMsg++;
		}
	}

	// Standard message (not CTCP tagged): Push
	else
		PushMessage(fNotice ? MSG_Notice : MSG_Message, szSender, szTarget, szText);
}

void C4Network2IRCClient::PopMessage()
{
	if (!iLogLength) return;
	// Unlink message
	C4Network2IRCMessage *pMsg = pLog;
	pLog = pMsg->Next;
	if (!pLog) pLogEnd = nullptr;
	if (pLogLastRead == pMsg) pLogLastRead = nullptr;
	// Delete it
	delete pMsg;
	iLogLength--;
	if (iUnreadLogLength > iLogLength) iUnreadLogLength = iLogLength;
}

void C4Network2IRCClient::PushMessage(C4Network2IRCMessageType eType, const char *szSource, const char *szTarget, const char *szText)
{
	// Create message
	C4Network2IRCMessage *pMsg = new C4Network2IRCMessage(eType, szSource, szTarget, szText);
	// Add to list
	if (pLogEnd)
	{
		pLogEnd->Next = pMsg;
	}
	else
	{
		pLog = pMsg;
	}
	pLogEnd = pMsg;
	// Count
	iLogLength++;
	iUnreadLogLength++;
	while (iLogLength > C4NetIRCMaxLogLength)
		PopMessage();
	// Notify
	if (pNotify)
		pNotify->PushEvent(Ev_IRC_Message, this);
}

C4Network2IRCChannel *C4Network2IRCClient::AddChannel(const char *szName)
{
	// Already exists?
	C4Network2IRCChannel *pChan = getChannel(szName);
	if (pChan) return pChan;
	// Create new, insert
	pChan = new C4Network2IRCChannel(szName);
	pChan->Next = pChannels;
	pChannels = pChan;
	return pChan;
}

void C4Network2IRCClient::DeleteChannel(C4Network2IRCChannel *pChannel)
{
	// Unlink
	if (pChannel == pChannels)
		pChannels = pChannel->Next;
	else
	{
		C4Network2IRCChannel *pPrev = pChannels;
		while (pPrev && pPrev->Next != pChannel)
			pPrev = pPrev->Next;
		if (pPrev)
			pPrev->Next = pChannel->Next;
	}
	// Delete
	delete pChannel;
}
