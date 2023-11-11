/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2017-2022, The LegacyClonk Team and contributors
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
#include <stdexcept>

#include <C4Aul.h>
#include <C4FindObject.h>

#include <format>

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

C4Value &C4ValueList::GetItem(int32_t iElem)
{
	if (iElem < 0) iElem = 0;
	if (iElem >= iSize && iElem < MaxSize) this->SetSize(iElem + 1);
	// out-of-memory? This might not be catched, but it's better than a segfault
	if (iElem >= iSize)
		throw C4AulExecError(nullptr, "out of memory");
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
	// FIXME: this should be one of C4ValueInt or (u)intptr_t (or C4ID), but which one?
	int32_t inSize = iSize;
	// Size. Reset if not found.
	try
	{
		pComp->Value(inSize);
	}
	catch (const StdCompiler::NotFoundException &)
	{
		Reset(); return;
	}
	// Separator
	if (!pComp->Separator(StdCompiler::SEP_SEP2))
	{
		assert(pComp->isCompiler());
		// No ';'-separator? So it's a really old value list format, or empty
		pComp->Separator(StdCompiler::SEP_SEP);
		this->SetSize(C4MaxVariable);
		// First variable was misinterpreted as size
		pData[0] = C4Value{C4V_Data{inSize}, C4V_Any};
		// Read remaining data
		pComp->Value(mkArrayAdaptS(pData + 1, C4MaxVariable - 1, C4Value()));
	}
	else
	{
		if (pComp->isCompiler())
		{
			// Allocate
			this->SetSize(inSize);
			// Values
			pComp->Value(mkArrayAdaptS(pData, iSize, C4Value()));
		}
		else
		{
			pComp->Value(mkArrayAdaptS(pData, iSize));
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
	if (!copyIndex.ConvertTo(C4V_Int)) throw std::runtime_error{std::format("array access: can not convert \"{}\" to int", GetC4VName(index.GetType()))};
	return copyIndex._getInt() < iSize;
}

C4Value &C4ValueArray::operator[](const C4Value &index)
{
	C4Value copyIndex = index;
	if (!copyIndex.ConvertTo(C4V_Int)) throw std::runtime_error{std::format("array access: can not convert \"{}\" to int", GetC4VName(index.GetType()))};
	return (*this)[copyIndex._getInt()];
}
