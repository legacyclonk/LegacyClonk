/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Lots of helper functions */

#include "C4Explorer.h"

#include "DllGetVersion.h"

bool GetPortrait(BYTE **ppBytes, unsigned int *ipSize)
	{
	// Old style - select random portrait from Graphics.c4g
	C4Group hGroup;
	int iCount;
	CString EntryName;
	if (!hGroup.Open(GetCfg()->AtExePath(C4CFN_Graphics))) return false;
	if ((iCount = hGroup.EntryCount("Portrait*.png")) < 1 ) return false;
	EntryName.Format("Portrait%d.png",Random(iCount)+1);
	if (!hGroup.LoadEntry(EntryName,reinterpret_cast<char **>(ppBytes),ipSize)) return false;
	hGroup.Close();
	return true;

	// New style: load portrait binary resource included in frontend
	/*if (GetBinaryResource(IDR_PORTRAIT, ppBytes, ipSize))
		{
		// Copy to a newly allocated buffer as it would be expected from C4Group::LoadEntry
		BYTE *bypNewbuf = new BYTE [*ipSize];
		MemCopy(*ppBytes, bypNewbuf, *ipSize);
		*ppBytes = bypNewbuf;
		return true;
		}*/

	return false;
	}

void LoadImageList(UINT ID, CImageList& ImgList, int Width, int Height)
	{
	CBitmap ListBmp;
	ListBmp.LoadBitmap(ID);
	const int nGrow = 20;  // Bei Bedarf anpassen!
	ImgList.DeleteImageList();
	ImgList.Create(Width, Height, ILC_COLOR16 | ILC_MASK, 0, nGrow);
	ImgList.Add(&ListBmp, TRANSPCOLOR);
	}

void LoadPalette(UINT ID, CPalette& Palette)
	{
	HRSRC hRsrc = FindResource(AfxGetResourceHandle(), MAKEINTRESOURCE(ID), RT_BITMAP);
	HGLOBAL hBMI = LoadResource(AfxGetResourceHandle(), hRsrc);
	BITMAPINFO* pBMI = (BITMAPINFO*) LockResource(hBMI);
	LOGPALETTE* pLogPal = (LOGPALETTE*) new char[sizeof(LOGPALETTE) + sizeof(PALETTEENTRY)*255];
	pLogPal->palVersion = 0x300;
	pLogPal->palNumEntries = 256;
	for (int i=0; i<256; i++)
		{
		pLogPal->palPalEntry[i].peRed   = pBMI->bmiColors[i].rgbRed;
		pLogPal->palPalEntry[i].peGreen = pBMI->bmiColors[i].rgbGreen;
		pLogPal->palPalEntry[i].peBlue  = pBMI->bmiColors[i].rgbBlue;
		pLogPal->palPalEntry[i].peFlags = 0;
		}
	HPALETTE hPalette = CreatePalette(pLogPal);
	Palette.Attach(hPalette);
	delete [] pLogPal;
	}

C4ExplorerApp *GetApp() 
	{
	return ((C4ExplorerApp*) AfxGetApp()); 
	}

C4ExplorerCfg *GetCfg() 
	{
	return &(GetApp()->Config); 
	}

CString MakeRTFBar(const char *szText, int iValue, int iColor)
	{
	CString ParLineFeed = "\\par"; ParLineFeed += LineFeed;
	CString sResult,sColor;

	sResult.Format("\\cf0%s\\tab\\cf%i",szText,iColor);

	iValue = BoundBy(iValue/4,0,20);

	for (int cnt=0; cnt<iValue; cnt++) sResult += "•"; // make bar remainder light gray!

	sResult += ParLineFeed;

	return sResult;
	}

CString MakeRTF(const char *szTitle, const char *szText, const char *szRTF)
	{

	CString ParLineFeed = "\\par"; ParLineFeed += LineFeed;
	CString sString;	

	// Header
	CString strCharset; strCharset.Format("\\fcharset%i", GetApp()->GetCharsetCode(GetCfg()->General.LanguageCharset));
	sString += "{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang1031{\\fonttbl {\\f0\\fnil" + strCharset + " Times New Roman;}}"; 
	sString += LineFeed;

	// Color table
	CString sColor;
	sString += "{\\colortbl ;";
	for (int cnt=0; cnt<12; cnt++)
		{
		sColor.Format("\\red%i\\green%i\\blue%i;",255-255*cnt/11,0,255*cnt/11);
		sString += sColor;
		}
	sString += "}"; sString += LineFeed;

	// Title
	if (szTitle && szTitle[0])
		{
		sString += "\\uc1\\pard\\tx1136\\ulnone\\b\\f0\\fs20 ";
		sString += szTitle;	sString += ParLineFeed;
		sString += "\\b0\\fs16\\par";	sString += LineFeed;
		}
	else
		{
		sString += "\\uc1\\pard\\tx1136\\ulnone\\f0\\fs16 ";
		}	

	// Process unformatted text - rewritten to avoid crashes...
	CStringEx sText;
	for (const char *pPos = szText; *pPos; pPos++)
		{
			// Prepend special characters with backslash
			if ((*pPos == '\\') || (*pPos == '{') || (*pPos == '}'))	sText += "\\";
			// Prepend line feed with \par tag
			if (*pPos == 0x0D) sText += "\\par";
			// Append character
			sText += *pPos;
		}

	// Append now formatted text
	sString += sText; 
	
	// Append pre-formatted text
	if (szRTF)
		sString += szRTF;

	// End of document
	sString += ParLineFeed; sString += "}";
	sString += LineFeed; sString += EndOfFile;

	return sString;
	}

CString GetPath(CString Path)
	{
	int NameStart = Path.ReverseFind('\\') + 1;
	if (NameStart > 0) return Path.Left(NameStart);
	else return "";
	}

bool PathWritable(CString Path)
	{
	if (Path.Right(1) != "\\") Path += "\\";
	CString TestFileName = Path + "~Test.tmp";
	DeleteFile(TestFileName);
	HANDLE hFile = CreateFile(TestFileName, GENERIC_WRITE, 0, NULL, 
		CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
		{
		CloseHandle(hFile);
		DeleteFile(TestFileName);
		return true;
		}
	else return false;
	}

CString NoBackslash(CString Path)
	{
	if (Path.Right(1)=="\\") return Path.Left(Path.GetLength()-1);
	return Path;
	}
	
bool SaveBinaryResource(WORD id, const char *szFilename)
	{
	BYTE *pBytes; int iSize;
	CStdFile hFile;
	if (!GetBinaryResource(id,&pBytes,&iSize)) return FALSE;
	if (!hFile.Save(szFilename,pBytes,iSize)) return FALSE;
	return TRUE;
	}

bool GetBinaryResource(WORD id, BYTE **ppBytes, int *ipSize)
	{
	HRSRC hRsrc = FindResource(AfxGetResourceHandle(), MAKEINTRESOURCE(id), "Binary");
	int iSize = SizeofResource(AfxGetResourceHandle(),hRsrc);
	HGLOBAL hGlbl = LoadResource(AfxGetResourceHandle(), hRsrc);
	BYTE *pBytes = (BYTE*) LockResource(hGlbl);
	*ppBytes = pBytes; *ipSize=iSize;
	if (*ppBytes) return TRUE;
	return FALSE;
	}

CString GetTextResource(WORD id)
	{
	CString iResult;
	char *cpBuffer;
	HRSRC hRsrc = FindResource(AfxGetResourceHandle(), MAKEINTRESOURCE(id), "Text");
	int iSize = SizeofResource(AfxGetResourceHandle(),hRsrc);
	HGLOBAL hGlbl = LoadResource(AfxGetResourceHandle(), hRsrc);
	cpBuffer = new char [iSize+1];
	SCopy( (char*) LockResource(hGlbl), cpBuffer); cpBuffer[iSize]=0;
	iResult = cpBuffer;
	delete [] cpBuffer;
	return iResult;
	}

CString TimeString(int iSeconds)
	{
	int iHours = iSeconds / 3600; iSeconds -= 3600*iHours;
	int iMinutes = iSeconds / 60; iSeconds -= 60*iMinutes;
	CString Result; Result.Format("%02d:%02d:%02d",iHours,iMinutes,iSeconds);
	return Result;
	}

CString DateString(int iTime)
	{
	time_t tTime = iTime; //time(&tTime);
	struct tm *pLocalTime;
  pLocalTime=localtime(&tTime);
	CString Result; Result.Format(	"%02d.%02d.%d %02d:%02d",
																 pLocalTime->tm_mday,
																 pLocalTime->tm_mon+1,
																 pLocalTime->tm_year+1900,
																 pLocalTime->tm_hour,
																 pLocalTime->tm_min);
	return Result;
	}

CString SaveFileDlg(CString FileFilter, PCSTR FileName)
	{
	CFileDialog FileDlg(false, NULL, FileName, OFN_HIDEREADONLY | OFN_NOCHANGEDIR, FileFilter);
	if (FileDlg.DoModal() == IDOK) return FileDlg.GetPathName();
	else return CString();
	}

CString OpenFileDlg(CString FileFilter, PCSTR FileName)
	{
	CFileDialog FileDlg(true, NULL, FileName, OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_FILEMUSTEXIST, FileFilter);
	if (FileDlg.DoModal() == IDOK) return FileDlg.GetPathName();
	else return CString();
	}

//---------------------------------------------------------------------------

bool SetRegValue(HKEY hBaseKey, PCSTR Key, PCSTR ValueName, DWORD Value)
{
	HKEY hKey = NULL;
	if (RegOpenKey(hBaseKey, Key, &hKey) != ERROR_SUCCESS) return false;
	if (RegSetValueEx(hKey, ValueName, 0, REG_DWORD, (BYTE*)&Value, sizeof Value) 
		!= ERROR_SUCCESS) return false;
	if (RegCloseKey(hKey) != ERROR_SUCCESS) return false;
	return true;
}

//---------------------------------------------------------------------------

bool SetRegValue(HKEY hBaseKey, PCSTR Key, PCSTR ValueName, CString Value)
{
	HKEY hKey = NULL;
	if (RegOpenKey(hBaseKey, Key, &hKey) != ERROR_SUCCESS) return false;
	if (RegSetValueExA(hKey, ValueName, 0, REG_SZ, (BYTE*)(PCSTR)Value, lstrlen(Value)+1) 
		!= ERROR_SUCCESS) return false;
	if (RegCloseKey(hKey) != ERROR_SUCCESS) return false;
	return true;
}

//---------------------------------------------------------------------------

bool DoInputDlg(const char *szPrompt, CString &rInput)
	{
	CInputDlg InputDlg;
	InputDlg.m_Prompt = szPrompt;
	InputDlg.m_Input = rInput;
	if (InputDlg.DoModal() != IDOK) return false;
	rInput = InputDlg.m_Input;
	if (rInput.IsEmpty()) return false;
	return true;
	}

int GetComCtlVersion()
{
	int Result = 0;
	HINSTANCE hComCtl;
	hComCtl = LoadLibrary("comctl32.dll");
	if (hComCtl)
	{
		HRESULT hr = S_OK;
		DLLGETVERSIONPROC pDllGetVersion;
		pDllGetVersion = (DLLGETVERSIONPROC) GetProcAddress(hComCtl, "DllGetVersion");
		if (pDllGetVersion)
		{
			DLLVERSIONINFO DVI;
			ZeroMemory(&DVI, sizeof DVI);
			DVI.cbSize = sizeof(DVI);
			hr = (*pDllGetVersion)(&DVI);
			if (SUCCEEDED(hr)) Result = DVI.dwMajorVersion * 100 + DVI.dwMinorVersion;
		}
		else Result = 400;
 		FreeLibrary(hComCtl);
	}
 	return Result;
}

bool DrawDecoPic(CImageList& ImgList, CDC& DC, const CWnd& Wnd, int iImage)
{
	if (Wnd.IsWindowVisible())
	{
		CRect Rect;
		Wnd.GetWindowRect(Rect);
		CWnd* pParentWnd = Wnd.GetParent();
		if (pParentWnd) pParentWnd->ScreenToClient(&Rect);
		else return false;
		return ImgList.Draw(&DC, iImage, CPoint(Rect.left,Rect.top), ILD_TRANSPARENT) == TRUE;
	}
	else return false;
}

/////////////////////////////////////////////////////////////////////////////
//
// MATHE

// Einen Wert von Wertebereich1 auf Wertebereich2 skalieren

int Scale(int v, int min1, int max1, int min2, int max2)
{
	return Round((max2-min2) * (float)(v-min1) / (float)(max1-min1) + min2);
}

//---------------------------------------------------------------------------

// Warum hat die RTL keine Rundungsfunktion?!

int Round(double n)
{
	if (n - (int)n >= 0.5) return (int)n + 1;
	else return (int)n;
}

//---------------------------------------------------------------------------

// Random, default 0-100

int Rand(int Min, int Max)
{
	//return Round(((float)rand() / RAND_MAX) * (Max - Min) + Min);
	return Scale(rand(), 0, RAND_MAX, Min, Max);
}

//---------------------------------------------------------------------------

int Limit(int n, int Min, int Max)
{
	if (n < Min) return Min;
	else if (n > Max) return Max;
	else return n;
}

int LimitEditValue(CEdit& Edit, int Min, int Max)
{
	CString NumString;
	Edit.GetWindowText(NumString);
	int Value = atoi(NumString);
	Value = Limit(Value, Min, Max);
	NumString.Format("%i", Value);
	Edit.SetWindowText(NumString);
	return Value;
}

void SetStatus(const char *szStatus)
	{
	GetApp()->m_pMainWnd->SendMessage(WM_USER_SETSTATUS, 0, (LPARAM)szStatus);
	}

void SetStatusAnimation(WORD id)
	{
	GetApp()->m_pMainWnd->SendMessage(WM_USER_SETSTATUS, id, (LPARAM)0);
	}

void RemoveCharacters(CString &rString, const char *szCharList)
	{
	CString sResult;
	for (const char *cpChar=rString; *cpChar; cpChar++)
		if (!SCharCount(*cpChar,szCharList))
			sResult += *cpChar;
	rString = sResult;
	}

bool EngineVersionOk(int engVer1,int engVer2,int engVer3,int engVer4, int ver1,int ver2,int ver3,int ver4, bool fIsNetworkReference)
	{
	if (fIsNetworkReference)
		{
		if (engVer1!=ver1) return false;
		if (engVer2!=ver2) return false;
		if (engVer3!=ver3) return false;
		if (engVer4!=ver4 && ver2==9) return false;
		return true;
		}
	//if (ver2==9 && ver3!=engVer3) return false; ...disabled this GWE version check
	return EngineVersionGreater(engVer1,engVer2,engVer3,engVer4,ver1,ver2,ver3,ver4,true);
	}

bool EngineVersionGreater(int engVer1,int engVer2,int engVer3,int engVer4, int ver1,int ver2,int ver3,int ver4, bool fEqualAllowed)
	{
	if (engVer1>ver1) return true; if (engVer1<ver1) return false;
	if (engVer2>ver2) return true; if (engVer2<ver2) return false;
	if (engVer3>ver3) return true; if (engVer3<ver3) return false;
	if (engVer4>ver4) return true; if (engVer4<ver4) return false;
	return fEqualAllowed;
	}

bool EngineVersionLess(int engVer1,int engVer2,int engVer3,int engVer4, int ver1,int ver2,int ver3,int ver4, bool fEqualAllowed)
	{
	return !EngineVersionGreater(engVer1,engVer2,engVer3,engVer4,ver1,ver2,ver3,ver4,!fEqualAllowed);
	}

DWORD RGB2BGR(DWORD dwClr)
	{
	return RGB(GetBValue(dwClr), GetGValue(dwClr), GetRValue(dwClr));
	}

/* Microsoft Knowledge Base Article - 29240 */

int H,L,S;

#define	 RANGE    255
#define  HLSMAX   RANGE /* H,L, and S vary over 0-HLSMAX */ 
#define  RGBMAX   255   /* R,G, and B vary over 0-RGBMAX */ 
                           /* HLSMAX BEST IF DIVISIBLE BY 6 */ 
                           /* RGBMAX, HLSMAX must each fit in a byte. */ 

/* Hue is undefined if Saturation is 0 (grey-scale) */ 
/* This value determines where the Hue scrollbar is */ 
/* initially set for achromatic colors */ 
#define UNDEFINED (HLSMAX*2/3)

void  RGBtoHLS(DWORD lRGBColor)
{
  WORD R,G,B;          /* input RGB values */ 
  BYTE cMax,cMin;      /* max and min RGB values */ 
	WORD  Rdelta,Gdelta,Bdelta; /* intermediate value: % of spread from max*/ 
  /* get R, G, and B out of DWORD */ 
  R = GetRValue(lRGBColor);
  G = GetGValue(lRGBColor);
  B = GetBValue(lRGBColor);

  /* calculate lightness */ 
  cMax = (BYTE) max( max(R,G), B);
  cMin = (BYTE) min( min(R,G), B);
  L = ( ((cMax+cMin)*HLSMAX) + RGBMAX )/(2*RGBMAX);

  if (cMax == cMin) {           /* r=g=b --> achromatic case */ 
     S = 0;                     /* saturation */ 
     H = UNDEFINED;             /* hue */ 
  }
  else {                        /* chromatic case */ 
     /* saturation */ 
     if (L <= (HLSMAX/2))
        S = ( ((cMax-cMin)*HLSMAX) + ((cMax+cMin)/2) ) / (cMax+cMin);
     else
        S = ( ((cMax-cMin)*HLSMAX) + ((2*RGBMAX-cMax-cMin)/2) )
           / (2*RGBMAX-cMax-cMin);

     /* hue */ 
  Rdelta = ( ((cMax-R)*(HLSMAX/6)) + ((cMax-cMin)/2) ) / (cMax-cMin);
  Gdelta = ( ((cMax-G)*(HLSMAX/6)) + ((cMax-cMin)/2) ) / (cMax-cMin);
  Bdelta = ( ((cMax-B)*(HLSMAX/6)) + ((cMax-cMin)/2) ) / (cMax-cMin);

     if (R == cMax)
        H = Bdelta - Gdelta;
     else if (G == cMax)
        H = (HLSMAX/3) + Rdelta - Bdelta;
     else /* B == cMax */ 
        H = ((2*HLSMAX)/3) + Gdelta - Rdelta;

     if (H < 0)
        H += HLSMAX;
     if (H > HLSMAX)
        H -= HLSMAX;
  }
}

/* utility routine for HLStoRGB */ 
WORD HueToRGB(WORD n1, WORD n2, WORD hue)
{
  /* range check: note values passed add/subtract thirds of range */ 
  if (hue < 0)
     hue += HLSMAX;

  if (hue > HLSMAX)
     hue -= HLSMAX;

  /* return r,g, or b value from this tridrant */ 
  if (hue < (HLSMAX/6))
      return ( n1 + (((n2-n1)*hue+(HLSMAX/12))/(HLSMAX/6)) );
  if (hue < (HLSMAX/2))
     return ( n2 );
  if (hue < ((HLSMAX*2)/3))
     return ( n1 +    (((n2-n1)*(((HLSMAX*2)/3)-hue)+(HLSMAX/12))/(HLSMAX/6))
);
  else
     return ( n1 );
}

DWORD HLStoRGB(WORD hue, WORD lum, WORD sat)
{
  WORD R,G,B;                /* RGB component values */ 
  WORD  Magic1,Magic2;       /* calculated magic numbers (really!) */ 

  if (sat == 0) 
	{            
		/* achromatic case */ 
    R=G=B=(lum*RGBMAX)/HLSMAX;
    if (hue != UNDEFINED) 
		{
        /* ERROR */ 
      }
   }
  else  {                    /* chromatic case */ 
     /* set up magic numbers */ 
     if (lum <= (HLSMAX/2))
        Magic2 = (lum*(HLSMAX + sat) + (HLSMAX/2))/HLSMAX;
     else
        Magic2 = lum + sat - ((lum*sat) + (HLSMAX/2))/HLSMAX;
     Magic1 = 2*lum-Magic2;

     /* get RGB, change units from HLSMAX to RGBMAX */ 
     R = (HueToRGB(Magic1,Magic2,hue+(HLSMAX/3))*RGBMAX + (HLSMAX/2))/HLSMAX;
     G = (HueToRGB(Magic1,Magic2,hue)*RGBMAX + (HLSMAX/2)) / HLSMAX;
     B = (HueToRGB(Magic1,Magic2,hue-(HLSMAX/3))*RGBMAX + (HLSMAX/2))/HLSMAX;
  }
  return(RGB(R,G,B));
}

/* Microsoft Knowledge Base Article - 29240 END */

bool IsBlueColor(DWORD &rClr)
{
	RGBtoHLS(rClr);
	// What's a blue hue?
	return Inside(H, 145, 175) && (S > 100);
}
