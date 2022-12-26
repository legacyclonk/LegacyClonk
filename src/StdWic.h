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

#pragma once

#include "C4WinRT.h"

#include <cstdint>
#include <wincodec.h>
#include <winnt.h>

#include <string>

// Makes working with Windows Imaging Component easier
class StdWic
{
public:
	StdWic(bool decode, const std::string &filename);
	StdWic(bool decode, const void *fileContents, std::size_t fileSize);

	void PrepareEncode(std::uint32_t width, std::uint32_t height,
		GUID containerFormat, WICPixelFormatGUID pixelFormat);
	void Encode(UINT lineCount, UINT stride, UINT bufferSize, const void *pixels);

	void PrepareDecode(GUID containerFormat);
	void GetSize(UINT *width, UINT *height);
	WICPixelFormatGUID GetOutputFormat();
	void SetOutputFormat(WICPixelFormatGUID outputFormat);
	void CopyPixels(const WICRect *rect, UINT stride, UINT bufferSize, void *pixels);

private:
	winrt::com_ptr<IWICImagingFactory> factory;
	winrt::com_ptr<IWICStream> stream;
	winrt::com_ptr<IWICBitmapEncoder> encoder;
	winrt::com_ptr<IWICBitmapFrameEncode> frame;

	StdWic();
	void CreateFactory();
	void CreateStream();
	void CreateOutputStream(const std::string &filename);
	void CreateInputStream(const void *fileContents, std::size_t size);

	// Bitmap frame of the source file.
	winrt::com_ptr<IWICBitmapFrameDecode> frameDecode;
	// Source bitmap when decoding. Points to frameDecode if no conversion is needed.
	winrt::com_ptr<IWICBitmapSource> source;

	static WICPixelFormatGUID GetPixelFormat(const winrt::com_ptr<IWICBitmapSource> &source);
};
