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

/* Buffered fast and network-safe random */

#pragma once

#ifdef DEBUGREC
#include <C4Record.h>
#endif

extern int RandomCount;
extern unsigned int RandomHold;
#ifdef C4ENGINE

inline void FixedRandom(uint32_t dwSeed)
{
	// for SafeRandom
	srand((unsigned)time(nullptr));
	RandomHold = dwSeed;
	RandomCount = 0;
}

inline int Random(int iRange)
{
	// next pseudorandom value
	RandomCount++;
#ifdef DEBUGREC
	C4RCRandom rc;
	rc.Cnt = RandomCount;
	rc.Range = iRange;
	if (iRange == 0)
		rc.Val = 0;
	else
	{
		RandomHold = RandomHold * 214013L + 2531011L;
		rc.Val = (RandomHold >> 16) % iRange;
	}
	AddDbgRec(RCT_Random, &rc, sizeof(rc));
	return rc.Val;
#else
	if (iRange == 0) return 0;
	RandomHold = RandomHold * 214013L + 2531011L;
	return (RandomHold >> 16) % iRange;
#endif
}

inline unsigned int SeededRandom(unsigned int iSeed, unsigned int iRange)
{
	if (!iRange) return 0;
	iSeed = iSeed * 214013L + 2531011L;
	return (iSeed >> 16) % iRange;
}

#endif

inline int SafeRandom(int range)
{
	if (!range) return 0;
	return rand() % range;
}

void Randomize3();
int Rnd3();
