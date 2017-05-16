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

#include <C4Include.h>
#include <C4Random.h>

// Random3

const int FRndRes = 500;

int32_t FRndBuf3[FRndRes];
int32_t FRndPtr3;

void Randomize3()
{
	FRndPtr3 = 0;
	for (int cnt = 0; cnt < FRndRes; cnt++) FRndBuf3[cnt] = Random(3) - 1;
}

int Rnd3()
{
	FRndPtr3++; if (FRndPtr3 == FRndRes) FRndPtr3 = 0;
#ifdef DEBUGREC
	AddDbgRec(RCT_Rn3, &FRndPtr3, sizeof(int));
#endif
	return FRndBuf3[FRndPtr3];
}
