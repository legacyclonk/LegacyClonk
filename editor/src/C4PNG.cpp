
/* used to load/decompress PNGs and hold image data */

#include "C4Explorer.h"
#include "C4PNG.h"

C4PNG::C4PNG()
: pImageData(NULL), pMask(NULL), pColorMask(NULL),
  iWidth(0), iHeight(0), iPitch(0), iMaskPitch(0),
	iSctX(0), iSctY(0), iSctWdt(0), iSctHgt(0),
	bSectionMode(false)
{

}

C4PNG::C4PNG(const C4PNG &PNG2)
{
	iWidth = PNG2.iWidth;
	iHeight = PNG2.iHeight;
	iPitch = PNG2.iPitch;
	iMaskPitch = PNG2.iMaskPitch;

	iSctX = PNG2.iSctX;
	iSctY = PNG2.iSctY;
	iSctWdt = PNG2.iSctWdt;
	iSctHgt = PNG2.iSctHgt;

	if(PNG2.pImageData)
	{
		pImageData = new BYTE [iPitch * iHeight];
		memcpy(pImageData, PNG2.pImageData, iPitch * iHeight);
	}
	if(PNG2.pMask)
	{
		pMask = new BYTE [iMaskPitch * iHeight];
		memcpy(pMask, PNG2.pMask, iMaskPitch * iHeight);
	}
	if(PNG2.pColorMask)
	{
		pColorMask = new BYTE [iWidth * iHeight];
		memcpy(pColorMask, PNG2.pColorMask, iWidth * iHeight);
	}
}

C4PNG::~C4PNG()
{
	Clear(true);
}

bool C4PNG::Load(C4Group *pParentGroup, CString strEntryName)
{
	// access entry
	unsigned int iSize;
	if(!pParentGroup->AccessEntry(strEntryName, &iSize))
		return false;
	// alloc buffer
	BYTE *pBuff = new BYTE [iSize];
	// read data
	if(!pParentGroup->Read(pBuff, iSize))
	{
		delete[] pBuff;
		return false;
	}
	// load png
	CPNGFile PNGFile;
	if(!PNGFile.Load(pBuff, iSize))
	{
		delete[] pBuff;
		return false;
	}
	// delete buffer
	delete[] pBuff;
	
	iWidth = PNGFile.iWdt;
	iHeight = PNGFile.iHgt;
	iPitch = iWidth * 3 + (3 - (iWidth * 3 - 1) % 4);
	iMaskPitch = iPitch; // iWidth + (3 - (iWidth - 1) % 4);

	iSctX = iSctY = 0;
	iSctWdt = iWidth; iSctHgt = iHeight;
	bSectionMode = false;

	delete[] pImageData; pImageData = NULL;
	delete[] pMask; pMask = NULL;

	pImageData = new BYTE [iHeight * iPitch];

#ifdef USE_MASK
	if(PNGFile.iClrType == PNG_COLOR_TYPE_RGB_ALPHA)
		pMask = new BYTE [iHeight * iMaskPitch];
#endif

	// get background bitmap
	/*CDIBitmap *pBG = &GetApp()->dibPaper;
	char *pBGBits = pBG->pBits;
	int iBGClrFormat = pBG->pBMIH->biBitCount;
	int iBGWdt = pBG->pBMIH->biWidth,
		  iBGHgt = pBG->pBMIH->biHeight;
	int iBGPitch = iBGWdt * iBGClrFormat / 8;
	iBGPitch += 3 - ((iBGPitch - 1) % 4);*/

	DWORD dwBGClr = RGB2BGR(GetSysColor(COLOR_BTNFACE));

	// read data & mix with bg
	for(int x = 0; x < iWidth; x++)
		for(int y = 0; y < iHeight; y++)
		{
			DWORD dwColor = PNGFile.GetPix(x, y);
			if((dwColor >> 24) != 0)
			{
				/*switch(iBGClrFormat)
				{
				case 1:
				case 2:
				case 4:
				case 8:
					dwBGClr = *(pBGBits + (x % iBGWdt) * iBGClrFormat / 8 + (y % iBGHgt) * iBGPitch);
					dwBGClr >>= ((x % iBGWdt) * iBGClrFormat) % 8;
					dwBGClr &= (1 << iBGClrFormat) - 1;
					dwBGClr = RGB(pBG->pBMI->bmiColors[dwBGClr].rgbRed, 
												pBG->pBMI->bmiColors[dwBGClr].rgbGreen,
												pBG->pBMI->bmiColors[dwBGClr].rgbBlue);
					break;
				case 24:
				case 32:
					dwBGClr = *(DWORD *)(pBGBits + (x % iBGWdt) * iBGClrFormat / 8 + (y % iBGHgt) * iBGPitch);
					dwBGClr &= (1 << iBGClrFormat) - 1;
					break;
				}*/
				// mix'em
				if((dwColor >> 24) != 255)
				{
					float fAlpha = float(dwColor >> 24) / 255;
					*(pImageData + x * 3 + y * iPitch + 0) = (BYTE)((1 - fAlpha) * BYTE(dwColor >>  0) + fAlpha * BYTE(dwBGClr >>  0));
					*(pImageData + x * 3 + y * iPitch + 1) = (BYTE)((1 - fAlpha) * BYTE(dwColor >>  8) + fAlpha * BYTE(dwBGClr >>  8));
					*(pImageData + x * 3 + y * iPitch + 2) = (BYTE)((1 - fAlpha) * BYTE(dwColor >> 16) + fAlpha * BYTE(dwBGClr >> 16));
				}
				else
				{
					*(pImageData + x * 3 + y * iPitch + 0) = BYTE(dwBGClr >>  0);
					*(pImageData + x * 3 + y * iPitch + 1) = BYTE(dwBGClr >>  8);
					*(pImageData + x * 3 + y * iPitch + 2) = BYTE(dwBGClr >> 16);
				}
			}
			else
			{
				*(pImageData + x * 3 + y * iPitch + 0) = BYTE(dwColor >>  0);
				*(pImageData + x * 3 + y * iPitch + 1) = BYTE(dwColor >>  8);
				*(pImageData + x * 3 + y * iPitch + 2) = BYTE(dwColor >> 16);
			}
		}

#ifdef USE_MASK
	// read data & create mask
	for(int x = 0; x < iWidth; x++)
		for(int y = 0; y < iHeight; y++)
		{
			DWORD dwColor = PNGFile.GetPix(x, y);
			if(pMask)
				if((dwColor >> 24) < 150)
				{
					*(pMask + x * 3 + y * iMaskPitch + 0) = 0xFF;
					*(pMask + x * 3 + y * iMaskPitch + 1) = 0xFF;
					*(pMask + x * 3 + y * iMaskPitch + 2) = 0xFF;
					*(pImageData + x * 3 + y * iPitch + 0) = BYTE(dwColor >>  0);
					*(pImageData + x * 3 + y * iPitch + 1) = BYTE(dwColor >>  8);
					*(pImageData + x * 3 + y * iPitch + 2) = BYTE(dwColor >> 16);
				}
				else
				{
					*(pMask + x * 3 + y * iMaskPitch + 0) = 0x00;
					*(pMask + x * 3 + y * iMaskPitch + 1) = 0x00;
					*(pMask + x * 3 + y * iMaskPitch + 2) = 0x00;
					*(pImageData + x * 3 + y * iPitch + 0) = 0xFF;
					*(pImageData + x * 3 + y * iPitch + 1) = 0xFF;
					*(pImageData + x * 3 + y * iPitch + 2) = 0xFF;
				}
			else
			{
				*(pImageData + x * 3 + y * iPitch + 0) = BYTE(dwColor >>  0);
				*(pImageData + x * 3 + y * iPitch + 1) = BYTE(dwColor >>  8);
				*(pImageData + x * 3 + y * iPitch + 2) = BYTE(dwColor >> 16);
			}
		}
#endif

	// success
	return true;
}

bool C4PNG::Draw(HDC hDC, RECT trect, DWORD *pColorDw)
{
	int iSourceX = iSctX, 
		  iSourceY = iSctY, 
		  iSourceWdt = iSctWdt,
			iSourceHgt = iSctHgt;

	// safety
	if (!iSourceWdt || !iSourceHgt) return false;

	// calc size
	int iTargetWdt = trect.right - trect.left, 
			iTargetHgt = trect.bottom - trect.top;

	if(bSectionMode)
	{
		iTargetWdt -= 20; iTargetHgt -= 20;
	}

	if(iSourceWdt * iTargetHgt > iSourceHgt * iTargetWdt)
		iTargetHgt = iSourceHgt * iTargetWdt / iSourceWdt;
	else
		iTargetWdt = iSourceWdt * iTargetHgt / iSourceHgt;

	// max zoom
	const int MAX_ZOOM = 3;
	if(iTargetHgt > iSourceHgt * MAX_ZOOM)
	{
		iTargetWdt = iTargetWdt * iSourceHgt * MAX_ZOOM / iTargetHgt;
		iTargetHgt = iSourceHgt * MAX_ZOOM;
	}
	if(iTargetWdt > iSourceWdt * MAX_ZOOM)
	{
		iTargetHgt = iTargetHgt * iSourceWdt * MAX_ZOOM / iTargetWdt;
		iTargetWdt = iSourceWdt * MAX_ZOOM;
	}

	// calc position
	int iTargetX = (trect.left + trect.right) / 2 - iTargetWdt / 2,
		  iTargetY = (trect.top + trect.bottom) / 2 - iTargetHgt / 2;

	// create bitmapinfo for image
	BITMAPINFO bmInfo;
	ZeroMem(&bmInfo, sizeof bmInfo);
	bmInfo.bmiHeader.biSize = sizeof bmInfo.bmiHeader;
	bmInfo.bmiHeader.biWidth = iWidth;
	bmInfo.bmiHeader.biHeight = iHeight;
	bmInfo.bmiHeader.biSizeImage = iPitch * iHeight;
	bmInfo.bmiHeader.biBitCount = 24;
	bmInfo.bmiHeader.biCompression = BI_RGB;
	bmInfo.bmiHeader.biPlanes = 1;

	// Mask?
	if(pMask)
	{

		// draw "shadow"
		if(bSectionMode)
			StretchDIBits(hDC, iTargetX + 3, iTargetY + iTargetHgt - 1 + 3,
												 iTargetWdt, -iTargetHgt,
												 iSourceX, iSourceY,
												 iSourceWdt, iSourceHgt,
												 pMask, &bmInfo, DIB_RGB_COLORS, SRCINVERT);

		// copy mask
		StretchDIBits(hDC, iTargetX, iTargetY + iTargetHgt - 1,
			                 iTargetWdt, -iTargetHgt,
											 iSourceX, iSourceY,
											 iSourceWdt, iSourceHgt,
											 pMask, &bmInfo, DIB_RGB_COLORS, SRCPAINT);
		// copy image
		StretchDIBits(hDC, iTargetX, iTargetY + iTargetHgt - 1,
			                 iTargetWdt, -iTargetHgt,
											 iSourceX, iSourceY,
											 iSourceWdt, iSourceHgt,
											 pImageData, &bmInfo, DIB_RGB_COLORS, SRCAND);

	}
	else
	{
		// copy image
		StretchDIBits(hDC, iTargetX, iTargetY + iTargetHgt - 1,
			                 iTargetWdt, -iTargetHgt,
											 iSourceX, iSourceY,
											 iSourceWdt, iSourceHgt,
											 pImageData, &bmInfo, DIB_RGB_COLORS, SRCCOPY);
	}

	// Draw color			Warning: colorization does currently not handle source facet coordinates - full pictures only!
	if (pColorDw)
		{
		DWORD clrColorDw = *pColorDw;

		// Create color mask if necessary
		if (!pColorMask)
			CreateColorMask();

		// create mask
		CSize clrSize(iWidth, iHeight);
		int clrPitch = clrSize.cx * 3 + (3 - (clrSize.cx * 3 - 1) % 4);
		BYTE *pDWMask = new BYTE [clrSize.cy * clrPitch];

		// create BMI for mask
		BITMAPINFO clrInfo;
		ZeroMem(&clrInfo, sizeof clrInfo);
		clrInfo.bmiHeader.biSize = sizeof clrInfo.bmiHeader;
		clrInfo.bmiHeader.biWidth = clrSize.cx;
		clrInfo.bmiHeader.biHeight = clrSize.cy;
		clrInfo.bmiHeader.biSizeImage = clrPitch * clrSize.cy;
		clrInfo.bmiHeader.biBitCount = 24;
		clrInfo.bmiHeader.biCompression = BI_RGB;
		clrInfo.bmiHeader.biPlanes = 1;

		int x, y;
		for (y = 0; y < clrSize.cy; y++)
			for (x = 0; x < clrSize.cx; x++)
				if (pColorMask[y * clrSize.cx + x])
				{
					*(pDWMask + x * 3 + y * clrPitch + 0) = 0;
					*(pDWMask + x * 3 + y * clrPitch + 1) = 0;
					*(pDWMask + x * 3 + y * clrPitch + 2) = 0;
				}
				else
				{
					*(pDWMask + x * 3 + y * clrPitch + 0) = 255;
					*(pDWMask + x * 3 + y * clrPitch + 1) = 255;
					*(pDWMask + x * 3 + y * clrPitch + 2) = 255;
				}

		// draw mask (1)
		StretchDIBits(hDC, iTargetX, iTargetY + iTargetHgt - 1,
			                 iTargetWdt, -iTargetHgt,
											 0, 0,
											 clrSize.cx, clrSize.cy,
											 pDWMask, &clrInfo, DIB_RGB_COLORS, SRCAND);


		for (y = 0; y < clrSize.cy; y++)
			for (x = 0; x < clrSize.cx; x++)
			{
				float iClr = (float) pColorMask[y * clrSize.cx + x] / 256;
				*(pDWMask + x * 3 + y * clrPitch + 0) = (BYTE) (((clrColorDw >> 0) % 256) * iClr);
				*(pDWMask + x * 3 + y * clrPitch + 1) = (BYTE) (((clrColorDw >> 8) % 256) * iClr);
				*(pDWMask + x * 3 + y * clrPitch + 2) = (BYTE) (((clrColorDw >> 16) % 256) * iClr);
			}

		// draw mask (2)
		StretchDIBits(hDC, iTargetX, iTargetY + iTargetHgt - 1,
			                 iTargetWdt, -iTargetHgt,
											 0, 0,
											 clrSize.cx, clrSize.cy,
											 pDWMask, &clrInfo, DIB_RGB_COLORS, SRCPAINT);
		
		// delete mask
		delete[] pDWMask;

		}

	return true;
}

bool C4PNG::Clear(bool bDeleteData)
{
	if (bDeleteData)
	{
		delete [] pImageData;
		delete [] pMask;
		delete [] pColorMask;
	}
	pImageData = NULL;
	pMask = NULL;
	pColorMask = NULL;
	return true;
}

void C4PNG::SetSection(int x, int y, int wdt, int hgt)
{
	if(!pImageData) return;
	iSctX = x;
	iSctY = y;
	iSctWdt = wdt;
	iSctHgt = hgt;
}

void C4PNG::CreateColorMask()
{
	pColorMask = new BYTE [iWidth * iHeight];

	for (int y = 0; y < iHeight; y++)
		for (int x = 0; x < iWidth; x++)
		{
			DWORD dwPix = RGB2BGR( *((DWORD*)(pImageData + y * iPitch + x * 3)) );
			if (IsBlueColor(dwPix))
				pColorMask[y * iWidth + x] = GetBValue(dwPix);
			else
				pColorMask[y * iWidth + x] = 0;
		}

}

bool C4PNG::HasData()
{
	return (pImageData != NULL);
}
