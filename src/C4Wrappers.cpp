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

/* Some useful wrappers to globals */

#include <C4Include.h>
#include <C4Wrappers.h>

#include <C4Random.h>
#include <C4Object.h>

// Materials

int32_t PixCol2MatOld(uint8_t pixc)
{
	if (pixc < GBM) return MNone;
	pixc &= 63; // Substract GBM, ignore IFT
	if (pixc > Game.Material.Num * C4M_ColsPerMat - 1) return MNone;
	return pixc / C4M_ColsPerMat;
}

int32_t PixCol2MatOld2(uint8_t pixc)
{
	int32_t iMat = ((int32_t)(pixc & 0x7f)) - 1;
	// if above MVehic, don't forget additional vehicle-colors
	if (iMat <= MVehic) return iMat;
	// equals middle vehicle-color
	if (iMat == MVehic + 1) return MVehic;
	// above: range check
	iMat -= 2; if (iMat >= Game.Material.Num) return MNone;
	return iMat;
}

// Graphics Resource

#define GfxR (&(Game.GraphicsResource))

// Messages

void GameMsgObject(const char *szText, C4Object *pTarget, int32_t iFCol)
{
	Game.Messages.New(C4GM_Target, szText, pTarget, NO_OWNER, 0, 0, (uint8_t)iFCol);
}

void GameMsgObjectPlayer(const char *szText, C4Object *pTarget, int32_t iPlayer, int32_t iFCol)
{
	Game.Messages.New(C4GM_TargetPlayer, szText, pTarget, iPlayer, 0, 0, (uint8_t)iFCol);
}

void GameMsgGlobal(const char *szText, int32_t iFCol)
{
	Game.Messages.New(C4GM_Global, szText, nullptr, ANY_OWNER, 0, 0, (uint8_t)iFCol);
}

void GameMsgPlayer(const char *szText, int32_t iPlayer, int32_t iFCol)
{
	Game.Messages.New(C4GM_GlobalPlayer, szText, nullptr, iPlayer, 0, 0, (uint8_t)iFCol);
}

void GameMsgObjectDw(const char *szText, C4Object *pTarget, uint32_t dwClr)
{
	Game.Messages.New(C4GM_Target, szText, pTarget, NO_OWNER, 0, 0, dwClr);
}

// Players

int32_t ValidPlr(int32_t plr)
{
	return Game.Players.Valid(plr);
}

int32_t Hostile(int32_t plr1, int32_t plr2)
{
	return Game.Players.Hostile(plr1, plr2);
}

// StdCompiler

void StdCompilerWarnCallback(void *pData, const char *szPosition, const char *szError)
{
	const char *szName = reinterpret_cast<const char *>(pData);
	if (!szPosition || !*szPosition)
		DebugLogF("WARNING: %s (in %s)", szError, szName);
	else
		DebugLogF("WARNING: %s (in %s, %s)", szError, szPosition, szName);
}
