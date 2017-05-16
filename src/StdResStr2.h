/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

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
