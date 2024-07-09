/*
 * LegacyClonk
 *
 * Copyright (c) 2023, The LegacyClonk Team and contributors
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

#pragma once

#include "C4ForwardDeclarations.h"
#include "C4GameObjects.h"
#include "C4Group.h"
#include "C4Landscape.h"
#include "C4MassMover.h"
#include "C4Material.h"
#include "C4Math.h"
#include "C4ObjectInfo.h"
#include "C4Particles.h"
#include "C4PathFinder.h"
#include "C4PlayerInfo.h"
#include "C4PXS.h"
#include "C4Scenario.h"
#include "C4Texture.h"
#include "C4Weather.h"

#include <limits>
#include <memory>

class C4Section
{
public:
	enum class Status
	{
		Deleted = 0,
		Active = 1
	};

	struct EnumeratedPtrTraits
	{
		using Denumerated = C4Section;
		using Enumerated = std::int32_t;

		static Enumerated Enumerate(Denumerated *section);
		static Denumerated *Denumerate(Enumerated number);
	};

	using EnumeratedPtr = C4EnumeratedPtr<EnumeratedPtrTraits>;

private:
	template<typename T>
	struct ConditionalDeleter
	{
		bool ShouldDelete;

		void operator()(T *const ptr)
		{
			if (ShouldDelete)
			{
				delete ptr;
			}
		}
	};

public:
	template<typename T>
	using ConditionallyDeletingPtr = std::unique_ptr<T, ConditionalDeleter<T>>;

public:
	static constexpr auto Main = "main";

public:
	C4Section() noexcept;
	explicit C4Section(std::string name);

	C4Section(const C4Section &) = delete;
	C4Section &operator=(const C4Section &) = delete;

	C4Section(C4Section &&) = delete;
	C4Section &operator=(C4Section &&) = delete;

	~C4Section() { Clear(); }

public:
	void Default();
	void Clear();

	std::string_view GetName() const { return name; }

	bool InitFromTemplate(C4Group &scenario);
	bool InitFromEmptyLandscape(C4Group &scenario, const C4SLandscape &landscape);

	bool InitMaterialTexture(C4Section *fallback);

	bool InitSecondPart();
	bool InitThirdPart();
	bool CheckObjectEnumeration();


private:
	void InitInEarth();
	void InitVegetation();
	void InitAnimals();
	void InitEnvironment();
	bool PlaceInEarth(C4ID id);


public:
	void Execute();

	void ExecObjects();

	void DeleteObjects(bool deleteInactive);

	void ClearPointers(C4Object *obj);
	void ClearSectionPointers(C4Section &section);
	void EnumeratePointers();
	void DenumeratePointers();

	bool AssignRemoval();

	C4Object *OverlapObject(std::int32_t tx, std::int32_t ty, std::int32_t width, std::int32_t height, std::int32_t category);

	void BlastObjects(int32_t tx, int32_t ty, int32_t level, C4Object *inobj, int32_t iCausedBy, C4Object *pByObj);
	void ShakeObjects(int32_t tx, int32_t ry, int32_t range, C4Object *causedBy);
	C4Object *FindBase(int32_t iPlayer, int32_t iIndex);
	C4Object *FindFriendlyBase(int32_t iPlayer, int32_t iIndex);
	C4Object *FindObjectByCommand(int32_t iCommand, C4Object *pTarget = nullptr, C4Value iTx = C4VNull, int32_t iTy = 0, C4Object *pTarget2 = nullptr, C4Object *pFindNext = nullptr);
	void CastObjects(C4ID id, C4Object *pCreator, int32_t num, int32_t level, int32_t tx, int32_t ty, int32_t iOwner = NO_OWNER, int32_t iController = NO_OWNER);
	void BlastCastObjects(C4ID id, C4Object *pCreator, int32_t num, int32_t tx, int32_t ty, int32_t iController = NO_OWNER);
	C4Object *PlaceVegetation(C4ID id, int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, int32_t iGrowth);
	C4Object *PlaceAnimal(C4ID idAnimal);

	C4Object *CreateObject(C4ID type, C4Object *pCreator, int32_t owner = NO_OWNER,
		int32_t x = 50, int32_t y = 50, int32_t r = 0,
		C4Fixed xdir = Fix0, C4Fixed ydir = Fix0, C4Fixed rdir = Fix0, int32_t iController = NO_OWNER);

	C4Object *CreateObject(C4Def *def, C4Object *pCreator, int32_t owner = NO_OWNER,
		int32_t x = 50, int32_t y = 50, int32_t r = 0,
		C4Fixed xdir = Fix0, C4Fixed ydir = Fix0, C4Fixed rdir = Fix0, int32_t iController = NO_OWNER);

	C4Object *CreateObjectConstruction(C4ID type,
		C4Object *pCreator,
		int32_t owner,
		int32_t ctx = 0, int32_t bty = 0,
		int32_t con = 1, bool terrain = false);

	C4Object *CreateInfoObject(C4ObjectInfo *cinf, int32_t owner,
		int32_t tx = 50, int32_t ty = 50);

	C4Object *FindObject(C4ID id,
		int32_t iX = 0, int32_t iY = 0, int32_t iWdt = 0, int32_t iHgt = 0,
		uint32_t ocf = OCF_All,
		const char *szAction = nullptr, C4Object *pActionTarget = nullptr,
		C4Object *pExclude = nullptr,
		C4Object *pContainer = nullptr,
		int32_t iOwner = ANY_OWNER,
		C4Object *pFindNext = nullptr,
		C4ObjectList *extraList = nullptr);
	C4Object *FindVisObject( // find object in view at pos, regarding parallaxity and visibility (but not distance)
		int32_t tx, int32_t ty, int32_t iPlr, const C4Facet &fctViewport,
		int32_t iX = 0, int32_t iY = 0, int32_t iWdt = 0, int32_t iHgt = 0,
		uint32_t ocf = OCF_All,
		C4Object *pExclude = nullptr,
		int32_t iOwner = ANY_OWNER,
		C4Object *pFindNext = nullptr);
	int32_t ObjectCount(C4ID id,
		int32_t x = 0, int32_t y = 0, int32_t wdt = 0, int32_t hgt = 0,
		uint32_t ocf = OCF_All,
		const char *szAction = nullptr, C4Object *pActionTarget = nullptr,
		C4Object *pExclude = nullptr,
		C4Object *pContainer = nullptr,
		int32_t iOwner = ANY_OWNER,
		C4ObjectList *extraList = nullptr);

	void SyncClearance();
	void Synchronize();
	void SynchronizeTransferZones();

public:
	inline bool MatValid(const std::int32_t mat)
	{
		return Inside(mat, 0, Material.Num - 1);
	}

	int32_t PixCol2MatOld(uint8_t pixc)
	{
		if (pixc < GBM) return MNone;
		pixc &= 63; // Substract GBM, ignore IFT
		if (pixc > Material.Num * C4M_ColsPerMat - 1) return MNone;
		return pixc / C4M_ColsPerMat;
	}

	int32_t PixCol2MatOld2(uint8_t pixc)
	{
		int32_t iMat = (pixc & 0x7f) - 1;
		// if above MVehic, don't forget additional vehicle-colors
		if (iMat <= Landscape.MVehic) return iMat;
		// equals middle vehicle-color
		if (iMat == Landscape.MVehic + 1) return Landscape.MVehic;
		// above: range check
		iMat -= 2; if (iMat >= Material.Num) return MNone;
		return iMat;
	}

	inline int32_t PixCol2Tex(uint8_t pixc)
	{
		// Remove IFT
		int32_t iTex = pixc & (IFT - 1);
		// Validate
		if (iTex >= C4M_MaxTexIndex) return 0;
		// Done
		return iTex;
	}

	inline int32_t PixCol2Mat(uint8_t pixc)
	{
		// Get texture
		int32_t iTex = PixCol2Tex(pixc);
		if (!iTex) return MNone;
		// Get material-texture mapping
		const C4TexMapEntry *pTex = TextureMap.GetEntry(iTex);
		// Return material
		return pTex ? pTex->GetMaterialIndex() : MNone;
	}

	inline uint8_t MatTex2PixCol(int32_t tex)
	{
		return uint8_t(tex);
	}

	inline uint8_t Mat2PixColDefault(int32_t mat)
	{
		return Material.Map[mat].DefaultMatTex;
	}

	inline int32_t MatDensity(int32_t mat)
	{
		if (!MatValid(mat)) return 0;
		return Material.Map[mat].Density;
	}

	inline int32_t MatPlacement(int32_t mat)
	{
		if (!MatValid(mat)) return 0;
		return Material.Map[mat].Placement;
	}

	inline int32_t MatDigFree(int32_t mat)
	{
		if (!MatValid(mat)) return 1;
		return Material.Map[mat].DigFree;
	}

	inline int32_t GBackWind(int32_t x, int32_t y)
	{
		return Landscape.GBackIFT(x, y) ? 0 : Weather.Wind;
	}

	bool IsActive() const noexcept
	{
		return status == Status::Active;
	}

	void UpdateRootParent();

	C4Section &GetRootSection() noexcept;

	bool IsChildOf(const C4Section &section) const noexcept;

	void PointToParentPoint(std::int32_t &x, std::int32_t &y, const C4Section *untilParent = nullptr) const noexcept;
	void ParentPointToPoint(std::int32_t &x, std::int32_t &y, const C4Section *untilParent = nullptr) const noexcept;
	std::tuple<C4Section &, std::int32_t, std::int32_t> PointToChildPoint(int32_t x, int32_t y) noexcept;

private:
	// Object function internals
	C4Object *NewObject(C4Def *ndef, C4Object *pCreator,
		int32_t owner, C4ObjectInfo *info,
		int32_t tx, int32_t ty, int32_t tr,
		C4Fixed xdir, C4Fixed ydir, C4Fixed rdir,
		int32_t con, int32_t iController);

	void ClearObjectPtrs(C4Object *obj);
	void ObjectRemovalCheck();

public:
	C4Group Group;
	C4Scenario C4S;
	C4Weather Weather;
	C4TextureMap TextureMap;
	C4MaterialMap Material;
	C4GameObjects Objects;
	C4Landscape Landscape;
	C4MassMoverSet MassMover;
	C4PXSSystem PXS;
	C4ParticleSystem Particles;
	C4PathFinder PathFinder;
	C4TransferZones TransferZones;
	C4Effect *GlobalEffects;
	bool ResortAnyObject{false};
	bool LandscapeLoaded{false};
	std::uint32_t RemovalDelay{};
	std::uint32_t Number;
	std::vector<EnumeratedPtr> Children;
	C4Section *Parent{nullptr};
	C4Section *RootParent{nullptr};
	C4Rect RenderAsChildBounds;
	bool ChildVisible{false};

public:
	static void ResetEnumerationIndex() noexcept;
	static std::uint32_t AcquireEnumerationIndex() noexcept;

	static inline constexpr std::uint32_t NoSectionSentinel{std::numeric_limits<std::uint32_t>::max()};

private:
	static inline std::uint32_t enumerationIndex{0};

private:
	std::string name;
	bool emptyLandscape{false};
	Status status{Status::Active};
};
