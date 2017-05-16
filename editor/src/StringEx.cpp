/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Extended CString */

#include "C4Explorer.h"

//---------------------------------------------------------------------------

// Insert  - Inserts a sub string into the string
// Returns - Reference to the same string object
// pos     - Position to insert at. Extends the string with spaces if needed
// s       - Sub string to insert

CStringEx& CStringEx::Insert(int pos, PCTSTR s)
{
	int len = lstrlen(s);
	if (len == 0)	return *this;

	int oldlen = GetLength();
	int newlen = oldlen + len;
	PTSTR str;
	if (pos >= oldlen) 
	{			
		// insert after end of string
		newlen += pos - oldlen ;
		str = GetBuffer(newlen);
		_tcsnset(str+oldlen, _T(' '), pos-oldlen);
		_tcsncpy(str+pos, s, len);
	} 
	else 
	{	
		// normal insert
		str = GetBuffer(newlen);
		memmove(str+pos+len, str+pos, sizeof(_T(' ')) *(oldlen-pos));
		_tcsncpy(str+pos, s, len);
	}
	ReleaseBuffer(newlen);

	return *this;
}

//---------------------------------------------------------------------------

// Insert  - Inserts a character into the string
// Returns - Reference to the same string object
// pos     - Position to insert at. Extends the string with spaces if needed
// c       - Char to insert

CStringEx& CStringEx::Insert(int pos, TCHAR c)
{
	TCHAR buf[2];
	buf[0] = c;
	buf[1] = _T('\0');
	return Insert(pos, buf);
}

//---------------------------------------------------------------------------

// Delete  - Deletes a segment of the string and resizes it
// Returns - Reference to the same string object
// pos     - Position of the string segment to remove
// len     - Number of characters to remove

CStringEx& CStringEx::Delete(int pos, int len)
{
	int strLen = GetLength();

	if (pos >= strLen) return *this;
	if (len < 0 || len > strLen - pos) len = strLen - pos;

	PTSTR str = GetBuffer(strLen);
	memmove(str + pos, str + pos + len, sizeof(_T(' ')) * (strLen - pos));
	ReleaseBuffer(strLen - len);

	return *this;
}

//---------------------------------------------------------------------------

// Replace - Replaces a substring with another
// Returns - Reference to the same string object
// pos     - Position of the substring
// len     - Length of substring to be replaced
// s       - New substring

CStringEx& CStringEx::Replace(int pos, int len, PCTSTR s)
{
	Delete(pos, len);
	Insert(pos, s);

	return *this;
}

//---------------------------------------------------------------------------

// Find     - Finds the position of a character in a string
// Returns  - A zero-based index
// ch       - Character to look for
// startpos	- Position to start looking from

int CStringEx::Find(TCHAR ch, int startpos) const
{
	// find first single character
	PTSTR psz = _tcschr(GetPCHData() + startpos, (_TUCHAR)ch);

	// return -1 if not found and index otherwise
	return (psz == NULL) ? -1 : (int)(psz - GetPCHData());
}

//---------------------------------------------------------------------------

// Find     - Finds the position of a substring in a string
// Returns  - A zero-based index
// pszSub  - Substring to look for
// startpos	- Position to start looking from

int CStringEx::Find(PCTSTR pszSub, int startpos) const
{
	ASSERT(AfxIsValidString(pszSub, false));

	// find first matching substring
	PTSTR psz = _tcsstr(GetPCHData()+startpos, pszSub);

	// return -1 for not found, distance from beginning otherwise
	return (psz == NULL) ? -1 : (int)(psz - GetPCHData());
}

//---------------------------------------------------------------------------

// FindNoCase	- Case insensitive find
// Returns    - A zero-based index
// ch         - Char to search for
// startpos 	- Position to start looking from

int CStringEx::FindNoCase(TCHAR ch, int startpos) const
{
	int locase = Find((TCHAR)tolower(ch), startpos);
	int upcase = Find((TCHAR)toupper(ch), startpos);

	return locase < upcase ? locase : upcase;
}

//---------------------------------------------------------------------------

// FindNoCase - Case insensitive find
// Returns    - A zero-based index
// pszSub    - Substring to search for
// startpos   - Position to start looking from

int CStringEx::FindNoCase(PCTSTR pszSub, int startpos) const
{
	CStringEx sLowerThis = *this;
	sLowerThis.MakeLower();

	CStringEx sLowerSub = pszSub;
	sLowerSub.MakeLower();

	return sLowerThis.Find(sLowerSub, startpos);
}

//---------------------------------------------------------------------------

// FindReplace     - Find a substring and replace with another
// Returns         - Number of instances replaced
// pszSub          - Substring to look for
// pszReplaceWith  - Substring to replace with
// bGlobal		     - Flag to indicate whether all occurances should be replaced

int CStringEx::FindReplace(PCTSTR pszSub, PCTSTR pszReplaceWith, bool bGlobal /*= true*/)
{
	int iReplaced = 0;

	// find first matching substring
	PTSTR psz;
	
	int pos = 0;
	int lenSub = lstrlen(pszSub);
	int lenReplaceWith = lstrlen(pszReplaceWith);
	while ((psz = _tcsstr(GetPCHData() + pos, pszSub)) != NULL)
	{
		pos = (int)(psz - GetPCHData());
		Replace(pos, lenSub, pszReplaceWith);
		pos += lenReplaceWith;
		iReplaced++;
		if (!bGlobal) break;
	}

	return iReplaced;
}

//---------------------------------------------------------------------------

// FindReplaceNoCase - Find a substring and replace with another
//                     Does case insensitive search
// Returns           - Number of instances replaced
// pszSub            - Substring to look for
// pszReplaceWith    - Substring to replace with
// bGlobal           - Flag to indicate whether all occurances should be replaced

int CStringEx::FindReplaceNoCase(PCTSTR pszSub, PCTSTR pszReplaceWith, bool bGlobal /*= true*/)
{
	CStringEx sLowerThis = *this;
	sLowerThis.MakeLower();

	CStringEx sLowerSub = pszSub;
	sLowerSub.MakeLower();

	int iReplaced = 0;

	// find first matching substring
	PTSTR psz;
	
	int pos = 0, offset = 0;
	int lenSub = lstrlen(pszSub);
	int lenReplaceWith = lstrlen(pszReplaceWith);
	while ((psz = _tcsstr(sLowerThis.GetPCHData() + pos, sLowerSub.GetPCHData())) != NULL)
	{
		pos = (int)(psz - sLowerThis.GetPCHData());
		Replace(pos+offset, lenSub, pszReplaceWith);
		offset += lenReplaceWith - lenSub;
		pos += lenSub;
		iReplaced++;
		if (!bGlobal) break;
	}

	return iReplaced;
}

//---------------------------------------------------------------------------

// ReverseFind - Searches for the last match of a substring
// Returns     - A zero-based index
// pszSub     - Substring to search for
// startpos    - Position to start looking from, in reverse dir

int CStringEx::ReverseFind(PCTSTR pszSub, int startpos) const
{
	int lenSub = lstrlen(pszSub);
	int len = lstrlen(GetPCHData());

	if (0 < lenSub && 0 < len)
	{
		if (startpos == -1 || startpos >= len) startpos = len - 1;
		for (PTSTR pszReverse = GetPCHData() + startpos; pszReverse != GetPCHData(); --pszReverse)
			if (_tcsncmp(pszSub, pszReverse, lenSub) == 0) return (pszReverse - GetPCHData());
	}
	return -1;
}

//---------------------------------------------------------------------------

// ReverseFindNoCase - Searches for the last match of a substring
//                     Search is case insensitive
// Returns           - A zero-based index
// pszSub            - Character to search for
// startpos          - Position to start looking from, in reverse dir

int CStringEx::ReverseFindNoCase(TCHAR ch, int startpos) const
{
	TCHAR a[2];
	a[1] = ch;
	a[2] = 0;
	return ReverseFindNoCase(a, startpos);
}

//---------------------------------------------------------------------------

// ReverseFindNoCase - Searches for the last match of a substring
//                     Search is case insensitive
// Returns           - A zero-based index
// pszSub           - Substring to search for
// startpos          - Position to start looking from, in reverse dir

int CStringEx::ReverseFindNoCase(PCTSTR pszSub, int startpos) const
{
	//PTSTR pszEnd = GetPCHData() + lstrlen

	int lenSub = lstrlen(pszSub);
	int len = lstrlen(GetPCHData());
	
	if (0 < lenSub && 0 < len)
	{
		if (startpos == -1 || startpos >= len) startpos = len - 1;
		for (PTSTR pszReverse = GetPCHData() + startpos; pszReverse >= GetPCHData(); --pszReverse)
			if (_tcsnicmp(pszSub, pszReverse, lenSub) == 0) return (pszReverse - GetPCHData());
	}
	return -1;
}

//---------------------------------------------------------------------------

// GetField - Gets a delimited field
// Returns  - A CStringEx object that contains a copy of the specified field
// delim    - The delimiter string
// fieldnum - The field index - zero is the first
// NOTE     - Returns the whole string for field zero if delim not found
//            Returns empty string if # of delim found is less than fieldnum

CStringEx CStringEx::GetField(PCTSTR delim, int fieldnum) const
{
	PTSTR psz, pszRemainder = GetPCHData(), pszret;
	int retlen, lenDelim = lstrlen(delim);

	while (fieldnum-- >= 0)
	{
		pszret = pszRemainder;
		psz = _tcsstr(pszRemainder, delim);
		if (psz)
		{
			// We did find the delim
			retlen = psz - pszRemainder;
			pszRemainder = psz + lenDelim;
		}
		else
		{
			retlen = lstrlen(pszRemainder);
			pszRemainder += retlen;	// change to empty string
		}
	}
	return Mid(pszret - GetPCHData(), retlen);
}

//---------------------------------------------------------------------------

// GetField - Gets a delimited field
// Returns  - A CStringEx object that contains a copy of the specified field
// delim    - The delimiter char
// fieldnum - The field index - zero is the first
// NOTE     - Returns the whole string for field zero if delim not found
//            Returns empty string if # of delim found is less than fieldnum

CStringEx CStringEx::GetField(TCHAR delim, int fieldnum) const
{
	PTSTR psz, pszRemainder = GetPCHData(), pszret;
	int retlen;

	while (fieldnum-- >= 0)
	{
		pszret = pszRemainder;
		psz = _tcschr(pszRemainder, (_TUCHAR)delim);
		if (psz)
		{
			// We did find the delim
			retlen = psz - pszRemainder;
			pszRemainder = psz + 1;
		}
		else
		{
			retlen = lstrlen(pszRemainder);
			pszRemainder += retlen;	// change to empty string
		}
	}
	return Mid(pszret - GetPCHData(), retlen);
}

//---------------------------------------------------------------------------

// GetFieldCount - Get number of fields in a string
// Returns       - The number of fields. Is always greater than zero
// delim         - The delimiter character

int CStringEx::GetFieldCount(TCHAR delim) const
{
	TCHAR a[2];
	a[0] = delim;
	a[1] = 0;
	return GetFieldCount(a);
}

//---------------------------------------------------------------------------

// GetFieldCount - Get number of fields in a string
// Returns       - The number of fields. Is always greater than zero
// delim         - The delimiter string

int CStringEx::GetFieldCount(PCTSTR delim) const
{
	PTSTR psz, pszRemainder = GetPCHData();
	int lenDelim = lstrlen(delim);

	int iCount = 1;
	while ((psz = _tcsstr(pszRemainder, delim)) != NULL)
	{
		pszRemainder = psz + lenDelim;
		iCount++;
	}

	return iCount;
}

//---------------------------------------------------------------------------

// GetDelimitedField - Finds a field delimited on both ends
// Returns           - A CStringEx object that contains a copy of the specified field
// delimStart        - Delimiter at the start of the field
// delimEnd          - Delimiter at the end of the field

CStringEx CStringEx::GetDelimitedField(PCTSTR delimStart, PCTSTR delimEnd, int fieldnum) const
{
	PTSTR psz, pszEnd, pszRet, pszRemainder = GetPCHData() ;
	int lenDelimStart = lstrlen(delimStart), lenDelimEnd = lstrlen(delimEnd);

	while (fieldnum-- >= 0)
	{
		psz = _tcsstr(pszRemainder, delimStart);
		if (psz)
		{
			// We did find the Start delim
			pszRet = pszRemainder = psz + lenDelimStart;
			pszEnd = _tcsstr(pszRemainder, delimEnd);
			if (pszEnd == NULL) return "";
			pszRemainder = psz + lenDelimEnd;
		}
		else return "";
	}
	return Mid(pszRet - GetPCHData(), pszEnd - pszRet);
}

//---------------------------------------------------------------------------

LPTSTR CStringEx::GetPCHData() const
{
#if _MSC_VER >= 1300
	return (LPTSTR) GetString();
#else
	return m_pchData;
#endif
}

