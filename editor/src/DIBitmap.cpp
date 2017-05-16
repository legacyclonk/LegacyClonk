/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Loads and draws bitmaps in windows */

#include "C4Explorer.h"

CDIBitmap::CDIBitmap() 
	{
	Default();
	}

CDIBitmap::~CDIBitmap()
	{
	Clear();
	}

bool CDIBitmap::Clear()
	{
	if (OwnBMI && pBMI) delete [] pBMI;  // Wenn BMI-Buffer selbst alloziert, löschen
	pBMI = NULL;
	pBMIH = NULL;
	pBits = NULL;
	OwnBMI = false;
	if (bypMask) delete [] bypMask; bypMask=NULL;
	iSectionX=iSectionY=iSectionWdt=iSectionHgt=0;
	return true;
	}

char* CDIBitmap::GetBitsPointer() const
	{
	switch (pBMIH->biBitCount)
		{
		case 4:
			return (char*)(pBMI->bmiColors + 16);
		case 8:
			return (char*)(pBMI->bmiColors + 256);
		case 24:
			return (char*)(pBMI->bmiColors);
		default:
			return NULL;
		}
	}

//---------------------------------------------------------------------------

// DIB aus Resource laden

bool CDIBitmap::Load(UINT ID, HMODULE hModule)
{
	// Eventuelles altes Bild löschen
	Clear();

	// DIB aus Resource laden
	if (!hModule) hModule = AfxGetResourceHandle();
	if (!hModule) return false;
	HRSRC hRsrc = FindResource(hModule, MAKEINTRESOURCE(ID), RT_BITMAP);
	if (!hRsrc) return false;
	HGLOBAL hBMI = LoadResource(hModule, hRsrc);
	if (!hBMI) return false;
	pBMI = (BITMAPINFO*) LockResource(hBMI);
	if (!pBMI) return false;
	pBMIH = &pBMI->bmiHeader;

	// Pointer auf Bits ermitteln
	pBits = GetBitsPointer();
	if (!pBits) return false;

	return true;
}

//---------------------------------------------------------------------------

// DIB aus einem BITMAPFILEHEADER im Speicher kopieren
// (Packed DIB = BITMAPINFO mit Bits dahinter, hat mit RLE nichts zu tun)
// BMFH.bfSize soll nicht immer stimmen, daher wird Größe extra übergeben

bool CDIBitmap::Load(BITMAPFILEHEADER* pBMFH, int Size)
	{
	// Eventuelles altes Bild löschen
	Clear();

	// Das gepackte DIB aus dem BMFH kopieren
	if (!pBMFH) return false;
	int PackedDIBSize = Size - sizeof(BITMAPFILEHEADER);
	pBMI = (BITMAPINFO*) new char[PackedDIBSize];
	if (!pBMI) return false;
	memcpy(pBMI, (char*) pBMFH + sizeof(BITMAPFILEHEADER), PackedDIBSize);
	pBMIH = &pBMI->bmiHeader;
	OwnBMI = true;

	// Pointer auf Bits ermitteln
	pBits = GetBitsPointer();
	if (!pBits) return false;

	return true;
	}

//---------------------------------------------------------------------------

// DIB aus BMP-Datei laden

bool CDIBitmap::Load(CString FileName)
{
	FILE* BmpFile = fopen(FileName, "rb");
	if (!BmpFile) return false;
	int FileLen = _filelength(_fileno(BmpFile));
	char* pBuffer = new char[FileLen];
	if (!pBuffer) return false;
	fread(pBuffer, FileLen, 1, BmpFile);
	fclose(BmpFile);
	if (!Load((BITMAPFILEHEADER*) pBuffer, FileLen)) return false;
	delete [] pBuffer;
	return true;
}

//---------------------------------------------------------------------------

// DIB aus geöffneter Gruppendatei laden

bool CDIBitmap::Load(C4Group& Group, CString FileName)
	{
	Clear();
	unsigned int BufferSize;
	char* pBuffer;
	if (Group.AccessEntry(FileName, &BufferSize))
		{
		pBuffer = new char[BufferSize];
		Group.Read(pBuffer, BufferSize);
		Load((BITMAPFILEHEADER*) pBuffer, BufferSize);
		delete [] pBuffer;
		return true;
		}
	else 
		return false;
	}

//---------------------------------------------------------------------------

// LandscapeBuffer laden

bool CDIBitmap::LoadLandscape(char* pBits, int w, int h, RGBQUAD* pPalette)
{
	if (!pBits) return false;

	if (!pBMI)  // Wenn noch kein BMI alloziert
	{
		// BMI allozieren und initialisieren
		pBMI = (BITMAPINFO*) new char[sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 256];
		if (!pBMI) return false;
		OwnBMI = true;
		pBMIH = &pBMI->bmiHeader;
		ZeroMem(pBMIH,sizeof(BITMAPINFOHEADER));
		pBMIH->biSize = sizeof(BITMAPINFOHEADER);
		pBMIH->biWidth = w;
		pBMIH->biHeight = -h;  // Buffer ist Top-Down
		pBMIH->biPlanes = 1;
		pBMIH->biBitCount = 8;
		this->pBits = pBits;
		for (int i=0; i < 256; i++) pBMI->bmiColors[i] = pPalette[i];  // Palette anpassen
	}
	else  // Sonst nur neue Größe anpassen
	{
		pBMIH->biWidth = w;
		pBMIH->biHeight = -h;
		this->pBits = pBits;
	}

	return true;
}


bool CDIBitmap::Draw(CDC* pDC, CWnd* pWnd) const
	{
	CRect Rect;
	pWnd->GetClientRect(Rect);
	return Draw(pDC->m_hDC, 0,0, Rect.right, Rect.bottom, true);
	}

bool CDIBitmap::Draw(HDC hDC, int iX, int iY, int iWdt, int iHgt, bool fStretch) const
	{	
	if (!pBMI) return false;
	
	int iSrcWdt = pBMIH->biWidth, iSrcHgt = abs(pBMIH->biHeight);
	if (!iWdt) iWdt = iSrcWdt; if (!iHgt) iHgt = iSrcHgt;
	
	HPALETTE hOldPal = SelectPalette(hDC, GetApp()->Palette, false); RealizePalette(hDC);

	SetStretchBltMode(hDC,STRETCH_DELETESCANS);	

	StretchDIBits(hDC, iX,iY, iWdt,iHgt, 0,0, iSrcWdt,iSrcHgt, pBits, pBMI, DIB_RGB_COLORS, SRCCOPY);
	
	SelectPalette(hDC, hOldPal, false);

	return true;
	}

bool CDIBitmap::DrawSection(int iSecIndexX, int iSecIndexY, CDC *pDC, CRect Rect, 
														bool fAspect, bool fCentered)
	{
	if (!pBMI) return false;

	HDC hDC = pDC->m_hDC;

	HPALETTE hOldPal = SelectPalette(hDC, GetApp()->Palette, false); RealizePalette(hDC);
	
	// No empty sections
	int iWdt = iSectionWdt, iHgt = iSectionHgt;
	if (!iWdt) iWdt = pBMIH->biWidth; 
	if (!iHgt) iHgt = abs(pBMIH->biHeight);

	if (!iWdt || !iHgt) return true;

	// Adjust section y source
	int iSrcY = 0;
	if (iSectionHgt) iSrcY = abs(pBMIH->biHeight)-(iSectionY+iSectionHgt);

	// Fixed aspect target rect
	if (fAspect)
		{
		int iImageWdt = iWdt;
		int iImageHgt = abs(iHgt);
		int iRectWdt = Rect.Width();
		int iRectHgt = Rect.Height();
		int iDrawWdt, iDrawHgt;
		if (iImageWdt > iImageHgt) 
			{	iDrawWdt = iRectWdt; iDrawHgt = iImageHgt * iRectWdt / iImageWdt;	iDrawHgt = min(iDrawHgt, iRectHgt);	}
		else 
			{	iDrawWdt = iImageWdt * iRectHgt / iImageHgt; iDrawWdt = min(iDrawWdt, iRectWdt); iDrawHgt = iRectHgt;	}		
		int iOffsetX = (iRectWdt-iDrawWdt)/2;
		int iOffsetY = (iRectHgt-iDrawHgt)/2;
		Rect.right = Rect.left+iDrawWdt;
		Rect.bottom = Rect.top+iDrawHgt;
		if (fCentered)
			{
			Rect.left+=iOffsetX; Rect.right+=iOffsetX;
			Rect.top+=iOffsetY; Rect.bottom+=iOffsetY;
			}
		}

	// Blit
	SetStretchBltMode(hDC,STRETCH_DELETESCANS);	
	
	// Normal
	if (!bypMask)
		{
		StretchDIBits(hDC, Rect.left,Rect.top, Rect.Width(),Rect.Height(), 
											 iSectionX+iWdt*iSecIndexX,
											 iSrcY + iHgt*iSecIndexY, 
											 iWdt,iHgt, 
											 pBits, pBMI, DIB_RGB_COLORS, SRCCOPY );	
		}
	
	// Transparent with shadow and mask
	else
		{
		
		SetColor(0,128,128,128);
		StretchDIBits(hDC, Rect.left+3,Rect.top+3, Rect.Width(),Rect.Height(), 
											 iSectionX+iWdt*iSecIndexX,
											 iSrcY + iHgt*iSecIndexY, 
											 iWdt,iHgt, 
											 bypMask, pBMI, DIB_RGB_COLORS, SRCINVERT );	
		
		SetColor(0,255,255,255);
		StretchDIBits(hDC, Rect.left,Rect.top, Rect.Width(),Rect.Height(), 
											 iSectionX+iWdt*iSecIndexX,
											 iSrcY + iHgt*iSecIndexY, 
											 iWdt,iHgt, 
											 bypMask, pBMI, DIB_RGB_COLORS, SRCPAINT );	
		StretchDIBits(hDC, Rect.left,Rect.top, Rect.Width(),Rect.Height(), 
											 iSectionX+iWdt*iSecIndexX,
											 iSrcY + iHgt*iSecIndexY, 
											 iWdt,iHgt, 
											 pBits, pBMI, DIB_RGB_COLORS, SRCAND );
		}			
	
	SelectPalette(hDC, hOldPal, false);

	return true;
	}

bool CDIBitmap::DrawTiles(CDC* pDC, CWnd* pWnd) const
{
	if (!pBMI) return false;

	CRect Rect;
	pWnd->GetClientRect(Rect);
	int sw = pBMIH->biWidth;
	int sh = pBMIH->biHeight;

	CPalette* pOldPal = pDC->SelectPalette(&GetApp()->Palette, false);
	pDC->RealizePalette();
	pDC->SetStretchBltMode(STRETCH_DELETESCANS);	
	for (int x=0; x < (Rect.right / sw + 1); x++)
		for (int y=0; y < (Rect.bottom / sh + 1); y++)
			StretchDIBits(*pDC, x*sw,y*sh, sw,sh, 0,0, sw,sh, pBits, pBMI, DIB_RGB_COLORS, SRCCOPY);
	pDC->SelectPalette(pOldPal, false);

	return true;
}

//---------------------------------------------------------------------------

bool CDIBitmap::DrawFixedAspect(CDC* pDC, CRect Rect, CSize* pSizeDrawn) const
{
	if (!pBMI) return false;

	int sw = pBMIH->biWidth;
	int sh = abs(pBMIH->biHeight);
	int rw = Rect.Width();
	int rh = Rect.Height();
	int dw, dh;

	if (sw > sh)
	{
		dw = rw;
		dh = sh * rw / sw;
		dh = min(dh, rh);
	}
	else
	{
		dw = sw * rh / sh;
		dw = min(dw, rw);
		dh = rh;
	}

	// Return value
	if (pSizeDrawn)
	{
		pSizeDrawn->cx = dw;
		pSizeDrawn->cy = dh;
	}

	// Doublebufferblit
	CDC MemDC;
	MemDC.CreateCompatibleDC(pDC);
	CBitmap Bmp;
	Bmp.CreateCompatibleBitmap(pDC, Rect.Width(), Rect.Height());
	CBitmap* pOldBmp = MemDC.SelectObject(&Bmp);
	MemDC.FillSolidRect(Rect, 0x00FFFFFF);
	Draw(MemDC, 0,0, dw,dh, true);
	pDC->SetStretchBltMode(STRETCH_DELETESCANS);	
	pDC->BitBlt(Rect.left, Rect.top, Rect.Width(), Rect.Height(), &MemDC, 0,0, SRCCOPY);
	MemDC.SelectObject(pOldBmp);

	return true;
}


void CDIBitmap::Default()
	{
	pBMI = NULL;
	pBMIH = NULL;
	pBits = NULL;
	OwnBMI = false;
	iSectionX=iSectionY=iSectionWdt=iSectionHgt=0;
	bypMask = NULL;
	}

void CDIBitmap::SetSection(int iWdt, int iHgt)
	{
	iSectionX=iSectionY=0;
	iSectionWdt=iWdt; iSectionHgt=iHgt;
	}

void CDIBitmap::SetSection(int iX, int iY, int iWdt, int iHgt)
	{
	iSectionX=iX; iSectionY=iY;
	iSectionWdt=iWdt; iSectionHgt=iHgt;
	}

bool CDIBitmap::DrawTiles(CDC * pDC, CRect &Rect)
{
	if (!pBMI) return false;

	int sw = pBMIH->biWidth;
	int sh = pBMIH->biHeight;

	CPalette* pOldPal = pDC->SelectPalette(&GetApp()->Palette, false);
	pDC->RealizePalette();
	pDC->SetStretchBltMode(STRETCH_DELETESCANS);	
	for (int x=0; x < (Rect.right / sw + 1); x++)
		for (int y=0; y < (Rect.bottom / sh + 1); y++)
			StretchDIBits(*pDC, x*sw,y*sh, sw,sh, 0,0, sw,sh, pBits, pBMI, DIB_RGB_COLORS, SRCCOPY);
	pDC->SelectPalette(pOldPal, false);

	return true;

}

void CDIBitmap::SetColor(int iColor, int iRed, int iGreen, int iBlue)
	{
	if (!pBMI || !pBMIH) return;
	switch (pBMIH->biBitCount)
		{
		case 4:
			if (Inside(iColor,0,15))
				{
				pBMI->bmiColors[iColor].rgbRed=iRed;
				pBMI->bmiColors[iColor].rgbGreen=iGreen;
				pBMI->bmiColors[iColor].rgbBlue=iBlue;
				}
			break;
		case 8:
			if (Inside(iColor,0,255))
				{
				pBMI->bmiColors[iColor].rgbRed=iRed;
				pBMI->bmiColors[iColor].rgbGreen=iGreen;
				pBMI->bmiColors[iColor].rgbBlue=iBlue;
				}
			break;
		case 24:
			break;
		}
	}

/*HBITMAP CreateMaskMonochrome(HBITMAP hSource)
	{
	// Get source bitmap info
  BITMAP bmSource;
  ::GetObject(hSource,sizeof(bmSource),&bmSource);
  int iWidth=bmSource.bmWidth;
  int iHeight=bmSource.bmHeight;
	int iSourcePitch=bmSource.bmWidthBytes;
	BYTE *bypSourceBits=(BYTE*)bmSource.bmBits;
	// 256-color source only
	if (bmSource.bmBitsPixel!=8) return NULL;
	// Prepare monochrome bitmap data
	int iPitch = (iWidth/8); if (iWidth%8) iPitch++; 
  if (iPitch%2) { iPitch>>=1; iPitch<<=1; iPitch+=2; } // Word align
	BYTE *bypBits = new BYTE [iPitch*iHeight];
	ZeroMem(bypBits,iPitch*iHeight);
	// Fill monochrome mask data
	for (int iY=0; iY<iHeight; iY++)
		for (int iX=0; iX<iWidth; iX++)
			if (bypSourceBits[iSourcePitch*iY+iX])
				bypBits[iPitch*(iHeight-1-iY)+iX/8] |= (1<<(7-iX%8));
	// Create bitmap
	HBITMAP hResult = CreateBitmap( iWidth, iHeight, 1, 1, bypBits );
	delete [] bypBits;
	return hResult;
	}*/

void CDIBitmap::CreateMask()
	{
	if (!pBMI || !pBMIH) return;
	if (bypMask) delete [] bypMask;
	bypMask=new BYTE [pBMIH->biSizeImage];
	MemCopy(pBits,bypMask,pBMIH->biSizeImage);
	for (DWORD cnt=0; cnt<pBMIH->biSizeImage; cnt++)
		if (bypMask[cnt]) bypMask[cnt]=0;
		else bypMask[cnt]=16;	
	}

bool CDIBitmap::HasMask()
	{
	return (bypMask!=NULL);
	}
