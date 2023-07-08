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

#include <Standard.h>
#include <StdFile.h>
#include <CStdFile.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>

#include <sys/stat.h>
#include <fcntl.h>

#include <algorithm>
#include <cstring>

CStdFile::CStdFile()
{
	Status = false;
	hFile = nullptr;
	ClearBuffer();
	ModeWrite = false;
	Name[0] = 0;
}

CStdFile::~CStdFile()
{
	Close();
}

bool CStdFile::Create(const char *szFilename, bool fCompressed, bool fExecutable, bool exclusive)
{
	SCopy(szFilename, Name, _MAX_PATH);
	// Set modes
	ModeWrite = true;
	// Open standard file
	if (fCompressed)
	{
		try
		{
			writeCompressedFile.reset(new StdGzCompressedFile::Write{szFilename});
		}
		catch (const StdGzCompressedFile::Exception &)
		{
			return false;
		}
	}
	else
	{
		if (fExecutable)
		{
			// Create an executable file
#ifdef _WIN32
			int mode = _S_IREAD | _S_IWRITE;
			int flags = _O_BINARY | _O_CREAT | _O_WRONLY | _O_TRUNC | (exclusive ? _O_EXCL : 0);
#else
			mode_t mode = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH;
			int flags = O_CREAT | O_WRONLY | O_TRUNC | (exclusive ? O_EXCL : 0);
#endif
			int fd = open(Name, flags, mode);
			if (fd == -1) return false;
			if (!(hFile = fdopen(fd, "wb"))) return false;
		}
		else
		{
			if (!(hFile = std::fopen(Name, "wb"))) return false;
		}
	}
	// Reset buffer
	ClearBuffer();
	// Set status
	Status = true;
	return true;
}

bool CStdFile::Open(const char *szFilename, bool fCompressed)
{
	SCopy(szFilename, Name, _MAX_PATH);
	// Set modes
	ModeWrite = false;
	// Open standard file
	if (fCompressed)
	{
		try
		{
			readCompressedFile.reset(new StdGzCompressedFile::Read{szFilename});
		}
		catch (const StdGzCompressedFile::Exception &)
		{
			return false;
		}
	}
	else
	{
		if (!(hFile = std::fopen(Name, "rb"))) return false;
	}
	// Reset buffer
	ClearBuffer();
	// Set status
	Status = true;
	return true;
}

bool CStdFile::Append(const char *szFilename)
{
	SCopy(szFilename, Name, _MAX_PATH);
	// Set modes
	ModeWrite = true;
	// Open standard file
	if (!(hFile = std::fopen(Name, "ab"))) return false;
	// Reset buffer
	ClearBuffer();
	// Set status
	Status = true;
	return true;
}

bool CStdFile::Close()

{
	bool rval = true;
	Status = false;
	Name[0] = 0;
	// Save buffer if in write mode
	if (ModeWrite && BufferLoad) if (!SaveBuffer()) rval = false;
	// Close file(s)
	readCompressedFile.reset();
	writeCompressedFile.reset();
	if (hFile) if (std::fclose(hFile) != 0) rval = false;
	hFile = nullptr;
	return !!rval;
}

bool CStdFile::Default()
{
	Status = false;
	Name[0] = 0;
	readCompressedFile.reset();
	writeCompressedFile.reset();
	hFile = nullptr;
	BufferLoad = BufferPtr = 0;
	return true;
}

bool CStdFile::Read(void *pBuffer, size_t iSize, size_t *ipFSize)
{
	size_t transfer;
	if (!pBuffer) return false;
	if (ModeWrite) return false;
	uint8_t *bypBuffer = static_cast<uint8_t *>(pBuffer);
	if (ipFSize) *ipFSize = 0;
	while (iSize > 0)
	{
		// Valid data in the buffer: Transfer as much as possible
		if (BufferLoad > BufferPtr)
		{
			transfer = std::min<size_t>(BufferLoad - BufferPtr, iSize);
			memmove(bypBuffer, Buffer + BufferPtr, transfer);
			BufferPtr += transfer;
			bypBuffer += transfer;
			iSize -= transfer;
			if (ipFSize) *ipFSize += transfer;
		}
		// Buffer empty: Load
		else if (LoadBuffer() <= 0) return false;
	}
	return true;
}

size_t CStdFile::LoadBuffer()
{
	if (hFile) BufferLoad = std::fread(Buffer, 1, CStdFileBufSize, hFile);
	if (readCompressedFile)
	{
		try
		{
			BufferLoad = readCompressedFile->ReadData(Buffer, CStdFileBufSize);
		}
		catch (const StdGzCompressedFile::Exception &)
		{
			BufferLoad = 0;
		}
	}
	BufferPtr = 0;
	return BufferLoad;
}

bool CStdFile::SaveBuffer()
{
	size_t saved = 0;
	if (hFile) saved = std::fwrite(Buffer, 1, BufferLoad, hFile);
	if (writeCompressedFile)
	{
		try
		{
			writeCompressedFile->WriteData(Buffer, BufferLoad);
			saved = BufferLoad;
		}
		catch (const StdGzCompressedFile::Exception &)
		{
			return false;
		}
	}
	if (saved != BufferLoad) return false;
	BufferLoad = 0;
	return true;
}

void CStdFile::ClearBuffer()
{
	BufferLoad = BufferPtr = 0;
}

bool CStdFile::Write(const void *pBuffer, size_t iSize)
{
	size_t transfer;
	if (!pBuffer) return false;
	if (!ModeWrite) return false;
	const uint8_t *bypBuffer = static_cast<const uint8_t *>(pBuffer);
	while (iSize > 0)
	{
		// Space in buffer: Transfer as much as possible
		if (BufferLoad < CStdFileBufSize)
		{
			transfer = std::min(CStdFileBufSize - BufferLoad, iSize);
			memcpy(Buffer + BufferLoad, bypBuffer, transfer);
			BufferLoad += transfer;
			bypBuffer += transfer;
			iSize -= transfer;
		}
		// Buffer full: Save
		else if (!SaveBuffer()) return false;
	}
	return true;
}

bool CStdFile::WriteString(const char *szStr)
{
	uint8_t nl[2] = { 0x0D, 0x0A };
	if (!szStr) return false;
	const auto size = SLen(szStr);
	if (!Write(static_cast<const void *>(szStr), size)) return false;
	if (!Write(nl, 2)) return false;
	return true;
}

bool CStdFile::Rewind()
{
	if (ModeWrite) return false;
	ClearBuffer();
	if (hFile) std::rewind(hFile);
	if (readCompressedFile)
	{
		readCompressedFile->Rewind();
	}
	return true;
}

bool CStdFile::Advance(size_t iOffset)
{
	if (ModeWrite) return false;
	while (iOffset > 0)
	{
		// Valid data in the buffer: Transfer as much as possible
		if (BufferLoad > BufferPtr)
		{
			const auto transfer = std::min(BufferLoad - BufferPtr, iOffset);
			BufferPtr += transfer;
			iOffset -= transfer;
		}
		// Buffer empty: Load or skip
		else
		{
			if (hFile) return !std::fseek(hFile, iOffset, SEEK_CUR); // uncompressed: Just skip
			if (LoadBuffer() <= 0) return false; // compressed: Read...
		}
	}
	return true;
}

bool CStdFile::Save(const char *szFilename, const uint8_t *bpBuf,
					size_t iSize, bool fCompressed, bool executable, bool exclusive)
{
	if (!bpBuf || (iSize < 1)) return false;
	if (!Create(szFilename, fCompressed, executable, exclusive)) return false;
	if (!Write(bpBuf, iSize)) return false;
	if (!Close()) return false;
	return true;
}

bool CStdFile::Load(const char *szFilename, uint8_t **lpbpBuf,
	size_t *ipSize, int iAppendZeros,
	bool fCompressed)
{
	size_t iSize = fCompressed ? UncompressedFileSize(szFilename) : FileSize(szFilename);
	if (!lpbpBuf || (iSize < 1)) return false;
	if (!Open(szFilename, fCompressed)) return false;
	*lpbpBuf = new uint8_t[iSize + iAppendZeros];
	if (!Read(*lpbpBuf, iSize)) { delete[] *lpbpBuf; return false; }
	if (iAppendZeros) std::fill_n(*lpbpBuf + iSize, iAppendZeros, 0);
	if (ipSize) *ipSize = iSize;
	Close();
	return true;
}

size_t UncompressedFileSize(const char *szFilename)
{
	try
	{
		StdGzCompressedFile::Read file{szFilename};
		return file.UncompressedSize();
	}
	catch (const StdGzCompressedFile::Exception &)
	{
		return 0;
	}
}

size_t CStdFile::AccessedEntrySize()
{
	if (hFile)
	{
		long pos = std::ftell(hFile);
		std::fseek(hFile, 0, SEEK_END);
		long r = std::ftell(hFile);
		std::fseek(hFile, pos, SEEK_SET);
		return static_cast<size_t>(r);
	}
	assert(!readCompressedFile);
	return 0;
}
