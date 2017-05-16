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

//================= C4FolderHead ====================

void C4FolderHead::Default()
  {
	Index = 0;
	Sort[0] = 0;
  }

void C4FolderHead::CompileFunc(StdCompiler *pComp)
  {
  pComp->Value(mkNamingAdapt(Index,                     "Index",                0));
  pComp->Value(mkNamingAdapt(mkStringAdaptMA(Sort),     "Sort",                 ""));
  }

//=================== C4Folder ======================

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
