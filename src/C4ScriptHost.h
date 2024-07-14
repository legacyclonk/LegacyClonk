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

/* Component host for CAulScript */

#pragma once

#include <C4ComponentHost.h>

#include <C4Aul.h>

// generic script host for objects
class C4ScriptHost : public C4AulScript, public C4ComponentHost
{
public:
	C4ScriptHost();
	~C4ScriptHost();
	bool Delete() override { return true; }

public:
	void Default();
	void Clear();
	void Close() override;
	bool Load(const char *szName, C4Group &hGroup, const char *szFilename,
		const char *szLanguage, C4Def *pDef, class C4LangStringTable *pLocalTable, bool fLoadTable = false);
	const char *GetControlDesc(const char *szFunctionFormat, int32_t iCom, C4ID *pidImage = nullptr, int32_t *piImagePhase = nullptr);
	void GetControlMethodMask(std::format_string<const char *> functionFormat, int32_t &first, int32_t &second);
	int32_t GetControlMethod(int32_t com, int32_t first, int32_t second);

	C4Value ObjectCall(C4Object *pCaller, C4Object *pObj, const char *szFunction, const C4AulParSet &pPars = C4AulParSet{}, bool fPassError = false, bool convertNilToIntBool = true);

	C4Value Call(C4Section &section, const char *szFunction, const C4AulParSet &pPars = C4AulParSet{}, bool fPassError = false, bool convertNilToIntBool = true)
	{
		if (!szFunction) return C4VNull;
		return FunctionCall(section, nullptr, szFunction, nullptr, pPars, false, fPassError, convertNilToIntBool);
	}

protected:
	class C4LangStringTable *pStringTable;
	void MakeScript();
	C4Value FunctionCall(C4Section &section, C4Object *pCaller, const char *szFunction, C4Object *pObj, const C4AulParSet &pPars = C4AulParSet{}, bool fPrivateCall = false, bool fPassError = false, bool convertNilToIntBool = true);
	bool ReloadScript(const char *szPath) override;
};

// script host for defs
class C4DefScriptHost : public C4ScriptHost
{
public:
	C4DefScriptHost() : C4ScriptHost() { Default(); }

	void Default();

	bool Delete() override { return false; } // do NOT delete this - it's just a class member!

protected:
	void AfterLink() override; // get common funcs

public:
	C4AulScriptFunc *SFn_CalcValue; // get object value
	C4AulScriptFunc *SFn_SellTo; // player par(0) sold the object
	C4AulScriptFunc *SFn_ControlTransfer; // object par(0) tries to get to par(1)/par(2)
	C4AulScriptFunc *SFn_CustomComponents; // PSF_GetCustomComponents
	int32_t ControlMethod[2], ContainedControlMethod[2], ActivationControlMethod[2];
};

// script host for scenario scripts
class C4GameScriptHost : public C4ScriptHost
{
public:
	C4GameScriptHost();
	~C4GameScriptHost();
	bool Delete() override { return false; } // do NOT delete this - it's just a class member!
	void Default();
	C4Value GRBroadcast(const char *szFunction, const C4AulParSet &pPars = C4AulParSet{}, bool fPassError = false, bool fRejectTest = false, bool convertNilToIntBool = true); // call function in scenario script and all goals/rules/environment objects

	// Global script data
	// FIXME: Move to C4AulScriptEngine
	int32_t Counter;
	bool Go;
	bool Execute();

	// Compile scenario script data
	void CompileFunc(StdCompiler *pComp);
};
