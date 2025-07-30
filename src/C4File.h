/*
 * LegacyClonk
 *
 * Copyright (c) 2025, The LegacyClonk Team and contributors
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

#pragma once

#include <cstdio>
#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

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

	void Rewind()
	{
		std::rewind(file.get());
	}

	void Close()
	{
		file.reset();
	}

	FILE *GetHandle() const noexcept { return file.get(); }

	explicit operator bool() const { return !!file; }

public:
	static std::tuple<bool, std::unique_ptr<std::byte[]>, std::size_t> LoadContents(const char *filename);

private:
	std::unique_ptr<FILE, decltype([](FILE *const file) { std::fclose(file); })> file;
};
