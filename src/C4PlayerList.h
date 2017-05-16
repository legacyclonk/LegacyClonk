/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
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

/* Dynamic list to hold runtime player data */

#ifndef INC_C4PlayerList
#define INC_C4PlayerList

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
		int GetIndex(C4Player *pPlr) const;
		int GetCountNotEliminated() const;
		int ControlTakenBy(int iControl) const;
		int AverageValueGain() const;
		C4Player *Get(int iPlayer) const;
		C4Player *GetByIndex(int iIndex) const;
		C4Player *GetByIndex(int iIndex, C4PlayerType eType) const;
		C4Player *GetByName(const char *szName, int iExcluding=NO_OWNER) const;
		C4Player *GetLocalByKbdSet(int iKbdSet) const;
		C4Player *GetLocalByIndex(int iIndex) const;
		C4Player *GetAtClient(int iClient, int iIndex=0) const;
		C4Player *GetAtClient(const char *szName, int iIndex=0) const;
		C4Player *GetAtRemoteClient(int iIndex=0) const;
		C4Player *GetByInfoID(int iInfoID) const;
		C4Player *Join(const char *szFilename, BOOL fScenarioInit, int iAtClient, const char *szAtClientName, class C4PlayerInfo *pInfo);
		BOOL CtrlJoinLocalNoNetwork(const char *szFilename, int iAtClient, const char *szAtClientName);
		BOOL CtrlJoin(const class C4Network2ResCore &ResCore, int iClientID, int idPlayerInfo);
		BOOL FileInUse(const char *szFilename) const;
		BOOL Retire(C4Player *pPlr);
		BOOL Evaluate();
		BOOL Save(bool fSaveLocalOnly);
		BOOL Save(C4Group &hGroup, bool fStoreTiny, const C4PlayerInfoList &rStoreList); // save all players present in the restore list
	  BOOL Remove(int iPlayer, bool fDisonnected, bool fNoCalls);
		bool RemoveUnjoined(int32_t iPlayer); // remove player objects only
	  BOOL Remove(C4Player *pPlr, bool fDisonnected, bool fNoCalls);
		BOOL RemoveAtRemoteClient(bool fDisonnected, bool fNoCalls);
		BOOL RemoveLocal(bool fDisonnected, bool fNoCalls);
		BOOL MouseControlTaken() const;
		BOOL RemoveAtClient(int iClient, bool fDisconnect);
		BOOL RemoveAtClient(const char *szName, bool fDisconnect);
		BOOL CtrlRemove(int iPlayer, bool fDisonnected);
		BOOL CtrlRemoveAtClient(int iClient, bool fDisonnected);
		BOOL CtrlRemoveAtClient(const char *szName, bool fDisonnected);
		BOOL Valid(int iPlayer) const;
		BOOL Hostile(int iPlayer1, int iPlayer2) const;
		bool HostilityDeclared(int iPlayer1, int iPlayer2) const; // check whether iPlayer1 treats iPlayer2 as hostile, but not vice versa!
		BOOL PositionTaken(int iPosition) const;
		BOOL ColorTaken(int iColor) const;
		int CheckColorDw(DWORD dwColor, C4Player *pExclude); // return minimum difference to the other player's colors
		BOOL ControlTaken(int iControl) const;
		BOOL SynchronizeLocalFiles(); // syncrhonize all local player files; resetting InGame times
		protected:
			int GetFreeNumber() const;
			void RecheckPlayerSort(C4Player *pForPlayer);

			friend class C4Player;
	};

#endif
