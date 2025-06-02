#include "C4ValueConstexpr.h"

#include <compare>
#include <type_traits>

#include "C4Id.h"
#include "C4StringTable.h"
#include "C4Value.h"
#include "StdHelpers.h"


std::strong_ordering C4ValueConstexpr::operator<=>(const C4Value &other) const
{
	using O = std::strong_ordering;
	const auto &otherRefVal = other.GetRefVal();
	const auto otherType = otherRefVal.GetType();
	const auto ownType = GetType();

	if (ownType != otherType)
	{
		return ownType <=> otherType;
	}

	return std::visit<O>(StdOverloadedCallable
	{
		[](Nil) { return O::equivalent; },
		[&otherRefVal](bool value) { return value <=> otherRefVal._getBool(); },
		[&otherRefVal](C4ValueInt value) { return value <=>otherRefVal._getInt(); },
		[&otherRefVal](C4ID value) { return value <=> otherRefVal._getC4ID(); },
		[&otherRefVal](std::string_view value)
		{
			const StdStrBuf &s{otherRefVal._getStr()->Data};
			const std::string_view otherString{s.getData(), s.getLength()};
			return value <=> otherString;
		}
	}, value);
}

std::size_t std::hash<C4ValueConstexpr>::operator()(const C4ValueConstexpr &value)
{
	return std::visit<std::size_t>(StdOverloadedCallable
	{
		[](C4ValueConstexpr::Nil) { return C4ValueBaseHasher::NilHash(); },
		[](auto value) { return C4ValueBaseHasher{}(value); }
	}, value.value);
}
