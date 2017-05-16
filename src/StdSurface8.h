/* by Sven2, 2001 */
// a wrapper class to DirectDraw surfaces

#pragma once

#include <Standard.h>
#include <StdColors.h>

class CSurface8
{
public:
	CSurface8();
	~CSurface8();
	CSurface8(int iWdt, int iHgt); // create new surface and init it

public:
	int Wdt, Hgt, Pitch; // size of surface
	int ClipX, ClipY, ClipX2, ClipY2;
	uint8_t *Bits;
	CStdPalette *pPal; // pal for this surface (usually points to the main pal)
	bool HasOwnPal(); // return whether the surface palette is owned
	void HLine(int iX, int iX2, int iY, int iCol);
	void Polygon(int iNum, int *ipVtx, int iCol);
	void Box(int iX, int iY, int iX2, int iY2, int iCol);
	void Circle(int x, int y, int r, uint8_t col);
	void ClearBox8Only(int iX, int iY, int iWdt, int iHgt); // clear box in 8bpp-surface only

	void SetPix(int iX, int iY, uint8_t byCol)
	{
		// clip
		if ((iX < ClipX) || (iX > ClipX2) || (iY < ClipY) || (iY > ClipY2)) return;
		// set pix in local copy...
		if (Bits) Bits[iY * Pitch + iX] = byCol;
	}

	uint8_t GetPix(int iX, int iY) // get pixel
	{
		if (iX < 0 || iY < 0 || iX >= Wdt || iY >= Hgt) return 0;
		return Bits ? Bits[iY * Pitch + iX] : 0;
	}

	inline uint8_t _GetPix(int x, int y) // get pixel (bounds not checked)
	{
		return Bits[y * Pitch + x];
	}

	bool Create(int iWdt, int iHgt, bool fOwnPal = false);
	void Clear();
	void Clip(int iX, int iY, int iX2, int iY2);
	void NoClip();
	bool Read(class CStdStream &hGroup, bool fOwnPal);
	bool Save(const char *szFilename, uint8_t *bpPalette = nullptr);
	void GetSurfaceSize(int &irX, int &irY); // get surface size
	void EnforceC0Transparency() { pPal->EnforceC0Transparency(); }
	void AllowColor(uint8_t iRngLo, uint8_t iRngHi, bool fAllowZero = false);
};
