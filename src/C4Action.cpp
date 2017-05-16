/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* The C4Action class is merely a simple data structure */

#include <C4Include.h>
#include <C4Object.h>

C4Action::C4Action()
  {
	Default();
  }

C4Action::~C4Action()
  {

  }

void C4Action::Default()
	{
  Act=ActIdle;
	ZeroMem(Name,C4D_MaxIDLen+1);
  Dir=DIR_None;
	DrawDir=Dir;
  ComDir=COMD_None;
  Time=0;
  Data=0;
  Target=Target2=NULL; 
  Phase=PhaseDelay=0;
	Facet.Default();
  FacetX=FacetY=0;
  t_attach=CNAT_None;
	}

void C4Action::CompileFunc(StdCompiler *pComp)
	{
	// Note: Compiled directly into "Object"-categories, so beware of name clashes (see C4Object::CompileFunc)
	pComp->Value(mkNamingAdapt( toC4CStr(Name),						"Action",							""								));
	pComp->Value(mkNamingAdapt( Dir,											"Dir",								DIR_None					));
	pComp->Value(mkNamingAdapt( ComDir,										"ComDir",							COMD_None					));
	pComp->Value(mkNamingAdapt( Time,											"ActionTime",					0									));
	pComp->Value(mkNamingAdapt( Data,											"ActionData",					0									));
	pComp->Value(mkNamingAdapt( Phase,										"Phase",							0									));
	pComp->Value(mkNamingAdapt( PhaseDelay,								"PhaseDelay",					0									));
	}

