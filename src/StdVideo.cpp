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

/* Some functions to help with saving AVI files using Video for Windows */

#ifdef _WIN32

#include <Standard.h>
#include <StdVideo.h>

#include <windows.h>

bool AVIOpenOutput(const char *szFilename,
	PAVIFILE *ppAviFile,
	PAVISTREAM *ppAviStream,
	int iWidth, int iHeight)
{
	// Init AVI system
	AVIFileInit();

	// Create avi file
	if (AVIFileOpen(
		ppAviFile,
		szFilename,
		OF_CREATE | OF_WRITE,
		nullptr) != 0)
	{
		return false;
	}

	// Create stream
	AVISTREAMINFO avi_info;
	RECT frame; frame.left = 0; frame.top = 0; frame.right = iWidth; frame.bottom = iHeight;
	avi_info.fccType = streamtypeVIDEO;
	avi_info.fccHandler = mmioFOURCC('M', 'S', 'V', 'C');
	avi_info.dwFlags = 0;
	avi_info.dwCaps = 0;
	avi_info.wPriority = 0;
	avi_info.wLanguage = 0;
	avi_info.dwScale = 1;
	avi_info.dwRate = 35;
	avi_info.dwStart = 0;
	avi_info.dwLength = 10; // ??
	avi_info.dwInitialFrames = 0;
	avi_info.dwSuggestedBufferSize = 0;
	avi_info.dwQuality = -1;
	avi_info.dwSampleSize = 0;
	avi_info.rcFrame = frame;
	avi_info.dwEditCount = 0;
	avi_info.dwFormatChangeCount = 0;
	SCopy("MyRecording", avi_info.szName);

	if (AVIFileCreateStream(
		*ppAviFile,
		ppAviStream,
		&avi_info) != 0)
	{
		return false;
	}

	return true;
}

bool AVICloseOutput(PAVIFILE *ppAviFile,
	PAVISTREAM *ppAviStream)
{
	if (ppAviStream && *ppAviStream)
	{
		AVIStreamRelease(*ppAviStream); *ppAviStream = nullptr;
	}
	if (ppAviFile && *ppAviFile)
	{
		AVIFileRelease(*ppAviFile); *ppAviFile = nullptr;
	}
	return true;
}

bool AVIPutFrame(PAVISTREAM pAviStream,
	long lFrame,
	void *lpInfo, long lInfoSize,
	void *lpData, long lDataSize)
{
	long lBytesWritten = 0, lSamplesWritten = 0;

	AVIStreamSetFormat(
		pAviStream,
		lFrame,
		lpInfo,
		lInfoSize
	);

	if (AVIStreamWrite(
		pAviStream,
		lFrame,
		1,
		lpData,
		lDataSize,
		AVIIF_KEYFRAME,
		&lSamplesWritten,
		&lBytesWritten) != 0) return false;

	return true;
}

#endif // _WIN32
