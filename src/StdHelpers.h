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

template<typename... T>
class StdOverloadedCallable : public T...
{
public:
	StdOverloadedCallable(T... bases) : T{bases}... {}
	using T::operator()...;
};

// based on boost container_hash's hashCombine
constexpr void HashCombine(std::size_t &hash, std::size_t nextHash)
{
	if constexpr (sizeof(std::size_t) == 4)
	{
#define rotateLeft32(x, r) (x << r) | (x >> (32 - r))
		constexpr std::size_t c1 = 0xcc9e2d51;
		constexpr std::size_t c2 = 0x1b873593;

		nextHash *= c1;
		nextHash = rotateLeft32(nextHash, 15);
		nextHash *= c2;

		hash ^= nextHash;
		hash = rotateLeft32(hash, 13);
		hash = hash * 5 + 0xe6546b64;
#undef rotateLeft32
	}
	else if constexpr (sizeof(std::size_t) == 8)
	{
		constexpr std::size_t m = 0xc6a4a7935bd1e995;
		constexpr int r = 47;

		nextHash *= m;
		nextHash ^= nextHash >> r;
		nextHash *= m;

		hash ^= nextHash;
		hash *= m;

		// Completely arbitrary number, to prevent 0's
		// from hashing to 0.
		hash += 0xe6546b64;
	}
	else
	{
		hash ^= nextHash + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	}
}
