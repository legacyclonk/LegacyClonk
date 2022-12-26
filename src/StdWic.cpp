/*
 * LegacyClonk
 *
 * Copyright (c) 2017-2019, The LegacyClonk Team and contributors
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
#include <StdWic.h>

#include <StdStringEncodingConverter.h>

#include <winerror.h>

#include <stdexcept>

StdWic::StdWic()
{
	MapHResultError(&StdWic::CreateFactory, this);
}

StdWic::StdWic(const bool decode, const std::string &filename)
	: StdWic()
{
	if (decode) throw std::runtime_error("Decoding from filename is not supported yet");
	MapHResultError(&StdWic::CreateOutputStream, this, filename);
}

StdWic::StdWic(const bool decode, const void *const fileContents, const std::size_t fileSize)
	: StdWic()
{
	if (!decode) throw std::runtime_error("Encoding to memory is not supported yet");
	MapHResultError(&StdWic::CreateInputStream, this, fileContents, fileSize);
}

void StdWic::CreateFactory()
{
	factory = winrt::create_instance<IWICImagingFactory>(CLSID_WICImagingFactory);
}

void StdWic::CreateStream()
{
	winrt::check_hresult(factory->CreateStream(stream.put()));
}

void StdWic::CreateOutputStream(const std::string &filename)
{
	CreateStream();
	winrt::check_hresult(stream->InitializeFromFilename(StdStringEncodingConverter::WinAcpToUtf16(filename).c_str(), GENERIC_WRITE));
}

void StdWic::CreateInputStream(const void *const fileContents, const std::size_t size)
{
	CreateStream();
	winrt::check_hresult(stream->InitializeFromMemory(static_cast<BYTE *>(const_cast<void *>(fileContents)), size));
}

void StdWic::PrepareEncode(const std::uint32_t width, const std::uint32_t height,
	const GUID containerFormat, const WICPixelFormatGUID pixelFormat)
{
	MapHResultError([&, this]
	{
		// Create encoder, output file stream and frame
		winrt::check_hresult(factory->CreateEncoder(containerFormat, nullptr, encoder.put()));
		winrt::check_hresult(encoder->Initialize(stream.get(), WICBitmapEncoderNoCache));

		// Create frame and set its pixel format and size
		winrt::check_hresult(encoder->CreateNewFrame(frame.put(), nullptr));
		winrt::check_hresult(frame->Initialize(nullptr));

		WICPixelFormatGUID usedPixelFormat = pixelFormat;
		winrt::check_hresult(frame->SetPixelFormat(&usedPixelFormat));

		if (usedPixelFormat != pixelFormat)
		{
			throw std::runtime_error{"The specified pixel format cannot be used"};
		}

		winrt::check_hresult(frame->SetSize(width, height));
	});
}

void StdWic::Encode(const UINT lineCount, const UINT stride, const UINT bufferSize,
	const void *const pixels)
{
	MapHResultError([&, this]
	{
		winrt::check_hresult(frame->WritePixels(lineCount, stride, bufferSize,
				static_cast<BYTE *>(const_cast<void *>(pixels))));

		winrt::check_hresult(frame->Commit());
		winrt::check_hresult(encoder->Commit());
	});
}

void StdWic::PrepareDecode(const GUID containerFormat)
{
	MapHResultError([&, this]
	{
		// Create and initialize decoder for the specified container format GUID
		winrt::com_ptr<IWICBitmapDecoder> decoder;
		winrt::check_hresult(factory->CreateDecoder(containerFormat, nullptr, decoder.put()));
		winrt::check_hresult(decoder->Initialize(stream.get(), WICDecodeMetadataCacheOnDemand));

		// Get bitmap frame
		winrt::check_hresult(decoder->GetFrame(0, frameDecode.put()));
		source = frameDecode;
	});
}

void StdWic::GetSize(UINT *const width, UINT *const height)
{
	MapHResultError([=, this]
	{
		winrt::check_hresult(source->GetSize(width, height));
	});
}

WICPixelFormatGUID StdWic::GetOutputFormat()
{
	return GetPixelFormat(source);
}

void StdWic::SetOutputFormat(const WICPixelFormatGUID outputFormat)
{
	MapHResultError([=, this]
	{
		// Do nothing if the current output format is already the desired format
		if (GetOutputFormat() == outputFormat) return;

		winrt::com_ptr<IWICBitmapSource> newSource;
		winrt::check_hresult(WICConvertBitmapSource(outputFormat, frameDecode.get(), newSource.put()));
		source = newSource;
	});
}

void StdWic::CopyPixels(
	const WICRect *const rect, const UINT stride, const UINT bufferSize, void *const pixels)
{
	MapHResultError([=, this]
	{
		// Copy pixels of decoded image
		winrt::check_hresult(source->CopyPixels(rect, stride, bufferSize, static_cast<BYTE *>(pixels)));
	});
}

WICPixelFormatGUID StdWic::GetPixelFormat(const winrt::com_ptr<IWICBitmapSource> &source)
{
	return MapHResultError([&]
	{
		WICPixelFormatGUID pixelFormat;
		winrt::check_hresult(source->GetPixelFormat(&pixelFormat));
		return pixelFormat;
	});
}
