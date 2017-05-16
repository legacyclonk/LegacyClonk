/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Some wrappers for easier access to the Windows registry */

#pragma once

#ifdef _WIN32

#include <windows.h>
#include "StdCompiler.h"

BOOL GetRegistryDWord(HKEY hKey, const char *szSubKey,
	const char *szValueName, DWORD *lpdwValue);

BOOL GetRegistryString(const char *szSubKey, const char *szValueName, char *sValue, DWORD dwValSize);
BOOL SetRegistryString(const char *szSubKey, const char *szValueName, const char *szValue);

BOOL DeleteRegistryKey(HKEY hKey, const char *szSubKey);

BOOL SetRegClassesRoot(const char *szSubKey,
	const char *szValueName,
	const char *szStringValue);

BOOL SetRegShell(const char *szClassName,
	const char *szShellName,
	const char *szShellCaption,
	const char *szCommand,
	BOOL fMakeDefault = FALSE);

BOOL RemoveRegShell(const char *szClassName,
	const char *szShellName);

BOOL SetRegFileClass(const char *szClassRoot,
	const char *szExtension,
	const char *szClassName,
	const char *szIconPath, int iIconNum,
	const char *szContentType);

BOOL StoreWindowPosition(HWND hwnd,
	const char *szWindowName,
	const char *szSubKey,
	BOOL fStoreSize = TRUE);

BOOL RestoreWindowPosition(HWND hwnd,
	const char *szWindowName,
	const char *szSubKey,
	BOOL fHidden = FALSE);

// config writer
class StdCompilerConfigWrite : public StdCompiler
{
public:
	// Construct with root key
	StdCompilerConfigWrite(HKEY hRoot, const char *szPath);
	~StdCompilerConfigWrite();

	// Properties
	virtual bool hasNaming() { return true; }

	// Naming
	virtual bool Name(const char *szName);
	virtual void NameEnd(bool fBreak = false);
	virtual bool FollowName(const char *szName);
	virtual bool Default(const char *szName);

	// Seperators
	virtual bool Seperator(Sep eSep);

	// Data writers
	virtual void DWord(int32_t &rInt);
	virtual void DWord(uint32_t &rInt);
	virtual void Word(int16_t &rShort);
	virtual void Word(uint16_t &rShort);
	virtual void Byte(int8_t &rByte);
	virtual void Byte(uint8_t &rByte);
	virtual void Boolean(bool &rBool);
	virtual void Character(char &rChar);
	virtual void String(char *szString, size_t iMaxLength, RawCompileType eType = RCT_Escaped);
	virtual void String(char **pszString, RawCompileType eType = RCT_Escaped);
	virtual void Raw(void *pData, size_t iSize, RawCompileType eType = RCT_Escaped);

	// Passes
	virtual void Begin();
	virtual void End();

private:
	// Key stack
	int iDepth;
	struct Key
	{
		StdStrBuf Name;
		HKEY Handle;
		Key *Parent;
	} *pKey;

	// Writing
	void CreateKey(HKEY hParent = 0);
	void WriteDWord(uint32_t iVal);
	void WriteString(const char *szStr);
};

// config reader
class StdCompilerConfigRead : public StdCompiler
{
public:
	// Construct with root key
	StdCompilerConfigRead(HKEY hRoot, const char *szPath);
	~StdCompilerConfigRead();

	// Properties
	virtual bool isCompiler() { return true; }
	virtual bool hasNaming() { return true; }

	// Naming
	virtual bool Name(const char *szName);
	virtual void NameEnd(bool fBreak = false);
	virtual bool FollowName(const char *szName);

	// Seperators
	virtual bool Seperator(Sep eSep);

	// Data writers
	virtual void DWord(int32_t &rInt);
	virtual void DWord(uint32_t &rInt);
	virtual void Word(int16_t &rShort);
	virtual void Word(uint16_t &rShort);
	virtual void Byte(int8_t &rByte);
	virtual void Byte(uint8_t &rByte);
	virtual void Boolean(bool &rBool);
	virtual void Character(char &rChar);
	virtual void String(char *szString, size_t iMaxLength, RawCompileType eType = RCT_Escaped);
	virtual void String(char **pszString, RawCompileType eType = RCT_Escaped);
	virtual void Raw(void *pData, size_t iSize, RawCompileType eType = RCT_Escaped);

	// Passes
	virtual void Begin();
	virtual void End();

private:
	// Key stack
	int iDepth;
	struct Key
	{
		StdStrBuf Name;
		HKEY Handle; // for keys only
		Key *Parent;
		bool Virtual;
		DWORD Type; // for values only
	} *pKey;

	// Reading
	uint32_t ReadDWord();
	StdStrBuf ReadString();
};

#endif
