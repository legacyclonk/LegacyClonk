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

/* Textures used by the landscape */

#include <C4Include.h>
#include <C4Texture.h>

#include <C4SurfaceFile.h>
#include <C4Group.h>
#include <C4Game.h>
#include <C4Config.h>
#include <C4Components.h>
#include <C4Application.h>
#include <C4Material.h>
#include <C4Landscape.h>
#include <C4Wrappers.h>

C4Texture::C4Texture()
{
	Name[0] = 0;
	Surface8 = nullptr;
	Surface32 = nullptr;
	Next = nullptr;
}

C4Texture::~C4Texture()
{
#ifdef C4ENGINE
	delete Surface8;
	delete Surface32;
#endif
}

C4TexMapEntry::C4TexMapEntry()
	: pMaterial(nullptr), iMaterialIndex(MNone) {}

void C4TexMapEntry::Clear()
{
	Material.Clear(); Texture.Clear();
	iMaterialIndex = MNone;
	pMaterial = nullptr;
	MatPattern.Clear();
}

bool C4TexMapEntry::Create(const char *szMaterial, const char *szTexture)
{
	// Clear previous data
	Clear();
	// Save names
	Material = szMaterial; Texture = szTexture;
	return true;
}

bool C4TexMapEntry::Init()
{
#ifdef C4ENGINE
	// Find material
	iMaterialIndex = Game.Material.Get(Material.getData());
	if (!MatValid(iMaterialIndex))
	{
		DebugLogF("Error initializing material %s-%s: Invalid material!", Material.getData(), Texture.getData());
		return false;
	}
	pMaterial = &Game.Material.Map[iMaterialIndex];
	// Special, hardcoded crap: change <liquid>-Smooth to <liquid>-Liquid
	const char *szTexture = Texture.getData();
	if (DensityLiquid(pMaterial->Density))
		if (SEqualNoCase(szTexture, "Smooth"))
			szTexture = "Liquid";
	// Find texture
	C4Texture *sfcTexture = Game.TextureMap.GetTexture(szTexture);
	if (!sfcTexture)
	{
		DebugLogF("Error initializing material %s-%s: Invalid texture!", Material.getData(), Texture.getData());
		Clear();
		return false;
	}
	// Get overlay properties
	int32_t iOverlayType = pMaterial->OverlayType;
	bool fMono = !!(iOverlayType & C4MatOv_Monochrome);
	int32_t iZoom = 0;
	if (iOverlayType & C4MatOv_Exact) iZoom = 1;
	if (iOverlayType & C4MatOv_HugeZoom) iZoom = 4;
	// Create pattern
	if (sfcTexture->Surface32)
		MatPattern.Set(sfcTexture->Surface32, iZoom, fMono);
	else
		MatPattern.Set(sfcTexture->Surface8, iZoom, fMono);
	MatPattern.SetColors(pMaterial->Color, pMaterial->Alpha);
#endif
	return true;
}

C4TextureMap::C4TextureMap()
{
	Default();
}

C4TextureMap::~C4TextureMap()
{
	Clear();
}

bool C4TextureMap::AddEntry(uint8_t byIndex, const char *szMaterial, const char *szTexture)
{
	// Security
	if (byIndex <= 0 || byIndex >= C4M_MaxTexIndex)
		return false;
	if (!szMaterial || !szTexture)
		return false;
	// Set entry and initialize
	Entry[byIndex].Create(szMaterial, szTexture);
	if (fInitialized)
	{
		if (!Entry[byIndex].Init())
		{
			// Clear entry if it could not be initialized
			Entry[byIndex].Clear();
			return false;
		}
#ifdef C4ENGINE
		// Landscape must be notified (new valid pixel clr)
		Game.Landscape.HandleTexMapUpdate();
#endif
	}
	return true;
}

bool C4TextureMap::AddTexture(const char *szTexture, C4Surface *sfcSurface)
{
	C4Texture *pTexture;
	if (!(pTexture = new C4Texture)) return false;
	SCopy(szTexture, pTexture->Name, C4M_MaxName);
	pTexture->Surface32 = sfcSurface;
	pTexture->Next = FirstTexture;
	FirstTexture = pTexture;
	return true;
}

bool C4TextureMap::AddTexture(const char *szTexture, CSurface8 *sfcSurface)
{
	C4Texture *pTexture;
	if (!(pTexture = new C4Texture)) return false;
	SCopy(szTexture, pTexture->Name, C4M_MaxName);
	pTexture->Surface8 = sfcSurface;
	pTexture->Next = FirstTexture;
	FirstTexture = pTexture;
	return true;
}

void C4TextureMap::Clear()
{
	for (int32_t i = 1; i < C4M_MaxTexIndex; i++)
		Entry[i].Clear();
	C4Texture *ctex, *next2;
	for (ctex = FirstTexture; ctex; ctex = next2)
	{
		next2 = ctex->Next;
		delete ctex;
	}
	FirstTexture = nullptr;
	fInitialized = false;
}

bool C4TextureMap::LoadFlags(C4Group &hGroup, const char *szEntryName, bool *pOverloadMaterials, bool *pOverloadTextures)
{
	// Load the file
	StdStrBuf TexMap;
	if (!hGroup.LoadEntryString(szEntryName, TexMap))
		return false;
	// Reset flags
	if (pOverloadMaterials) *pOverloadMaterials = false;
	if (pOverloadTextures) *pOverloadTextures = false;
	// Check if there are flags in there
	for (const char *pPos = TexMap.getData(); pPos && *pPos; pPos = SSearch(pPos, "\n"))
	{
		// Go over newlines
		while (*pPos == '\r' || *pPos == '\n') pPos++;
		// Flag?
		if (pOverloadMaterials && SEqual2(pPos, "OverloadMaterials"))
			*pOverloadMaterials = true;
		if (pOverloadTextures && SEqual2(pPos, "OverloadTextures"))
			*pOverloadTextures = true;
	}
	// Done
	return true;
}

int32_t C4TextureMap::LoadMap(C4Group &hGroup, const char *szEntryName, bool *pOverloadMaterials, bool *pOverloadTextures)
{
	char *bpMap;
	char szLine[100 + 1];
	int32_t cnt, iIndex, iTextures = 0;
	// Load text file into memory
	if (!hGroup.LoadEntry(szEntryName, &bpMap, nullptr, 1)) return 0;
	// Scan text buffer lines
	for (cnt = 0; SCopySegment(bpMap, cnt, szLine, 0x0A, 100); cnt++)
		if ((szLine[0] != '#') && (SCharCount('=', szLine) == 1))
		{
			SReplaceChar(szLine, 0x0D, 0x00);
			if (Inside<int32_t>(iIndex = strtol(szLine, nullptr, 10), 0, C4M_MaxTexIndex - 1))
			{
				const char *szMapping = szLine + SCharPos('=', szLine) + 1;
				StdStrBuf Material, Texture;
				Material.CopyUntil(szMapping, '-'); Texture.Copy(SSearch(szMapping, "-"));
				if (AddEntry(iIndex, Material.getData(), Texture.getData()))
					iTextures++;
			}
		}
		else
		{
			if (SEqual2(szLine, "OverloadMaterials")) { fOverloadMaterials = true; if (pOverloadMaterials) *pOverloadMaterials = true; }
			if (SEqual2(szLine, "OverloadTextures"))  { fOverloadTextures  = true; if (pOverloadTextures)  *pOverloadTextures  = true; }
		}
	// Delete buffer, return entry count
	delete[] bpMap;
	fEntriesAdded = false;
	return iTextures;
}

int32_t C4TextureMap::Init()
{
	int32_t iRemoved = 0;
	// Initialize texture mappings
	int32_t i;
	for (i = 0; i < C4M_MaxTexIndex; i++)
		if (!Entry[i].isNull())
			if (!Entry[i].Init())
			{
#ifdef C4ENGINE
				LogF("Error in TextureMap initialization at entry %d", static_cast<int>(i));
#endif
				Entry[i].Clear();
				iRemoved++;
			}
	fInitialized = true;
	return iRemoved;
}

bool C4TextureMap::SaveMap(C4Group &hGroup, const char *szEntryName)
{
#ifdef C4ENGINE
	// build file in memory
	StdStrBuf sTexMapFile;
	// add desc
	sTexMapFile.Append("# Automatically generated texture map" LineFeed);
	sTexMapFile.Append("# Contains material-texture-combinations added at runtime" LineFeed);
	// add overload-entries
	if (fOverloadMaterials) sTexMapFile.Append("# Import materials from global file as well" LineFeed "OverloadMaterials" LineFeed);
	if (fOverloadTextures) sTexMapFile.Append("# Import textures from global file as well" LineFeed "OverloadTextures" LineFeed);
	sTexMapFile.Append(LineFeed);
	// add entries
	for (int32_t i = 0; i < C4M_MaxTexIndex; i++)
		if (!Entry[i].isNull())
		{
			// compose line
			sTexMapFile.AppendFormat("%d=%s-%s" LineFeed, i, Entry[i].GetMaterialName(), Entry[i].GetTextureName());
		}
	// create new buffer allocated with new [], because C4Group cannot handle StdStrBuf-buffers
	size_t iBufSize = sTexMapFile.getLength();
	uint8_t *pBuf = new uint8_t[iBufSize];
	memcpy(pBuf, sTexMapFile.getData(), iBufSize);
	// add to group
	bool fSuccess = !!hGroup.Add(szEntryName, pBuf, iBufSize, false, true);
	if (!fSuccess) delete[] pBuf;
	// done
	return fSuccess;
#else
	return false;
#endif
}

int32_t C4TextureMap::LoadTextures(C4Group &hGroup, C4Group *OverloadFile)
{
	int32_t texnum = 0;

#ifdef C4ENGINE

	// overload: load from other file
	if (OverloadFile) texnum += LoadTextures(*OverloadFile);

	char texname[256 + 1];
	C4Surface *ctex;
	size_t binlen;
	// newgfx: load PNG-textures first
	hGroup.ResetSearch();
	while (hGroup.AccessNextEntry(C4CFN_PNGFiles, &binlen, texname))
	{
		// check if it already exists in the map
		SReplaceChar(texname, '.', 0);
		if (GetTexture(texname)) continue;
		SAppend(".png", texname);
		// load
		if (ctex = GroupReadSurfacePNG(hGroup))
		{
			SReplaceChar(texname, '.', 0);
			if (AddTexture(texname, ctex)) texnum++;
			else delete ctex;
		}
	}
	// Load all bitmap files from group
	hGroup.ResetSearch();
	CSurface8 *ctex8;
	while (hGroup.AccessNextEntry(C4CFN_BitmapFiles, &binlen, texname))
	{
		// check if it already exists in the map
		SReplaceChar(texname, '.', 0);
		if (GetTexture(texname)) continue;
		SAppend(".bmp", texname);
		if (ctex8 = GroupReadSurface8(hGroup))
		{
			ctex8->AllowColor(0, 2, true);
			SReplaceChar(texname, '.', 0);
			if (AddTexture(texname, ctex8)) texnum++;
			else delete ctex;
		}
	}

#endif

	return texnum;
}

void C4TextureMap::MoveIndex(uint8_t byOldIndex, uint8_t byNewIndex)
{
	Entry[byNewIndex] = Entry[byOldIndex];
	fEntriesAdded = true;
}

int32_t C4TextureMap::GetIndex(const char *szMaterial, const char *szTexture, bool fAddIfNotExist, const char *szErrorIfFailed)
{
	uint8_t byIndex;
	// Find existing
	for (byIndex = 1; byIndex < C4M_MaxTexIndex; byIndex++)
		if (!Entry[byIndex].isNull())
			if (SEqualNoCase(Entry[byIndex].GetMaterialName(), szMaterial))
				if (!szTexture || SEqualNoCase(Entry[byIndex].GetTextureName(), szTexture))
					return byIndex;
	// Add new entry
	if (fAddIfNotExist)
		for (byIndex = 1; byIndex < C4M_MaxTexIndex; byIndex++)
			if (Entry[byIndex].isNull())
			{
				if (AddEntry(byIndex, szMaterial, szTexture))
				{
					fEntriesAdded = true;
					return byIndex;
				}
#ifdef C4ENGINE
				if (szErrorIfFailed) DebugLogF("Error getting MatTex %s-%s for %s from TextureMap: Init failed.", szMaterial, szTexture, szErrorIfFailed);
#endif
				return 0;
			}
	// Else, fail
#ifdef C4ENGINE
	if (szErrorIfFailed) DebugLogF("Error getting MatTex %s-%s for %s from TextureMap: %s.", szMaterial, szTexture, szErrorIfFailed, fAddIfNotExist ? "Map is full!" : "Entry not found.");
#endif
	return 0;
}

int32_t C4TextureMap::GetIndexMatTex(const char *szMaterialTexture, const char *szDefaultTexture, bool fAddIfNotExist, const char *szErrorIfFailed)
{
	// split material/texture pair
	StdStrBuf Material, Texture;
	Material.CopyUntil(szMaterialTexture, '-');
	Texture.Copy(SSearch(szMaterialTexture, "-"));
	// texture not given or invalid?
	int32_t iMatTex = 0;
	if (Texture.getData())
		if (iMatTex = GetIndex(Material.getData(), Texture.getData(), fAddIfNotExist))
			return iMatTex;
	if (szDefaultTexture)
		if (iMatTex = GetIndex(Material.getData(), szDefaultTexture, fAddIfNotExist))
			return iMatTex;
#ifdef C4ENGINE
	// search material
	const auto iMaterial = Game.Material.Get(szMaterialTexture);
	if (!MatValid(iMaterial))
	{
		if (szErrorIfFailed) DebugLogF("Error getting MatTex for %s: Invalid material", szErrorIfFailed);
		return 0;
	}
	// return default map entry
	return Game.Material.Map[iMaterial].DefaultMatTex;
#else
	return 0;
#endif
}

C4Texture *C4TextureMap::GetTexture(const char *szTexture)
{
	C4Texture *pTexture;
	for (pTexture = FirstTexture; pTexture; pTexture = pTexture->Next)
		if (SEqualNoCase(pTexture->Name, szTexture))
			return pTexture;
	return nullptr;
}

bool C4TextureMap::CheckTexture(const char *szTexture)
{
#ifdef C4ENGINE
	C4Texture *pTexture;
	for (pTexture = FirstTexture; pTexture; pTexture = pTexture->Next)
		if (SEqualNoCase(pTexture->Name, szTexture))
			return true;
	return false;
#else
	return true;
#endif
}

const char *C4TextureMap::GetTexture(size_t iIndex)
{
	C4Texture *pTexture;
	size_t cindex;
	for (pTexture = FirstTexture, cindex = 0; pTexture; pTexture = pTexture->Next, cindex++)
		if (cindex == iIndex)
			return pTexture->Name;
	return nullptr;
}

void C4TextureMap::Default()
{
	FirstTexture = nullptr;
	fEntriesAdded = false;
	fOverloadMaterials = false;
	fOverloadTextures = false;
	fInitialized = false;
}

void C4TextureMap::StoreMapPalette(uint8_t *bypPalette, C4MaterialMap &rMaterial)
{
	// Zero palette
	std::fill_n(bypPalette, 256 * 3, 0);
	// Sky color
	bypPalette[0] = 192;
	bypPalette[1] = 196;
	bypPalette[2] = 252;
	// Material colors by texture map entries
	bool fSet[256]{};
	int32_t i;
	for (i = 0; i < C4M_MaxTexIndex; i++)
	{
		// Find material
		C4Material *pMat = Entry[i].GetMaterial();
		if (pMat)
		{
			bypPalette[3 * i + 0] = pMat->Color[6];
			bypPalette[3 * i + 1] = pMat->Color[7];
			bypPalette[3 * i + 2] = pMat->Color[8];
			bypPalette[3 * (i + IFT) + 0] = pMat->Color[3];
			bypPalette[3 * (i + IFT) + 1] = pMat->Color[4];
			bypPalette[3 * (i + IFT) + 2] = pMat->Color[5];
			fSet[i] = fSet[i + IFT] = true;
		}
	}
	// Crosscheck colors, change equal palette entries
	for (i = 0; i < 256; i++) if (fSet[i])
		for (;;)
		{
			// search equal entry
			int32_t j = 0;
			for (; j < i; j++) if (fSet[j])
				if (
					bypPalette[3 * i + 0] == bypPalette[3 * j + 0] &&
					bypPalette[3 * i + 1] == bypPalette[3 * j + 1] &&
					bypPalette[3 * i + 2] == bypPalette[3 * j + 2])
					break;
			// not found? ok then
			if (j >= i) break;
			// change randomly
			if (rand() < RAND_MAX / 2) bypPalette[3 * i + 0] += 3; else bypPalette[3 * i + 0] -= 3;
			if (rand() < RAND_MAX / 2) bypPalette[3 * i + 1] += 3; else bypPalette[3 * i + 1] -= 3;
			if (rand() < RAND_MAX / 2) bypPalette[3 * i + 2] += 3; else bypPalette[3 * i + 2] -= 3;
		}
}
