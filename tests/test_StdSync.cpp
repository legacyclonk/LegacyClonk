/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2023, The LegacyClonk Team and contributors
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

#include "StdSync.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Set event", "[cstdevent]")
{
	CStdEvent event{};
	event.Set();

	REQUIRE(event.WaitFor(0));
}

TEST_CASE("Initial state", "[cstdevent]")
{
	CStdEvent event{true};
	REQUIRE(event.WaitFor(0));
}

TEST_CASE("Will fail", "[cstdevent]")
{
	CStdEvent event{false};
	REQUIRE(event.WaitFor(0));
}
