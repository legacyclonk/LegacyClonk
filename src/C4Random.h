/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2020, The LegacyClonk Team and contributors
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

/* Buffered fast and network-safe random */

#pragma once

#ifdef DEBUGREC
#include <C4Record.h>

#include <atomic>
#endif

#include <cstdint>
#include <cstdlib>
#include <ctime>

class C4Random
{
public:
	explicit C4Random(const std::uint32_t seed) : randomCount{0}, randomHold{seed} {}
	C4Random(const C4Random &) = delete;
	C4Random &operator=(const C4Random &) = delete;
	C4Random(C4Random &&) = default;
	C4Random &operator=(C4Random &&) = default;

public:
	int Random(const int range)
	{
		// next pseudorandom value
		randomCount++;
	#ifdef DEBUGREC
		C4RCRandom rc;
		rc.Cnt = randomCount;
		rc.Range = range;
		rc.Id = id;
		if (range == 0)
			rc.Val = 0;
		else
		{
			randomHold = randomHold * 214013L + 2531011L;
			rc.Val = (randomHold >> 16) % range;
		}
		AddDbgRec(RCT_Random, &rc, sizeof(rc));
		return rc.Val;
	#else
		if (range == 0) return 0;
		randomHold = randomHold * 214013L + 2531011L;
		return (randomHold >> 16) % range;
	#endif
	}

public:
	std::uint32_t GetRandomCount() const noexcept { return randomCount; }
	C4Random Clone() noexcept { return C4Random{randomHold}; }

public:
	static C4Random Default;

private:
	static inline std::atomic_uint32_t NextId{0};

private:
	std::uint32_t randomCount;
	std::uint32_t randomHold;

#ifdef DEBUGREC
	std::uint32_t id{NextId.fetch_add(1, std::memory_order_acq_rel)};
#endif

	friend inline void FixedRandom(uint32_t);
};

inline void FixedRandom(uint32_t dwSeed)
{
	// for SafeRandom
	srand(static_cast<unsigned>(time(nullptr)));
	C4Random::Default = C4Random{dwSeed};
}

inline int Random(int iRange)
{
	return C4Random::Default.Random(iRange);
}

inline int SafeRandom(int range)
{
	if (!range) return 0;
	return rand() % range;
}

void Randomize3();
int Rnd3();
