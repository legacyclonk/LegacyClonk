/* Handles Music Files */

#pragma once

#ifdef USE_FMOD
#include <fmod.h>
#endif
#ifdef HAVE_LIBSDL_MIXER
#define USE_RWOPS
#include <SDL_mixer.h>
#undef USE_RWOPS
#endif

/* Base class */
class C4MusicFile
{
public:
	C4MusicFile() : LastPlayed(-1), NoPlay(false), SongExtracted(false) {}
	virtual ~C4MusicFile() {}

	// data
	char FileName[_MAX_FNAME + 1];
	C4MusicFile *pNext;
	int LastPlayed;
	bool NoPlay;

	virtual bool Init(const char *strFile);
	virtual bool Play(bool loop = false) = 0;
	virtual void Stop(int fadeout_ms = 0) = 0;
	virtual void CheckIfPlaying() = 0;
	virtual void SetVolume(int) = 0;

protected:
	// helper: copy data to a (temp) file
	bool ExtractFile();
	bool RemTempFile(); // remove the temp file

	bool SongExtracted;
};

#ifdef USE_FMOD
class C4MusicFileMID : public C4MusicFile
{
public:
	bool Play(bool loop = false);
	void Stop(int fadeout_ms = 0);
	void CheckIfPlaying();
	void SetVolume(int);

protected:
	FMUSIC_MODULE *mod;
};

/* MOD class */

class C4MusicFileMOD : public C4MusicFile
{
public:
	C4MusicFileMOD();
	~C4MusicFileMOD();
	bool Play(bool loop = false);
	void Stop(int fadeout_ms = 0);
	void CheckIfPlaying();
	void SetVolume(int);

protected:
	FMUSIC_MODULE *mod;
	char *Data;
};

/* MP3 class */

class C4MusicFileMP3 : public C4MusicFile
{
public:
	C4MusicFileMP3();
	~C4MusicFileMP3();
	bool Play(bool loop = false);
	void Stop(int fadeout_ms = 0);
	void CheckIfPlaying();
	void SetVolume(int);

protected:
	FSOUND_STREAM *stream;
	char *Data;
	int Channel;
};

/* Ogg class */

class C4MusicFileOgg : public C4MusicFile
{
public:
	C4MusicFileOgg();
	~C4MusicFileOgg();
	bool Play(bool loop = false);
	void Stop(int fadeout_ms = 0);
	void CheckIfPlaying();
	void SetVolume(int);

	static signed char __stdcall OnEnd(FSOUND_STREAM *stream, void *buff, int length, void *param);

protected:
	FSOUND_STREAM *stream;
	char *Data;
	int Channel;

	bool Playing;
};
#endif

#ifdef HAVE_LIBSDL_MIXER
typedef struct _Mix_Music Mix_Music;
class C4MusicFileSDL : public C4MusicFile
{
public:
	C4MusicFileSDL();
	~C4MusicFileSDL();
	bool Play(bool loop = false);
	void Stop(int fadeout_ms = 0);
	void CheckIfPlaying();
	void SetVolume(int);

protected:
	char *Data;
	Mix_Music *Music;
};
#endif // HAVE_LIBSDL_MIXER
