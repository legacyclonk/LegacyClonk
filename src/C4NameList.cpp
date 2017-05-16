/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* A static list of strings and integer values, i.e. for material amounts */

#include <C4Include.h>
#include <C4NameList.h>

C4NameList::C4NameList()
{
	Clear();
}

void C4NameList::Clear()
{
	std::memset(this, 0, sizeof(C4NameList));
}

void C4NameList::CompileFunc(StdCompiler *pComp, bool fValues)
{
	bool fCompiler = pComp->isCompiler();
	for (int32_t cnt = 0; cnt < C4MaxNameList; cnt++)
		if (fCompiler || Name[cnt][0])
		{
			if (cnt) pComp->Seperator(StdCompiler::SEP_SEP2);
			// Name
			pComp->Value(mkDefaultAdapt(mkStringAdapt(Name[cnt], C4MaxName, StdCompiler::RCT_Idtf), ""));
			// Value
			if (fValues)
			{
				pComp->Seperator(StdCompiler::SEP_SET);
				pComp->Value(mkDefaultAdapt(Count[cnt], 0));
			}
		}
}
