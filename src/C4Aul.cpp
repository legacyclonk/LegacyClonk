/* by Sven2, 2001 */
// C4Aul script engine CP conversion

#include <C4Include.h>
#include <C4Aul.h>

#ifndef BIG_C4INCLUDE
#include <C4Config.h>
#include <C4Def.h>
#include <C4Log.h>
#include <C4Components.h>
#endif

C4AulError::C4AulError() {}

void C4AulError::show()
{
#ifdef C4ENGINE
	// simply log error message
	if (sMessage)
		DebugLog(sMessage.getData());
#endif
}

C4AulFunc::C4AulFunc(C4AulScript *pOwner, const char *pName, bool bAtEnd) :
	MapNext(nullptr),
	LinkedTo(nullptr),
	OverloadedBy(nullptr),
	NextSNFunc(nullptr)
{
	// reg2list (at end or at the beginning)
	Owner = pOwner;
	if (bAtEnd)
	{
		if (Prev = Owner->FuncL)
		{
			Prev->Next = this;
			Owner->FuncL = this;
		}
		else
		{
			Owner->Func0 = this;
			Owner->FuncL = this;
		}
		Next = nullptr;
	}
	else
	{
		if (Next = Owner->Func0)
		{
			Next->Prev = this;
			Owner->Func0 = this;
		}
		else
		{
			Owner->Func0 = this;
			Owner->FuncL = this;
		}
		Prev = nullptr;
	}

	// store name
	SCopy(pName, (char *)&Name, C4AUL_MAX_Identifier);
	// add to global lookuptable with this name
	Owner->Engine->FuncLookUp.Add(this, bAtEnd);
}

C4AulFunc::~C4AulFunc()
{
	// if it's a global: remove the global link!
	if (LinkedTo && Owner)
		if (LinkedTo->Owner == Owner->Engine)
			delete LinkedTo;
	// unlink func
	if (LinkedTo)
	{
		// find prev
		C4AulFunc *pAkt = this;
		while (pAkt->LinkedTo != this) pAkt = pAkt->LinkedTo;
		if (pAkt == LinkedTo)
			pAkt->LinkedTo = nullptr;
		else
			pAkt->LinkedTo = LinkedTo;
		LinkedTo = nullptr;
	}
	// remove from list
	if (Prev) Prev->Next = Next;
	if (Next) Next->Prev = Prev;
	if (Owner)
	{
		if (Owner->Func0 == this) Owner->Func0 = Next;
		if (Owner->FuncL == this) Owner->FuncL = Prev;
		Owner->Engine->FuncLookUp.Remove(this);
	}
}

void C4AulFunc::DestroyLinked()
{
	// delete all functions linked to this one.
	while (LinkedTo)
		delete LinkedTo;
}

C4AulFunc *C4AulFunc::GetLocalSFunc(const char *szIdtf)
{
	// owner is engine, i.e. this is a global func?
	if (Owner == Owner->Engine && LinkedTo)
	{
		// then search linked scope first
		if (C4AulFunc *pFn = LinkedTo->Owner->GetSFunc(szIdtf)) return pFn;
	}
	// search local owner list
	return Owner->GetSFunc(szIdtf);
}

C4AulFunc *C4AulFunc::FindSameNameFunc(C4Def *pScope)
{
	// Note: NextSNFunc forms a ring, not a list
	// find function
	C4AulFunc *pFunc = this, *pResult = nullptr;
	do
	{
		// definition matches? This is the one
		if (pFunc->Owner->Def == pScope)
		{
			pResult = pFunc; break;
		}
		// global function? Set func, but continue searching
		// (may be overloaded by a local fn)
		if (pFunc->Owner == Owner->Engine)
			pResult = pFunc;
	} while ((pFunc = pFunc->NextSNFunc) && pFunc != this);
	return pResult;
}

StdStrBuf C4AulScriptFunc::GetFullName()
{
	// "lost" function?
	StdStrBuf sOwner;
	if (!Owner)
	{
		sOwner.Ref("(unknown) ");
	}
	else if (Owner->Def)
	{
		sOwner.Format("%s::", C4IdText(Owner->Def->id));
	}
	else if (Owner->Engine == Owner)
	{
		sOwner.Ref("global ");
	}
	else
	{
		sOwner.Ref("game ");
	}
	StdStrBuf sResult;
	sResult.Format("%s%s", sOwner.getData(), Name);
	return sResult;
}

C4AulScript::C4AulScript()
{
	// init defaults
	Default();
}

void C4AulScript::Default()
{
	// not compiled
	State = ASS_NONE;
	Script = nullptr;
	Code = CPos = nullptr;
	CodeSize = CodeBufSize = 0;
	IncludesResolved = false;

	// defaults
	idDef = C4ID_None;
	Strict = NONSTRICT;
	Preparsing = Resolving = false;
	Temporary = false;
	LocalNamed.Reset();

	// prepare lists
	Child0 = ChildL = Prev = Next = nullptr;
	Owner = Engine = nullptr;
	Func0 = FuncL = nullptr;
	// prepare include list
	Includes.clear();
	Appends.clear();
}

C4AulScript::~C4AulScript()
{
	// clear
	Clear();
	// unreg
	Unreg();
}

void C4AulScript::Unreg()
{
	// remove from list
	if (Prev) Prev->Next = Next; else if (Owner) Owner->Child0 = Next;
	if (Next) Next->Prev = Prev; else if (Owner) Owner->ChildL = Prev;
	Prev = Next = Owner = nullptr;
}

void C4AulScript::Clear()
{
	// remove includes
	Includes.clear();
	Appends.clear();
	// delete child scripts + funcs
	while (Child0)
		if (Child0->Delete()) delete Child0; else Child0->Unreg();
	while (Func0) delete Func0;
	// delete script+code
	Script.Clear();
	delete[] Code; Code = nullptr;
	CodeSize = CodeBufSize = 0;
	// reset flags
	State = ASS_NONE;
}

void C4AulScript::Reg2List(C4AulScriptEngine *pEngine, C4AulScript *pOwner)
{
	// already regged? (def reloaded)
	if (Owner) return;
	// reg to list
	Engine = pEngine;
	if (Owner = pOwner)
	{
		if (Prev = Owner->ChildL)
			Prev->Next = this;
		else
			Owner->Child0 = this;
		Owner->ChildL = this;
	}
	else
		Prev = nullptr;
	Next = nullptr;
}

C4AulFunc *C4AulScript::GetOverloadedFunc(C4AulFunc *ByFunc)
{
	assert(ByFunc);
	// search local list
	C4AulFunc *f = ByFunc;
	if (f) f = f->Prev; else f = FuncL;
	while (f)
	{
		if (SEqual(ByFunc->Name, f->Name)) break;
		f = f->Prev;
	}
#ifdef _DEBUG
	C4AulFunc *f2 = Engine ? Engine->GetFunc(ByFunc->Name, this, ByFunc) : nullptr;
	assert(f == f2);
#endif
	// nothing found? then search owner, if existant
	if (!f && Owner)
	{
		if (f = Owner->GetFuncRecursive(ByFunc->Name))
			// just found the global link?
			if (ByFunc && f->LinkedTo == ByFunc)
				f = Owner->GetOverloadedFunc(f);
	}
	// return found fn
	return f;
}

C4AulFunc *C4AulScript::GetFuncRecursive(const char *pIdtf)
{
	// search local list
	C4AulFunc *f = GetFunc(pIdtf);
	if (f) return f;
	// nothing found? then search owner, if existant
	else if (Owner) return Owner->GetFuncRecursive(pIdtf);
	return nullptr;
}

C4AulFunc *C4AulScript::GetFunc(const char *pIdtf)
{
	return Engine ? Engine->GetFunc(pIdtf, this, nullptr) : nullptr;
}

C4AulScriptFunc *C4AulScript::GetSFuncWarn(const char *pIdtf, C4AulAccess AccNeeded, const char *WarnStr)
{
	// no identifier
	if (!pIdtf || !pIdtf[0]) return nullptr;
	// get func?
	C4AulScriptFunc *pFn = GetSFunc(pIdtf, AccNeeded, true);
	if (!pFn)
		Warn(FormatString("Error getting %s function '%s'", WarnStr, pIdtf).getData(), nullptr);
	return pFn;
}

C4AulScriptFunc *C4AulScript::GetSFunc(const char *pIdtf, C4AulAccess AccNeeded, bool fFailsafe)
{
	// failsafe call
	if (*pIdtf == '~') { fFailsafe = true; pIdtf++; }

	// get function reference from table
	C4AulScriptFunc *pFn = GetSFunc(pIdtf);

	// undefined function
	if (!pFn)
	{
		// not failsafe?
		if (!fFailsafe)
		{
			// show error
			C4AulParseError err(this, "Undefined function: ", pIdtf);
			err.show();
		}
		return nullptr;
	}

	// check access
	if (pFn->Access < AccNeeded)
	{
		// no access? show error
		C4AulParseError err(this, "insufficient access level");
		err.show();
		// don't even break in strict execution, because the caller might be non-strict
	}

	// return found function
	return pFn;
}

C4AulScriptFunc *C4AulScript::GetSFunc(const char *pIdtf)
{
	// get func by name; return script func
	if (!pIdtf) return nullptr;
	if (!pIdtf[0]) return nullptr;
	if (pIdtf[0] == '~') pIdtf++;
	C4AulFunc *f = GetFunc(pIdtf);
	if (!f) return nullptr;
	return f->SFunc();
}

C4AulScriptFunc *C4AulScript::GetSFunc(int iIndex, const char *szPattern)
{
	C4AulFunc *f;
	C4AulScriptFunc *sf;
	// loop through script funcs
	f = FuncL;
	while (f)
	{
		if (sf = f->SFunc())
			if (!szPattern || SEqual2(sf->Name, szPattern))
			{
				if (!iIndex) return sf;
				--iIndex;
			}
		f = f->Prev;
	}

	// indexed script func not found
	return nullptr;
}

void C4AulScriptFunc::CopyBody(C4AulScriptFunc &FromFunc)
{
	// copy some members, that are set before linking
	Access = FromFunc.Access;
	Desc.Copy(FromFunc.Desc);
	DescText.Copy(FromFunc.DescText);
	DescLong.Copy(FromFunc.DescLong);
	Condition = FromFunc.Condition;
	idImage = FromFunc.idImage;
	iImagePhase = FromFunc.iImagePhase;
	ControlMethod = FromFunc.ControlMethod;
	Script = FromFunc.Script;
	VarNamed = FromFunc.VarNamed;
	ParNamed = FromFunc.ParNamed;
	bNewFormat = FromFunc.bNewFormat;
	bReturnRef = FromFunc.bReturnRef;
	pOrgScript = FromFunc.pOrgScript;
	for (int i = 0; i < C4AUL_MAX_Par; i++)
		ParType[i] = FromFunc.ParType[i];
	// must reset field here
	NextSNFunc = nullptr;
}

void C4AulScript::AddFunc(const char *pIdtf, C4ScriptFnDef *Def)
{
	// create def func
	new C4AulDefFunc(this, pIdtf, Def);
}

// C4AulScriptEngine

C4AulScriptEngine::C4AulScriptEngine() :
	warnCnt(0), errCnt(0), nonStrictCnt(0), lineCnt(0)
{
	// /me r b engine
	Engine = this;
	ScriptName.Ref(C4CFN_System);
	Strict = MAXSTRICT;

	Global.Reset();

	GlobalNamedNames.Reset();
	GlobalNamed.Reset();
	GlobalNamed.SetNameList(&GlobalNamedNames);
	GlobalConstNames.Reset();
	GlobalConsts.Reset();
	GlobalConsts.SetNameList(&GlobalConstNames);
}

C4AulScriptEngine::~C4AulScriptEngine() { Clear(); }

void C4AulScriptEngine::Clear()
{
	// clear inherited
	C4AulScript::Clear();
	// clear own stuff
	// reset values
	warnCnt = errCnt = nonStrictCnt = lineCnt = 0;
	// resetting name lists will reset all data lists, too
	// except not...
	Global.Reset();
	GlobalNamedNames.Reset();
	GlobalConstNames.Reset();
	GlobalConsts.Reset();
	GlobalConsts.SetNameList(&GlobalConstNames);
	GlobalNamed.Reset();
	GlobalNamed.SetNameList(&GlobalNamedNames);
}

void C4AulScriptEngine::UnLink()
{
	// unlink scripts
	C4AulScript::UnLink();
	// clear string table ("hold" strings only)
	Strings.Clear();
	// Do not clear global variables and constants, because they are registered by the
	// preparser. Note that keeping those fields means that you cannot delete a global
	// variable or constant at runtime by removing it from the script.
}

void C4AulScriptEngine::RegisterGlobalConstant(const char *szName, const C4Value &rValue)
{
	// Register name and set value.
	// AddName returns the index of existing element if the name is assigned already.
	// That is OK, since it will only change the value of the global ("overload" it).
	// A warning would be nice here. However, this warning would show up whenever a script
	// containing globals constants is recompiled.
	GlobalConsts[GlobalConstNames.AddName(szName)] = rValue;
}

bool C4AulScriptEngine::GetGlobalConstant(const char *szName, C4Value *pTargetValue)
{
	// get index of global by name
	int32_t iConstIndex = GlobalConstNames.GetItemNr(szName);
	// not found?
	if (iConstIndex < 0) return false;
	// if it's found, assign the value if desired
	if (pTargetValue) *pTargetValue = GlobalConsts[iConstIndex];
	// constant exists
	return true;
}

bool C4AulScriptEngine::DenumerateVariablePointers()
{
	Global.DenumeratePointers();
	GlobalNamed.DenumeratePointers();
	// runtime data only: don't denumerate consts
	return true;
}

void C4AulScriptEngine::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(Global, "Globals", C4ValueList()));
	C4ValueMapData GlobalNamedDefault;
	GlobalNamedDefault.SetNameList(&GlobalNamedNames);
	pComp->Value(mkNamingAdapt(GlobalNamed, "GlobalNamed", GlobalNamedDefault));
}

// C4AulFuncMap

static const size_t CapacityInc = 1024;

C4AulFuncMap::C4AulFuncMap() : Capacity(CapacityInc), FuncCnt(0), Funcs(new C4AulFunc *[CapacityInc])
{
	memset(Funcs, 0, sizeof(C4AulFunc *) * Capacity);
}

C4AulFuncMap::~C4AulFuncMap()
{
	delete[] Funcs;
}

unsigned int C4AulFuncMap::Hash(const char *name)
{
	// Fowler/Noll/Vo hash
	unsigned int h = 2166136261u;
	while (*name)
		h = (h ^ * (name++)) * 16777619;
	return h;
}

C4AulFunc *C4AulFuncMap::GetFirstFunc(const char *Name)
{
	if (!Name) return nullptr;
	C4AulFunc *Func = Funcs[Hash(Name) % Capacity];
	while (Func && !SEqual(Name, Func->Name))
		Func = Func->MapNext;
	return Func;
}

C4AulFunc *C4AulFuncMap::GetNextSNFunc(const C4AulFunc *After)
{
	C4AulFunc *Func = After->MapNext;
	while (Func && !SEqual(After->Name, Func->Name))
		Func = Func->MapNext;
	return Func;
}

C4AulFunc *C4AulFuncMap::GetFunc(const char *Name, const C4AulScript *Owner, const C4AulFunc *After)
{
	if (!Name) return nullptr;
	C4AulFunc *Func = Funcs[Hash(Name) % Capacity];
	if (After)
	{
		while (Func && Func != After)
			Func = Func->MapNext;
		if (Func)
			Func = Func->MapNext;
	}
	while (Func && (Func->Owner != Owner || !SEqual(Name, Func->Name)))
		Func = Func->MapNext;
	return Func;
}

void C4AulFuncMap::Add(C4AulFunc *func, bool bAtStart)
{
	if (++FuncCnt > Capacity)
	{
		int NCapacity = Capacity + CapacityInc;
		C4AulFunc **NFuncs = new C4AulFunc*[NCapacity];
		memset(NFuncs, 0, sizeof(C4AulFunc *) * NCapacity);
		for (int i = 0; i < Capacity; ++i)
		{
			while (Funcs[i])
			{
				// Get a pointer to the bucket
				C4AulFunc **pNFunc = &(NFuncs[Hash(Funcs[i]->Name) % NCapacity]);
				// get a pointer to the end of the linked list
				while (*pNFunc) pNFunc = &((*pNFunc)->MapNext);
				// Move the func over
				*pNFunc = Funcs[i];
				// proceed with the next list member
				Funcs[i] = Funcs[i]->MapNext;
				// Terminate the linked list
				(*pNFunc)->MapNext = 0;
			}
		}
		Capacity = NCapacity;
		delete[] Funcs;
		Funcs = NFuncs;
	}
	// Get a pointer to the bucket
	C4AulFunc **pFunc = &(Funcs[Hash(func->Name) % Capacity]);
	if (bAtStart)
	{
		// move the current first to the second position
		func->MapNext = *pFunc;
	}
	else
	{
		// get a pointer to the end of the linked list
		while (*pFunc)
		{
			pFunc = &((*pFunc)->MapNext);
		}
	}
	// Add the func
	*pFunc = func;
}

void C4AulFuncMap::Remove(C4AulFunc *func)
{
	C4AulFunc **pFunc = &Funcs[Hash(func->Name) % Capacity];
	while (*pFunc != func)
	{
		pFunc = &((*pFunc)->MapNext);
		assert(*pFunc); // crash on remove of a not contained func
	}
	*pFunc = (*pFunc)->MapNext;
	--FuncCnt;
}
