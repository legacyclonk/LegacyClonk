/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2022, The LegacyClonk Team and contributors
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
	ScriptName = std::format("{}" DirSep "{}", hGroup.GetFullName().getData(), Filename);
	// preparse script
	MakeScript();
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
	Console.UpdateInputCtrl();
}

int32_t C4ScriptHost::GetControlMethod(int32_t com, int32_t first, int32_t second)
{
	return ((first >> com) & 0x01) | (((second >> com) & 0x01) << 1);
}

C4Value C4ScriptHost::ObjectCall(C4Object *pCaller, C4Object *pObj, const char *szFunction, const C4AulParSet &pPars, bool fPassError, bool convertNilToIntBool)
{
	if (!szFunction) return C4VNull;
	return FunctionCall(*pObj->Section, pCaller, szFunction, pObj, pPars, false, fPassError, convertNilToIntBool);
}

void C4ScriptHost::GetControlMethodMask(const std::format_string<const char *> functionFormat, int32_t &first, int32_t &second)
{
	first = second = 0;

	if (!Script) return;

	// Scan for com defined control functions
	int32_t iCom;
	char szFunction[256 + 1];
	for (iCom = 0; iCom < ComOrderNum; iCom++)
	{
		FormatWithNull(szFunction, functionFormat, ComName(ComOrder(iCom)));
		C4AulScriptFunc *func = GetSFunc(szFunction);

		if (func)
		{
			first |= ((func->ControlMethod) & 0x01) << iCom;
			second |= ((func->ControlMethod >> 1) & 0x01) << iCom;
		}
	}
}

C4Value C4ScriptHost::FunctionCall(C4Section &section, C4Object *pCaller, const char *szFunction, C4Object *pObj, const C4AulParSet &Pars, bool fPrivateCall, bool fPassError, bool convertNilToIntBool)
{
	// get needed access
	C4AulAccess Acc = AA_PRIVATE;
	if (pObj && (pObj != pCaller) && !fPrivateCall)
		if (pCaller) Acc = AA_PUBLIC; else Acc = AA_PROTECTED;
	// get function
	C4AulScriptFunc *pFn;
	if (!(pFn = GetSFunc(szFunction, Acc))) return C4VNull;
	// Call code
	return pFn->Exec(section, pObj, Pars, fPassError, true, convertNilToIntBool);
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
	// Compose script function
	const char *const comName{ComName(iCom)};
	std::string function{std::vformat(szFunctionFormat, std::make_format_args(comName))};
	// Remove failsafe indicator
	if (function.starts_with('~'))
	{
		function.erase(function.begin());
	}
	// Find function reference
	C4AulScriptFunc *pFn = GetSFunc(function.c_str());
	// Get image id
	if (pidImage) { *pidImage = idDef; if (pFn) *pidImage = pFn->idImage; }
	// Get image phase
	if (piImagePhase) { *piImagePhase = 0; if (pFn) *piImagePhase = pFn->iImagePhase; }
	// Return function desc
	if (pFn && pFn->Desc.getLength()) return pFn->DescText.getData();
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
			FormatWithNull(WhereStr, "Action {}: StartCall", pad->Name); pad->StartCall = GetSFuncWarn(pad->SStartCall, CallAccess, WhereStr);
			FormatWithNull(WhereStr, "Action {}: PhaseCall", pad->Name); pad->PhaseCall = GetSFuncWarn(pad->SPhaseCall, CallAccess, WhereStr);
			FormatWithNull(WhereStr, "Action {}: EndCall",   pad->Name); pad->EndCall   = GetSFuncWarn(pad->SEndCall,   CallAccess, WhereStr);
			FormatWithNull(WhereStr, "Action {}: AbortCall", pad->Name); pad->AbortCall = GetSFuncWarn(pad->SAbortCall, CallAccess, WhereStr);
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
	char buffer[500];
	if (Go && !Tick10)
	{
		FormatWithNull(buffer, PSF_Script, Counter++);
		return static_cast<bool>(Call(*Game.Sections.front(), buffer)); // FIXME
	}
	return false;
}

C4Value C4GameScriptHost::GRBroadcast(const char *szFunction, const C4AulParSet &pPars, bool fPassError, bool fRejectTest, bool convertNilToIntBool)
{
	// call objects first - scenario script might overwrite hostility, etc...

	for (const auto &section : Game.Sections)
	{
		C4Object *pObj;
		for (C4ObjectLink *clnk = section->Objects.ObjectsInt().First; clnk; clnk = clnk->Next) if (pObj = clnk->Obj)
			if (pObj->Category & (C4D_Goal | C4D_Rule | C4D_Environment))
				if (pObj->Status)
				{
					C4Value vResult(pObj->Call(szFunction, pPars, fPassError, convertNilToIntBool));
					// rejection tests abort on first nonzero result
					if (fRejectTest) if (vResult) return vResult;
				}
	}
	// scenario script call
	return Call(*Game.Sections.front(), szFunction, pPars, fPassError, convertNilToIntBool); // FIXME
}

void C4GameScriptHost::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(Go,      "Go",      false));
	pComp->Value(mkNamingAdapt(Counter, "Counter", 0));
}
