/*
 * LegacyClonk
 *
 * Copyright (c) 2019-2021, The LegacyClonk Team and contributors
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

#pragma once
#include "C4Value.h"

class C4ValueContainer
{
public:
	virtual ~C4ValueContainer() {}

	virtual void CompileFunc(StdCompiler *pComp) = 0;
	virtual void DenumeratePointers() = 0;

	virtual bool hasIndex(const C4Value &index) const = 0;
	virtual C4Value &operator[](const C4Value &index) = 0;

	virtual C4ValueContainer *IncRef() = 0;
	virtual void DecRef() = 0;

	virtual C4ValueContainer *IncElementRef() = 0;
	virtual void DecElementRef() = 0;
};
