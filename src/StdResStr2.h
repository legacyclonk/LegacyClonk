/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Load strings from a primitive memory string table */

#pragma once

const char *LoadResStr(const char *id);
char *LoadResStrNoAmp(const char *id);
char *GetResStr(const char *id, const char *strTable);

void SetResStrTable(char *pTable);
void ClearResStrTable();
bool IsResStrTableLoaded();
