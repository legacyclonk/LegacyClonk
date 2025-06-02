#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "C4Id.h"
#include "StdHelpers.h"

// C4Value type
enum C4V_Type
{
	C4V_Any          = 0, // unknown / no type
	C4V_Int          = 1, // Integer
	C4V_Bool         = 2, // Boolean
	C4V_C4ID         = 3, // C4ID
	C4V_C4Object     = 4, // Pointer on Object

	C4V_String       = 5, // String

	C4V_Array        = 6, // pointer on array of values
	C4V_Map          = 7, // pointer on map of values

	C4V_pC4Value     = 8, // reference on a value (variable)

	C4V_C4ObjectEnum = 9, // enumerated object
};

using C4ValueInt = std::int32_t;

struct C4ValueBaseHasher
{
	static std::size_t NilHash()
	{
		return hashIt(C4V_Any);
	}

	static std::size_t ObjectHash(C4ValueInt objectNumber)
	{
		return hashIt(C4V_C4Object, objectNumber);
	}

	std::size_t operator()(bool value) const
	{
		return hashIt(C4V_Bool, value);
	}

	std::size_t operator()(C4ValueInt value) const
	{
		return hashIt(C4V_Int, value);
	}

	std::size_t operator()(C4ID value) const
	{
		return hashIt(C4V_C4ID, static_cast<C4ValueInt>(value));
	}

	std::size_t operator()(std::string_view value) const
	{
		return hashIt(C4V_String, value);
	}

private:
	template <typename T>
	static std::size_t hashIt(const T &value)
	{
		return std::hash<T>{}(value);
	}

	template <typename T1, typename T2>
	static std::size_t hashIt(const T1 &value1, const T2 &value2)
	{
		return hashCombine(hashIt(value1), hashIt(value2));
	}
};
