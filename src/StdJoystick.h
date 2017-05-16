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

/* Simple joystick handling with DirectInput 1 */

#include <Standard.h>
#include <windows.h>
#include <mmsystem.h>

const int32_t PAD_Axis_POVx = 6;
const int32_t PAD_Axis_POVy = 7; // virtual axises of the coolie hat

const int CStdGamepad_MaxGamePad = 15, // maximum number of supported gamepads
					CStdGamepad_MaxCalAxis = 6,  // maximum number of calibrated axises
					CStdGamepad_MaxAxis = 8;     // number of axises plus coolie hat axises

class CStdGamePad
	{
	public:
		enum AxisPos { Low, Mid, High, }; // quantized axis positions
	private:
		int id; // gamepad number
		JOYINFOEX joynfo; // WIN32 gamepad info

	public:
		uint32_t dwAxisMin[CStdGamepad_MaxCalAxis], dwAxisMax[CStdGamepad_MaxCalAxis]; // axis ranges - auto calibrated
		bool fAxisCalibrated[CStdGamepad_MaxCalAxis]; // set if an initial value for axis borders has been determined already

		CStdGamePad(int id); // ctor

		void ResetCalibration(); // resets axis min and max
		void SetCalibration(uint32_t *pdwAxisMin, uint32_t *pdwAxisMax, bool *pfAxisCalibrated);
		void GetCalibration(uint32_t *pdwAxisMin, uint32_t *pdwAxisMax, bool *pfAxisCalibrated);

		bool Update(); // read current gamepad data
		uint32_t GetButtons(); // returns bitmask of pressed buttons for last retrieved info
		AxisPos GetAxisPos(int idAxis); // return axis extension - mid for error or center position
	};
