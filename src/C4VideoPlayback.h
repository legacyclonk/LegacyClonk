/* by Sven2, 2006 */
// InGame sequence video playback support

#pragma once

#ifndef BIG_C4INCLUDE
#include "C4Gui.h"
#endif
#include <StdVideo.h>

typedef struct _SMPEG SMPEG;
typedef struct _SMPEG_Info SMPEG_Info;
typedef struct SDL_Surface SDL_Surface;

// a preloaded video file
class C4VideoFile
{
private:
	StdStrBuf sFilename; // path to filename for avi file
	bool fIsTemp;        // if set, the file is temporary will be deleted upon destruction of this object
	C4VideoFile *pNext;

public:
	C4VideoFile() : fIsTemp(false), pNext(nullptr) {}
	~C4VideoFile() { Clear(); }

	void Clear();

	bool Load(const char *szFilename, bool fTemp = false); // create from file
	bool Load(class C4Group &hGrp, const char *szFilename); // create from group: Extract to temp file if group is packed

	C4VideoFile *GetNext() const { return pNext; }
	const char *GetFilename() const { return sFilename.getData(); }

	void SetNext(C4VideoFile *pNewNext) { pNext = pNewNext; }
};

// playback dialog
class C4VideoShowDialog : public C4GUI::FullscreenDialog
{
private:
#ifdef _WIN32
	CStdAVIFile AVIFile;
	C4SoundEffect *pAudioTrack;
#endif
#ifdef HAVE_LIBSDL_MIXER
	SMPEG *mpeg;
	SMPEG_Info *mpeg_info;
	SDL_Surface *surface;
#endif
	C4FacetExSurface fctBuffer;
	time_t iStartFrameTime;

protected:
	virtual int32_t GetZOrdering() { return C4GUI_Z_VIDEO; }
	virtual bool IsExclusiveDialog() { return true; }

	void VideoDone(); // mark video done

public:
	C4VideoShowDialog() : C4GUI::FullscreenDialog(nullptr, nullptr)
#ifdef _WIN32
		, pAudioTrack(nullptr)
#endif
#ifdef HAVE_LIBSDL_MIXER
		, mpeg(0)
		, mpeg_info(0)
		, surface(0)
#endif
	{}
	~C4VideoShowDialog();

	bool LoadVideo(C4VideoFile *pVideoFile);

	virtual void DrawElement(C4FacetEx &cgo); // draw current video frame
};

// main playback class (C4Game member)
class C4VideoPlayer
{
private:
	C4VideoFile *pFirstVideo;

	// plays the specified video and returns when finished or skipped by user (blocking call)
	bool PlayVideo(C4VideoFile *pVideoFile);

public:
	C4VideoPlayer() : pFirstVideo(nullptr) {}
	~C4VideoPlayer() { Clear(); }

	// delete all loaded videos
	void Clear();

	// preload videos from file
	bool PreloadVideos(class C4Group &rFromGroup);

	// plays the specified video and returns when finished or skipped by user (blocking call)
	// video speicifed by filename - uses preload database if possible; otherwise gets video from root video collection
	bool PlayVideo(const char *szVideoFilename);
};
