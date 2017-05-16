/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Extended CString */

typedef LPCSTR PCTSTR;

class CStringEx : public CString
{
public:
	CStringEx() : CString(){};
	CStringEx(const CString& stringSrc) : CString(stringSrc) {}
	CStringEx(const CStringEx& stringSrc) : CString(stringSrc) {}
	CStringEx(TCHAR ch, int nRepeat = 1) : CString(ch, nRepeat) {}
	CStringEx(PCTSTR lpch, int nLength) : CString(lpch, nLength) {}
	CStringEx(const unsigned char* psz) : CString(psz) {}
	CStringEx(PCWSTR psz) : CString(psz) {}
	CStringEx(PCSTR psz) : CString(psz) {}
	CStringEx& Insert(int pos, PCTSTR s);
	CStringEx& Insert(int pos, TCHAR c);
	CStringEx& Delete(int pos, int len = 1);
	CStringEx& Replace(int pos, int len, PCTSTR s);
	int Find(TCHAR ch, int startpos = 0) const;
	int Find(PCTSTR pszSub, int startpos = 0) const;
	int FindNoCase(TCHAR ch, int startpos = 0) const;
	int FindNoCase(PCTSTR pszSub, int startpos = 0) const;
	int FindReplace(PCTSTR pszSub, PCTSTR pszReplaceWith, bool bGlobal = true);
	int FindReplaceNoCase(PCTSTR pszSub, PCTSTR pszReplaceWith, bool bGlobal = true);
	int ReverseFind(TCHAR ch) const { return CString::ReverseFind(ch); }
	int ReverseFind(PCTSTR pszSub, int startpos = -1) const;
	int ReverseFindNoCase(TCHAR ch, int startpos = -1 ) const;
	int ReverseFindNoCase(PCTSTR pszSub, int startpos = -1) const;
	CStringEx GetField(PCTSTR delim, int fieldnum) const;
	CStringEx GetField(TCHAR delim, int fieldnum) const;
	int GetFieldCount(PCTSTR delim) const;
	int GetFieldCount(TCHAR delim) const;
	CStringEx GetDelimitedField(PCTSTR delimStart, PCTSTR delimEnd, int fieldnum = 0) const;
protected:
	LPTSTR GetPCHData() const;
};
