/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
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

/* Lots of file helpers */

#pragma once

#include <ctime>

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <io.h>
#define F_OK 0
#define MakeDirectory CreateDirectoryA
#else
#include <dirent.h>
#include <limits.h>
#define _O_BINARY 0
#define _MAX_PATH PATH_MAX
#define _MAX_FNAME NAME_MAX

bool MakeDirectory(const char *pathname, void * = nullptr);
bool CopyFile(const char *szSource, const char *szTarget, bool FailIfExists);
#endif

#ifdef _WIN32
#define DirSep "\\"
#define DirectorySeparator '\\'
#define AltDirectorySeparator '/'
#else
#define DirSep "/"
#define DirectorySeparator '/'
#define AltDirectorySeparator '\\'
#endif

const char *GetWorkingDirectory();
bool SetWorkingDirectory(const char *szPath);
char *GetFilename(char *path);
const char *GetFilenameOnly(const char *strFilename);
const char *GetC4Filename(const char *szPath); // returns path to file starting at first .c4*-directory
int GetTrailingNumber(const char *strString);
char *GetExtension(char *fname);
const char *GetFilename(const char *path);
const char *GetExtension(const char *fname);
void DefaultExtension(char *szFileName, const char *szExtension);
void EnforceExtension(char *szFileName, const char *szExtension);
void EnforceExtension(class StdStrBuf *sFilename, const char *szExtension);
void RemoveExtension(char *szFileName);
void RemoveExtension(StdStrBuf *psFileName);
void AppendBackslash(char *szFileName);
void TruncateBackslash(char *szFilename);
void MakeTempFilename(char *szFileName);
void MakeTempFilename(class StdStrBuf *sFileName);
bool WildcardListMatch(const char *szWildcardList, const char *szString); // match string in list like *.png|*.bmp
bool WildcardMatch(const char *szFName1, const char *szFName2);
bool TruncatePath(char *szPath);
// szBuffer has to be of at least _MAX_PATH length.
bool GetParentPath(const char *szFilename, char *szBuffer);
bool GetParentPath(const char *szFilename, StdStrBuf *outBuf);
bool GetRelativePath(const char *strPath, const char *strRelativeTo, char *strBuffer, int iBufferSize = _MAX_PATH);
const char *GetRelativePathS(const char *strPath, const char *strRelativeTo);
bool IsGlobalPath(const char *szPath);

bool DirectoryExists(const char *szFileName);
bool FileExists(const char *szFileName);
size_t FileSize(const char *fname);
size_t FileSize(int fdes);
time_t FileTime(const char *fname);
bool EraseFile(const char *szFileName);
bool RenameFile(const char *szFileName, const char *szNewFileName);
bool MakeOriginalFilename(char *szFilename);
void MakeFilenameFromTitle(char *szTitle);

bool CopyDirectory(const char *szSource, const char *szTarget, bool fResetAttributes = false);
bool EraseDirectory(const char *szDirName);

bool ItemIdentical(const char *szFilename1, const char *szFilename2);
inline bool ItemExists(const char *szItemName) { return FileExists(szItemName); }
bool RenameItem(const char *szItemName, const char *szNewItemName);
bool EraseItem(const char *szItemName);
bool CopyItem(const char *szSource, const char *szTarget, bool fResetAttributes = false);
bool CreateItem(const char *szItemname);
bool MoveItem(const char *szSource, const char *szTarget);

int ForEachFile(const char *szDirName, bool(*fnCallback)(const char *));

class DirectoryIterator
{
public:
	DirectoryIterator(const char *dirname);
	DirectoryIterator();
	~DirectoryIterator();
	// prevent misuses
	DirectoryIterator(const DirectoryIterator &) = delete;
	const char *operator*() const;
	DirectoryIterator &operator++();
	void operator++(int);
	void Reset(const char *dirname);
	void Reset();

protected:
	char filename[_MAX_PATH + 1];
#ifdef _WIN32
	struct _finddata_t fdt; intptr_t fdthnd;

	friend class C4GroupEntry;
#else
	DIR *d;
	dirent *ent;
#endif
};

bool ReadFileLine(FILE *fhnd, char *tobuf, int maxlen);
void AdvanceFileLine(FILE *fhnd);
