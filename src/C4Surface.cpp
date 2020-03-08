/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
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

/* Extension to CSurface that handles bitmaps in C4Group files */

#include <C4Include.h>
#include <C4Surface.h>
#include <C4GroupSet.h>

#include <C4Group.h>
#include <C4Log.h>

#include <Bitmap256.h>
#include <StdBitmap.h>
#include <StdJpeg.h>
#include <StdPNG.h>
#include <StdDDraw2.h>

#include <cstdint>
#include <memory>
#include <stdexcept>

bool C4Surface::LoadAny(C4Group &hGroup, const char *szName, bool fOwnPal, bool fNoErrIfNotFound)
{
	// Entry name
	char szFilename[_MAX_FNAME + 1];
	SCopy(szName, szFilename, _MAX_FNAME);
	char *szExt = GetExtension(szFilename);
	if (!*szExt)
	{
		// no extension: Default to extension that is found as file in group
		const char *const extensions[] = { "png", "bmp", "jpeg", "jpg", nullptr };
		int i = 0; const char *szExt;
		while (szExt = extensions[i++])
		{
			EnforceExtension(szFilename, szExt);
			if (hGroup.FindEntry(szFilename)) break;
		}
	}
	// Load surface
	return Load(hGroup, szFilename, fOwnPal, fNoErrIfNotFound);
}

bool C4Surface::LoadAny(C4GroupSet &hGroupset, const char *szName, bool fOwnPal, bool fNoErrIfNotFound)
{
	// Entry name
	char szFilename[_MAX_FNAME + 1];
	SCopy(szName, szFilename, _MAX_FNAME);
	char *szExt = GetExtension(szFilename);
	C4Group *pGroup;
	if (!*szExt)
	{
		// no extension: Default to extension that is found as file in group
		const char *const extensions[] = { "png", "bmp", "jpeg", "jpg", nullptr };
		int i = 0; const char *szExt;
		while (szExt = extensions[i++])
		{
			EnforceExtension(szFilename, szExt);
			pGroup = hGroupset.FindEntry(szFilename);
			if (pGroup) break;
		}
	}
	if (!pGroup) return false;
	// Load surface
	return Load(*pGroup, szFilename, fOwnPal, fNoErrIfNotFound);
}

bool C4Surface::Load(C4Group &hGroup, const char *szFilename, bool fOwnPal, bool fNoErrIfNotFound)
{
	if (!hGroup.AccessEntry(szFilename))
	{
		// file not found
		if (!fNoErrIfNotFound) LogF("%s: %s%c%s", LoadResStr("IDS_PRC_FILENOTFOUND"), hGroup.GetFullName().getData(), (char)DirectorySeparator, szFilename);
		return false;
	}
	// determine file type by file extension and load accordingly
	bool fSuccess;
	if (SEqualNoCase(GetExtension(szFilename), "png"))
		fSuccess = !!ReadPNG(hGroup);
	else if (SEqualNoCase(GetExtension(szFilename), "jpeg")
		|| SEqualNoCase(GetExtension(szFilename), "jpg"))
		fSuccess = ReadJPEG(hGroup);
	else
		fSuccess = !!Read(hGroup, fOwnPal);
	// loading error? log!
	if (!fSuccess)
		LogF("%s: %s%c%s", LoadResStr("IDS_ERR_NOFILE"), hGroup.GetFullName().getData(), (char)DirectorySeparator, szFilename);
	// done, success
	return fSuccess;
}

bool C4Surface::ReadPNG(CStdStream &hGroup)
{
	// create mem block
	int iSize = hGroup.AccessedEntrySize();
	std::unique_ptr<uint8_t[]> pData(new uint8_t[iSize]);
	// load file into mem
	hGroup.Read(pData.get(), iSize);
	// load as png file
	std::unique_ptr<StdBitmap> bmp;
	std::uint32_t width, height; bool useAlpha;
	try
	{
		CPNGFile png(pData.get(), iSize);
		width = png.Width(); height = png.Height(), useAlpha = png.UsesAlpha();
		bmp.reset(new StdBitmap(width, height, useAlpha));
		png.Decode(bmp->GetBytes());
	}
	catch (const std::runtime_error &e)
	{
		LogF("Could not create surface from PNG file: %s", e.what());
		bmp.reset();
	}
	// free file data
	pData.reset();
	// abort if loading wasn't successful
	if (!bmp) return false;
	// create surface(s) - do not create an 8bit-buffer!
	if (!Create(width, height)) return false;
	// lock for writing data
	if (!Lock()) return false;
	if (Textures.empty())
	{
		Unlock();
		return false;
	}
	// write pixels
	for (const auto &texture : Textures)
	{
		if (!texture->Lock()) continue;
		for (uint32_t y = 0; y < texture->Height; ++y)
		{
#if 0
#ifndef __BIG_ENDIAN__
			if (useAlpha)
			{
				// Optimize the easy case of a png in the same format as the display
				// 32 bit
				auto *pix = reinterpret_cast<uint32_t *>(texture->texLock.pBits + y + texture->texLock.Pitch);
				memcpy(pix, static_cast<uint32_t *>(bmp->GetPixelAddr32(0, y)) + texture->X, texture->Width * 4);
				for (uint32_t x = 0; x < texture->Width; ++x)
				{
					// if color is fully transparent, ensure it's black
					if ((*pix & 0xff) == 0xff)
					{
						*pix = 0xff000000;
					}

					++pix;
				}
			}
			else
#endif
#endif
			{
				// Loop through every pixel and convert
				for (uint32_t x = 0; x < texture->Width; ++x)
				{
					uint32_t dwCol = bmp->GetPixel(texture->X + x, texture->Y + y);
					// if color is fully transparent, ensure it's black
					if (dwCol >> 24 == 0xff) dwCol = 0xff000000;
					// set pix in surface
					texture->SetPix(x, y, dwCol);
				}
			}
		}

		texture->Unlock();
	}

	// unlock
	Unlock();
	// Success
	return true;
}

bool C4Surface::SavePNG(C4Group &hGroup, const char *szFilename, bool fSaveAlpha, bool fApplyGamma, bool fSaveOverlayOnly)
{
	// Using temporary file at C4Group temp path
	char szTemp[_MAX_PATH + 1];
	SCopy(C4Group_GetTempPath(), szTemp);
	SAppend(GetFilename(szFilename), szTemp);
	MakeTempFilename(szTemp);
	// Save to temporary file
	if (!CSurface::SavePNG(szTemp, fSaveAlpha, fApplyGamma, fSaveOverlayOnly)) return false;
	// Move temp file to group
	if (!hGroup.Move(szTemp, GetFilename(szFilename))) return false;
	// Success
	return true;
}

bool C4Surface::Copy(C4Surface &fromSfc)
{
	// Clear anything old
	Clear();
	// Default to other surface's color depth
	Default();
	// Create surface
	if (!Create(fromSfc.Wdt, fromSfc.Hgt)) return false;
	// Blit copy
	if (!lpDDraw->BlitSurface(&fromSfc, this, 0, 0, false))
	{
		Clear(); return false;
	}
	// Success
	return true;
}

bool C4Surface::ReadJPEG(CStdStream &hGroup)
{
	// create mem block
	size_t size = hGroup.AccessedEntrySize();
	unsigned char *pData = new unsigned char[size];
	// load file into mem
	hGroup.Read(pData, size);

	bool locked = false;
	try
	{
		StdJpeg jpeg(pData, size);
		const std::uint32_t width = jpeg.Width(), height = jpeg.Height();

		// create surface(s) - do not create an 8bit-buffer!
		if (!Create(width, height)) return false;

		// lock for writing data
		if (!Lock()) return false;
		locked = true;

		// put the data in the image
		for (std::uint32_t y = 0; y < height; ++y)
		{
			const auto row = jpeg.DecodeRow();
			for (std::uint32_t x = 0; x < width; ++x)
			{
				const auto pixel = static_cast<const uint8_t *>(row) + x * 3;
				SetPixDw(x, y, C4RGB(pixel[0], pixel[1], pixel[2]));
			}
		}
		jpeg.Finish();
	}
	catch (const std::runtime_error &e)
	{
		LogF("Could not create surface from JPEG file: %s", e.what());
	}

	// unlock
	if (locked) Unlock();
	// free data
	delete[] pData;
	// return if successful
	return true;
}
