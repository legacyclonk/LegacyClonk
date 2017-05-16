/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
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
#include <string>

// Reads and writes PNG files
class CPNGFile
{
public:
	// Creates an object that can be used to write to the specified file.
	CPNGFile(const std::string &filename, std::uint32_t width, std::uint32_t height, bool useAlpha);
	// Writes the specified image to the PNG file. Don't use this object after calling.
	void Encode(const void *pixels);

	// Creates an object that can be used to read the specified file contents.
	CPNGFile(const void *fileContents, std::size_t fileSize);
	// Reads the PNG file into the specified buffer. Don't use this object after calling.
	void Decode(void *pixels);

	~CPNGFile();

	std::uint32_t Width() const;
	std::uint32_t Height() const;
	bool UsesAlpha() const;

	// Use Pimpl so we don't have to include png.h in the header
private:
	struct Impl;
	const std::unique_ptr<Impl> impl;
};
