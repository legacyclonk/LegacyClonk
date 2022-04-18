/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
 * Copyright (c) 2017-2021, The LegacyClonk Team and contributors
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

// a wrapper class to DirectDraw surfaces

#include <Standard.h>
#include <StdSurface8.h>
#include <Bitmap256.h>
#include <StdPNG.h>
#include <StdDDraw2.h>
#include <CStdFile.h>
#include <Bitmap256.h>

#include <utility>

#include "limits.h"

CSurface8::CSurface8()
{
	Wdt = Hgt = Pitch = 0;
	ClipX = ClipY = ClipX2 = ClipY2 = 0;
	Bits = nullptr;
	pPal = nullptr;
}

CSurface8::CSurface8(int iWdt, int iHgt)
{
	Wdt = Hgt = Pitch = 0;
	ClipX = ClipY = ClipX2 = ClipY2 = 0;
	Bits = nullptr;
	pPal = nullptr;
	Create(iWdt, iHgt);
}

CSurface8::~CSurface8()
{
	Clear();
}

void CSurface8::Clear()
{
	// clear bitmap-copy
	delete[] Bits; Bits = nullptr;
	// clear pal
	if (HasOwnPal()) delete pPal;
	pPal = nullptr;
}

bool CSurface8::HasOwnPal()
{
	return pPal != lpDDrawPal;
}

void CSurface8::Box(int iX, int iY, int iX2, int iY2, int iCol)
{
	for (int cy = iY; cy <= iY2; cy++) HLine(iX, iX2, cy, iCol);
}

void CSurface8::NoClip()
{
	ClipX = 0; ClipY = 0; ClipX2 = Wdt - 1; ClipY2 = Hgt - 1;
}

void CSurface8::Clip(int iX, int iY, int iX2, int iY2)
{
	ClipX  = BoundBy(iX,  0, Wdt - 1); ClipY  = BoundBy(iY,  0, Hgt - 1);
	ClipX2 = BoundBy(iX2, 0, Wdt - 1); ClipY2 = BoundBy(iY2, 0, Hgt - 1);
}

void CSurface8::HLine(int iX, int iX2, int iY, int iCol)
{
	for (int cx = iX; cx <= iX2; cx++) SetPix(cx, iY, iCol);
}

bool CSurface8::Create(int iWdt, int iHgt, bool fOwnPal)
{
	Clear();
	// check size
	if (!iWdt || !iHgt) return false;
	Wdt = iWdt; Hgt = iHgt;

	// create/link to pal
	if (fOwnPal)
	{
		pPal = new CStdPalette;
		memcpy(pPal, &lpDDraw->Pal, sizeof(CStdPalette));
	}
	else
		pPal = &lpDDraw->Pal;

	Bits = new uint8_t[Wdt * Hgt]{};
	Pitch = Wdt;
	// update clipping
	NoClip();
	return true;
}

bool CSurface8::Read(CStdStream &hGroup, bool fOwnPal)
{
	int cnt, lcnt, iLineRest;
	CBitmap256Info BitmapInfo;
	// read bmpinfo-header
	if (!hGroup.Read(&BitmapInfo, sizeof(CBitmapInfo))) return false;
	// is it 8bpp?
	if (BitmapInfo.Info.biBitCount == 8)
	{
		if (!hGroup.Read((reinterpret_cast<uint8_t *>(&BitmapInfo)) + sizeof(CBitmapInfo), sizeof(BitmapInfo) - sizeof(CBitmapInfo))) return false;
		if (!hGroup.Advance(BitmapInfo.FileBitsOffset())) return false;
	}
	else
	{
		// read 24bpp
		if (BitmapInfo.Info.biBitCount != 24) return false;
		if (!hGroup.Advance((static_cast<CBitmapInfo>(BitmapInfo)).FileBitsOffset())) return false;
	}

	// Create and lock surface
	if (!Create(BitmapInfo.Info.biWidth, BitmapInfo.Info.biHeight, fOwnPal)) return false;

	if (BitmapInfo.Info.biBitCount == 8)
	{
		if (HasOwnPal())
		{
			// Copy palette
			for (cnt = 0; cnt < 256; cnt++)
			{
				pPal->Colors[cnt * 3 + 0] = BitmapInfo.Colors[cnt].rgbRed;
				pPal->Colors[cnt * 3 + 1] = BitmapInfo.Colors[cnt].rgbGreen;
				pPal->Colors[cnt * 3 + 2] = BitmapInfo.Colors[cnt].rgbBlue;
				pPal->Alpha[cnt] = 0;
			}
		}
	}

	// create line buffer
	int iBufSize = DWordAligned(BitmapInfo.Info.biWidth * BitmapInfo.Info.biBitCount / 8);
	uint8_t *pBuf = new uint8_t[iBufSize];
	// Read lines
	iLineRest = DWordAligned(BitmapInfo.Info.biWidth) - BitmapInfo.Info.biWidth;
	for (lcnt = Hgt - 1; lcnt >= 0; lcnt--)
	{
		if (!hGroup.Read(pBuf, iBufSize))
		{
			Clear(); delete[] pBuf; return false;
		}
		uint8_t *pPix = pBuf;
		for (int x = 0; x < BitmapInfo.Info.biWidth; ++x)
			switch (BitmapInfo.Info.biBitCount)
			{
			case 8:
				SetPix(x, lcnt, *pPix++);
				break;
			case 24:
				return false;
				break;
			}
	}
	// free buffer again
	delete[] pBuf;

	return true;
}

bool CSurface8::Save(const char *szFilename, uint8_t *bpPalette)
{
	CBitmap256Info BitmapInfo;
	BitmapInfo.Set(Wdt, Hgt, bpPalette ? bpPalette : pPal->Colors);

	// Create file & write info
	CStdFile hFile;

	if (!hFile.Create(szFilename)
		|| !hFile.Write(&BitmapInfo, sizeof(BitmapInfo)))
	{
		return false;
	}

	// Write lines
	char bpEmpty[4]{}; int iEmpty = DWordAligned(Wdt) - Wdt;
	for (int cnt = Hgt - 1; cnt >= 0; cnt--)
	{
		if (!hFile.Write(Bits + (Pitch * cnt), Wdt))
		{
			return false;
		}
		if (iEmpty)
			if (!hFile.Write(bpEmpty, iEmpty))
			{
				return false;
			}
	}

	// Close file
	hFile.Close();

	// Success
	return true;
}

void CSurface8::GetSurfaceSize(int &irX, int &irY)
{
	// simply assign stored values
	irX = Wdt;
	irY = Hgt;
}

void CSurface8::ClearBox8Only(int iX, int iY, int iWdt, int iHgt)
{
	// clear rect; assume clip already
	for (int y = iY; y < iY + iHgt; ++y)
		for (int x = iX; x < iX + iWdt; ++x)
			Bits[y * Pitch + x] = 0;
	// done
}

void CSurface8::Circle(int x, int y, int r, uint8_t col)
{
	for (int ycnt = -r; ycnt < r; ycnt++)
	{
		int lwdt = static_cast<int>(sqrt(float(r * r - ycnt * ycnt)));
		for (int xcnt = 2 * lwdt - 1; xcnt >= 0; xcnt--)
			SetPix(x - lwdt + xcnt, y + ycnt, col);
	}
}

/* Polygon drawing code extracted from ALLEGRO by Shawn Hargreaves */

struct CPolyEdge // An edge for the polygon drawer
{
	int y; // Current (starting at the top) y position
	int bottom; // bottom y position of this edge
	int x; // Fixed point x position
	int dx; // Fixed point x gradient
	int w; // Width of line segment
	struct CPolyEdge *prev; // Doubly linked list
	struct CPolyEdge *next;
};

#define POLYGON_FIX_SHIFT 16

static void fill_edge_structure(CPolyEdge *edge, int *i1, int *i2)
{
	if (i2[1] < i1[1]) std::swap(i1, i2);
	edge->y = i1[1];
	edge->bottom = i2[1] - 1;
	edge->dx = ((i2[0] - i1[0]) << POLYGON_FIX_SHIFT) / (i2[1] - i1[1]);
	edge->x = (i1[0] << POLYGON_FIX_SHIFT) + (1 << (POLYGON_FIX_SHIFT - 1)) - 1;
	edge->prev = nullptr;
	edge->next = nullptr;
	if (edge->dx < 0)
		edge->x += std::min<int>(edge->dx + (1 << POLYGON_FIX_SHIFT), 0);
	edge->w = std::max<int>(Abs(edge->dx) - (1 << POLYGON_FIX_SHIFT), 0);
}

static CPolyEdge *add_edge(CPolyEdge *list, CPolyEdge *edge, int sort_by_x)
{
	CPolyEdge *pos = list;
	CPolyEdge *prev = nullptr;
	if (sort_by_x)
	{
		while ((pos) && (pos->x + pos->w / 2 < edge->x + edge->w / 2))
		{
			prev = pos; pos = pos->next;
		}
	}
	else
	{
		while ((pos) && (pos->y < edge->y))
		{
			prev = pos; pos = pos->next;
		}
	}
	edge->next = pos;
	edge->prev = prev;
	if (pos) pos->prev = edge;
	if (prev) { prev->next = edge; return list; }
	else return edge;
}

static CPolyEdge *remove_edge(CPolyEdge *list, CPolyEdge *edge)
{
	if (edge->next) edge->next->prev = edge->prev;
	if (edge->prev) { edge->prev->next = edge->next; return list; }
	else return edge->next;
}

// Global polygon quick buffer
const int QuickPolyBufSize = 20;
CPolyEdge QuickPolyBuf[QuickPolyBufSize];

void CSurface8::Polygon(int iNum, int *ipVtx, int iCol)
{
	// Variables for polygon drawer
	int c, x1, x2, y;
	int top = INT_MAX;
	int bottom = INT_MIN;
	int *i1, *i2;
	CPolyEdge *edge, *next_edge, *edgebuf;
	CPolyEdge *active_edges = nullptr;
	CPolyEdge *inactive_edges = nullptr;
	bool use_qpb = false;

	// Poly Buf
	if (iNum <= QuickPolyBufSize)
	{
		edgebuf = QuickPolyBuf; use_qpb = true;
	}
	else
	{
		edgebuf = new CPolyEdge[iNum];
	}

	// Fill the edge table
	edge = edgebuf;
	i1 = ipVtx;
	i2 = ipVtx + (iNum - 1) * 2;
	for (c = 0; c < iNum; c++)
	{
		if (i1[1] != i2[1])
		{
			fill_edge_structure(edge, i1, i2);
			if (edge->bottom >= edge->y)
			{
				if (edge->y < top) top = edge->y;
				if (edge->bottom > bottom) bottom = edge->bottom;
				inactive_edges = add_edge(inactive_edges, edge, false);
				edge++;
			}
		}
		i2 = i1; i1 += 2;
	}

	// For each scanline in the polygon...
	for (c = top; c <= bottom; c++)
	{
		// Check for newly active edges
		edge = inactive_edges;
		while ((edge) && (edge->y == c))
		{
			next_edge = edge->next;
			inactive_edges = remove_edge(inactive_edges, edge);
			active_edges = add_edge(active_edges, edge, true);
			edge = next_edge;
		}

		// Draw horizontal line segments
		edge = active_edges;
		while ((edge) && (edge->next))
		{
			x1 = edge->x >> POLYGON_FIX_SHIFT;
			x2 = (edge->next->x + edge->next->w) >> POLYGON_FIX_SHIFT;
			y = c;
			// Fix coordinates
			if (x1 > x2) std::swap(x1, x2);
			// Set line
			for (int xcnt = x2 - x1; xcnt >= 0; xcnt--) SetPix(x1 + xcnt, y, iCol);
			edge = edge->next->next;
		}

		// Update edges, sorting and removing dead ones
		edge = active_edges;
		while (edge)
		{
			next_edge = edge->next;
			if (c >= edge->bottom)
			{
				active_edges = remove_edge(active_edges, edge);
			}
			else
			{
				edge->x += edge->dx;
				while ((edge->prev) && (edge->x + edge->w / 2 < edge->prev->x + edge->prev->w / 2))
				{
					if (edge->next) edge->next->prev = edge->prev;
					edge->prev->next = edge->next;
					edge->next = edge->prev;
					edge->prev = edge->prev->prev;
					edge->next->prev = edge;
					if (edge->prev) edge->prev->next = edge;
					else active_edges = edge;
				}
			}
			edge = next_edge;
		}
	}

	// Clear scratch memory
	if (!use_qpb) delete[] edgebuf;
}

void CSurface8::AllowColor(uint8_t iRngLo, uint8_t iRngHi, bool fAllowZero)
{
	// change colors
	int xcnt, ycnt;
	if (iRngHi < iRngLo) return;
	for (ycnt = 0; ycnt < Hgt; ycnt++)
	{
		for (xcnt = 0; xcnt < Wdt; xcnt++)
		{
			uint8_t px = GetPix(xcnt, ycnt);
			if (px || !fAllowZero)
				if ((px < iRngLo) || (px > iRngHi))
					SetPix(xcnt, ycnt, iRngLo + px % (iRngHi - iRngLo + 1));
		}
	}
}
