/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2020, The LegacyClonk Team and contributors
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

/* Some useful wrappers to globals */

#pragma once

#include <C4Id.h>
#include <C4Log.h>

// Graphics Resource

#define GfxR (&(Game.GraphicsResource))

// Ticks

#define Tick2 Game.iTick2
#define Tick3 Game.iTick3
#define Tick5 Game.iTick5
#define Tick10 Game.iTick10
#define Tick35 Game.iTick35
#define Tick255 Game.iTick255
#define Tick1000 Game.iTick1000

// Players

int32_t ValidPlr(int32_t plr);
int32_t Hostile(int32_t plr1, int32_t plr2);


// Density

inline bool DensitySolid(int32_t dens)
{
	return (dens >= C4M_Solid);
}

inline bool DensitySemiSolid(int32_t dens)
{
	return (dens >= C4M_SemiSolid);
}

inline bool DensityLiquid(int32_t dens)
{
	return ((dens >= C4M_Liquid) && (dens < C4M_Solid));
}

// Materials
// StdCompiler

void StdCompilerWarnCallback(void *pData, const char *szPosition, const char *szError);

template <class CompT, class StructT>
bool CompileFromBuf_LogWarn(StructT &&TargetStruct, const typename CompT::InT &SrcBuf, const char *szName)
{
	try
	{
		CompT Compiler;
		Compiler.setInput(SrcBuf);
		Compiler.setWarnCallback(StdCompilerWarnCallback, reinterpret_cast<void *>(const_cast<char *>(szName)));
		Compiler.Compile(TargetStruct);
		return true;
	}
	catch (const StdCompiler::Exception &e)
	{
		if (!e.Pos.getLength())
			LogF("ERROR: %s (in %s)", e.what(), szName);
		else
			LogF("ERROR: %s (in %s, %s)", e.what(), e.Pos.getData(), szName);
		return false;
	}
}

template <class CompT, class StructT>
bool DecompileToBuf_Log(StructT &&TargetStruct, typename CompT::OutT *pOut, const char *szName)
{
	if (!pOut) return false;
	try
	{
		pOut->Take(DecompileToBuf<CompT>(TargetStruct));
		return true;
	}
	catch (const StdCompiler::Exception &e)
	{
		LogF("ERROR: %s (in %s)", e.what(), szName);
		return false;
	}
}
