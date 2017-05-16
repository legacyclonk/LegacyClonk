/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
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

/* A very primitive piece of surface */

#ifndef STD_FACET
#define STD_FACET

#include <StdSurface2.h>

class CFacet  
	{
	public:
		CFacet() : Surface(NULL), X(0), Y(0), Wdt(0), Hgt(0) { }
		~CFacet() { }
  public: 
    SURFACE Surface; 
    int X,Y,Wdt,Hgt; 
  public:
	  void Draw(SURFACE sfcSurface, int iX, int iY, int iPhaseX=0, int iPhaseY=0);
		void Default() { Surface=NULL; X=Y=Wdt=Hgt=0; }
		void Clear() { Surface=NULL; X=Y=Wdt=Hgt=0; }
    void Set(SURFACE nsfc, int nx, int ny, int nwdt, int nhgt)
			{ Surface=nsfc; X=nx; Y=ny; Wdt=nwdt; Hgt=nhgt; }
	};
#endif // STD_FACET
