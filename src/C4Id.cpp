/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* 32-bit value to identify object definitions */

#include <C4Include.h>
#include <C4Id.h>

C4ID C4Id(const char *szId)
{
	if (!szId) return C4ID_None;
	// Numerical id
	if (Inside(szId[0], '0', '9') && Inside(szId[1], '0', '9') && Inside(szId[2], '0', '9') && Inside(szId[3], '0', '9'))
	{
		int iResult;
		sscanf(szId, "%d", &iResult);
		return iResult;
	}
	// NONE?
	if (SEqual(szId, "NONE"))
		return 0;
	// Literal id
	return (((DWORD)szId[3]) << 24) + (((DWORD)szId[2]) << 16) + (((DWORD)szId[1]) << 8) + ((DWORD)szId[0]);
}

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
	if (Inside((int)id, 0, 9999))
	{
		osprintf(sBuf, "%04i", static_cast<unsigned int>(id));
	}
	// Literal id
	else
	{
		sBuf[0] = (char)((id & 0x000000FF) >> 0);
		sBuf[1] = (char)((id & 0x0000FF00) >> 8);
		sBuf[2] = (char)((id & 0x00FF0000) >> 16);
		sBuf[3] = (char)((id & 0xFF000000) >> 24);
		sBuf[4] = 0;
	}
}

BOOL LooksLikeID(const char *szText)
{
	int cnt;
	if (SLen(szText) != 4) return FALSE;
	for (cnt = 0; cnt < 4; cnt++)
		if (!(Inside(szText[cnt], 'A', 'Z') || Inside(szText[cnt], '0', '9') || (szText[cnt] == '_')))
			return FALSE;
	return TRUE;
}

bool LooksLikeID(C4ID id)
{
	// don't allow 0000, since this may indicate error
	if (Inside((int)id, 1, 9999)) return true;
	for (int cnt = 0; cnt < 4; cnt++)
	{
		BYTE b = (BYTE)id & 0xFF;
		if (!(Inside((char)b, 'A', 'Z') || Inside((char)b, '0', '9') || (b == '_'))) return false;
		id >>= 8;
	}
	return true;
}
