/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
 * Copyright (c) 2017-2021, The LegacyClonk Team and contributors
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

// links aul scripts; i.e. resolves includes & appends, etc

#include <C4Include.h>
#include <C4Aul.h>

#include <C4Def.h>
#include <C4Game.h>
#include <C4Log.h>

// ResolveAppends and ResolveIncludes must be called both
// for each script. ResolveAppends has to be called first!
bool C4AulScript::ResolveAppends(C4DefList *rDefs)
{
	// resolve children appends
	for (C4AulScript *s = Child0; s; s = s->Next) s->ResolveAppends(rDefs);
	// resolve local appends
	if (State != ASS_PREPARSED) return false;
	for (const auto [a, nowarn] : Appends)
	{
		if (a != static_cast<C4ID>(-1))
		{
			C4Def *Def = rDefs->ID2Def(a);
			if (Def)
				AppendTo(Def->Script, true);
			else if (!nowarn)
			{
				// save id in buffer because AulWarn will use the buffer of C4IdText
				// to get the id of the object in which the error occurs...
				// (stupid static buffers...)
				char strID[5]; *strID = 0;
				strcpy(strID, C4IdText(a));
				Warn("script to #appendto not found: ", strID);
			}
		}
		else
		{
			// append to all defs
			for (std::size_t i{0}; const auto pDef = rDefs->GetDef(i); ++i)
			{
				if (pDef == Def) continue;
				// append
				AppendTo(pDef->Script, true);
			}
		}
	}
	return true;
}

bool C4AulScript::ResolveIncludes(C4DefList *rDefs)
{
	// resolve children includes
	for (C4AulScript *s = Child0; s; s = s->Next) s->ResolveIncludes(rDefs);
	// Had been preparsed?
	if (State != ASS_PREPARSED) return false;
	// has already been resolved?
	if (IncludesResolved) return true;
	// catch circular includes
	if (Resolving)
	{
		C4AulParseError(this, "Circular include chain detected - ignoring all includes!").show();
		IncludesResolved = true;
		State = ASS_LINKED;
		return false;
	}
	Resolving = true;
	// append all includes to local script
	for (const auto [i, nowarn] : Includes)
	{
		C4Def *Def = rDefs->ID2Def(i);
		if (Def)
		{
			// resolve #includes in included script first (#include-chains :( )
			if (!(static_cast<C4AulScript &>(Def->Script)).IncludesResolved)
				if (!Def->Script.ResolveIncludes(rDefs))
					continue; // skip this #include

			Def->Script.AppendTo(*this, false);
		}
		else if (!nowarn)
		{
			// save id in buffer because AulWarn will use the buffer of C4IdText
			// to get the id of the object in which the error occurs...
			// (stupid static buffers...)
			char strID[5]; *strID = 0;
			strcpy(strID, C4IdText(i));
			Warn("script to #include not found: ", strID);
		}
	}
	IncludesResolved = true;
	// includes/appends are resolved now (for this script)
	Resolving = false;
	State = ASS_LINKED;
	return true;
}

void C4AulScript::AppendTo(C4AulScript &Scr, bool bHighPrio)
{
	// definition appends
	if (Def && Scr.Def) Scr.Def->IncludeDefinition(Def);
	// append all funcs
	// (in reverse order if inserted at begin of list)
	C4AulScriptFunc *sf;
	for (C4AulFunc *f = bHighPrio ? Func0 : FuncL; f; f = bHighPrio ? f->Next : f->Prev)
		// script funcs only
		if (sf = f->SFunc())
			// no need to append global funcs
			if (sf->Access != AA_GLOBAL)
			{
				// append: create copy
				// (if high priority, insert at end, otherwise at the beginning)
				C4AulScriptFunc *sfc = new C4AulScriptFunc(&Scr, sf->Name, bHighPrio);
				sfc->CopyBody(*sf);
				// link the copy to a local function
				if (sf->LinkedTo)
				{
					sfc->LinkedTo = sf->LinkedTo;
					sf->LinkedTo = sfc;
				}
				else
				{
					sfc->LinkedTo = sf;
					sf->LinkedTo = sfc;
				}
			}
	// mark as linked
	// increase code size needed
	Scr.CodeSize += CodeSize + 1;
	// append all local vars (if any existing)
	assert(!Def || this == &Def->Script);
	assert(!Scr.Def || &Scr.Def->Script == &Scr);
	if (LocalNamed.iSize == 0)
		return;
	if (!Scr.Def)
	{
		Warn("could not append local variables to global script!", "");
		return;
	}
	// copy local var definitions
	for (int ivar = 0; ivar < LocalNamed.iSize; ivar++)
		Scr.LocalNamed.AddName(LocalNamed.pNames[ivar]);
}

void C4AulScript::UnLink()
{
	// unlink children
	for (C4AulScript *s = Child0; s; s = s->Next) s->UnLink();

	// do not unlink temporary (e.g., DirectExec-script in ReloadDef)
	if (Temporary) return;

	// check if byte code needs to be freed
	delete[] Code; Code = nullptr;

	// delete included/appended functions
	C4AulFunc *pFunc = Func0;
	while (pFunc)
	{
		C4AulFunc *pNextFunc = pFunc->Next;

		// clear stuff that's set in AfterLink
		pFunc->UnLink();

		if (pFunc->SFunc()) if (pFunc->Owner != pFunc->SFunc()->pOrgScript)
			if (!pFunc->LinkedTo || pFunc->LinkedTo->SFunc()) // do not kill global links; those will be deleted if corresponding sfunc in script is deleted
				delete pFunc;

		pFunc = pNextFunc;
	}
	// includes will have to be re-resolved now
	IncludesResolved = false;

	if (State > ASS_PREPARSED) State = ASS_PREPARSED;
}

void C4AulScriptFunc::UnLink()
{
	OwnerOverloaded = nullptr;

	// clear desc information, ParseDesc will set these later on
	idImage = C4ID_None;
	iImagePhase = 0;
	Condition = nullptr;
	ControlMethod = C4AUL_ControlMethod_All;

	C4AulFunc::UnLink();
}

void C4AulScript::AfterLink()
{
	// for all funcs: search functions that have the same name in
	// the whole script tree (for great fast direct object call)
	for (C4AulFunc *Func = Func0; Func; Func = Func->Next)
		// same-name ring not yet build for this function name?
		if (!Func->NextSNFunc && !Func->OverloadedBy)
		{
			// init
			Func->NextSNFunc = Func;
			// search complete tree for functions with same name
			// (expect all scripts "behind" this point to be already checked
			//  - so after-link calls for childs must be done after this).
			C4AulScript *pPos = this;
			while (pPos)
			{
				// has children? go down in hierarchy
				if (pPos->Child0)
					pPos = pPos->Child0;
				else
				{
					// last child? go up in hierarchy
					while (!pPos->Next && pPos->Owner)
						pPos = pPos->Owner;
					// next node
					pPos = pPos->Next;
				}
				if (!pPos) break;
				// has function with same name?
				C4AulFunc *pFn = pPos->GetFunc(Func->Name);
				if (pFn)
				{
					// resolve overloads
					while (pFn->OverloadedBy) pFn = pFn->OverloadedBy;
					// link
					pFn->NextSNFunc = Func->NextSNFunc;
					Func->NextSNFunc = pFn;
				}
			}
		}
	// call for childs
	for (C4AulScript *s = Child0; s; s = s->Next) s->AfterLink();
}

bool C4AulScript::ReloadScript(const char *szPath)
{
	// call for childs
	for (C4AulScript *s = Child0; s; s = s->Next)
		if (s->ReloadScript(szPath))
			return true;
	return false;
}

void C4AulScriptEngine::Link(C4DefList *rDefs)
{
	try
	{
		// resolve appends
		ResolveAppends(rDefs);

		// resolve includes
		ResolveIncludes(rDefs);

		// parse script funcs descs
		ParseDescs();

		// parse the scripts to byte code
		Parse();

		// engine is always parsed (for global funcs)
		State = ASS_PARSED;

		// get common funcs
		AfterLink();

		// non-strict scripts?
		if (nonStrictCnt)
		{
			// warn!
			// find first non-#strict script
			C4AulScript *pNonStrictScr = FindFirstNonStrictScript();
			if (pNonStrictScr)
				pNonStrictScr->Warn("using non-#strict syntax!", nullptr);
			else
			{
				Warn("non-#strict script detected, but def is lost", nullptr);
				Warn("please contact piracy@treffpunktclonk.net for further instructions", nullptr);
			}
			Warn(std::format("{} script{} use non-#strict syntax!", nonStrictCnt, (nonStrictCnt != 1 ? "s" : "")), nullptr);
		}

		// update material pointers
		Game.UpdateMaterialScriptPointers();

		// display state
		LogNTr(spdlog::level::info, "C4AulScriptEngine linked - {} line{}, {} warning{}, {} error{}",
			lineCnt, (lineCnt != 1 ? "s" : ""), warnCnt, (warnCnt != 1 ? "s" : ""), errCnt, (errCnt != 1 ? "s" : ""));

		// reset counters
		warnCnt = errCnt = nonStrictCnt = lineCnt = 0;
	}
	catch (const C4AulError &err)
	{
		// error??! show it!
		err.show();
	}
}

void C4AulScriptEngine::ReLink(C4DefList *rDefs)
{
	// unlink scripts
	UnLink();

	// unlink defs
	if (rDefs) rDefs->ResetIncludeDependencies();

	// clear string table
	Game.Script.UnLink();

	// re-link
	Link(rDefs);

	// update effect and material pointers
	Game.UpdateScriptPointers();
}

bool C4AulScriptEngine::ReloadScript(const char *szScript, C4DefList *pDefs)
{
	// reload
	if (!C4AulScript::ReloadScript(szScript))
		return false;
	// relink
	ReLink(pDefs);
	// ok
	return true;
}
