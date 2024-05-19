/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2021, The LegacyClonk Team and contributors
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

/* Lots of constants */

#pragma once

#include <cstdint>

// Main

const int C4MaxNameList = 10,
          C4MaxName = 30, // player names, etc.
          C4MaxLongName = 120, // scenario titles, etc. - may include markup
          C4MaxComment = 256, // network game and player comments
          C4MaxDefString = 100,
          C4MaxTitle = 512,
          C4MaxMessage = 256,

          C4MaxVariable = 10,

          C4ViewDelay = 100,
          C4RetireDelay = 60,

          C4MaxColor = 12,
          C4MaxKey = 12,
          C4MaxKeyboardSet = 4,
          C4MaxControlSet = C4MaxKeyboardSet + 4, // keyboard sets+gamepads

          C4MaxControlRate = 20,

          C4MaxGammaUserRamps = 8,
          C4MaxGammaRamps     = C4MaxGammaUserRamps + 1;

// gamma ramp indices
#define C4GRI_SCENARIO  0
#define C4GRI_SEASON    1
#define C4GRI_RESERVED1 2
#define C4GRI_DAYTIME   3
#define C4GRI_RESERVED2 4
#define C4GRI_LIGHTNING 5
#define C4GRI_MAGIC     6
#define C4GRI_RESERVED3 7

#define C4GRI_USER      8

const int C4M_MaxName = 15,
          C4M_MaxDefName = 2 * C4M_MaxName + 1,
          C4M_ColsPerMat = 3,
          C4M_MaxTexIndex = 127; // last texture map index is reserved for diff

const int C4S_MaxPlayer = 4;

const int C4D_MaxName = C4MaxName,
          C4D_MaxVertex = 30,
          C4D_MaxIDLen = C4D_MaxName;

const int C4Px_MaxParticle = 256, // maximum number of particles of one type
          C4Px_BufSize = 128, // number of particles in one buffer
          C4Px_MaxIDLen = 30; // maximum length of internal identifiers

const int C4SymbolSize = 35,
          C4SymbolBorder = 5,
          C4UpperBoardHeight = 50,
          C4PictureSize = 64,
          C4MaxPictureSize = 150,
          C4MaxBigIconSize = 64;

const int C4P_MaxPosition = 4;

const int C4P_Control_None       = -1,
          C4P_Control_Keyboard1  = 0,
          C4P_Control_Keyboard2  = 1,
          C4P_Control_Keyboard3  = 2,
          C4P_Control_Keyboard4  = 3,
          C4P_Control_GamePad1   = 4,
          C4P_Control_GamePad2   = 5,
          C4P_Control_GamePad3   = 6,
          C4P_Control_GamePad4   = 7,
          C4P_Control_GamePadMax = C4P_Control_GamePad4;

constexpr int C4MaxGamePadButtons = 16,
              C4MaxGamePadAxis = 6;

const int C4ViewportScrollBorder = 40; // scrolling past landscape allowed at range of this border

// Engine Return Values

const int C4XRV_Completed = 0,
          C4XRV_Failure   = 1;

// Object Character Flags

const uint32_t OCF_None             = 0,
               OCF_All              = ~OCF_None,
               OCF_Normal           = 1,
               OCF_Construct        = 1 << 1,
               OCF_Grab             = 1 << 2,
               OCF_Carryable        = 1 << 3,
               OCF_OnFire           = 1 << 4,
               OCF_HitSpeed1        = 1 << 5,
               OCF_FullCon          = 1 << 6,
               OCF_Inflammable      = 1 << 7,
               OCF_Chop             = 1 << 8,
               OCF_Rotate           = 1 << 9,
               OCF_Exclusive        = 1 << 10,
               OCF_Entrance         = 1 << 11,
               OCF_HitSpeed2        = 1 << 12,
               OCF_HitSpeed3        = 1 << 13,
               OCF_Collection       = 1 << 14,
               OCF_Living           = 1 << 15,
               OCF_HitSpeed4        = 1 << 16,
               OCF_FightReady       = 1 << 17,
               OCF_LineConstruct    = 1 << 18,
               OCF_Prey             = 1 << 19,
               OCF_AttractLightning = 1 << 20,
               OCF_NotContained     = 1 << 21,
               OCF_CrewMember       = 1 << 22,
               OCF_Edible           = 1 << 23,
               OCF_InLiquid         = 1 << 24,
               OCF_InSolid          = 1 << 25,
               OCF_InFree           = 1 << 26,
               OCF_Available        = 1 << 27,
               OCF_PowerConsumer    = 1 << 28,
               OCF_PowerSupply      = 1 << 29,
               OCF_Container        = 1 << 30,
               OCF_Alive            = 1 << 31;

// Contact / Attachment

const uint8_t // Directional
	CNAT_None        = 0,
	CNAT_Left        = 1,
	CNAT_Right       = 2,
	CNAT_Top         = 4,
	CNAT_Bottom      = 8,
	CNAT_Center      = 16,
	// Additional flags
	CNAT_MultiAttach = 32, // new attachment behaviour; see C4Shape::Attach
	CNAT_NoCollision = 64; // turn off collision for this vertex

const uint8_t CNAT_Flags = CNAT_MultiAttach | CNAT_NoCollision; // all attchment flags that can be combined with regular attachment

// Keyboard Input Controls

const int C4DoubleClick = 10;

const int CON_CursorLeft = 0,
          CON_CursorToggle = 1,
          CON_CursorRight = 2,
          CON_Throw = 3,
          CON_Up = 4,
          CON_Dig = 5,
          CON_Left = 6,
          CON_Down = 7,
          CON_Right = 8,
          CON_Menu = 9,
          CON_Special = 10,
          CON_Special2 = 11;

// Control Commands

const uint8_t COM_Single = 64,
              COM_Double = 128;

const uint8_t COM_None = 0;

const uint8_t COM_Left  = 1,
              COM_Right = 2,
              COM_Up    = 3,
              COM_Down  = 4,
              COM_Throw = 5,
              COM_Dig   = 6,

              COM_Special  = 7,
              COM_Special2 = 8,

              COM_Contents = 9,

              COM_WheelUp   = 10,
              COM_WheelDown = 11,

              COM_CursorLeft   = 12,
              COM_CursorRight  = 13,
              COM_CursorToggle = 14,
              COM_CursorFirst = COM_CursorLeft,
              COM_CursorLast = COM_CursorToggle,
              COM_Jump = 15,

              COM_Release      = 16, // TODO: Change all "+16" to "|COM_Release"
              COM_Left_R       = COM_Left     + 16,
              COM_Right_R      = COM_Right    + 16,
              COM_Up_R         = COM_Up       + 16,
              COM_Down_R       = COM_Down     + 16,
              COM_Throw_R      = COM_Throw    + 16,
              COM_Dig_R        = COM_Dig      + 16,
              COM_Special_R    = COM_Special  + 16,
              COM_Special2_R   = COM_Special2 + 16,
              COM_CursorLeft_R   = COM_CursorLeft   + 16,
              COM_CursorToggle_R = COM_CursorToggle + 16,
              COM_CursorRight_R  = COM_CursorRight  + 16,
              COM_Jump_R       = COM_Jump     + 16,
              COM_ReleaseFirst = COM_Left_R,
              COM_ReleaseLast  = COM_Jump_R,

              COM_Left_S     = COM_Left     | COM_Single,
              COM_Right_S    = COM_Right    | COM_Single,
              COM_Up_S       = COM_Up       | COM_Single,
              COM_Down_S     = COM_Down     | COM_Single,
              COM_Throw_S    = COM_Throw    | COM_Single,
              COM_Dig_S      = COM_Dig      | COM_Single,
              COM_Jump_S     = COM_Jump     | COM_Single,
              COM_Special_S  = COM_Special  | COM_Single,
              COM_Special2_S = COM_Special2 | COM_Single,
              COM_CursorLeft_S   = COM_CursorLeft   | COM_Single,
              COM_CursorToggle_S = COM_CursorToggle | COM_Single,
              COM_CursorRight_S  = COM_CursorRight  | COM_Single,

              COM_Left_D     = COM_Left     | COM_Double,
              COM_Right_D    = COM_Right    | COM_Double,
              COM_Up_D       = COM_Up       | COM_Double,
              COM_Down_D     = COM_Down     | COM_Double,
              COM_Throw_D    = COM_Throw    | COM_Double,
              COM_Dig_D      = COM_Dig      | COM_Double,
	          COM_Jump_D     = COM_Jump     | COM_Double,
              COM_Special_D  = COM_Special  | COM_Double,
              COM_Special2_D = COM_Special2 | COM_Double,
              COM_CursorLeft_D   = COM_CursorLeft   | COM_Double,
              COM_CursorToggle_D = COM_CursorToggle | COM_Double,
              COM_CursorRight_D  = COM_CursorRight  | COM_Double;

const uint8_t COM_Help       = 35,
              COM_PlayerMenu = 36,
              COM_Chat       = 37;

const uint8_t COM_MenuEnter    = 38,
              COM_MenuEnterAll = 39,
              COM_MenuClose    = 40,
              COM_MenuShowText = 42,
              COM_MenuLeft     = 52,
              COM_MenuRight    = 53,
              COM_MenuUp       = 54,
              COM_MenuDown     = 55,
              COM_MenuSelect   = 60,

              COM_MenuFirst = COM_MenuEnter,
              COM_MenuLast  = COM_MenuSelect,

              COM_MenuNavigation1 = COM_MenuShowText,
              COM_MenuNavigation2 = COM_MenuSelect,

              COM_ClearPressedComs = 61;

// SendCommand

const int32_t C4P_Command_None   = 0,
              C4P_Command_Set    = 1,
              C4P_Command_Add    = 2,
              C4P_Command_Append = 4,
              C4P_Command_Range  = 8;

// Owners

const int NO_OWNER  = -1,
          ANY_OWNER = -2;

// Base functionalities

const int BASE_RegenerateEnergyPrice = 5,
          BASEFUNC_Default          = 0xffff,
          BASEFUNC_AutoSellContents = 1 << 0,
          BASEFUNC_RegenerateEnergy = 1 << 1,
          BASEFUNC_Buy              = 1 << 2,
          BASEFUNC_Sell             = 1 << 3,
          BASEFUNC_RejectEntrance   = 1 << 4,
          BASEFUNC_Extinguish       = 1 << 5;

// League (escape those damn circular includes)

enum C4LeagueDisconnectReason
{
	C4LDR_Unknown,
	C4LDR_ConnectionFailed,
	C4LDR_Desync,
};

// Player (included by C4PlayerInfo and C4Player)

enum C4PlayerType
{
	C4PT_None = 0,
	C4PT_User = 1,   // Normal player
	C4PT_Script = 2, // AI players, etc.
};

// AllowPictureStack (DefCore value)

enum C4AllowPictureStack
{
	APS_Color    = 1 << 0,
	APS_Graphics = 1 << 1,
	APS_Name     = 1 << 2,
	APS_Overlay  = 1 << 3,
};
