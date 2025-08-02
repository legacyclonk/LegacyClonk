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

/* Handles group files */

#pragma once

#include <CStdFile.h>
#include <StdBuf.h>
#include <StdCompiler.h>

// C4Group-Rewind-warning:
// The current C4Group-implementation cannot handle random file access very well,
// because all files are written within a single zlib-stream.
// For every out-of-order-file accessed a group-rewind must be performed, and every
// single file up to the accessed file unpacked. As a workaround, all C4Groups are
// packed in a file order matching the reading order of the engine.
// If the reading order doesn't match the packing order, and a rewind has to be performed,
// a warning is issued in Debug-builds of the engine. But since some components require
// random access because they are loaded on-demand at runtime (e.g. global sounds), the
// warning may be temp disabled for those files using C4GRP_DISABLE_REWINDWARN and
// C4GRP_ENABLE_REWINDWARN. A ref counter keeps track of nested calls to those functions.
//
// If you add any new components to scenario or definition files, remember to adjust the
// sort order lists in C4Components.h accordingly, and enforce a reading order for that
// component.
//
// Maybe some day, someone will write a C4Group-implementation that is probably capable of
// random access...
#ifndef NDEBUG
extern int iC4GroupRewindFilePtrNoWarn;
#define C4GRP_DISABLE_REWINDWARN ++iC4GroupRewindFilePtrNoWarn;
#define C4GRP_ENABLE_REWINDWARN --iC4GroupRewindFilePtrNoWarn;
#else
#define C4GRP_DISABLE_REWINDWARN ;
#define C4GRP_ENABLE_REWINDWARN ;
#endif

const int C4GroupFileVer1 = 1, C4GroupFileVer2 = 2;

const int C4GroupMaxMaker = 30,
          C4GroupMaxPassword = 30,
          C4GroupMaxError = 100;

#define C4GroupFileID "RedWolf Design GrpFolder"

void C4Group_SetMaker(const char *szMaker);
void C4Group_SetTempPath(const char *szPath);
const char *C4Group_GetTempPath();
void C4Group_SetSortList(const char **ppSortList);
void C4Group_SetProcessCallback(bool(*fnCallback)(const char *, int));
bool C4Group_IsGroup(const char *szFilename);
bool C4Group_CopyItem(const char *szSource, const char *szTarget, bool fNoSort = false, bool fResetAttributes = false);
bool C4Group_MoveItem(const char *szSource, const char *szTarget, bool fNoSort = false);
bool C4Group_DeleteItem(const char *szItem, bool fRecycle = false);
bool C4Group_PackDirectoryTo(const char *szFilename, const char *szFilenameTo, bool overwriteTarget = false);
bool C4Group_PackDirectory(const char *szFilename);
bool C4Group_UnpackDirectory(const char *szFilename);
bool C4Group_ExplodeDirectory(const char *szFilename);
bool C4Group_ReadFile(const char *szFilename, char **pData, size_t *iSize);
bool C4Group_GetFileCRC(const char *szFilename, uint32_t *pCRC32);
bool C4Group_GetFileContentsCRC(const char *szFilename, uint32_t *pCRC32);
bool C4Group_GetFileSHA1(const char *szFilename, uint8_t *pSHA1);

bool EraseItemSafe(const char *szFilename);

extern const char *C4CFN_FLS[];

extern time_t C4Group_AssumeTimeOffset;

#pragma pack (push, 1)

class C4GroupHeader
{
public:
	char id[24 + 4]{};
	int32_t Ver1{}, Ver2{};
	int32_t Entries{};
	char Maker[C4GroupMaxMaker + 2]{};
	char Password[C4GroupMaxPassword + 2]{};
	int32_t Creation{}, Original{};
	uint8_t fbuf[92]{};

public:
	void Init();
};

const char C4GECS_None = 0,
           C4GECS_Old = 1,
           C4GECS_New = 2;

class C4GroupEntryCore
{
public:
	char FileName[260]{};
	int32_t Packed{}, ChildGroup{};
	int32_t Size{}, __Unused{}, Offset{};
	uint32_t Time{};
	char HasCRC{};
	unsigned int CRC{};
	char Executable{};
	uint8_t fbuf[26]{};
};

#pragma pack (pop)

const int C4GRES_InGroup = 0,
          C4GRES_OnDisk = 1,
          C4GRES_InMemory = 2,
          C4GRES_Deleted = 3;

class C4GroupEntry : public C4GroupEntryCore
{
public:
	~C4GroupEntry();

public:
	char DiskPath[_MAX_PATH + 1]{};
	int Status{};
	bool DeleteOnDisk{};
	bool HoldBuffer{};
	bool BufferIsStdbuf{};
	bool NoSort{};
	uint8_t *bpMemBuf{};
	C4GroupEntry *Next{};

public:
	void Set(const DirectoryIterator &iter, const char *szPath);
};

const int GRPF_Inactive = 0,
          GRPF_File = 1,
          GRPF_Folder = 2;

class C4Group
{
public:
	enum class OpenFlags
	{
		None = 0,
		Overwrite = 1 << 0
	};

public:
	C4Group();
	~C4Group();

	C4Group(const C4Group &) = delete;
	C4Group &operator=(const C4Group &) = delete;

	C4Group(C4Group &&) = delete;
	C4Group &operator=(C4Group &&) = delete;

protected:
	int Status;
	char FileName[_MAX_PATH + 1];
	// Parent status
	C4Group *Mother;
	bool ExclusiveChild;
	// File & Folder
	C4GroupEntry *SearchPtr;
	CStdFile StdFile;
	size_t iCurrFileSize; // size of last accessed file
	// File only
	size_t FilePtr;
	int MotherOffset;
	int EntryOffset;
	bool Modified;
	C4GroupHeader Head;
	C4GroupEntry *FirstEntry;
	// Folder only
	DirectoryIterator FolderSearch;
	C4GroupEntry FolderSearchEntry;
	C4GroupEntry LastFolderSearchEntry;

	bool StdOutput;
	bool(*fnProcessCallback)(const char *, int);
	char ErrorString[C4GroupMaxError + 1];
	bool MadeOriginal;

	bool NoSort; // If this flag is set, all entries will be marked NoSort in AddEntry

public:
	bool Open(const char *szGroupName, bool fCreate = false, OpenFlags flags = OpenFlags::None);
	bool Close();
	bool Save(bool fReOpen);
	bool OpenAsChild(C4Group *pMother, const char *szEntryName, bool fExclusive = false);
	C4Group *GrabMother();
	bool Add(const char *szFiles);
	bool Add(const char *szFile, const char *szAddAs);
	bool Add(const char *szName, void *pBuffer, size_t iSize, bool fChild = false, bool fHoldBuffer = false, time_t iTime = 0, bool fExecutable = false);
	bool Add(const char *szName, StdBuf &pBuffer, bool fChild = false, bool fHoldBuffer = false, time_t iTime = 0, bool fExecutable = false);
	bool Add(const char *szName, StdStrBuf &pBuffer, bool fChild = false, bool fHoldBuffer = false, time_t iTime = 0, bool fExecutable = false);
	bool Merge(const char *szFolders);
	bool Move(const char *szFiles);
	bool Move(const char *szFile, const char *szAddAs);
	bool Extract(const char *szFiles, const char *szExtractTo = nullptr, const char *szExclude = nullptr);
	bool ExtractEntry(const char *szFilename, const char *szExtractTo = nullptr);
	bool Delete(const char *szFiles, bool fRecursive = false);
	bool DeleteEntry(const char *szFilename, bool fRecycle = false);
	bool Rename(const char *szFile, const char *szNewName);
	bool Sort(const char *szSortList);
	bool SortByList(const char **ppSortList, const char *szFilename = nullptr);
	bool View(const char *szFiles);
	bool GetOriginal();
	bool AccessEntry(const char *szWildCard,
		size_t *iSize = nullptr, char *sFileName = nullptr,
		bool *fChild = nullptr);
	bool AccessNextEntry(const char *szWildCard,
		size_t *iSize = nullptr, char *sFileName = nullptr,
		bool *fChild = nullptr);
	bool LoadEntry(const char *szEntryName, char **lpbpBuf,
		size_t *ipSize = nullptr, int iAppendZeros = 0);
	bool LoadEntry(const char *szEntryName, StdBuf &Buf);
	bool LoadEntryString(const char *szEntryName, StdStrBuf &Buf);
	bool FindEntry(const char *szWildCard,
		char *sFileName = nullptr,
		size_t *iSize = nullptr,
		bool *fChild = nullptr);
	bool FindNextEntry(const char *szWildCard,
		char *sFileName = nullptr,
		size_t *iSize = nullptr,
		bool *fChild = nullptr,
		bool fStartAtFilename = false);
	bool Read(void *pBuffer, size_t iSize);
	bool Advance(size_t iOffset);
	void SetMaker(const char *szMaker);
	void SetStdOutput(bool fStatus);
	void MakeOriginal(bool fOriginal);
	void ResetSearch();
	const char *GetError();
	const char *GetMaker();
	const char *GetPassword();
	const char *GetName();
	StdStrBuf GetFullName() const;
	int EntryCount(const char *szWildCard = nullptr);
	int EntrySize(const char *szWildCard = nullptr);
	size_t AccessedEntrySize() { return iCurrFileSize; } // retrieve size of last accessed entry
	uint32_t EntryTime(const char *szFilename);
	unsigned int EntryCRC32(const char *szWildCard = nullptr);
	int32_t GetCreation();
	int GetStatus();
	inline bool IsOpen() { return Status != GRPF_Inactive; }
	C4Group *GetMother();
	inline bool IsPacked() { return Status == GRPF_File; }
	inline bool HasPackedMother() { if (!Mother) return false; return Mother->IsPacked(); }
	inline bool SetNoSort(bool fNoSort) { NoSort = fNoSort; return true; }
#ifndef NDEBUG
	void PrintInternals(const char *szIndent = nullptr);
#endif

protected:
	void Init();
	void Default();
	void Clear();
	bool EnsureChildFilePtr(C4Group *pChild);
	bool CloseExclusiveMother();
	bool Error(const char *szStatus);
	bool OpenReal(const char *szGroupName);
	bool OpenRealGrpFile();
	bool SetFilePtr(size_t iOffset);
	bool RewindFilePtr();
	bool AdvanceFilePtr(size_t iOffset, C4Group *pByChild = nullptr);
	bool AddEntry(int status,
		bool childgroup,
		const char *fname,
		size_t size,
		time_t time,
		char cCRC,
		unsigned int iCRC,
		const char *entryname = nullptr,
		uint8_t *membuf = nullptr,
		bool fDeleteOnDisk = false,
		bool fHoldBuffer = false,
		bool fExecutable = false,
		bool fBufferIsStdbuf = false);
	bool AddEntryOnDisk(const char *szFilename, const char *szAddAs = nullptr, bool fMove = false);
	bool SetFilePtr2Entry(const char *szName, C4Group *pByChild = nullptr);
	bool AppendEntry2StdFile(C4GroupEntry *centry, CStdFile &stdfile);
	C4GroupEntry *GetEntry(const char *szName);
	C4GroupEntry *SearchNextEntry(const char *szName);
	C4GroupEntry *GetNextFolderEntry();
	bool CalcCRC32(C4GroupEntry *pEntry);
};

inline C4Group::OpenFlags operator|(const C4Group::OpenFlags first, const C4Group::OpenFlags second) noexcept
{
	return static_cast<C4Group::OpenFlags>(std::to_underlying(first) | std::to_underlying(second));
}

inline C4Group::OpenFlags operator&(const C4Group::OpenFlags first, const C4Group::OpenFlags second) noexcept
{
	return static_cast<C4Group::OpenFlags>(std::to_underlying(first) & std::to_underlying(second));
}
