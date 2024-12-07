/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2021, The LegacyClonk Team and contributors
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

/* Fixed point math extracted from ALLEGRO by Shawn Hargreaves */

/* The Clonk engine uses fixed point math for exact object positions.
   This is rather silly. Nowadays we should simply use floats. However,
   I never dared changing the whole thing. */
/* 01-17-2002: I think the whole, ugly fixed-thing has to be revived,
   because floating point calculations are not guaranteed to be network
   safe...however, it can be solved as a data type with operator
   overloading, automatic type conversions, etc now   - Sven2 */
/* After some time with synchronous float use, C4Fixed is used again to
   work around the problem that different compilers produce different
   floating point code, leading to desyncs between linux and windows
   engines. */

#pragma once

#include <cmath>
#include <compare>

#include "StdCompiler.h"
#include "StdAdaptors.h"

extern long SineTable[9001]; // external table of sine values

// fixpoint shift (check 64 bit emulation before changing!)
constexpr int32_t FIXED_SHIFT{16};
// fixpoint factor
constexpr int32_t FIXED_FPF{1 << FIXED_SHIFT};

class C4Fixed
{
	friend constexpr int fixtoi(const C4Fixed &x);
	friend constexpr int fixtoi(const C4Fixed &x, int32_t prec);
	friend constexpr C4Fixed itofix(int32_t x);
	friend constexpr C4Fixed itofix(int32_t x, int32_t prec);
	friend constexpr float fixtof(const C4Fixed &x);
	friend constexpr C4Fixed ftofix(float x);

	friend inline void CompileFunc(C4Fixed &rValue, StdCompiler *pComp);

public:
	int32_t val; // internal value

public:
	constexpr C4Fixed() {}
	constexpr C4Fixed(const C4Fixed &rCpy) : val(rCpy.val) {}
	constexpr C4Fixed &operator=(const C4Fixed &other)
	{
		val = other.val;
		return *this;
	}

	// Conversion must be done by the conversion routines itofix, fixtoi, ftofix and fixtof
	// in order to be backward compatible, so everything is private.

private:
	explicit constexpr C4Fixed(int32_t iVal)
		: val(iVal * FIXED_FPF) {}
	explicit constexpr C4Fixed(int32_t iVal, int32_t iPrec)
		: val(iPrec < FIXED_FPF
			? iVal * (FIXED_FPF / iPrec) + (iVal * (FIXED_FPF % iPrec)) / iPrec
			: int32_t(int64_t(iVal) * FIXED_FPF / iPrec)
		) {}
	explicit constexpr C4Fixed(float fVal)
		: val(static_cast<int32_t>(fVal * float(FIXED_FPF))) {}

	// round to int
	constexpr int32_t to_int() const
	{
		int32_t r = val;
		// be carefull not to overflow
		r += (val <= 0x7fffffff - FIXED_FPF / 2) * FIXED_FPF / 2;
		// ensure that -x.50 is rounded to -(x+1)
		r -= (r < 0);
		r >>= FIXED_SHIFT;
		// round 32767.5 to 32768 (not that anybody cares)
		r += (val > 0x7fffffff - FIXED_FPF / 2);
		return r;
	}

	constexpr int32_t to_int(int32_t prec) const
	{
		int64_t r = val;
		r *= prec;
		r += FIXED_FPF / 2;
		r -= (r < 0);
		r >>= FIXED_SHIFT;
		return int32_t(r);
	}

	// convert to floating point value
	constexpr float to_float() const
	{
		return float(val) / float(FIXED_FPF);
	}

public:
	// set integer (allowed for historic reasons)
	constexpr C4Fixed &operator=(int32_t x) { return *this = C4Fixed(x); }

	// test value
	constexpr operator bool() const { return !!val; }
	constexpr bool operator!() const { return !val; }

	// arithmetic operations
	constexpr C4Fixed &operator+=(const C4Fixed &fVal2)
	{
		val += fVal2.val;
		return *this;
	}

	constexpr C4Fixed &operator-=(const C4Fixed &fVal2)
	{
		val -= fVal2.val;
		return *this;
	}

	constexpr C4Fixed &operator*=(const C4Fixed &fVal2)
	{
		val = int32_t((int64_t(val) * fVal2.val) / FIXED_FPF);
		return *this;
	}

	constexpr C4Fixed &operator*=(int32_t iVal2)
	{
		val *= iVal2;
		return *this;
	}

	constexpr C4Fixed &operator/=(const C4Fixed &fVal2)
	{
		val = int32_t((int64_t(val) * FIXED_FPF) / fVal2.val);
		return *this;
	}

	constexpr C4Fixed &operator/=(int32_t iVal2)
	{
		val /= iVal2;
		return *this;
	}

	constexpr C4Fixed operator-() const
	{
		C4Fixed fr; fr.val = -val; return fr;
	}

	constexpr C4Fixed operator+() const
	{
		return *this;
	}

	constexpr bool operator== (const C4Fixed &fVal2) const { return val == fVal2.val; }
	constexpr std::strong_ordering operator<=>(const C4Fixed &value) const { return val <=> value.val; }

	// and wrappers
	constexpr C4Fixed &operator+=(int32_t iVal2) { return operator += (C4Fixed(iVal2)); }
	constexpr C4Fixed &operator-=(int32_t iVal2) { return operator -= (C4Fixed(iVal2)); }

	constexpr C4Fixed operator+(const C4Fixed &fVal2) const { return C4Fixed(*this) += fVal2; }
	constexpr C4Fixed operator-(const C4Fixed &fVal2) const { return C4Fixed(*this) -= fVal2; }
	constexpr C4Fixed operator*(const C4Fixed &fVal2) const { return C4Fixed(*this) *= fVal2; }
	constexpr C4Fixed operator/(const C4Fixed &fVal2) const { return C4Fixed(*this) /= fVal2; }

	constexpr C4Fixed operator+(int32_t iVal2) const { return C4Fixed(*this) += iVal2; }
	constexpr C4Fixed operator-(int32_t iVal2) const { return C4Fixed(*this) -= iVal2; }
	constexpr C4Fixed operator*(int32_t iVal2) const { return C4Fixed(*this) *= iVal2; }
	constexpr C4Fixed operator/(int32_t iVal2) const { return C4Fixed(*this) /= iVal2; }

	constexpr bool operator==(int32_t iVal2) const { return operator==(C4Fixed(iVal2)); }
	constexpr std::strong_ordering operator<=>(const std::int32_t value) const { return *this <=> C4Fixed{value}; }

	C4Fixed sin_deg() const
	{
		// adjust angle
		int32_t v = int32_t((int64_t(val) * 100) / FIXED_FPF); if (v < 0) v = 18000 - v; v %= 36000;
		// get sine
		C4Fixed fr;
		switch (v / 9000)
		{
		case 0: fr.val = +SineTable[v];         break;
		case 1: fr.val = +SineTable[18000 - v]; break;
		case 2: fr.val = -SineTable[v - 18000]; break;
		case 3: fr.val = -SineTable[36000 - v]; break;
		}
		return fr;
	}

	C4Fixed cos_deg() const
	{
		// adjust angle
		int32_t v = int32_t((int64_t(val) * 100) / FIXED_FPF); if (v < 0) v = -v; v %= 36000;
		// get cosine
		C4Fixed fr;
		switch (v / 9000)
		{
		case 0: fr.val = +SineTable[9000 - v];  break;
		case 1: fr.val = -SineTable[v - 9000];  break;
		case 2: fr.val = -SineTable[27000 - v]; break;
		case 3: fr.val = +SineTable[v - 27000]; break;
		}
		return fr;
	}
};

// conversion
constexpr float fixtof(const C4Fixed &x)             { return x.to_float(); }
constexpr C4Fixed ftofix(float x)                    { return C4Fixed(x); }
constexpr int fixtoi(const C4Fixed &x)               { return x.to_int(); }
constexpr int fixtoi(const C4Fixed &x, int32_t prec) { return x.to_int(prec); }
constexpr C4Fixed itofix(int32_t x)                  { return C4Fixed(x); }
constexpr C4Fixed itofix(int32_t x, int32_t prec)    { return C4Fixed(x, prec); }

// additional functions
inline C4Fixed Sin(const C4Fixed &fAngle) { return fAngle.sin_deg(); }
inline C4Fixed Cos(const C4Fixed &fAngle) { return fAngle.cos_deg(); }
constexpr C4Fixed FIXED100(int x)          { return itofix(x, 100); }
constexpr C4Fixed FIXED256(int x)          { C4Fixed r; r.val = x * FIXED_FPF / 256; return r; }
constexpr C4Fixed FIXED10(int x)           { return itofix(x, 10); }

// define 0
inline constexpr C4Fixed Fix0 = itofix(0);

// conversion...
// note: keep out! really dirty casts!
inline void FLOAT_TO_FIXED(C4Fixed *pVal)
{
	*pVal = ftofix(*reinterpret_cast<float *>(pVal));
}

#undef inline

// CompileFunc for C4Fixed
inline void CompileFunc(C4Fixed &rValue, StdCompiler *pComp)
{
	char cFormat = 'F';
	try
	{
		// Read/write type
		pComp->Character(cFormat);
	}
	catch (const StdCompiler::NotFoundException &)
	{
		// Expect old format if not found
		cFormat = 'F';
	}
	// Read value (as int32_t)
	pComp->Value(mkCastAdapt<int32_t>(rValue));
	// convert, if neccessary
	if (cFormat == 'f')
		FLOAT_TO_FIXED(&rValue);
}
