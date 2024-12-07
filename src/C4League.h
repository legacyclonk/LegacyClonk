/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
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

/* engine handler of league system */

#pragma once

#include <C4EnumInfo.h>
#include <C4Network2Reference.h>
#include <C4Gui.h>

#include <StdSha1.h>

#define C4League_Name_Valid_Characters "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD9\xDA\xDB\xDC\xDD\xDF\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF\x20\x2E\x2D\x5F"

// maximum league count a game can run in
const int32_t C4NetMaxLeagues = 10;

enum C4LeagueAction : std::uint8_t
{
	C4LA_None,             // no action

	C4LA_Start,            // [host]   Game registration
	C4LA_Update,           // [host]   Game status update (heartbeat)
	C4LA_End,              // [host]   Final game status
	C4LA_PlrAuthCheck,     // [host]   Player authentication check

	C4LA_RefQuery,         // [client] Query reference list
	C4LA_PlrAuth,          // [client] Player authentication request

	C4LA_ReportDisconnect, // [both]   Sent by host and client when a client is disconnected irregularly
};

template<>
struct C4EnumInfo<C4LeagueAction>
{
	static inline constexpr auto data = mkEnumInfo<C4LeagueAction>("C4LA_",
		{
			{ C4LA_Start,        "Start"  },
			{ C4LA_Update,       "Update" },
			{ C4LA_End,          "End" },
			{ C4LA_PlrAuthCheck, "Join" },

			{ C4LA_RefQuery, "" },
			{ C4LA_PlrAuth,  "Auth" },

			{ C4LA_ReportDisconnect, "ReportDisconnect" }
		}
	);
};

class C4LeagueRequestHead
{
public:
	C4LeagueRequestHead(C4LeagueAction eAction, const char *szCSID = "", const char *szAUID = "")
		: eAction(eAction), CSID(szCSID), AUID(szAUID) {}

private:
	C4LeagueAction eAction;
	StdStrBuf CSID;
	StdStrBuf AUID;
	StdStrBuf Checksum;

	// Auth
	StdStrBuf Account;
	StdStrBuf Password;
	StdStrBuf NewAccount;
	StdStrBuf NewPassword;

public:
	void SetChecksum(const char *szChecksum) { Checksum = szChecksum; }
	void SetAuth(const char *szAccount, const char *szPassword);
	void SetNewAccount(const char *szNewAccount);
	void SetNewPassword(const char *szNewPassword);

	void CompileFunc(StdCompiler *pComp);
};

class C4LeagueReportDisconnectHead : public C4LeagueRequestHead
{
private:
	C4LeagueDisconnectReason eReason;

public:
	C4LeagueReportDisconnectHead(const char *szCSID, C4LeagueDisconnectReason eReason) : eReason(eReason), C4LeagueRequestHead(C4LA_ReportDisconnect, szCSID, nullptr) {}

public:
	void CompileFunc(StdCompiler *pComp);
};

class C4LeagueRequestHeadEnd : public C4LeagueRequestHead
{
public:
	C4LeagueRequestHeadEnd(C4LeagueAction eAction, const char *szCSID, const char *szRecordName = nullptr, const uint8_t *pRecordSHA = nullptr)
		: C4LeagueRequestHead(eAction, szCSID), RecordName(szRecordName)
	{
		if (pRecordSHA)
			memcpy(RecordSHA, pRecordSHA, StdSha1::DigestLength);
	}

private:
	StdStrBuf RecordName;
	uint8_t RecordSHA[StdSha1::DigestLength];

public:
	void CompileFunc(StdCompiler *pComp);
};

class C4LeagueResponseHead
{
public:
	C4LeagueResponseHead() {}

private:
	StdStrBuf Status;
	StdStrBuf CSID;
	StdStrBuf Message;

	// Auth
	StdStrBuf Account;
	StdStrBuf AUID;
	StdStrBuf FBID;

public:
	const char *getCSID() const { return CSID.getData(); }
	const char *getMessage() const { return Message.getData(); }
	bool isSuccess() const { return SEqualNoCase(Status.getData(), "Success"); }
	bool isStatusRegister() const { return SEqualNoCase(Status.getData(), "Register"); }
	const char *getAccount() const { return Account.getData(); }
	const char *getAUID() const { return AUID.getData(); }
	const char *getFBID() const { return FBID.getData(); }

	void CompileFunc(StdCompiler *pComp);
};

class C4LeagueResponseHeadStart : public C4LeagueResponseHead
{
private:
	StdStrBuf League;
	StdStrBuf StreamingAddr;
	int32_t fHaveSeed;
	int32_t iSeed;
	int32_t iMaxPlayers;

public:
	const char *getLeague() const { return League.getData(); }
	const char *getStreamingAddr() const { return StreamingAddr.getData(); }
	bool haveSeed() const { return !!fHaveSeed; }
	int32_t getSeed() const { return iSeed; }
	int32_t getMaxPlayers() const { return iMaxPlayers; }

	void CompileFunc(StdCompiler *pComp);
};

class C4LeagueResponseHeadUpdate : public C4LeagueResponseHead
{
private:
	StdStrBuf League;
	C4ClientPlayerInfos PlrInfos;

public:
	const char *getLeague() const { return League.getData(); }
	const C4ClientPlayerInfos &GetPlrInfos() const { return PlrInfos; }

	void CompileFunc(StdCompiler *pComp);
};

class C4LeagueResponseHeadAuthCheck : public C4LeagueResponseHead
{
private:
	StdStrBuf Leagues[C4NetMaxLeagues];
	int32_t Scores[C4NetMaxLeagues];
	int32_t Ranks[C4NetMaxLeagues];
	int32_t RankSymbols[C4NetMaxLeagues];
	StdStrBuf ProgressData[C4NetMaxLeagues];
	StdStrBuf ClanTag;

public:
	int32_t getScore(const char *szLeague) const;
	int32_t getRank(const char *szLeague) const;
	int32_t getRankSymbol(const char *szLeague) const;
	const char *getProgressData(const char *szLeague) const;
	const char *getClanTag() const { return ClanTag.getData(); }

	void CompileFunc(StdCompiler *pComp);
};

// a linked list of all known feedback IDs, associated to player IDs
class C4LeagueFBIDList
{
private:
	struct FBIDItem
	{
		StdStrBuf Account;
		StdStrBuf FBID;
		FBIDItem *pNext;
	} *pFirst;

public:
	C4LeagueFBIDList() : pFirst(nullptr) {}
	~C4LeagueFBIDList() { Clear(); }
	void Clear();
	void RemoveFBIDByAccount(const char *szAccount);
	bool FindFBIDByAccount(const char *szAccount, StdStrBuf *pFBIDOut);

	void AddFBID(const char *szFBID, const char *szAccount);
};

class C4LeagueClient : public C4Network2RefClient
{
private:
	StdStrBuf CSID;
	C4LeagueAction eCurrAction;
	C4LeagueFBIDList FBIDList;

public:
	C4LeagueClient() : CSID(), eCurrAction(C4LA_None), C4Network2RefClient() {}
	const char *getCSID() const { return CSID.getData(); }
	C4LeagueAction getCurrentAction() const { return eCurrAction; }
	void ResetCurrentAction() { eCurrAction = C4LA_None; }

	// Action "Start"
	bool Start(const C4Network2Reference &Ref);
	bool GetStartReply(StdStrBuf *pMessage, StdStrBuf *pLeague, StdStrBuf *pStreamingAddr, int32_t *pSeed, int32_t *pMaxPlayers);

	// Action "Update"
	bool Update(const C4Network2Reference &Ref);
	bool GetUpdateReply(StdStrBuf *pMessage, C4ClientPlayerInfos *pPlayerLeagueInfos);

	// Action "End"
	bool End(const C4Network2Reference &Ref, const char *szRecordName, const uint8_t *pRecordSHA);
	bool GetEndReply(StdStrBuf *pMessage, class C4RoundResultsPlayers *pRoundResults);

	// Action "Auth"
	bool Auth(const C4PlayerInfo &PlrInfo, const char *szAccount, const char *szPassword, const char *szNewAccount = nullptr, const char *szNewPassword = nullptr);
	bool GetAuthReply(StdStrBuf *pMessage, StdStrBuf *pAUID, StdStrBuf *pAccount, bool *pRegister);

	// Action "Join"
	bool AuthCheck(const C4PlayerInfo &PlrInfo);
	bool GetAuthCheckReply(StdStrBuf *pMessage, const char *szLeague, class C4PlayerInfo *pPlrInfo);

	// Action "Disconnect" - sent by host and client when one or more clients are disconnected irregularly
	bool ReportDisconnect(const C4ClientPlayerInfos &rSendPlayerFBIDs, C4LeagueDisconnectReason eReason);
	bool GetReportDisconnectReply(StdStrBuf *pMessage);

private:
	void ModifyForChecksum(std::string &data, const char *replace);
	void ModifyForChecksum(const void *pData, size_t iDataSize, char *pReplace, uint32_t iChecksum, uint32_t iCheckMask);
};

// dialog shown for each participating player to enter league authentication data
class C4LeagueSignupDialog : public C4GUI::Dialog
{
private:
	C4GUI::CheckBox *pChkPassword;
	C4GUI::LabeledEdit *pEdtAccount, *pEdtPass, *pEdtPass2;
	C4GUI::Button *pBtnOK, *pBtnAbort;
	int32_t iEdtPassSpace;
	StdStrBuf strPlayerName;

public:
	C4LeagueSignupDialog(const char *szPlayerName, const char *szLeagueName, const char *szLeagueServerName, const char *szAccountPref, const char *szPassPref, bool fWarnThirdParty, bool fRegister);
	~C4LeagueSignupDialog() {}

	const char *GetAccount();
	bool HasPass();
	const char *GetPass();

	// check for errors (overridden)
	virtual void UserClose(bool fOK) override;

	// show modal league dialog to query password for player; return
	static bool ShowModal(const char *szPlayerName, const char *szLeagueName, const char *szLeagueServerName, StdStrBuf *psAccount, StdStrBuf *psPass, bool fWarnThirdParty, bool fRegister);

private:
	void OnChkPassword();
};
