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
#include <expected>
#include <filesystem>
#include <format>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
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
	[[nodiscard]] bool ReadExact(this auto &&self, const std::span<std::byte> buffer)
	{
		return self.ReadExact(buffer.data(), buffer.size_bytes());
	}

	[[nodiscard]] bool ReadExact(this auto &&self, void *const buffer, const std::size_t size)
	{
		const auto read = self.Read(buffer, size);
		return read && *read == size;
	}

	[[nodiscard]] std::optional<std::size_t> Read(this auto &&self, const std::span<std::byte> buffer)
	{
		return self.Read(buffer.data(), buffer.size_bytes());
	}

	[[nodiscard]] std::optional<std::size_t> Write(this auto &&self, const std::span<const std::byte> buffer)
	{
		return self.Write(buffer.data(), buffer.size_bytes());
	}

	[[nodiscard]] bool WriteExact(this auto &&self, const std::span<const std::byte> buffer)
	{
		return self.WriteExact(buffer.data(), buffer.size_bytes());
	}

	[[nodiscard]] bool WriteExact(this auto &&self, const void *const buffer, const std::size_t size)
	{
		const auto written = self.Write(buffer, size);
		return written && *written == size;
	}

	template<typename... Args> requires (sizeof...(Args) > 0)
	[[nodiscard]] bool WriteString(this auto &&self, const std::format_string<Args...> fmt, Args &&...args)
	{
		return self.WriteString(std::format(fmt, std::forward<Args>(args)...));
	}

	[[nodiscard]] bool WriteString(this auto &&self, const std::string_view value)
	{
		return self.WriteExact(value.data(), value.size());
	}

	template<typename... Args> requires (sizeof...(Args) > 0)
	[[nodiscard]] bool WriteStringLine(this auto &&self, const std::format_string<Args...> fmt, Args &&...args)
	{
		return self.WriteString(std::format("{}\n", std::format(fmt, std::forward<Args>(args)...)));
	}

	[[nodiscard]] bool WriteStringLine(this auto &&self, const std::string_view value)
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
	C4File(const char *filename, const char *mode);
	C4File(const std::filesystem::path &filename, const char *mode);
	explicit C4File(FILE *file);

public:
	[[nodiscard]] std::expected<void, std::error_code> Open(const char *filename, const char *mode);
	[[nodiscard]] std::expected<void, std::error_code> Open(const std::filesystem::path &filename, const char *mode);

	template<typename T>
	[[nodiscard]] bool ReadElement(T &ptr)
	{
		return ReadElements(std::span<T, 1>{&ptr, 1});
	}

	template<typename T, std::size_t Extent>
	[[nodiscard]] bool ReadElements(const std::span<T, Extent> buffer)
	{
		const auto size = ReadInternal(buffer.data(), sizeof(T), buffer.size());
		return size && *size == buffer.size();
	}

	template<typename T>
	[[nodiscard]] bool WriteElement(T &&ptr)
	{
		return WriteElements(std::span<std::remove_reference_t<T>, 1>{&ptr, 1});
	}

	template<typename T, std::size_t Extent>
	[[nodiscard]] bool WriteElements(const std::span<T, Extent> buffer)
	{
		const auto written = WriteInternal(buffer.data(), sizeof(T), buffer.size());
		return written && written == buffer.size();
	}

	[[nodiscard]] std::optional<std::size_t> Read(void *buffer, std::size_t size);
	[[nodiscard]] std::optional<std::size_t> Write(const void *buffer, std::size_t size);

	[[nodiscard]] std::expected<void, std::error_code> Seek(std::int64_t offset, SeekMode mode);
	[[nodiscard]] std::expected<std::uint64_t, std::error_code> Tell() const;
	[[nodiscard]] std::expected<void, std::error_code> Flush();
	[[nodiscard]] std::expected<void, std::error_code> Rewind();
	[[nodiscard]] bool AtEnd() const;
	[[nodiscard]] std::optional<std::error_code> GetError() const;
	void Close();

	FILE *GetHandle() const { return file.get(); }
	explicit operator bool() const { return GetHandle(); }

private:
	std::optional<std::size_t> ReadInternal(void *buffer, std::size_t elementSize, std::size_t count);
	std::optional<std::size_t> WriteInternal(const void *buffer, std::size_t elementSize, std::size_t count);

public:
	static std::expected<std::pair<std::unique_ptr<std::byte[]>, std::size_t>, std::error_code> LoadContents(const char *filename);

private:
	std::unique_ptr<FILE, decltype([](FILE *const file) { std::fclose(file); })> file;
};

