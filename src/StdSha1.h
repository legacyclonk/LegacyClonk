/*
 * LegacyClonk
 *
 * Copyright (c) 2017-2019, The LegacyClonk Team and contributors
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

#include <cstddef>
#include <memory>

// SHA-1 calculation
class StdSha1
{
public:
	static const size_t DigestLength = 20;

	StdSha1();
	~StdSha1();
	// Adds the specified buffer to the data to be hashed.
	void Update(const void *buffer, std::size_t len);
	/* Returns the calculated hash.
	 * After calling, do not call Update without calling Reset first. */
	void GetHash(void *result);
	// Discards the current hash so another hash can be calculated.
	void Reset();

private:
	struct Impl;
	const std::unique_ptr<Impl> impl;
};
