/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Core component of a folder */

#include <C4Include.h>
#include <C4Folder.h>

#ifndef BIG_C4INCLUDE
#include <C4Random.h>
#include <C4Group.h>
#include <C4Components.h>
#include <C4Game.h>
#ifdef C4ENGINE
#include <C4Wrappers.h>
#endif
#endif

#ifdef C4GROUP
#include "C4CompilerWrapper.h"
#endif

// C4FolderHead

void C4FolderHead::Default()
{
	Index = 0;
	Sort[0] = 0;
}

void C4FolderHead::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(Index,                 "Index", 0));
	pComp->Value(mkNamingAdapt(mkStringAdaptMA(Sort), "Sort",  ""));
}

// C4Folder

C4Folder::C4Folder()
{
	Default();
}

void C4Folder::Default()
{
	Head.Default();
}

void C4Folder::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(Head, "Head"));
}
