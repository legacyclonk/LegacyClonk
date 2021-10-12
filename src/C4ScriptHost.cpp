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

/* Handles script file components (calls, inheritance, function maps) */

#include <C4Include.h>
#include <C4ScriptHost.h>

#include <C4Console.h>
#include <C4ObjectCom.h>
#include <C4Object.h>
#include <C4Wrappers.h>

// C4ScriptHost

C4ScriptHost::C4ScriptHost() { Default(); }
C4ScriptHost::~C4ScriptHost() { Clear(); }

void C4ScriptHost::Default()
{
	C4AulScript::Default();
	C4ComponentHost::Default();
	pStringTable = nullptr;
}

void C4ScriptHost::Clear()
{
	C4AulScript::Clear();
	C4ComponentHost::Clear();
	pStringTable = nullptr;
}

bool C4ScriptHost::Load(const char *szName, C4Group &hGroup, const char *szFilename,
	const char *szLanguage, C4Def *pDef, class C4LangStringTable *pLocalTable, bool fLoadTable)
{
	// Set definition and id
	Def = pDef; idDef = Def ? Def->id : 0;
	// Base load
	bool fSuccess = C4ComponentHost::LoadAppend(szName, hGroup, szFilename, szLanguage);
	// String Table
	pStringTable = pLocalTable;
	// load it if specified
	if (pStringTable && fLoadTable)
		pStringTable->LoadEx("StringTbl", hGroup, C4CFN_ScriptStringTbl, szLanguage);
	// set name
	ScriptName.Format("%s" DirSep "%s", hGroup.GetFullName().getData(), Filename);
	// preparse script
#ifdef C4ENGINE
	MakeScript();
#endif
	// Success
	return fSuccess;
}

void C4ScriptHost::MakeScript()
{
	// clear prev
	Script.Clear();

	// create script
	if (pStringTable)
	{
		pStringTable->ReplaceStrings(Data, Script, FilePath);
	}
	else
	{
		Script.Ref(Data);
	}

	// preparse script
	Preparse();
}

void C4ScriptHost::Close()
{
	// Base close
	C4ComponentHost::Close();
	// Make executable script
	MakeScript();
	// Update console
#ifdef C4ENGINE
	Console.UpdateInputCtrl();
#endif
}

int32_t C4ScriptHost::GetControlMethod(int32_t com, int32_t first, int32_t second)
{
	return ((first >> com) & 0x01) | (((second >> com) & 0x01) << 1);
}

void C4ScriptHost::GetControlMethodMask(const char *szFunctionFormat, int32_t &first, int32_t &second)
{
	first = second = 0;

	if (!Script) return;

	// Scan for com defined control functions
#ifdef C4ENGINE
	int32_t iCom;
	char szFunction[256 + 1];
	for (iCom = 0; iCom < ComOrderNum; iCom++)
	{
		sprintf(szFunction, szFunctionFormat, ComName(ComOrder(iCom)));
		C4AulScriptFunc *func = GetSFunc(szFunction);

		if (func)
		{
			first |= ((func->ControlMethod) & 0x01) << iCom;
			second |= ((func->ControlMethod >> 1) & 0x01) << iCom;
		}
	}
#endif
}

C4Value C4ScriptHost::FunctionCall(C4Object *pCaller, const char *szFunction, C4Object *pObj, const C4AulParSet &Pars, bool fPrivateCall, bool fPassError)
{
#ifdef C4ENGINE

	// get needed access
	C4AulAccess Acc = AA_PRIVATE;
	if (pObj && (pObj != pCaller) && !fPrivateCall)
		if (pCaller) Acc = AA_PUBLIC; else Acc = AA_PROTECTED;
	// get function
	C4AulScriptFunc *pFn;
	if (!(pFn = GetSFunc(szFunction, Acc))) return C4VNull;
	// Call code
	return pFn->Exec(pObj, Pars, fPassError);

#else

	return 0;

#endif
}

bool C4ScriptHost::ReloadScript(const char *szPath)
{
	// this?
	if (SEqualNoCase(szPath, FilePath) || (pStringTable && SEqualNoCase(szPath, pStringTable->GetFilePath())))
	{
		// try reload
		char szParentPath[_MAX_PATH + 1]; C4Group ParentGrp;
		if (GetParentPath(szPath, szParentPath))
			if (ParentGrp.Open(szParentPath))
				if (Load(Name, ParentGrp, Filename, Config.General.Language, nullptr, pStringTable))
					return true;
	}
	// call for childs
	return C4AulScript::ReloadScript(szPath);
}

const char *C4ScriptHost::GetControlDesc(const char *szFunctionFormat, int32_t iCom, C4ID *pidImage, int32_t *piImagePhase)
{
#ifdef C4ENGINE
	// Compose script function
	char szFunction[256 + 1];
	sprintf(szFunction, szFunctionFormat, ComName(iCom));
	// Remove failsafe indicator
	if (szFunction[0] == '~') memmove(szFunction, szFunction + 1, sizeof(szFunction) - 1);
	// Find function reference
	C4AulScriptFunc *pFn = GetSFunc(szFunction);
	// Get image id
	if (pidImage) { *pidImage = idDef; if (pFn) *pidImage = pFn->idImage; }
	// Get image phase
	if (piImagePhase) { *piImagePhase = 0; if (pFn) *piImagePhase = pFn->iImagePhase; }
	// Return function desc
	if (pFn && pFn->Desc.getLength()) return pFn->DescText.getData();
#endif
	// No function
	return nullptr;
}

// C4DefScriptHost

void C4DefScriptHost::Default()
{
	C4ScriptHost::Default();
	SFn_CalcValue = SFn_SellTo = SFn_ControlTransfer = SFn_CustomComponents = nullptr;
	ControlMethod[0] = ControlMethod[1] = ContainedControlMethod[0] = ContainedControlMethod[1] = ActivationControlMethod[0] = ActivationControlMethod[1] = 0;
}

void C4DefScriptHost::AfterLink()
{
	C4AulScript::AfterLink();
	// Search cached functions
	char WhereStr[C4MaxName + 18];
	SFn_CalcValue        = GetSFunc(PSF_CalcValue,           AA_PROTECTED);
	SFn_SellTo           = GetSFunc(PSF_SellTo,              AA_PROTECTED);
	SFn_ControlTransfer  = GetSFunc(PSF_ControlTransfer,     AA_PROTECTED);
	SFn_CustomComponents = GetSFunc(PSF_GetCustomComponents, AA_PROTECTED);
	if (Def)
	{
		C4AulAccess CallAccess = AA_PRIVATE;
		for (int32_t cnt = 0; cnt < Def->ActNum; cnt++)
		{
			C4ActionDef *pad = &Def->ActMap[cnt];
			sprintf(WhereStr, "Action %s: StartCall", pad->Name); pad->StartCall = GetSFuncWarn(pad->SStartCall, CallAccess, WhereStr);
			sprintf(WhereStr, "Action %s: PhaseCall", pad->Name); pad->PhaseCall = GetSFuncWarn(pad->SPhaseCall, CallAccess, WhereStr);
			sprintf(WhereStr, "Action %s: EndCall",   pad->Name); pad->EndCall   = GetSFuncWarn(pad->SEndCall,   CallAccess, WhereStr);
			sprintf(WhereStr, "Action %s: AbortCall", pad->Name); pad->AbortCall = GetSFuncWarn(pad->SAbortCall, CallAccess, WhereStr);
		}
		Def->TimerCall = GetSFuncWarn(Def->STimerCall, CallAccess, "TimerCall");
	}
	// Check if there are any Control/Contained/Activation script functions
	GetControlMethodMask(PSF_Control,                    ControlMethod[0],           ControlMethod[1]);
	GetControlMethodMask(PSF_ContainedControl,  ContainedControlMethod[0],  ContainedControlMethod[1]);
	GetControlMethodMask(PSF_Activate,         ActivationControlMethod[0], ActivationControlMethod[1]);
}

// C4GameScriptHost

C4GameScriptHost::C4GameScriptHost() : Counter(0), Go(false) {}
C4GameScriptHost::~C4GameScriptHost() {}

void C4GameScriptHost::Default()
{
	C4ScriptHost::Default();
	Counter = 0;
	Go = false;
}

bool C4GameScriptHost::Execute()
{
	if (!Script) return false;
#ifdef C4ENGINE
	char buffer[500];
	if (Go && !Tick10)
	{
		sprintf(buffer, PSF_Script, Counter++);
		return static_cast<bool>(Call(buffer));
	}
#endif
	return false;
}

C4Value C4GameScriptHost::GRBroadcast(const char *szFunction, const C4AulParSet &pPars, bool fPassError, bool fRejectTest)
{
	// call objects first - scenario script might overwrite hostility, etc...
	C4Object *pObj;
	for (C4ObjectLink *clnk = Game.Objects.ObjectsInt().First; clnk; clnk = clnk->Next) if (pObj = clnk->Obj)
		if (pObj->Category & (C4D_Goal | C4D_Rule | C4D_Environment))
			if (pObj->Status)
			{
				C4Value vResult(pObj->Call(szFunction, pPars, fPassError));
				// rejection tests abort on first nonzero result
				if (fRejectTest) if (vResult) return vResult;
			}
	// scenario script call
	return Call(szFunction, pPars, fPassError);
}

void C4GameScriptHost::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(Go,      "Go",      false));
	pComp->Value(mkNamingAdapt(Counter, "Counter", 0));
}
