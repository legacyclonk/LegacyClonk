/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Loads a version info resource from executable files */

#include "C4Explorer.h"
#include "FileVersion.h"

//---------------------------------------------------------------------------

#pragma comment(lib, "version")

//---------------------------------------------------------------------------

CFileVersion::CFileVersion(LPCTSTR ModuleName)
{ 
	pVersionData = NULL;
	LangCharset = 0;

	if (ModuleName) Open(ModuleName);
}

//---------------------------------------------------------------------------

CFileVersion::~CFileVersion() 
{ 
	Close();
} 

//---------------------------------------------------------------------------

void CFileVersion::Close()
{
	delete [] pVersionData; 
	pVersionData = NULL;
	LangCharset = 0;
}

//---------------------------------------------------------------------------

BOOL CFileVersion::Open(LPCTSTR ModuleName)
{
	ASSERT(_tcslen(ModuleName) > 0);
	ASSERT(pVersionData == NULL);

	// Get the version information size for allocate the buffer
	DWORD Handle;	 
	DWORD DataSize = ::GetFileVersionInfoSize((LPTSTR)ModuleName, &Handle); 
	if (DataSize == 0) return false;

	// Allocate buffer and retrieve version information
	pVersionData = new BYTE[DataSize]; 
	if (!::GetFileVersionInfo((LPTSTR)ModuleName, Handle, DataSize, (void**)pVersionData))
	{
		Close();
		return false;
	}

	// Retrieve the first language and character-set identifier
	UINT QuerySize;
	DWORD* pTransTable;
	if (!::VerQueryValue(pVersionData, _T("\\VarFileInfo\\Translation"), (void **)&pTransTable, &QuerySize) )
	{
		Close();
		return false;
	}

	// Swap the words to have lang-charset in the correct format
	LangCharset = MAKELONG(HIWORD(pTransTable[0]), LOWORD(pTransTable[0]));

	return true;
}

//---------------------------------------------------------------------------

CString CFileVersion::QueryValue(LPCTSTR ValueName, DWORD OtherLangCharset)
{
	// Must call Open() first
	ASSERT(pVersionData != NULL);
	if (pVersionData == NULL) return (CString)_T("");

	// If no lang-charset specified use default
	if (!OtherLangCharset) OtherLangCharset = LangCharset;

	// Query version information value
	UINT QuerySize;
	LPVOID lpData;
	CString strValue, strBlockName;
	strBlockName.Format(_T("\\StringFileInfo\\%08lx\\%s"), OtherLangCharset, ValueName);
	if (::VerQueryValue((void **)pVersionData, strBlockName.GetBuffer(0), &lpData, &QuerySize) )
		strValue = (LPCTSTR)lpData;

	strBlockName.ReleaseBuffer();
	return strValue;
}

//---------------------------------------------------------------------------

BOOL CFileVersion::GetFixedInfo(VS_FIXEDFILEINFO& vsffi)
{
	// Must call Open() first
	ASSERT(pVersionData != NULL);
	if (pVersionData == NULL) return false;

	UINT QuerySize;
	VS_FIXEDFILEINFO* pVsffi;
	if (::VerQueryValue((void **)pVersionData, _T("\\"), (void**)&pVsffi, &QuerySize))
	{
		vsffi = *pVsffi;
		return true;
	}

	return false;
}

//---------------------------------------------------------------------------

CString CFileVersion::GetFixedFileVersion()
{
	CString Version;
	VS_FIXEDFILEINFO vsffi;

	if (GetFixedInfo(vsffi))
	{
		Version.Format ("%u,%u,%u,%u", HIWORD(vsffi.dwFileVersionMS),
			LOWORD(vsffi.dwFileVersionMS),
			HIWORD(vsffi.dwFileVersionLS),
			LOWORD(vsffi.dwFileVersionLS));
	}
	return Version;
}

//---------------------------------------------------------------------------

CString CFileVersion::GetFixedProductVersion()
{
	CString Version;
	VS_FIXEDFILEINFO vsffi;

	if (GetFixedInfo(vsffi))
	{
		Version.Format ("%u,%u,%u,%u", HIWORD(vsffi.dwProductVersionMS),
			LOWORD(vsffi.dwProductVersionMS),
			HIWORD(vsffi.dwProductVersionLS),
			LOWORD(vsffi.dwProductVersionLS));
	}
	return Version;
}

//---------------------------------------------------------------------------
