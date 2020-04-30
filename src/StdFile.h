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

/* Lots of file helpers */

#pragma once

#include <Standard.h>
#include <stdio.h>
#include <stdlib.h>

#include <filesystem>
#define STDFILE_DEPRECATED [[deprecated("Use std::filesystem instead.")]]

#ifdef _WIN32
#include <io.h>
#define F_OK 0
#else
#include <dirent.h>
#include <limits.h>
#define _O_BINARY 0
#define _MAX_PATH PATH_MAX
#define _MAX_FNAME NAME_MAX

STDFILE_DEPRECATED bool CreateDirectory(const char *pathname, void * = 0);
STDFILE_DEPRECATED bool CopyFile(const char *szSource, const char *szTarget, bool FailIfExists);
#endif

#ifdef _WIN32
#define DirectorySeparator '\\'
#define AltDirectorySeparator '/'
#else
#define DirectorySeparator '/'
#define AltDirectorySeparator '\\'
#endif

STDFILE_DEPRECATED const char *GetWorkingDirectory();
STDFILE_DEPRECATED bool SetWorkingDirectory(const char *szPath);
STDFILE_DEPRECATED char *GetFilename(char *path);
STDFILE_DEPRECATED const char *GetFilenameOnly(const char *strFilename);
const char *GetC4Filename(const char *szPath); // returns path to file starting at first .c4*-directory
int GetTrailingNumber(const char *strString);
STDFILE_DEPRECATED char *GetExtension(char *fname);
STDFILE_DEPRECATED const char *GetFilename(const char *path);
STDFILE_DEPRECATED const char *GetExtension(const char *fname);
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
STDFILE_DEPRECATED bool GetParentPath(const char *szFilename, char *szBuffer);
STDFILE_DEPRECATED bool GetParentPath(const char *szFilename, StdStrBuf *outBuf);
STDFILE_DEPRECATED bool GetRelativePath(const char *strPath, const char *strRelativeTo, char *strBuffer, int iBufferSize = _MAX_PATH);
STDFILE_DEPRECATED const char *GetRelativePathS(const char *strPath, const char *strRelativeTo);
bool IsGlobalPath(const char *szPath);

STDFILE_DEPRECATED bool DirectoryExists(const char *szFileName);
STDFILE_DEPRECATED bool FileExists(const char *szFileName);
STDFILE_DEPRECATED size_t FileSize(const char *fname);
STDFILE_DEPRECATED size_t FileSize(int fdes);
STDFILE_DEPRECATED int FileTime(const char *fname);
bool EraseFile(const std::filesystem::path &fileName);
STDFILE_DEPRECATED bool RenameFile(const char *szFileName, const char *szNewFileName);
bool MakeOriginalFilename(char *szFilename);
void MakeFilenameFromTitle(char *szTitle);

STDFILE_DEPRECATED bool CopyDirectory(const char *szSource, const char *szTarget, bool fResetAttributes = false);
STDFILE_DEPRECATED bool EraseDirectory(const char *szDirName);

STDFILE_DEPRECATED bool ItemIdentical(const char *szFilename1, const char *szFilename2);
STDFILE_DEPRECATED inline bool ItemExists(const char *szItemName) { return FileExists(szItemName); }
STDFILE_DEPRECATED bool RenameItem(const char *szItemName, const char *szNewItemName);
STDFILE_DEPRECATED bool EraseItem(const char *szItemName);
STDFILE_DEPRECATED bool CopyItem(const char *szSource, const char *szTarget, bool fResetAttributes = false);
STDFILE_DEPRECATED bool CreateItem(const char *szItemname);
STDFILE_DEPRECATED bool MoveItem(const char *szSource, const char *szTarget);

STDFILE_DEPRECATED int ForEachFile(const char *szDirName, bool(*fnCallback)(const char *));

class DirectoryIterator
{
public:
	STDFILE_DEPRECATED DirectoryIterator(const char *dirname);
	STDFILE_DEPRECATED DirectoryIterator();
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
	struct _finddata_t fdt; int fdthnd;

	friend class C4GroupEntry;
#else
	DIR *d;
	dirent *ent;
#endif
};

STDFILE_DEPRECATED bool ReadFileLine(FILE *fhnd, char *tobuf, int maxlen);
STDFILE_DEPRECATED void AdvanceFileLine(FILE *fhnd);

STDFILE_DEPRECATED std::string operator/(const std::string &left, const std::string &right);

#undef STDFILE_DEPRECATED
