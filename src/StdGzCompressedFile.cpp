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

#include <StdGzCompressedFile.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <format>
#include <memory>

namespace StdGzCompressedFile
{
Read::Read(const std::string &filename)
	: file{filename, "rb"}
{
	if (!file)
	{
		throw Exception{std::format("Opening \"{}\": {}", filename, std::strerror(errno))};
	}

	gzStream.next_out = nullptr;
	gzStream.avail_out = 0;

	PrepareInflate();
}

Read::~Read()
{
	if (file)
	{
		file.Close();
	}

	if (gzStreamValid)
	{
		inflateEnd(&gzStream);
	}
}

void Read::CheckMagicBytes()
{
	uint8_t magicBytesBuffer[2];
	uint8_t *readTarget = magicBytesBuffer;
	unsigned int readLeft = 2;

	for (; readLeft > 0;)
	{
		if (bufferedSize == 0)
		{
			RefillBuffer();

			if (file.AtEnd() && bufferedSize == 0)
			{
				throw Exception("Unexpected end of file while reading the magic bytes");
			}
		}

		const auto progress = readLeft > bufferedSize ? bufferedSize : readLeft;
		std::memcpy(readTarget, bufferPtr, progress);

		readLeft -= progress;
		readTarget += progress;
		bufferPtr += progress;
		bufferedSize -= progress;
	}

	if (!std::equal(magicBytesBuffer, std::end(magicBytesBuffer), C4GroupMagic) &&
		!std::equal(magicBytesBuffer, std::end(magicBytesBuffer), GZMagic))
	{
		throw Exception("Magic bytes don't match");
	}
}

void Read::PrepareInflate()
{
	uint8_t fakeBuf;

	CheckMagicBytes();

	const auto storeNextOut = gzStream.next_out;
	const auto storeAvailOut = gzStream.avail_out;

	gzStream.zalloc = nullptr;
	gzStream.zfree = nullptr;
	gzStream.opaque = nullptr;
	gzStream.next_in = GZMagic;
	gzStream.avail_in = sizeof(GZMagic);

	gzStream.next_out = &fakeBuf;
	gzStream.avail_out = 1;

	if (const auto ret = inflateInit2(&gzStream, 15 + 16); ret != Z_OK) // window size 15 + automatic gzip
	{
		throw Exception(std::string{"inflateInit2 failed: "} + zError(ret));
	}

	if (const auto ret = inflate(&gzStream, Z_NO_FLUSH); ret != Z_OK)
	{
		throw Exception(std::string{"inflate on the fake magic failed: "} + zError(ret));
	}

	if (gzStream.avail_in > 0)
	{
		throw Exception("After inflating the fake magic, avail_in must be 0");
	}

	gzStream.next_out = storeNextOut;
	gzStream.avail_out = storeAvailOut;

	gzStream.next_in = bufferPtr;
	gzStream.avail_in = bufferedSize;

	gzStreamValid = true;
}

size_t Read::UncompressedSize()
{
	std::unique_ptr<uint8_t[]> buffer{new uint8_t[ChunkSize]};
	size_t size = 0;
	for (;;)
	{
		const auto progress = ReadData(buffer.get(), ChunkSize);
		if (progress == 0) break;

		size += progress;
	}
	return size;
}

size_t Read::ReadData(uint8_t *const toBuffer, const size_t size)
{
	size_t readSize = 0;
	gzStream.next_out = toBuffer;
	gzStream.avail_out = checked_cast<unsigned int>(size);
	for (; size > readSize;)
	{
		if (!gzStreamValid && !file.AtEnd() && bufferedSize != 0)
		{
			PrepareInflate();
		}

		if (gzStream.avail_in == 0)
		{
			if (bufferedSize == 0)
			{
				RefillBuffer();

				if (file.AtEnd() && bufferedSize == 0)
				{
					break;
				}
			}

			gzStream.next_in = bufferPtr;
			gzStream.avail_in = bufferedSize;
		}

		const auto oldAvailIn = gzStream.avail_in;
		const auto oldAvailOut = gzStream.avail_out;

		if (const auto ret = inflate(&gzStream, Z_SYNC_FLUSH); ret != Z_OK)
		{
			if (ret == Z_STREAM_END)
			{
				inflateEnd(&gzStream);
				gzStreamValid = false;
			}
			else if (ret != Z_BUF_ERROR && gzStream.avail_out != 0)
			{
				throw Exception(std::string{"inflate failed: "} + zError(ret));
			}
		}
		const auto outProgress = oldAvailOut - gzStream.avail_out;
		position += outProgress;
		readSize += outProgress;

		const auto inProgress = oldAvailIn - gzStream.avail_in;
		bufferPtr += inProgress;
		bufferedSize -= inProgress;
	}

	return readSize;
}

void Read::RefillBuffer()
{
	if (const auto result = file.Read(buffer.get(), ChunkSize))
	{
		bufferedSize = *result;
		bufferPtr = buffer.get();
	}
	else
	{
		throw Exception("fread failed");
	}
}

void Read::Rewind()
{
	position = 0;
	if (const auto result = file.Rewind(); !result)
	{
		throw Exception(std::string{"inflateInit2 failed: "} + strerror(static_cast<int>(result.error())));
	}

	inflateEnd(&gzStream);

	gzStream.next_out = nullptr;
	gzStream.avail_out = 0;
	bufferedSize = 0;
	PrepareInflate();
}

Write::Write(const std::string &filename)
	: file{filename, "wb"}
{
	if (!file)
	{
		throw Exception{std::format("Opening \"{}\": {}", filename, std::strerror(errno))};
	}

	gzStream.zalloc = nullptr;
	gzStream.zfree = nullptr;
	gzStream.opaque = nullptr;
	gzStream.next_in = nullptr;
	gzStream.avail_in = 0;
	gzStream.next_out = buffer.get();
	gzStream.avail_out = ChunkSize;

	if (const auto ret = deflateInit2(&gzStream, CompressionLevel, Z_DEFLATED, 15 + 16, 9, Z_DEFAULT_STRATEGY); ret != Z_OK)
	{
		throw Exception(std::string{"deflateInit2 failed: "} + zError(ret));
	}
}

Write::~Write() noexcept(false)
{
	if (file)
	{
		DeflateToBuffer(nullptr, 0, Z_FINISH, Z_STREAM_END);

		FlushBuffer();
		file.Close();
	}

	deflateEnd(&gzStream);
}

void Write::FlushBuffer()
{
	if (!file.WriteExact(buffer.get(), bufferedSize))
	{
		throw Exception("fwrite failed");
	}

	bufferedSize = 0;
}

void Write::DeflateToBuffer(const uint8_t *const fromBuffer, const size_t size, int flushMode, int expectedRet)
{
	gzStream.next_in = fromBuffer;
	gzStream.avail_in = checked_cast<unsigned int>(size);

	if (!magicBytesDone)
	{
		static_assert(ChunkSize >= sizeof(C4GroupMagic));
		std::copy(C4GroupMagic, std::end(C4GroupMagic), buffer.get());

		uint8_t magicDummy[2];
		gzStream.next_out = magicDummy;
		gzStream.avail_out = 2;

		if (const auto ret = deflate(&gzStream, Z_NO_FLUSH); ret != Z_OK)
		{
			throw Exception(std::string{"Deflating into the magic dummy buffer: "} + zError(ret));
		}

		magicBytesDone = true;
		gzStream.next_out = buffer.get() + 2;
		gzStream.avail_out = ChunkSize - 2;
		bufferedSize += 2;
	}

	int ret = Z_BUF_ERROR;
	while (ret == Z_BUF_ERROR || gzStream.avail_in > 0 || (ret == Z_OK && flushMode == Z_FINISH))
	{
		if (gzStream.avail_out == 0)
		{
			FlushBuffer();

			gzStream.next_out = buffer.get();
			gzStream.avail_out = ChunkSize;
		}

		const auto oldAvailOut = gzStream.avail_out;
		ret = deflate(&gzStream, flushMode);
		bufferedSize += oldAvailOut - gzStream.avail_out;

		if (ret == Z_STREAM_ERROR)
		{
			break;
		}
	}

	if (ret == Z_STREAM_ERROR || ret != expectedRet)
	{
		throw Exception(std::string{"Deflating the data to write: "} + zError(ret));
	}
}

void Write::WriteData(const uint8_t *const fromBuffer, const size_t size)
{
	DeflateToBuffer(fromBuffer, size, Z_NO_FLUSH, Z_OK);
}
}
