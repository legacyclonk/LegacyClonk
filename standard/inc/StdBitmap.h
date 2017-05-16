/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Bitmap handling routines */

#pragma pack( push, def_pack , 1)

#include "Bitmap256.h"

class CStdBitmapHead: public BITMAPFILEHEADER
	{
	public:
		CStdBitmapHead(void);
	public:
		void Clear(void);
		void Set(int iBitOffset);
		BOOL Valid(void);
	};

class CStdBitmapInfo: public BITMAPINFOHEADER
	{
	public:
		CStdBitmapInfo(void);
	public:
		void Clear(void);
		void Set(int iWdt, int iHgt, int iBitsPerPixel);
		int Pitch(void);
	};

class CStdBitmap
	{
	public:
		CStdBitmap();
		~CStdBitmap();
	public:
		CStdBitmapHead Head;
		CStdBitmapInfo Info;
		RGBQUAD Colors[256];
		BYTE *Bits;
	public:
		BOOL Ensure(int iWdt, int iHgt, int iBitsPerPixel);
		int getPitch();
		int getHeight();
		int getWidth();
		void Clear(void);
		BOOL Create(int iWdt, int iHgt, int iBitsPerPixel);
		BOOL Save(const char *szFileName);
		BOOL Load(const char *szFileName);
		BOOL Enlarge(int iWdt, int iHgt);
	};

#pragma pack( pop, def_pack )
