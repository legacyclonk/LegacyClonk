/*
 * LegacyClonk
 *
 * Copyright (c) 2017, The LegacyClonk Team and contributors
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

#include <cstddef>
#include <cstdint>
#include <memory>

// JPEG decoding
class StdJpeg
{
public:
	StdJpeg(const void *fileContents, std::size_t fileSize);
	~StdJpeg();
	// Decodes one row of JPEG data
	const void *DecodeRow();
	// Call after reading the last row
	void Finish();

	std::uint32_t Width() const;
	std::uint32_t Height() const;

private:
	struct Impl;
	const std::unique_ptr<Impl> impl;
};
