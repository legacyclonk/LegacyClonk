/*
 * LegacyClonk
 *
 * Copyright (c) 2020, The LegacyClonk Team and contributors
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

#include "StdCompiler.h"

#include <chrono>
#include <utility>

template<typename Rep, typename Period = std::ratio<1>, typename Clock = std::chrono::steady_clock>
class C4Cooldown
{
public:
	using DurationType = std::chrono::duration<Rep, Period>;

public:
	C4Cooldown(const DurationType &cooldown = {}) noexcept : cooldown{cooldown} {}
	C4Cooldown(C4Cooldown &&other) noexcept : cooldown{std::exchange(other.cooldown, 0)}, lastReset{std::exchange(other.lastReset, 0)} {}

public:
	void Reset() { lastReset = Clock::now(); }
	bool Elapsed() const noexcept { return std::chrono::duration_cast<DurationType>(Clock::now() - lastReset) >= GetCooldown(); }
	bool TryReset();

	auto GetRemainingTime() const noexcept -> DurationType;
	DurationType GetCooldown() const noexcept { return cooldown; }

	void CompileFunc(class StdCompiler *comp);
	void CompileFunc(class StdCompiler *comp, DurationType minimumCooldown);

	bool operator==(const C4Cooldown &other) { return cooldown == other.cooldown; }

	template<typename R, typename P>
	C4Cooldown &operator=(const std::chrono::duration<R, P> &other) { cooldown = std::chrono::duration_cast<DurationType>(other); return *this; }

private:
	typename Clock::time_point lastReset;
	DurationType cooldown;
};

template<typename Rep, typename Period, typename Clock>
bool C4Cooldown<Rep, Period, Clock>::TryReset()
{
	const bool success{Elapsed()};
	if (success)
	{
		Reset();
	}

	return success;
}

template<typename Rep, typename Period, typename Clock>
auto C4Cooldown<Rep, Period, Clock>::GetRemainingTime() const noexcept -> DurationType
{
	if (const auto difference = Clock::now() - lastReset; difference >= cooldown)
	{
		return {};
	}
	else
	{
		return std::chrono::duration_cast<DurationType>(cooldown - difference);
	}
}


template<typename Rep, typename Period, typename Clock>
void C4Cooldown<Rep, Period, Clock>::CompileFunc(StdCompiler *comp)
{
	comp->Value(cooldown);
}

template<typename Rep, typename Period, typename Clock>
void C4Cooldown<Rep, Period, Clock>::CompileFunc(StdCompiler *comp, DurationType minimumCooldown)
{
	CompileFunc(comp);

	if (comp->isCompiler())
	{
		cooldown = std::max(cooldown, minimumCooldown);
	}
}

template<typename Rep, typename Period> C4Cooldown(std::chrono::duration<Rep, Period>) -> C4Cooldown<Rep, Period>;

using C4CooldownSeconds = C4Cooldown<std::chrono::seconds::rep, std::chrono::seconds::period, std::chrono::steady_clock>;
