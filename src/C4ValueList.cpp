/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2017-2020, The LegacyClonk Team and contributors
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

#include <C4Include.h>
#include <C4ValueList.h>
#include <algorithm>
#include <stdexcept>

#include <C4Aul.h>
#include <C4FindObject.h>

C4ValueList::C4ValueList()
	: iSize(0), pData(nullptr) {}

C4ValueList::C4ValueList(int32_t inSize)
	: iSize(0), pData(nullptr)
{
	SetSize(inSize);
}

C4ValueList::C4ValueList(const C4ValueList &ValueList2)
	: iSize(0), pData(nullptr)
{
	SetSize(ValueList2.GetSize());
	for (int32_t i = 0; i < iSize; i++)
		pData[i].Set(ValueList2.GetItem(i));
}

C4ValueList::~C4ValueList()
{
	delete[] pData; pData = nullptr;
	iSize = 0;
}

C4ValueList &C4ValueList::operator=(const C4ValueList &ValueList2)
{
	this->SetSize(ValueList2.GetSize());
	for (int32_t i = 0; i < iSize; i++)
		pData[i].Set(ValueList2.GetItem(i));
	return *this;
}

class C4SortObjectSTL
{
private:
	C4SortObject &rSorter;

public:
	C4SortObjectSTL(C4SortObject &rSorter) : rSorter(rSorter) {}
	bool operator()(const C4Value &v1, const C4Value &v2) { return rSorter.Compare(v1._getObj(), v2._getObj()) > 0; }
};

class C4SortObjectSTLCache
{
private:
	C4SortObject &rSorter;
	C4Value *pVals;

public:
	C4SortObjectSTLCache(C4SortObject &rSorter, C4Value *pVals) : rSorter(rSorter), pVals(pVals) {}
	bool operator()(int32_t n1, int32_t n2) { return rSorter.CompareCache(n1, n2, pVals[n1]._getObj(), pVals[n2]._getObj()) > 0; }
};

void C4ValueList::Sort(class C4SortObject &rSort)
{
	if (rSort.PrepareCache(this))
	{
		// Initialize position array
		intptr_t i, *pPos = new intptr_t[iSize];
		for (i = 0; i < iSize; i++) pPos[i] = i;
		// Sort
		std::stable_sort(pPos, pPos + iSize, C4SortObjectSTLCache(rSort, pData));
		// Save actual object pointers in array (hacky).
		for (i = 0; i < iSize; i++)
			pPos[i] = reinterpret_cast<intptr_t>(pData[pPos[i]]._getObj());
		// Set the values
		for (i = 0; i < iSize; i++)
			pData[i].SetObject(reinterpret_cast<C4Object *>(pPos[i]));
		delete[] pPos;
	}
	else
		// Be sure to use stable sort, as otherweise the algorithm isn't garantueed
		// to produce identical results on all platforms!
		std::stable_sort(pData, pData + iSize, C4SortObjectSTL(rSort));
}

C4Value &C4ValueList::GetItem(int32_t iElem)
{
	if (iElem < 0) iElem = 0;
	if (iElem >= iSize && iElem < MaxSize) this->SetSize(iElem + 1);
	// out-of-memory? This might not be catched, but it's better than a segfault
	if (iElem >= iSize)
#ifdef C4ENGINE
		throw C4AulExecError(nullptr, "out of memory");
#else
		return pData[0]; // must return something here...
#endif
	// return
	return pData[iElem];
}

void C4ValueList::SetSize(int32_t inSize)
{
	// array made smaller? Well, just ignore the additional allocated mem then
	if (inSize <= iSize)
	{
		// free values in undefined area
		for (int i = inSize; i < iSize; i++) pData[i].Set0();
		iSize = inSize;
		return;
	}

	// bounds check
	if (inSize > MaxSize) return;

	// create new array (initialises)
	C4Value *pnData = new C4Value[inSize];
	if (!pnData) return;

	// move existing values
	int32_t i;
	for (i = 0; i < iSize; i++)
		pData[i].Move(&pnData[i]);

	// replace
	delete[] pData;
	pData = pnData;
	iSize = inSize;
}

bool C4ValueList::operator==(const C4ValueList &IntList2) const
{
	for (int32_t i = 0; i < (std::max)(iSize, IntList2.GetSize()); i++)
		if (GetItem(i) != IntList2.GetItem(i))
			return false;

	return true;
}

void C4ValueList::Reset()
{
	delete[] pData; pData = nullptr;
	iSize = 0;
}

void C4ValueList::DenumeratePointers()
{
	for (int32_t i = 0; i < iSize; i++)
		pData[i].DenumeratePointer();
}

void C4ValueList::CompileFunc(class StdCompiler *pComp)
{
	int32_t inSize = iSize;
	// Size. Reset if not found.
	try
	{
		pComp->Value(inSize);
	}
	catch (StdCompiler::NotFoundException *pExc)
	{
		Reset(); delete pExc; return;
	}
	// Separator
	if (!pComp->Separator(StdCompiler::SEP_SEP2))
	{
		assert(pComp->isCompiler());
		// No ';'-separator? So it's a really old value list format, or empty
		pComp->Separator(StdCompiler::SEP_SEP);
		this->SetSize(C4MaxVariable);
		// First variable was misinterpreted as size
		pData[0].SetInt(inSize);
		// Read remaining data
		pComp->Value(mkArrayAdapt(pData + 1, C4MaxVariable - 1, C4Value()));
	}
	else
	{
		if (pComp->isCompiler())
		{
			// Allocate
			this->SetSize(inSize);
			// Values
			pComp->Value(mkArrayAdapt(pData, iSize, C4Value()));
		}
		else
		{
			pComp->Value(mkArrayAdapt(pData, iSize));
		}
	}
}

C4ValueArray::C4ValueArray()
	: C4ValueList() {}

C4ValueArray::C4ValueArray(int32_t inSize)
	: C4ValueList(inSize) {}

C4ValueArray::C4ValueArray(const C4ValueArray &Array2)
	: C4ValueList(Array2) {}

C4ValueArray::~C4ValueArray() {}

C4ValueArray *C4ValueArray::SetLength(int32_t size)
{
	if (GetRefCount() > 1)
	{
		C4ValueArray *pNew = static_cast<C4ValueArray *>((new C4ValueArray(size))->IncRef());
		for (int32_t i = 0; i < (std::min)(size, iSize); i++)
			pNew->pData[i].Set(pData[i]);
		DecRef();
		return pNew;
	}
	else
	{
		SetSize(size);
		return this;
	}
}

bool C4ValueArray::hasIndex(const C4Value &index) const
{
	C4Value copyIndex = index;
	if (!copyIndex.ConvertTo(C4V_Int)) throw new std::runtime_error(std::string{"array access: can not convert \""} + GetC4VName(index.GetType()) + "\" to int");
	return copyIndex._getInt() < iSize;
}

C4Value &C4ValueArray::operator[](const C4Value &index)
{
	C4Value copyIndex = index;
	if (!copyIndex.ConvertTo(C4V_Int)) throw new std::runtime_error(std::string{"array access: can not convert \""} + GetC4VName(index.GetType()) + "\" to int");
	return (*this)[copyIndex._getInt()];
}
