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

bool C4File::Seek(const std::int64_t offset, const SeekMode mode)
{
#ifdef _WIN32
	return _fseeki64(file.get(), offset, std::to_underlying(mode)) == 0;
#else
	return fseeko(file.get(), offset, std::to_underlying(mode)) == 0;
#endif
}

std::int64_t C4File::Tell() const
{
#ifdef _WIN32
	return _ftelli64(file.get());
#else
	return ftello(file.get());
#endif
}

std::tuple<bool, std::unique_ptr<std::byte[]>, std::size_t> C4File::LoadContents(const char *const filename)
{
	C4File file{filename, "rb"};
	if (!file)
	{
		return {};
	}

	if (!file.Seek(0, C4File::SeekMode::End))
	{
		return {};
	}

	const auto size = file.Tell();

	if (size == -1)
	{
		return {};
	}
	else if (size == 0)
	{
		return {true, nullptr, 0};
	}

	if (!file.Seek(0, C4File::SeekMode::Start))
	{
		return {};
	}

	auto buffer = std::make_unique_for_overwrite<std::byte[]>(static_cast<std::size_t>(size));

	if (!file.ReadExact(std::span{buffer.get(), static_cast<std::size_t>(size)}))
	{
		return {};
	}

	return {true, std::move(buffer), static_cast<std::size_t>(size)};
}
