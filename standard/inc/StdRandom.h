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

/* Some wrappers to runtime-library random */

#ifndef INC_STDRANDOM
#define INC_STDRANDOM

#include <time.h>

extern int RandomCount;
extern unsigned int RandomHold;

inline void FixedRandom(DWORD dwSeed)
	{
	RandomHold=dwSeed; // srand
	RandomCount=0;
	}

inline void Randomize()
	{
	FixedRandom((unsigned)time(NULL));
	}

inline int Random(int iRange)
	{
	RandomCount++;
	if (iRange==0) return 0;
	RandomHold = RandomHold * 214013L + 2531011L;
	return (RandomHold >> 16) % iRange;
	}

#endif // INC_STDRANDOM
