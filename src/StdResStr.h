/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Load multi-language strings from the resource string table */

char *LoadResStr(WORD id);
#ifdef _WIN32
void SetStringResource(HINSTANCE hResInst, int iExtended);
#endif
void SetResourceStringUnscramble(void (*pResourceStringUnscramble)(char*));
