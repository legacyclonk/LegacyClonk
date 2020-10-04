/*
 * LegacyClonk
 *
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

#pragma once

class C4CurlSystem
{
public:
	C4CurlSystem();
	~C4CurlSystem();

	C4CurlSystem(const C4CurlSystem &) = delete;
	C4CurlSystem &operator=(const C4CurlSystem &) = delete;

	C4CurlSystem(C4CurlSystem &&) = delete;
	C4CurlSystem &operator=(C4CurlSystem &&) = delete;
};
