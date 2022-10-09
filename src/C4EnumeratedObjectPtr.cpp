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

static_assert(std::is_same_v<C4EnumeratedObjectPtr::Enumerated, decltype(C4Object::Number)>, "C4EnumeratedObjectPtr::Enumerated must match the type of C4Object::Number");

void C4EnumeratedObjectPtr::Enumerate()
{
	number = Game.Objects.ObjectNumber(object);
}

void C4EnumeratedObjectPtr::Denumerate()
{
	// only for compatibility with old savegames where Game.Objects.Enumerated() has been used -.-
	if (Inside(number, C4EnumPointer1, C4EnumPointer2))
	{
		object = Game.Objects.ObjectPointer(number - C4EnumPointer1);
	}
	else
	{
		object = Game.Objects.ObjectPointer(number);
	}
}

void C4EnumeratedObjectPtr::CompileFunc(StdCompiler *compiler, bool intPack)
{
	if (intPack)
	{
		compiler->Value(mkIntPackAdapt(number));
	}
	else
	{
		compiler->Value(number);
	}
}
