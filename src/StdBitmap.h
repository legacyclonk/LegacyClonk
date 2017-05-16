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

#include <cstdint>
#include <memory>

// A B8G8R8 or B8G8R8A8 bitmap.
class StdBitmap
{
public:
	// Creates a B8G8R8 bitmap if useAlpha is false or an B8G8R8A8 bitmap otherwise.
	StdBitmap(std::uint32_t width, std::uint32_t height, bool useAlpha);

	// Returns a pointer to the bitmap bytes.
	const void *GetBytes() const;
	void *GetBytes();

	// Returns a pointer to the pixel at the specified position.
	const void *GetPixelAddr(std::uint32_t x, std::uint32_t y) const;
	void *GetPixelAddr(std::uint32_t x, std::uint32_t y);
	// Returns a pointer to the pixel at the specified position, assuming the bitmap is in B8G8R8 format.
	const void *GetPixelAddr24(std::uint32_t x, std::uint32_t y) const;
	void *GetPixelAddr24(std::uint32_t x, std::uint32_t y);
	// Returns a pointer to the pixel in the bitmap bytes at the specified position, assuming the bitmap is in B8G8R8A8 format.
	const void *GetPixelAddr32(std::uint32_t x, std::uint32_t y) const;
	void *GetPixelAddr32(std::uint32_t x, std::uint32_t y);

	// Returns the color of the pixel at the specified position.
	std::uint32_t GetPixel(std::uint32_t x, std::uint32_t y) const;
	// Returns the color of the pixel at the specified position, assuming the bitmap is in B8G8R8 format.
	std::uint32_t GetPixel24(std::uint32_t x, std::uint32_t y) const;
	// Returns the color of the pixel at the specified position, assuming the bitmap is in B8G8R8A8 format.
	std::uint32_t GetPixel32(std::uint32_t x, std::uint32_t y) const;

	// Sets the color of the pixel at the specified position.
	void SetPixel(std::uint32_t x, std::uint32_t y, std::uint32_t value);
	// Sets the color of the pixel at the specified position, assuming the bitmap is in B8G8R8 format.
	void SetPixel24(std::uint32_t x, std::uint32_t y, std::uint32_t value);
	// Sets the color of the pixel at the specified position, assuming the bitmap is in B8G8R8A8 format.
	void SetPixel32(std::uint32_t x, std::uint32_t y, std::uint32_t value);

private:
	std::uint32_t width, height;
	bool useAlpha;
	const std::unique_ptr<std::uint8_t[]> bytes;
};
