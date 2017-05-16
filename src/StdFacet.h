/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* A very primitive piece of surface */

#pragma once

#include <StdSurface2.h>

class CFacet
{
public:
	CFacet() : Surface(nullptr), X(0), Y(0), Wdt(0), Hgt(0) {}
	~CFacet() {}

public:
	SURFACE Surface;
	int X, Y, Wdt, Hgt;

public:
	void Default() { Surface = nullptr; X = Y = Wdt = Hgt = 0; }
	void Clear() { Surface = nullptr; X = Y = Wdt = Hgt = 0; }

	void Set(SURFACE nsfc, int nx, int ny, int nwdt, int nhgt)
	{
		Surface = nsfc; X = nx; Y = ny; Wdt = nwdt; Hgt = nhgt;
	}
};
