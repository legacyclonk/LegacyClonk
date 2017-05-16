#ifdef _WIN32
#include <Windows.h>
#include <MMSystem.h>
#include <VFW.h>
#include <Stdio.h>
#endif

#include "Standard.h"
#include "StdFile.h"
#include "CStdFile.h"
#include "StdBitmap.h"
#include "StdVideo.h"
#include "loadpng/loadpng.h"
#include "loadpng/savepng.h"
#include "../engine/inc/C4Version.h"

#define VERSION_STRING "RedWolf Design  anigrab  " C4VERSION

class AniSpec
{
public:
	AniSpec() { ZeroMem(this,sizeof(AniSpec)); }
public:
	int Count,First,Last;
	int Width,Height,BitDepth,Pitch;
	bool Flip,Reverse;
	int FacetX,FacetY,FacetWidth,FacetHeight;
	int TargetX,TargetY;
	int IndexWidth,IndexBase,IndexStep;
};

int scanNumberLength(const char *strFilename)
{
	int iNumberWidth = 0;
	const char *pEndOfName = GetExtension(strFilename);
	if (pEndOfName) pEndOfName-=2;
	if (!pEndOfName) pEndOfName = strFilename + SLen(strFilename) - 1;
	while (pEndOfName && (pEndOfName>=strFilename) && Inside(*pEndOfName,'0','9'))
	{
		iNumberWidth++;
		pEndOfName--;
	}
	return iNumberWidth;
}

void getSequenceFilename(const char *strName, int iIndex, AniSpec &spcSpec, char *strBuffer)
{
	// Prepare number format
	char strFormat[10]; 
	sprintf(strFormat,"%%0%ii",spcSpec.IndexWidth);
	// Create index number
	char strNumber[100+1];
	sprintf(strNumber, strFormat, spcSpec.IndexBase + iIndex * spcSpec.IndexStep);
	// Compose filename
	SCopy(strName, strBuffer);
	MemCopy(strNumber, GetExtension(strBuffer)-1-spcSpec.IndexWidth, spcSpec.IndexWidth);
}

const char* getSequenceFilenameStatic(const char *strName, int iIndex, AniSpec &spcSpec)
{
	static char strBuffer[2048];
	getSequenceFilename(strName,iIndex,spcSpec,strBuffer);
	return strBuffer;
}

#ifdef _WIN32
// Avi globals
PAVISTREAM hAviStream=NULL;
PGETFRAME hGetFrame=NULL;

bool aniOpenSourceAvi(const char *szFilename, AniSpec &spcSpec)
{
	// Init avi
	AVIFileInit();
	// Open avi
	if (!AVIOpenGrab(szFilename, &hAviStream, &hGetFrame,
									 spcSpec.Count,
									 spcSpec.Width, spcSpec.Height,
									 spcSpec.BitDepth, spcSpec.Pitch))
	{ 
		AVICloseGrab(&hAviStream,&hGetFrame);
		printf("Error on %s\n",szFilename); 
		return false; 
	}
	// Set default spec by scanned values
	spcSpec.FacetX=0; spcSpec.FacetY=0; 
	spcSpec.FacetWidth=spcSpec.Width; spcSpec.FacetHeight=spcSpec.Height;
	spcSpec.First=0; spcSpec.Last=spcSpec.Count-1; 
	// Success
	return true;
}
#else
bool aniOpenSourceAvi(const char *szFilename, AniSpec &spcSpec)
{
	return false; 
}
#endif

// Png globals
char strPngName[2048+1]="";

bool aniOpenSourcePng(const char *szFilename, AniSpec &spcSpec)
{
	// Store file name
	SCopy(szFilename,strPngName,2048);

	// Get and truncate input extension (default png if no extension found)
	char strExtension[100+1] = "png";
	if (SLen(GetExtension(strPngName)))
	{
		SCopy(GetExtension(strPngName),strExtension);
		GetExtension(strPngName)[-1]=0;
	}

	// No numbers in input and specified file not found: append 0000
	if (!SCharCountEx(GetFilename(strPngName),"0123456789"))
		if (!FileExists(szFilename))
			SAppend("0000",strPngName);

	// (Re)append input extension
	EnforceExtension(strPngName,strExtension);

	// Scan index width and base
	spcSpec.IndexWidth = scanNumberLength(strPngName);
	sscanf(GetExtension(strPngName)-1-spcSpec.IndexWidth,"%d",&spcSpec.IndexBase);

	// Scan index step (max 10)
	const int maxStep = 50;
	for (spcSpec.IndexStep = 1; (spcSpec.IndexStep<=maxStep) && !FileExists(getSequenceFilenameStatic(strPngName,1,spcSpec)); spcSpec.IndexStep++);
	// No valid step detected: reset to one (there's only one frame anyway)
	if (spcSpec.IndexStep>maxStep) 
		spcSpec.IndexStep = 1;

	// No index width: assume single frame
	if (!spcSpec.IndexWidth)
		spcSpec.Count = 1;
	// Else: scan count
	else
	{
		char strFilename[1024+1];
		SCopy(strPngName,strFilename);
		while (FileExists(strFilename))
		{
			// Count file
			spcSpec.Count++;
			// Compose next filename
			getSequenceFilename(strPngName, spcSpec.Count, spcSpec, strFilename);
		}
	}

	// Get specs from first input file
	int iDepth;
	if (!getPngInfo(strPngName,&spcSpec.Width,&spcSpec.Height,&iDepth))
	{
		printf("File check failure: %s\n",strPngName);
		return false;
	}
	spcSpec.Pitch = spcSpec.Width * iDepth; // Calculate pitch ourselves, knowing loadPng does not pad...
	spcSpec.BitDepth = iDepth*8;

	// Set default spec by scanned values
	spcSpec.FacetX=0; spcSpec.FacetY=0; 
	spcSpec.FacetWidth=spcSpec.Width; spcSpec.FacetHeight=spcSpec.Height;
	spcSpec.First=0; spcSpec.Last=spcSpec.Count-1; 

	// Success
	return true;
}

bool aniOpenSource(const char *szFilename, AniSpec &spcSpec)
{
	// No extension: default to png
	char strFilename[1024 + 1];
	SCopy(szFilename, strFilename);
	DefaultExtension(strFilename, "png");
	// Avi
	if (SEqualNoCase(GetExtension(strFilename), "avi"))
		return aniOpenSourceAvi(strFilename,spcSpec);
	// Png sequence
	if (SEqualNoCase(GetExtension(strFilename), "png"))
		return aniOpenSourcePng(strFilename, spcSpec);
	// Unknown type
	return false;
}

bool aniCloseSource()
{
#ifdef _WIN32
	// Split this into type dependent functions...
	if (hAviStream && hGetFrame)
	{
		AVICloseGrab(&hAviStream,&hGetFrame);
		hAviStream=NULL; hGetFrame=NULL;
	}
	// Done
	return true;
#else
	return false;
#endif
}

bool aniGrabFrameAvi(AniSpec &spcSpec, int iFrame, CStdBitmap &bmpResult)
{
#ifdef _WIN32
	void *pFrame;
	if (pFrame = AVIStreamGetFrame(hGetFrame,iFrame))
	{
		BYTE *bpBits = ((BYTE*)pFrame) + sizeof(BITMAPINFOHEADER);
		StdBlit( (uint8_t*) bpBits,
						 spcSpec.Pitch,spcSpec.Height,
						 spcSpec.FacetX,spcSpec.FacetY,spcSpec.FacetWidth,spcSpec.FacetHeight,
						 (uint8_t*) bmpResult.Bits,
						 bmpResult.getPitch(),bmpResult.getHeight(),
						 spcSpec.TargetX + spcSpec.FacetWidth * ( spcSpec.Reverse ? (spcSpec.Count-1-(iFrame-spcSpec.First)) : (iFrame-spcSpec.First) ),
						 spcSpec.TargetY,
						 spcSpec.FacetWidth,spcSpec.FacetHeight,
						 spcSpec.BitDepth/8,
						 spcSpec.Flip);
	}
	return true;
#else
	return false;
#endif
}

bool aniGrabFramePng(AniSpec &spcSpec, int iFrame, CStdBitmap &bmpResult)
{
	// Get filename
	char strFilename[2048+1];
	getSequenceFilename(strPngName,iFrame,spcSpec,strFilename);
	// Load png
	BYTE *pBits;							 // Notice for the blit we're using the previously determined specs,
	int iWidth,iHeight;				 // might alternatively use each image's values here...
	//pBits = new BYTE [spcSpec.Pitch * spcSpec.Height];
	if (!loadPng(strFilename,pBits,iWidth,iHeight,spcSpec.BitDepth/8,true))
		return false;
	// Blit facet to result bitmap
	StdBlit( (uint8_t*) pBits,
					 spcSpec.Pitch,spcSpec.Height,
					 spcSpec.FacetX,spcSpec.FacetY,spcSpec.FacetWidth,spcSpec.FacetHeight,
					 (uint8_t*) bmpResult.Bits,
					 bmpResult.getPitch(),bmpResult.getHeight(),
					 spcSpec.TargetX + spcSpec.FacetWidth * ( spcSpec.Reverse ? (spcSpec.Count-1-(iFrame-spcSpec.First)) : (iFrame-spcSpec.First) ),
					 spcSpec.TargetY,
					 spcSpec.FacetWidth,spcSpec.FacetHeight,
					 spcSpec.BitDepth/8,
					 spcSpec.Flip);
	// Deallocate buffer
	delete [] pBits;
	// Success
	return true;
}

bool aniGrabFrame(AniSpec &spcSpec, int iFrame, CStdBitmap &bmpResult)
{
#ifdef _WIN32
	// Avi
	if (hAviStream && hGetFrame)
		return aniGrabFrameAvi(spcSpec,iFrame,bmpResult);
#endif
	// Png sequence
	if (strPngName[0])
		return aniGrabFramePng(spcSpec,iFrame,bmpResult);
	// Unknown type
	return false;
}

bool addAniSegment(const char *szInput, CStdBitmap &bmpResult, int &iTargetX, int &iTargetY)
{
	// Log
	printf("%s\n",szInput);

	// Scan filename and flip/reverse options
	char szFilename[_MAX_PATH+1];
	bool fFlip=false,fReverse=false;
	SCopyUntil(szInput, szFilename, '=');
	while ((szFilename[0]=='!') || (szFilename[0]=='?'))
	{
		if (szFilename[0]=='!') fFlip=true; 
		if (szFilename[0]=='?') fReverse=true; 
		SCopy(szFilename + 1, szFilename); 
	}
	
	// Scan source
	AniSpec srcSpec;
	if (!aniOpenSource(szFilename, srcSpec))
		return false;

	// Store current output position in spec
	srcSpec.TargetX = iTargetX;
	srcSpec.TargetY = iTargetY;

	// Override specs by parameter
	const char *cpMeasure;
	char szMeasurement[_MAX_PATH+1];
	// Flip and reverse
	srcSpec.Flip = fFlip;
	srcSpec.Reverse = fReverse;
	// Measurements
	if (cpMeasure=SSearch(szInput,"="))
		for (int iMeasure=0; SCopySegment(cpMeasure,iMeasure,szMeasurement,'&') && szMeasurement[0]; iMeasure++)
		{
			// Source frame range
			if (SCharCount('-',szMeasurement)==1)
				{ sscanf(szMeasurement,"%d-%d",&srcSpec.First,&srcSpec.Last); srcSpec.Count=srcSpec.Last-srcSpec.First+1; }
			// Source facet
			else if (SCharCount(',',szMeasurement)==3)
				sscanf(szMeasurement,"%d,%d,%d,%d",&srcSpec.FacetX,&srcSpec.FacetY,&srcSpec.FacetWidth,&srcSpec.FacetHeight);
			// Target coordinates
			else if (SCharCount(',',szMeasurement)==1)
				sscanf(szMeasurement,"%d,%d",&srcSpec.TargetX,&srcSpec.TargetY);
			// Single frame
			else
				{ sscanf(szMeasurement,"%d",&srcSpec.First); srcSpec.Last=srcSpec.First; srcSpec.Count=1; }
		}		

	// Ensure target bitmap to suit spec and target coordinates
	if (!bmpResult.Ensure( srcSpec.TargetX + srcSpec.FacetWidth * srcSpec.Count,
												 srcSpec.TargetY + srcSpec.FacetHeight,
												 srcSpec.BitDepth ))
		{ printf("Error on target bitmap creation\n"); aniCloseSource(); return false; }
	
	// Grab facets from frames
	for (int iFrame=srcSpec.First; iFrame<=srcSpec.Last; iFrame++)
		if (!aniGrabFrame(srcSpec,iFrame,bmpResult))
			{ printf("Error on frame %i\n",iFrame); aniCloseSource(); return false; }


	// Store output coordinates for return
	iTargetX = srcSpec.TargetX;
	iTargetY = srcSpec.TargetY;
	// Advance vertical output position
	iTargetY += srcSpec.FacetHeight;

	// Close source
	aniCloseSource();

	// Success 
	return true;
}

bool grabAni(const char *szInput, const char *szOutput)
{
	char szSegment[_MAX_PATH+1];

	// Result bitmap
	CStdBitmap bmpResult;
	int iTargetX=0,iTargetY=0;

	// Process input sections
	for (int iInput=0; SCopySegment(szInput,iInput,szSegment,';') && szSegment[0]; iInput++)
		if (!addAniSegment(szSegment,bmpResult,iTargetX,iTargetY))
			return false;

	// Save result bitmap
	printf("Saving %s...\n",szOutput);

	// BMP
	if (SEqualNoCase(GetExtension(szOutput), "bmp"))
	{
		if (!bmpResult.Save(szOutput))
			return false;
	}
	// PNG
	else
	{
		if (!SavePNG(szOutput, bmpResult.Bits, bmpResult.getWidth(), bmpResult.getHeight(), 4))
			return false;
	}

	return true;
}



int main(int argc, char *argv[])
{
	char szInput[10000 + 1]="";
	char szOutput[1000 + 1]="";
 
  printf("\n%s\n", VERSION_STRING);

	// Print usage
	if (argc<2)
	{
		printf("\n");

		printf("  Usage: anigrab <input file(s)> <output file>\n\n");	

		printf("  Input: 24-bit rgb uncompressed avi(s)\n");
		printf("         32-bit rgba numbered png sequence(s)\n\n");

		printf(" Output: 24-bit rgb bmp\n");
		printf("         32-bit rgba png\n\n");

		printf("  Specs: !walk.png           flip frames\n");
		printf("         ?walk.png           reverse frame order\n");
		printf("         walk.png=12         single source frame\n");
		printf("         walk.png=1-10       source frame range: first-last\n");
		printf("         walk.png=5,5,10,10  source frame facet: x,y,wdt,hgt\n");
		printf("         walk.png=160,75     target coordinates: x,y\n");
		printf("\n");

		printf("Example: anigrab ?!walk.avi=2-10&10,10,25,15 jump0000.png graphics.png\n");
	}

	// Parse parameters
	for (int carg=1; carg<argc; carg++)
	{
		// Input file
		if (carg < argc-1)
		{
			SNewSegment(szInput, ";");
			SAppend(argv[carg], szInput);
			continue;
		}
		// Output file
		else
		{
			SCopy(argv[carg],szOutput);
			continue;
		}
	}

	// Process files
	if (szInput[0] && szOutput[0])
	{
		//printf("Input: %s\nOutput: %s\n",szInput,szOutput);
    grabAni(szInput,szOutput);
	}

	return 0;
	};
