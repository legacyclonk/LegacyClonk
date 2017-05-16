/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

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
