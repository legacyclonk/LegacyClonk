/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2020, The LegacyClonk Team and contributors
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

/* A structure for handling 256-color bitmap files */

#include <Standard.h>
#include <Bitmap256.h>

#include <cstring>
#include <algorithm>

CBitmapInfo::CBitmapInfo()
{
	Default();
}

void CBitmapInfo::Default()
{
	Head = {};
	Info = {};
}

int CBitmapInfo::FileBitsOffset()
{
	return Head.bfOffBits - sizeof(CBitmapInfo);
}

CBitmap256Info::CBitmap256Info()
{
	Default();
}

int CBitmap256Info::FileBitsOffset()
{
	return Head.bfOffBits - sizeof(CBitmap256Info);
}

void CBitmap256Info::Set(int iWdt, int iHgt, uint8_t *bypPalette)
{
	Default();
	// Set header
	Head.bfType = 0x4d42; // bmp magic "BM"
	Head.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD) + DWordAligned(iWdt) * iHgt;
	Head.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD);
	// Set bitmap info
	Info.biSize = sizeof(BITMAPINFOHEADER);
	Info.biWidth = iWdt;
	Info.biHeight = iHgt;
	Info.biPlanes = 1;
	Info.biBitCount = 8;
	Info.biCompression = 0;
	Info.biSizeImage = iWdt * iHgt;
	Info.biClrUsed = Info.biClrImportant = 256;
	// Set palette
	for (int cnt = 0; cnt < 256; cnt++)
	{
		Colors[cnt].rgbRed   = bypPalette[cnt * 3 + 0];
		Colors[cnt].rgbGreen = bypPalette[cnt * 3 + 1];
		Colors[cnt].rgbBlue  = bypPalette[cnt * 3 + 2];
	}
}

void CBitmap256Info::Default()
{
	CBitmapInfo::Default();
	std::fill_n(Colors, std::size(Colors), RGBQUAD{});
}
