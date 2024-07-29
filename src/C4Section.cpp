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

#include "C4Game.h"
#include "C4Math.h"
#include "C4Random.h"
#include "C4Section.h"

C4Section::EnumeratedPtrTraits::Enumerated C4Section::EnumeratedPtrTraits::Enumerate(Denumerated *const section)
{
	if (section)
	{
		return section->Number;
	}

	return C4Section::NoSectionSentinel;
}

C4Section::EnumeratedPtrTraits::Denumerated *C4Section::EnumeratedPtrTraits::Denumerate(const Enumerated number)
{
	return Game.GetSectionByNumber(number);
}

C4Section::C4Section() noexcept
	: C4Section{AcquireEnumerationIndex()}
{
	Default();
}

C4Section::C4Section(std::string name)
	: C4Section{}
{
	this->name = std::move(name);
}

C4Section::C4Section(const std::uint32_t number) noexcept
	: Weather{*this}, TextureMap{*this}, Material{*this}, Landscape{*this}, MassMover{*this}, PXS{*this}, Particles{*this}, Number{number}, numberAsString{std::format("{}", number)}
{
	Default();
}

void C4Section::Default()
{
	Material.Default();
	Objects.Default();
	Objects.BackObjects.Default();
	Objects.ForeObjects.Default();
	Weather.Default();
	Landscape.Default();
	TextureMap.Default();
	MassMover.Default();
	PXS.Default();
	C4S.Default();
	PathFinder.Default();
	TransferZones.Default();
	GlobalEffects = nullptr;
	Children.clear();
	Parent = nullptr;
}

void C4Section::Clear()
{
	Weather.Clear();
	Landscape.Clear();
	DeleteObjects(true);
	PXS.Clear();
	Material.Clear();
	TextureMap.Clear(); // texture map *MUST* be cleared after the materials, because of the patterns!
	PathFinder.Clear();
	TransferZones.Clear();
	delete GlobalEffects;
	GlobalEffects = nullptr;
	Children.clear();
	Parent = nullptr;
}

bool C4Section::InitFromTemplate(C4Group &scenario, const bool savegame)
{
	if (!savegame)
	{
		C4S = Game.C4S;
	}

	if (name.empty())
	{
		if (!Group.Open(scenario.GetFullName().getData()))
		{
			return false;
		}
	}
	else
	{
		if (!Group.OpenAsChild(&scenario, std::format(C4CFN_Section, name).c_str()))
		{
			LogFatalNTr("GROUP");
			return false;
		}

		if (!C4S.Load(Group, true))
		{
			LogFatalNTr("C4S");
			return false;
		}
	}

	return true;
}

bool C4Section::InitFromEmptyLandscape(C4Group &scenario, const C4SLandscape &landscape)
{
	if (!Group.Open(scenario.GetFullName().getData()))
	{
		LogFatalNTr("GROUP");
		return false;
	}

	name = std::format("Empty{}", Number);

	C4S = Game.C4S;
	C4S.Head = Game.C4S.Head;
	C4S.Definitions = Game.C4S.Definitions;
	C4S.Game = Game.C4S.Game;
	std::ranges::copy(Game.C4S.PlrStart, C4S.PlrStart);
	C4S.Landscape = landscape;
	C4S.Animals = Game.C4S.Animals;
	C4S.Weather = Game.C4S.Weather;
	C4S.Disasters = Game.C4S.Disasters;
	C4S.Environment = Game.C4S.Environment;

	emptyLandscape = true;
	return true;
}

bool C4Section::AssumeGroupAsSaveGameGroup()
{
	SaveGameGroup.emplace();

	if (!SaveGameGroup->Open(Group.GetFullName().getData()))
	{
		return false;
	}

	return true;
}

bool C4Section::LoadSaveGame(C4Group &scenario, std::string_view entryName)
{
	SaveGameGroup.emplace();

	if (!SaveGameGroup->OpenAsChild(&scenario, entryName.data()))
	{
		return false;
	}

	C4ComponentHost sectionRuntimeData;
	if (!sectionRuntimeData.Load(C4CFN_SectionRuntimeData, *SaveGameGroup, C4CFN_SectionRuntimeData))
	{
		return false;
	}

	if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(mkParAdapt(*this, false), StdStrBuf{sectionRuntimeData.GetData(), sectionRuntimeData.GetDataSize(), false}, C4CFN_SectionRuntimeData))
	{
		return false;
	}

	return true;
}

bool C4Section::InitFromSaveGameAfterLoad(C4Group &scenario)
{
	if (emptyLandscape)
	{
		if (!Group.Open(scenario.GetFullName().getData()))
		{
			LogFatalNTr("GROUP");
			return false;
		}
	}
	else
	{
		if (!InitFromTemplate(scenario, true))
		{
			return false;
		}
	}

	return true;
}

bool C4Section::SaveRuntimeData(C4Group &group)
{
	std::string output;
	if (!DecompileToBuf_Log<StdCompilerINIWrite>(mkParAdapt(*this, false), &output, C4CFN_SectionRuntimeData))
	{
		return false;
	}

	StdStrBuf copy{output.c_str(), output.size()};
	return group.Add(C4CFN_SectionRuntimeData, copy, false, true);
}

bool C4Section::InitMaterialTexture(C4Section *const fallback)
{
	// Clear old data
	TextureMap.Clear();
	Material.Clear();

	// Check for scenario local materials
	const bool isMainSection{name.empty()};
	bool haveSectionMaterials{!emptyLandscape && Group.FindEntry(C4CFN_Material)};

	if (!haveSectionMaterials && fallback)
	{
		Material.CopyMaterials(fallback->Material);

		if (SaveGameGroup)
		{
			C4Group group;
			if (group.OpenAsChild(&*SaveGameGroup, C4CFN_Material))
			{
				if (!TextureMap.InitFromMapAndExistingTextures(group, C4CFN_TexMap, fallback->TextureMap))
				{
					LogFatal(C4ResStrTableKey::IDS_ERR_SCENARIOMATERIALS, group.GetError());
					return false;
				}
			}
			else
			{
				TextureMap = fallback->TextureMap;
			}
		}
		else
		{
			TextureMap = fallback->TextureMap;
		}
	}
	else
	{
		bool haveSectionSavegameMaterials{SaveGameGroup && SaveGameGroup->FindEntry(C4CFN_Material)};
		bool haveScenarioFileMaterials{!isMainSection && Game.ScenarioFile.FindEntry(C4CFN_Material)};
		bool skipNextTexMap{false};
		bool onlyLoadTexMap{false};

		// Load all materials
		auto matRes = Game.Parameters.GameRes.iterRes(NRT_Material);
		bool fFirst = true, fOverloadMaterials = true, fOverloadTextures = true;
		long tex_count = 0, mat_count = 0;
		while (fOverloadMaterials || fOverloadTextures)
		{
			// Are there any scenario local materials that need to be looked at firs?
			C4Group Mats;
			if (haveSectionSavegameMaterials)
			{
				if (!Mats.OpenAsChild(&*SaveGameGroup, C4CFN_Material))
				{
					LogFatal(C4ResStrTableKey::IDS_ERR_SCENARIOMATERIALS, Mats.GetError());
					return false;
				}

				// Once only
				haveSectionSavegameMaterials = false;
				skipNextTexMap = !isMainSection;
				onlyLoadTexMap = !isMainSection;
			}
			else if (haveSectionMaterials)
			{
				if (!Mats.OpenAsChild(&Group, C4CFN_Material))
				{
					LogFatal(C4ResStrTableKey::IDS_ERR_SCENARIOMATERIALS, Mats.GetError());
					return false;
				}
				// Once only
				haveSectionMaterials = false;
			}
			else if (haveScenarioFileMaterials)
			{
				if (!Mats.OpenAsChild(&Game.ScenarioFile, C4CFN_Material))
				{
					LogFatal(C4ResStrTableKey::IDS_ERR_SCENARIOMATERIALS, Mats.GetError());
					return false;
				}

				haveScenarioFileMaterials = false;
			}
			else
			{
				if (matRes == matRes.end()) break;
				// Find next external material source
				if (!Mats.Open(matRes->getFile()))
				{
					LogFatal(C4ResStrTableKey::IDS_ERR_EXTERNALMATERIALS, matRes->getFile(), Mats.GetError());
					return false;
				}
				++matRes;
			}

			// First material file? Load texture map.
			bool fNewOverloadMaterials = false, fNewOverloadTextures = false;
			if (fFirst)
			{
				long tme_count = TextureMap.LoadMap(Mats, C4CFN_TexMap, &fNewOverloadMaterials, &fNewOverloadTextures);
				Log(C4ResStrTableKey::IDS_PRC_TEXMAPENTRIES, tme_count);
				// Only once
				fFirst = false;
			}
			else if (skipNextTexMap)
			{
				// Skip texture map
				skipNextTexMap = false;
				fNewOverloadMaterials = fOverloadMaterials;
				fNewOverloadTextures = fOverloadTextures;
			}
			else
			{
				// Check overload-flags only
				if (!C4TextureMap::LoadFlags(Mats, C4CFN_TexMap, &fNewOverloadMaterials, &fNewOverloadTextures))
					fOverloadMaterials = fOverloadTextures = false;
			}

			if (onlyLoadTexMap)
			{
				onlyLoadTexMap = false;
			}
			else
			{
				// Load textures
				if (fOverloadTextures)
				{
					int iTexs = TextureMap.LoadTextures(Mats);
					// Automatically continue search if no texture was found
					if (!iTexs) fNewOverloadTextures = true;
					tex_count += iTexs;
				}

				// Load materials
				if (fOverloadMaterials)
				{
					int iMats = Material.Load(Mats);
					// Automatically continue search if no material was found
					if (!iMats) fNewOverloadMaterials = true;
					mat_count += iMats;
				}

				// Set flags
				fOverloadTextures = fNewOverloadTextures;
				fOverloadMaterials = fNewOverloadMaterials;
			}
		}

		// Logs
		Log(C4ResStrTableKey::IDS_PRC_TEXTURES, tex_count);
		Log(C4ResStrTableKey::IDS_PRC_MATERIALS, mat_count);

		// Load material enumeration
		if (SaveGameGroup && !Material.LoadEnumeration(*SaveGameGroup))
		{
			LogFatal(C4ResStrTableKey::IDS_PRC_NOMATENUM); return false;
		}

		// Initialize texture map
		TextureMap.Init();
	}

	// Cross map mats (after texture init, because Material-Texture-combinations are used)
	Material.CrossMapMaterials();

	// Check material number
	if (Material.Num > C4MaxMaterial)
	{
		LogFatal(C4ResStrTableKey::IDS_PRC_TOOMANYMATS); return false;
	}

	// Enumerate materials
	if (!Landscape.EnumerateMaterials()) return false;

	// get material script funcs
	Material.UpdateScriptPointers();

	return true;
}

bool C4Section::InitSecondPart(C4Random &random)
{
	LandscapeLoaded = false;
	if (!(emptyLandscape ? Landscape.InitEmpty(random, true, LandscapeLoaded) : Landscape.Init(Group, SaveGameGroup ? &*SaveGameGroup : nullptr, random, name.empty(), false, true, LandscapeLoaded, C4S.Head.SaveGame)))
	{
		LogFatal(C4ResStrTableKey::IDS_ERR_GBACK);
		return false;
	}

	// the savegame flag is set if runtime data is present, in which case this is to be used
	if (LandscapeLoaded && !SaveGameGroup)
	{
		Landscape.ScenarioInit(random);
	}

	// Init main object list
	Objects.Init(Landscape.Width, Landscape.Height);

	// Pathfinder
	PathFinder.Init(Landscape, [](C4Landscape &landscape, const std::int32_t x, const std::int32_t y)
	{
		if (!Inside(x, 0, landscape.Width - 1) || !Inside(y, 0, landscape.Height - 1)) return false;
		return !DensitySolid(landscape.GetDensity(x, y));
	}, &TransferZones);

	// PXS
	if (SaveGameGroup && SaveGameGroup->FindEntry(C4CFN_PXS))
	{
		if (!PXS.Load(*SaveGameGroup))
		{
		   LogFatal(C4ResStrTableKey::IDS_ERR_PXS);
		   return false;
		}
	}
	else if (LandscapeLoaded)
	{
		PXS.Clear();
		PXS.Default();
	}

	// MassMover
	if (SaveGameGroup && SaveGameGroup->FindEntry(C4CFN_MassMover))
	{
		if (!MassMover.Load(*SaveGameGroup))
		{
			LogFatal(C4ResStrTableKey::IDS_ERR_MOVER); return false;
		}
	}
	else if (LandscapeLoaded)
	{
		// clear and set mass-mover to default values
		MassMover.Clear();
		MassMover.Default();
	}

	// Load objects
	std::optional<bool> objectsLoaded;
	if (SaveGameGroup)
	{
		objectsLoaded = Objects.Load(*this, *SaveGameGroup, "");
	}

	if (!objectsLoaded && !emptyLandscape)
	{
		objectsLoaded = Objects.Load(*this, Group, "");
	}

	if (objectsLoaded && !*objectsLoaded)
	{
		return false;
	}

	Group.Close();

	if (SaveGameGroup)
	{
		SaveGameGroup->Close();
	}

	return true;
}

bool C4Section::FinishObjectLoading(const bool renumberEverything)
{
	if (const std::optional<int> loadedObjects{Objects.AfterLoad(*this, true, renumberEverything)}; loadedObjects)
	{
		Log(C4ResStrTableKey::IDS_PRC_OBJECTSLOADED, *loadedObjects);
	}

	return true;
}

bool C4Section::InitThirdPart()
{
	for (const auto &def : Game.Defs)
	{
		def->Script.Call(*this, PSF_InitializeDef, {name.empty() ? C4VNull : C4VString(name.c_str())});
	}

	// Environment
	if (!C4S.Head.NoInitialize && LandscapeLoaded)
	{
		Log(C4ResStrTableKey::IDS_PRC_ENVIRONMENT);
		InitVegetation();
		InitInEarth();
		InitAnimals();
		InitEnvironment();
		Landscape.PostInitMap();
	}

	if (LandscapeLoaded) Weather.Init(!C4S.Head.SaveGame);

	return true;
}

bool C4Section::CheckObjectEnumeration()
{
	// Check valid & maximum number & duplicate numbers
	int32_t iMax = 0;
	C4Object *cObj; C4ObjectLink *clnk;
	C4Object *cObj2; C4ObjectLink *clnk2;
	clnk = Objects.First; if (!clnk) clnk = Objects.InactiveObjects.First;
	while (clnk)
	{
		// Invalid number
		cObj = clnk->Obj;
		if (cObj->Number < 1)
		{
			LogNTr(spdlog::level::err, "Invalid object enumeration number ({}) of object {} (x={})", cObj->Number, C4IdText(cObj->id), cObj->x);
			return false;
		}
		// Max
		if (cObj->Number > iMax) iMax = cObj->Number;
		// Duplicate
		for (clnk2 = Objects.First; clnk2 && (cObj2 = clnk2->Obj); clnk2 = clnk2->Next)
			if (cObj2 != cObj)
				if (cObj->Number == cObj2->Number)
				{
					LogNTr(spdlog::level::err, "Duplicate object enumeration number {} ({} and {})", cObj2->Number, cObj->GetName(), cObj2->GetName());
					return false;
				}
		for (clnk2 = Objects.InactiveObjects.First; clnk2 && (cObj2 = clnk2->Obj); clnk2 = clnk2->Next)
			if (cObj2 != cObj)
				if (cObj->Number == cObj2->Number)
				{
					LogNTr(spdlog::level::err, "Duplicate object enumeration number {} ({} and {}(i))", cObj2->Number, cObj->GetName(), cObj2->GetName());
					return false;
				}
		// next
		if (!clnk->Next)
			if (clnk == Objects.Last) clnk = Objects.InactiveObjects.First; else clnk = nullptr;
		else
			clnk = clnk->Next;
	}
	// Adjust enumeration index
	if (iMax > Game.ObjectEnumerationIndex) Game.ObjectEnumerationIndex = iMax;
	// Done
	return true;
}

static int32_t ListExpandValids(C4IDList &rlist,
	C4ID *idlist, int32_t maxidlist)
{
	int32_t cnt, cnt2, ccount, cpos;
	for (cpos = 0, cnt = 0; rlist.GetID(cnt); cnt++)
		if (Game.Defs.ID2Def(rlist.GetID(cnt, &ccount)))
			for (cnt2 = 0; cnt2 < ccount; cnt2++)
				if (cpos < maxidlist)
				{
					idlist[cpos] = rlist.GetID(cnt); cpos++;
				}
	return cpos;
}

void C4Section::InitInEarth()
{
	const int32_t maxvid = 100;
	int32_t cnt, vidnum;
	C4ID vidlist[maxvid];
	// Amount
	int32_t amt = (Landscape.Width * Landscape.Height / 5000) * C4S.Landscape.InEarthLevel.Evaluate(C4Random::Default) / 100;
	// List all valid IDs from C4S
	vidnum = ListExpandValids(C4S.Landscape.InEarth, vidlist, maxvid);
	// Place
	if (vidnum > 0)
		for (cnt = 0; cnt < amt; cnt++)
			PlaceInEarth(vidlist[Random(vidnum)]);
}

void C4Section::InitVegetation()
{
	const int32_t maxvid = 100;
	int32_t cnt, vidnum;
	C4ID vidlist[maxvid];
	// Amount
	int32_t amt = (Landscape.Width / 50) * C4S.Landscape.VegLevel.Evaluate(C4Random::Default) / 100;
	// Get percentage vidlist from C4S
	vidnum = ListExpandValids(C4S.Landscape.Vegetation, vidlist, maxvid);
	// Place vegetation
	if (vidnum > 0)
		for (cnt = 0; cnt < amt; cnt++)
			PlaceVegetation(vidlist[Random(vidnum)], 0, 0, Landscape.Width, Landscape.Height, -1);
}

void C4Section::InitAnimals()
{
	int32_t cnt, cnt2;
	C4ID idAnimal; int32_t iCount;
	// Place animals
	for (cnt = 0; (idAnimal = C4S.Animals.FreeLife.GetID(cnt, &iCount)); cnt++)
		for (cnt2 = 0; cnt2 < iCount; cnt2++)
			PlaceAnimal(idAnimal);
	// Place nests
	for (cnt = 0; (idAnimal = C4S.Animals.EarthNest.GetID(cnt, &iCount)); cnt++)
		for (cnt2 = 0; cnt2 < iCount; cnt2++)
			PlaceInEarth(idAnimal);
}

void C4Section::InitEnvironment()
{
	// Place environment objects
	int32_t cnt, cnt2;
	C4ID idType; int32_t iCount;
	for (cnt = 0; (idType = C4S.Environment.Objects.GetID(cnt, &iCount)); cnt++)
		for (cnt2 = 0; cnt2 < iCount; cnt2++)
			CreateObject(idType, nullptr);
}

bool C4Section::PlaceInEarth(C4ID id)
{
	int32_t cnt, tx, ty;
	for (cnt = 0; cnt < 35; cnt++) // cheap trys
	{
		tx = Random(Landscape.Width); ty = Random(Landscape.Height);
		if (Landscape.GetMat(tx, ty) == Landscape.MEarth)
			if (CreateObject(id, nullptr, NO_OWNER, tx, ty, Random(360)))
				return true;
	}
	return false;
}

C4Object *C4Section::PlaceVegetation(C4ID id, int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, int32_t iGrowth)
{
	int32_t cnt, iTx, iTy, iMaterial;

	// Get definition
	C4Def *pDef;
	if (!(pDef = Game.Defs.ID2Def(id))) return nullptr;

	// No growth specified: full or random growth
	if (iGrowth <= 0)
	{
		iGrowth = FullCon;
		if (pDef->Growth) if (!Random(3)) iGrowth = Random(FullCon) + 1;
	}

	// Place by placement type
	switch (pDef->Placement)
	{
	// Surface soil
	case C4D_Place_Surface:
		for (cnt = 0; cnt < 20; cnt++)
		{
			// Random hit within target area
			iTx = iX + Random(iWdt); iTy = iY + Random(iHgt);
			// Above IFT
			while ((iTy > 0) && Landscape.GBackIFT(iTx, iTy)) iTy--;
			// Above semi solid
			if (!Landscape.AboveSemiSolid(iTx, iTy) || !Inside<int32_t>(iTy, 50, Landscape.Height - 50))
				continue;
			// Free above
			if (Landscape.GBackSemiSolid(iTx, iTy - pDef->Shape.Hgt) || Landscape.GBackSemiSolid(iTx, iTy - pDef->Shape.Hgt / 2))
				continue;
			// Free upleft and upright
			if (Landscape.GBackSemiSolid(iTx - pDef->Shape.Wdt / 2, iTy - pDef->Shape.Hgt * 2 / 3) || Landscape.GBackSemiSolid(iTx + pDef->Shape.Wdt / 2, iTy - pDef->Shape.Hgt * 2 / 3))
				continue;
			// Soil check
			iTy += 3; // two pix into ground
			iMaterial = Landscape.GetMat(iTx, iTy);
			if (iMaterial != MNone) if (Material.Map[iMaterial].Soil)
			{
				if (!pDef->Growth) iGrowth = FullCon;
				iTy += 5;
				return CreateObjectConstruction(id, nullptr, NO_OWNER, iTx, iTy, iGrowth);
			}
		}
		break;

	// Underwater
	case C4D_Place_Liquid:
		// Random range
		iTx = iX + Random(iWdt); iTy = iY + Random(iHgt);
		// Find liquid
		if (!Landscape.FindSurfaceLiquid(iTx, iTy, pDef->Shape.Wdt, pDef->Shape.Hgt))
			if (!Landscape.FindLiquid(iTx, iTy, pDef->Shape.Wdt, pDef->Shape.Hgt))
				return nullptr;
		// Liquid bottom
		if (!Landscape.SemiAboveSolid(iTx, iTy)) return nullptr;
		iTy += 3;
		// Create object
		return CreateObjectConstruction(id, nullptr, NO_OWNER, iTx, iTy, iGrowth);
	}

	// Undefined placement type
	return nullptr;
}

C4Object *C4Section::PlaceAnimal(C4ID idAnimal)
{
	C4Def *pDef = Game.Defs.ID2Def(idAnimal);
	if (!pDef) return nullptr;
	int32_t iX, iY;
	// Placement
	switch (pDef->Placement)
	{
	// Running free
	case C4D_Place_Surface:
		iX = Random(Landscape.Width); iY = Random(Landscape.Height);
		if (!Landscape.FindSolidGround(iX, iY, pDef->Shape.Wdt)) return nullptr;
		break;
	// In liquid
	case C4D_Place_Liquid:
		iX = Random(Landscape.Width); iY = Random(Landscape.Height);
		if (!Landscape.FindSurfaceLiquid(iX, iY, pDef->Shape.Wdt, pDef->Shape.Hgt))
			if (!Landscape.FindLiquid(iX, iY, pDef->Shape.Wdt, pDef->Shape.Hgt))
				return nullptr;
		iY += pDef->Shape.Hgt / 2;
		break;
	// Floating in air
	case C4D_Place_Air:
		iX = Random(Landscape.Width);
		for (iY = 0; (iY < Landscape.Height) && !Landscape.GBackSemiSolid(iX, iY); iY++);
		if (iY <= 0) return nullptr;
		iY = Random(iY);
		break;
	default:
		return nullptr;
	}
	// Create object
	return CreateObject(idAnimal, nullptr, NO_OWNER, iX, iY);
}

void C4Section::Execute()
{
	if (GlobalEffects)
	{
		GlobalEffects->Execute(nullptr);
	}

	Particles.GlobalParticles.Exec(*this);
	PXS.Execute();
	MassMover.Execute();
	Weather.Execute();
	Landscape.Execute();
}

void C4Section::ExecObjects() // Every Tick1 by Execute
{
#ifdef DEBUGREC
	AddDbgRec(RCT_Block, "ObjEx", 6);
#endif

	// Execute objects - reverse order to ensure
	for (auto it = Objects.BeginLast(); it != std::default_sentinel; ++it)
	{
		if ((*it)->Status)
			// Execute object
			(*it)->Execute();
		else
			// Status reset: process removal delay
			if ((*it)->RemovalDelay > 0) (*it)->RemovalDelay--;
	}

#ifdef DEBUGREC
	AddDbgRec(RCT_Block, "ObjCC", 6);
#endif

	// Cross check objects
	Objects.CrossCheck();

#ifdef DEBUGREC
	AddDbgRec(RCT_Block, "ObjRs", 6);
#endif

	// Resort
	if (ResortAnyObject)
	{
		ResortAnyObject = false;
		Objects.ResortUnsorted();
	}
	if (Objects.ResortProc) Objects.ExecuteResorts();

#ifdef DEBUGREC
	AddDbgRec(RCT_Block, "ObjRm", 6);
#endif

	// Removal
	if (!Tick255) ObjectRemovalCheck();
}

void C4Section::DeleteObjects(const bool deleteInactive)
{
	Objects.DeleteObjects();
	Objects.BackObjects.Clear();
	Objects.ForeObjects.Clear();

	if (deleteInactive)
	{
		Objects.InactiveObjects.DeleteObjects();
	}

	ResortAnyObject = false;
}

void C4Section::ClearPointers(C4Object *const obj)
{
	Objects.BackObjects.ClearPointers(obj);
	Objects.ForeObjects.ClearPointers(obj);
	ClearObjectPtrs(obj);
	TransferZones.ClearPointers(obj);

	if (GlobalEffects)
	{
		GlobalEffects->ClearPointers(obj);
	}
}

void C4Section::EnumeratePointers()
{
	if (GlobalEffects)
	{
		GlobalEffects->EnumeratePointers();
	}

	for (auto &child : Children)
	{
		child.Enumerate();
	}
}

void C4Section::DenumeratePointers()
{
	if (GlobalEffects)
	{
		GlobalEffects->DenumeratePointers(false);
	}

	std::erase_if(Children, [this](auto &child)
	{
		child.Denumerate();

		if (child)
		{
			child->Parent = this;
			return false;
		}
		else
		{
			return true;
		}
	});

	UpdateRootParent();
}

bool C4Section::AssignRemoval()
{
	if (!IsActive()) return false;

	for (C4ObjectLink *link{Objects.First}; link; link = link->Next)
	{
		link->Obj->AssignRemoval();
	}

	if (GlobalEffects)
	{
		GlobalEffects->ClearAll(nullptr, C4FxCall_RemoveClear);
		delete GlobalEffects;
		GlobalEffects = nullptr;
	}

	Particles.ClearParticles();
	RemovalDelay = 3;

	for (auto &child : Children)
	{
		child->Parent = nullptr;
		child->RootParent = nullptr;
	}

	Children.clear();

	return true;
}

C4Object *C4Section::OverlapObject(const std::int32_t tx, const std::int32_t ty, const std::int32_t width, const std::int32_t height, const std::int32_t category)
{
	C4Object *cObj; C4ObjectLink *clnk;
	C4Rect rect1, rect2;
	rect1.x = tx; rect1.y = ty; rect1.Wdt = width; rect1.Hgt = height;
	C4LArea Area(&Objects.Sectors, tx, ty, width, height); C4LSector *pSector;
	for (C4ObjectList *pObjs = Area.FirstObjectShapes(&pSector); pSector; pObjs = Area.NextObjectShapes(&pSector))
		for (clnk = pObjs->First; clnk && (cObj = clnk->Obj); clnk = clnk->Next)
			if (cObj->Status) if (!cObj->Contained)
				if (cObj->Category & category & C4D_SortLimit)
				{
					rect2 = cObj->Shape; rect2.x += cObj->x; rect2.y += cObj->y;
					if (rect1.Overlap(rect2)) return cObj;
				}
	return nullptr;
}

C4Object *C4Section::NewObject(C4Def *pDef, C4Object *pCreator,
	int32_t iOwner, C4ObjectInfo *pInfo,
	int32_t iX, int32_t iY, int32_t iR,
	C4Fixed xdir, C4Fixed ydir, C4Fixed rdir,
	int32_t iCon, int32_t iController)
{
	// Safety
	if (!pDef) return nullptr;
#ifdef DEBUGREC
	C4RCCreateObj rc;
	rc.id = pDef->id;
	rc.oei = Game.ObjectEnumerationIndex + 1;
	rc.x = iX; rc.y = iY; rc.ownr = iOwner;
	AddDbgRec(RCT_CrObj, &rc, sizeof(rc));
#endif
	// Create object
	auto obj = std::make_unique<C4Object>();
	C4Object *const objPtr{obj.get()};

	// Initialize object
	obj->Init(pDef, *this, pCreator, iOwner, pInfo, iX, iY, iR, xdir, ydir, rdir, iController);
	// Enumerate object
	obj->Number = ++Game.ObjectEnumerationIndex;
	// Add to object list

	if (Objects.Add(obj.get()))
	{
		obj.release();
	}
	else
	{
		return nullptr;
	}

	// From now on, object is ready to be used in scripts!
	// Construction callback
	C4AulParSet pars(C4VObj(pCreator));
	objPtr->Call(PSF_Construction, pars);
	// AssignRemoval called? (Con 0)
	if (!objPtr->Status) { return nullptr; }
	// Do initial con
	objPtr->DoCon(iCon, true);
	// AssignRemoval called? (Con 0)
	if (!objPtr->Status) { return nullptr; }
	// Success
	return objPtr;
}

C4Object *C4Section::CreateObject(C4ID id, C4Object *pCreator, int32_t iOwner,
	int32_t x, int32_t y, int32_t r,
	C4Fixed xdir, C4Fixed ydir, C4Fixed rdir, int32_t iController)
{
	C4Def *pDef;
	// Get pDef
	if (!(pDef = Game.Defs.ID2Def(id))) return nullptr;
	// Create object
	return NewObject(pDef, pCreator,
		iOwner, nullptr,
		x, y, r,
		xdir, ydir, rdir,
		FullCon, iController);
}

C4Object *C4Section::CreateObject(C4Def *const def, C4Object *pCreator, int32_t iOwner,
	int32_t x, int32_t y, int32_t r,
	C4Fixed xdir, C4Fixed ydir, C4Fixed rdir, int32_t iController)
{
	// Create object
	return NewObject(def, pCreator,
		iOwner, nullptr,
		x, y, r,
		xdir, ydir, rdir,
		FullCon, iController);
}

C4Object *C4Section::CreateInfoObject(C4ObjectInfo *cinf, int32_t iOwner,
	int32_t tx, int32_t ty)
{
	C4Def *def;
	// Valid check
	if (!cinf) return nullptr;
	// Get def
	if (!(def = Game.Defs.ID2Def(cinf->id))) return nullptr;
	// Create object
	return NewObject(def, nullptr,
		iOwner, cinf,
		tx, ty, 0,
		Fix0, Fix0, Fix0,
		FullCon, NO_OWNER);
}

C4Object *C4Section::CreateObjectConstruction(C4ID id,
	C4Object *pCreator,
	int32_t iOwner,
	int32_t iX, int32_t iBY,
	int32_t iCon,
	bool fTerrain)
{
	C4Def *pDef;
	C4Object *pObj;

	// Get def
	if (!(pDef = Game.Defs.ID2Def(id))) return nullptr;

	int32_t dx, dy, dwdt, dhgt;
	dwdt = pDef->Shape.Wdt; dhgt = pDef->Shape.Hgt;
	dx = iX - dwdt / 2; dy = iBY - dhgt;

	// Terrain & Basement
	if (fTerrain)
	{
		// Clear site background (ignored for ultra-large structures)
		if (dwdt * dhgt < 12000)
			Landscape.DigFreeRect(dx, dy, dwdt, dhgt);
		// Raise Terrain
		Landscape.RaiseTerrain(dx, dy + dhgt, dwdt);
		// Basement
		if (pDef->Basement)
		{
			const int32_t BasementStrength = 8;
			// Border basement
			if (pDef->Basement > 1)
			{
				Landscape.DrawMaterialRect(Landscape.MGranite, dx, dy + dhgt, std::min<int32_t>(pDef->Basement, dwdt), BasementStrength);
				Landscape.DrawMaterialRect(Landscape.MGranite, dx + dwdt - std::min<int32_t>(pDef->Basement, dwdt), dy + dhgt, std::min<int32_t>(pDef->Basement, dwdt), BasementStrength);
			}
			// Normal basement
			else
				Landscape.DrawMaterialRect(Landscape.MGranite, dx, dy + dhgt, dwdt, BasementStrength);
		}
	}

	// Create object
	if (!(pObj = NewObject(pDef,
		pCreator,
		iOwner, nullptr,
		iX, iBY, 0,
		Fix0, Fix0, Fix0,
		iCon, pCreator ? pCreator->Controller : NO_OWNER))) return nullptr;

	return pObj;
}

void C4Section::BlastObjects(int32_t tx, int32_t ty, int32_t level, C4Object *inobj, int32_t iCausedBy, C4Object *pByObj)
{
	C4Object *cObj; C4ObjectLink *clnk;

	// layer check: Blast in same layer only
	if (pByObj) pByObj = pByObj->pLayer;

	// Contained blast
	if (inobj)
	{
		inobj->Blast(level, iCausedBy);
		for (clnk = Objects.First; clnk && (cObj = clnk->Obj); clnk = clnk->Next)
			if (cObj->Status) if (cObj->Contained == inobj) if (cObj->pLayer == pByObj)
				cObj->Blast(level, iCausedBy);
	}

	// Uncontained blast local outside objects
	else
	{
		for (clnk = Objects.First; clnk && (cObj = clnk->Obj); clnk = clnk->Next)
			if (cObj->Status) if (!cObj->Contained) if (cObj->pLayer == pByObj)
			{
				// Direct hit (5 pixel range to all sides)
				if (Inside<int32_t>(ty - (cObj->y + cObj->Shape.y), -5, cObj->Shape.Hgt - 1 + 10))
					if (Inside<int32_t>(tx - (cObj->x + cObj->Shape.x), -5, cObj->Shape.Wdt - 1 + 10))
						cObj->Blast(level, iCausedBy);
				// Shock wave hit (if in level range, living, object and vehicle only. No structures/StatickBack, as this would mess up castles, elevators, etc.!)
				if (cObj->Category & (C4D_Living | C4D_Object | C4D_Vehicle))
					if (!cObj->Def->NoHorizontalMove)
						if (Abs(ty - cObj->y) <= level)
							if (Abs(tx - cObj->x) <= level)
							{
								// vehicles and floating objects only if grab+pushable (no throne, no tower entrances...)
								if (cObj->Def->Grab != 1)
								{
									if (cObj->Category & C4D_Vehicle)
										continue;
									if (cObj->Action.Act >= 0)
										if (cObj->Def->ActMap[cObj->Action.Act].Procedure == DFA_FLOAT)
											continue;
								}
								if (cObj->Category & C4D_Living)
								{
									// living takes additional dmg from blasts
									cObj->DoEnergy(-level / 2, false, C4FxCall_EngBlast, iCausedBy);
									cObj->DoDamage(level / 2, iCausedBy, C4FxCall_DmgBlast);
								}

								// force argument evaluation order
								const auto p2 = itofix(-level + Abs(ty - cObj->y)) / BoundBy<int32_t>(cObj->Mass / 10, 4, (cObj->Category & C4D_Living) ? 8 : 20);
								const auto p1 = itofix(Sign(cObj->x - tx + Rnd3()) * (level - Abs(tx - cObj->x))) / BoundBy<int32_t>(cObj->Mass / 10, 4, (cObj->Category & C4D_Living) ? 8 : 20);
								cObj->Fling(p1, p2, true, iCausedBy);
							}
			}
	}
}

void C4Section::ShakeObjects(int32_t tx, int32_t ty, int32_t range, C4Object *const causedBy)
{
	C4Object *cObj; C4ObjectLink *clnk;

	const std::int32_t controller{causedBy ? causedBy->Controller : NO_OWNER};

	for (clnk = Objects.First; clnk && (cObj = clnk->Obj); clnk = clnk->Next)
		if (cObj->Status) if (!cObj->Contained)
			if (cObj->Category & C4D_Living)
				if (Abs(ty - cObj->y) <= range)
					if (Abs(tx - cObj->x) <= range)
						if (!Random(3))
							if (cObj->Action.t_attach)
								if (!Landscape.MatVehicle(cObj->Shape.AttachMat))
								{
									cObj->Fling(itofix(Rnd3()), Fix0, false, controller);
								}
}

C4Object *C4Section::FindBase(int32_t iPlayer, int32_t iIndex)
{
	C4Object *cObj; C4ObjectLink *clnk;
	for (clnk = Objects.First; clnk && (cObj = clnk->Obj); clnk = clnk->Next)
		// Status
		if (cObj->Status)
			// Base
			if (cObj->Base == iPlayer)
				// Index
				if (iIndex == 0) return cObj;
				else iIndex--;
	// Not found
	return nullptr;
}

C4Object *C4Section::FindFriendlyBase(int32_t iPlayer, int32_t iIndex)
{
	C4Object *cObj; C4ObjectLink *clnk;
	for (clnk = Objects.First; clnk && (cObj = clnk->Obj); clnk = clnk->Next)
		// Status
		if (cObj->Status)
			// Base
			if (ValidPlr(cObj->Base))
				// friendly Base
				if (!Hostile(cObj->Base, iPlayer))
					// Index
					if (iIndex == 0) return cObj;
					else iIndex--;
	// Not found
	return nullptr;
}

C4Object *C4Section::FindObjectByCommand(int32_t iCommand, C4Object *pTarget, C4Value iTx, int32_t iTy, C4Object *pTarget2, C4Object *pFindNext)
{
	C4Object *cObj; C4ObjectLink *clnk;
	for (clnk = Objects.First; clnk && (cObj = clnk->Obj); clnk = clnk->Next)
	{
		// find next
		if (pFindNext) { if (cObj == pFindNext) pFindNext = nullptr; continue; }
		// Status
		if (cObj->Status)
			// Check commands
			for (C4Command *pCommand = cObj->Command; pCommand; pCommand = pCommand->Next)
				// Command
				if (pCommand->Command == iCommand)
					// Target
					if (!pTarget || (pCommand->Target == pTarget))
						// Position
						if ((!iTx && !iTy) || ((pCommand->Tx == iTx) && (pCommand->Ty == iTy)))
							// Target2
							if (!pTarget2 || (pCommand->Target2 == pTarget2))
								// Found
								return cObj;
	}
	// Not found
	return nullptr;
}

void C4Section::CastObjects(C4ID id, C4Object *pCreator, int32_t num, int32_t level, int32_t tx, int32_t ty, int32_t iOwner, int32_t iController)
{
	int32_t cnt;
	for (cnt = 0; cnt < num; cnt++)
	{
		// force argument evaluation order
		const auto r4 = itofix(Random(3) + 1);
		const auto r3 = FIXED10(Random(2 * level + 1) - level);
		const auto r2 = FIXED10(Random(2 * level + 1) - level);
		const auto r1 = Random(360);
		CreateObject(id, pCreator, iOwner, tx, ty, r1, r2, r3, r4, iController);
	}
}

void C4Section::BlastCastObjects(C4ID id, C4Object *pCreator, int32_t num, int32_t tx, int32_t ty, int32_t iController)
{
	int32_t cnt;
	for (cnt = 0; cnt < num; cnt++)
	{
		// force argument evaluation order
		const auto r4 = itofix(Random(3) + 1);
		const auto r3 = FIXED10(Random(61) - 40);
		const auto r2 = FIXED10(Random(61) - 30);
		const auto r1 = Random(360);
		CreateObject(id, pCreator, NO_OWNER, tx, ty, r1, r2, r3, r4, iController);
	}
}

C4Object *C4Section::FindObject(C4ID id,
	int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt,
	uint32_t ocf,
	const char *szAction, C4Object *pActionTarget,
	C4Object *pExclude,
	C4Object *pContainer,
	int32_t iOwner,
	C4Object *pFindNext, C4ObjectList *const extraList)
{
	C4Object *pClosest = nullptr;
	int32_t iClosest = 0, iDistance, iFartherThan = -1;
	C4Object *cObj;
	C4ObjectLink *cLnk;
	C4Def *pDef;
	C4Object *pFindNextCpy = pFindNext;

	// check the easy cases first
	if (id != C4ID_None)
	{
		if (!(pDef = Game.Defs.ID2Def(id))) return nullptr; // no valid def
		if (!pDef->Count) return nullptr; // no instances at all
	}

	// Finding next closest: find closest but further away than last closest
	if (pFindNext && (iWdt == -1) && (iHgt == -1))
	{
		iFartherThan = (pFindNext->x - iX) * (pFindNext->x - iX) + (pFindNext->y - iY) * (pFindNext->y - iY);
		pFindNext = nullptr;
	}

	bool bFindActIdle = SEqual(szAction, "Idle") || SEqual(szAction, "ActIdle");

	std::array<C4ObjectLink *, 1> objectLink{Objects.First};
	C4Game::MultipleObjectLists lists{objectLink, extraList ? extraList->First : nullptr};

	// Scan all objects
	while ((cObj = lists.Next()))
	{
		// Not skipping to find next
		if (!pFindNext)
			// ID
			if ((id == C4ID_None) || (cObj->Def->id == id))
				// OCF (match any specified)
				if (cObj->OCF & ocf)
					// Exclude
					if (cObj != pExclude)
						// Action
						if (!szAction || !szAction[0] || (bFindActIdle && cObj->Action.Act <= ActIdle) || ((cObj->Action.Act > ActIdle) && SEqual(szAction, cObj->Def->ActMap[cObj->Action.Act].Name)))
							// ActionTarget
							if (!pActionTarget || ((cObj->Action.Act > ActIdle) && ((cObj->Action.Target == pActionTarget) || (cObj->Action.Target2 == pActionTarget))))
								// Container
								if (!pContainer || (cObj->Contained == pContainer) || ((reinterpret_cast<std::intptr_t>(pContainer) == NO_CONTAINER) && !cObj->Contained) || ((reinterpret_cast<std::intptr_t>(pContainer) == ANY_CONTAINER) && cObj->Contained))
									// Owner
									if ((iOwner == ANY_OWNER) || (cObj->Owner == iOwner))
										// Area
									{
										// Full range
										if ((iX == 0) && (iY == 0) && (iWdt == 0) && (iHgt == 0))
											return cObj;
										// Point
										if ((iWdt == 0) && (iHgt == 0))
										{
											if (Inside<int32_t>(iX - (cObj->x + cObj->Shape.x), 0, cObj->Shape.Wdt - 1))
												if (Inside<int32_t>(iY - (cObj->y + cObj->Shape.y), 0, cObj->Shape.Hgt - 1))
													return cObj;
											continue;
										}
										// Closest
										if ((iWdt == -1) && (iHgt == -1))
										{
											iDistance = (cObj->x - iX) * (cObj->x - iX) + (cObj->y - iY) * (cObj->y - iY);
											// same distance?
											if ((iDistance == iFartherThan) && !pFindNextCpy)
												return cObj;
											// nearer than/first closest?
											if (!pClosest || (iDistance < iClosest))
												if (iDistance > iFartherThan)
												{
													pClosest = cObj; iClosest = iDistance;
												}
										}
										// Range
										else if (Inside<int32_t>(cObj->x - iX, 0, iWdt - 1) && Inside<int32_t>(cObj->y - iY, 0, iHgt - 1))
											return cObj;
									}

		// Find next mark reached
		if (cObj == pFindNextCpy) pFindNext = pFindNextCpy = nullptr;
	}

	return pClosest;
}

C4Object *C4Section::FindVisObject(int32_t tx, int32_t ty, int32_t iPlr, const C4Facet &fctViewport,
	int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt,
	uint32_t ocf,
	C4Object *pExclude,
	int32_t iOwner,
	C4Object *pFindNext)
{
	// FIXME: Use C4FindObject here for optimization
	C4Object *cObj; C4ObjectLink *cLnk; C4ObjectList *pLst = &Objects.ForeObjects;

	// scan all object lists separately
	while (pLst)
	{
		// Scan all objects in list
		for (cLnk = Objects.First; cLnk && (cObj = cLnk->Obj); cLnk = cLnk->Next)
		{
			// Not skipping to find next
			if (!pFindNext)
				// Status
				if (cObj->Status == C4OS_NORMAL)
					// exclude fore/back-objects from main list
					if ((pLst != &Objects) || (!(cObj->Category & C4D_BackgroundOrForeground)))
						// exclude MouseIgnore-objects
						if (~cObj->Category & C4D_MouseIgnore)
							// OCF (match any specified)
							if (cObj->OCF & ocf)
								// Exclude
								if (cObj != pExclude)
									// Container
									if (!cObj->Contained)
										// Owner
										if ((iOwner == ANY_OWNER) || (cObj->Owner == iOwner))
											// Visibility
											if (!cObj->Visibility || cObj->IsVisible(iPlr, false))
												// Area
											{
												// Layer check: Layered objects are invisible to players whose cursor is in another layer
												if (ValidPlr(iPlr))
												{
													C4Object *pCursor = Game.Players.Get(iPlr)->Cursor;
													if (!pCursor || (pCursor->pLayer != cObj->pLayer)) continue;
												}
												// Full range
												if ((iX == 0) && (iY == 0) && (iWdt == 0) && (iHgt == 0))
													return cObj;
												// get object position
												int32_t iObjX, iObjY; cObj->GetViewPos(iObjX, iObjY, tx, ty, fctViewport);
												// Point search
												if ((iWdt == 0) && (iHgt == 0))
												{
													if (Inside<int32_t>(iX - (iObjX + cObj->Shape.x), 0, cObj->Shape.Wdt - 1))
														if (Inside<int32_t>(iY - (iObjY + cObj->Shape.y - cObj->addtop()), 0, cObj->Shape.Hgt + cObj->addtop() - 1))
															return cObj;
													continue;
												}
												// Range
												if (Inside<int32_t>(iObjX - iX, 0, iWdt - 1) && Inside<int32_t>(iObjY - iY, 0, iHgt - 1))
												{
													return cObj;
												}
											}

			// Find next mark reached
			if (cObj == pFindNext) pFindNext = nullptr;
		}
		// next list
		if (pLst == &Objects.ForeObjects) pLst = &Objects;
		else if (pLst == &Objects) pLst = &Objects.BackObjects;
		else pLst = nullptr;
	}

	// none found
	return nullptr;
}

int32_t C4Section::ObjectCount(C4ID id,
	int32_t x, int32_t y, int32_t wdt, int32_t hgt,
	uint32_t ocf,
	const char *szAction, C4Object *pActionTarget,
	C4Object *pExclude,
	C4Object *pContainer,
	int32_t iOwner, C4ObjectList *const extraList)
{
	int32_t iResult = 0; C4Def *pDef;
	// check the easy cases first
	if (id != C4ID_None)
	{
		if (!(pDef = Game.Defs.ID2Def(id))) return 0; // no valid def
		if (!pDef->Count) return 0; // no instances at all
		if (!x && !y && !wdt && !hgt && ocf == OCF_All && !szAction && !pActionTarget && !pExclude && !pContainer && (iOwner == ANY_OWNER))
			// plain id-search: return known count
			return pDef->Count;
	}
	C4Object *cObj;
	bool bFindActIdle = SEqual(szAction, "Idle") || SEqual(szAction, "ActIdle");

	std::array<C4ObjectLink *, 1> objectLink{Objects.First};
	C4Game::MultipleObjectLists lists{objectLink, extraList ? extraList->First : nullptr};

	while ((cObj = lists.Next()))
		// ID
		if ((id == C4ID_None) || (cObj->Def->id == id))
			// OCF
			if (cObj->OCF & ocf)
				// Exclude
				if (cObj != pExclude)
					// Action
					if (!szAction || !szAction[0] || (bFindActIdle && cObj->Action.Act <= ActIdle) || ((cObj->Action.Act > ActIdle) && SEqual(szAction, cObj->Def->ActMap[cObj->Action.Act].Name)))
						// ActionTarget
						if (!pActionTarget || ((cObj->Action.Act > ActIdle) && ((cObj->Action.Target == pActionTarget) || (cObj->Action.Target2 == pActionTarget))))
							// Container
							if (!pContainer || (cObj->Contained == pContainer) || ((reinterpret_cast<std::intptr_t>(pContainer) == NO_CONTAINER) && !cObj->Contained) || ((reinterpret_cast<std::intptr_t>(pContainer) == ANY_CONTAINER) && cObj->Contained))
								// Owner
								if ((iOwner == ANY_OWNER) || (cObj->Owner == iOwner))
									// Area
								{
									// Full range
									if ((x == 0) && (y == 0) && (wdt == 0) && (hgt == 0))
									{
										iResult++; continue;
									}
									// Point
									if ((wdt == 0) && (hgt == 0))
									{
										if (Inside<int32_t>(x - (cObj->x + cObj->Shape.x), 0, cObj->Shape.Wdt - 1))
											if (Inside<int32_t>(y - (cObj->y + cObj->Shape.y), 0, cObj->Shape.Hgt - 1))
											{
												iResult++; continue;
											}
										continue;
									}
									// Range
									if (Inside<int32_t>(cObj->x - x, 0, wdt - 1) && Inside<int32_t>(cObj->y - y, 0, hgt - 1))
									{
										iResult++; continue;
									}
								}

	return iResult;
}

void C4Section::SyncClearance()
{
	PXS.SyncClearance();
	Objects.SyncClearance();
}

void C4Section::Synchronize()
{
	Landscape.Synchronize();
	MassMover.Synchronize();
	PXS.Synchronize();
	Objects.Synchronize();
}

void C4Section::SynchronizeTransferZones()
{
	TransferZones.Clear();
	Objects.UpdateTransferZones();
}

void C4Section::UpdateRootParent()
{
	if (Parent)
	{
		C4Section *section{Parent};
		while (section->Parent)
		{
			section = section->Parent;
		}

		RootParent = section;
	}
	else
	{
		RootParent = nullptr;
	}
}

C4Section &C4Section::GetRootSection() noexcept
{
	return RootParent ? *RootParent : *this;
}

bool C4Section::IsChildOf(const C4Section &section) const noexcept
{
	for (const C4Section *sectionPtr{this}; sectionPtr->Parent; sectionPtr = sectionPtr->Parent)
	{
		if (sectionPtr->Parent == &section)
		{
			return true;
		}
	}

	return false;
}

void C4Section::PointToParentPoint(std::int32_t &x, std::int32_t &y, const C4Section *untilParent) const noexcept
{
	if (this == untilParent || !Parent)
	{
		return;
	}

	if (!untilParent)
	{
		untilParent = RootParent;
	}

	for (const C4Section *sectionPtr{this}; sectionPtr != untilParent; sectionPtr = sectionPtr->Parent)
	{
		x += sectionPtr->RenderAsChildBounds.x;
		y += sectionPtr->RenderAsChildBounds.y;
	}
}

void C4Section::ParentPointToPoint(std::int32_t &x, std::int32_t &y, const C4Section *untilParent) const noexcept
{
	if (this == untilParent || !Parent)
	{
		return;
	}

	if (!untilParent)
	{
		untilParent = RootParent;
	}

	for (const C4Section *sectionPtr{this}; sectionPtr != untilParent; sectionPtr = sectionPtr->Parent)
	{
		x -= sectionPtr->RenderAsChildBounds.x;
		y -= sectionPtr->RenderAsChildBounds.y;
	}
}

std::tuple<C4Section &, std::int32_t, std::int32_t> C4Section::PointToChildPoint(std::int32_t x, std::int32_t y) noexcept
{
	C4Section *childPtr{this};

	for (;;)
	{

		for (const auto &child : Children)
		{
			if (const auto &bounds = child->RenderAsChildBounds; bounds.Contains(x, y))
			{
				childPtr = *child;
				x -= bounds.x;
				y -= bounds.y;
				goto next;
			}
		}

		break;

	next:
		continue;
	}

	return {*childPtr, x, y};
}

void C4Section::CompileFunc(StdCompiler *const comp, const bool mainSection)
{
	if (!mainSection)
	{
		auto name = comp->Name("Section");
		comp->Value(mkNamingAdapt(emptyLandscape, "EmptyLandscape"));
		if (emptyLandscape)
		{
			comp->Value(mkNamingAdapt(mkParAdapt(C4S, false), "Scenario"));
		}
		else
		{
			comp->Value(mkNamingAdapt(this->name, "Template"));
		}
	}

	comp->Value(mkNamingAdapt(Weather,       "Weather"));
	comp->Value(mkNamingAdapt(Landscape,     "Landscape"));
	comp->Value(mkNamingAdapt(Landscape.Sky, "Sky"));
}

std::unique_ptr<C4Section> C4Section::FromSaveGame(C4Group &scenario, const std::string_view entryName)
{
	const std::size_t separator{entryName.find('-')};
	const auto numberSubstr = entryName.substr(separator + 1, entryName.size() - separator - 4);

	std::uint32_t number;
	if (std::from_chars(numberSubstr.data(), numberSubstr.data() + numberSubstr.size(), number).ec != std::errc{})
	{
		return nullptr;
	}

	std::unique_ptr<C4Section> section{new C4Section{number}};
	if (section->LoadSaveGame(scenario, entryName))
	{
		return section;
	}

	return nullptr;
}

void C4Section::ClearObjectPtrs(C4Object *const obj)
{
	// May not call Objects.ClearPointers() because that would
	// remove pObj from primary list and pObj is to be kept
	// until CheckObjectRemoval().
	C4Object *cObj; C4ObjectLink *clnk;
	for (clnk = Objects.First; clnk && (cObj = clnk->Obj); clnk = clnk->Next)
		cObj->ClearPointers(obj);
	// check in inactive objects as well
	for (clnk = Objects.InactiveObjects.First; clnk && (cObj = clnk->Obj); clnk = clnk->Next)
		cObj->ClearPointers(obj);
}

void C4Section::ClearSectionPointers(C4Section &section)
{
	if (Parent == &section)
	{
		Parent = nullptr;
		RootParent = nullptr;
	}
}

// Deletes removal-assigned data from list.
// Pointer clearance is done by AssignRemoval.

void C4Section::ObjectRemovalCheck() // Every Tick255 by ExecObjects
{
	C4Object *cObj; C4ObjectLink *clnk, *next;
	for (clnk = Objects.First; clnk && (cObj = clnk->Obj); clnk = next)
	{
		next = clnk->Next;
		if (!cObj->Status && (cObj->RemovalDelay == 0))
		{
			Objects.Remove(cObj);
			delete cObj;
		}
	}
}

void C4Section::ResetEnumerationIndex() noexcept
{
	assert(!Game.IsRunning);
	enumerationIndex = 0;
}

std::uint32_t C4Section::AcquireEnumerationIndex() noexcept
{
	const std::uint32_t index{enumerationIndex};

	if (++enumerationIndex == NoSectionSentinel)
	{
		++enumerationIndex;
	}

	return index;
}

void C4Section::AdjustEnumerationIndex(const std::uint32_t index) noexcept
{
	enumerationIndex = index;
}
