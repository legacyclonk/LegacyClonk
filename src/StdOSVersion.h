/*
 * LegacyClonk
 *
 * Copyright (c) 2025, The LegacyClonk Team and contributors
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

#include "StdCompiler.h"

#include <compare>
#include <cstdint>
#include <format>

class CStdOSVersion
{
public:
	CStdOSVersion(const std::uint16_t major = 0, const std::uint16_t minor = 0, const std::uint16_t build = 0)
		: major{major}, minor{minor}, build{build} {}

public:
	constexpr bool operator==(const CStdOSVersion &other) const noexcept = default;
	constexpr std::strong_ordering operator<=>(const CStdOSVersion &other) const noexcept = default;

	void CompileFunc(StdCompiler *comp);

public:
	std::uint16_t GetMajor() const noexcept { return major; }
	std::uint16_t GetMinor() const noexcept { return minor; }
	std::uint16_t GetBuild() const noexcept { return build; }

public:
	static CStdOSVersion GetLocal();
	static std::string GetFriendlyProductName();

private:
	std::uint16_t major;
	std::uint16_t minor;
	std::uint16_t build;

	friend struct std::formatter<CStdOSVersion>;
};

template<>
struct std::formatter<CStdOSVersion>
{
	constexpr auto parse(std::format_parse_context &context)
	{
		return context.begin();
	}

	auto format(const CStdOSVersion &version, std::format_context &context) const
	{
		return std::format_to(context.out(), "{}.{}.{}", version.major, version.minor, version.build);
	}
};
