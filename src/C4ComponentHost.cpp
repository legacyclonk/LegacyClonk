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

/* Holds a single text file component from a group */

#include <C4Include.h>
#include <C4ComponentHost.h>
#include <C4Application.h>
#include <C4Language.h>
#include <StdRegistry.h>

C4ComponentHost *pCmpHost = nullptr;

extern C4Config *pConfig; // Some cheap temporary hack to get to config in Engine + Frontend...
// This is so good - we can use it everywear!!!!

#if defined(C4ENGINE) && defined(_WIN32)

BOOL CALLBACK ComponentDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (!pCmpHost) return FALSE;

	switch (Msg)
	{
	case WM_CLOSE:
		pCmpHost->Close();
		break;

	case WM_DESTROY:
		StoreWindowPosition(hDlg, "Component", Config.GetSubkeyPath("Console"), false);
		break;

	case WM_INITDIALOG:
		pCmpHost->InitDialog(hDlg);
		RestoreWindowPosition(hDlg, "Component", Config.GetSubkeyPath("Console"));
		return TRUE;

	case WM_COMMAND:
		// Evaluate command
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			pCmpHost->Close();
			return TRUE;

		case IDOK:
			// IDC_EDITDATA to Data
			char buffer[65000];
			GetDlgItemText(hDlg, IDC_EDITDATA, buffer, 65000);
			pCmpHost->Modified = true;
			pCmpHost->Data.Copy(buffer);
			pCmpHost->Close();
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}

void C4ComponentHost::InitDialog(HWND hDlg)
{
	hDialog = hDlg;
	// Set text
	SetWindowText(hDialog, Name);
	SetDlgItemText(hDialog, IDOK, LoadResStr("IDS_BTN_OK"));
	SetDlgItemText(hDialog, IDCANCEL, LoadResStr("IDS_BTN_CANCEL"));
	if (Data.getLength())   SetDlgItemText(hDialog, IDC_EDITDATA, Data.getData());
}

#endif

C4ComponentHost::C4ComponentHost()
{
	Default();
}

C4ComponentHost::~C4ComponentHost()
{
	Clear();
}

void C4ComponentHost::Default()
{
	Data.Clear();
	Modified = false;
	Name[0] = 0;
	Filename[0] = 0;
	FilePath[0] = 0;
#ifdef _WIN32
	hDialog = nullptr;
#endif
}

void C4ComponentHost::Clear()
{
	Data.Clear();
#ifdef _WIN32
	if (hDialog) DestroyWindow(hDialog); hDialog = nullptr;
#endif
}

bool C4ComponentHost::Load(const char *szName,
	CppC4Group &group,
	const char *szFilename,
	const char *szLanguage)
{
	// Clear any old stuff
	Clear();
	// Store name & filename
	SCopy(szName, Name);
	SCopy(szFilename, Filename);
	// Load component - try all segmented filenames
	char strEntry[_MAX_FNAME + 1], strEntryWithLanguage[_MAX_FNAME + 1];
	for (int iFilename = 0; SCopySegment(Filename, iFilename, strEntry, '|', _MAX_FNAME); iFilename++)
	{
		// Try to insert all language codes provided into the filename
		char strCode[3] = "";
		for (int iLang = 0; SCopySegment(szLanguage ? szLanguage : "", iLang, strCode, ',', 2); iLang++)
		{
			// Insert language code
			sprintf(strEntryWithLanguage, strEntry, strCode);

			bool success = false;
			CppC4Group_ForEachEntryByWildcard(group, "", strEntryWithLanguage, [&group, &success, this](const auto &info)
			{
				CppC4Group_LoadEntryString(group, info.fileName, Data);
				if (pConfig->General.fUTF8) Data.EnsureUnicode();

				SCopy(info.fileName.c_str(), Filename, _MAX_FNAME);
				CopyFilePathFromGroup(group);
				return !(success = true);
			});

			if (success)
			{
				return true;
			}

			// Couldn't insert language code anyway - no point in trying other languages
			if (!SSearch(strEntry, "%s")) break;
		}
	}
	// Truncate any additional segments from stored filename
	SReplaceChar(Filename, '|', 0);
	CopyFilePathFromGroup(group);
	// Not loaded
	return false;
}

bool C4ComponentHost::Load(const char *szName,
	C4GroupSet &hGroupSet,
	const char *szFilename,
	const char *szLanguage)
{
	// Clear any old stuff
	Clear();
	// Store name & filename
	SCopy(szName, Name);
	SCopy(szFilename, Filename);
	// Load component - try all segmented filenames
	char strEntry[_MAX_FNAME + 1], strEntryWithLanguage[_MAX_FNAME + 1];
	for (int iFilename = 0; SCopySegment(Filename, iFilename, strEntry, '|', _MAX_FNAME); iFilename++)
	{
		// Try to insert all language codes provided into the filename
		char strCode[3] = "";
		for (int iLang = 0; SCopySegment(szLanguage ? szLanguage : "", iLang, strCode, ',', 2); iLang++)
		{
			// Insert language code
			sprintf(strEntryWithLanguage, strEntry, strCode);

			if (hGroupSet.LoadEntryString(strEntryWithLanguage, Data))
			{
				if (pConfig->General.fUTF8) Data.EnsureUnicode();

				// Store actual filename
				auto [group, fileName] = hGroupSet.FindEntry(strEntryWithLanguage);
				SCopy(fileName.c_str(), Filename, _MAX_FNAME);
				CopyFilePathFromGroup(*group);
				// Got it
				return true;
			}
			// Couldn't insert language code anyway - no point in trying other languages
			if (!SSearch(strEntry, "%s")) break;
		}
	}
	// Truncate any additional segments from stored filename
	SReplaceChar(Filename, '|', 0);
	// skip full path (unknown)
	FilePath[0] = 0;
	// Not loaded
	return false;
}

bool C4ComponentHost::LoadEx(const char *szName,
	CppC4Group &group,
	const char *szFilename,
	const char *szLanguage)
{
	// Load from a group set containing the provided group and
	// alternative groups for cross-loading from a language pack
	C4GroupSet hGroups;
	hGroups.RegisterGroup(group, false, 1000, C4GSCnt_Component); // Provided group gets highest priority
	hGroups.RegisterGroups(Languages.GetPackGroups(pConfig->AtExeRelativePath(group.getFullName().c_str())), C4GSCnt_Language);
	// Load from group set
	return Load(szName, hGroups, szFilename, szLanguage);
}

bool C4ComponentHost::LoadAppend(const char *szName,
	CppC4Group &group, const char *szFilename,
	const char *szLanguage)
{
	Clear();

	// Store name & filename
	SCopy(szName, Name);
	SCopy(szFilename, Filename);

	// Load component (segmented filename)
	char str1[_MAX_FNAME + 1], str2[_MAX_FNAME + 1];
	size_t iFileCnt = 0, iFileSizeSum = 0;
	int cseg, clseg;
	std::string fileName;
	for (cseg = 0; SCopySegment(Filename, cseg, str1, '|', _MAX_FNAME); cseg++)
	{
		char szLang[3] = "";
		for (clseg = 0; SCopySegment(szLanguage ? szLanguage : "", clseg, szLang, ',', 2); clseg++)
		{
			sprintf(str2, str1, szLang);

			CppC4Group_ForEachEntryByWildcard(group, "", str2, [&iFileCnt, &iFileSizeSum, &fileName](const auto &info) -> bool
			{
				++iFileCnt;
				iFileSizeSum += 1 + info.size;
				fileName = info.fileName;
				return false;
			});

			if (!SSearch(str1, "%s")) break;
		}
	}

	// No matching files found?
	if (!iFileCnt) return false;

	// Allocate memory
	Data.SetLength(iFileSizeSum);

	// Search files to read contents
	char *pPos = Data.getMData(); *pPos = 0;
	for (cseg = 0; SCopySegment(Filename, cseg, str1, '|', _MAX_FNAME); cseg++)
	{
		char szLang[3] = "";
		for (clseg = 0; SCopySegment(szLanguage ? szLanguage : "", clseg, szLang, ',', 2); clseg++)
		{
			sprintf(str2, str1, szLang);

			CppC4Group_ForEachEntryByWildcard(group, "", str2, [&group, &pPos, this](const auto &info) -> bool
			{
				auto data = group.getEntryData(info.fileName);
				if (data)
				{
					*pPos++ = '\n';
					SCopy(static_cast<const char *>(data->data), pPos, Data.getPtr(Data.getLength()) - pPos);
					return false;
				}

				return true;
			});

			if (!SSearch(str1, "%s")) break;
		}
	}

	SReplaceChar(Filename, '|', 0);
	CopyFilePathFromGroup(group);
	return !!iFileCnt;
}

// Construct full path
void C4ComponentHost::CopyFilePathFromGroup(CppC4Group &group)
{
	SCopy(pConfig->AtExeRelativePath(group.getFullName().c_str()), FilePath, _MAX_PATH - 1);
	SAppendChar(DirectorySeparator, FilePath);
	SAppend(Filename, FilePath, _MAX_PATH);
}

bool C4ComponentHost::Save(CppC4Group &group)
{
	if (!Modified) return true;
	if (!Data) return group.deleteEntry(Filename);

	StdStrBuf buf;
	buf.Copy(Data);
	return CppC4Group_Add(group, Filename, std::move(buf));
}

void C4ComponentHost::Open()
{
	pCmpHost = this;

#if defined(C4ENGINE) && defined(_WIN32)
	DialogBox(Application.hInstance,
		MAKEINTRESOURCE(IDD_COMPONENT),
		Application.pWindow->hWindow,
		(DLGPROC)ComponentDlgProc);
#endif

	pCmpHost = nullptr;
}

bool C4ComponentHost::GetLanguageString(const char *szLanguage, StdStrBuf &rTarget)
{
	const char *cptr;
	// No good parameters
	if (!szLanguage || !Data) return false;
	// Search for two-letter language identifier in text body, i.e. "DE:"
	char langindex[4] = "";
	for (int clseg = 0; SCopySegment(szLanguage ? szLanguage : "", clseg, langindex, ',', 2); clseg++)
	{
		SAppend(":", langindex);
		if (cptr = SSearch(Data.getData(), langindex))
		{
			// Return the according string
			int iEndPos = SCharPos('\r', cptr);
			if (iEndPos < 0) iEndPos = SCharPos('\n', cptr);
			if (iEndPos < 0) iEndPos = strlen(cptr);
			rTarget.Copy(cptr, iEndPos);
			return true;
		}
	}
	// Language string not found
	return false;
}

void C4ComponentHost::Close()
{
#ifdef _WIN32
	if (!hDialog) return;
	EndDialog(hDialog, 1);
	hDialog = nullptr;
#endif
}

void C4ComponentHost::TrimSpaces()
{
	Data.TrimSpaces();
	Modified = true;
}
