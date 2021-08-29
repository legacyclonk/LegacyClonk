/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
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

#include <C4Include.h>
#include "C4Update.h"
#include "C4Version.h"
#include "C4Config.h"
#include "C4Components.h"
#include "C4Group.h"
#include "C4Log.h"

C4Config *GetCfg();

#ifdef _WIN32
#include <direct.h>
#endif

// helper
bool C4Group_CopyEntry(C4Group *pFrom, C4Group *pTo, const char *strItemName)
{
	// read entry
	char *pData; size_t iSize;
	if (!pFrom->LoadEntry(strItemName, &pData, &iSize))
		return false;
	// write entry (keep time)
	int iEntryTime = pFrom->EntryTime(strItemName);
	if (!pTo->Add(strItemName, pData, iSize, false, true, iEntryTime))
		return false;
	return true;
}

bool C4Group_ApplyUpdate(C4Group &hGroup)
{
	// Process object update group (GRPUP_Entries.txt found)
	C4UpdatePackage Upd;
	if (hGroup.FindEntry(C4CFN_UpdateEntries))
		if (Upd.Load(&hGroup))
		{
			// Do update check first (ensure packet has everything it needs in order to perfom the update)
			const auto result = Upd.Check(&hGroup);
			switch (result)
			{
			// Bad version - checks against version of the applying executable (major version must match, minor version must be equal or higher)
			case C4UpdatePackage::CheckResult::BadVersion:
				fprintf(stderr, "This update %s can only be applied using version %d.%d.%d.%d or higher.\n", Upd.Name, Upd.RequireVersion[0], Upd.RequireVersion[1], Upd.RequireVersion[2], Upd.RequireVersion[3]);
				return false;
			// Target not found: keep going
			case C4UpdatePackage::CheckResult::NoSource:
				fprintf(stderr, "Target %s for update %s not found. Ignoring.\n", Upd.DestPath, Upd.Name);
				return true;
			// Target mismatch: abort updating
			case C4UpdatePackage::CheckResult::BadSource:
				fprintf(stderr, "Target %s incorrect version for update %s. Ignoring.\n", Upd.DestPath, Upd.Name);
				return true;
			// Target already updated: keep going
			case C4UpdatePackage::CheckResult::AlreadyUpdated:
				fprintf(stderr, "Target %s already up-to-date at %s.\n", Upd.DestPath, Upd.Name);
				return true;
			// Ok to perform update
			case C4UpdatePackage::CheckResult::Ok:
				printf("Updating %s to %s... ", Upd.DestPath, Upd.Name);
				// Make sure the user sees the message while the work is in progress
				fflush(stdout);
				// Execute update
				if (Upd.Execute(&hGroup))
				{
					printf("Ok\n");
					return true;
				}
				else
				{
					printf("Failed\n");
					return false;
				}
			}
			fprintf(stderr, "Unknown error while updating.\n");
			return false;
		}

	// Process binary update group (AutoUpdate.txt found, additional binary files found)
	if (hGroup.EntryCount(C4CFN_UpdateCore))
		if (hGroup.EntryCount() - hGroup.EntryCount(C4CFN_UpdateCore) - hGroup.EntryCount("*.c4u") > 0)
		{
			// Notice: AutoUpdate.txt is currently not processed...
			char strEntry[_MAX_FNAME + 1] = "";
			StdStrBuf strList;
			printf("Updating binaries...\n");
			hGroup.ResetSearch();
			// Look for binaries
			while (hGroup.FindNextEntry("*", strEntry))
				// Accept everything except *.c4u, AutoUpdate.txt, and c4group.exe (which is assumed not to work under Windows)
				if (!WildcardMatch("*.c4u", strEntry) && !WildcardMatch(C4CFN_UpdateCore, strEntry) && !WildcardMatch("c4group.exe", strEntry))
				{
					strList += strEntry; strList += ";";
				}
			// Extract binaries to current working directory
			if (!hGroup.Extract(strList.getData()))
				return false;
			// If extracted file is a group, explode it (this is meant for Clonk.app on Mac)
			for (int i = 0; SGetModule(strList.getData(), i, strEntry); i++)
				if (C4Group_IsGroup(strEntry))
				{
					printf("Exploding: %s\n", strEntry);
					if (!C4Group_ExplodeDirectory(strEntry))
						return false;
				}
		}

	// Process any child updates (*.c4u)
	if (hGroup.FindEntry("*.c4u"))
	{
		// Process all children
		char strEntry[_MAX_FNAME + 1] = "";
		C4Group hChild;
		hGroup.ResetSearch();
		while (hGroup.FindNextEntry("*.c4u", strEntry))
			if (hChild.OpenAsChild(&hGroup, strEntry))
			{
				bool ok = C4Group_ApplyUpdate(hChild);
				hChild.Close();
				// Failure on child update
				if (!ok) return false;
			}
	}

	// Success
	return true;
}

// *** C4GroupEx
class C4GroupEx : public C4Group
{
public:
	// some funcs to alter internal values of groups.
	// Needed to create byte-correct updated files

	void SetHead(C4Group &rByGrp)
	{
		// Cheat away the protection
		C4GroupHeader *pHdr = &static_cast<C4GroupEx &>(rByGrp).Head;
		// save Entries
		int Entries = Head.Entries;
		// copy
		memcpy(&Head, pHdr, sizeof(Head));
		// restore
		Head.Entries = Entries;
	}

	bool HeadIdentical(C4Group &rByGrp, bool fLax)
	{
		// Cheat away the protection
		C4GroupHeader *pHdr = &static_cast<C4GroupEx &>(rByGrp).Head;
		// overwrite entries field
		int Entries = Head.Entries;
		Head.Entries = pHdr->Entries;
		// overwrite creation field
		int Creation = Head.Creation;
		if (fLax) Head.Creation = pHdr->Creation;
		// compare
		bool fIdentical = !memcmp(&Head, pHdr, sizeof(C4GroupHeader));
		// restore field values
		Head.Entries = Entries;
		Head.Creation = Creation;
		// okay
		return fIdentical;
	}

	C4GroupEntryCore SavedCore;
	void SaveEntryCore(C4Group &rByGrp, const char *szEntry)
	{
		C4GroupEntryCore *pCore = (static_cast<C4GroupEx &>(rByGrp)).GetEntry(szEntry);
		// copy core
		memcpy(&SavedCore.Time, &pCore->Time, reinterpret_cast<char *>(&SavedCore) + sizeof(SavedCore) - reinterpret_cast<char *>(&SavedCore.Time));
	}
	void SetSavedEntryCore(const char *szEntry)
	{
		C4GroupEntryCore *pCore = GetEntry(szEntry);
		// copy core
		memcpy(&pCore->Time, &SavedCore.Time, reinterpret_cast<char *>(&SavedCore) + sizeof(SavedCore) - reinterpret_cast<char *>(&SavedCore.Time));
	}

	void SetEntryTime(const char *szEntry, int iEntryTime)
	{
		C4GroupEntryCore *pCore = GetEntry(szEntry);
		if (pCore) pCore->Time = iEntryTime;
	}

	void SetNoSort(const char *szEntry)
	{
		C4GroupEntry *pEntry = GetEntry(szEntry);
		if (pEntry) pEntry->NoSort = true;
	}

	// close without header update
	bool Close(bool fHeaderUpdate)
	{
		if (fHeaderUpdate) return C4Group::Close(); else { bool fSuccess = Save(false); Clear(); return fSuccess; }
	}
};

// *** C4UpdatePackageCore

C4UpdatePackageCore::C4UpdatePackageCore() :
	RequireVersion{}, Name{}, DestPath{}, GrpUpdate{}, UpGrpCnt{},
	GrpChks1{}, GrpChks2{} {}

void C4UpdatePackageCore::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(toC4CArr(RequireVersion), "RequireVersion"));
	pComp->Value(mkNamingAdapt(toC4CStr(Name),           "Name",        ""));
	pComp->Value(mkNamingAdapt(toC4CStr(DestPath),       "DestPath",    ""));
	pComp->Value(mkNamingAdapt(GrpUpdate,                "GrpUpdate",   0));
	pComp->Value(mkNamingAdapt(UpGrpCnt,                 "TargetCount", 0));
	pComp->Value(mkNamingAdapt(toC4CArrU(GrpChks1),      "GrpChks1"));
	pComp->Value(mkNamingAdapt(GrpChks2,                 "GrpChks2",    0u));
	pComp->Value(mkNamingAdapt(toC4CArrU(GrpContentsCRC1),      "GrpContentsCRC1"));
	pComp->Value(mkNamingAdapt(GrpContentsCRC2,                 "GrpContentsCRC2",    0u));
}

bool C4UpdatePackageCore::Load(C4Group &hGroup)
{
	// Load from group
	StdStrBuf Source;
	if (!hGroup.LoadEntryString(C4CFN_UpdateCore, Source))
		return false;
	try
	{
		// Compile data
		CompileFromBuf<StdCompilerINIRead>(mkNamingAdapt(*this, "Update"), Source);
	}
	catch (const StdCompiler::Exception &)
	{
		return false;
	}
	return true;
}

bool C4UpdatePackageCore::Save(C4Group &hGroup)
{
	try
	{
		// decompile data
		StdStrBuf Core = DecompileToBuf<StdCompilerINIWrite>(mkNamingAdapt(*this, "Update"));
		char *stupid_buffer = new char[Core.getLength() + 1];
		memcpy(stupid_buffer, Core.getMData(), Core.getLength() + 1);
		// add to group
		return hGroup.Add(C4CFN_UpdateCore, stupid_buffer, Core.getLength(), false, true);
	}
	catch (const StdCompiler::Exception &)
	{
		return false;
	}
}

// *** C4UpdatePackage

bool C4UpdatePackage::Load(C4Group *pGroup)
{
	// read update core
	StdStrBuf Source;
	if (!pGroup->LoadEntryString(C4CFN_UpdateCore, Source))
		return false;
	try
	{
		// Compile data
		CompileFromBuf<StdCompilerINIRead>(mkNamingAdapt(*this, "Update"), Source);
	}
	catch (const StdCompiler::Exception &e)
	{
		StdStrBuf Name = pGroup->GetFullName() + DirSep + C4CFN_UpdateCore;
		WriteLog("ERROR: %s (in %s)", e.Msg.getData(), Name.getData());
		return false;
	}
	return true;
}

bool C4UpdatePackage::Execute(C4Group *pGroup)
{
	// search target
	C4GroupEx TargetGrp;
	char strTarget[_MAX_PATH]; SCopy(DestPath, strTarget, _MAX_PATH);
	char *p = strTarget, *lp = strTarget;
	while (p = strchr(p + 1, '\\'))
	{
		*p = 0;
		if (!*(p + 1)) break;
		if (!SEqual(lp, ".."))
			if (TargetGrp.Open(strTarget))
			{
				// packed?
				bool fPacked = TargetGrp.IsPacked();
				// maker check (someone might try to unpack directories w/o asking user)
				if (fPacked)
					if (!SEqual(TargetGrp.GetMaker(), pGroup->GetMaker()))
						return false;
				// Close Group
				TargetGrp.Close(true);
				if (fPacked)
					// Unpack
					C4Group_UnpackDirectory(strTarget);
			}
			else
			{
				// GrpUpdate -> file must exist
				if (GrpUpdate) return false;
				// create dir
				CreateDirectory(strTarget, nullptr);
			}
		*p = '\\'; lp = p + 1;
	}

	// try to open it
	if (!TargetGrp.Open(strTarget, !GrpUpdate))
		return false;

	// check if the update is allowed
	if (GrpUpdate)
	{
		// check checksum
		uint32_t iContentsCRC32;
		if (!C4Group_GetFileContentsCRC(TargetGrp.GetFullName().getData(), &iContentsCRC32))
			return false;
		int i = 0;
		for (; i < UpGrpCnt; i++)
			if (GrpContentsCRC1[i] && iContentsCRC32 == GrpContentsCRC1[i])
				break;
		if (i >= UpGrpCnt)
		{
			uint32_t iCRC32;
			if (!C4Group_GetFileCRC(TargetGrp.GetFullName().getData(), &iCRC32))
				return false;
			int i = 0;
			for (; i < UpGrpCnt; i++)
				if (iCRC32 == GrpChks1[i])
					break;
			if (i >= UpGrpCnt)
				return false;
		}
	}
	else
	{
		// only allow Extra.c4g-Updates
		if (!SEqual2(DestPath, "Extra.c4g"))
			return false;
	}

	// update children
	char ItemFileName[_MAX_PATH];
	pGroup->ResetSearch();
	while (pGroup->FindNextEntry("*", ItemFileName))
		if (!SEqual(ItemFileName, C4CFN_UpdateCore) && !SEqual(ItemFileName, C4CFN_UpdateEntries))
			DoUpdate(pGroup, &TargetGrp, ItemFileName);

	// do GrpUpdate
	if (GrpUpdate)
		DoGrpUpdate(pGroup, &TargetGrp);

	// close the group
	TargetGrp.Close(false);

	if (GrpUpdate)
	{
		// check the result
		uint32_t iResChks, iResContentsChks;
		if (!C4Group_GetFileCRC(strTarget, &iResChks))
			return false;
		if (!C4Group_GetFileContentsCRC(strTarget, &iResContentsChks))
			return false;
		if ((!GrpContentsCRC2 || GrpContentsCRC2 != iResContentsChks) && iResChks != GrpChks2)
		{
			return false;
		}
	}

	return true;
}

bool C4UpdatePackage::Optimize(C4Group *pGroup, const char *strTarget)
{
	// Open target group
	C4GroupEx TargetGrp;
	if (!TargetGrp.Open(strTarget))
		return false;

	// Both groups must be packed
	if (!pGroup->IsPacked() || !TargetGrp.IsPacked())
	{
		TargetGrp.Close(false);
		return false;
	}

	// update children
	char ItemFileName[_MAX_PATH];
	pGroup->ResetSearch();
	while (pGroup->FindNextEntry("*", ItemFileName))
		if (!SEqual(ItemFileName, C4CFN_UpdateCore) && !SEqual(ItemFileName, C4CFN_UpdateEntries))
			Optimize(pGroup, &TargetGrp, ItemFileName);

	// set header
	if (TargetGrp.HeadIdentical(*pGroup, true))
		TargetGrp.SetHead(*pGroup);

	// save
	TargetGrp.Close(false);

	// okay
	return true;
}

C4UpdatePackage::CheckResult C4UpdatePackage::Check(C4Group *pGroup)
{
	// Version requirement is set
	if (RequireVersion[0])
	{
		// Engine and game version must match (rest ignored)
		if ((C4XVER1 != RequireVersion[0]) || (C4XVER2 != RequireVersion[1]))
			return CheckResult::BadSource;
	}

	// only group updates have any special needs
	if (!GrpUpdate) return CheckResult::Ok;

	// check source file
	C4Group TargetGrp;
	if (!TargetGrp.Open(DestPath))
		return CheckResult::NoSource;
	if (!TargetGrp.IsPacked())
		return CheckResult::BadSource;
	TargetGrp.Close();

	// check source crc
	uint32_t iCRC32, iContentsCRC32;
	if (!C4Group_GetFileContentsCRC(DestPath, &iContentsCRC32))
		return CheckResult::BadSource;

	if (GrpContentsCRC2 && GrpContentsCRC2 == iContentsCRC32)
		// so there's nothing to do
		return CheckResult::AlreadyUpdated;

	int i = 0;
	for (; i < UpGrpCnt; i++)
		if (GrpContentsCRC1[i] && iContentsCRC32 == GrpContentsCRC1[i])
			break;

	if (i >= UpGrpCnt)
	{
		if (!C4Group_GetFileCRC(DestPath, &iCRC32))
			return CheckResult::BadSource;
		// equal to destination group?
		if (iCRC32 == GrpChks2)
			// so there's nothing to do
			return CheckResult::AlreadyUpdated;
		// check if it's one of our registered sources
		int i = 0;
		for (; i < UpGrpCnt; i++)
			if (iCRC32 == GrpChks1[i])
				break;
		if (i >= UpGrpCnt)
			return CheckResult::BadSource;
	}

	// ok
	return CheckResult::Ok;
}

bool C4UpdatePackage::DoUpdate(C4Group *pGrpFrom, C4GroupEx *pGrpTo, const char *strFileName)
{
	// group file?
	C4Group ItemGroupFrom;
	if (ItemGroupFrom.OpenAsChild(pGrpFrom, strFileName))
	{
		// try to open target group
		C4GroupEx ItemGroupTo;
		char strTempGroup[_MAX_PATH + 1]; strTempGroup[0] = 0;
		if (!ItemGroupTo.OpenAsChild(pGrpTo, strFileName))
		{
			// create (emtpy) temp dir
			SCopy(GetCfg()->AtExePath("~tmp"), strTempGroup, _MAX_PATH);
			MakeTempFilename(strTempGroup);
			// open/create it
			if (!ItemGroupTo.Open(strTempGroup, true))
				return false;
		}
		// update children
		char ItemFileName[_MAX_PATH];
		ItemGroupFrom.ResetSearch();
		while (ItemGroupFrom.FindNextEntry("*", ItemFileName))
			if (!SEqual(ItemFileName, C4CFN_UpdateCore) && !SEqual(ItemFileName, C4CFN_UpdateEntries))
				DoUpdate(&ItemGroupFrom, &ItemGroupTo, ItemFileName);
		// set maker (always)
		ItemGroupTo.SetMaker(ItemGroupFrom.GetMaker());
		if (GrpUpdate)
		{
			DoGrpUpdate(&ItemGroupFrom, &ItemGroupTo);
			// write group (do not change any headers set by DoGrpUpdate!)
			ItemGroupTo.Close(false);
			// temporary group?
			if (strTempGroup[0])
				if (!pGrpTo->Move(strTempGroup, strFileName))
					return false;
			// set core (C4Group::Save overwrites it)
			pGrpTo->SaveEntryCore(*pGrpFrom, strFileName);
			pGrpTo->SetSavedEntryCore(strFileName);
			// flag as no-resort
			pGrpTo->SetNoSort(strFileName);
		}
		else
		{
			// write group
			ItemGroupTo.Close(true);
			// temporary group?
			if (strTempGroup[0])
				if (!pGrpTo->Move(strTempGroup, strFileName))
					return false;
		}
	}
	else
	{
		char strMsg[1024];
		sprintf(strMsg, "updating %s\\%s\n", pGrpTo->GetFullName().getData(), strFileName);
#ifdef _MSC_VER
		OutputDebugString(strMsg);
#elif _DEBUG
		puts(strMsg);
#endif
		if (!C4Group_CopyEntry(pGrpFrom, pGrpTo, strFileName))
			return false;
		// set core
		pGrpTo->SaveEntryCore(*pGrpFrom, strFileName);
		pGrpTo->SetSavedEntryCore(strFileName);
	}
	// ok
	return true;
}

bool C4UpdatePackage::DoGrpUpdate(C4Group *pUpdateData, C4GroupEx *pGrpTo)
{
	char *pData;
	// sort entries
	if (pUpdateData->LoadEntry(C4CFN_UpdateEntries, &pData, nullptr, 1))
	{
		// delete all entries that do not appear in the entries list
		char strItemName[_MAX_FNAME + 1], strItemName2[_MAX_FNAME + 1];
		pGrpTo->ResetSearch();
		while (pGrpTo->FindNextEntry("*", strItemName))
		{
			bool fGotIt = false;
			for (int i = 0; fGotIt = SCopySegment(pData, i, strItemName2, '|', _MAX_FNAME); i++)
			{
				// remove separator
				char *pSep = strchr(strItemName2, '=');
				if (pSep) *pSep = '\0';
				// in list?
				if (SEqual(strItemName, strItemName2))
					break;
			}
			if (!fGotIt)
				pGrpTo->DeleteEntry(strItemName);
		}
		// set entry times, set sort list
		char strSortList[32767] = "";
		for (int i = 0; SCopySegment(pData, i, strItemName, '|', _MAX_FNAME); i++)
		{
			// get time (if given)
			char *pTime = strchr(strItemName, '=');
			if (pTime) *pTime++ = '\0';
			// set
			if (pTime) pGrpTo->SetEntryTime(strItemName, atoi(pTime));
			// update EntryCRC32. This will make updates to old groups invalid
			// however, it's needed so updates will update the EntryCRC of *unchanged* files correctly
			pGrpTo->EntryCRC32(strItemName);
			// copy to sort list
			SAppend(strItemName, strSortList);
			SAppendChar('|', strSortList);
		}
		// sort by list
		pGrpTo->Sort(strSortList);
		delete[] pData;
	}
	// copy header from update group
	pGrpTo->SetHead(*pUpdateData);
	// ok
	return true;
}

bool C4UpdatePackage::Optimize(C4Group *pGrpFrom, C4GroupEx *pGrpTo, const char *strFileName)
{
	// group file?
	C4Group ItemGroupFrom;
	if (!ItemGroupFrom.OpenAsChild(pGrpFrom, strFileName))
		return true;
	// try to open target group
	C4GroupEx ItemGroupTo;
	char strTempGroup[_MAX_PATH + 1]; strTempGroup[0] = 0;
	if (!ItemGroupTo.OpenAsChild(pGrpTo, strFileName))
		return true;
	// update children
	char ItemFileName[_MAX_PATH];
	ItemGroupFrom.ResetSearch();
	while (ItemGroupFrom.FindNextEntry("*", ItemFileName))
		Optimize(&ItemGroupFrom, &ItemGroupTo, ItemFileName);
	// set head
	if (ItemGroupTo.HeadIdentical(ItemGroupFrom, true))
		ItemGroupTo.SetHead(ItemGroupFrom);
	// write group (do not change any headers set by DoGrpUpdate!)
	ItemGroupTo.Close(false);
	// set core (C4Group::Save overwrites it)
	pGrpTo->SaveEntryCore(*pGrpFrom, strFileName);
	pGrpTo->SetSavedEntryCore(strFileName);
	return true;
}

bool C4UpdatePackage::MakeUpdate(const char *strFile1, const char *strFile2, const char *strUpdateFile, const char *strName)
{
	// open Log
	if (!Log.Create("Update.log"))
		return false;

	// begin message
	WriteLog("Source: %s\nTarget: %s\nOutput: %s\n\n", strFile1, strFile2, strUpdateFile);

	// open both groups
	C4Group Group1, Group2;
	if (!Group1.Open(strFile1)) { WriteLog("Error: could not open %s!\n", strFile1); return false; }
	if (!Group2.Open(strFile2)) { WriteLog("Error: could not open %s!\n", strFile2); return false; }

	// All groups to be compared need to be packed
	if (!Group1.IsPacked()) { WriteLog("Error: source group %s not packed!\n", strFile1); return false; }
	if (!Group2.IsPacked()) { WriteLog("Error: target group %s not packed!\n", strFile2); return false; }
	if (Group1.HasPackedMother()) { WriteLog("Error: source group %s must not have a packed mother group!\n", strFile1); return false; }
	if (Group2.HasPackedMother()) { WriteLog("Error: target group %s must not have a packed mother group!\n", strFile2); return false; }

	// create/open update-group
	C4GroupEx UpGroup;
	if (!UpGroup.Open(strUpdateFile, true)) { WriteLog("Error: could not open %s!\n", strUpdateFile); return false; }

	// may be continued update-file -> try to load core
	UpGrpCnt = 0;
	bool fContinued = C4UpdatePackageCore::Load(UpGroup);

	// save crc2 for later check
	unsigned int iOldChks2 = GrpChks2;
	unsigned int iOldContentsChks2 = GrpContentsCRC2;

	// create core info
	if (strName)
		SCopy(strName, Name, C4MaxName);
	else
		sprintf(Name, "%s Update", GetFilename(strFile1));
	SCopy(strFile1, DestPath, _MAX_PATH);
	GrpUpdate = true;
	if (!C4Group_GetFileCRC(strFile1, &GrpChks1[UpGrpCnt]))
	{
		WriteLog("Error: could not calc checksum for %s!\n", strFile1); return false;
	}
	if (!C4Group_GetFileCRC(strFile2, &GrpChks2))
	{
		WriteLog("Error: could not calc checksum for %s!\n", strFile2); return false;
	}
	if (!C4Group_GetFileContentsCRC(strFile1, &GrpContentsCRC1[UpGrpCnt]))
	{
		WriteLog("Error: could not calc contents checksum for %s!\n", strFile1); return false;
	}
	if (!C4Group_GetFileContentsCRC(strFile2, &GrpContentsCRC2))
	{
		WriteLog("Error: could not calc contents checksum for %s!\n", strFile2); return false;
	}
	if (fContinued)
	{
		// continuation check: GrpChks2 matches?
		if (iOldContentsChks2 ? GrpContentsCRC2 != iOldContentsChks2 : GrpChks2 != iOldChks2)
			// that would mess up the update result...
		{
			WriteLog("Error: could not add to update package - target groups don't match (checksum error)\n"); return false;
		}
		// already supported by this update?
		int i = 0;
		for (; i < UpGrpCnt; i++)
			if (GrpChks1[UpGrpCnt] == GrpChks1[i] || (GrpContentsCRC1[i] != 0 && GrpContentsCRC1[UpGrpCnt] != 0 && GrpContentsCRC1[i] == GrpContentsCRC1[UpGrpCnt]))
				break;
		if (i < UpGrpCnt)
		{
			WriteLog("This update already supports the version of the source file.\n"); return false;
		}
	}

	UpGrpCnt++;

	// save core
	if (!C4UpdatePackageCore::Save(UpGroup))
	{
		WriteLog("Could not save update package core!\n"); return false;
	}

	// compare groups, create update
	bool fModified = false;
	bool fSuccess = MkUp(&Group1, &Group2, &UpGroup, &fModified);
	// close (save) it
	UpGroup.Close(false);
	// error?
	if (!fSuccess)
	{
		WriteLog("Update package not created.\n");
		remove(strUpdateFile);
		return false;
	}

	WriteLog("Update package created.\n");
	return true;
}

bool C4UpdatePackage::MkUp(C4Group *pGrp1, C4Group *pGrp2, C4GroupEx *pUpGrp, bool *fModified)
{
	// (CAUTION: pGrp1 may be nullptr - that means that there is no counterpart for Grp2
	//           in the base group)

	// compare headers
	if (!pGrp1 ||
		pGrp1->GetCreation() != pGrp2->GetCreation() ||
		pGrp1->GetOriginal() != pGrp2->GetOriginal() ||
		!SEqual(pGrp1->GetMaker(), pGrp2->GetMaker()) ||
		!SEqual(pGrp1->GetPassword(), pGrp2->GetPassword()))
		*fModified = true;
	// set header
	pUpGrp->SetHead(*pGrp2);
	// compare entries
	char strItemName[_MAX_PATH], strItemName2[_MAX_PATH]; StdStrBuf EntryList;
	strItemName[0] = strItemName2[0] = 0;
	pGrp2->ResetSearch(); if (!*fModified) pGrp1->ResetSearch();
	int iChangedEntries = 0;
	while (pGrp2->FindNextEntry("*", strItemName, nullptr, nullptr, !!strItemName[0]))
	{
		// add to entry list
		if (!!EntryList) EntryList.AppendChar('|');
		EntryList.AppendFormat("%s=%d", strItemName, pGrp2->EntryTime(strItemName));
		// no modification detected yet? then check order
		if (!*fModified)
		{
			if (!pGrp1->FindNextEntry("*", strItemName2, nullptr, nullptr, !!strItemName2[0]))
				*fModified = true;
			else if (!SEqual(strItemName, strItemName2))
				*fModified = true;
		}

		// TODO: write DeleteEntries.txt

		// a child group?
		C4GroupEx ChildGrp2;
		if (ChildGrp2.OpenAsChild(pGrp2, strItemName))
		{
			// open in Grp1
			C4Group *pChildGrp1 = new C4GroupEx();
			if (!pGrp1 || !pChildGrp1->OpenAsChild(pGrp1, strItemName))
			{
				delete pChildGrp1; pChildGrp1 = nullptr;
			}
			// open group for update data
			C4GroupEx UpdGroup; char strTempGroupName[_MAX_FNAME + 1];
			strTempGroupName[0] = 0;
			if (!UpdGroup.OpenAsChild(pUpGrp, strItemName))
			{
				// create new group (may be temporary)
				SCopy(GetCfg()->AtTempPath("~upd"), strTempGroupName, _MAX_FNAME);
				MakeTempFilename(strTempGroupName);
				if (!UpdGroup.Open(strTempGroupName, true)) { delete pChildGrp1; WriteLog("Error: could not create temp group\n"); return false; }
			}
			// do nested MkUp-search
			bool Modified = false;
			bool fSuccess = MkUp(pChildGrp1, &ChildGrp2, &UpdGroup, &Modified);
			// sort & close
			extern const char **C4Group_SortList;
			UpdGroup.SortByList(C4Group_SortList, ChildGrp2.GetName());
			UpdGroup.Close(false);
			// check entry times
			if (!pGrp1 || (pGrp1->EntryTime(strItemName) != pGrp2->EntryTime(strItemName)))
				Modified = true;
			// add group (if modified)
			if (fSuccess && Modified)
			{
				if (strTempGroupName[0])
					if (!pUpGrp->Move(strTempGroupName, strItemName))
					{
						WriteLog("Error: could not add modified group\n");
						return false;
					}
				// copy core
				pUpGrp->SaveEntryCore(*pGrp2, strItemName);
				pUpGrp->SetSavedEntryCore(strItemName);
				// got a modification in a subgroup
				*fModified = true;
				iChangedEntries++;
			}
			else
				// delete group (do not remove groups that existed before!)
				if (strTempGroupName[0])
					if (remove(strTempGroupName))
						if (rmdir(strTempGroupName))
						{
							WriteLog("Error: could not delete temporary directory\n"); return false;
						}
			delete pChildGrp1;
		}
		else
		{
			// compare them (size & crc32)
			if (!pGrp1 ||
				pGrp1->EntrySize(strItemName) != pGrp2->EntrySize(strItemName) ||
				pGrp1->EntryCRC32(strItemName) != pGrp2->EntryCRC32(strItemName))
			{
				bool fCopied = false;

				// save core (EntryCRC32 might set additional fields)
				pUpGrp->SaveEntryCore(*pGrp2, strItemName);

				// already in update grp?
				if (pUpGrp->EntryTime(strItemName) != pGrp2->EntryTime(strItemName) ||
					pUpGrp->EntrySize(strItemName) != pGrp2->EntrySize(strItemName) ||
					pUpGrp->EntryCRC32(strItemName) != pGrp2->EntryCRC32(strItemName))
				{
					// copy it
					if (!C4Group_CopyEntry(pGrp2, pUpGrp, strItemName))
					{
						WriteLog("Error: could not add changed entry to update group\n");
						return false;
					}
					// set entry core
					pUpGrp->SetSavedEntryCore(strItemName);
					// modified...
					*fModified = true;
					fCopied = true;
				}
				iChangedEntries++;

				WriteLog("%s\\%s: update%s\n", pGrp2->GetFullName().getData(), strItemName, fCopied ? "" : " (already in group)");
			}
		}
	}
	// write entries list (always)
	if (!pUpGrp->Add(C4CFN_UpdateEntries, EntryList, false, true))
	{
		WriteLog("Error: could not save entry list!");
		return false;
	}

	if (iChangedEntries > 0)
		WriteLog("%s: %d/%d changed (%s)\n", pGrp2->GetFullName().getData(), iChangedEntries, pGrp2->EntryCount(), *fModified ? "update" : "skip");

	// success
	return true;
}
