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

#include "C4Math.h"

#include <cmath>
#include <numbers>

int32_t Distance(int32_t iX1, int32_t iY1, int32_t iX2, int32_t iY2)
{
	int64_t dx = int64_t(iX1) - iX2, dy = int64_t(iY1) - iY2;
	int64_t d2 = dx * dx + dy * dy;
	if (d2 < 0) return -1;
	int32_t dist = int32_t(sqrt(double(d2)));
	if (int64_t(dist) * dist < d2) ++dist;
	if (int64_t(dist) * dist > d2) --dist;
	return dist;
}

int Angle(int iX1, int iY1, int iX2, int iY2)
{
	int iAngle = static_cast<int>(180.0 * atan2f(float(Abs(iY1 - iY2)), float(Abs(iX1 - iX2))) * std::numbers::inv_pi_v<float>);
	if (iX2 > iX1)
	{
		if (iY2 < iY1) iAngle = 90 - iAngle; else iAngle = 90 + iAngle;
	}
	else
	{
		if (iY2 < iY1) iAngle = 270 + iAngle; else iAngle = 270 - iAngle;
	}
	return iAngle;
}

/* Fast integer exponentiation */
int Pow(int base, int exponent)
{
	if (exponent < 0) return 0;

	int result = 1;

	if (exponent & 1) result = base;
	exponent >>= 1;

	while (exponent)
	{
		base *= base;
		if (exponent & 1) result *= base;
		exponent >>= 1;
	}

	return result;
}
