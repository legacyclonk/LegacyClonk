/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Lots of helper functions */

template<class T> void SetZero(T& Obj) { memset(&Obj, 0, sizeof Obj); };

#define WHITE       0x00FFFFFF
#define BLACK       0x00000000
#define TRANSPCOLOR 0x00FF00FF

#define IDOK_AUTOCOPY 12345

#define ID_TIMER_AUTOEDITSCAN 67890
#define ID_TIMER_WIPESTATUS		34567

const int C4MsgBox_Ok = 0,
					C4MsgBox_OkCancel = 1,
					C4MsgBox_YesNo = 2,
					C4MsgBox_Retry = 3;

bool GetPortrait(BYTE **ppBytes, unsigned int *ipSize);
void LoadImageList(UINT ID, CImageList& ImgList, int Width, int Height);
void LoadPalette(UINT ID, CPalette& Palette);
bool PathWritable(CString Path);
C4ExplorerApp *GetApp();
C4ExplorerCfg *GetCfg();
CString MakeRTF(const char *szTitle, const char *szText, const char *szRTF=NULL);
CString MakeRTFBar(const char *szText, int iValue, int iColor);
CString GetPath(CString Path);
CString NoBackslash(CString Path);
bool GetBinaryResource(WORD id, BYTE **ppBytes, int *ipSize);
bool SaveBinaryResource(WORD id, const char *szFilename);
CString GetTextResource(WORD id);
CString TimeString(int iSeconds);
CString DateString(int iTime);
int C4MessageBox(const char *szText, int iType = C4MsgBox_Ok, const char *szCheckPrompt = NULL, BOOL *fpCheck = NULL);
CString SaveFileDlg(CString FileFilter, PCSTR FileName);
CString OpenFileDlg(CString FileFilter, PCSTR FileName);
bool SetRegValue(HKEY hBaseKey, PCSTR Key, PCSTR ValueName, DWORD Value);
bool SetRegValue(HKEY hBaseKey, PCSTR Key, PCSTR ValueName, CString Value);
bool DoInputDlg(const char *szPrompt, CString &rInput);
int GetComCtlVersion();
bool DrawDecoPic(CImageList& ImgList, CDC& DC, const CWnd& Wnd, int iImage);
int Scale(int v, int min1, int max1, int min2, int max2);
int Round(double n);
int Rand(int Min, int Max);
int Limit(int n, int Min, int Max);
int LimitEditValue(CEdit& Edit, int Min, int Max);
void SetStatus(const char *szStatus);
void SetStatusAnimation(WORD id);
void RemoveCharacters(CString &rString, const char *szCharList);
bool EngineVersionOk(int engVer1,int engVer2,int engVer3,int engVer4, int ver1,int ver2,int ver3,int ver4, bool fIsNetworkReference=false);
bool EngineVersionGreater(int engVer1,int engVer2,int engVer3,int engVer4, int ver1,int ver2,int ver3,int ver4,bool fEqualAllowed=false);
bool EngineVersionLess(int engVer1,int engVer2,int engVer3,int engVer4, int ver1,int ver2,int ver3,int ver4,bool fEqualAllowed=false);
DWORD RGB2BGR(DWORD dwClr);
bool IsBlueColor(DWORD &rClr);