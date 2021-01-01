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

#pragma once

#include <C4Surface.h>

class C4Texture
{
	friend class C4TextureMap;

public:
	C4Texture();
	~C4Texture();
	C4Surface *Surface32;
	CSurface8 *Surface8;

protected:
	char Name[C4M_MaxName + 1];
	C4Texture *Next;
};

class C4TexMapEntry
{
	friend class C4TextureMap;

public:
	C4TexMapEntry();

private:
	StdStrBuf Material, Texture;
	int32_t iMaterialIndex;
	C4Material *pMaterial;
	CPattern MatPattern;

public:
	bool isNull() const { return Material.isNull(); }
	const char *GetMaterialName() const { return Material.getData(); }
	const char *GetTextureName() const { return Texture.getData(); }
	int32_t GetMaterialIndex() const { return iMaterialIndex; }
	C4Material *GetMaterial() const { return pMaterial; }
	const CPattern &getPattern() const { return MatPattern; }
	void Clear();
	bool Create(const char *szMaterial, const char *szTexture);
	bool Init();
};

class C4TextureMap
{
public:
	C4TextureMap();
	~C4TextureMap();

protected:
	C4TexMapEntry Entry[C4M_MaxTexIndex];
	C4Texture *FirstTexture;
	bool fOverloadMaterials;
	bool fOverloadTextures;
	bool fInitialized; // Set after Init() - newly added entries initialized automatically

public:
	bool fEntriesAdded;

public:
	const C4TexMapEntry *GetEntry(int32_t iIndex) const { return Inside<int32_t>(iIndex, 0, C4M_MaxTexIndex - 1) ? &Entry[iIndex] : nullptr; }
	void RemoveEntry(int32_t iIndex) { if (Inside<int32_t>(iIndex, 1, C4M_MaxTexIndex - 1)) Entry[iIndex].Clear(); }
	void Default();
	void Clear();
	void StoreMapPalette(uint8_t *bypPalette, C4MaterialMap &rMaterials);
	static bool LoadFlags(C4Group &hGroup, const char *szEntryName, bool *pOverloadMaterials, bool *pOverloadTextures);
	int32_t LoadMap(C4Group &hGroup, const char *szEntryName, bool *pOverloadMaterials, bool *pOverloadTextures);
	int32_t Init();
	bool SaveMap(C4Group &hGroup, const char *szEntryName);
	int32_t LoadTextures(C4Group &hGroup, C4Group *OverloadFile = nullptr);
	const char *GetTexture(size_t iIndex);
	void MoveIndex(uint8_t byOldIndex, uint8_t byNewIndex); // change index of texture
	int32_t GetIndex(const char *szMaterial, const char *szTexture, bool fAddIfNotExist = true, const char *szErrorIfFailed = nullptr);
	int32_t GetIndexMatTex(const char *szMaterialTexture, const char *szDefaultTexture = nullptr, bool fAddIfNotExist = true, const char *szErrorIfFailed = nullptr);
	C4Texture *GetTexture(const char *szTexture);
	bool CheckTexture(const char *szTexture); // return whether texture exists
	bool AddEntry(uint8_t byIndex, const char *szMaterial, const char *szTexture);

protected:
	bool AddTexture(const char *szTexture, C4Surface *sfcSurface);
	bool AddTexture(const char *szTexture, CSurface8 *sfcSurface);
};
