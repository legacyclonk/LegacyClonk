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

/* Load strings from a primitive memory string table */

#ifndef INC_STD_RES_STR_2_H
#define INC_STD_RES_STR_2_H

const char *LoadResStr(const char *id);
char *LoadResStrNoAmp(const char *id);
char *GetResStr(const char *id, const char *strTable);

void SetResStrTable(char *pTable);
void ClearResStrTable();
bool IsResStrTableLoaded();

#endif // INC_STD_RES_STR_2_H
