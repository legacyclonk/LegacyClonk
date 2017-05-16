/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

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
