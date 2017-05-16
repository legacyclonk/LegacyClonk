/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* A wrapper to DirectSound - derived from DXSDK samples */

struct CSoundObject;

CSoundObject *DSndObjCreate(BYTE *bpWaveBuf, int iConcurrent = 1);
BOOL DSndObjPlay(CSoundObject *hSO, DWORD dwPlayFlags);
BOOL DSndObjStop(CSoundObject *hSO);
BOOL DSndObjPlaying(CSoundObject *hSO);
void DSndObjDestroy(CSoundObject *hSO);
BOOL DSndObjSetVolume(CSoundObject *pSO, long lVolume);

#define DSBPLAY_LOOPING 0x00000001
