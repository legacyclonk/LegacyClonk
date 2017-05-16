/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Captures uncompressed AVI from game view */

#ifndef INC_C4Video
#define INC_C4Video

#include <C4Surface.h>

// avoid pulling in vfw.h in every file
struct IAVIFile;
struct IAVIStream;

class C4Video
{
public:
	C4Video();
	~C4Video();

protected:
	bool Active;
	IAVIFile   *pAviFile;
	IAVIStream *pAviStream;
	int AviFrame;
	double AspectRatio;
	int X, Y, Width, Height;
	BYTE *Buffer;
	int BufferSize;
	int InfoSize;
	bool Recording;
	SURFACE Surface;
	int ShowFlash;

public:
	void Draw();
	void Draw(C4FacetEx &cgo);
	void Resize(int iChange);
	bool Start(const char *szFilename);
	void Default();
	void Init(SURFACE sfcSource, int iWidth = 768, int iHeight = 576);
	void Clear();
	bool Start();
	bool Stop();
	bool Toggle();
	void Execute();
	bool Reduce();
	bool Enlarge();

protected:
	bool RecordFrame();
	bool AdjustPosition();
};

#endif
