/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* A static list of strings and integer values, i.e. for material amounts */

#ifndef INC_C4NameList
#define INC_C4NameList

class C4NameList
  {
  public:
    C4NameList();
  public:
    char Name[C4MaxNameList][C4MaxName+1];
    int32_t Count[C4MaxNameList];
  public:
    void Clear();
		bool Add(const char *szName, int32_t iCount=0);
		bool Set(const char *szName, int32_t iCount);
		bool Read(const char *szSource, int32_t iDefValue=0);
		bool Write(char *szTarget, bool fValues=true);
	public:
		bool IsEmpty();
		bool operator==(const C4NameList& rhs)
			{	return MemEqual((const uint8_t*)this,(const uint8_t*)&rhs,sizeof(C4NameList)); }
    void CompileFunc(StdCompiler *pComp, bool fValues = true);
  };

#endif
