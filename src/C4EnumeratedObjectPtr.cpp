/*
 * LegacyClonk
 *
 * Copyright (c) 2020, The LegacyClonk Team and contributors
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

#include "C4EnumeratedObjectPtr.h"

#include "C4Game.h"
#include "C4Object.h"
#include "StdAdaptors.h"
#include "StdCompiler.h"

#include <type_traits>

static_assert(std::is_same_v<C4EnumeratedObjectPtrTraits::Enumerated, decltype(C4Object::Number)>, "C4EnumeratedObjectPtr::Enumerated must match the type of C4Object::Number");

std::int32_t C4EnumeratedObjectPtrTraits::Enumerate(C4Object *const obj)
{
	return Game.ObjectNumber(obj);
}

C4Object *C4EnumeratedObjectPtrTraits::Denumerate(const std::int32_t number)
{
	// only for compatibility with old savegames where Game.Objects.Enumerated() has been used -.-
	if (Inside(number, C4EnumPointer1, C4EnumPointer2))
	{
		return Game.ObjectPointer(number - C4EnumPointer1);
	}
	else
	{
		return Game.ObjectPointer(number);
	}
}
