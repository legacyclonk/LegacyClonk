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

/* Load multi-language strings from the resource string table */

char *LoadResStr(WORD id);
#ifdef _WIN32
void SetStringResource(HINSTANCE hResInst, int iExtended);
#endif
void SetResourceStringUnscramble(void (*pResourceStringUnscramble)(char*));
