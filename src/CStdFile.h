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

/* A handy wrapper class to gzio files */

#pragma once

#include <StdFile.h>
#include <StdGzCompressedFile.h>

#include <memory>
#include <cstdio>

constexpr unsigned int CStdFileBufSize = 4096;

class CStdFile
{
public:
	CStdFile();
	~CStdFile();
	bool Status;
	char Name[_MAX_PATH + 1];

protected:
	FILE *hFile;
	std::shared_ptr<StdGzCompressedFile::Read> readCompressedFile;
	std::shared_ptr<StdGzCompressedFile::Write> writeCompressedFile;
	uint8_t Buffer[CStdFileBufSize];
	size_t BufferLoad, BufferPtr;
	bool ModeWrite;

public:
	bool Create(const char *szFileName, bool fCompressed = false, bool fExecutable = false, bool exclusive = false);
	bool Open(const char *szFileName, bool fCompressed = false);
	bool Append(const char *szFilename); // append (uncompressed only)
	bool Close();
	bool Default();
	bool Read(void *pBuffer, size_t iSize) { return Read(pBuffer, iSize, nullptr); }
	bool Read(void *pBuffer, size_t iSize, size_t *ipFSize);
	bool Write(const void *pBuffer, size_t iSize);
	bool WriteString(const char *szStr);
	bool Rewind();
	bool Advance(size_t iOffset);
	// Single line commands
	bool Load(const char *szFileName, uint8_t **lpbpBuf,
		size_t *ipSize = nullptr, int iAppendZeros = 0,
		bool fCompressed = false);
	bool Save(const char *szFileName, const uint8_t *bpBuf,
			  size_t iSize,
			  bool fCompressed = false,
			  bool executable = false,
			  bool exclusive = false);
	// flush contents to disk
	inline bool Flush() { if (ModeWrite && BufferLoad) return SaveBuffer(); else return true; }
	size_t AccessedEntrySize();

protected:
	void ClearBuffer();
	size_t LoadBuffer();
	bool SaveBuffer();
};

size_t UncompressedFileSize(const char *szFileName);
