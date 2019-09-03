/*
 * LegacyClonk
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2011-2016, The OpenClonk Team and contributors
 * Copyright (c) 2019, The LegacyClonk Team and contributors
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

#include "C4Include.h"
#include "C4TimeMilliseconds.h"
#include <limits>

C4TimeMilliseconds C4TimeMilliseconds::Now()
{
	return C4TimeMilliseconds(static_cast<uint32_t>(timeGetTime()));
}

StdStrBuf C4TimeMilliseconds::AsString() const
{
	if (inf == PositiveInfinity)
	{
		return StdStrBuf{"POSITIVE INFINITY"};
	}
	if (inf == NegativeInfinity)
	{
		return StdStrBuf{"NEGATIVE INFINITY"};
	}

	return FormatString("%u:%02u:%02u:%03u:", time / 1000 / 60 / 60, (time / 1000 / 60) % 60, (time / 1000) % 60, time % 1000);
}

C4TimeMilliseconds& C4TimeMilliseconds::operator=(const C4TimeMilliseconds& rhs) = default;

bool operator==( const C4TimeMilliseconds& lhs, const C4TimeMilliseconds& rhs )
{
	return lhs.inf == rhs.inf && 
	       lhs.time == rhs.time;
}

bool operator<( const C4TimeMilliseconds& lhs, const C4TimeMilliseconds& rhs )
{
	if (lhs.inf != C4TimeMilliseconds::NoInfinity ||
	    rhs.inf != C4TimeMilliseconds::NoInfinity)
	{
		return lhs.inf < rhs.inf;
	}
	return lhs.time < rhs.time;
}

int32_t operator-(const C4TimeMilliseconds& lhs, const C4TimeMilliseconds& rhs)
{
	// if infinity is set, nothing else than infinity matters (infinity + 100 == infinity)
	if (lhs.inf != C4TimeMilliseconds::NoInfinity ||
	    rhs.inf != C4TimeMilliseconds::NoInfinity)
	{
		int infinityTo = lhs.inf - rhs.inf;
		
		if (infinityTo < 0) return std::numeric_limits<int32_t>::min();
		if (infinityTo > 0) return std::numeric_limits<int32_t>::max();
		return 0;
	}
	// otherwise, as usual
	return int32_t(lhs.time - rhs.time);
}
