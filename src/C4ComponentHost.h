/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Holds a single text file component from a group */

#pragma once

#include "C4GroupSet.h"

class C4ComponentHost
{
public:
	C4ComponentHost();
	virtual ~C4ComponentHost();
	const char *GetFilePath() const { return FilePath; }
	void Default();
	void Clear();
	void Open();
	const char *GetData() { return Data.getData(); }
	size_t GetDataSize() { return Data.getLength(); }
	virtual void Close();
	bool Load(const char *szName, C4Group &hGroup, const char *szFilename, const char *szLanguage = nullptr);
	bool Load(const char *szName, C4GroupSet &hGroupSet, const char *szFilename, const char *szLanguage = nullptr);
	bool LoadEx(const char *szName, C4Group &hGroup, const char *szFilename, const char *szLanguage = nullptr);
	bool LoadAppend(const char *szName, C4Group &hGroup, const char *szFilename, const char *szLanguage = nullptr);
	bool Save(C4Group &hGroup);
	bool GetLanguageString(const char *szLanguage, class StdStrBuf &rTarget);
	void TrimSpaces();

protected:
	StdCopyStrBuf Data;
	bool Modified;
	char Name[_MAX_FNAME + 1];
	char Filename[_MAX_FNAME + 1];
	char FilePath[_MAX_PATH + 1];
	void CopyFilePathFromGroup(const C4Group &hGroup);
#ifdef _WIN32
	HWND hDialog;
	void InitDialog(HWND hDlg);
	friend BOOL CALLBACK ComponentDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
#endif
};
