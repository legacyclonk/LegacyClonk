
/* used to load/decompress PNGs and hold image data */

#ifndef C4PNG_H

#define C4PNG_H

#include "StdPNG.h"

class C4PNG
{
public:
	bool HasData();
	C4PNG();
	C4PNG(const C4PNG &PNG2);
	virtual ~C4PNG();

	bool Load(C4Group *pParentGroup, CString strEntryName);
	bool Draw(HDC hDC, RECT trect, DWORD *pColorDw = 0);
	bool Clear(bool bDeleteData);
	void SetSection(int x, int y, int wdt, int hgt);

	int iWidth, iHeight, iPitch, iMaskPitch;

	BYTE *pImageData;
	BYTE *pMask;
	BYTE *pColorMask;

	int iSctX, iSctY, iSctWdt, iSctHgt;
	bool bSectionMode;
protected:
	void CreateColorMask();
};

#endif