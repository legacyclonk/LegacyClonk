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

/* Console mode dialog for object properties and script interface */

#include <C4Include.h>
#include <C4PropertyDlg.h>

#include <C4Console.h>
#include <C4Application.h>
#include <C4Object.h>
#include <C4Wrappers.h>
#include <C4Player.h>

#include <StdRegistry.h>

#include "imgui.h"

C4PropertyDlg::C4PropertyDlg()
{
	Default();
}

C4PropertyDlg::~C4PropertyDlg()
{
	Clear();
}

void C4PropertyDlg::Open()
{
	Active = true;
}

void C4PropertyDlg::Update(C4ObjectList &rSelection)
{
	if (!Active) return;
	// Set new selection
	Selection.Copy(rSelection);
	// Update contents
	Update();
}

void C4PropertyDlg::Update()
{
	if (!Active) return;

	idSelectedDef = C4ID_None;

	// Compose info text by selected object(s)
	switch (Selection.ObjectCount())
	{
	// No selection
	case 0:
		selectionText.Ref(LoadResStr("IDS_CNS_NOOBJECT"));
		break;
	// One selected object
	case 1:
	{
		C4Object *cobj = Selection.GetObject();
		// Type
		selectionText.Format(LoadResStr("IDS_CNS_TYPE"), cobj->GetName(), C4IdText(cobj->Def->id));
		// Owner
		if (ValidPlr(cobj->Owner))
		{
			selectionText.Append(LineFeed);
			selectionText.AppendFormat(LoadResStr("IDS_CNS_OWNER"), Game.Players.Get(cobj->Owner)->GetName());
		}
		// Contents
		if (cobj->Contents.ObjectCount())
		{
			selectionText.Append(LineFeed);
			selectionText.Append(LoadResStr("IDS_CNS_CONTENTS"));
			selectionText.Append(static_cast<const StdStrBuf &>(cobj->Contents.GetNameList(Game.Defs)));
		}
		// Action
		if (cobj->Action.Act != ActIdle)
		{
			selectionText.Append(LineFeed);
			selectionText.Append(LoadResStr("IDS_CNS_ACTION"));
			selectionText.Append(cobj->Def->ActMap[cobj->Action.Act].Name);
		}
		// Locals
		int cnt; bool fFirstLocal = true;
		for (cnt = 0; cnt < cobj->Local.GetSize(); cnt++)
			if (cobj->Local[cnt])
			{
				// Header
				if (fFirstLocal) { selectionText.Append(LineFeed); selectionText.Append(LoadResStr("IDS_CNS_LOCALS")); fFirstLocal = false; }
				selectionText.Append(LineFeed);
				// Append id
				selectionText.AppendFormat(" Local(%d) = ", cnt);
				// write value
				selectionText.Append(static_cast<const StdStrBuf &>(cobj->Local[cnt].GetDataString()));
			}
		// Locals (named)
		for (cnt = 0; cnt < cobj->LocalNamed.GetAnzItems(); cnt++)
		{
			// Header
			if (fFirstLocal) { selectionText.Append(LineFeed); selectionText.Append(LoadResStr("IDS_CNS_LOCALS")); fFirstLocal = false; }
			selectionText.Append(LineFeed);
			// Append name
			selectionText.AppendFormat(" %s = ", cobj->LocalNamed.pNames->pNames[cnt]);
			// write value
			selectionText.Append(static_cast<const StdStrBuf &>(cobj->LocalNamed.pData[cnt].GetDataString()));
		}
		// Effects
		for (C4Effect *pEffect = cobj->pEffects; pEffect; pEffect = pEffect->pNext)
		{
			// Header
			if (pEffect == cobj->pEffects)
			{
				selectionText.Append(LineFeed);
				selectionText.Append(LoadResStr("IDS_CNS_EFFECTS"));
			}
			selectionText.Append(LineFeed);
			// Effect name
			selectionText.AppendFormat(" %s: Interval %d", pEffect->Name, pEffect->iIntervall);
		}
		// Store selected def
		idSelectedDef = cobj->id;
		break;
	}
	// Multiple selected objects
	default:
		selectionText.Format(LoadResStr("IDS_CNS_MULTIPLEOBJECTS"), Selection.ObjectCount());
		break;
	}
}

void C4PropertyDlg::Default()
{
	Active = false;
	idSelectedDef = C4ID_None;
	Selection.Default();
	selectedFunction = nullptr;
}

void C4PropertyDlg::Clear()
{
	Selection.Clear();
	selectionText.Clear();
	Active = false;
}

void C4PropertyDlg::Execute()
{
	if (!Tick35) Update();
}

void C4PropertyDlg::ClearPointers(C4Object *pObj)
{
	Selection.ClearPointers(pObj);
}

void C4PropertyDlg::Draw()
{
	if (!Active) return;

	ImGui::Begin(LoadResStr("IDS_DLG_PROPERTIES"), &Active);

	if (ImGui::BeginChild("##properties", {}, true))
	{
		ImGui::TextWrapped("%s", selectionText.isNull() ? "" : selectionText.getData());
		ImGui::EndChild();
	}

	const auto addFunctionEntry = [this](C4AulFunc *const func)
	{
		if (ImGui::Selectable(std::string{func->Name}.append("()").c_str(), selectedFunction == func))
		{
			selectedFunction = func;
		}
	};

	if (ImGui::BeginCombo("##maininput", selectedFunction ? selectedFunction->Name : nullptr))
	{
		// Add global and standard functions
		for (C4AulFunc *func{Game.ScriptEngine.GetFirstFunc()}; func; func = Game.ScriptEngine.GetNextFunc(func))
		{
			if (func->GetPublic())
			{
				addFunctionEntry(func);
			}
		}

		// Add scenario script functions
		C4AulScriptFunc *func;
		if (C4Object *const obj{Selection.GetObject()}; obj)
		{
			for (std::int32_t i{0}; (func = obj->Def->Script.GetSFunc(i)); ++i)
			{
				// Public functions only
				if (func->Access == AA_PUBLIC)
				{
					addFunctionEntry(func);
				}
			}
		}

		ImGui::EndCombo();
	}

	ImGui::SameLine();

	ImGui::Button("OK");

	ImGui::End();

	if (!Active)
	{
		Clear();
	}
}
