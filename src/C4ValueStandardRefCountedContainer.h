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
#include "C4ValueContainer.h"

template <typename T>
class C4ValueStandardRefCountedContainer : public C4ValueContainer
{
	size_t referenceCount = 0;
	size_t elementReferenceCount = 0;

public:
	virtual C4ValueContainer *IncRef() override
	{
		if (referenceCount && elementReferenceCount)
		{
			auto newContainer = new T(*static_cast<T *>(this));
			++newContainer->referenceCount;
			return newContainer;
		}
		++referenceCount;
		return this;
	}

	virtual void DecRef() override
	{
		assert(referenceCount);
		if (!--referenceCount)
		{
			delete this;
		}
	}

	virtual C4ValueContainer *IncElementRef() override
	{
		if (referenceCount > 1)
		{
			auto newContainer = new T(*static_cast<T *>(this));
			++newContainer->referenceCount;
			newContainer->elementReferenceCount = 1;
			DecRef();
			return newContainer;
		}
		++elementReferenceCount;
		return this;
	}

	virtual void DecElementRef() override
	{
		assert(elementReferenceCount);
		--elementReferenceCount;
	}

protected:
	size_t GetRefCount()
	{
		return referenceCount;
	}
};
