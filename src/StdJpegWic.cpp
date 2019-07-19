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

// JPEG decoding using Windows Imaging Component

#include <Standard.h>
#include <StdJpeg.h>
#include <StdWic.h>

#include <cassert>
#include <cstdint>

struct StdJpeg::Impl
{
	StdWic wic;
	std::uint32_t width, height, currentRow;
	std::size_t rowSize;
	std::unique_ptr<uint8_t[]> rowBuffer;

	Impl(const void *const fileContents, const std::size_t fileSize)
		: wic(true, fileContents, fileSize), currentRow(0)
	{
		wic.PrepareDecode(GUID_ContainerFormatJpeg);
		// Store dimensions
		UINT width, height;
		wic.GetSize(&width, &height);
		this->width = width; this->height = height;

		// Convert to 24bpp RGB if necessary
		wic.SetOutputFormat(GUID_WICPixelFormat24bppRGB);

		rowSize = width * 3;
		rowBuffer.reset(new uint8_t[rowSize]);
	}

	// Reads one row of jpeg data
	const void *DecodeRow()
	{
		assert(currentRow < height);
		const WICRect rect = { 0, static_cast<INT>(currentRow++), static_cast<INT>(width), 1 };
		wic.CopyPixels(&rect, rowSize, rowSize, rowBuffer.get());
		return rowBuffer.get();
	}
};

StdJpeg::StdJpeg(const void *const fileContents, const std::size_t fileSize)
	: impl(new Impl(fileContents, fileSize)) {};

StdJpeg::~StdJpeg() = default;

const void *StdJpeg::DecodeRow() { return impl->DecodeRow(); }

void StdJpeg::Finish() {}

std::uint32_t StdJpeg::Width()  const { return impl->width; }
std::uint32_t StdJpeg::Height() const { return impl->height; }
