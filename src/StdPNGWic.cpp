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

// PNG encoding/decoding using Windows Imaging Component

#include <Standard.h>
#include <StdPNG.h>
#include <StdWic.h>

struct CPNGFile::Impl
{
	StdWic wic;

	std::uint32_t width, height, rowSize;
	bool useAlpha;

	// Creates an object that can be used to write to the specified file.
	Impl(const std::string &filename,
		const std::uint32_t width, const std::uint32_t height, const bool useAlpha)
		: width(width), height(height), useAlpha(useAlpha),
		rowSize(width * (useAlpha ? 4 : 3)),
		wic(false, filename)
	{
		wic.PrepareEncode(width, height, GUID_ContainerFormatPng,
			(useAlpha ? GUID_WICPixelFormat32bppBGRA : GUID_WICPixelFormat24bppBGR));
	}

	// Writes the specified image to the PNG file. Don't use this object after calling.
	void Encode(const void *const pixels)
	{
		wic.Encode(height, rowSize, height * rowSize, pixels);
	}

	// Creates an object that can be used to read the specified file contents.
	Impl(const void *const fileContents, const std::size_t fileSize)
		: wic(true, fileContents, fileSize)
	{
		wic.PrepareDecode(GUID_ContainerFormatPng);
		// Store dimensions
		UINT width, height;
		wic.GetSize(&width, &height);
		this->width = width; this->height = height;

		// Convert any other format than 24bpp BGR or 32bpp BGRA to 32bpp BGRA
		useAlpha = (wic.GetOutputFormat() != GUID_WICPixelFormat24bppBGR);
		if (useAlpha) wic.SetOutputFormat(GUID_WICPixelFormat32bppBGRA);

		rowSize = width * (useAlpha ? 4 : 3);
	}

	// Reads the PNG file into the specified buffer. Don't use this object after calling.
	void Decode(void *const pixels)
	{
		wic.CopyPixels(nullptr, rowSize, height * rowSize, pixels);

		// Invert alpha channel
		if (useAlpha)
		{
			for (std::uint32_t y = 0; y < height; ++y)
			{
				for (std::uint32_t x = 0; x < width; ++x)
				{
					auto &alpha = static_cast<std::uint8_t *>(pixels)[(y * width + x) * 4 + 3];
					alpha = 255 - alpha;
				}
			}
		}
	}
};

CPNGFile::CPNGFile(const std::string &filename,
	const std::uint32_t width, const std::uint32_t height, const bool useAlpha)
	: impl(new Impl(filename, width, height, useAlpha)) {}

void CPNGFile::Encode(const void *const pixels) { impl->Encode(pixels); }

CPNGFile::CPNGFile(const void *const fileContents, const std::size_t fileSize)
	: impl(new Impl(fileContents, fileSize)) {}

void CPNGFile::Decode(void *const pixels) { impl->Decode(pixels); }

CPNGFile::~CPNGFile() = default;

std::uint32_t CPNGFile::Width()  const { return impl->width; }
std::uint32_t CPNGFile::Height() const { return impl->height; }
bool CPNGFile::UsesAlpha()       const { return impl->useAlpha; }
