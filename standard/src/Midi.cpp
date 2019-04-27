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

/* Play midis using mci */

#include <Standard.h>

#ifdef HAVE_MIDI_H
#include <mmsystem.h>
#include <midi.h>
#include <stdio.h>

BOOL PlayMidi(const char *sFileName, HWND appWnd)
	{
  char buf[256];
  sprintf(buf, "open \"%s\" type sequencer alias ITSMYMUSIC", sFileName);  
	if (mciSendString("close all", NULL, 0, NULL) != 0)
    return FALSE;  
  if (mciSendString(buf, NULL, 0, NULL) != 0)
    return FALSE;
  if (mciSendString("play ITSMYMUSIC from 0 notify", NULL, 0, appWnd) != 0)
    return FALSE; 
  return TRUE;
	}

BOOL PauseMidi()
	{
	if (mciSendString("stop ITSMYMUSIC", NULL, 0, NULL) != 0) return FALSE;
	return TRUE;
	}

BOOL ResumeMidi(HWND appWnd)
	{       
	if (mciSendString("play ITSMYMUSIC notify", NULL, 0, appWnd) != 0) return FALSE;
	return TRUE;
	}

BOOL StopMidi()
	{
	if (mciSendString("close all", NULL, 0, NULL) != 0) return FALSE;
	return TRUE;
	}

BOOL ReplayMidi(HWND appWnd)
	{
	if (mciSendString("play ITSMYMUSIC from 0 notify", NULL, 0, appWnd) != 0) return FALSE;
	return TRUE;
	}
#endif //HAVE_MIDI_H
