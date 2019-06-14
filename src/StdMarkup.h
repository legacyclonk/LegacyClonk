/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
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

// markup tags for fonts

#pragma once

#include <Standard.h>
#include <StdBuf.h>

class CBltTransform;

// one markup tag
class CMarkupTag
{
public:
	CMarkupTag *pPrev, *pNext;

	CMarkupTag() : pPrev(0), pNext(0) {};
	virtual ~CMarkupTag() {};

	virtual void Apply(CBltTransform &rBltTrf, bool fDoClr, uint32_t &dwClr) = 0; // assign markup
	virtual const char *TagName() = 0; // get character string for this tag
	virtual StdStrBuf ToMarkup() { return FormatString("<%s>", TagName()); };
};

// markup tag for italic text
class CMarkupTagItalic : public CMarkupTag
{
public:
	CMarkupTagItalic() : CMarkupTag() {}

	virtual void Apply(CBltTransform &rBltTrf, bool fDoClr, uint32_t &dwClr); // assign markup
	virtual const char *TagName() { return "i"; }
};

// markup tag for colored text
class CMarkupTagColor : public CMarkupTag
{
private:
	uint32_t dwClr; // color

public:
	CMarkupTagColor(uint32_t dwClr) : CMarkupTag(), dwClr(dwClr) {}

	virtual void Apply(CBltTransform &rBltTrf, bool fDoClr, uint32_t &dwClr); // assign markup
	virtual const char *TagName() { return "c"; }
	virtual StdStrBuf ToMarkup() { return FormatString("<%s %x>", TagName(), dwClr); };
};

// markup rendering functionality for text
class CMarkup
{
private:
	CMarkupTag *pTags, *pLast; // tag list; single linked
	bool fDoClr; // set if color changes should be made (not in text shadow!)

	void Push(CMarkupTag *pTag)
	{
		if ((pTag->pPrev = pLast)) pLast->pNext = pTag; else pTags = pTag; pLast = pTag;
	}

	CMarkupTag *Pop()
	{
		CMarkupTag *pL = pLast; if (!pL) return 0; if ((pLast = pL->pPrev)) pLast->pNext = nullptr; else pTags = nullptr; return pL;
	}

public:
	CMarkup(bool fDoClr) { pTags = pLast = nullptr; this->fDoClr = fDoClr; };

	~CMarkup()
	{
		CMarkupTag *pTag = pTags, *pNext; while (pTag) { pNext = pTag->pNext; delete pTag; pTag = pNext; }
	}

	bool Read(const char **ppText, bool fSkip = false); // get markup from text
	bool SkipTags(const char **ppText); // extract markup from text; return whether end is reached

	void Apply(CBltTransform &rBltTrf, uint32_t &dwClr) // assign markup to vertices
	{
		for (CMarkupTag *pTag = pTags; pTag; pTag = pTag->pNext) pTag->Apply(rBltTrf, fDoClr, dwClr);
	}

	StdStrBuf ToMarkup();

	bool Clean() { return !pTags; } // empty?

	static bool StripMarkup(char *szText); // strip any markup codes from given text buffer
	static bool StripMarkup(class StdStrBuf *sText); // strip any markup codes from given text buffer
};
