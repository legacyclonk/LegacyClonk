/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
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

/* Control packets contain all player input in the message queue */

#pragma once

#include "C4AulScriptStrict.h"
#include "C4ForwardDeclarations.h"
#include "C4Id.h"
#include "C4PacketBase.h"
#include "C4PlayerInfo.h"
#include "C4Client.h"

#include <format>
#include <string>

class C4Record;

// *** control base classes

class C4ControlPacket : public C4PacketBase
{
public:
	C4ControlPacket();
	~C4ControlPacket();

protected:
	int32_t iByClient;

public:
	int32_t getByClient() const { return iByClient; }
	bool LocalControl() const;
	bool HostControl() const { return iByClient == C4ClientIDHost; }

	void SetByClient(int32_t iByClient);

	virtual bool PreExecute(const std::shared_ptr<spdlog::logger> &logger) const { return true; }
	virtual void Execute(const std::shared_ptr<spdlog::logger> &logger) const = 0;
	virtual void PreRec(C4Record *pRecord) {}

	// allowed in lobby (without dynamic loaded)?
	virtual bool Lobby() const { return false; }
	// allowed as direct/private control?
	virtual bool Sync() const { return true; }

	virtual void CompileFunc(StdCompiler *pComp) override;
};

#define DECLARE_C4CONTROL_VIRTUALS \
	virtual void Execute(const std::shared_ptr<spdlog::logger> &logger) const override; \
	virtual void CompileFunc(StdCompiler *pComp) override;

class C4Control : public C4PacketBase
{
public:
	C4Control();
	~C4Control();

protected:
	C4PacketList Pkts;

public:
	void Clear();

	// packet list wrappers
	C4IDPacket *firstPkt() const { return Pkts.firstPkt(); }
	C4IDPacket *nextPkt(C4IDPacket *pPkt) const { return Pkts.nextPkt(pPkt); }

	void Add(C4PacketType eType, C4ControlPacket *pCtrl) { Pkts.Add(eType, pCtrl); }

	void Take(C4Control &Ctrl) { Pkts.Take(Ctrl.Pkts); }
	void Append(const C4Control &Ctrl) { Pkts.Append(Ctrl.Pkts); }
	void Copy(const C4Control &Ctrl) { Clear(); Pkts.Append(Ctrl.Pkts); }
	void Remove(C4IDPacket *pPkt) { Pkts.Remove(pPkt); }
	void Delete(C4IDPacket *pPkt) { Pkts.Delete(pPkt); }

	// control execution
	bool PreExecute(const std::shared_ptr<spdlog::logger> &logger) const;
	void Execute(const std::shared_ptr<spdlog::logger> &logger) const;
	void PreRec(C4Record *pRecord) const;

	virtual void CompileFunc(StdCompiler *pComp) override;
};

// *** control packets

enum C4CtrlValueType
{
	C4CVT_None = -1,
	C4CVT_ControlRate = 0,
	C4CVT_DisableDebug = 1,
	C4CVT_MaxPlayer = 2,
	C4CVT_TeamDistribution = 3,
	C4CVT_TeamColors = 4,
	C4CVT_FairCrew = 5,
};

class C4ControlSet : public C4ControlPacket // sync, lobby
{
public:
	C4ControlSet()
		: eValType(C4CVT_None), iData(0) {}
	C4ControlSet(C4CtrlValueType eValType, int32_t iData)
		: eValType(eValType), iData(iData) {}

protected:
	C4CtrlValueType eValType;
	int32_t iData;

public:
	// C4CVT_TeamDistribution and C4CVT_TeamColors are lobby-packets
	virtual bool Lobby() const override { return eValType == C4CVT_TeamDistribution || eValType == C4CVT_TeamColors; }

	DECLARE_C4CONTROL_VIRTUALS
};

class C4ControlScript : public C4ControlPacket // sync
{
public:
	enum { SCOPE_Console = -2, SCOPE_Global = -1, }; // special scopes to be passed as target objects

	C4ControlScript()
		: iTargetObj(-1) {}
	C4ControlScript(const std::uint32_t sectionNumber, const char *szScript, int32_t iTargetObj = SCOPE_Global, C4AulScriptStrict strict = C4AulScriptStrict::MAXSTRICT)
		: sectionNumber{sectionNumber}, iTargetObj(iTargetObj), Strict{strict}, Script(szScript, true) {}

protected:
	std::uint32_t sectionNumber;
	int32_t iTargetObj;
	C4AulScriptStrict Strict;
	StdStrBuf Script;

public:
	void SetTargetObj(int32_t iObj) { iTargetObj = iObj; }
	DECLARE_C4CONTROL_VIRTUALS

public:
	static void CheckStrictness(const C4AulScriptStrict strict, StdCompiler &comp);
};

class C4ControlPlayerSelect : public C4ControlPacket // sync
{
public:
	C4ControlPlayerSelect()
		: iPlr(-1), iObjCnt(0), pObjNrs(nullptr) {}
	C4ControlPlayerSelect(int32_t iPlr, const C4ObjectList &Objs);
	~C4ControlPlayerSelect() { delete[] pObjNrs; }

protected:
	int32_t iPlr;
	int32_t iObjCnt;
	int32_t *pObjNrs;

public:
	DECLARE_C4CONTROL_VIRTUALS
};

class C4ControlPlayerControl : public C4ControlPacket // sync
{
public:
	C4ControlPlayerControl()
		: iPlr(-1), iCom(-1), iData(-1) {}
	C4ControlPlayerControl(int32_t iPlr, int32_t iCom, int32_t iData)
		: iPlr(iPlr), iCom(iCom), iData(iData) {}

protected:
	int32_t iPlr, iCom, iData;

public:
	DECLARE_C4CONTROL_VIRTUALS
};

class C4ControlPlayerCommand : public C4ControlPacket // sync
{
public:
	C4ControlPlayerCommand()
		: iPlr(-1), iCmd(-1) {}
	C4ControlPlayerCommand(int32_t iPlr, int32_t iCmd, int32_t iX, int32_t iY,
		C4Object *pTarget, C4Object *pTarget2, int32_t iData, int32_t iAddMode);

protected:
	int32_t iPlr, iCmd, iX, iY, iTarget, iTarget2, iData, iAddMode;

public:
	DECLARE_C4CONTROL_VIRTUALS
};

class C4ControlSyncCheck : public C4ControlPacket // not sync
{
public:
	C4ControlSyncCheck();

protected:
	int32_t Frame;
	int32_t ControlTick;
	int32_t Random3;
	int32_t RandomCount;
	int32_t AllCrewPosX;
	int32_t PXSCount;
	int32_t MassMoverIndex;
	int32_t ObjectCount;
	int32_t ObjectEnumerationIndex;
	int32_t SectShapeSum;

public:
	void Set();
	int32_t getFrame() const { return Frame; }
	virtual bool Sync() const override { return false; }
	DECLARE_C4CONTROL_VIRTUALS

protected:
	static int32_t GetAllCrewPosX();
};

class C4ControlSynchronize : public C4ControlPacket // sync
{
public:
	C4ControlSynchronize(bool fSavePlrFiles = false, bool fSyncClearance = false)
		: fSavePlrFiles(fSavePlrFiles), fSyncClearance(fSyncClearance) {}

protected:
	bool fSavePlrFiles, fSyncClearance;

public:
	DECLARE_C4CONTROL_VIRTUALS
};

class C4ControlClientJoin : public C4ControlPacket // not sync, lobby
{
public:
	C4ControlClientJoin() {}
	C4ControlClientJoin(const C4ClientCore &Core) : Core(Core) {}

public:
	C4ClientCore Core;

public:
	virtual bool Sync() const override { return false; }
	virtual bool Lobby() const override { return true; }
	DECLARE_C4CONTROL_VIRTUALS
};

enum C4ControlClientUpdType
{
	CUT_None = -1, CUT_Activate = 0, CUT_SetObserver = 1
};

class C4ControlClientUpdate : public C4ControlPacket // sync, lobby
{
public:
	C4ControlClientUpdate() {}
	C4ControlClientUpdate(int32_t iID, C4ControlClientUpdType eType, int32_t iData = 0)
		: iID(iID), eType(eType), iData(iData) {}

public:
	int32_t iID;
	C4ControlClientUpdType eType;
	int32_t iData;

public:
	virtual bool Sync() const override { return false; }
	virtual bool Lobby() const override { return true; }
	DECLARE_C4CONTROL_VIRTUALS
};

class C4ControlClientRemove : public C4ControlPacket // not sync, lobby
{
public:
	C4ControlClientRemove() {}
	C4ControlClientRemove(int32_t iID, const char *szReason = "") : iID(iID), strReason(szReason) {}

public:
	int32_t iID;
	StdStrBuf strReason;

public:
	virtual bool Sync() const override { return false; }
	virtual bool Lobby() const override { return true; }
	DECLARE_C4CONTROL_VIRTUALS
};

// control used for initial player info, as well as for player info updates
class C4ControlPlayerInfo : public C4ControlPacket // not sync, lobby
{
public:
	C4ControlPlayerInfo() {}
	C4ControlPlayerInfo(const C4ClientPlayerInfos &PlrInfo)
		: PlrInfo(PlrInfo) {}

protected:
	C4ClientPlayerInfos PlrInfo;

public:
	const C4ClientPlayerInfos &GetInfo() const { return PlrInfo; }
	virtual bool Sync() const override { return false; }
	virtual bool Lobby() const override { return true; }
	DECLARE_C4CONTROL_VIRTUALS
};

struct C4ControlJoinPlayer : public C4ControlPacket // sync
{
public:
	C4ControlJoinPlayer() : iAtClient(-1), idInfo(-1) {}
	C4ControlJoinPlayer(const char *szFilename, int32_t iAtClient, int32_t iIDInfo, const C4Network2ResCore &ResCore);
	C4ControlJoinPlayer(const char *szFilename, int32_t iAtClient, int32_t iIDInfo);

protected:
	StdStrBuf Filename;
	int32_t iAtClient;
	int32_t idInfo;
	bool fByRes;
	StdBuf PlrData; // for fByRes == false
	C4Network2ResCore ResCore; // for fByRes == true

public:
	DECLARE_C4CONTROL_VIRTUALS
	virtual bool PreExecute(const std::shared_ptr<spdlog::logger> &logger) const override;
	virtual void PreRec(C4Record *pRecord) override;
	void Strip();
};

enum C4ControlEMObjectAction
{
	EMMO_Move,      // move objects by offset
	EMMO_Enter,     // enter objects into iTargetObj
	EMMO_Duplicate, // duplicate objects at same position; reset EditCursor
	EMMO_Script,    // execute Script
	EMMO_Remove,    // remove objects
	EMMO_Exit,      // exit objects
};

class C4ControlEMMoveObject : public C4ControlPacket // sync
{
public:
	C4ControlEMMoveObject() : pObjects(nullptr) {}
	C4ControlEMMoveObject(C4ControlEMObjectAction eAction, int32_t tx, int32_t ty, std::uint32_t sectionNumber, C4Object *pTargetObj,
		int32_t iObjectNum = 0, int32_t *pObjects = nullptr, const char *szScript = nullptr, C4AulScriptStrict strict = C4AulScriptStrict::MAXSTRICT);
	~C4ControlEMMoveObject();

protected:
	C4ControlEMObjectAction eAction; // action to be performed
	int32_t tx, ty; // target position
	std::uint32_t sectionNumber{}; // section index
	int32_t iTargetObj; // enumerated ptr to target object
	int32_t iObjectNum; // number of objects moved
	C4AulScriptStrict Strict; // strictness for the script to execute
	int32_t *pObjects; // pointer on array of objects moved
	StdStrBuf Script; // script to execute

public:
	DECLARE_C4CONTROL_VIRTUALS
};

enum C4ControlEMDrawAction
{
	EMDT_SetMode, // set new landscape mode
	EMDT_Brush,   // drawing tool
	EMDT_Fill,    // drawing tool
	EMDT_Line,    // drawing tool
	EMDT_Rect,    // drawing tool
};

class C4ControlEMDrawTool : public C4ControlPacket // sync
{
public:
	C4ControlEMDrawTool() {}
	C4ControlEMDrawTool(C4ControlEMDrawAction eAction, int32_t iMode,
		int32_t iX = -1, int32_t iY = -1, int32_t iX2 = -1, int32_t iY2 = -1, int32_t iGrade = -1,
		bool fIFT = true, std::uint32_t sectionNumber = 0, const char *szMaterial = nullptr, const char *szTexture = nullptr);

protected:
	C4ControlEMDrawAction eAction; // action to be performed
	int32_t iMode; // new mode, or mode action was performed in (action will fail if changed)
	int32_t iX, iY, iX2, iY2, iGrade; // drawing parameters
	bool fIFT; // sky/tunnel-background
	std::uint32_t sectionNumber; // section index
	StdStrBuf Material; // used material
	StdStrBuf Texture; // used texture

public:
	DECLARE_C4CONTROL_VIRTUALS
};

enum C4ControlMessageType
{
	C4CMT_Normal  = 0,
	C4CMT_Me      = 1,
	C4CMT_Say     = 2,
	C4CMT_Team    = 3,
	C4CMT_Private = 4,
	C4CMT_Sound   = 5, // "message" is played as a sound instead
	C4CMT_Alert   = 6, // no message. just flash taskbar for inactive clients.
	C4CMT_System  = 10,
};

class C4ControlMessage : public C4ControlPacket // not sync, lobby
{
public:
	C4ControlMessage()
		: eType(C4CMT_Normal), iPlayer(-1) {}
	C4ControlMessage(C4ControlMessageType eType, const char *szMessage, int32_t iPlayer = -1, int32_t iToPlayer = -1)
		: eType(eType), iPlayer(iPlayer), iToPlayer(iToPlayer), Message(szMessage, true) {}

protected:
	C4ControlMessageType eType;
	int32_t iPlayer, iToPlayer;
	StdStrBuf Message;

public:
	virtual bool Sync() const override { return false; }
	virtual bool Lobby() const override { return true; }
	DECLARE_C4CONTROL_VIRTUALS
};

class C4ControlRemovePlr : public C4ControlPacket // sync
{
public:
	C4ControlRemovePlr()
		: iPlr(-1) {}
	C4ControlRemovePlr(int32_t iPlr, bool fDisconnected)
		: iPlr(iPlr), fDisconnected(fDisconnected) {}

protected:
	int32_t iPlr;
	bool fDisconnected;

public:
	DECLARE_C4CONTROL_VIRTUALS
};

class C4ControlDebugRec : public C4ControlPacket // sync
{
public:
	C4ControlDebugRec() {}
	C4ControlDebugRec(StdBuf &Data)
		: Data(StdBuf::TakeOrRef(Data)) {}

protected:
	StdBuf Data;

public:
	DECLARE_C4CONTROL_VIRTUALS
};

enum C4ControlVoteType
{
	VT_None = -1,
	VT_Cancel,
	VT_Kick,
	VT_Pause
};

class C4ControlVote : public C4ControlPacket
{
public:
	C4ControlVote(C4ControlVoteType eType = VT_None, bool fApprove = true, int iData = 0)
		: eType(eType), fApprove(fApprove), iData(iData) {}

private:
	C4ControlVoteType eType;
	bool fApprove;
	int32_t iData;

public:
	C4ControlVoteType getType() const { return eType; }
	bool isApprove() const { return fApprove; }
	int32_t getData() const { return iData; }

	StdStrBuf getDesc() const;
	StdStrBuf getDescWarning() const;

	virtual bool Sync() const override { return false; }

	DECLARE_C4CONTROL_VIRTUALS
};

class C4ControlVoteEnd : public C4ControlVote
{
public:
	C4ControlVoteEnd(C4ControlVoteType eType = VT_None, bool fApprove = true, int iData = 0)
		: C4ControlVote(eType, fApprove, iData) {}

	virtual bool Sync() const override { return true; }

	DECLARE_C4CONTROL_VIRTUALS
};

class C4ControlInternalScriptBase : public C4ControlPacket
{
public:
	C4ControlInternalScriptBase() noexcept = default;
	C4ControlInternalScriptBase(const std::uint32_t sectionNumber) noexcept : sectionNumber(sectionNumber) {}
public:
	virtual int32_t Scope() const { return C4ControlScript::SCOPE_Global; }
	virtual bool Allowed() const { return true; }
	virtual std::string FormatScript() const = 0;

	DECLARE_C4CONTROL_VIRTUALS

private:
	std::uint32_t sectionNumber{};
};

class C4ControlEMDropDef : public C4ControlInternalScriptBase
{
	C4ID id = C4ID_None;
	int32_t x = 0;
	int32_t y = 0;

public:
	C4ControlEMDropDef() {}
	C4ControlEMDropDef(const std::uint32_t sectionNumber, C4ID id, int32_t x, int32_t y) : C4ControlInternalScriptBase{sectionNumber}, id(id), x(x), y(y) {}
	virtual void CompileFunc(StdCompiler *pComp) override;
	virtual bool Allowed() const override;
	virtual std::string FormatScript() const override;
};

class C4ControlInternalPlayerScriptBase : public C4ControlInternalScriptBase
{
protected:
	int32_t plr = NO_OWNER;

public:
	C4ControlInternalPlayerScriptBase() {}
	C4ControlInternalPlayerScriptBase(const std::uint32_t sectionNumber, int32_t plr) : C4ControlInternalScriptBase{sectionNumber}, plr(plr) {}
	virtual void CompileFunc(StdCompiler *pComp) override;
	virtual bool Allowed() const override;
};

class C4ControlMessageBoardAnswer : public C4ControlInternalPlayerScriptBase
{
	int32_t obj = 0;
	std::string answer;

public:
	C4ControlMessageBoardAnswer() {}
	C4ControlMessageBoardAnswer(const std::uint32_t sectionNumber, int32_t obj, int32_t plr, const std::string &answer) : C4ControlInternalPlayerScriptBase{sectionNumber, plr}, obj(obj), answer(answer) {}
	virtual void CompileFunc(StdCompiler *pComp) override;
	virtual std::string FormatScript() const override;
};

class C4ControlCustomCommand : public C4ControlInternalPlayerScriptBase
{
	std::string command;
	std::string argument;

public:
	C4ControlCustomCommand() {}
	C4ControlCustomCommand(const std::uint32_t sectionNumber, int32_t plr, const std::string &command, const std::string &argument) : C4ControlInternalPlayerScriptBase(sectionNumber, plr), command(command), argument(argument) {}
	virtual bool Allowed() const override;
	virtual void CompileFunc(StdCompiler *pComp) override;
	virtual std::string FormatScript() const override;
};

class C4ControlInitScenarioPlayer : public C4ControlInternalPlayerScriptBase
{
	int32_t team = 0;

public:
	C4ControlInitScenarioPlayer() {}
	C4ControlInitScenarioPlayer(const std::uint32_t sectionNumber, int32_t plr, int32_t team) : C4ControlInternalPlayerScriptBase(sectionNumber, plr), team(team) {}
	virtual void CompileFunc(StdCompiler *pComp) override;
	virtual std::string FormatScript() const override { return std::format("InitScenarioPlayer({},{})", plr, team); }
};

class C4ControlActivateGameGoalMenu : public C4ControlInternalPlayerScriptBase
{
public:
	C4ControlActivateGameGoalMenu() {}
	C4ControlActivateGameGoalMenu(const std::uint32_t sectionNumber, int32_t plr) : C4ControlInternalPlayerScriptBase(sectionNumber, plr) {}
	virtual std::string FormatScript() const override { return std::format("ActivateGameGoalMenu({})", plr); }
};

class C4ControlToggleHostility : public C4ControlInternalPlayerScriptBase
{
	int32_t opponent = 0;

public:
	C4ControlToggleHostility() {}
	C4ControlToggleHostility(const std::uint32_t sectionNumber, int32_t plr, int32_t opponent) : C4ControlInternalPlayerScriptBase(sectionNumber, plr), opponent(opponent) {}
	virtual void CompileFunc(StdCompiler *pComp) override;
	virtual std::string FormatScript() const override { return std::format("SetHostility({},{},!Hostile({},{},true))", plr, opponent, plr, opponent); }
};

class C4ControlSurrenderPlayer : public C4ControlInternalPlayerScriptBase
{
public:
	C4ControlSurrenderPlayer() {}
	C4ControlSurrenderPlayer(const std::uint32_t sectionNumber, int32_t plr) : C4ControlInternalPlayerScriptBase(sectionNumber, plr) {}
	virtual std::string FormatScript() const override { return std::format("SurrenderPlayer({})", plr); }
};

class C4ControlActivateGameGoalRule : public C4ControlInternalPlayerScriptBase
{
	int32_t obj = 0;

public:
	C4ControlActivateGameGoalRule() {}
	C4ControlActivateGameGoalRule(const std::uint32_t sectionNumber, int32_t plr, int32_t obj) : C4ControlInternalPlayerScriptBase(sectionNumber, plr), obj(obj)  {}
	virtual void CompileFunc(StdCompiler *pComp) override;
	virtual std::string FormatScript() const override { return std::format("Activate({})", plr); }
	virtual int32_t Scope() const override { return obj; }
};

class C4ControlSetPlayerTeam : public C4ControlInternalPlayerScriptBase
{
	int32_t team = 0;

public:
	C4ControlSetPlayerTeam() {}
	C4ControlSetPlayerTeam(const std::uint32_t sectionNumber, int32_t plr, int32_t team) : C4ControlInternalPlayerScriptBase(sectionNumber, plr), team(team)  {}
	virtual void CompileFunc(StdCompiler *pComp) override;
	virtual std::string FormatScript() const override { return std::format("SetPlayerTeam({},{})", plr, team); }
};

class C4ControlEliminatePlayer : public C4ControlInternalPlayerScriptBase
{
public:
	C4ControlEliminatePlayer() {}
	C4ControlEliminatePlayer(const std::uint32_t sectionNumber, int32_t plr) : C4ControlInternalPlayerScriptBase(sectionNumber, plr) {}
	virtual bool Allowed() const override { return HostControl(); }
	virtual std::string FormatScript() const override { return std::format("EliminatePlayer({})", plr); }
};

class C4ControlSectionLoaded : public C4ControlPacket
{
public:
	C4ControlSectionLoaded(const std::uint32_t sectionNumber, const bool success)
		: sectionNumber{sectionNumber}, success{success} {}

public:
	virtual bool Sync() const override { return true; }

	DECLARE_C4CONTROL_VIRTUALS

private:
	std::uint32_t sectionNumber;
	bool success;
};

class C4ControlSectionLoadFinished : public C4ControlPacket
{
public:
	C4ControlSectionLoadFinished(const std::uint32_t sectionNumber, const bool success)
		: sectionNumber{sectionNumber}, success{success} {}

public:
	virtual bool Sync() const override { return true; }

	DECLARE_C4CONTROL_VIRTUALS

private:
	std::uint32_t sectionNumber;
	bool success;
};
