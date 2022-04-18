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

/* Dynamic list to hold runtime player data */

#pragma once

#include "C4PlayerInfo.h"

class C4Player;
class C4PlayerInfoList;

class C4PlayerList
{
public:
	C4PlayerList();
	~C4PlayerList();

public:
	C4Player *First;

public:
	void Default();
	void Clear();
	void Execute();
	void DenumeratePointers();
	void EnumeratePointers();
	void ClearPointers(C4Object *pObj);
	int GetCount() const;
	int GetCount(C4PlayerType eType) const;
	int GetCountNotEliminated() const;
	int AverageValueGain() const;
	C4Player *Get(int iPlayer) const;
	C4Player *GetByIndex(int iIndex) const;
	C4Player *GetByIndex(int iIndex, C4PlayerType eType) const;
	C4Player *GetByName(const char *szName, int iExcluding = NO_OWNER) const;
	C4Player *GetLocalByKbdSet(int iKbdSet) const;
	C4Player *GetLocalByIndex(int iIndex) const;
	C4Player *GetAtClient(int iClient, int iIndex = 0) const;
	C4Player *GetAtRemoteClient(int iIndex = 0) const;
	C4Player *GetByInfoID(int iInfoID) const;
	C4Player *Join(const char *szFilename, bool fScenarioInit, int iAtClient, const char *szAtClientName, class C4PlayerInfo *pInfo);
	bool CtrlJoinLocalNoNetwork(const char *szFilename, int iAtClient, const char *szAtClientName);
	bool FileInUse(const char *szFilename) const;
	bool Retire(C4Player *pPlr);
	bool Evaluate();
	bool Save(bool fSaveLocalOnly);
	bool Save(C4Group &hGroup, bool fStoreTiny, const C4PlayerInfoList &rStoreList); // save all players present in the restore list
	bool Remove(int iPlayer, bool fDisonnected, bool fNoCalls);
	bool RemoveUnjoined(int32_t iPlayer); // remove player objects only
	bool Remove(C4Player *pPlr, bool fDisonnected, bool fNoCalls);
	bool RemoveAtRemoteClient(bool fDisonnected, bool fNoCalls);
	bool RemoveLocal(bool fDisonnected, bool fNoCalls);
	bool MouseControlTaken() const;
	bool RemoveAtClient(int iClient, bool fDisconnect);
	bool CtrlRemove(int iPlayer, bool fDisonnected);
	bool Valid(int iPlayer) const;
	bool Hostile(int iPlayer1, int iPlayer2) const;
	bool HostilityDeclared(int iPlayer1, int iPlayer2) const; // check whether iPlayer1 treats iPlayer2 as hostile, but not vice versa!
	bool PositionTaken(int iPosition) const;
	bool ColorTaken(int iColor) const;
	bool ControlTaken(int iControl) const;
	bool SynchronizeLocalFiles(); // syncrhonize all local player files; resetting InGame times
	void ClearLocalPlayerPressedComs();

protected:
	int GetFreeNumber() const;
	void RecheckPlayerSort(C4Player *pForPlayer);

	friend class C4Player;
};
