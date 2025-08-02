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
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

class C4FileBase
{
public:
	enum class SeekMode
	{
		Start = SEEK_SET,
		Current = SEEK_CUR,
		End = SEEK_END
	};

public:
	bool ReadExact(this auto &&self, const std::span<std::byte> buffer)
	{
		return self.ReadExact(buffer.data(), buffer.size_bytes());
	}

	std::pair<bool, std::size_t> Read(this auto &&self, const std::span<std::byte> buffer)
	{
		return self.Read(buffer.data(), buffer.size_bytes());
	}

	std::pair<bool, std::size_t> ReadAt(this auto &&self, const std::span<std::byte> buffer, const std::size_t offset)
	{
		return self.ReadAt(buffer.data(), buffer.size_bytes(), offset);
	}

	std::pair<bool, std::size_t> Write(this auto &&self, const std::span<const std::byte> buffer)
	{
		return self.Write(buffer.data(), buffer.size_bytes());
	}

	bool WriteExact(this auto &&self, const std::span<const std::byte> buffer)
	{
		return self.WriteExact(buffer.data(), buffer.size_bytes());
	}

	bool WriteExactAt(this auto &&self, const std::span<const std::byte> buffer, const std::size_t offset)
	{
		return self.WriteExactAt(buffer.data(), buffer.size_bytes(), offset);
	}

	template<typename... Args> requires (sizeof...(Args) > 0)
	bool WriteString(this auto &&self, const std::format_string<Args...> fmt, Args &&...args)
	{
		return self.WriteString(std::format(fmt, std::forward<Args>(args)...));
	}

	bool WriteString(this auto &&self, const std::string_view value)
	{
		return self.WriteExact(value.data(), value.size());
	}

	template<typename... Args> requires (sizeof...(Args) > 0)
	bool WriteStringLine(this auto &&self, const std::format_string<Args...> fmt, Args &&...args)
	{
		return self.WriteString(std::format("{}\n", std::format(fmt, std::forward<Args>(args)...)));
	}

	bool WriteStringLine(this auto &&self, const std::string_view value)
	{
		return self.WriteString(std::format("{}\n", value));
	}
};

class C4File : public C4FileBase
{
#ifndef _WIN32
	static_assert(sizeof(off_t) == sizeof(std::int64_t), "64-bit off_t required");
#endif

public:
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

	explicit C4File(FILE *const file)
		: file{file}
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

	bool ReadExact(void *const buffer, const std::size_t size)
	{
		return std::fread(buffer, 1, size, file.get()) == size;
	}

	std::pair<bool, std::size_t> Read(void *const buffer, const std::size_t size)
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

	std::pair<bool, std::size_t> Write(const void *const buffer, const std::size_t size)
	{
		const std::size_t result{std::fwrite(buffer, 1, size, file.get())};
		return {result != -1, result};
	}

	bool WriteExact(const void *const buffer, const std::size_t size)
	{
		return std::fwrite(buffer, 1, size, file.get()) == size;
	}

	bool Seek(const long offset, const SeekMode mode);

	std::int64_t Tell() const;

	void Flush()
	{
		std::fflush(file.get());
	}

	bool Rewind()
	{
		std::rewind(file.get());
		return errno != 0;
	}

	bool AtEnd() const
	{
		return std::feof(file.get());
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
