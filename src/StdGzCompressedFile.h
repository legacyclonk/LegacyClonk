/*
 * LegacyClonk
 *
 * Copyright (c) 2017-2021, The LegacyClonk Team and contributors
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

// wraps zlib's inflate for reading and deflate for writing gzip compressed group files with C4Group magic bytes

#pragma once

#include "C4File.h"
#include "Standard.h"

#include <cstdio>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

#include <zlib.h>

namespace StdGzCompressedFile
{
using Exception = std::runtime_error;

static constexpr uint8_t C4GroupMagic[2] = {0x1e, 0x8c};
static constexpr uint8_t GZMagic[2] = {0x1f, 0x8b};
static constexpr auto ChunkSize = 1024 * 1024;

class Read
{
	std::unique_ptr<uint8_t[]> buffer{new uint8_t[ChunkSize]};
	uint8_t *bufferPtr = nullptr;

	// the gzip struct only has size fields of unsigned int
	// and this value is bounded by ChunkSize anyway
	unsigned int bufferedSize = 0;

	C4File file;
	size_t position = 0;
	z_stream gzStream;
	bool gzStreamValid = false;

public:
	Read(const std::string &filename);
	~Read();
	size_t UncompressedSize();
	size_t ReadData(uint8_t *toBuffer, size_t size);
	void Rewind();

private:
	void CheckMagicBytes();
	void PrepareInflate();
	void RefillBuffer();
};

class Write
{
	C4File file;
	z_stream gzStream;
	std::unique_ptr<uint8_t[]> buffer{new uint8_t[ChunkSize]};
	// the gzip struct only has size fields of unsigned int
	// and this value is bounded by ChunkSize anyway
	unsigned int bufferedSize = 0;
	bool magicBytesDone = false;

public:
	Write(const std::string &filename);
	~Write() noexcept(false);
	void WriteData(const uint8_t *const fromBuffer, const size_t size);

private:
	void FlushBuffer();
	void DeflateToBuffer(const uint8_t *const fromBuffer, const size_t size, int flushMode, int expectedRet);

private:
	static constexpr auto CompressionLevel = 2;
};
}
