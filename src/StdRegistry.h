/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2021, The LegacyClonk Team and contributors
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

/* Some wrappers for easier access to the Windows registry */

#pragma once

#include "C4Windows.h"
#include "StdCompiler.h"

bool GetRegistryDWord(HKEY hKey, const char *szSubKey,
	const char *szValueName, DWORD *lpdwValue);

bool GetRegistryString(const char *szSubKey, const char *szValueName, char *sValue, DWORD dwValSize);
bool SetRegistryString(const char *szSubKey, const char *szValueName, const char *szValue);

bool DeleteRegistryKey(HKEY hKey, const char *szSubKey);

bool SetRegClassesRoot(const char *szSubKey,
	const char *szValueName,
	const char *szStringValue);

bool SetRegShell(const char *szClassName,
	const char *szShellName,
	const char *szShellCaption,
	const char *szCommand,
	bool fMakeDefault = false);

bool RemoveRegShell(const char *szClassName,
	const char *szShellName);

bool SetRegFileClass(const char *szClassRoot,
	const char *szExtension,
	const char *szClassName,
	const char *szIconPath, int iIconNum,
	const char *szContentType);

bool StoreWindowPosition(HWND hwnd,
	const char *szWindowName,
	const char *szSubKey,
	bool fStoreSize = true);

bool RestoreWindowPosition(HWND hwnd,
	const char *szWindowName,
	const char *szSubKey);

// config writer
class StdCompilerConfigWrite : public StdCompiler
{
public:
	// Construct with root key
	StdCompilerConfigWrite(HKEY hRoot, const char *szPath);
	~StdCompilerConfigWrite();

	// Properties
	virtual bool hasNaming() override { return true; }

	// Naming
	virtual NameGuard Name(const char *szName) override;
	virtual void NameEnd(bool fBreak = false) override;
	virtual bool Default(const char *szName) override;

	// Separators
	virtual bool Separator(Sep eSep) override;

	// Data writers
	virtual void QWord(int64_t &rInt) override;
	virtual void QWord(uint64_t &rInt) override;
	virtual void DWord(int32_t &rInt) override;
	virtual void DWord(uint32_t &rInt) override;
	virtual void Word(int16_t &rShort) override;
	virtual void Word(uint16_t &rShort) override;
	virtual void Byte(int8_t &rByte) override;
	virtual void Byte(uint8_t &rByte) override;
	virtual void Boolean(bool &rBool) override;
	virtual void Character(char &rChar) override;
	virtual void String(char *szString, size_t iMaxLength, RawCompileType eType = RCT_Escaped) override;
	virtual void String(std::string &str, RawCompileType type = RCT_Escaped) override;
	virtual void Raw(void *pData, size_t iSize, RawCompileType eType = RCT_Escaped) override;

	// Passes
	virtual void Begin() override;
	virtual void End() override;

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
	template<typename T>
	void WriteInteger(T value);
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
	virtual bool isCompiler() override { return true; }
	virtual bool hasNaming() override { return true; }

	// Naming
	virtual NameGuard Name(const char *szName) override;
	virtual void NameEnd(bool fBreak = false) override;

	// Separators
	virtual bool Separator(Sep eSep) override;

	// Data writers
	virtual void QWord(int64_t &rInt) override;
	virtual void QWord(uint64_t &rInt) override;
	virtual void DWord(int32_t &rInt) override;
	virtual void DWord(uint32_t &rInt) override;
	virtual void Word(int16_t &rShort) override;
	virtual void Word(uint16_t &rShort) override;
	virtual void Byte(int8_t &rByte) override;
	virtual void Byte(uint8_t &rByte) override;
	virtual void Boolean(bool &rBool) override;
	virtual void Character(char &rChar) override;
	virtual void String(char *szString, size_t iMaxLength, RawCompileType eType = RCT_Escaped) override;
	virtual void String(std::string &str, RawCompileType type = RCT_Escaped) override;
	virtual void Raw(void *pData, size_t iSize, RawCompileType eType = RCT_Escaped) override;

	// Passes
	virtual void Begin() override;
	virtual void End() override;

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
	template<typename T>
	T ReadInteger(DWORD type, DWORD alternativeType);
	uint32_t ReadDWord();
	StdStrBuf ReadString();
};
