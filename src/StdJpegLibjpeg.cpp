/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender  RedWolf Design
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

// JPEG decoding using libjpeg

#include <Standard.h>
#include <StdJpeg.h>

#include <stdexcept>

// Some distributions ship jpeglib.h with extern "C", others don't - gah.
extern "C"
{
#include <jpeglib.h>
}

struct StdJpeg::Impl
{
	jpeg_decompress_struct cinfo;
	jpeg_error_mgr error_mgr;
	jpeg_source_mgr source_mgr;

	static constexpr JOCTET end_of_input[] = { 0xff, JPEG_EOI };

	void *rowBuffer;

	Impl(const void *const fileContents, const std::size_t fileSize)
	{
		try
		{
			Init(fileContents, fileSize);
		}
		catch (const std::runtime_error &)
		{
			Clear();
			throw;
		}
	}

	void Init(const void *const fileContents, const std::size_t fileSize)
	{
		// Error handling: Throw exception on critical error; ignore non-critical messages
		cinfo.err = jpeg_std_error(&error_mgr);
		error_mgr.error_exit = [](const j_common_ptr cinfo)
		{
			char buffer[JMSG_LENGTH_MAX];
			cinfo->err->format_message(cinfo, buffer);
			throw std::runtime_error(buffer);
		};
		error_mgr.output_message = [](j_common_ptr) {};
		jpeg_create_decompress(&cinfo);

		// no fancy function calling
		cinfo.src = &source_mgr;
		source_mgr.next_input_byte = static_cast<const JOCTET *>(fileContents);
		source_mgr.bytes_in_buffer = fileSize;
		source_mgr.init_source = [](j_decompress_ptr) {};
		source_mgr.fill_input_buffer = [](const j_decompress_ptr cinfo)
		{
			// The doc says to give fake end-of-inputs if there is no more data
			cinfo->src->next_input_byte = end_of_input;
			cinfo->src->bytes_in_buffer = sizeof(end_of_input);
			return static_cast<boolean>(true);
		};
		source_mgr.skip_input_data = [](const j_decompress_ptr cinfo, const long num_bytes)
		{
			cinfo->src->next_input_byte += num_bytes;
			cinfo->src->bytes_in_buffer -= num_bytes;
			if (cinfo->src->bytes_in_buffer <= 0)
			{
				cinfo->src->next_input_byte = end_of_input;
				cinfo->src->bytes_in_buffer = sizeof(end_of_input);
			}
		};
		source_mgr.resync_to_restart = jpeg_resync_to_restart;
		source_mgr.term_source = [](j_decompress_ptr) {};

		// a missing image is an error
		jpeg_read_header(&cinfo, TRUE);

		// Let libjpeg convert for us
		cinfo.out_color_space = JCS_RGB;
		jpeg_start_decompress(&cinfo);

		// Make a one-row-high sample array that will go away at jpeg_destroy_decompress
		const JDIMENSION samplesPerRow = cinfo.output_width * cinfo.output_components;
		rowBuffer = (*cinfo.mem->alloc_sarray)
			(reinterpret_cast<j_common_ptr>(&cinfo), JPOOL_IMAGE, samplesPerRow, 1);
	}

	~Impl()
	{
		Clear();
	}

	void Clear()
	{
		jpeg_destroy_decompress(&cinfo);
	}

	void Finish()
	{
		jpeg_finish_decompress(&cinfo);
	}

	const void *DecodeRow()
	{
		if (cinfo.output_scanline >= cinfo.output_height) return nullptr;

		// read an 1-row-array of scanlines
		jpeg_read_scanlines(&cinfo, static_cast<JSAMPARRAY>(rowBuffer), 1);

		return static_cast<JSAMPARRAY>(rowBuffer)[0];
	}
};

constexpr JOCTET StdJpeg::Impl::end_of_input[];

StdJpeg::StdJpeg(const void *const fileContents, const std::size_t fileSize)
	: impl(new Impl(fileContents, fileSize)) {}

StdJpeg::~StdJpeg() = default;

const void *StdJpeg::DecodeRow() { return impl->DecodeRow(); }
void StdJpeg::Finish() { impl->Finish(); }

std::uint32_t StdJpeg::Width()  const { return impl->cinfo.output_width; }
std::uint32_t StdJpeg::Height() const { return impl->cinfo.output_height; }
