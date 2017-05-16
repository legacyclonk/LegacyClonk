/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* A structure for handling 256-color bitmap files */

#pragma once

#ifdef _WIN32

#include <windows.h>

#else

#pragma pack(push, 2)
typedef struct tagBITMAPFILEHEADER
{
	uint16_t bfType;
	uint32_t bfSize;
	uint16_t bfReserved1;
	uint16_t bfReserved2;
	uint32_t bfOffBits;
} BITMAPFILEHEADER, *LPBITMAPFILEHEADER, *PBITMAPFILEHEADER;
#pragma pack(pop)

typedef struct tagBITMAPINFOHEADER
{
	uint32_t biSize;
	int32_t  biWidth;
	int32_t  biHeight;
	uint16_t biPlanes;
	uint16_t biBitCount;
	uint32_t biCompression;
	uint32_t biSizeImage;
	int32_t  biXPelsPerMeter;
	int32_t  biYPelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef struct tagRGBQUAD
{
	uint8_t rgbBlue;
	uint8_t rgbGreen;
	uint8_t rgbRed;
	uint8_t rgbReserved;
} RGBQUAD, *LPRGBQUAD;

#endif

#pragma pack(push, def_pack, 1)

class CBitmapInfo
{
public:
	CBitmapInfo();
	void Default();

public:
	BITMAPFILEHEADER Head;
	BITMAPINFOHEADER Info;

	int FileBitsOffset();
};

class CBitmap256Info : public CBitmapInfo
{
public:
	CBitmap256Info();
	RGBQUAD Colors[256];

public:
	void Default();
	void Set(int iWdt, int iHgt, uint8_t *bypPalette);

	int FileBitsOffset();
};

#pragma pack(pop, def_pack)
