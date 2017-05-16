/*
 * LegacyClonk
 *
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

// PNG encoding/decoding using libpng

#include <Standard.h>
#include <StdPNG.h>

#include <png.h>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>

struct CPNGFile::Impl
{
	// true if this instance is used for writing a PNG file or false if it is used for reading
	bool writeMode;
	// Pointer to the output file if this instance is used for writing
	FILE *outputFile;
	// Current read position in the input file contents
	const png_byte *inputFileContents;
	std::size_t inputFilePos, inputFileSize;

	std::uint32_t width, height, rowSize;
	bool useAlpha;

	// libpng structs
	png_structp png_ptr;
	png_infop info_ptr;

	// Initializes attributes to zero
	Impl() :
		outputFile(nullptr), inputFileContents(nullptr),
		png_ptr(nullptr), info_ptr(nullptr) {}

	~Impl()
	{
		ClearNoExcept();
	}

	// Frees resources. Can be called from destructor or unfinished constructor.
	void Clear()
	{
		// Clear internal png ptrs
		if (png_ptr || info_ptr)
		{
			if (writeMode)
			{
				png_destroy_write_struct(&png_ptr, &info_ptr);
				png_ptr = nullptr; info_ptr = nullptr;
			}
			else
			{
				png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
				png_ptr = nullptr; info_ptr = nullptr;
			}
		}
		// Close file if open
		if (outputFile)
		{
			const auto result = fclose(outputFile);
			outputFile = nullptr;
			if (result != 0) throw std::runtime_error("fclose failed");
		}
	}

	void ClearNoExcept() noexcept
	{
		try
		{
			Clear();
		}
		catch (const std::runtime_error &)
		{
		}
	}

	// Initialize for encoding
	Impl(const std::string &filename,
		const unsigned int width, const unsigned int height, const bool useAlpha)
		: Impl()
	{
		try
		{
			PrepareEncoding(filename, width, height, useAlpha);
		}
		catch (const std::runtime_error &)
		{
			ClearNoExcept();
			throw;
		}
	}

	// Prepares writing to the specified file
	void PrepareEncoding(const std::string &filename,
		const unsigned int width, const unsigned int height, const bool useAlpha)
	{
		// open the file
		outputFile = fopen(filename.c_str(), "wb");
		if (!outputFile) throw std::runtime_error("fopen failed");
		// init png-structs for writing
		writeMode = true;
		png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, this, ErrorCallbackFn, WarningCallbackFn);
		if (!png_ptr) throw std::runtime_error("png_create_write_struct failed");
		info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr) throw std::runtime_error("png_create_info_struct failed");
		// io initialization
		png_init_io(png_ptr, outputFile);
		// set header
		png_set_IHDR(png_ptr, info_ptr, width, height, 8,
			(useAlpha ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB),
			PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		// Store image properties
		this->width = width;
		this->height = height;
		this->useAlpha = useAlpha;
		CalculateRowSize();
		// Clonk expects the alpha channel to be inverted
		png_set_invert_alpha(png_ptr);
		// Write header
		png_write_info(png_ptr, info_ptr);
		// image data is given as bgr...
		png_set_bgr(png_ptr);
	}

	void Encode(const void *const pixels)
	{
		// Write the whole image
		png_write_image(png_ptr, CreateRowBuffer(pixels).get());
		// Write footer
		png_write_end(png_ptr, info_ptr);
		// Close the file
		Clear();
	}

	// Initialize for decoding
	Impl(const void *const fileContents, const std::size_t fileSize)
		: Impl()
	{
		try
		{
			PrepareDecoding(fileContents, fileSize);
		}
		catch (const std::runtime_error &)
		{
			ClearNoExcept();
			throw;
		}
	}

	// Prepares reading from the specified file contents
	void PrepareDecoding(const void *const fileContents, const std::size_t fileSize)
	{
		// store file ptr
		inputFileContents = static_cast<const png_byte *>(fileContents);
		inputFilePos = 0;
		inputFileSize = fileSize;
		// check file
		if (fileSize < 8 || png_sig_cmp(const_cast<png_bytep>(inputFileContents), 0, 8) != 0)
		{
			throw std::runtime_error("File is not a PNG file");
		}
		// setup png for reading
		writeMode = false;
		png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, this, ErrorCallbackFn, WarningCallbackFn);
		if (!png_ptr) throw std::runtime_error("png_create_read_struct failed");
		info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr) throw std::runtime_error("png_create_info_struct failed");
		// set file-reading proc
		png_set_read_fn(png_ptr, this, &ReadCallbackFn);
		// Clonk expects the alpha channel to be inverted
		png_set_invert_alpha(png_ptr);
		// read info
		png_read_info(png_ptr, info_ptr);
		// assign local vars
		int bitsPerChannel, colorType;
		png_get_IHDR(png_ptr, info_ptr, nullptr, nullptr, &bitsPerChannel, &colorType,
			nullptr, nullptr, nullptr);
		// convert to bgra
		if (colorType == PNG_COLOR_TYPE_PALETTE && bitsPerChannel <= 8 ||
			colorType == PNG_COLOR_TYPE_GRAY && bitsPerChannel < 8)
		{
			png_set_expand(png_ptr);
		}
		if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		{
			png_set_expand(png_ptr);
		}
		if (bitsPerChannel == 16)
		{
			png_set_strip_16(png_ptr);
		}
		else if (bitsPerChannel < 8)
		{
			png_set_packing(png_ptr);
		}
		if (colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
		{
			png_set_gray_to_rgb(png_ptr);
		}
		png_set_bgr(png_ptr);
		// update info
		png_read_update_info(png_ptr, info_ptr);
		png_uint_32 ihdrWidth, ihdrHeight;
		png_get_IHDR(png_ptr, info_ptr, &ihdrWidth, &ihdrHeight, &bitsPerChannel, &colorType,
			nullptr, nullptr, nullptr);
		// check format
		if (colorType != PNG_COLOR_TYPE_RGB && colorType != PNG_COLOR_TYPE_RGB_ALPHA)
		{
			throw std::runtime_error("Unsupported color type");
		}
		// Store image properties
		width = ihdrWidth;
		height = ihdrHeight;
		useAlpha = (colorType == PNG_COLOR_TYPE_RGB_ALPHA);
		CalculateRowSize();
	}

	void Decode(void *const pixels)
	{
		png_read_image(png_ptr, CreateRowBuffer(pixels).get());
		Clear();
	}

	// Calculates the row size and assures that it is the same as reported by libpng.
	void CalculateRowSize()
	{
		const auto expectedRowSize = (useAlpha ? 4 : 3) * width;
		rowSize = png_get_rowbytes(png_ptr, info_ptr);
		if (expectedRowSize != rowSize) throw std::runtime_error("libpng uses unexpected row size");
	}

	// Creates the row pointer array that is needed by libpng for reading and writing PNG files
	std::unique_ptr<png_bytep[]> CreateRowBuffer(const void *const pixels)
	{
		std::unique_ptr<png_bytep[]> rowBuf(new png_bytep[height]);
		uint32_t rowIndex = 0;
		std::generate_n(rowBuf.get(), height,
			[&] { return static_cast<png_bytep>(const_cast<void *>(pixels)) + rowIndex++ * rowSize; });
		return rowBuf;
	}

	// Called by libpng when more input is needed to decode a file
	static void PNGAPI ReadCallbackFn(const png_structp png_ptr,
		const png_bytep data, const png_size_t length)
	{
		const auto pngFile = static_cast<Impl *>(png_get_io_ptr(png_ptr));
		// Do not try to read beyond end of file
		const std::size_t newPos = pngFile->inputFilePos + length;
		if (newPos < pngFile->inputFilePos || newPos > pngFile->inputFileSize)
		{
			png_error(png_ptr, "Cannot read beyond end of file");
		}
		// Copy bytes and update position
		std::copy_n(pngFile->inputFileContents + pngFile->inputFilePos, length, data);
		pngFile->inputFilePos = newPos;
	}

	// Error callback for libpng
	static void PNGAPI ErrorCallbackFn(const png_structp png_ptr, const png_const_charp msg)
	{
		throw std::runtime_error(std::string() + "libpng error: " + msg);
	}

	// Warning callback for libpng
	static void PNGAPI WarningCallbackFn(const png_structp png_ptr, const png_const_charp msg)
	{
		// Ignore
	}
};

// Initialize for encoding
CPNGFile::CPNGFile(const std::string &filename,
	const std::uint32_t width, const std::uint32_t height, const bool useAlpha)
	: impl(new Impl(filename, width, height, useAlpha)) {}

void CPNGFile::Encode(const void *pixels) { impl->Encode(pixels); }

// Initialize for decoding
CPNGFile::CPNGFile(const void *const fileContents, const std::size_t fileSize)
	: impl(new Impl(fileContents, fileSize)) {}

void CPNGFile::Decode(void *const pixels) { impl->Decode(pixels); }

CPNGFile::~CPNGFile() = default;

std::uint32_t CPNGFile::Width()  const { return impl->width; }
std::uint32_t CPNGFile::Height() const { return impl->height; }
bool CPNGFile::UsesAlpha()       const { return impl->useAlpha; }
