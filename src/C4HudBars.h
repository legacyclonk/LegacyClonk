/*
 * LegacyClonk
 *
 * Copyright (c) 2021, The LegacyClonk Team and contributors
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

/* Allows to define custom energy bars */

#pragma once

#include "C4FacetEx.h"
#include "C4Id.h"
#include "C4Value.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class C4Object;

class C4HudBarException : public std::runtime_error
{
public:
	using runtime_error::runtime_error;
};

struct C4HudBar
{
	static constexpr std::int32_t Maximum{1000000};

	std::int32_t Value{0};
	std::int32_t Max{Maximum};
	bool Visible{true};

	bool operator==(const C4HudBar &rhs) const noexcept = default;

	void CompileFunc(StdCompiler *comp);
};

class C4HudBarDef
{
public:
	enum class Physical
	{
		None = 0,
		Energy = 1,
		Magic = 2,
		Breath = 3,
		First = None,
		Last = Breath
	};

	enum class Hide
	{
		Never = 0,
		AsDef = 0x1, // according to C4Def::HideHUDBars; otherwise HideHUDBars is ignored
		Empty = 0x2,
		Full = 0x4,
		EmptyFull = Empty | Full,
		All = EmptyFull | AsDef
	};

public:
	C4HudBarDef() noexcept = default;
	C4HudBarDef(std::string_view name, std::string_view file, std::shared_ptr<C4FacetExID> gfx, std::uint32_t index, Physical physical = Physical::None);

	bool operator==(const C4HudBarDef &rhs) const noexcept;

	static Hide DefaultHide(Physical physical) noexcept;
	static std::int32_t DefaultIndex(Physical physical) noexcept;

	std::size_t GetHash() const noexcept;
	void CompileFunc(StdCompiler *comp);

public:
	std::string name;
	Physical physical{Physical::None};
	Hide hide{Hide::Empty};

	std::string gfx;
	std::shared_ptr<C4FacetExID> facet; // calculated from gfx
	std::uint32_t index{0};
	bool advance{true};

	std::int32_t valueIndex{-1};
	std::int32_t value{0};
	std::int32_t max{C4HudBar::Maximum};
	bool visible{true};
	float scale{1.0f}; // calculated from gfx.scale
};


constexpr C4HudBarDef::Hide operator &(const C4HudBarDef::Hide a, const C4HudBarDef::Hide b) noexcept
{
	return static_cast<C4HudBarDef::Hide>(std::to_underlying(a) & std::to_underlying(b));
}

constexpr C4HudBarDef::Hide operator|(const C4HudBarDef::Hide a, const C4HudBarDef::Hide b) noexcept
{
	using und_t = std::underlying_type_t<C4HudBarDef::Hide>;
	return static_cast<C4HudBarDef::Hide>(static_cast<und_t>(a) | static_cast<und_t>(b));
}

constexpr C4HudBarDef::Hide operator~(const C4HudBarDef::Hide a) noexcept
{
	return static_cast<C4HudBarDef::Hide>(~std::to_underlying(a));
}

class C4HudBarsDef
{
public:
	struct Gfx
	{
		std::string key;
		std::string file;
		std::int32_t amount{0};
		std::int32_t scale{0};

		Gfx() noexcept = default;
		Gfx(std::string key, std::string file, std::int32_t amount, std::int32_t scale) noexcept;

		bool operator==(const Gfx &rhs) const noexcept;

		void CompileFunc(StdCompiler *comp);
	};

	using Gfxs = std::map<std::string, Gfx, std::less<>>;
	using Bars = std::vector<C4HudBarDef>;
	using Names = std::unordered_map<std::string, std::uint32_t, C4TransparentHash, std::equal_to<>>;

	C4HudBarsDef() = default;
	C4HudBarsDef(Gfxs &&gfxs, Bars &&bars);
	C4HudBarsDef(const Gfxs &gfxs, const Bars &bars, const Names &names);

	C4HudBarsDef(const C4HudBarsDef &other) = default;
	C4HudBarsDef(C4HudBarsDef &&other) = default;
	C4HudBarsDef &operator=(const C4HudBarsDef &other) = default;
	C4HudBarsDef &operator=(C4HudBarsDef &&other) = default;

	static void PopulateNamesFromValues(const std::function<void (std::string)> &error, const Bars &bars, Names &names);

	bool operator==(const C4HudBarsDef &rhs) const noexcept;

	std::size_t GetHash() const noexcept;

public:
	// the definiton is processed and flattened into a vector of energy bar values
	// they contain everything needed to draw the energy bars
	Gfxs gfxs;
	Bars bars;
	Names names;
};

class C4HudBars
{
public:
	std::shared_ptr<const C4HudBarsDef> def;
	std::vector<C4HudBar> values;

	C4HudBars(std::shared_ptr<const C4HudBarsDef> def) noexcept;

	void DrawHudBars(C4Facet &cgo, C4Object &obj) const noexcept;
	void SetHudBarValue(std::string_view name, std::int32_t value, std::int32_t max = 0);
	void SetHudBarVisibility(std::string_view name, bool visible);

private:
	C4HudBar &BarVal(const char *functionName, std::string_view name);
};

namespace std
{
	template<>
	struct hash<const C4HudBarsDef>
	{
		std::size_t operator()(const C4HudBarsDef &value) const noexcept;
	};
}

class C4HudBarsUniquifier
{
public:
	bool LoadDefaultBars();
	void Clear();

	const std::shared_ptr<C4HudBars> &DefaultBars() const noexcept { return defaultBars; }
	std::shared_ptr<C4FacetExID> GetFacet(const std::function<void(std::string)> &error, const C4HudBarsDef::Gfxs &gfx, std::string_view file);
	std::shared_ptr<const C4HudBarsDef> UniqueifyDefinition(std::unique_ptr<C4HudBarsDef> definition);
	std::shared_ptr<C4HudBars> Instantiate(std::shared_ptr<const C4HudBarsDef> definition);
	std::shared_ptr<C4HudBars> DefineHudBars(C4ValueHash &graphics, const C4ValueArray &definition);

private:
	void ProcessGraphics(C4ValueHash &map, C4HudBarsDef::Gfxs &gfx);
	void ProcessGroup(std::int32_t &valueIndex, const C4HudBarsDef::Gfxs &graphics, const C4ValueArray &group, C4HudBarsDef::Bars &bars, bool advanceAlways);
	void ProcessHudBar(std::int32_t &valueIndex, const C4HudBarsDef::Gfxs &graphics, const C4ValueHash &bar, C4HudBarsDef::Bars &bars, bool advance);

	using C4HudBarsDefRef = std::reference_wrapper<const C4HudBarsDef>;
	using Definitions = std::unordered_map<C4HudBarsDefRef, std::weak_ptr<const C4HudBarsDef>, std::hash<const C4HudBarsDef>, std::equal_to<const C4HudBarsDef>>;

	std::unordered_map<std::string, std::weak_ptr<C4FacetExID>, C4TransparentHash, std::equal_to<>> graphics;
	Definitions definitions;

	std::shared_ptr<C4HudBars> defaultBars;
};


class C4HudBarsAdapt
{
protected:
	std::shared_ptr<C4HudBars> &bars;

public:
	C4HudBarsAdapt(std::shared_ptr<C4HudBars> &bars) : bars{bars} {}
	void CompileFunc(StdCompiler *comp);

	// Default checking / setting
	bool operator==(std::shared_ptr<C4HudBars> pDefault) { return bars == pDefault; }
	void operator=(std::shared_ptr<C4HudBars> pDefault) { bars = pDefault; }
};
