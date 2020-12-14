/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
 * Copyright (c) 2017-2020, The LegacyClonk Team and contributors
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

// executes script functions

#include <C4Include.h>
#include <C4Aul.h>

#include <C4Object.h>
#include <C4Config.h>
#include <C4Game.h>
#include <C4ValueHash.h>
#include <C4Wrappers.h>

C4AulExecError::C4AulExecError(C4Object *pObj, const char *szError) : cObj(pObj)
{
	// direct error message string
	sMessage.Format("ERROR: %s", szError ? szError : "(no error message)");
}

void C4AulExecError::show() const
{
#ifdef C4ENGINE
	// log
	C4AulError::show();
	// debug mode object message
	if (Game.DebugMode)
		if (cObj)
			Game.Messages.New(C4GM_Target, sMessage, cObj, NO_OWNER);
		else
			Game.Messages.New(C4GM_Global, sMessage, nullptr, ANY_OWNER);
#endif
}

const int MAX_CONTEXT_STACK = 512;
const int MAX_VALUE_STACK = 1024;

void C4AulScriptContext::dump(StdStrBuf Dump)
{
	bool fDirectExec = !*Func->Name;
	if (!fDirectExec)
	{
		// Function name
		Dump.Append(Func->Name);
		// Parameters
		Dump.AppendChar('(');
		int iNullPars = 0;
		for (int i = 0; i < C4AUL_MAX_Par; i++)
			if (Pars + i < Vars)
				if (!Pars[i])
					iNullPars++;
				else
				{
					if (i > iNullPars)
						Dump.AppendChar(',');
					// Insert missing null parameters
					while (iNullPars > 0)
					{
						Dump.Append("0,");
						iNullPars--;
					}
					// Insert parameter
					Dump.Append(Pars[i].GetDataString());
				}
		Dump.AppendChar(')');
	}
	else
		Dump.Append(Func->Owner->ScriptName);
	// Context
	if (Obj)
		Dump.AppendFormat(" (obj %s)", C4VObj(Obj).GetDataString().getData());
	else if (Func->Owner->Def != nullptr)
		Dump.AppendFormat(" (def %s)", Func->Owner->Def->Name.getData());
	// Script
	if (!fDirectExec && Func->Owner)
		Dump.AppendFormat(" (%s:%d)",
			Func->pOrgScript->ScriptName.getData(),
			SGetLine(Func->pOrgScript->GetScript(), CPos ? CPos->SPos : Func->Script));
	// Log it
	DebugLog(Dump.getData());
}

class C4AulExec
{
public:
	C4AulExec()
		: pCurCtx(Contexts - 1), pCurVal(Values - 1), iTraceStart(-1) {}

private:
	C4AulScriptContext Contexts[MAX_CONTEXT_STACK];
	C4Value Values[MAX_VALUE_STACK];

	C4AulScriptContext *pCurCtx;
	C4Value *pCurVal;

	int iTraceStart;
	bool fProfiling;
	time_t tDirectExecStart, tDirectExecTotal; // profiler time for DirectExec
	C4AulScript *pProfiledScript;

public:
	C4Value Exec(C4AulScriptFunc *pSFunc, C4Object *pObj, const C4Value pPars[], bool fPassErrors, bool fTemporaryScript = false);
	C4Value Exec(C4AulBCC *pCPos, bool fPassErrors);

	void StartTrace();
	void StartProfiling(C4AulScript *pScript); // resets profling times and starts recording the times
	void StopProfiling(); // stop the profiler and displays results
	void AbortProfiling() { fProfiling = false; }
	inline void StartDirectExec() { if (fProfiling) tDirectExecStart = timeGetTime(); }
	inline void StopDirectExec() { if (fProfiling) tDirectExecTotal += timeGetTime() - tDirectExecStart; }

private:
	void PushContext(const C4AulScriptContext &rContext)
	{
		if (pCurCtx >= Contexts + MAX_CONTEXT_STACK - 1)
			throw C4AulExecError(pCurCtx->Obj, "context stack overflow!");
		*++pCurCtx = rContext;
		// Trace?
		if (iTraceStart >= 0)
		{
			StdStrBuf Buf("T");
			Buf.AppendChars('>', ContextStackSize() - iTraceStart);
			pCurCtx->dump(std::move(Buf));
		}
		// Profiler: Safe time to measure difference afterwards
		if (fProfiling) pCurCtx->tTime = timeGetTime();
	}

	void PopContext()
	{
		if (pCurCtx < Contexts)
			throw C4AulExecError(pCurCtx->Obj, "context stack underflow!");
		// Profiler adding up times
		if (fProfiling)
		{
			time_t dt = timeGetTime() - pCurCtx->tTime;
			if (dt && pCurCtx->Func)
				pCurCtx->Func->tProfileTime += dt;
		}
		// Trace done?
		if (iTraceStart >= 0)
		{
			if (ContextStackSize() <= iTraceStart)
			{
				iTraceStart = -1;
			}
		}
		if (pCurCtx->TemporaryScript)
			delete pCurCtx->Func->Owner;
		pCurCtx--;
	}

	void CheckOverflow(intptr_t iCnt)
	{
		if (ValueStackSize() + iCnt > MAX_VALUE_STACK)
			throw C4AulExecError(pCurCtx->Obj, "internal error: value stack overflow!");
	}

	void PushString(C4String *Str)
	{
		CheckOverflow(1);
		(++pCurVal)->SetString(Str);
	}

	void PushArray(C4ValueArray *Array)
	{
		CheckOverflow(1);
		(++pCurVal)->SetArray(Array);
	}

	void PushMap(C4ValueHash *Map)
	{
		CheckOverflow(1);
		(++pCurVal)->SetMap(Map);
	}

	void PushValue(const C4Value &rVal)
	{
		CheckOverflow(1);
		(++pCurVal)->Set(rVal);
	}

	void PushValueRef(C4Value &rVal)
	{
		CheckOverflow(1);
		(++pCurVal)->SetRef(&rVal);
	}

	void PushNullVals(intptr_t iCnt)
	{
		CheckOverflow(iCnt);
		pCurVal += iCnt;
	}

	bool PopValue()
	{
		if (LocalValueStackSize() < 1)
			throw C4AulExecError(pCurCtx->Obj, "internal error: value stack underflow!");
		(pCurVal--)->Set0();
		return true;
	}

	void PopValues(intptr_t n)
	{
		if (LocalValueStackSize() < n)
			throw C4AulExecError(pCurCtx->Obj, "internal error: value stack underflow!");
		while (n--)
			(pCurVal--)->Set0();
	}

	void PopValuesUntil(C4Value *pUntilVal)
	{
		if (pUntilVal < Values - 1)
			throw C4AulExecError(pCurCtx->Obj, "internal error: value stack underflow!");
		while (pCurVal > pUntilVal)
			(pCurVal--)->Set0();
	}

	int ContextStackSize() const
	{
		return pCurCtx - Contexts + 1;
	}

	int ValueStackSize() const
	{
		return pCurVal - Values + 1;
	}

	int LocalValueStackSize() const
	{
		return ContextStackSize()
			? pCurVal - pCurCtx->Vars - pCurCtx->Func->VarNamed.iSize + 1
			: pCurVal - Values + 1;
	}

	template<bool asReference = false, bool allowAny = true>
	void CheckOpPar(C4Value *value, C4V_Type expectedType, const char *operatorName, const char *operandPosition = "")
	{
		const bool isAtLeastStrict3 = pCurCtx->Func->pOrgScript->Strict >= C4AulScriptStrict::STRICT3;
		if constexpr (asReference)
		{
			if (!value->ConvertTo(C4V_pC4Value))
			{
				throw C4AulExecError(pCurCtx->Obj,
					FormatString("operator \"%s\"%s: got \"%s\", but expected \"%s&\"!",
						operatorName, operandPosition, value->GetTypeInfo(), GetC4VName(expectedType)).getData());
			}

			if (!isAtLeastStrict3 && (expectedType != C4V_pC4Value) && !*value)
			{
				value->GetRefVal().Set0();
			}
			if (!value->GetRefVal().ConvertTo(expectedType))
			{
				throw C4AulExecError(pCurCtx->Obj,
					FormatString("operator \"%s\"%s: got \"%s&\", but expected \"%s&\"!",
						operatorName, operandPosition, value->GetRefVal().GetTypeInfo(), GetC4VName(expectedType)).getData());
			}
		}
		else
		{
			if (!isAtLeastStrict3 && (expectedType != C4V_pC4Value) && !*value)
			{
				value->Set0();
			}
			if (!value->ConvertTo(expectedType))
				throw C4AulExecError(pCurCtx->Obj,
					FormatString("operator \"%s\"%s: got \"%s\", but expected \"%s\"!",
						operatorName, operandPosition, value->GetTypeInfo(), GetC4VName(expectedType)).getData());
		}
		if constexpr (!allowAny)
		{
			if (isAtLeastStrict3 && value->GetType() == C4V_Any)
				throw C4AulExecError(pCurCtx->Obj,
					FormatString("operator \"%s\"%s: got nil, but expected \"%s\"!",
						operatorName, operandPosition, GetC4VName(expectedType)).getData());
		}
	}

	template<C4V_Type leftAsReference = C4V_Any, C4V_Type rightAsReference = C4V_Any, bool leftAllowAny = true, bool rightAllowAny = true>
	void CheckOpPars(intptr_t iOpID)
	{
		CheckOpPar<leftAsReference != C4V_Any, leftAllowAny>(&pCurVal[-1], leftAsReference != C4V_Any ? leftAsReference : C4ScriptOpMap[iOpID].Type1, C4ScriptOpMap[iOpID].Identifier, " left side");
		CheckOpPar<rightAsReference != C4V_Any, rightAllowAny>(pCurVal, rightAsReference != C4V_Any ? rightAsReference : C4ScriptOpMap[iOpID].Type2, C4ScriptOpMap[iOpID].Identifier, " right side");
	}

	template<C4V_Type asReference = C4V_Any, bool allowAny = true>
	void CheckOpPar(intptr_t iOpID)
	{
		if constexpr (asReference != C4V_Any)
			CheckOpPar<true>(pCurVal, asReference, C4ScriptOpMap[iOpID].Identifier);
		else
			CheckOpPar<false>(pCurVal, C4ScriptOpMap[iOpID].Type1, C4ScriptOpMap[iOpID].Identifier);
	}

	C4AulBCC *Call(C4AulFunc *pFunc, C4Value *pReturn, C4Value *pPars, C4Object *pObj = nullptr, C4Def *pDef = nullptr, bool globalContext = false);
};

C4AulExec AulExec;

C4Value C4AulExec::Exec(C4AulScriptFunc *pSFunc, C4Object *pObj, const C4Value *pnPars, bool fPassErrors, bool fTemporaryScript)
{
	// Push parameters
	C4Value *pPars = pCurVal + 1;
	if (pnPars)
		for (int i = 0; i < C4AUL_MAX_Par; i++)
			PushValue(pnPars[i]);

	// Push variables
	C4Value *pVars = pCurVal + 1;
	PushNullVals(pSFunc->VarNamed.iSize);

	// Derive definition context from function owner (legacy)
	C4Def *pDef = pObj ? pObj->Def : pSFunc->Owner->Def;

	// Executing function in right context?
	// This must hold: The scripter might try to access local variables that don't exist!
	assert(!pSFunc->Owner->Def || pDef == pSFunc->Owner->Def);

	// Push a new context
	C4AulScriptContext ctx;
	ctx.Obj = pObj;
	ctx.Def = pDef;
	ctx.Return = nullptr;
	ctx.Pars = pPars;
	ctx.Vars = pVars;
	ctx.Func = pSFunc;
	ctx.TemporaryScript = fTemporaryScript;
	ctx.CPos = nullptr;
	ctx.Caller = nullptr;
	PushContext(ctx);

	// Execute
	return Exec(pSFunc->Code, fPassErrors);
}

C4Value C4AulExec::Exec(C4AulBCC *pCPos, bool fPassErrors)
{
	// Save start context
	C4AulScriptContext *pOldCtx = pCurCtx;

	try
	{
		for (;;)
		{
			bool fJump = false;
			switch (pCPos->bccType)
			{
			case AB_NIL:
				PushValue(C4VNull);
				break;

			case AB_INT:
				PushValue(C4VInt(static_cast<C4ValueInt>(pCPos->bccX)));
				break;

			case AB_BOOL:
				PushValue(C4VBool(!!pCPos->bccX));
				break;

			case AB_STRING:
				PushString(reinterpret_cast<C4String *>(pCPos->bccX));
				break;

			case AB_C4ID:
				PushValue(C4VID(static_cast<C4ID>(pCPos->bccX)));
				break;

			case AB_EOFN:
				throw C4AulExecError(pCurCtx->Obj, "function didn't return");

			case AB_ERR:
				throw C4AulExecError(pCurCtx->Obj, "syntax error: see previous parser error for details.");

			case AB_PARN_R:
				PushValueRef(pCurCtx->Pars[pCPos->bccX]);
				break;
			case AB_PARN_V:
				PushValue(pCurCtx->Pars[pCPos->bccX]);
				break;

			case AB_VARN_R:
				PushValueRef(pCurCtx->Vars[pCPos->bccX]);
				break;
			case AB_VARN_V:
				PushValue(pCurCtx->Vars[pCPos->bccX]);
				break;

			case AB_LOCALN_R: case AB_LOCALN_V:
				if (!pCurCtx->Obj)
					throw C4AulExecError(pCurCtx->Obj, "can't access local variables in a definition call!");
				if (pCurCtx->Func->Owner->Def != pCurCtx->Obj->Def)
				{
					const auto localName = pCurCtx->Func->Owner->Def->Script.LocalNamed.pNames[pCPos->bccX];
					if (pCurCtx->Func->pOrgScript->Strict >= C4AulScriptStrict::STRICT3 || pCurCtx->Obj->LocalNamed.pNames->iSize <= pCPos->bccX)
					{
						throw C4AulExecError(pCurCtx->Obj, FormatString("can't access local variable \"%s\" after ChangeDef!", localName).getData());
					}

					const auto actualLocalName = pCurCtx->Obj->Def->Script.LocalNamed.pNames[pCPos->bccX];

					if (!SEqual(localName, actualLocalName))
					{
						DebugLogF("WARNING: accessing local variable \"%s\" actually accesses \"%s\" because of illegal access after ChangeDef", localName, actualLocalName);
						pCurCtx->dump(StdStrBuf(" by: "));
					}
				}
				if (pCPos->bccType == AB_LOCALN_R)
					PushValueRef(*pCurCtx->Obj->LocalNamed.GetItem(pCPos->bccX));
				else
					PushValue(*pCurCtx->Obj->LocalNamed.GetItem(pCPos->bccX));
				break;

			case AB_GLOBALN_R:
				PushValueRef(*Game.ScriptEngine.GlobalNamed.GetItem(pCPos->bccX));
				break;
			case AB_GLOBALN_V:
				PushValue(*Game.ScriptEngine.GlobalNamed.GetItem(pCPos->bccX));
				break;
			// prefix
			case AB_Inc1: // ++
				CheckOpPar<C4V_Int, false>(pCPos->bccX);
				++pCurVal->GetData().Int;
				pCurVal->HintType(C4V_Int);
				break;
			case AB_Dec1: // --
				CheckOpPar<C4V_Int, false>(pCPos->bccX);
				--pCurVal->GetData().Int;
				pCurVal->HintType(C4V_Int);
				break;
			case AB_BitNot: // ~
				CheckOpPar<C4V_Any, false>(pCPos->bccX);
				pCurVal->SetInt(~pCurVal->_getInt());
				break;
			case AB_Not: // !
				CheckOpPar(pCPos->bccX);
				pCurVal->SetBool(!pCurVal->_getRaw());
				break;
			case AB_Neg: // -
				CheckOpPar<C4V_Any, false>(pCPos->bccX);
				pCurVal->SetInt(-pCurVal->_getInt());
				break;
			// postfix (whithout second statement)
			case AB_Inc1_Postfix: // ++
			{
				CheckOpPar<C4V_Int, false>(pCPos->bccX);
				auto &orig = pCurVal->GetRefVal();
				pCurVal->SetInt(orig._getInt());
				++orig.GetData().Int;
				orig.HintType(C4V_Int);
				break;
			}
			case AB_Dec1_Postfix: // --
			{
				CheckOpPar<C4V_Int, false>(pCPos->bccX);
				auto &orig = pCurVal->GetRefVal();
				pCurVal->SetInt(orig._getInt());
				--orig.GetData().Int;
				orig.HintType(C4V_Int);
				break;
			}
			// postfix
			case AB_Pow: // **
			{
				CheckOpPars<C4V_Any, C4V_Any, false, false>(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetInt(Pow(pPar1->_getInt(), pPar2->_getInt()));
				PopValue();
				break;
			}
			case AB_Div: // /
			{
				CheckOpPars<C4V_Any, C4V_Any, false, false>(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				if (pPar2->_getInt())
					pPar1->SetInt(pPar1->_getInt() / pPar2->_getInt());
				else
					pPar1->Set0();
				PopValue();
				break;
			}
			case AB_Mul: // *
			{
				CheckOpPars<C4V_Any, C4V_Any, false, false>(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetInt(pPar1->_getInt() * pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_Mod: // %
			{
				CheckOpPars<C4V_Any, C4V_Any, false, false>(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				if (pPar2->_getInt())
					pPar1->SetInt(pPar1->_getInt() % pPar2->_getInt());
				else
					pPar1->Set0();
				PopValue();
				break;
			}
			case AB_Sub: // -
			{
				CheckOpPars<C4V_Any, C4V_Any, false, false>(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetInt(pPar1->_getInt() - pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_Sum: // +
			{
				CheckOpPars<C4V_Any, C4V_Any, false, false>(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetInt(pPar1->_getInt() + pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_LeftShift: // <<
			{
				CheckOpPars<C4V_Any, C4V_Any, false, false>(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetInt(pPar1->_getInt() << pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_RightShift: // >>
			{
				CheckOpPars<C4V_Any, C4V_Any, false, false>(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetInt(pPar1->_getInt() >> pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_LessThan: // <
			{
				CheckOpPars<C4V_Any, C4V_Any, false, false>(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetBool(pPar1->_getInt() < pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_LessThanEqual: // <=
			{
				CheckOpPars<C4V_Any, C4V_Any, false, false>(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetBool(pPar1->_getInt() <= pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_GreaterThan: // >
			{
				CheckOpPars<C4V_Any, C4V_Any, false, false>(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetBool(pPar1->_getInt() > pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_GreaterThanEqual: // >=
			{
				CheckOpPars<C4V_Any, C4V_Any, false, false>(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetBool(pPar1->_getInt() >= pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_Concat: // ..
			case AB_ConcatIt: // ..=
			{
				const auto operatorName = C4ScriptOpMap[pCPos->bccX].Identifier;
				CheckOpPars<C4V_Any, C4V_Any, false, false>(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1;
				C4Value *pPar2 = pCurVal;
				const auto assignmentOperator = pCPos->bccType == AB_ConcatIt;
				if (assignmentOperator) pPar1 = &pPar1->GetRefVal();
				switch (pPar1->GetType())
				{
					case C4V_Map:
					{
						CheckOpPar(pPar2, C4V_Map, operatorName, " right side");
						const auto lhsMap = pPar1->_getMap();
						// copy if necessary
						const auto lhs = !assignmentOperator ? static_cast<C4ValueHash *>(lhsMap->IncRef()->IncElementRef()) : lhsMap;
						for (const auto &[key, value] : *pPar2->_getMap())
							(*lhs)[key] = value;

						if (!assignmentOperator)
						{
							pPar1->SetMap(lhs);
							lhs->DecElementRef();
							lhs->DecRef();
						}
						PopValue();
						break;
					}
					case C4V_Array:
					{
						CheckOpPar(pPar2, C4V_Array, operatorName, " right side");
						const auto lhsSize = pPar1->_getArray()->GetSize();
						const auto rhsSize = pPar2->_getArray()->GetSize();
						pPar1->SetArrayLength(lhsSize + rhsSize, pCurCtx);
						auto &lhs = *pPar1->_getArray();
						auto &rhs = *pPar2->_getArray();

						for (auto i = 0; i < rhsSize; ++i)
							lhs[lhsSize + i] = rhs[i];

						PopValue();
						break;
					}
					default:
					{
						auto par1String = pPar1->toString();
						if (!par1String)
						{
							throw C4AulExecError(pCurCtx->Obj, FormatString("operator \"%s\" left side: can not convert \"%s\" to \"string\", \"array\" or \"map\"!", operatorName, GetC4VName(pPar1->GetType())).getData());
						}
						auto par2String = pPar2->toString();
						if (!par2String)
						{
							throw C4AulExecError(pCurCtx->Obj, FormatString("operator \"%s\" right side: can not convert \"%s\" to \"string\"!", operatorName, GetC4VName(pPar2->GetType())).getData());
						}

						par1String->Append(*par2String);
						pPar1->SetString(new C4String(std::move(*par1String), &pCurCtx->Func->Owner->GetEngine()->Strings));
						PopValue();
						break;
					}
				}
				break;
			}
			case AB_EqualIdent: // old ==
			{
				CheckOpPars(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetBool(pPar1->Equals(*pPar2, C4AulScriptStrict::NONSTRICT));
				PopValue();
				break;
			}
			case AB_Equal: // new ==
			{
				CheckOpPars(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetBool(pPar1->Equals(*pPar2, pCurCtx->Func->pOrgScript->Strict));
				PopValue();
				break;
			}
			case AB_NotEqualIdent: // old !=
			{
				CheckOpPars(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetBool(!pPar1->Equals(*pPar2, C4AulScriptStrict::NONSTRICT));
				PopValue();
				break;
			}
			case AB_NotEqual: // new !=
			{
				CheckOpPars(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetBool(!pPar1->Equals(*pPar2, pCurCtx->Func->pOrgScript->Strict));
				PopValue();
				break;
			}
			case AB_SEqual: // S=, eq
			{
				CheckOpPars(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetBool(SEqual(pPar1->_getStr() ? pPar1->_getStr()->Data.getData() : "",
					pPar2->_getStr() ? pPar2->_getStr()->Data.getData() : ""));
				PopValue();
				break;
			}
			case AB_SNEqual: // ne
			{
				CheckOpPars(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetBool(!SEqual(pPar1->_getStr() ? pPar1->_getStr()->Data.getData() : "",
					pPar2->_getStr() ? pPar2->_getStr()->Data.getData() : ""));
				PopValue();
				break;
			}
			case AB_BitAnd: // &
			{
				CheckOpPars<C4V_Any, C4V_Any, false, false>(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetInt(pPar1->_getInt() & pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_BitXOr: // ^
			{
				CheckOpPars<C4V_Any, C4V_Any, false, false>(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetInt(pPar1->_getInt() ^ pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_BitOr: // |
			{
				CheckOpPars<C4V_Any, C4V_Any, false, false>(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetInt(pPar1->_getInt() | pPar2->_getInt());
				PopValue();
				break;
			}
			case AB_And: // &&
			{
				CheckOpPars(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetBool(pPar1->_getRaw() && pPar2->_getRaw());
				PopValue();
				break;
			}
			case AB_Or: // ||
			{
				CheckOpPars(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->SetBool(pPar1->_getRaw() || pPar2->_getRaw());
				PopValue();
				break;
			}

			case AB_PowIt: // **=
			{
				CheckOpPars<C4V_Int, C4V_Any, false, false>(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->GetData().Int = Pow(pPar1->GetData().Int, pPar2->_getInt());
				pPar1->HintType(C4V_Int);
				PopValue();
				break;
			}
			case AB_MulIt: // *=
			{
				CheckOpPars<C4V_Int, C4V_Any, false, false>(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->GetData().Int *= pPar2->_getInt();
				pCurVal->HintType(C4V_Int);
				PopValue();
				break;
			}
			case AB_DivIt: // /=
			{
				CheckOpPars<C4V_Int, C4V_Any, false, false>(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->GetData().Int = pPar2->_getInt() ? pPar1->GetData().Int / pPar2->_getInt() : 0;
				pPar1->HintType(C4V_Int);
				PopValue();
				break;
			}
			case AB_ModIt: // %=
			{
				CheckOpPars<C4V_Int, C4V_Any, false, false>(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->GetData().Int = pPar2->_getInt() ? pPar1->GetData().Int % pPar2->_getInt() : 0;
				pPar1->HintType(C4V_Int);
				PopValue();
				break;
			}
			case AB_Inc: // +=
			{
				CheckOpPars<C4V_Int, C4V_Any, false, false>(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->GetData().Int += pPar2->_getInt();
				pPar1->HintType(C4V_Int);
				PopValue();
				break;
			}
			case AB_Dec: // -=
			{
				CheckOpPars<C4V_Int, C4V_Any, false, false>(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->GetData().Int -= pPar2->_getInt();
				pPar1->HintType(C4V_Int);
				PopValue();
				break;
			}
			case AB_LeftShiftIt: // <<=
			{
				CheckOpPars<C4V_Int, C4V_Any, false, false>(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->GetData().Int <<= pPar2->_getInt();
				pPar1->HintType(C4V_Int);
				PopValue();
				break;
			}
			case AB_RightShiftIt: // >>=
			{
				CheckOpPars<C4V_Int, C4V_Any, false, false>(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->GetData().Int >>= pPar2->_getInt();
				pPar1->HintType(C4V_Int);
				PopValue();
				break;
			}
			case AB_AndIt: // &=
			{
				CheckOpPars<C4V_Int, C4V_Any, false, false>(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->GetData().Int &= pPar2->_getInt();
				pPar1->HintType(C4V_Int);
				PopValue();
				break;
			}
			case AB_OrIt: // |=
			{
				CheckOpPars<C4V_Int, C4V_Any, false, false>(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->GetData().Int |= pPar2->_getInt();
				pPar1->HintType(C4V_Int);
				PopValue();
				break;
			}
			case AB_XOrIt: // ^=
			{
				CheckOpPars<C4V_Int, C4V_Any, false, false>(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				pPar1->GetData().Int ^= pPar2->_getInt();
				pPar1->HintType(C4V_Int);
				PopValue();
				break;
			}
			case AB_Set: // =
			{
				CheckOpPars(pCPos->bccX);
				C4Value *pPar1 = pCurVal - 1, *pPar2 = pCurVal;
				*pPar1 = *pPar2;
				PopValue();
				break;
			}
			case AB_ARRAY:
			{
				// Create array
				C4ValueArray *pArray = new C4ValueArray(pCPos->bccX);

				// Pop values from stack
				for (int i = 0; i < pCPos->bccX; i++)
					pArray->GetItem(i) = pCurVal[i - pCPos->bccX + 1];

				// Push array
				if (pCPos->bccX > 0)
				{
					PopValues(pCPos->bccX - 1);
					pCurVal->SetArray(pArray);
				}
				else
					PushArray(pArray);

				break;
			}

			case AB_MAP:
			{
				C4ValueHash *map = new C4ValueHash;
				for (int i = 0; i < pCPos->bccX; ++i)
				{
					(*map)[pCurVal[2 * (i - pCPos->bccX) + 1]] = pCurVal[2 * (i - pCPos->bccX) + 2];
				}

				if (pCPos->bccX > 0)
				{
					PopValues(2 * pCPos->bccX - 1);
					pCurVal->SetMap(map);
				}
				else
					PushMap(map);

				break;
			}

			case AB_ARRAYA_R: case AB_ARRAYA_V:
			{
				C4Value &Container = pCurVal[-1].GetRefVal();
				C4Value &Index = pCurVal[0];
				if (Container.GetType() == C4V_Any)
				{
					throw C4AulExecError(pCurCtx->Obj, "indexed access [index]: array, map or string expected, but got nil");
				}

				if (Container.ConvertTo(C4V_Map) || Container.ConvertTo(C4V_Array) || Container.ConvertTo(C4V_C4Object))
				{
					Container.GetContainerElement(&Index, pCurVal[-1], pCurCtx, pCPos->bccType == AB_ARRAYA_V);
					// Remove index
					PopValue();
					break;
				}


				if (Container.ConvertTo(C4V_String))
				{
					if (!Index.ConvertTo(C4V_Int))
						throw C4AulExecError(pCurCtx->Obj, FormatString("indexed string access: index of type %s, int expected!", Index.GetTypeName()).getData());

					auto index = Index._getInt();
					StdStrBuf &str = Container._getStr()->Data;
					if (index < 0)
					{
						index += checked_cast<C4ValueInt>(str.getLength());
					}

					if (index < 0 || static_cast<std::size_t>(index) >= str.getLength())
					{
						pCurVal[-1].Set0();
					}
					else
					{
						StdStrBuf result;
						result.AppendChar(str.getData()[index]);
						pCurVal[-1].SetString(new C4String(std::move(result), &pCurCtx->Func->Owner->GetEngine()->Strings));
					}
					PopValue();
					break;
				}
				else
					throw C4AulExecError(pCurCtx->Obj, FormatString("indexed access: can't access %s by index!", Container.GetTypeName()).getData());
			}

			case AB_MAPA_R: case AB_MAPA_V:
			{
				C4Value &Map = pCurVal->GetRefVal();
				if (Map.GetType() == C4V_Any)
				{
					throw C4AulExecError(pCurCtx->Obj, "map access with .: map expected, but got nil!");
				}

				if (!Map.ConvertTo(C4V_Map) && !Map.ConvertTo(C4V_C4Object))
				{
					throw C4AulExecError(pCurCtx->Obj, FormatString("map access with .: map expected, but got \"%s\"!", GetC4VName(Map.GetType())).getData());
				}

				C4Value key(reinterpret_cast<C4String *>(pCPos->bccX));
				Map.GetContainerElement(&key, *pCurVal, pCurCtx, pCPos->bccType == AB_MAPA_V);

				break;
			}

			case AB_ARRAY_APPEND:
			{
				C4Value &Array = pCurVal[0].GetRefVal();
				// Typcheck
				if (!Array.ConvertTo(C4V_Array) || Array.GetType() != C4V_Array)
					throw C4AulExecError(pCurCtx->Obj, FormatString("array append accesss: can't access %s as an array!", Array.GetType() == C4V_Any ? "nil" : Array.GetTypeName()).getData());

				C4Value index = C4VInt(Array._getArray()->GetSize());
				Array.GetContainerElement(&index, pCurVal[0], pCurCtx);

				break;
			}

			case AB_DEREF:
				pCurVal[0].Deref();

			case AB_STACK:
				if (pCPos->bccX < 0)
					PopValues(-pCPos->bccX);
				else
					PushNullVals(pCPos->bccX);
				break;

			case AB_JUMP:
				fJump = true;
				pCPos += pCPos->bccX;
				break;

			case AB_JUMPAND:
				if (!pCurVal[0])
				{
					fJump = true;
					pCPos += pCPos->bccX;
				}
				else
				{
					PopValue();
				}
				break;

			case AB_JUMPOR:
				if (!!pCurVal[0])
				{
					fJump = true;
					pCPos += pCPos->bccX;
				}
				else
				{
					PopValue();
				}
				break;

			case AB_JUMPNIL:
				if (pCurVal[0].GetType() == C4V_Any)
				{
					pCurVal[0].Deref();
					fJump = true;
					pCPos += pCPos->bccX;
				}
				break;

			case AB_JUMPNOTNIL:
				if (pCurVal[0].GetType() != C4V_Any)
				{
					fJump = true;
					pCPos += pCPos->bccX;
				}
				else
				{
					PopValue();
				}
				break;

			case AB_CONDN:
				if (!pCurVal[0])
				{
					fJump = true;
					pCPos += pCPos->bccX;
				}
				PopValue();
				break;

			case AB_RETURN:
			{
				// Resolve reference
				if (!pCurCtx->Func->SFunc()->bReturnRef)
					pCurVal->Deref();

				// Trace
				if (iTraceStart >= 0)
				{
					StdStrBuf Buf("T");
					Buf.AppendChars('>', ContextStackSize() - iTraceStart);
					LogF("%s%s returned %s", Buf.getData(), pCurCtx->Func->Name, pCurVal->GetDataString().getData());
				}

				// External call?
				C4Value *pReturn = pCurCtx->Return;
				if (!pReturn)
				{
					// Get return value and stop executing.
					C4Value rVal = *pCurVal;
					PopValuesUntil(pCurCtx->Pars - 1);
					PopContext();
					return rVal;
				}

				// Save return value
				if (pCurVal != pReturn)
					pReturn->Set(*pCurVal);

				// Pop context
				PopContext();

				// Clear value stack, except return value
				PopValuesUntil(pReturn);

				// Jump back, continue.
				pCPos = pCurCtx->CPos + 1;
				fJump = true;

				break;
			}

			case AB_FUNC:
			{
				// Get function call data
				C4AulFunc *pFunc = reinterpret_cast<C4AulFunc *>(pCPos->bccX);

				if (C4AulScriptFunc *sfunc = pFunc->SFunc(); sfunc && sfunc->Access < sfunc->pOrgScript->GetAllowedAccess(pFunc, pCurCtx->Func->pOrgScript))
				{
					throw C4AulExecError(pCurCtx->Obj, FormatString("Insufficient access level for function \"%s\"!", pFunc->Name).getData());
				}
				C4Value *pPars = pCurVal - pFunc->GetParCount() + 1;
				// Save current position
				pCurCtx->CPos = pCPos;
				// Do the call
				C4AulBCC *pJump = Call(pFunc, pPars, pPars, nullptr);
				if (pJump)
				{
					pCPos = pJump;
					fJump = true;
				}
				break;
			}

			case AB_VAR_R: case AB_VAR_V:
				if (!pCurVal->ConvertTo(C4V_Int))
					throw C4AulExecError(pCurCtx->Obj, FormatString("Var: index of type %s, int expected!", pCurVal->GetTypeName()).getData());
				// Push reference to variable on the stack
				if (pCPos->bccType == AB_VAR_R)
					pCurVal->SetRef(&pCurCtx->NumVars.GetItem(pCurVal->_getInt()));
				else
					pCurVal->Set(pCurCtx->NumVars.GetItem(pCurVal->_getInt()));
				break;

			case AB_PAR_R: case AB_PAR_V:
				if (!pCurVal->ConvertTo(C4V_Int))
					throw C4AulExecError(pCurCtx->Obj, FormatString("Par: index of type %s, int expected!", pCurVal->GetTypeName()).getData());
				// Push reference to parameter on the stack
				if (pCurVal->_getInt() >= 0 && pCurVal->_getInt() < static_cast<int>(pCurCtx->ParCnt()))
				{
					if (pCPos->bccType == AB_PAR_R)
						pCurVal->SetRef(&pCurCtx->Pars[pCurVal->_getInt()]);
					else
						pCurVal->Set(pCurCtx->Pars[pCurVal->_getInt()]);
				}
				else
					pCurVal->Set0();
				break;

			case AB_FOREACH_NEXT:
			{
				// This should always hold
				assert(pCurVal->ConvertTo(C4V_Int));
				int iItem = pCurVal->_getInt();
				// Check array the first time only
				if (!iItem)
				{
					if (!pCurVal[-1].ConvertTo(C4V_Array))
						throw C4AulExecError(pCurCtx->Obj, FormatString("for: array expected, but got %s!", pCurVal[-1].GetTypeName()).getData());
					if (!pCurVal[-1]._getArray())
						throw C4AulExecError(pCurCtx->Obj, FormatString("for: array expected, but got nil!").getData());
				}
				C4ValueArray *pArray = pCurVal[-1]._getArray();
				// No more entries?
				if (pCurVal->_getInt() >= pArray->GetSize())
					break;
				// Get next
				pCurCtx->Vars[pCPos->bccX] = pArray->GetItem(iItem);
				// Save position
				pCurVal->SetInt(iItem + 1);
				// Jump over next instruction
				pCPos += 2;
				fJump = true;
				break;
			}

			case AB_FOREACH_MAP_NEXT:
			{
				// This should always hold
				assert(pCurVal[-1].ConvertTo(C4V_Int));
				using Iterator = C4ValueHash::Iterator;
				auto iterator = reinterpret_cast<Iterator *>(pCurVal[0]._getRef());
				// Check map the first time only
				if (!iterator)
				{
					if (!pCurVal[-2].ConvertTo(C4V_Map))
						throw C4AulExecError(pCurCtx->Obj, FormatString("for: map expected, but got %s!", pCurVal[-1].GetTypeName()).getData());
					if (!pCurVal[-2]._getMap())
						throw C4AulExecError(pCurCtx->Obj, FormatString("for: map expected, but got nil!").getData());
				}
				C4ValueHash *map = pCurVal[-2]._getMap();
				if (!iterator)
				{
					iterator = new Iterator (map->begin());
					pCurVal[0].SetInt(1);
					pCurVal[0].GetData().Ref = reinterpret_cast<C4Value *>(iterator);
				}
				// No more entries?
				if (*iterator == map->end())
				{
					delete iterator;
					break;
				}
				// Get next
				pCurCtx->Vars[pCPos->bccX] = (**iterator).first;
				pCurCtx->Vars[pCurVal[-1]._getInt()] = (**iterator).second;

				++(*iterator);
				// Jump over next instruction
				pCPos += 2;
				fJump = true;
				break;
			}

			case AB_IVARN:
				pCurCtx->Vars[pCPos->bccX] = pCurVal[0];
				PopValue();
				break;

			case AB_CALLNS:
				// Ignore. TODO: Fix this.
				break;

			case AB_CALL:
			case AB_CALLFS:
			case AB_CALLGLOBAL:
			{
				const auto isGlobal = pCPos->bccType == AB_CALLGLOBAL;
				C4Value *pPars = pCurVal - C4AUL_MAX_Par + 1;
				C4Value *pTargetVal = pCurVal - C4AUL_MAX_Par;

				// Check for call to null
				if (!isGlobal && !*pTargetVal)
					throw C4AulExecError(pCurCtx->Obj, "Object call: target is zero!");

				// Get call target - "object" or "id" are allowed
				C4Object *pDestObj{}; C4Def *pDestDef{};
				if (!isGlobal)
				{
					if (pTargetVal->ConvertTo(C4V_C4Object))
					{
						// object call
						pDestObj = pTargetVal->_getObj();
						pDestDef = pDestObj->Def;
					}
					else if (pTargetVal->ConvertTo(C4V_C4ID))
					{
						// definition call
						pDestObj = nullptr;
						pDestDef = C4Id2Def(pTargetVal->_getC4ID());
						// definition must be known
						if (!pDestDef)
							throw C4AulExecError(pCurCtx->Obj,
								FormatString("Definition call: Definition for id %s not found!", C4IdText(pTargetVal->_getC4ID())).getData());
					}
					else
						throw C4AulExecError(pCurCtx->Obj,
							FormatString("Object call: Invalid target type %s, expected object or id!", pTargetVal->GetTypeName()).getData());
				}

				// Resolve overloads
				C4AulFunc *pFunc = reinterpret_cast<C4AulFunc *>(pCPos->bccX);
				while (pFunc->OverloadedBy)
					pFunc = pFunc->OverloadedBy;

				if (!isGlobal)
				{
					// Search function for given context
					pFunc = pFunc->FindSameNameFunc(pDestDef);
					if (!pFunc && pCPos->bccType == AB_CALLFS)
					{
						PopValuesUntil(pTargetVal);
						pTargetVal->Set0();
						break;
					}
				}

				// Function not found?
				if (!pFunc)
				{
					const char *szFuncName = reinterpret_cast<C4AulFunc *>(pCPos->bccX)->Name;
					if (pDestObj)
						throw C4AulExecError(pCurCtx->Obj,
							FormatString("Object call: No function \"%s\" in object \"%s\"!", szFuncName, pTargetVal->GetDataString().getData()).getData());
					else
						throw C4AulExecError(pCurCtx->Obj,
							FormatString("Definition call: No function \"%s\" in definition \"%s\"!", szFuncName, pDestDef->Name.getData()).getData());
				}

				else if (C4AulScriptFunc *sfunc = pFunc->SFunc(); sfunc)
				{
					C4AulScript *script = sfunc->pOrgScript;
					if (sfunc->Access < script->GetAllowedAccess(pFunc, sfunc->pOrgScript))
					{
						throw C4AulExecError(pCurCtx->Obj, FormatString("Insufficient access level for function \"%s\"!", pFunc->Name).getData());
					}
				}

				// Save function back (optimization)
				pCPos->bccX = reinterpret_cast<std::intptr_t>(pFunc);

				// Save current position
				pCurCtx->CPos = pCPos;

				// Call function
				C4AulBCC *pNewCPos = Call(pFunc, pTargetVal, pPars, pDestObj, pDestDef, isGlobal);
				if (pNewCPos)
				{
					// Jump
					pCPos = pNewCPos;
					fJump = true;
				}

				break;
			}

			default:
			case AB_NilCoalescing:
				assert(false);
			}

			// Continue
			if (!fJump)
				pCPos++;
		}
	}
	catch (const C4AulError &e)
	{
		// Save current position
		pOldCtx->CPos = pCPos;
		// Pass?
		if (fPassErrors)
			throw;
		// Show
		e.show();
		// Trace
		for (C4AulScriptContext *pCtx = pCurCtx; pCtx >= Contexts; pCtx--)
			pCtx->dump(StdStrBuf(" by: "));
		// Unwind stack
		C4Value *pUntil = nullptr;
		while (pCurCtx >= pOldCtx)
		{
			pUntil = pCurCtx->Pars - 1;
			PopContext();
		}
		if (pUntil)
			PopValuesUntil(pUntil);
	}

	// Return nothing
	return C4VNull;
}

C4AulBCC *C4AulExec::Call(C4AulFunc *pFunc, C4Value *pReturn, C4Value *pPars, C4Object *pObj, C4Def *pDef, bool globalContext)
{
	// No object given? Use current context
	if (globalContext)
	{
		pObj = nullptr;
		pDef = nullptr;
	}
	else if (!pObj && !pDef)
	{
		assert(pCurCtx >= Contexts);
		pObj = pCurCtx->Obj;
		pDef = pCurCtx->Def;
	}

	// Script function?
	C4AulScriptFunc *pSFunc = pFunc->SFunc();

	const bool convertToAnyEagerly = pCurCtx->Func->pOrgScript->Strict < C4AulScriptStrict::STRICT3;

	const bool convertNilToIntBool = convertToAnyEagerly && pSFunc && pSFunc->pOrgScript->Strict >= C4AulScriptStrict::STRICT3;

	// Convert parameters (typecheck)
	const auto pTypes = pFunc->GetParType();
	for (int i = 0; i < pFunc->GetParCount(); i++)
	{
		if (convertToAnyEagerly && pTypes[i] != C4V_pC4Value && !pPars[i])
		{
			pPars[i].Set0();
		}
		if (!pPars[i].ConvertTo(pTypes[i]))
			throw C4AulExecError(pCurCtx->Obj,
				FormatString("call to \"%s\" parameter %d: got \"%s\", but expected \"%s\"!",
					pFunc->Name, i + 1, pPars[i].GetTypeName(), GetC4VName(pTypes[i])
				).getData());

		if (convertNilToIntBool && pPars[i].GetType() == C4V_Any)
		{
			if (pTypes[i] == C4V_Int)
			{
				pPars[i].SetInt(0);
			}
			else if (pTypes[i] == C4V_Bool)
			{
				pPars[i].SetBool(false);
			}
		}
	}

	if (pSFunc)
	{
		// Push variables
		C4Value *pVars = pCurVal + 1;
		PushNullVals(pSFunc->VarNamed.iSize);

		// Check context
		assert(!pSFunc->Owner->Def || pDef == pSFunc->Owner->Def);

		// Push a new context
		C4AulScriptContext ctx;
		ctx.Obj = pObj;
		ctx.Def = pDef;
		ctx.Caller = pCurCtx;
		ctx.Return = pReturn;
		ctx.Pars = pPars;
		ctx.Vars = pVars;
		ctx.Func = pSFunc;
		ctx.TemporaryScript = false;
		ctx.CPos = nullptr;
		PushContext(ctx);

		// Jump to code
		return pSFunc->Code;
	}
	else
	{
		// Create new context
		C4AulContext CallCtx;
		CallCtx.Obj = pObj;
		CallCtx.Def = pDef;
		CallCtx.Caller = pCurCtx;

#ifdef DEBUGREC_SCRIPT
		if (Game.FrameCounter >= DEBUGREC_START_FRAME)
		{
			StdStrBuf sCallText;
			if (pObj)
				sCallText.AppendFormat("Object(%d): ", pObj->Number);
			sCallText.Append(pFunc->Name);
			sCallText.AppendChar('(');
			for (int i = 0; i < C4AUL_MAX_Par; ++i)
			{
				if (i) sCallText.AppendChar(',');
				C4Value &rV = pPars[i];
				if (rV.GetType() == C4V_String)
				{
					C4String *s = rV.getStr();
					if (!s)
						sCallText.Append("(Snull)");
					else
					{
						sCallText.Append("\"");
						sCallText.Append(s->Data);
						sCallText.Append("\"");
					}
				}
				else
					sCallText.Append(rV.GetDataString());
			}
			sCallText.AppendChar(')');
			sCallText.AppendChar(';');
			AddDbgRec(RCT_AulFunc, sCallText.getData(), sCallText.getLength() + 1);
		}
#endif
		// Execute
#ifdef _DEBUG
		C4AulScriptContext *pCtx = pCurCtx;
#endif
		if (pReturn > pCurVal)
			PushValue(pFunc->Exec(&CallCtx, pPars, true));
		else
			pReturn->Set(pFunc->Exec(&CallCtx, pPars, true));
#ifdef _DEBUG
		assert(pCtx == pCurCtx);
#endif

		// Remove parameters from stack
		PopValuesUntil(pReturn);

		// Continue
		return nullptr;
	}
}

void C4AulStartTrace()
{
	AulExec.StartTrace();
}

void C4AulExec::StartTrace()
{
	if (iTraceStart < 0)
		iTraceStart = ContextStackSize();
}

void C4AulExec::StartProfiling(C4AulScript *pProfiledScript)
{
	// stop previous profiler run
	if (fProfiling) AbortProfiling();
	fProfiling = true;
	// resets profling times and starts recording the times
	this->pProfiledScript = pProfiledScript;
	time_t tNow = timeGetTime();
	tDirectExecStart = tNow; // in case profiling is started from DirectExec
	tDirectExecTotal = 0;
	pProfiledScript->ResetProfilerTimes();
	for (C4AulScriptContext *pCtx = Contexts; pCtx <= pCurCtx; ++pCtx)
		pCtx->tTime = tNow;
}

void C4AulExec::StopProfiling()
{
	// stop the profiler and displays results
	if (!fProfiling) return;
	fProfiling = false;
	// collect profiler times
	C4AulProfiler Profiler;
	Profiler.CollectEntry(nullptr, tDirectExecTotal);
	pProfiledScript->CollectProfilerTimes(Profiler);
	Profiler.Show();
}

void C4AulProfiler::StartProfiling(C4AulScript *pScript)
{
	AulExec.StartProfiling(pScript);
}

void C4AulProfiler::StopProfiling()
{
	AulExec.StopProfiling();
}

void C4AulProfiler::Abort()
{
	AulExec.AbortProfiling();
}

void C4AulProfiler::CollectEntry(C4AulScriptFunc *pFunc, time_t tProfileTime)
{
	// zero entries are not collected to have a cleaner list
	if (!tProfileTime) return;
	// add entry to list
	Entry e;
	e.pFunc = pFunc;
	e.tProfileTime = tProfileTime;
	Times.push_back(e);
}

void C4AulProfiler::Show()
{
	// sort by time
	std::sort(Times.rbegin(), Times.rend());
	// display them
	Log("Profiler statistics:");
	Log("==============================");
	typedef std::vector<Entry> EntryList;
	for (EntryList::iterator i = Times.begin(); i != Times.end(); ++i)
	{
		Entry &e = (*i);
		LogF("%05dms\t%s", static_cast<int>(e.tProfileTime), e.pFunc ? (e.pFunc->GetFullName().getData()) : "Direct exec");
	}
	Log("==============================");
	// done!
}

C4Value C4AulFunc::Exec(C4Object *pObj, const C4AulParSet &pPars, bool fPassErrors)
{
	// construct a dummy caller context
	C4AulContext ctx;
	ctx.Obj = pObj;
	ctx.Def = pObj ? pObj->Def : nullptr;
	ctx.Caller = nullptr;
	// execute
	return Exec(&ctx, pPars.Par, fPassErrors);
}

C4Value C4AulScriptFunc::Exec(C4AulContext *pCtx, const C4Value pPars[], bool fPassErrors)
{
#ifdef C4ENGINE

	// handle easiest case first
	if (Owner->State != ASS_PARSED) return C4VNull;

	// execute
	return AulExec.Exec(this, pCtx->Obj, pPars, fPassErrors);

#else

	return C4AulNull;

#endif
}

C4Value C4AulScriptFunc::Exec(C4Object *pObj, const C4AulParSet &pPars, bool fPassErrors)
{
#ifdef C4ENGINE

	// handle easiest case first
	if (Owner->State != ASS_PARSED) return C4VNull;

	// execute
	return AulExec.Exec(this, pObj, pPars.Par, fPassErrors);

#else

	return C4AulNull;

#endif
}

C4Value C4AulScript::DirectExec(C4Object *pObj, const char *szScript, const char *szContext, bool fPassErrors, C4AulScriptStrict Strict)
{
#ifdef DEBUGREC_SCRIPT
	AddDbgRec(RCT_DirectExec, szScript, strlen(szScript) + 1);
	int32_t iObjNumber = pObj ? pObj->Number : -1;
	AddDbgRec(RCT_DirectExec, &iObjNumber, sizeof(int32_t));
#endif
	// profiler
	AulExec.StartDirectExec();
	// Create a new temporary script as child of this script
	C4AulScript *pScript = new C4AulScript();
	pScript->Script.Copy(szScript);
	pScript->ScriptName = FormatString("%s in %s", szContext, ScriptName.getData());
	pScript->Strict = Strict;
	pScript->Temporary = true;
	pScript->State = ASS_LINKED;
	if (pObj)
	{
		pScript->Def = pObj->Def;
		pScript->LocalNamed = pObj->Def->Script.LocalNamed;
	}
	else
	{
		pScript->Def = nullptr;
	}
	pScript->Reg2List(Engine, this);
	// Add a new function
	C4AulScriptFunc *pFunc = new C4AulScriptFunc(pScript, "");
	pFunc->Script = pScript->Script.getData();
	pFunc->pOrgScript = pScript;
	// Parse function
	try
	{
		pScript->ParseFn(pFunc, true);
	}
	catch (const C4AulError &ex)
	{
		ex.show();
		delete pFunc;
		delete pScript;
		return C4VNull;
	}
	pFunc->Code = pScript->Code;
	pScript->State = ASS_PARSED;
	// Execute. The TemporaryScript-parameter makes sure the script will be deleted later on.
	C4Value vRetVal(AulExec.Exec(pFunc, pObj, nullptr, fPassErrors, true));
	// profiler
	AulExec.StopDirectExec();
	return vRetVal;
}

void C4AulScript::ResetProfilerTimes()
{
	// zero all profiler times of owned functions
	C4AulScriptFunc *pSFunc;
	for (C4AulFunc *pFn = Func0; pFn; pFn = pFn->Next)
		if (pSFunc = pFn->SFunc())
			pSFunc->tProfileTime = 0;
	// reset sub-scripts
	for (C4AulScript *pScript = Child0; pScript; pScript = pScript->Next)
		pScript->ResetProfilerTimes();
}

void C4AulScript::CollectProfilerTimes(class C4AulProfiler &rProfiler)
{
	// collect all profiler times of owned functions
	C4AulScriptFunc *pSFunc;
	for (C4AulFunc *pFn = Func0; pFn; pFn = pFn->Next)
		if (pSFunc = pFn->SFunc())
			rProfiler.CollectEntry(pSFunc, pSFunc->tProfileTime);
	// collect sub-scripts
	for (C4AulScript *pScript = Child0; pScript; pScript = pScript->Next)
		pScript->CollectProfilerTimes(rProfiler);
}
