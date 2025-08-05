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

#include "C4File.h"

C4File::C4File(const char *const filename, const char *const mode)
{
	(void) Open(filename, mode);
}

C4File::C4File(const std::string &filename, const char *const mode)
{
	(void) Open(filename, mode);
}

C4File::C4File(FILE *const file) : file{file}
{
}

static std::unexpected<std::error_code> GetErrorUnexpected(const int error) noexcept
{
	return std::unexpected{std::make_error_code(static_cast<std::errc>(error))};
}

static std::unexpected<std::error_code> GetErrnoUnexpected() noexcept
{
	return GetErrorUnexpected(errno);
}

std::expected<void, std::error_code> C4File::Open(const char *const filename, const char *const mode)
{
	file.reset(std::fopen(filename, mode));
	if (file)
	{
		return {};
	}
	else
	{
		return GetErrnoUnexpected();
	}
}

std::expected<void, std::error_code> C4File::Open(const std::string &filename, const char *const mode)
{
	return Open(filename.c_str(), mode);
}

std::optional<std::size_t> C4File::Read(void *const buffer, const std::size_t size)
{
	return ReadInternal(buffer, 1, size);
}

std::optional<std::size_t> C4File::Write(const void *const buffer, const std::size_t size)
{
	return WriteInternal(buffer, 1, size);
}

std::expected<void, std::error_code> C4File::Seek(const std::int64_t offset, const SeekMode mode)
{
#ifdef _WIN32
	const auto result = _fseeki64(file.get(), offset, std::to_underlying(mode)) == 0;
#else
	const auto result = fseeko(file.get(), offset, std::to_underlying(mode)) == 0;
#endif
	if (result == 0)
	{
		return {};
	}
	else
	{
		return GetErrnoUnexpected();
	}
}

std::expected<std::uint64_t, std::error_code> C4File::Tell() const
{
#ifdef _WIN32
	const auto result = _ftelli64(file.get());
#else
	const auto result = ftello(file.get());
#endif

	if (result != -1)
	{
		return {static_cast<std::uint64_t>(result)};
	}
	else
	{
		return GetErrnoUnexpected();
	}
}

std::expected<void, std::error_code> C4File::Flush()
{
	if (std::fflush(file.get()) != EOF)
	{
		return {};
	}
	else
	{
		return GetErrnoUnexpected();
	}
}

std::expected<void, std::error_code> C4File::Rewind()
{
	std::rewind(file.get());

	if (const int error{errno}; error == 0)
	{
		return {};
	}
	else
	{
		return GetErrorUnexpected(error);
	}
}

bool C4File::AtEnd() const
{
	return std::feof(file.get());
}

std::optional<std::error_code> C4File::GetError() const
{
	return std::ferror(file.get()) ? std::optional{std::make_error_code(static_cast<std::errc>(errno))} : std::nullopt;
}

void C4File::Close()
{
	file.reset();
}

std::optional<std::size_t> C4File::ReadInternal(void *const buffer, const std::size_t elementSize, const std::size_t count)
{
	const std::size_t result{std::fread(buffer, elementSize, count, file.get())};
	return result != -1 ? std::optional{result} : std::nullopt;
}

std::optional<std::size_t> C4File::WriteInternal(const void *const buffer, const std::size_t elementSize, const std::size_t count)
{
	const std::size_t result{std::fwrite(buffer, elementSize, count, file.get())};
	return result != -1 ? std::optional{result} : std::nullopt;
}

std::expected<std::pair<std::unique_ptr<std::byte[]>, std::size_t>, std::error_code> C4File::LoadContents(const char *const filename)
{
	C4File file{filename, "rb"};
	if (!file)
	{
		return GetErrnoUnexpected();
	}

	if (const auto result = file.Seek(0, C4File::SeekMode::End); !result)
	{
		return std::unexpected{result.error()};
	}

	const auto size = file.Tell();

	if (!size)
	{
		return std::unexpected{size.error()};
	}
	else if (*size == 0)
	{
		return {std::pair{nullptr, 0}};
	}

	if (const auto result = file.Seek(0, C4File::SeekMode::Start); !result)
	{
		return std::unexpected{result.error()};
	}

	auto buffer = std::make_unique_for_overwrite<std::byte[]>(static_cast<std::size_t>(*size));

	if (!file.ReadExact(buffer.get(), *size))
	{
		return {};
	}

	return {std::pair{std::move(buffer), static_cast<std::size_t>(*size)}};
}
