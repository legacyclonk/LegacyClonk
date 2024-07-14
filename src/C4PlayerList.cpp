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

#include <C4Include.h>
#include <C4PlayerList.h>

#include <C4Components.h>
#include <C4FullScreen.h>
#include <C4Console.h>
#include <C4Log.h>
#include <C4Player.h>
#include <C4Object.h>

C4PlayerList::C4PlayerList()
{
	Default();
}

C4PlayerList::~C4PlayerList()
{
	Clear();
}

void C4PlayerList::Default()
{
	First = nullptr;
}

void C4PlayerList::Clear()
{
	C4Player *pPlr;
	while (pPlr = First)
	{
		First = pPlr->Next; delete pPlr;
	}
	First = nullptr;
}

void C4PlayerList::Execute()
{
	C4Player *pPlr;
	// Execute
	for (pPlr = First; pPlr; pPlr = pPlr->Next)
		pPlr->Execute();
	// Check retirement
	for (pPlr = First; pPlr; pPlr = pPlr->Next)
		if (pPlr->Eliminated && !pPlr->RetireDelay)
		{
			Retire(pPlr); break;
		}
}

void C4PlayerList::ClearPointers(C4Object *pObj)
{
	for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
		pPlr->ClearPointers(pObj, false);
}

bool C4PlayerList::Valid(int iPlayer) const
{
	for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
		if (pPlr->Number == iPlayer)
			return true;
	return false;
}

bool C4PlayerList::Hostile(int iPlayer1, int iPlayer2) const
{
	C4Player *pPlr1 = Get(iPlayer1);
	C4Player *pPlr2 = Get(iPlayer2);
	if (!pPlr1 || !pPlr2) return false;
	if (pPlr1->Number == pPlr2->Number) return false;
	if (pPlr1->Hostility.GetIDCount(pPlr2->Number + 1)
		|| pPlr2->Hostility.GetIDCount(pPlr1->Number + 1))
		return true;
	return false;
}

bool C4PlayerList::HostilityDeclared(int iPlayer1, int iPlayer2) const
{
	// check one-way-hostility
	C4Player *pPlr1 = Get(iPlayer1);
	C4Player *pPlr2 = Get(iPlayer2);
	if (!pPlr1 || !pPlr2) return false;
	if (pPlr1->Number == pPlr2->Number) return false;
	if (pPlr1->Hostility.GetIDCount(pPlr2->Number + 1))
		return true;
	return false;
}

bool C4PlayerList::PositionTaken(int iPosition) const
{
	for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
		if (pPlr->Position == iPosition)
			return true;
	return false;
}

bool C4PlayerList::ColorTaken(int iColor) const
{
	for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
		if (pPlr->Color == iColor)
			return true;
	return false;
}

bool C4PlayerList::ControlTaken(int iControl) const
{
	for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
		if (pPlr->Control == iControl)
			if (pPlr->LocalControl)
				return true;
	return false;
}

C4Player *C4PlayerList::Get(int iNumber) const
{
	for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
		if (pPlr->Number == iNumber)
			return pPlr;
	return nullptr;
}

C4Player *C4PlayerList::GetByIndex(int iIndex) const
{
	for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
		if (!iIndex--)
			return pPlr;
	return nullptr;
}

C4Player *C4PlayerList::GetByIndex(int iIndex, C4PlayerType eType) const
{
	for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
		if (pPlr->GetType() == eType)
			if (!iIndex--)
				return pPlr;
	return nullptr;
}

C4Player *C4PlayerList::GetLocalByKbdSet(int iKbdSet) const
{
	for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
		if (pPlr->LocalControl)
			if (pPlr->Control == iKbdSet)
				return pPlr;
	return nullptr;
}

C4Player *C4PlayerList::GetByInfoID(int iInfoID) const
{
	for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
		if (pPlr->ID == iInfoID) return pPlr;
	return nullptr;
}

int C4PlayerList::GetCount() const
{
	int iCount = 0;
	for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
		iCount++;
	return iCount;
}

int C4PlayerList::GetCount(C4PlayerType eType) const
{
	int iCount = 0;
	for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
		if (pPlr->GetType() == eType)
			iCount++;
	return iCount;
}

int C4PlayerList::GetFreeNumber() const
{
	int iNumber = -1;
	bool fFree;
	do
	{
		iNumber++; fFree = true;
		for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
			if (pPlr->Number == iNumber)
				fFree = false;
	} while (!fFree);
	return iNumber;
}

bool C4PlayerList::Remove(int iPlayer, bool fDisconnect, bool fNoCalls)
{
	return Remove(Get(iPlayer), fDisconnect, fNoCalls);
}

bool C4PlayerList::RemoveUnjoined(int32_t iPlayer)
{
	// Savegame resume missing player: Remove player objects only
	for (C4Object *const obj : Game.GetAllObjectsWithStatus())
	{
		if (obj->IsPlayerObject(iPlayer))
			obj->AssignRemoval(true);
	}
	return true;
}

bool C4PlayerList::Remove(C4Player *pPlr, bool fDisconnect, bool fNoCalls)
{
	if (!pPlr) return false;

	// inform script
	if (!fNoCalls)
		Game.Script.GRBroadcast(PSF_RemovePlayer, {C4VInt(pPlr->Number), C4VInt(pPlr->Team)});

	// Transfer ownership of other objects to team members
	if (!fNoCalls) pPlr->NotifyOwnedObjects();

	// NET2: update player info list
	if (pPlr->ID)
	{
		C4PlayerInfo *pInfo = Game.PlayerInfos.GetPlayerInfoByID(pPlr->ID);
		if (pInfo)
		{
			pInfo->SetRemoved();
			if (fDisconnect)
				pInfo->SetDisconnected();
		}
		// if player wasn't evaluated, store round results anyway
		if (!pPlr->Evaluated) Game.RoundResults.EvaluatePlayer(pPlr);
	}

	C4Player *pPrev = First;
	while (pPrev && pPrev->Next != pPlr) pPrev = pPrev->Next;
	if (pPrev) pPrev->Next = pPlr->Next;
	else First = pPlr->Next;

	// Remove eliminated crew
	if (!fNoCalls) pPlr->RemoveCrewObjects();

	// Clear object info pointers
	pPlr->CrewInfoList.DetachFromObjects();

	// Clear viewports
	Game.GraphicsSystem.CloseViewport(pPlr->Number, fNoCalls);
	// Check fullscreen viewports
	FullScreen.ViewportCheck();

	// Remove player
	delete pPlr;

	// Validate object owners
	Game.ValidateOwners();

	// Update console
	Console.UpdateMenus();
	return true;
}

C4Player *C4PlayerList::Join(const char *szFilename, bool fScenarioInit, int iAtClient, const char *szAtClientName, C4PlayerInfo *pInfo, C4Section &initialSection)
{
	assert(pInfo);

	// safeties
	if (szFilename && !*szFilename) szFilename = nullptr;

	// Log
	if (fScenarioInit)
	{
		Log(C4ResStrTableKey::IDS_PRC_JOINPLR, pInfo->GetName());
	}
	else
	{
		Log(C4ResStrTableKey::IDS_PRC_RECREATE, pInfo->GetName());
	}

	// Too many players
	if (1) // replay needs to check, too!
		if (GetCount() + 1 > Game.Parameters.MaxPlayers)
		{
			Log(C4ResStrTableKey::IDS_PRC_TOOMANYPLRS, Game.Parameters.MaxPlayers);
			return nullptr;
		}

	// Check duplicate file usage
	if (szFilename) if (FileInUse(szFilename))
	{
		Log(C4ResStrTableKey::IDS_PRC_PLRFILEINUSE); return nullptr;
	}

	// Create
	C4Player *pPlr = new C4Player;

	// Append to player list
	C4Player *pLast = First;
	for (; pLast && pLast->Next; pLast = pLast->Next);
	if (pLast) pLast->Next = pPlr; else First = pPlr;

	// Init
	if (!pPlr->Init(GetFreeNumber(), iAtClient, szAtClientName, szFilename, fScenarioInit, pInfo, initialSection))
	{
		Remove(pPlr, false, false); Log(C4ResStrTableKey::IDS_PRC_JOINFAIL); return nullptr;
	}

	// Done
	return pPlr;
}

bool C4PlayerList::CtrlJoinLocalNoNetwork(const char *szFilename, int iAtClient, const char *szAtClientName)
{
	assert(!Game.Network.isEnabled());
	// security
	if (!ItemExists(szFilename)) return false;
	// join via player info
	bool fSuccess = Game.PlayerInfos.DoLocalNonNetworkPlayerJoin(szFilename);

	// Done
	return fSuccess;
}

void SetClientPrefix(char *szFilename, const char *szClient)
{
	char szTemp[1024 + 1];
	// Compose prefix
	char szPrefix[1024 + 1];
	SCopy(szClient, szPrefix); SAppendChar('-', szPrefix);
	// Prefix already set?
	SCopy(GetFilename(szFilename), szTemp, SLen(szPrefix));
	if (SEqualNoCase(szTemp, szPrefix)) return;
	// Insert prefix
	SCopy(GetFilename(szFilename), szTemp);
	SCopy(szPrefix, GetFilename(szFilename));
	SAppend(szTemp, szFilename);
}

bool C4PlayerList::Save(C4Group &hGroup, bool fStoreTiny, const C4PlayerInfoList &rStoreList)
{
	StdStrBuf sTempFilename;
	bool fSuccess = true;
	// Save to external player files and add to group
	for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
	{
		// save only those in the list, and only those with a filename
		C4PlayerInfo *pNfo = rStoreList.GetPlayerInfoByID(pPlr->ID);
		if (!pNfo) continue;
		if (!pNfo->GetFilename() || !*pNfo->GetFilename()) continue;
		// save over original file?
		bool fStoreOnOriginal = (!fStoreTiny && pNfo->GetType() == C4PT_User);
		// Create temporary file
		sTempFilename.Copy(Config.AtTempPath(pNfo->GetFilename()));
		if (fStoreOnOriginal)
			if (!C4Group_CopyItem(pPlr->Filename, sTempFilename.getData()))
				return false;
		// Open group
		C4Group PlrGroup;
		if (!PlrGroup.Open(sTempFilename.getData(), !fStoreOnOriginal))
			return false;
		// Save player
		if (!pPlr->Save(PlrGroup, true, fStoreOnOriginal)) return false;
		PlrGroup.Close();
		// Add temp file to group
		if (!hGroup.Move(sTempFilename.getData(), pNfo->GetFilename())) return false;
	}
	return fSuccess;
}

bool C4PlayerList::Save(bool fSaveLocalOnly)
{
	// do not save in replays
	if (Game.MainSection.C4S.Head.Replay) return true;
	// Save to external player files
	for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
		if (pPlr->GetType() != C4PT_Script)
			if (!fSaveLocalOnly || pPlr->LocalControl)
				if (!pPlr->Save())
					return false;
	return true;
}

bool C4PlayerList::Evaluate()
{
	for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
		pPlr->Evaluate();
	return true;
}

bool C4PlayerList::Retire(C4Player *pPlr)
{
	if (!pPlr) return false;

	if (!pPlr->Evaluated)
	{
		pPlr->Evaluate();
		if (!Game.Control.isReplay() && pPlr->GetType() != C4PT_Script) pPlr->Save();
	}
	Remove(pPlr, false, false);

	return true;
}

int C4PlayerList::AverageValueGain() const
{
	int iResult = 0;
	if (First)
	{
		for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
			iResult += std::max<int32_t>(pPlr->ValueGain, 0);
		iResult /= GetCount();
	}
	return iResult;
}

C4Player *C4PlayerList::GetByName(const char *szName, int iExcluding) const
{
	for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
		if (SEqual(pPlr->GetName(), szName))
			if (pPlr->Number != iExcluding)
				return pPlr;
	return nullptr;
}

bool C4PlayerList::FileInUse(const char *szFilename) const
{
	// Check original player files
	C4Player *cPlr = First;
	for (; cPlr; cPlr = cPlr->Next)
		if (ItemIdentical(cPlr->Filename, szFilename))
			return true;
	// Compare to any network path player files with prefix (hack)
	if (Game.Network.isEnabled())
	{
		char szWithPrefix[_MAX_PATH + 1];
		SCopy(GetFilename(szFilename), szWithPrefix);
		SetClientPrefix(szWithPrefix, Game.Clients.getLocalName());
		for (cPlr = First; cPlr; cPlr = cPlr->Next)
			if (SEqualNoCase(GetFilename(cPlr->Filename), szWithPrefix))
				return true;
	}
	// Not in use
	return false;
}

C4Player *C4PlayerList::GetLocalByIndex(int iIndex) const
{
	int cindex = 0;
	for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
		if (pPlr->LocalControl)
		{
			if (cindex == iIndex) return pPlr;
			cindex++;
		}
	return nullptr;
}

bool C4PlayerList::RemoveAtClient(int iClient, bool fDisconnect)
{
	C4Player *pPlr;
	// Get players
	while (pPlr = GetAtClient(iClient))
	{
		// Log
		Log(C4ResStrTableKey::IDS_PRC_REMOVEPLR, pPlr->GetName());
		// Remove
		Remove(pPlr, fDisconnect, false);
	}
	return true;
}

bool C4PlayerList::CtrlRemove(int iPlayer, bool fDisconnect)
{
	// Add packet to input
	Game.Input.Add(CID_RemovePlr, new C4ControlRemovePlr(iPlayer, fDisconnect));
	return true;
}

C4Player *C4PlayerList::GetAtClient(int iClient, int iIndex) const
{
	int cindex = 0;
	for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
		if (pPlr->AtClient == iClient)
		{
			if (cindex == iIndex) return pPlr;
			cindex++;
		}
	return nullptr;
}

bool C4PlayerList::RemoveAtRemoteClient(bool fDisconnect, bool fNoCalls)
{
	C4Player *pPlr;
	// Get players
	while (pPlr = GetAtRemoteClient())
	{
		// Log
		Log(C4ResStrTableKey::IDS_PRC_REMOVEPLR, pPlr->GetName());
		// Remove
		Remove(pPlr, fDisconnect, fNoCalls);
	}
	return true;
}

C4Player *C4PlayerList::GetAtRemoteClient(int iIndex) const
{
	int cindex = 0;
	for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
		if (pPlr->AtClient != Game.Control.ClientID())
		{
			if (cindex == iIndex) return pPlr;
			cindex++;
		}
	return nullptr;
}

bool C4PlayerList::RemoveLocal(bool fDisconnect, bool fNoCalls)
{
	// (used by league system the set local fate)
	C4Player *pPlr;
	do
		for (pPlr = First; pPlr; pPlr = pPlr->Next)
			if (pPlr->LocalControl)
			{
				// Log
				Log(C4ResStrTableKey::IDS_PRC_REMOVEPLR, pPlr->GetName());
				// Remove
				Remove(pPlr, fDisconnect, fNoCalls);
				break;
			}
	while (pPlr);

	return true;
}

void C4PlayerList::EnumeratePointers()
{
	for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
		pPlr->EnumeratePointers();
}

void C4PlayerList::DenumeratePointers()
{
	for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
		pPlr->DenumeratePointers();
}

bool C4PlayerList::MouseControlTaken() const
{
	for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
		if (pPlr->MouseControl)
			if (pPlr->LocalControl)
				return true;
	return false;
}

int C4PlayerList::GetCountNotEliminated() const
{
	int iCount = 0;
	for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
		if (!pPlr->Eliminated)
			iCount++;
	return iCount;
}

bool C4PlayerList::SynchronizeLocalFiles()
{
	// message
	Log(C4ResStrTableKey::IDS_PRC_SYNCPLRS);
	bool fSuccess = true;
	// check all players
	for (C4Player *pPlr = First; pPlr; pPlr = pPlr->Next)
		// eliminated players will be saved soon, anyway
		if (!pPlr->Eliminated)
			if (!pPlr->LocalSync()) fSuccess = false;
	// done
	return fSuccess;
}

void C4PlayerList::ClearLocalPlayerPressedComs()
{
	C4Player *plr;
	for (int cnt = 0; plr = Game.Players.GetLocalByIndex(cnt); cnt++)
	{
		plr->ClearPressedComsSynced();
	}
}

void C4PlayerList::RecheckPlayerSort(C4Player *pForPlayer)
{
	if (!pForPlayer || !First) return;
	int iNumber = pForPlayer->Number;
	// get entry that should be the previous
	// (use '<=' to run past pForPlayer itself)
	C4Player *pPrev = First;
	while (pPrev->Next && pPrev->Next->Number <= iNumber)
		pPrev = pPrev->Next;
	// if it's correctly sorted, pPrev should point to pForPlayer itself
	if (pPrev == pForPlayer) return;
	// otherwise, pPrev points to the entry that should be the new previous
	// or to First if pForPlayer should be the head entry
	// re-link it there
	// first remove from old link pos
	C4Player **pOldLink = &First;
	while (*pOldLink && *pOldLink != pForPlayer) pOldLink = &((*pOldLink)->Next);
	if (*pOldLink) *pOldLink = pForPlayer->Next;
	// then link into new
	if (pPrev == First && pPrev->Number > iNumber)
	{
		// at head
		pForPlayer->Next = pPrev;
		First = pForPlayer;
	}
	else
	{
		// after prev
		pForPlayer->Next = pPrev->Next;
		pPrev->Next = pForPlayer;
	}
}
