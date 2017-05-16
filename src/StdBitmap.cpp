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

#include <Standard.h>
#include <StdBitmap.h>

StdBitmap::StdBitmap(const std::uint32_t width, const std::uint32_t height, const bool useAlpha)
	: width(width), height(height), useAlpha(useAlpha),
	bytes(new uint8_t[width * height * (useAlpha ? 4 : 3)]) {}

const void *StdBitmap::GetBytes() const
{
	return bytes.get();
}

void *StdBitmap::GetBytes()
{
	return const_cast<void *>(const_cast<const StdBitmap &>(*this).GetBytes());
}

const void *StdBitmap::GetPixelAddr(const std::uint32_t x, const std::uint32_t y) const
{
	return useAlpha ? GetPixelAddr32(x, y) : GetPixelAddr24(x, y);
}

void *StdBitmap::GetPixelAddr(const std::uint32_t x, const std::uint32_t y)
{
	return const_cast<void *>(const_cast<const StdBitmap &>(*this).GetPixelAddr(x, y));
}

const void *StdBitmap::GetPixelAddr24(const std::uint32_t x, const std::uint32_t y) const
{
	return &bytes[(y * width + x) * 3];
}

void *StdBitmap::GetPixelAddr24(const std::uint32_t x, const std::uint32_t y)
{
	return const_cast<void *>(const_cast<const StdBitmap &>(*this).GetPixelAddr24(x, y));
}

const void *StdBitmap::GetPixelAddr32(const std::uint32_t x, const std::uint32_t y) const
{
	return &bytes[(y * width + x) * 4];
}

void *StdBitmap::GetPixelAddr32(const std::uint32_t x, const std::uint32_t y)
{
	return const_cast<void *>(const_cast<const StdBitmap &>(*this).GetPixelAddr32(x, y));
}

std::uint32_t StdBitmap::GetPixel(const std::uint32_t x, const std::uint32_t y) const
{
	return useAlpha ? GetPixel32(x, y) : GetPixel24(x, y);
}

std::uint32_t StdBitmap::GetPixel24(const std::uint32_t x, const std::uint32_t y) const
{
	const auto pixel = static_cast<const std::uint8_t *>(GetPixelAddr24(x, y));
	return RGB(pixel[0], pixel[1], pixel[2]);
}

std::uint32_t StdBitmap::GetPixel32(const std::uint32_t x, const std::uint32_t y) const
{
	return *static_cast<const std::uint32_t *>(GetPixelAddr32(x, y));
}

void StdBitmap::SetPixel(const std::uint32_t x, const std::uint32_t y, const std::uint32_t value)
{
	return useAlpha ? SetPixel32(x, y, value) : SetPixel24(x, y, value);
}

void StdBitmap::SetPixel24(const std::uint32_t x, const std::uint32_t y, const std::uint32_t value)
{
	const auto pixel = static_cast<std::uint8_t *>(GetPixelAddr24(x, y));
	pixel[0] = GetRValue(value);
	pixel[1] = GetGValue(value);
	pixel[2] = GetBValue(value);
}

void StdBitmap::SetPixel32(const std::uint32_t x, const std::uint32_t y, const std::uint32_t value)
{
	*static_cast<std::uint32_t *>(GetPixelAddr32(x, y)) = value;
}
