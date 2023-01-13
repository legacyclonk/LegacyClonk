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
#include <wrl.h>

#include <stdexcept>

StdWic::StdWic()
{
	CreateFactory();
}

StdWic::StdWic(const bool decode, const std::string &filename)
	: StdWic()
{
	if (decode) throw std::runtime_error("Decoding from filename is not supported yet");
	CreateOutputStream(filename);
}

StdWic::StdWic(const bool decode, const void *const fileContents, const std::size_t fileSize)
	: StdWic()
{
	if (!decode) throw std::runtime_error("Encoding to memory is not supported yet");
	CreateInputStream(fileContents, fileSize);
}

void StdWic::CreateFactory()
{
	ThrowIfFailed(CoCreateInstance(
		CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
		IID_IWICImagingFactory, reinterpret_cast<LPVOID *>(factory.GetAddressOf())),
		"CoCreateInstance failed to create an instance of IWICImagingFactory");
}

void StdWic::CreateStream()
{
	ThrowIfFailed(factory->CreateStream(stream.GetAddressOf()), "CreateStream failed");
}

void StdWic::CreateOutputStream(const std::string &filename)
{
	CreateStream();
	ThrowIfFailed(stream->InitializeFromFilename(
		StdStringEncodingConverter::WinAcpToUtf16(filename).c_str(), GENERIC_WRITE),
		"InitializeFromFilename failed");
}

void StdWic::CreateInputStream(const void *const fileContents, const std::size_t size)
{
	CreateStream();
	ThrowIfFailed(
		stream->InitializeFromMemory(static_cast<BYTE *>(const_cast<void *>(fileContents)), size),
		"InitializeFromMemory failed");
}

void StdWic::ThrowIfFailed(const HRESULT result, const std::string msg)
{
	if (FAILED(result))
	{
		throw std::runtime_error(msg + " (error " + std::to_string(result) + ")");
	}
}

void StdWic::PrepareEncode(const std::uint32_t width, const std::uint32_t height,
	const GUID containerFormat, const WICPixelFormatGUID pixelFormat)
{
	// Create encoder, output file stream and frame
	ThrowIfFailed(factory->CreateEncoder(containerFormat, nullptr, encoder.GetAddressOf()),
		"CreateEncoder failed");
	ThrowIfFailed(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache),
		"IWICBitmapEncoder::Initialize failed");

	// Create frame and set its pixel format and size
	ThrowIfFailed(encoder->CreateNewFrame(frame.GetAddressOf(), nullptr),
		"CreateNewFrame failed");
	ThrowIfFailed(frame->Initialize(nullptr),
		"IWICBitmapFrameEncode::Initialize failed");
	WICPixelFormatGUID usedPixelFormat = pixelFormat;
	ThrowIfFailed(frame->SetPixelFormat(&usedPixelFormat),
		"SetPixelFormat failed");
	if (usedPixelFormat != pixelFormat)
	{
		throw std::runtime_error("The specified pixel format cannot be used");
	}
	ThrowIfFailed(frame->SetSize(width, height),
		"SetSize failed");
}

void StdWic::Encode(const UINT lineCount, const UINT stride, const UINT bufferSize,
	const void *const pixels)
{
	ThrowIfFailed(
		frame->WritePixels(lineCount, stride, bufferSize,
			static_cast<BYTE *>(const_cast<void *>(pixels))),
		"WritePixels failed");

	ThrowIfFailed(frame->Commit(), "frame->Commit failed");
	ThrowIfFailed(encoder->Commit(), "encoder->Commit failed");
}

void StdWic::PrepareDecode(const GUID containerFormat)
{
	// Create and initialize decoder for the specified container format GUID
	Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
	ThrowIfFailed(factory->CreateDecoder(containerFormat, nullptr, decoder.GetAddressOf()),
		"CreateDecoder failed");
	ThrowIfFailed(decoder->Initialize(stream.Get(), WICDecodeMetadataCacheOnDemand),
		"Initialize failed");

	// Get bitmap frame
	ThrowIfFailed(decoder->GetFrame(0, frameDecode.GetAddressOf()), "GetFrame failed");
	source = frameDecode;
}

void StdWic::GetSize(UINT *const width, UINT *const height)
{
	ThrowIfFailed(source->GetSize(width, height), "GetSize failed");
}

WICPixelFormatGUID StdWic::GetOutputFormat()
{
	return GetPixelFormat(source);
}

void StdWic::SetOutputFormat(const WICPixelFormatGUID outputFormat)
{
	// Do nothing if the current output format is already the desired format
	if (GetOutputFormat() == outputFormat) return;

	Microsoft::WRL::ComPtr<IWICBitmapSource> newSource;
	ThrowIfFailed(WICConvertBitmapSource(outputFormat, frameDecode.Get(), newSource.GetAddressOf()),
		"WICConvertBitmapSource failed");
	source = newSource;
}

void StdWic::CopyPixels(
	const WICRect *const rect, const UINT stride, const UINT bufferSize, void *const pixels)
{
	// Copy pixels of decoded image
	ThrowIfFailed(source->CopyPixels(rect, stride, bufferSize, static_cast<BYTE *>(pixels)),
		"CopyPixels failed");
}

WICPixelFormatGUID StdWic::GetPixelFormat(const Microsoft::WRL::ComPtr<IWICBitmapSource> source)
{
	WICPixelFormatGUID pixelFormat;
	ThrowIfFailed(source->GetPixelFormat(&pixelFormat), "GetPixelFormat failed");
	return pixelFormat;
}
