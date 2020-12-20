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

/* 32-bit value to identify object definitions */

#include <C4Include.h>
#include <C4Id.h>

static char C4IdTextBuffer[5];

const char *C4IdText(C4ID id)
{
	GetC4IdText(id, C4IdTextBuffer);
	return C4IdTextBuffer;
}

void GetC4IdText(C4ID id, char *sBuf)
{
	// Invalid parameters
	if (!sBuf) return;
	// No id
	if (id == C4ID_None) { SCopy("NONE", sBuf); return; }
	// Numerical id
	if (Inside(static_cast<int>(id), 0, 9999))
	{
		osprintf(sBuf, "%04i", static_cast<unsigned int>(id));
	}
	// Literal id
	else
	{
		sBuf[0] = static_cast<char>((id & 0x000000FF) >> 0);
		sBuf[1] = static_cast<char>((id & 0x0000FF00) >> 8);
		sBuf[2] = static_cast<char>((id & 0x00FF0000) >> 16);
		sBuf[3] = static_cast<char>((id & 0xFF000000) >> 24);
		sBuf[4] = 0;
	}
}

bool LooksLikeID(C4ID id)
{
	// don't allow 0000, since this may indicate error
	if (Inside(static_cast<int>(id), 1, 9999)) return true;
	for (int cnt = 0; cnt < 4; cnt++)
	{
		char b = static_cast<char>(id & 0xFF);
		if (!(Inside(b, 'A', 'Z') || Inside(b, '0', '9') || (b == '_'))) return false;
		id >>= 8;
	}
	return true;
}

// make sure that C4ID values are consistent

static_assert("NONE"_id == 0);
static_assert("CLNK"_id == 0x4B4E4C43);
static_assert("1337"_id == 1337);
