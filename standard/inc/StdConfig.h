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

/* Auto-registering data structures */

#ifndef STDCONFIG_H
#define STDCONFIG_H

#include <StdCompiler.h>

const int CFG_MaxString	= 1024;

const int CFG_String		= 1,
					CFG_Integer		= 2,

					CFG_Company		= 10,
					CFG_Product		= 11,
					CFG_Section		= 12,
					CFG_End				= 0;

class CStdConfigValue 
	{ 
	public:
		int Type;
		const char *Name;
		int Offset;
		long Default; // Pointers are cast into this field, so be prepared
                  // to hold 64 bit on x86_64
  };

class CStdConfig  
	{
	public:
		CStdConfig();
		~CStdConfig();
	protected:
		void LoadDefault(CStdConfigValue *pCfgMap, void *vpData, const char *szOnlySection=NULL);
		BOOL Save(CStdConfigValue *pCfgMap, void *vpData);
		BOOL Load(CStdConfigValue *pCfgMap, void *vpData);
	};

#endif // STDCONFIG_H
