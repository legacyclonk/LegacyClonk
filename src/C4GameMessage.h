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

/* Text messages drawn inside the viewport */

#pragma once

#include "C4ForwardDeclarations.h"
#include <C4Surface.h>
#include <C4Gui.h>

#include <memory>
#include <vector>

const int32_t C4GM_MaxText = 256,
              C4GM_MinDelay = 20;

const int32_t C4GM_Global = 1,
              C4GM_GlobalPlayer = 2,
              C4GM_Target = 3,
              C4GM_TargetPlayer = 4;

const int32_t C4GM_NoBreak    = 1 << 0,
              C4GM_Bottom     = 1 << 1, // message placed at bottom of screen
              C4GM_Multiple   = 1 << 2,
              C4GM_Top        = 1 << 3,
              C4GM_Left       = 1 << 4,
              C4GM_Right      = 1 << 5,
              C4GM_HCenter    = 1 << 6,
              C4GM_VCenter    = 1 << 7,
              C4GM_DropSpeech = 1 << 8, // cut any text after '$'
              C4GM_WidthRel   = 1 << 9,
              C4GM_XRel       = 1 << 10,
              C4GM_YRel       = 1 << 11,
              C4GM_ALeft      = 1 << 12,
              C4GM_ACenter    = 1 << 13,
              C4GM_ARight     = 1 << 14;

const int32_t C4GM_PositioningFlags = C4GM_Bottom | C4GM_Top | C4GM_Left | C4GM_Right | C4GM_HCenter | C4GM_VCenter;

class C4GameMessage
{
	friend class C4GameMessageList;

public:
	void Draw(C4FacetEx &cgo, int32_t iPlayer);
	C4GameMessage();
	~C4GameMessage();

protected:
	int32_t X, Y, Wdt;
	int32_t Delay;
	uint32_t ColorDw;
	int32_t Player;
	int32_t Type;
	C4Section *Section{nullptr};
	C4Object *Target;
	StdStrBuf Text;
	C4ID DecoID; StdStrBuf PortraitDef;
	C4GUI::FrameDecoration *pFrameDeco;
	uint32_t dwFlags;

protected:
	void Init(int32_t iType, const StdStrBuf &Text, C4Section *section, C4Object *pTarget, int32_t iPlayer, int32_t iX, int32_t iY, uint32_t dwCol, C4ID idDecoID, const char *szPortraitDef, uint32_t dwFlags, int width);
	void Append(const char *szText, bool fNoDuplicates = false);
	bool Execute();
	void UpdateDef(C4ID idUpdDef);

public:
	int32_t GetPositioningFlags() const { return dwFlags & C4GM_PositioningFlags; }
};

class C4GameMessageList
{
public:
	C4GameMessageList();
	~C4GameMessageList();

protected:
	std::vector<std::unique_ptr<C4GameMessage>> Messages;

public:
	void Clear();
	void Execute();
	void Draw(C4FacetEx &cgo, int32_t iPlayer, C4Section &viewSection);
	void ClearPlayers(int32_t iPlayer, int32_t dwPositioningFlags);
	void ClearPointers(C4Object *pObj);
	void UpdateDef(C4ID idUpdDef); // called after reloaddef
	bool New(int32_t iType, const StdStrBuf &Text, C4Section *section, C4Object *pTarget, int32_t iPlayer, int32_t iX = -1, int32_t iY = -1, uint32_t dwClr = 0xffFFFFFF, C4ID idDecoID = C4ID_None, const char *szPortraitDef = nullptr, uint32_t dwFlags = 0u, int32_t width = 0);
	bool New(int32_t iType, const char *szText, C4Section *section, C4Object *pTarget, int32_t iPlayer, int32_t iX, int32_t iY, uint8_t bCol);
	bool New(int32_t iType, const char *szText, C4Section *section, C4Object *pTarget, int32_t iPlayer, int32_t iX, int32_t iY, uint32_t dwClr, C4ID idDecoID = C4ID_None, const char *szPortraitDef = nullptr, uint32_t dwFlags = 0u, int32_t width = 0);
	bool Append(int32_t iType, const char *szText, C4Section *section, C4Object *pTarget, int32_t iPlayer, int32_t iX, int32_t iY, uint8_t bCol, bool fNoDuplicates = false);
};

void GameMsgObject(const char *szText, C4Object *pTarget, int32_t iFCol = FWhite);
void GameMsgObjectPlayer(const char *szText, C4Object *pTarget, int32_t iPlayer, int32_t iFCol = FWhite);
void GameMsgGlobal(const char *szText, int32_t iFCol = FWhite);
void GameMsgPlayer(const char *szText, int32_t iPlayer, int32_t iFCol = FWhite);

void GameMsgObjectDw(const char *szText, C4Object *pTarget, uint32_t dwClr);
