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

class C4File
{
#ifndef _WIN32
	static_assert(sizeof(off_t) == sizeof(std::int64_t), "64-bit off_t required");
#endif

	enum class SeekMode
	{
		Start = SEEK_SET,
		Current = SEEK_CUR,
		End = SEEK_END
	};

public:
	explicit C4File() = default;

	C4File(const char *const filename, const char *const mode)
		: file{std::fopen(filename, mode)}
	{
	}

	C4File(const std::string &filename, const char *const mode)
		: C4File{filename.c_str(), mode}
	{
	}

public:
	bool Open(const char *const filename, const char *const mode)
	{
		file.reset(std::fopen(filename, mode));
		return file.get();
	}

	bool Open(const std::string &filename, const char *const mode)
	{
		return Open(filename.c_str(), mode);
	}

	template<typename T>
	bool ReadElement(T &ptr)
	{
		return std::fread(&ptr, sizeof(T), 1, file.get()) == 1;
	}

	template<typename T, std::size_t Extent>
	bool ReadElements(const std::span<T, Extent> buffer)
	{
		return std::fread(buffer.data(), sizeof(T), buffer.size(), file.get()) == buffer.size();
	}

	bool Read(const std::span<std::byte> buffer)
	{
		return std::fread(buffer.data(), 1, buffer.size(), file.get()) == buffer.size();
	}

	bool ReadRaw(void *const buffer, const std::size_t size)
	{
		return std::fread(buffer, 1, size, file.get()) == size;
	}

	std::pair<bool, std::size_t> ReadPartial(const std::span<std::byte> buffer)
	{
		const std::size_t result{std::fread(buffer.data(), 1, buffer.size(), file.get())};
		return {result != -1, result};
	}

	std::pair<bool, std::size_t> ReadPartialRaw(void *const buffer, const std::size_t size)
	{
		const std::size_t result{std::fread(buffer, 1, size, file.get())};
		return {result != -1, result};
	}

	template<typename T>
	bool WriteElement(T &&ptr)
	{
		return std::fwrite(&ptr, sizeof(T), 1, file.get()) == 1;
	}

	template<typename T, std::size_t Extent>
	bool WriteElements(const std::span<T, Extent> buffer)
	{
		return std::fwrite(buffer.data(), sizeof(T), buffer.size(), file.get()) == buffer.size();
	}

	bool Write(const std::span<const std::byte> buffer)
	{
		return std::fwrite(buffer.data(), 1, buffer.size(), file.get()) == buffer.size();
	}

	bool WriteRaw(const void *const buffer, const std::size_t size)
	{
		return std::fwrite(buffer, 1, size, file.get()) == size;
	}

	template<typename... Args>
	bool WriteString(const std::format_string<Args...> fmt, Args &&...args)
	{
		return WriteString(std::format(fmt, std::forward<Args>(args)...));
	}

	bool WriteString(const std::string_view value)
	{
		return Write(std::as_bytes(std::span{value.data(), value.size()}));
	}

	template<typename... Args>
	bool WriteStringLine(const std::format_string<Args...> fmt, Args &&...args)
	{
		return WriteString(std::format("{}\n", std::format(fmt, std::forward<Args>(args)...)));
	}

	bool WriteStringLine(const std::string_view value)
	{
		return WriteString(std::format("{}\n", value));
	}

	bool Seek(const long offset, const SeekMode mode);

	std::int64_t Tell() const;

	void Flush()
	{
		std::fflush(file.get());
	}

	void Close()
	{
		file.reset();
	}

	explicit operator bool() const { return !!file; }

public:
	static std::tuple<bool, std::unique_ptr<std::byte[]>, std::size_t> LoadContents(const char *filename);

private:
	std::unique_ptr<FILE, decltype([](FILE *const file) { std::fclose(file); })> file;
};

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
