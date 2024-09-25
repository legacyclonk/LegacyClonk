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

#include "C4HudBars.h"

#include "C4Def.h"
#include "C4Game.h"
#include "C4Log.h"
#include "C4Object.h"
#include "C4ValueList.h"
#include "C4ValueHash.h"
#include "C4Wrappers.h"

#include "StdHelpers.h"

#include <format>

void C4HudBar::CompileFunc(StdCompiler *const comp)
{
	comp->Value(mkNamingAdapt(Value, "Value", 0));
	comp->Value(mkNamingAdapt(Max, "Max", DefaultMaximum));
	comp->Value(mkNamingAdapt(Visible, "Visible", true));
}

C4HudBars::C4HudBars(std::shared_ptr<const C4HudBarsDef> def) noexcept : def{std::move(def)}
{
	for (const auto &bardef : this->def->bars)
	{
		if (bardef.physical == C4HudBarDef::Physical::None)
			values.emplace_back(bardef.value, bardef.max, bardef.visible);
	}
}

C4HudBar &C4HudBars::BarVal(const std::string_view name)
{
	if (const auto index = def->names.find(name); index != def->names.end())
	{
		if (index->second < def->bars.size())
		{
			auto &bardef = def->bars[index->second];
			if (bardef.valueIndex >= 0)
			{
				if (std::cmp_less(bardef.valueIndex, values.size()))
				{
					return values.at(bardef.valueIndex);
				}
				else
				{
					throw C4HudBarException{std::format("bar \"{}\" value index out of range.", name)};
				}
			}
			else
			{
				throw C4HudBarException{std::format("bar \"{}\" is based is based on physical and can not be set directly.", name)};
			}
		}
	}

	throw C4HudBarException{std::format("bar \"{}\" was not defined.", name)};
}

void C4HudBars::SetValue(const std::string_view name, const std::int32_t value, const std::int32_t max)
{
	C4HudBar &barVal{BarVal(name)};
	barVal.Value = value;
	if(max > 0) barVal.Max = max;
}

void C4HudBars::SetVisibility(const std::string_view name, const bool visible)
{
	BarVal(name).Visible = visible;
}

void C4HudBars::Draw(C4Facet &cgo, C4Object &obj) const noexcept
{
	bool needsAdvance = false;
	std::int32_t maxWidth = 0;

	for (const auto &bardef : def->bars)
	{
		std::int32_t value{0};
		std::int32_t max{0};
		bool visible{true};
		const auto hideHUDBars = static_cast<bool>(bardef.hide & C4HudBarDef::Hide::AsDef);

		switch (bardef.physical)
		{
		case C4HudBarDef::Physical::Energy:
			if (hideHUDBars && obj.Def->HideHUDBars & C4Def::HB_Energy)
			{
				visible = false;
			}

			value = obj.Energy;
			max = obj.GetPhysical()->Energy;
			break;

		case C4HudBarDef::Physical::Breath:
			if (hideHUDBars && obj.Def->HideHUDBars & C4Def::HB_Breath)
			{
				visible = false;
			}

			value = obj.Breath;
			max = obj.GetPhysical()->Breath;
			break;

		case C4HudBarDef::Physical::Magic:
			if (hideHUDBars && obj.Def->HideHUDBars & C4Def::HB_MagicEnergy)
			{
				visible = false;
			}
			// draw in units of MagicPhysicalFactor, so you can get a full magic energy bar by script even if partial magic energy training is not fulfilled
			value = obj.MagicEnergy / MagicPhysicalFactor;
			max = obj.GetPhysical()->Magic / MagicPhysicalFactor;
			break;

		default:
			const auto &barval = values.at(bardef.valueIndex);
			value = barval.Value;
			max = barval.Max;
			visible = barval.Visible;
			break;
		}

		if (((bardef.hide & C4HudBarDef::Hide::Empty) == C4HudBarDef::Hide::Empty && value == 0) || ((bardef.hide & C4HudBarDef::Hide::Full) == C4HudBarDef::Hide::Full && value >= max))
		{
			visible = false;
		}

		if (!visible)
		{
			if (needsAdvance && bardef.advance)
			{
				cgo.X += maxWidth;
				maxWidth = 0;
				needsAdvance = false;
			}
			continue;
		}

		const std::int32_t width{bardef.facet->Wdt};
		cgo.Wdt = width;
		cgo.DrawEnergyLevelEx(value, max, *bardef.facet, bardef.index, bardef.scale);

		maxWidth = std::max<std::int32_t>(maxWidth, width + 1);

		if (bardef.advance)
		{
			cgo.X += maxWidth;
			maxWidth = 0;
			needsAdvance = false;
		}
		else
		{
			needsAdvance = true;
		}
	}
}

C4HudBarDef::C4HudBarDef(const std::string_view name, const std::string_view gfx, std::shared_ptr<C4FacetExID> facet, const std::uint32_t index, const Physical physical) :
	name{name}, physical{physical}, hide{DefaultHide(physical)},
	gfx{gfx}, facet{std::move(facet)}, index{index}
{}

bool C4HudBarDef::operator==(const C4HudBarDef &rhs) const noexcept
{
	return
		name == rhs.name &&
		physical == rhs.physical &&
		hide == rhs.hide &&
		gfx == rhs.gfx &&
		facet == rhs.facet &&
		index == rhs.index &&
		advance == rhs.advance &&
		value == rhs.value &&
		max == rhs.max &&
		visible == rhs.visible;
}

C4HudBarDef::Hide C4HudBarDef::DefaultHide(const C4HudBarDef::Physical physical) noexcept
{
	switch (physical)
	{
	case Physical::Energy:
		return Hide::Never | Hide::AsDef;

	case Physical::Breath:
		return Hide::Full | Hide::AsDef;

	case Physical::Magic:
	default:
		return Hide::Empty | Hide::AsDef;
	}
}

int32_t C4HudBarDef::DefaultIndex(const C4HudBarDef::Physical physical) noexcept
{
	switch (physical)
	{
	case Physical::Energy:
		return 0;

	case Physical::Magic:
		return 1;

	case Physical::Breath:
		return 2;

	default:
		return 0;
	}
}

std::size_t C4HudBarDef::GetHash() const noexcept
{
	return HashArguments(name, physical, hide, gfx, index, advance, valueIndex, value, max, visible);
}

void C4HudBarDef::CompileFunc(StdCompiler *const comp)
{
	comp->Value(mkNamingAdapt(name, "Name"));
	StdEnumEntry<Physical> PhysicalEntries[]
	{
		{ "Energy", Physical::Energy },
		{ "Magic", Physical::Magic },
		{ "Breath", Physical::Breath },
	};
	comp->Value(mkNamingAdapt(mkEnumAdaptT<std::uint8_t>(physical, PhysicalEntries), "Physical", Physical::None));
	StdBitfieldEntry<std::uint8_t> HideEntries[]
	{
		{ "HideHud", std::to_underlying(Hide::AsDef) },
		{ "Empty", std::to_underlying(Hide::Empty) },
		{ "Full", std::to_underlying(Hide::Full) },
	};

	auto hideVal = static_cast<std::uint8_t>(hide);
	comp->Value(mkNamingAdapt(mkBitfieldAdapt<std::uint8_t>(hideVal, HideEntries), "Hide", Hide::Empty));
	hide = static_cast<Hide>(hideVal);

	comp->Value(mkNamingAdapt(gfx, "Gfx"));
	comp->Value(mkNamingAdapt(index, "Index"));
	comp->Value(mkNamingAdapt(advance, "Advance", true));
	comp->Value(mkNamingAdapt(valueIndex, "ValueIndex", -1));
	comp->Value(mkNamingAdapt(value, "Value", 0));
	comp->Value(mkNamingAdapt(max, "Max", C4HudBar::DefaultMaximum));
	comp->Value(mkNamingAdapt(visible, "Visible", true));
	// gfx and scale are restored from def.gfxs
}

namespace
{
	template <typename Map>
	bool MapCompare(const Map &lhs, const Map &rhs) noexcept
	{
		return lhs.size() == rhs.size()
			&& std::equal(lhs.begin(), lhs.end(), rhs.begin());
	}
}

void C4HudBarsDef::Gfx::CompileFunc(StdCompiler *const comp)
{
	comp->Value(mkNamingAdapt(Key, "Key"));
	comp->Value(mkNamingAdapt(File, "File"));
	comp->Value(mkNamingAdapt(Amount, "Amount"));
	comp->Value(mkNamingAdapt(Scale, "Scale"));
}

C4HudBarsDef::C4HudBarsDef(Gfxs &&gfxs, Bars &&bars) : gfxs{std::move(gfxs)}, bars{std::move(bars)}
{
	PopulateNamesFromValues([](std::string msg) { LogFatalNTr(msg); }, this->bars, names);
}

C4HudBarsDef::C4HudBarsDef(const Gfxs &gfxs, const C4HudBarsDef::Bars &bars, const C4HudBarsDef::Names &names) : gfxs{gfxs}, bars{bars}, names{names} {}

void C4HudBarsDef::PopulateNamesFromValues(const std::function<void(std::string)> &error, const C4HudBarsDef::Bars &bars, C4HudBarsDef::Names &names)
{
	std::int32_t i{0};
	for (const auto &bar : bars)
	{
		const auto success = names.emplace(bar.name, i);
		if (!success.second)
		{
			error(std::format("C4HudBarsDef {} definition, names must be unique, duplicate detected", bar.name));
		}
		++i;
	}
}

bool C4HudBarsDef::operator==(const C4HudBarsDef &rhs) const noexcept
{
	return MapCompare(gfxs, rhs.gfxs) && bars == rhs.bars;
}

std::size_t C4HudBarsDef::GetHash() const noexcept
{
	std::size_t result{0};
	for (const auto &gfx : gfxs)
	{
		HashCombineArguments(result, gfx.second.Key, gfx.second.File, gfx.second.Amount, gfx.second.Scale);
	}

	for (const auto &bardef : bars)
	{
		HashCombine(result, bardef.GetHash());
	}

	return result;
}

std::size_t std::hash<const C4HudBarsDef>::operator()(const C4HudBarsDef &value) const noexcept
{
	return value.GetHash();
}

bool C4HudBarsUniquifier::LoadDefaultBars()
{
	static constexpr auto File = "EnergyBars";

	C4HudBarsDef::Gfxs gfxs{{File, C4HudBarsDef::Gfx{File, File, 3, 100}}};

	const auto gfx = GetFacet([](std::string msg) { LogFatalNTr(std::format("Could not load default hud bars \"{}\": {}", File, msg)); }, gfxs, File);
	if (!gfx)
	{
		return false;
	}

	defaultBars = RegisterAndCreateInstance
	(
		std::make_unique<C4HudBarsDef>
		(
			std::move(gfxs),
			C4HudBarsDef::Bars
			{
				C4HudBarDef{"Energy", File, gfx, 0, C4HudBarDef::Physical::Energy},
				C4HudBarDef{"Magic", File, gfx, 1, C4HudBarDef::Physical::Magic},
				C4HudBarDef{"Breath", File, gfx, 2, C4HudBarDef::Physical::Breath}
			}
		)
	);

	return true;
}

void C4HudBarsUniquifier::Clear()
{
	defaultBars.reset();
	assert(graphics.empty());
	assert(definitions.empty());
}

std::shared_ptr<C4FacetExID> C4HudBarsUniquifier::GetFacet(const std::function<void(std::string)> &error, const C4HudBarsDef::Gfxs &gfxs, const std::string_view gfx)
{
	if (const auto it = graphics.find(gfx); it != graphics.end())
	{
		auto facet = it->second.lock();
		if (facet)
		{
			return facet;
		}
	}

	// facet needs to be loaded
	std::int32_t amount;
	std::int32_t scale;
	std::string_view file;

	if (const auto it = gfxs.find(gfx); it != gfxs.end())
	{
		amount = it->second.Amount;
		scale = it->second.Scale;
		file = it->second.File;
	}
	else
	{
		error(std::format("missing key \"{}\" in graphics definition", gfx));
		return nullptr;
	}

	const auto deleter = [key{std::string{gfx}}, this](C4FacetExID *facet)
	{
		graphics.erase(key);
		delete facet;
	};

	const std::shared_ptr<C4FacetExID> facet{new C4FacetExID, deleter};
	const bool success{Game.GraphicsResource.LoadFile(*facet, file.data(), Game.GraphicsResource.Files)};
	if(!success)
	{
		error(std::format("could not load hud bar graphic \"{}\"", file));
		return nullptr;
	}

	const std::int32_t barWdt{facet->Surface->Wdt / (amount * 2)};
	const std::int32_t barHgt{facet->Surface->Hgt / 3};
	const std::int32_t scaledWdt{(barWdt*100) / scale};
	const std::int32_t scaledHgt{(barHgt*100) / scale};

	facet->Set(facet->Surface, 0, 0, scaledWdt, scaledHgt);

	graphics.emplace(gfx, std::weak_ptr<C4FacetExID>{facet});
	return facet;
}

std::shared_ptr<C4HudBars> C4HudBarsUniquifier::RegisterAndCreateInstance(std::unique_ptr<C4HudBarsDef> definition)
{
	// definition always gets owned by a shared_ptr,
	// that either ends up being the canonical shared_ptr,
	// or goes out of scope deleting the definition.
	// the weak ptr remembers the custom deleter
	const auto deleter = [this](const C4HudBarsDef *def)
	{
		definitions.erase(*def);
		delete def;
	};

	std::shared_ptr<const C4HudBarsDef> shared{definition.release(), deleter};
	const auto &[it, success] = definitions.emplace(*shared.get(), std::weak_ptr<const C4HudBarsDef>(shared));
	if (!success)
	{
		// definition already existed, create shared ptr from existing weak_ptr
		return std::make_shared<C4HudBars>(it->second.lock());
	}
	return std::make_shared<C4HudBars>(std::move(shared));
}

std::shared_ptr<C4HudBars> C4HudBarsUniquifier::DefineHudBars(C4ValueHash &graphics, const C4ValueArray &definition)
{
	std::int32_t valueIndex{0};
	C4HudBarsDef::Gfxs gfx;
	C4HudBarsDef::Bars bars;
	C4HudBarsDef::Names names;

	ProcessGraphics(graphics, gfx);
	ProcessGroup(valueIndex, gfx, definition, bars, true);

	const auto error = [](std::string msg) { throw C4HudBarException{std::move(msg)}; };
	C4HudBarsDef::PopulateNamesFromValues(error, bars, names);

	return RegisterAndCreateInstance(std::make_unique<C4HudBarsDef>(gfx, bars, names));
}

static std::string_view StdStrBufToStringView(const StdStrBuf &buf)
{
	if (buf.isNull())
	{
		return {};
	}

	return {buf.getData(), buf.getLength()};
}

static std::string_view StringToStringView(const C4String *const string)
{
	return StdStrBufToStringView(string->Data);
}

void C4HudBarsUniquifier::ProcessGraphics(C4ValueHash &map, C4HudBarsDef::Gfxs &gfx)
{
	const auto keyAmount = C4VString("Amount");
	const auto keyScale = C4VString("Scale");
	const auto keyFile = C4VString("File");

	for (const auto &[key, val] : map)
	{
		if (key.GetType() != C4V_String)
		{
			throw C4HudBarException{"keys within maps are expected to be of type string"};
		}

		const auto _key = key.GetRefVal()._getStr()->Data;

		if (val.GetType() != C4V_Map)
		{
			throw C4HudBarException{std::format("key \"{}\" is not a map, got: {}", StdStrBufToStringView(_key), val.GetDataString())};
		}

		const auto &m = *val.GetRefVal()._getMap();

		C4Value file{m[keyFile]};
		const auto _file = file ? file.getStr()->Data : _key;

		// TODO: Check Type and const?
		auto _amount = m[keyAmount];
		auto _scale = m[keyScale];
		std::int32_t amount{_amount.getInt()};
		std::int32_t scale{_scale.getInt()};
		if (amount == 0) amount = 1;
		if (scale == 0) scale = 100;

		if (const auto success = gfx.try_emplace(std::string{StdStrBufToStringView(_key)}, std::string{StdStrBufToStringView(_key)}, std::string{StdStrBufToStringView(_file)}, amount, scale).second; !success)
		{
			throw C4HudBarException{std::format("duplicate key \"{}\" in gfx description ", StdStrBufToStringView(_key))};
		}
	}
}

void C4HudBarsUniquifier::ProcessGroup(std::int32_t &valueIndex, const C4HudBarsDef::Gfxs &graphics, const C4ValueArray &group, C4HudBarsDef::Bars &bars, const bool advanceAlways)
{
	const auto error = [](const char *msg, const C4Value &val)
	{
		const std::string dataString{val.GetDataString()};
		throw C4HudBarException{std::vformat(msg, std::make_format_args(dataString))};
	};

	const std::int32_t size{group.GetSize()};

	for (std::int32_t i{0}; i < size; ++i)
	{
		const auto &element = group[i];
		switch (element.GetType())
		{
		case C4V_Map:
			if (const auto *map = element._getMap(); map)
			{
				ProcessHudBar(valueIndex, graphics, *map, bars, advanceAlways || i == size-1);
			}
			else
			{
				error("got unexpected value: {}", element);
			}
			break;

		case C4V_Array:
			if(advanceAlways)
			{
				if (const auto *array = element._getArray(); array)
				{
					ProcessGroup(valueIndex, graphics, *array, bars, false);
				}
				else
				{
					error("got unexpected value: {}", element);
				}
			}
			else
			{
				error("groups in groups are not allowed: {}", element);
			}
			break;

		default:
			error("array or map expected, got: {}", element);
		}
	}
}

void C4HudBarsUniquifier::ProcessHudBar(std::int32_t &valueIndex, const C4HudBarsDef::Gfxs &graphics, const C4ValueHash &bar, C4HudBarsDef::Bars &bars, const bool advance)
{
	auto name = bar[C4VString("Name")];
	const auto *_name = name.getStr();
	if (!_name)
	{
		throw C4HudBarException{std::format("HudBar definition has invalid Name, got: {}", name.GetDataString())};
	}

	const auto error = [_name](const char *property, C4Value &val)
	{
		throw C4HudBarException{std::format("\"{}\" definition has invalid {}, got {}", val.GetDataString(), property, StringToStringView(_name))};
	};

	C4Value gfx{bar[C4VString("Gfx")]};
	const auto *_gfx = gfx.getStr();
	if (!_gfx) error("Gfx", gfx);

	C4Value physical{bar[C4VString("Physical")]};
	auto _physical = static_cast<C4HudBarDef::Physical>(physical.getInt());
	if (_physical < C4HudBarDef::Physical::First || _physical > C4HudBarDef::Physical::Last) error("Physical", physical);

	C4Value hide{bar[C4VString("Hide")]};
	auto _hide = C4HudBarDef::Hide::Empty;
	if (hide) _hide = static_cast<C4HudBarDef::Hide>(hide.getInt());
	if ((_hide & ~C4HudBarDef::Hide::All) != C4HudBarDef::Hide::Never) error("Hide", hide);

	C4Value index{bar[C4VString("Index")]};
	C4Value value{bar[C4VString("Value")]};
	const auto _index = index.getInt();
	const auto _value = value.getInt();
	if (_index < 0) error("Index", index);
	if (_value < 0) error("Value", value);

	C4ValueInt _max{C4HudBar::DefaultMaximum};
	if (bar.contains(C4VString("Max")))
	{
		auto max = bar[C4VString("Max")];
		_max = max.getInt();
		if (_max < 0) error("Max", max);
	}

	bool _visible{true};
	if (bar.contains(C4VString("Visible")))
	{
		auto visible = bar[C4VString("Visible")];
		_visible = visible.getBool();
	}

	{
		const auto file = _gfx->Data;

		const auto facetError = [_name](std::string msg)
		{
			throw C4HudBarException{std::format("HudBar \"{}\" {}", StringToStringView(_name), msg)};
		};

		const auto facet = GetFacet(facetError, graphics, file.getData());

		C4HudBarDef bar{StringToStringView(_name), StdStrBufToStringView(file), facet, static_cast<std::uint32_t>(_index), _physical};

		if (physical)
		{
			bar.physical = _physical;
			bar.hide = C4HudBarDef::DefaultHide(_physical);
		}
		else
		{
			bar.valueIndex = valueIndex++;
		}

		if (hide) bar.hide = _hide;
		bar.value = _value;
		bar.max = _max;
		bar.visible = _visible;
		bar.advance = advance;
		auto scale = graphics.at(file.getData()).Scale;
		bar.scale = static_cast<float>(scale) / 100.0f;
		bars.push_back(bar);
	}
}


void C4HudBarsAdapt::CompileFunc(StdCompiler *const comp)
{
	if (!comp->isCompiler())
	{
		if (bars)
		{
			std::vector<C4HudBarsDef::Gfx> temp;
			for (const auto &it: bars->def->gfxs)
				temp.emplace_back(it.second);

			comp->Value(mkNamingAdapt(mkSTLContainerAdapt(temp), "Gfx", std::vector<C4HudBarsDef::Gfx>{}));

			comp->Value(mkNamingAdapt(mkSTLContainerAdapt(const_cast<std::vector<C4HudBarDef> &>(bars->def->bars)), "Def", C4HudBarsDef::Bars{}));
			comp->Value(mkNamingAdapt(mkSTLContainerAdapt(bars->values), "Bar", std::vector<C4HudBar>{}));
		}
	}
	else
	{
		C4HudBarsDef::Gfxs gfxs_;
		std::vector<C4HudBarsDef::Gfx> temp;
		comp->Value(mkNamingAdapt(mkSTLContainerAdapt(temp), "Gfx", std::vector<C4HudBarsDef::Gfx>{}));
		for (const auto &gfx : temp)
			gfxs_.emplace(gfx.Key, gfx);

		C4HudBarsDef::Bars bars_;
		comp->Value(mkNamingAdapt(mkSTLContainerAdapt(bars_), "Def", C4HudBarsDef::Bars{}));

		auto def = std::make_unique<C4HudBarsDef>(std::move(gfxs_), std::move(bars_));

		// get facets and restore scale from gfxs
		const auto facetError = [comp](std::string msg) { comp->Warn("Error loading HudBars {}", msg); };
		for (auto &bar : def->bars)
		{
			bar.facet = Game.HudBars.GetFacet(facetError, def->gfxs, bar.gfx.c_str());
			if (!bar.facet)
			{
				bars = Game.HudBars.DefaultBars();
				return;
			}
			bar.scale = static_cast<float>(def->gfxs.at(bar.gfx).Scale) / 100.0f;
		}

		const auto instance = Game.HudBars.RegisterAndCreateInstance(std::move(def));
		comp->Value(mkNamingAdapt(mkSTLContainerAdapt(instance->values), "Bar", std::vector<C4HudBar>{}));

		bars = instance;
	}
}
